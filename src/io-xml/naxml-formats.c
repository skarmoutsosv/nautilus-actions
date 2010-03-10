/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <api/na-iexporter.h>

#include "naxml-formats.h"

NAIExporterFormat naxml_formats[] = {

	/* GCONF_SCHEMA_V1: a schema with owner, short and long descriptions;
	 * each action has its own schema addressed by the id
	 * (historical format up to v1.10.x serie)
	 */
	{ NAXML_FORMAT_GCONF_SCHEMA_V1,
			N_( "Export as a _full GConf schema file" ),
			N_( "This used to be the historical export format.\n" \
				"The exported schema file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ) },

	/* GCONF_SCHEMA_V2: the lightest schema still compatible with gconftool-2 --install-schema-file
	 * (no owner, no short nor long descriptions) - introduced in v 1.11
	 */
	{ NAXML_FORMAT_GCONF_SCHEMA_V2,
			N_( "Export as a _light GConf schema (v2) file" ),
			N_( "This format has been introduced in v 1.11 serie.\n" \
				"This is the lightest schema still compatible with GConf command-line tools,\n" \
				"while keeping backward compatibility with older Nautilus Actions Configuration " \
				"Tool versions.\n"
				"The exported schema file may later be imported via :\n" \
				"- Import assistant of the Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --import-schema-file command-line tool." ) },

	/* GCONF_ENTRY: not a schema, but a dump of the GConf entry
	 * introduced in v 1.11
	 */
	{ NAXML_FORMAT_GCONF_ENTRY,
			N_( "Export as a GConf _dump file" ),
			N_( "This format has been introduced in v 1.11 serie, " \
				"and should be the preferred format for newly exported items.\n" \
				"It is not backward compatible with previous Nautilus Actions " \
				"Configuration Tool versions,\n" \
				"though it may still be imported via standard GConf command-line tools.\n" \
				"The exported dump file may later be imported via :\n" \
				"- Import assistant of a compatible Nautilus Actions Configuration Tool,\n" \
				"- or via the gconftool-2 --load command-line tool." ) },

	{ NULL }
};