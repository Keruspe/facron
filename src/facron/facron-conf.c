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
#include "facron-parser.h"

#include <stdio.h>
#include <stdlib.h>

struct FacronConf
{
    FacronParser    *parser;
    FacronConfEntry *entries;
    const char      *filename;
};

typedef enum
{
    ADD,
    REMOVE
} FacronAction;

static bool
facron_conf_load (FacronConf *conf)
{
    if (!facron_parser_reload (conf->parser))
        return false;

    fprintf (stderr, "Notice: loading configuration from %s\n", conf->filename);

    for (FacronConfEntry *entry; (entry = facron_parser_parse_entry (conf->parser, conf->entries)); conf->entries = entry);

    return true;
}

static FacronConfEntry *
facron_conf_reload (FacronConf *conf)
{
    FacronConfEntry *entries = conf->entries;

    conf->entries = NULL;

    if (facron_conf_load (conf))
        return entries;

    conf->entries = entries;
    return NULL;
}

static void
facron_conf_walk (FacronAction           action,
                  const FacronConfEntry *entries,
                  int                    fanotify_fd)
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

    for (const FacronConfEntry *entry = entries; entry; entry = facron_conf_entry_get_next (entry))
        facron_conf_entry_apply (entry, fanotify_fd, flag, notice);
}

void
facron_conf_apply (FacronConf *conf,
                   int         fanotify_fd)
{
    facron_conf_walk (ADD, conf->entries, fanotify_fd);
}

static inline void
facron_conf_unapply (const FacronConfEntry *entries,
                     int                    fanotify_fd)
{
    facron_conf_walk (REMOVE, entries, fanotify_fd);
}

void
facron_conf_reapply (FacronConf *conf,
                     int         fanotify_fd)
{
    FacronConfEntry *old_entries = facron_conf_reload (conf);

    facron_conf_unapply (old_entries, fanotify_fd);
    facron_conf_apply (conf, fanotify_fd);
    facron_conf_entries_free (old_entries);
}

void
facron_conf_handle(FacronConf     *conf,
                   const char     *path,
                   size_t          path_len,
                   FacronMetadata *metadata)
{
    for (const FacronConfEntry *entry = conf->entries; entry; entry = facron_conf_entry_get_next (entry))
        facron_conf_entry_handle (entry, path, path_len, metadata);
}

void
facron_conf_free (FacronConf *conf,
                  int         fanotify_fd)
{
    facron_conf_unapply (conf->entries, fanotify_fd);
    facron_conf_entries_free (conf->entries);
    facron_parser_free (conf->parser);
    free (conf);
}

FacronConf *
facron_conf_new (const char *filename)
{
    FacronConf *conf = (FacronConf *) malloc (sizeof (FacronConf));

    conf->parser = facron_parser_new (filename);
    conf->entries = NULL;
    conf->filename = filename;

    facron_conf_load (conf);

    return conf;
}
