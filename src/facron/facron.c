/*
 *      This file is part of facron.
 *
 *      Copyright 2012-2015 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include "facron-conf.h"

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h>

#include <linux/limits.h>

static int fanotify_fd;
static FacronConf *_conf = NULL;

static inline void
cleanup (void)
{
    facron_conf_free (_conf, fanotify_fd);
    close (fanotify_fd);
}

static void
signal_handler (int signum)
{
    int status = signum;

    switch (signum)
    {
    case SIGUSR1:
        facron_conf_reapply (_conf, fanotify_fd);
        break;
    case SIGTERM:
        status = EXIT_SUCCESS;
    default:
        fprintf (stderr, "Signal %d received, exiting.\n", signum);
        cleanup ();
        exit (status);
    }
}

static inline void
usage (char *callee)
{
    fprintf (stderr, "USAGE: %s [--conf|-c config_file] [--daemon|-d]\n", callee);
    exit (EXIT_FAILURE);
}

int
main (int   argc,
      char *argv[])
{
    struct option long_options[] = {
        { "background", no_argument,       NULL, 'd' }, /* legacy compat */
        { "conf",       required_argument, NULL, 'c' },
        { "daemon",     no_argument,       NULL, 'd' },
        { 0,            no_argument,       NULL, 0   }
    };

    const char *conf_file = SYSCONFDIR "/facron.conf";
    bool daemon = false;
    int c;

    while ((c = getopt_long (argc, argv, "c:d", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'c':
            conf_file = optarg;
            break;
        case 'd':
            daemon = true;
            break;
        default:
            usage (argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (daemon)
    {
        pid_t p = fork ();
        if (p)
        {
            waitpid (p, NULL, 0);
            return EXIT_SUCCESS;
        }
        else
        {
            if (fork ())
                return EXIT_SUCCESS;
        }
    }

    signal (SIGTERM, &signal_handler);
    signal (SIGINT,  &signal_handler);
    signal (SIGUSR1, &signal_handler);

    if ((fanotify_fd = fanotify_init (FAN_CLASS_NOTIF|FAN_CLOEXEC|FAN_NONBLOCK, O_RDONLY|O_LARGEFILE)) < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return EXIT_FAILURE;
    }

    _conf = facron_conf_new (conf_file);
    facron_conf_apply (_conf, fanotify_fd);

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

            facron_conf_handle (_conf, path, path_len, metadata);

next:
            close (metadata->fd);
        }
    }

fail:
    cleanup ();

    return EXIT_FAILURE;
}
