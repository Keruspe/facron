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

#ifndef __FACRON_CONF_ENTRY_H__
#define __FACRON_CONF_ENTRY_H__

#include <stdbool.h>
#include "facron.h"

typedef struct FacronConfEntry FacronConfEntry;

struct FacronConfEntry
{
    FacronConfEntry *next;
    char *path;
    unsigned long long mask[MAX_MASK_LEN];
    char *command[MAX_CMD_LEN];
};

void facron_conf_entry_free (FacronConfEntry *entry, bool follow);

FacronConfEntry *facron_conf_entry_new (FacronConfEntry *next, char *path);

#endif /* __FACRON_CONF_ENTRY_H_ */
