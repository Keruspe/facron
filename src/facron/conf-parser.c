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

#include "conf-parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/fanotify.h>

static FacronChar
to_FacronChar (char c)
{
    switch (c)
    {
    case 'a': case 'A':
        return C_A;
    case 'b': case 'B':
        return C_B;
    case 'c': case 'C':
        return C_C;
    case 'd': case 'D':
        return C_D;
    case 'e': case 'E':
        return C_E;
    case 'f': case 'F':
        return C_F;
    case 'g': case 'G':
        return C_G;
    case 'h': case 'H':
        return C_H;
    case 'i': case 'I':
        return C_I;
    case 'j': case 'J':
        return C_J;
    case 'k': case 'K':
        return C_K;
    case 'l': case 'L':
        return C_L;
    case 'm': case 'M':
        return C_M;
    case 'n': case 'N':
        return C_N;
    case 'o': case 'O':
        return C_O;
    case 'p': case 'P':
        return C_P;
    case 'q': case 'Q':
        return C_Q;
    case 'r': case 'R':
        return C_R;
    case 's': case 'S':
        return C_S;
    case 't': case 'T':
        return C_T;
    case 'u': case 'U':
        return C_U;
    case 'v': case 'V':
        return C_V;
    case 'w': case 'W':
        return C_W;
    case 'x': case 'X':
        return C_X;
    case 'y': case 'Y':
        return C_Y;
    case 'z': case 'Z':
        return C_Z;
    case '_':
        return UNDERSCORE;
    case '|':
        return PIPE;
    case ' ': case '\t': case '\n': case '\r':
        return SPACE;
    default:
        return OTHER;
    }
}

