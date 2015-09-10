/*
 *      This file is part of facron.
 *
 *      Copyright 2012-2013 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
 *
 *      facron is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      facron is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with facron.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "facron-conf-entry.h"
#include "facron-lexer.h"
#include "facron-parser.h"

#include <string.h>
#include <unistd.h>

struct FacronParser
{
    FacronLexer     *lexer;
};

FacronConfEntry *
facron_parser_parse_entry (FacronParser    *parser,
                           FacronConfEntry *previous)
{
    if (!facron_lexer_read_line (parser->lexer))
        return NULL;

    if (facron_lexer_invalid_line (parser->lexer))
        goto fail_early;

    char *path = facron_lexer_read_string (parser->lexer);

    if (access (path, R_OK))
    {
        fprintf (stderr, "warning: No such file or directory: \"%s\"\n", path);
        free (path);
        goto fail_early;
    }

    facron_lexer_skip_spaces (parser->lexer);

    if (facron_lexer_end_of_line (parser->lexer))
    {
        fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
        goto fail_early;
    }

    FacronConfEntry *entry = facron_conf_entry_new (previous, path);

    int n = 0;
    FacronResult result;
    unsigned long long mask;

    while (n < 511 && (result = facron_lexer_next_token (parser->lexer, &mask))) /* != S_END */
    {
        switch (result)
        {
        case R_ERROR:
            goto fail;
        case R_COMMA:
            facron_conf_entry_apply_mask (entry, n++, mask);
            break;
        case R_PIPE:
            facron_conf_entry_apply_mask (entry, n, mask);
            break;
        default:
            break;
        }
    }

    facron_conf_entry_apply_mask (entry, n, mask);

    if (!n && !facron_conf_entry_validate (entry))
    {
        fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
        goto fail;
    }

    facron_lexer_skip_spaces (parser->lexer);

    for (n = 0; !facron_lexer_end_of_line (parser->lexer) && n < 511; ++n)
    {
        facron_conf_entry_add_command (entry, facron_lexer_read_string (parser->lexer));
        facron_lexer_skip_spaces (parser->lexer);
    }

    if (!n)
    {
        fprintf (stderr, "Error: no command line specified\n");
        goto fail;
    }

    return entry;

fail:
    facron_conf_entry_free (entry);
fail_early:
    return facron_parser_parse_entry (parser, previous);
}

bool
facron_parser_reload (FacronParser *parser)
{
    return facron_lexer_reload_file (parser->lexer);
}

void
facron_parser_free (FacronParser *parser)
{
    facron_lexer_free (parser->lexer);
    free (parser);
}

FacronParser *
facron_parser_new (const char *filename)
{
    FacronParser *parser = (FacronParser *) malloc (sizeof (FacronParser));

    parser->lexer = facron_lexer_new (filename);

    return parser;
}
