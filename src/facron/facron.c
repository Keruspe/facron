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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fanotify.h>

#include <linux/fanotify.h>
#include <linux/limits.h>

static void
walk_conf (int fanotify_fd, FacronConfEntry *conf, int flag)
{
    while (conf)
    {
        fanotify_mark (fanotify_fd, flag, conf->mask, AT_FDCWD, conf->path);
        conf = conf->next;
    }
}

#define APPLY_CONF walk_conf (fanotify_fd, conf, FAN_MARK_ADD);
#define UNAPPLY_CONF walk_conf (fanotify_fd, conf, FAN_MARK_REMOVE);
#define REAPPLY_CONF UNAPPLY_CONF APPLY_CONF

int
main (void)
{
    int ret = EXIT_FAILURE;
    int fanotify_fd = fanotify_init (FAN_CLASS_NOTIF, O_RDONLY|O_LARGEFILE);

    /* TODO: sigterm */

    if (fanotify_fd < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return EXIT_FAILURE;
    }

    fanotify_mark (fanotify_fd, FAN_MARK_ADD, FAN_MODIFY|FAN_CLOSE_WRITE, AT_FDCWD, "/etc/facron.conf");

    FacronConfEntry *conf = load_conf ();
    if (!conf)
    {
        fprintf (stderr, "Failed to load configuration file, do \"/etc/facron.conf\" exist?\n");
        goto fail;
    }

    APPLY_CONF

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

            if (!strcmp (path, "/etc/facron.conf"))
            {
                conf = reload_conf (conf);
                REAPPLY_CONF
            }

next:
            close (metadata->fd);
        }
    }

    ret = EXIT_SUCCESS;

fail:
    fanotify_mark (fanotify_fd, FAN_MARK_REMOVE, FAN_MODIFY|FAN_CLOSE_WRITE, AT_FDCWD, "/etc/facron.conf");
    UNAPPLY_CONF
    unload_conf (conf);
    close (fanotify_fd);

    return ret;
}
