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

typedef struct
{
    char *path;
    unsigned long mask;
} FacronEntry;

typedef struct fanotify_event_metadata FacronMetadata;

static const char *
read_next (FacronEntry *entry)
{
    /* TODO */
    entry->path = NULL;
    entry->mask = 0;

    return entry->path;
}

static bool
watch_next (int fanotify_fd)
{
    FacronEntry entry;
    read_next (&entry);
    if (!read_next (&entry))
        return false;

    /* TODO: FAN_MARK_REMOVE */
    return fanotify_mark (fanotify_fd, FAN_MARK_ADD, entry.mask, AT_FDCWD, entry.path);
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

    while (watch_next (fanotify_fd));

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