static const char symbols[ERROR][NB_CHARS] =
{
          /*  A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  _,  |,   , ... */
    [_0] = { __, __, __, __, __, F0, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, __ },
    /* FAN_ */
    [F0] = { F1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [F1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, F2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [F2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, F3, __, __, __ },
    [F3] = { A0, __, C0, __, E0, __, __, __, __, __, __, __, M0, __, O0, __, Q0, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    /* FAN_ACCESS */
    [A0] = { __, __, A1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A1] = { __, __, A2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A2] = { __, __, __, __, A3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A4, __, __, __, __, __, __, __, __, __, __, __ },
    [A4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A5, __, __, __, __, __, __, __, __, __, __, __ },
    [A5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A6, _0, _0, __ },
    /* FAN_ACCESS_PERM */
    [A6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A7, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A7] = { __, __, __, __, A8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A9, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A9] = { __, __, __, __, __, __, __, __, __, __, __, __, AA, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [AA] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_MODIFY */
    [M0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, M1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M1] = { __, __, __, M2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M2] = { __, __, __, __, __, __, __, __, M3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M3] = { __, __, __, __, __, M4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, M5, __, __, __, __, __ },
    [M5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_CLOSE */
    [C0] = { __, __, __, __, __, __, __, __, __, __, __, C1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, C2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, C3, __, __, __, __, __, __, __, __, __, __, __ },
    [C3] = { __, __, __, __, C4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, C5, _0, _0, __ },
    /* FAN_CLOSE_ */
    [C5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, N0, __, __, __, __, __, __, __, __, W0, __, __, __, __, __, __, __ },
    /* FAN_CLOSE_WRITE */
    [W0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, W1, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W1] = { __, __, __, __, __, __, __, __, W2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, W3, __, __, __, __, __, __, __, __, __, __ },
    [W3] = { __, __, __, __, W4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_CLOSE_NOWRITE */
    [N0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, N1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N2, __, __, __, __, __, __, __ },
    [N2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N3, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N3] = { __, __, __, __, __, __, __, __, N4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N5, __, __, __, __, __, __, __, __, __, __ },
    [N5] = { __, __, __, __, N6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_OPEN */
    [O0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, OA, __, O1, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O1] = { __, __, __, __, O2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, O3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O4, _0, _0, __ },
    /* FAN_OPEN_PERM */
    [O4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O5, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O5] = { __, __, __, __, O6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O7, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O7] = { __, __, __, __, __, __, __, __, __, __, __, __, O8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_Q_OVERFLOW */
    [Q0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q1, __, __, __ },
    [Q1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q3, __, __, __, __, __, __, __, __ },
    [Q3] = { __, __, __, __, Q4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q5, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q5] = { __, __, __, __, __, Q6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q6] = { __, __, __, __, __, __, __, __, __, __, __, Q7, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q7] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q9, __, __, __, __, __, __, __ },
    [Q9] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_ONDIR */
    [OA] = { __, __, __, OB, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OB] = { __, __, __, __, __, __, __, __, OC, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OC] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, OD, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OD] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_EVENT_ON_CHILD */
    [E0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E1, __, __, __, __, __, __, __, __ },
    [E1] = { __, __, __, __, E2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, E3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E4, __, __, __, __, __, __, __, __, __, __ },
    [E4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E5, __, __, __ },
    [E5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, E6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, E7, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E7] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E8, __, __, __ },
    [E8] = { __, __, E9, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E9] = { __, __, __, __, __, __, __, EA, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EA] = { __, __, __, __, __, __, __, __, EB, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EB] = { __, __, __, __, __, __, __, __, __, __, __, EC, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EC] = { __, __, __, ED, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [ED] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ }
          /*  A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  _,  |, \ , ... */
};

static inline bool
is_space (char c, bool allow_newline)
{
    return (c == ' ' || c == '\t' || (allow_newline && (c == '\n' || c == '\r')));
}

static unsigned long long
FacronToken_to_mask (FacronToken t)
{
    switch (t)
    {
    case FAN_ACCESS_TOK:
        return FAN_ACCESS;
    case FAN_MODIFY_TOK:
        return FAN_MODIFY;
    case FAN_CLOSE_WRITE_TOK:
        return FAN_CLOSE_WRITE;
    case FAN_CLOSE_NOWRITE_TOK:
        return FAN_CLOSE_NOWRITE;
    case FAN_OPEN_TOK:
        return FAN_OPEN;
    case FAN_Q_OVERFLOW_TOK:
        return FAN_Q_OVERFLOW;
    case FAN_OPEN_PERM_TOK:
        return FAN_OPEN_PERM;
    case FAN_ACCESS_PERM_TOK:
        return FAN_ACCESS_PERM;
    case FAN_ONDIR_TOK:
        return FAN_ONDIR;
    case FAN_EVENT_ON_CHILD_TOK:
        return FAN_EVENT_ON_CHILD;
    case FAN_CLOSE_TOK:
        return FAN_CLOSE;
    case EMPTY:
        return 0;
    default:
        fprintf (stderr, "Warning: unknown token: %d\n", t);
        return 0;
    }
}

static FacronConfEntry *
read_next (FacronConfEntry *previous, FILE *conf)
{
    size_t dummy_len = 0;
    ssize_t len;
    char *line = NULL;
    if ((len = getline (&line, &dummy_len, conf)) < 1)
        return NULL;

    if (is_space (line[0], true))
    {
        free (line);
        return read_next (previous, conf);
    }

    char *line_beg = line;
    for (ssize_t i = 1; i < len; ++i)
    {
        if (is_space (line[i], false))
        {
            line[i] = '\0';
            line += (i + 1);
            len -= (i + 1);
            break;
        }
    }

    while (is_space (line[0], false))
    {
        ++line;
        --len;
    }

    FacronConfEntry *entry = (FacronConfEntry *) malloc (sizeof (FacronConfEntry));
    entry->path = strdup (line_beg);
    entry->mask = 0;
    entry->next = previous;

    if (access (entry->path, R_OK))
    {
        fprintf (stderr, "warning: No such file or directory: \"%s\"\n", entry->path);
        goto fail;
    }

    FacronToken token = EMPTY;
    for (ssize_t i = 0; i < len; ++i)
    {
        FacronToken prev_token = token;
        FacronChar c = to_FacronChar (line[i]);
        token = symbols[token][c];
        switch (token)
        {
        case ERROR:
            fprintf (stderr, "Error at char %c: \"%s\" not understood\n", line[i], line);
            free (line_beg);
            free (entry->path);
            free (entry);
            return read_next (previous, conf);
        case EMPTY:
            entry->mask |= FacronToken_to_mask (prev_token);
            if (c == SPACE)
            {
                if (!entry->mask)
                {
                    fprintf (stderr, "Error: no Fanotify mask has been specified.\n");
                    goto fail;
                }
                line += (i + 1);
                len -= (i + 1);
                goto end;
            }
        default:
            break;
        }
    }

end:;
    int n = 0;
    for (ssize_t i = 0; len > 0 && n < 511; ++n, i = 0)
    {
        while (is_space (line[0], false) && i < len)
        {
            ++line;
            --len;
        }

        while (i < len && !is_space (line[i], true))
            ++i;

        if (i == len)
            break;

        line[i] = '\0';
        entry->command[n] = strdup (line);
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
    free (line_beg);
    free (entry->path);
    free (entry);
    return read_next (previous, conf);
}

FacronConfEntry *
load_conf (void)
{
    FILE *conf_file = fopen ("/etc/facron.conf", "r");

    if (!conf_file)
        return NULL;

    FacronConfEntry *conf = NULL;
    for (FacronConfEntry *entry; (entry = read_next (conf, conf_file)); conf = entry);

    fclose (conf_file);
    return conf;
}

void
unload_conf (FacronConfEntry *conf)
{
    while (conf)
    {
        free (conf-> path);
        for (int i = 0; i < 512 && conf->command[i]; ++i)
            free (conf->command[i]);
        FacronConfEntry *old = conf;
        conf = conf->next;
        free (old);
    }
}
