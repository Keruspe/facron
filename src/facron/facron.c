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

#include "config.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/fanotify.h>

#include <linux/fanotify.h>
#include <linux/limits.h>

typedef enum
{
    EMPTY,
    F,
    FA,
    FAN,
    FAN_,
    FAN_T,
    FAN_TE,
    FAN_TES,
    FAN_TEST,
    ERROR,
    __ = ERROR
} FacronToken;

static const char syms[FAN_TEST]['z' - 'a' + 2] =
{
                /* a,  b,  c,  d,      e,  f,  g,  h,  i,  j,  k,  l,  m,   n,  o,  p,  q,  r,       s,        t,  u,  v,  w,  x,  y,  z,    _, other */
    [EMPTY]   = { __, __, __, __,     __,  F, __, __, __, __, __, __, __,  __, __, __, __, __,      __,       __, __, __, __, __, __, __,   __, __ },
    [F]       = { FA, __, __, __,     __, __, __, __, __, __, __, __, __,  __, __, __, __, __,      __,       __, __, __, __, __, __, __,   __, __ },
    [FA]      = { __, __, __, __,     __, __, __, __, __, __, __, __, __, FAN, __, __, __, __,      __,       __, __, __, __, __, __, __,   __, __ },
    [FAN]     = { __, __, __, __,     __, __, __, __, __, __, __, __, __,  __, __, __, __, __,      __,       __, __, __, __, __, __, __, FAN_, __ },
    [FAN_]    = { __, __, __, __,     __, __, __, __, __, __, __, __, __,  __, __, __, __, __,      __,    FAN_T, __, __, __, __, __, __,   __, __ },
    [FAN_T]   = { __, __, __, __, FAN_TE, __, __, __, __, __, __, __, __,  __, __, __, __, __,      __,       __, __, __, __, __, __, __,   __, __ },
    [FAN_TE]  = { __, __, __, __,     __, __, __, __, __, __, __, __, __,  __, __, __, __, __, FAN_TES,       __, __, __, __, __, __, __,   __, __ },
    [FAN_TES] = { __, __, __, __,     __, __, __, __, __, __, __, __, __,  __, __, __, __, __,      __, FAN_TEST, __, __, __, __, __, __,   __, __ }
};

typedef struct
{
    char *path;
    unsigned long mask;
} FacronEntry;

typedef struct fanotify_event_metadata FacronMetadata;

static bool
is_space (char c)
{
    return (c == ' ' || c == '\t');
}

static int
to_val (char c)
{
    if (c == '_')
        return 'z' - 'a' + 1;
    if (c >= 'A' && c <= 'Z')
        c += ('a' -  'A');
    if (c >= 'a' && c <= 'z')
        return c - 'a';
    return 'z' - 'a' + 2;
}

static bool
read_next (FacronEntry *entry, FILE *conf)
{
    entry->path = NULL;
    entry->mask = 0;

    size_t len;
    if (getline (&entry->path, &len, conf) < 1)
        return false;
    char *line = entry->path;

    for (size_t i = 0; i < len; ++i)
    {
        if (is_space (line[i]))
        {
            line[i] = '\0';
            line += i + 1;
            len -= (i + 1);
            printf ("path to monitor: \"%s\"\n", entry->path);
            break;
        }
    }

    while (is_space (line[0]))
    {
        ++line;
        --len;
    }

    FacronToken token = EMPTY;

    for (size_t i = 0; i < len; ++i)
    {
        if (line[i] == '\n')
            break;
        token = syms[token][to_val (line[i])];
        switch (token)
        {
        case FAN_TEST:
            printf ("Found FAN_TEST !\n");
            break;
        case ERROR:
            fprintf (stderr, "Error: \"%s\" not understood\n", line);
            return false;
        default:
            break;
        }
    }

    return true;
}

static bool
watch_next (int fanotify_fd, FILE *conf)
{
    FacronEntry entry;
    if (!read_next (&entry, conf))
        return false;

    /* TODO: FAN_MARK_REMOVE */
    return fanotify_mark (fanotify_fd, FAN_MARK_ADD, entry.mask, AT_FDCWD, entry.path);
}

static void
load_conf (int fanotify_fd)
{
    FILE *conf = fopen ("/etc/facron.conf", "r");

    while (watch_next (fanotify_fd, conf));

    fclose (conf);
}

int
main (void)
{
    int fanotify_fd = fanotify_init (FAN_CLASS_NOTIF, O_RDONLY | O_LARGEFILE);

    if (fanotify_fd < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return -1;
    }

    load_conf (fanotify_fd);

    char buf[4096];
    size_t len;

    while ((len = read (fanotify_fd, buf, sizeof (buf))) > 0)
    {
        char path[PATH_MAX];
        int path_len;

        FacronMetadata *metadata = (FacronMetadata *) buf;

        while (FAN_EVENT_OK (metadata, len))
        {
            if (metadata->vers < 2)
            {
                fprintf (stderr, "Kernel fanotify version too old\n");
                close (metadata->fd);
                goto fail;
            }

            if (metadata->fd < 0)
                continue;

            sprintf (path, "/proc/self/fd/%d", metadata->fd);
            path_len = readlink (path, path, sizeof (path) - 1);
            if (path_len < 0)
                goto next;
            path[path_len] = '\0';

next:
            close (metadata->fd);
        }
    }

fail:
    close (fanotify_fd);

    return 0;
}
