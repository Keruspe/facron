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

#ifndef __FACRON_CONF_ENTRY_H__
#define __FACRON_CONF_ENTRY_H__

#include <stdbool.h>
#include <unistd.h>

#define MAX_CMD_LEN 512
#define MAX_MASK_LEN 512

typedef struct FacronConfEntry FacronConfEntry;
typedef struct fanotify_event_metadata FacronMetadata;

const FacronConfEntry *facron_conf_entry_get_next (const FacronConfEntry *entry);

void facron_conf_entry_apply_mask (FacronConfEntry   *entry,
                                   int                n_mask,
                                   unsigned long long mask);

void facron_conf_entry_add_command (FacronConfEntry *entry,
                                    char            *command);

bool facron_conf_entry_validate (const FacronConfEntry *entry);

void facron_conf_entry_apply (const FacronConfEntry *entry,
                              int                    fanotify_fd,
                              int                    flag,
                              bool                   notice);

void facron_conf_entry_handle (const FacronConfEntry *entry,
                               const char            *path,
                               size_t                 path_len,
                               const FacronMetadata  *metadata);

void facron_conf_entry_free (FacronConfEntry *entry);
void facron_conf_entries_free (FacronConfEntry *entry);

FacronConfEntry *facron_conf_entry_new (FacronConfEntry *next,
                                        char            *path);

#endif /* __FACRON_CONF_ENTRY_H_ */
