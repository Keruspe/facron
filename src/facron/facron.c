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
#include "conf-parser.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fanotify.h>

#include <linux/fanotify.h>
#include <linux/limits.h>

static int fanotify_fd;
static FacronConfEntry *_conf = NULL;

static inline void
walk_conf (int flag)
{
    for (FacronConfEntry *entry = _conf; entry; entry = entry->next)
        fanotify_mark (fanotify_fd, flag, entry->mask, AT_FDCWD, entry->path);
}


static inline void
apply_conf (void)
{
    _conf = load_conf ();
    walk_conf (FAN_MARK_ADD);
}

static inline void
unapply_conf (void)
{
    walk_conf (FAN_MARK_REMOVE);
    unload_conf (_conf);
}

static inline void
reapply_conf (void)
{
    unapply_conf ();
    apply_conf ();
}

static void
cleanup (void)
{
    unapply_conf ();
    close (fanotify_fd);
}

static void
signal_handler (int signum)
{
    switch (signum)
    {
    case SIGUSR1:
        reapply_conf ();
        break;
    case SIGTERM:
        signum = EXIT_SUCCESS;
    default:
        printf ("Signal %d received, exiting.\n", signum);
        cleanup ();
        exit (signum);
    }
}

int
main (void)
{
    signal (SIGTERM, &signal_handler);
    signal (SIGINT, &signal_handler);
    signal (SIGUSR1, &signal_handler);

    if ((fanotify_fd = fanotify_init (FAN_CLASS_NOTIF, O_RDONLY|O_LARGEFILE)) < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return EXIT_FAILURE;
    }

    apply_conf ();

    char buf[4096];
    size_t len;

    while ((len = read (fanotify_fd, buf, sizeof (buf))) > 0)
    {
        char path[PATH_MAX];
        int path_len;

        for (FacronMetadata *metadata = (FacronMetadata *) buf; FAN_EVENT_OK (metadata, len); metadata = FAN_EVENT_NEXT (metadata, len))
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
            printf ("%s\n", path);

next:
            close (metadata->fd);
        }
    }

fail:
    cleanup ();

    return EXIT_FAILURE;
}
