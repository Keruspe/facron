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

#include "conf-lexer.h"
#include "conf-parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct FacronParser
{
    int dummy;
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

static inline bool
is_space (char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

FacronConf *
read_next (FacronConf *previous, FILE *conf)
{
    size_t dummy_len = 0;
    ssize_t len;
    char *line = NULL;
    if ((len = getline (&line, &dummy_len, conf)) < 1)
        return NULL;

    char *line_beg = line;
    if (is_space (line[0]))
        goto fail_early;

    for (ssize_t i = 1; i < len; ++i)
    {
        if (is_space (line[i]))
        {
            line[i] = '\0';
            line += (i + 1);
            len -= (i + 1);
            break;
        }
    }

    if (access (line_beg, R_OK))
    {
        fprintf (stderr, "warning: No such file or directory: \"%s\"\n", line_beg);
        goto fail_early;
    }

    while (is_space (line[0]))
    {
        ++line;
        --len;
    }

    if (0 == len)
    {
        fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
        goto fail_early;
    }

    FacronConf *entry = (FacronConf *) calloc (1, sizeof (FacronConf));
    entry->path = strdup (line_beg);
    entry->next = previous;

    int n = 0;
    ssize_t i = 0;
    FacronState state;
    unsigned long long mask;
    while ((state = next_token (line, &i, len, &mask))) /* != S_END */
    {
        switch (state)
        {
        case S_ERROR:
            line[len - 1] = '\0';
            fprintf (stderr, "Error at char %c: \"%s\" not understood\n", line[i], line + i);
            goto fail;
        case S_COMMA:
            entry->mask[n++] |= mask;
            break;
        case S_PIPE:
            entry->mask[n] |= mask;
        default:
            break;
        }
    }
    entry->mask[n] |= mask;
    line += (i + 1);
    len -= (i + 1);

    if (n == 0 && !entry->mask[n])
    {
        fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
        goto fail;
    }

    n = 0;
    for (i = 0; len > 0 && n < 511; ++n, i = 0)
    {
        while (is_space (line[0]) && i < len)
        {
            ++line;
            --len;
        }

        char delim = (line[0] == '"' || line[0] == '\'') ? line[0] : '\0';
        if (delim != '\0')
        {
            ++line;
            --len;
        }

        while (i < len &&
                ((delim == '\0' && !is_space (line[i])) ||
                 (delim != '\0' && line[i] != delim)))
        {
            ++i;
        }

        if (i == len)
            break;

        line[i] = '\0';
        entry->command[n] = (!strcmp (line, "$$")) ? strdup (entry->path):
                            (!strcmp (line, "$@")) ? dirname (entry->path):
                            (!strcmp (line, "$#")) ? basename (entry->path):
                            strdup (line);
        line += (i + 1);
        len -= (i + 1);
    }

    if (n == 0)
    {
        fprintf (stderr, "Error: no command line specified for \"%s\"\n", entry->path);
        goto fail;
    }

    entry->command[n] = NULL;

    free (line_beg);
    return entry;

fail:
    free (entry->path);
    free (entry);
fail_early:
    free (line_beg);
    return read_next (previous, conf);
}
