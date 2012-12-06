/*
 *      This file is part of facron.
 *
 *      Copyright 2012 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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
    FacronLexer *lexer;
    FacronConfEntry *previous_entry;
};

static inline char *
basename (const char *filename)
{
    return strdup (strrchr (filename, '/') + 1);
}

static inline char *
dirname (const char *filename)
{
    char *c = strrchr (filename, '/');
    while (c != filename && c[-1] == '/')
        --c;
    return (c == filename) ? strdup (".") :
                            (char *) memcpy (calloc (c - filename + 1, sizeof (char)), filename, c - filename);
}

FacronConfEntry *
facron_parser_parse_entry (FacronParser *parser)
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

    FacronConfEntry *entry = facron_conf_entry_new (parser->previous_entry, path);

    int n = 0;
    FacronResult result;
    unsigned long long mask;
    while ((result = facron_lexer_next_token (parser->lexer, &mask))) /* != S_END */
    {
        switch (result)
        {
        case R_ERROR:
            goto fail;
        case R_COMMA:
            entry->mask[n++] |= mask;
            break;
        case R_PIPE:
            entry->mask[n] |= mask;
            break;
        default:
            break;
        }
    }
    entry->mask[n] |= mask;

    if (n == 0 && !entry->mask[n])
    {
        fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
        goto fail;
    }

    n = 0;
    facron_lexer_skip_spaces (parser->lexer);
    while (!facron_lexer_end_of_line (parser->lexer) && n < 511)
    {
        char *tmp = facron_lexer_read_string (parser->lexer);
        entry->command[n++] = (!strcmp (tmp, "$$")) ? strdup (entry->path):
                              (!strcmp (tmp, "$@")) ? dirname (entry->path):
                              (!strcmp (tmp, "$#")) ? basename (entry->path):
                              NULL;
        if (entry->command[n-1])
            free (tmp);
        else
            entry->command[n-1] = tmp;

        facron_lexer_skip_spaces (parser->lexer);
    }

    if (n == 0)
    {
        fprintf (stderr, "Error: no command line specified for \"%s\"\n", entry->path);
        goto fail;
    }

    entry->command[n] = NULL;
    parser->previous_entry = entry;

    return entry;

fail:
    free (entry->path);
    free (entry);
fail_early:
    return facron_parser_parse_entry (parser);
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
facron_parser_new (void)
{
    FacronParser *parser = (FacronParser *) malloc (sizeof (FacronParser));

    parser->lexer = facron_lexer_new ();
    parser->previous_entry = NULL;

    return parser;
}
