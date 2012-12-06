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

#include "conf.h"

#include <stdio.h>
#include <stdlib.h>

FacronConf *
load_conf (void)
{
    FILE *conf_file = fopen ("/etc/facron.conf", "r");

    if (!conf_file)
    {
        fprintf (stderr, "Error: could not load configuration file, does \"/etc/facron.conf\" exist?\n");
        return NULL;
    }

    fprintf (stderr, "Notice: loading configuration\n");

    FacronConf *conf = NULL;
    for (FacronConf *entry; (entry = read_next (conf, conf_file)); conf = entry);

    fclose (conf_file);
    return conf;
}

void
unload_conf (FacronConf *conf)
{
    while (conf)
    {
        free (conf-> path);
        for (int i = 0; i < 512 && conf->command[i]; ++i)
            free (conf->command[i]);
        FacronConf *old = conf;
        conf = conf->next;
        free (old);
    }
}
