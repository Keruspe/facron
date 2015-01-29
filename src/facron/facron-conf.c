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

struct FacronConf
{
    FacronParser *parser;
    FacronConfEntry *entries;
    const char *filename;
};

static bool
facron_conf_load (FacronConf *conf)
{
    if (!facron_parser_reload (conf->parser))
        return false;

    fprintf (stderr, "Notice: loading configuration from %s\n", conf->filename);

    for (FacronConfEntry *entry; (entry = facron_parser_parse_entry (conf->parser)); conf->entries = entry);

    return true;
}

bool
facron_conf_reload (FacronConf *conf)
{
    FacronConfEntry *entries = conf->entries;
    conf->entries = NULL;
    if (!facron_conf_load (conf))
    {
        conf->entries = entries;
        return false;
    }
    if (entries)
        facron_conf_entry_free (entries, true);
    return true;
}

const FacronConfEntry *
facron_conf_get_entries (FacronConf *conf)
{
    return conf->entries;
}

void
facron_conf_free (FacronConf *conf)
{
    if (conf->entries)
        facron_conf_entry_free (conf->entries, true);
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
