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

#ifndef __FACRON_CONF_H__
#define __FACRON_CONF_H__

#include "facron-conf-entry.h"

#include <stdbool.h>

typedef struct FacronConf FacronConf;

void facron_conf_apply   (FacronConf *conf,
                          int         fanotify_fd);
void facron_conf_reapply (FacronConf *conf,
                          int         fanotify_fd);

const FacronConfEntry *facron_conf_get_entries (FacronConf *conf);

void facron_conf_free (FacronConf *conf,
                       int         fanotify_fd);

FacronConf *facron_conf_new (const char *filename);

#endif /* __FACRON_CONF_H_ */
