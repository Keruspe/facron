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

#include "config.h"
#include "facron-conf.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/fanotify.h>
#include <sys/wait.h>

#include <linux/fanotify.h>
#include <linux/limits.h>

static int fanotify_fd;
static FacronConf *_conf = NULL;

typedef struct fanotify_event_metadata FacronMetadata;

typedef enum
{
    ADD,
    REMOVE
} FacronAction;

static inline void
walk_conf (FacronAction action)
{
    int flag;
    bool notice = false;

    switch (action) {
    case ADD:
        flag = FAN_MARK_ADD;
        notice = true;
        break;
    default:
        flag = FAN_MARK_REMOVE;
        break;
    }

    for (const FacronConfEntry *entry = facron_conf_get_entries (_conf); entry; entry = entry->next)
    {
        if (notice)
            fprintf (stderr, "Notice: tracking \"%s\"\n", entry->path);

        for (int i = 0; i < 512 && entry->mask[i]; ++i)
            fanotify_mark (fanotify_fd, flag, entry->mask[i], AT_FDCWD, entry->path);
    }
}


static inline void
apply_conf (void)
{
    walk_conf (ADD);
}

static inline void
unapply_conf (void)
{
    walk_conf (REMOVE);
}

static inline void
reapply_conf (void)
{
    if (facron_conf_reload (_conf))
    {
        unapply_conf ();
        apply_conf ();
    }
}

static inline void
cleanup (void)
{
    unapply_conf ();
    facron_conf_free (_conf);
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
        fprintf (stderr, "Signal %d received, exiting.\n", signum);
        cleanup ();
        exit (signum);
    }
}

static inline void
usage (char *callee)
{
    fprintf (stderr, "USAGE: %s [--background]", callee);
    exit (EXIT_FAILURE);
}

static inline char *
print_number (unsigned int n)
{
    char *tmp = NULL;
    if (asprintf (&tmp, "%u", n) < 1)
        return strdup ("0");
    return tmp;
}

typedef struct CommandBackup CommandBackup;
struct CommandBackup
{
    int index;
    char *field;
    CommandBackup *next;
};

static void
exec_command (char *command[512])
{
    static unsigned int count = 0;

    CommandBackup *backup = NULL;
    for (unsigned int i = 0; i < 512 && command[i]; ++i)
    {
        char *field = command[i];
        bool subst = false;
        if (!strcmp ("$+", field))
        {
            ++count;
            subst = true;
        }
        else if (!strcmp ("$-", field))
        {
            --count;
            subst = true;
        }
        else if (!strcmp ("$=", field))
            subst = true;

        if (subst)
        {
            CommandBackup *b = (CommandBackup *) malloc (sizeof (CommandBackup));
            b->index = i;
            b->field = field;
            b->next = backup;
            backup = b;
            command[i] = print_number (count);
        }
    }
   
    pid_t p = fork ();
    if (p)
        waitpid (p, NULL, 0);
    else
    {
        if (fork ())
            exit (EXIT_SUCCESS);
        else
            execv (command[0], command);
    }

    for (CommandBackup *next; backup != NULL; next = backup->next, free (backup), backup = next)
    {
        free (command[backup->index]);
        command[backup->index] = backup->field;
    }
}

int
main (int argc, char *argv[])
{
    bool background = false;

    switch (argc)
    {
    case 1:
        break;
    case 2:
        if (strcmp (argv[1], "--background"))
            usage (argv[0]);
        background = true;
        break;
    default:
        usage (argv[0]);
    }

    if (background)
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
    signal (SIGINT, &signal_handler);
    signal (SIGUSR1, &signal_handler);

    if ((fanotify_fd = fanotify_init (FAN_CLASS_NOTIF, O_RDONLY|O_LARGEFILE)) < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return EXIT_FAILURE;
    }

    _conf = facron_conf_new ();
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

            for (const FacronConfEntry *entry = facron_conf_get_entries (_conf); entry; entry = entry->next)
            {
                if (!strcmp (entry->path, path))
                {
                    for (int i = 0; i < 512 && entry->mask[i]; ++i)
                    {
                        if ((entry->mask[i] & metadata->mask) == entry->mask[i])
                            exec_command ((char **) entry->command);
                    }
                }
                else
                {
                    size_t plen = strlen (entry->path);
                    for (int i = 0; i < 512 && entry->mask[i]; ++i)
                    {
                        if ((entry->mask[i] & FAN_EVENT_ON_CHILD) && memcmp (entry->path, path, plen) && (entry->mask[i] & metadata->mask) == entry->mask[i])
                            exec_command ((char **) entry->command);
                    }
                }
            }

next:
            close (metadata->fd);
        }
    }

fail:
    cleanup ();

    return EXIT_FAILURE;
}
