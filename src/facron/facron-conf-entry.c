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

#include "facron-conf-entry.h"

#include <stdlib.h>

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
