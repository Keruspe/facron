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

#include <fcntl.h>
#include <sys/fanotify.h>

struct FacronConf
{
    FacronParser *parser;
    FacronConfEntry *entries;
    const char *filename;
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

    for (FacronConfEntry *entry; (entry = facron_parser_parse_entry (conf->parser)); conf->entries = entry);

    return true;
}

static FacronConfEntry *
facron_conf_reload (FacronConf *conf)
{
    FacronConfEntry *entries = conf->entries;
    conf->entries = NULL;
    if (!facron_conf_load (conf))
    {
        conf->entries = entries;
        return NULL;
    }
    else
    {
        return entries;
    }
}

static void
facron_conf_walk (FacronAction action, const FacronConfEntry *entries, int fanotify_fd)
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

    for (const FacronConfEntry *entry = entries; entry; entry = entry->next)
    {
        if (notice)
            fprintf (stderr, "Notice: tracking \"%s\"\n", entry->path);

        for (int i = 0; i < MAX_MASK_LEN && entry->mask[i]; ++i)
            fanotify_mark (fanotify_fd, flag, entry->mask[i], AT_FDCWD, entry->path);
    }
}

void
facron_conf_apply (const FacronConfEntry *entries, int fanotify_fd)
{
    facron_conf_walk (ADD, entries, fanotify_fd);
}

void
facron_conf_unapply (const FacronConfEntry *entries, int fanotify_fd)
{
    facron_conf_walk (REMOVE, entries, fanotify_fd);
}

void
facron_conf_reapply (FacronConf *conf, int fanotify_fd)
{
    FacronConfEntry *old_entries = facron_conf_reload (conf);

    facron_conf_unapply (old_entries, fanotify_fd);
    facron_conf_apply (conf->entries, fanotify_fd);
    facron_conf_entries_free (old_entries);
}

const FacronConfEntry *
facron_conf_get_entries (FacronConf *conf)
{
    return conf->entries;
}

void
facron_conf_free (FacronConf *conf, int fanotify_fd)
{
    facron_conf_unapply (conf->entries, fanotify_fd);
    if (conf->entries)
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
