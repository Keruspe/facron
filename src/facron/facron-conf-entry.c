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

#include "facron-conf-entry.h"
#include "facron-util.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct FacronConfEntry
{
    FacronConfEntry   *next;
    char              *path;
    unsigned long long mask[MAX_MASK_LEN];
    char              *command[MAX_CMD_LEN];
    int                n_command;
};

typedef struct CommandBackup CommandBackup;
struct CommandBackup
{
    int            index;
    char          *field;
    CommandBackup *next;
};

const FacronConfEntry *
facron_conf_entry_get_next (const FacronConfEntry *entry)
{
    return (entry) ? entry->next : NULL;
}

void
facron_conf_entry_apply_mask (FacronConfEntry   *entry,
                              int                n_mask,
                              unsigned long long mask)
{
    entry->mask[n_mask] |= mask;
}

void
facron_conf_entry_add_command (FacronConfEntry *entry,
                               char            *command)
{
    entry->command[entry->n_command++] = command;
}

bool
facron_conf_entry_validate (const FacronConfEntry *entry)
{
    return entry && entry->mask[0];
}

void
facron_conf_entry_apply (const FacronConfEntry *entry,
                         int                    fanotify_fd,
                         int                    flag,
                         bool                   notice)
{
    if (notice)
        fprintf (stderr, "Notice: tracking \"%s\"\n", entry->path);

    for (int i = 0; i < MAX_MASK_LEN && entry->mask[i]; ++i)
        fanotify_mark (fanotify_fd, flag, entry->mask[i], AT_FDCWD, entry->path);
}

void
facron_conf_entry_handle (const FacronConfEntry *entry,
                          const char            *path,
                          size_t                 path_len,
                          const FacronMetadata  *metadata)
{
    if (!strcmp (entry->path, path))
    {
        for (int i = 0; i < MAX_MASK_LEN && entry->mask[i]; ++i)
        {
            if ((entry->mask[i] & metadata->mask) == entry->mask[i])
                facron_exec_command ((char **) entry->command, path, metadata->pid);
        }
    }
    else
    {
        size_t plen = strlen (entry->path);
        for (int i = 0; i < MAX_MASK_LEN && entry->mask[i]; ++i)
        {
            if ((entry->mask[i] & FAN_EVENT_ON_CHILD) &&
                (size_t)path_len >= plen &&
                (entry->path[plen - 1] == '/' || path[plen] == '/') &&
                !memcmp (entry->path, path, plen) &&
                (entry->mask[i] & metadata->mask) == (entry->mask[i] & ~FAN_EVENT_ON_CHILD))
                    facron_exec_command ((char **) entry->command, path, metadata->pid);
        }
    }
}

void
facron_conf_entry_free (FacronConfEntry *entry)
{
    free (entry->path);
    for (int i = 0; i < MAX_CMD_LEN && entry->command[i]; ++i)
        free (entry->command[i]);
    free (entry);
}

void
facron_conf_entries_free (FacronConfEntry *entry)
{
    if (!entry)
        return;

    for (FacronConfEntry *next = entry->next; entry; entry = next, next = entry->next)
        facron_conf_entry_free (entry);
}

FacronConfEntry *
facron_conf_entry_new (FacronConfEntry *next,
                       char            *path)
{
    FacronConfEntry *entry = (FacronConfEntry *) malloc (sizeof (FacronConfEntry));

    entry->next = next;
    entry->path = path;

    return entry;
}
