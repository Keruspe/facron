# This file is part of facron.
#
# Copyright 2012 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
#
# facron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# facron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with facron.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS += \
	bin/facron \
	$(NULL)

bin_facron_SOURCES = \
	src/facron/facron.c \
	src/facron/facron-conf.h \
	src/facron/facron-conf.c \
	src/facron/facron-conf-entry.h \
	src/facron/facron-conf-entry.c \
	src/facron/facron-lexer.h \
	src/facron/facron-lexer.c \
	src/facron/facron-parser.h \
	src/facron/facron-parser.c \
	$(NULL)

bin_facron_CFLAGS = \
	$(AM_CFLAGS) \
	$(NULL)

bin_facron_LDADD = \
	$(NULL)
