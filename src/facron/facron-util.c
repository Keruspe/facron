/*
 *      This file is part of facron.
 *
 *      Copyright 2015 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include "facron-util.h"

#include <stdio.h>
#include <stdlib.h>
#define basename
#include <string.h>
#undef basename

#include <sys/wait.h>

typedef struct CommandBackup CommandBackup;
struct CommandBackup
{
    int            index;
    char          *field;
    CommandBackup *next;
};

static inline char *
print_pid (pid_t pid)
{
    char *tmp = NULL;
    if (asprintf (&tmp, "%d", pid) < 1)
        return strdup ("0");
    return tmp;
}

static inline char *
print_number (unsigned int n)
{
    char *tmp = NULL;
    if (asprintf (&tmp, "%u", n) < 1)
        return strdup ("0");
    return tmp;
}

static inline char *
basename (const char *filename)
{
    char *bn = strrchr (filename, '/');
    return strdup (bn ? bn + 1 : filename);
}

static inline char *
substr (const char *str,
        size_t      len)
{
    return (char *) memcpy (calloc (len + 1, sizeof (char)), str, len);
}

static inline char *
dirname (const char *filename)
{
    char *c = strrchr (filename, '/');
    if (!c)
        return strdup (".");

    if (c[1] == '\0')
    {
        while (c != filename && c[-1] == '/')
            --c;
        c = memrchr (filename, '/', c - filename);
    }

    while (c != filename && c[-1] == '/')
        --c;
    return (c != filename) ? substr (filename, c - filename) :
                             (filename[1] == '/') ? strdup ("//") :
                                                    strdup ("/");
}

void
facron_exec_command (char       *command[MAX_CMD_LEN],
                     const char *path,
                     pid_t       pid)
{
    static unsigned int count = 0;

    if (!command || !*command)
        return;

    CommandBackup *backup = NULL;
    for (unsigned int i = 0; i < MAX_CMD_LEN && command[i]; ++i)
    {
        char *field = command[i];
        char *subst = NULL;

        if (!strcmp ("$$", field))
            subst = strdup (path);
        else if (!strcmp ("$@", field))
            subst = dirname (path);
        else if (!strcmp ("$#", field))
            subst = basename (path);
	else if (!strcmp ("$*", field))
            subst = print_pid (pid);
        else if (!strcmp ("$+", field))
            subst = print_number (++count);
        else if (!strcmp ("$-", field))
            subst = print_number (--count);
        else if (!strcmp ("$=", field))
            subst = print_number (count);

        if (subst)
        {
            CommandBackup *b = (CommandBackup *) malloc (sizeof (CommandBackup));
            b->index = i;
            b->field = field;
            b->next = backup;
            backup = b;
            command[i] = subst;
        }
    }

    pid_t p = fork ();
    if (p)
        waitpid (p, NULL, 0);
    else
    {
        if (fork ())
            _exit (EXIT_SUCCESS);
        else
            execv (command[0], command);
    }

    for (CommandBackup *next; backup != NULL; next = backup->next, free (backup), backup = next)
    {
        free (command[backup->index]);
        command[backup->index] = backup->field;
    }
}
