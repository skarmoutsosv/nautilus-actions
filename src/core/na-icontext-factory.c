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

#include <api/na-ifactory-object-data.h>
#include <api/na-data-def.h>
#include <api/na-data-types.h>

NADataDef data_def_conditions [] = {

	{ NAFO_DATA_BASENAMES,
				TRUE,
				TRUE,
				TRUE,
				N_( "List of patterns to be matched against the selected file(s)/folder(s)" ),
				N_( "A list of strings with joker '*' or '?' to be matched against the name(s) " \
					"of the selected file(s)/folder(s). Each selected items must match at least " \
					"one of the filename patterns for the action or the menu be candidate to " \
					"display.\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to '*'." ),
				NAFD_TYPE_STRING_LIST,
				"[*]",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"basenames",
				"Basenames",
				'b',
				"basename",
				0,
				G_OPTION_ARG_STRING_ARRAY,
				N_( "A pattern to be matched against basenames of selected file(s)/folder(s). " \
					"May include wildcards (* or ?). " \
					"You must set one option for each pattern you need" ),
				N_( "<EXPR>" ) },

	{ NAFO_DATA_MATCHCASE,
				TRUE,
				TRUE,
				TRUE,
				N_( "Whether the specified basenames are case sensitive (default)" ),
				N_( "Must be set to 'true' if the filename patterns are case sensitive, to 'false' " \
					"otherwise. E.g., if you need to match a filename in a case-sensitive manner, " \
					"set this key to 'true'. If you also want, for example '*.jpg' to match 'photo.JPG', " \
					"then set 'false'.\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to 'true'." ),
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"matchcase",
				"Matchcase",
				'a',
				"match-case",
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_MIMETYPES,
				TRUE,
				TRUE,
				TRUE,
				N_( "List of patterns to be matched against the mimetypes of the selected file(s)/folder(s)" ),
				N_( "A list of strings with joker '*' to be matched against the mimetypes of the " \
					"selected file(s)/folder(s). Each selected items must match at least one of " \
					"the mimetype patterns for the action to appear.\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to '*/*'." ),
				NAFD_TYPE_STRING_LIST,
				"[*/*]",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"mimetypes",
				"MimeTypes",
				'm',
				"mimetype",
				0,
				G_OPTION_ARG_STRING_ARRAY,
				N_( "A pattern to be matched against mimetypes of selected file(s)/folder(s). " \
					"May include wildcards (* or ?). " \
					"You must set one option for each pattern you need" ),
				N_( "<EXPR>" ) },

	/* a runtime boolean set to TRUE if we detect that the previous string list
	 * just cover all mimetypes - this let us optimize the check for candidate
	 * in the menu plugin
	 * when FALSE, than we have to check each and every mimetype :(
	 */
	{ NAFO_DATA_ALL_MIMETYPES,
				FALSE,
				FALSE,
				TRUE,
				"Does the mimetypes list is generic ?",
				"The generic wildcard may be coded as '*', or '*/*' or 'all' or 'all/*' or 'all/all'."
				"In each case, we will try to spend as less time as possible to check " \
				"selection mimetypes",
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				0,
				NULL,
				0,
				0,
				NULL,
				NULL },

	{ NAFO_DATA_ISFILE,
				TRUE,
				TRUE,
				TRUE,
				N_( "Whether the profile applies to files" ),
				N_( "Set to 'true' if the selection can have files, to 'false' otherwise.\n" \
					"This setting is tied in with the 'isdir' setting. The valid combinations are: \n" \
					"isfile=TRUE and isdir=FALSE: the selection may hold only files\n" \
					"isfile=FALSE and isdir=TRUE: the selection may hold only folders\n" \
					"isfile=TRUE and isdir=TRUE: the selection may hold both files and folders\n" \
					"isfile=FALSE and isdir=FALSE: this is an invalid combination " \
					"(your configuration will never appear).\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to 'true'." ),
				NAFD_TYPE_BOOLEAN,
				"true",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"isfile",
				NULL,
				'f',
				"accept-files",
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_ISDIR,
				TRUE,
				TRUE,
				TRUE,
				N_( "Whether the profile applies to folders" ),
				N_( "Set to 'true' if the selection can have folders, to 'false' otherwise.\n" \
					"This setting is tied in with the 'isfile' setting. The valid combinations are: \n" \
					"isfile=TRUE and isdir=FALSE: the selection may hold only files\n" \
					"isfile=FALSE and isdir=TRUE: the selection may hold only folders\n" \
					"isfile=TRUE and isdir=TRUE: the selection may hold both files and folders\n" \
					"isfile=FALSE and isdir=FALSE: this is an invalid combination " \
					"(your configuration will never appear).\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to 'false'." ),
				NAFD_TYPE_BOOLEAN,
				"false",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"isdir",
				NULL,
				'd',
				"accept-dirs",
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	/* this is obsoleted starting with 2.30-newdata (released as 2.31.x serie)
	 * and replaced by count-sign and count-number
	 */
	{ NAFO_DATA_MULTIPLE,
				TRUE,
				FALSE,
				FALSE,
				N_( "Whether the selection may be multiple" ),
				N_( "If you need more than one files or folders to be selected, set this " \
					"key to 'true'. If you want just one file or folder, set it to 'false'.\n" \
					"This obviously only applies when there is a selection.\n" \
					"Defaults to 'false'." ),
				NAFD_TYPE_BOOLEAN,
				"false",
				FALSE,
				FALSE,
				FALSE,
				FALSE,
				"accept-multiple-files",
				NULL,
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_SCHEMES,
				TRUE,
				TRUE,
				TRUE,
				N_( "List of schemes to be matched against those of selected file(s)/folder(s)" ),
										/* too long string for iso c: 510 (max=509) */
				N_( "Defines the list of valid schemes to be matched against the selected " \
					"items. The scheme is the protocol used to access the files. The " \
					"keyword to use is the one used in the URI by the file manager.\n" \
					"Examples of valid URI include:\n" \
					"- file:///tmp/foo.txt\n" \
					"- sftp:///root@test.example.net/tmp/foo.txt\n" \
					"The most common schemes are:\n" \
					"'file': local files\n" \
					"'sftp': files accessed via SSH\n" \
					"'ftp': files accessed via FTP\n" \
					"'smb': files accessed via Samba (Windows share)\n" \
					"'dav': files accessed via WebDAV.\n" \
					"All schemes used by your favorite file manager may be used here.\n" \
					"This obviously only applies when there is a selection, " \
					"or when targeting the special 'x-nautilus-desktop' scheme.\n" \
					"Defaults to 'file'." ),
				NAFD_TYPE_STRING_LIST,
				"[file]",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"schemes",
				"Schemes",
				's',
				"scheme",
				0,
				G_OPTION_ARG_STRING,
				N_( "A valid GIO scheme where the selected file(s)/folder(s) should be located. " \
					"You must set one option for each scheme you need" ),
				N_( "<STRING>" ) },

	{ NAFO_DATA_FOLDERS,
				TRUE,
				TRUE,
				TRUE,
				N_( "List of folders" ),
				N_( "Defines the list of valid paths to be matched against the current folder.\n " \
					"All folders 'under' the specified path are considered valid.\n" \
					"Defaults to '/'." ),
				NAFD_TYPE_STRING_LIST,
				"[/]",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"folders",
				"Folders",
				'r',
				"folder",
				0,
				G_OPTION_ARG_STRING,
				N_( "The path of a (parent) directory for which the item will be displayed. " \
					"You must set one option for each folder you need" ),
				N_( "<PATH>" ) },

	{ NAFO_DATA_SELECTION_COUNT,
				TRUE,
				TRUE,
				TRUE,
				N_( "Operator of the selection count relation" ),
				N_( "Whether this profile may be selected depending of the count of the selection.\n" \
					"This is a string of the form \"{'<'|'='|'>'} number\".\n" \
					"Examples of valid strings are: \"=0\", \"> 1\", \"< 10\".\n" \
					"Defaults to \">0\"." ),
				NAFD_TYPE_STRING,
				">0",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"selection-count",
				"SelectionCount",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_ONLY_SHOW,
				TRUE,
				TRUE,
				TRUE,
				N_( "Only show in environment" ),
				N_( "Defaults to all." ),
				NAFD_TYPE_STRING_LIST,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"only-show-in",
				"OnlyShowIn",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_NOT_SHOW,
				TRUE,
				TRUE,
				TRUE,
				N_( "Not show in environment" ),
				N_( "Defaults to none." ),
				NAFD_TYPE_STRING_LIST,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"not-show-in",
				"NotShowIn",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_TRY_EXEC,
				TRUE,
				TRUE,
				TRUE,
				N_( "Try exec" ),
				N_( "Note that, when specified, only the presence and the executability status of " \
					"the specified file are checked.\n" \
					"Parameters may appear in TryExec value, and will be substituted at runtime.\n" \
					"Defaults to successful." ),
				NAFD_TYPE_STRING,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"try-exec",
				"TryExec",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_SHOW_IF_REGISTERED,
				TRUE,
				TRUE,
				TRUE,
				N_( "Show if registered" ),
				N_( "The well-known name of a DBus service.\n" \
					"The item will be candidate if the named service is registered on session DBus at runtime.\n" \
					"Parameters may appear in ShowIfRegistered value, and will be substituted at runtime.\n" \
					"Defaults to successful." ),
				NAFD_TYPE_STRING,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"show-if-registered",
				"ShowIfRegistered",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_SHOW_IF_TRUE,
				TRUE,
				TRUE,
				TRUE,
				N_( "Show if True" ),
				N_( "A command which, when executed, should output a string on stdout.\n" \
					"The item will be candidate if the outputed string is equal to \"true\".\n" \
					"Parameters may appear in ShowIfTrue value, and will be substituted at runtime.\n" \
					"Defaults to successful." ),
				NAFD_TYPE_STRING,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"show-if-true",
				"ShowIfTrue",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_SHOW_IF_RUNNING,
				TRUE,
				TRUE,
				TRUE,
				N_( "Show if running" ),
				N_( "The name of a process.\n" \
					"The item will be candidate if the process name is found in memory at runtime.\n" \
					"Parameters may appear in ShowIfRunning value, and will be substituted at runtime.\n" \
					"Defaults to successful." ),
				NAFD_TYPE_STRING,
				NULL,
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"show-if-running",
				"ShowIfRunning",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NAFO_DATA_CAPABILITITES,
				TRUE,
				TRUE,
				TRUE,
				N_( "Capabilities" ),
				N_( "A list of capabilities each item of the selection must satisfy in order the item be candidate.\n" \
					"Capabilities may be negated.\n" \
					"Capabilities have to be choosen between following predefined ones:\n" \
					"- Owner: current user is the owner of selected items\n" \
					"- Readable: selected items are readable by user (probably more usefull when negated)\n" \
					"- Writable: selected items are writable by user\n" \
					"- Executable: selected items are executable by user\n" \
					"- Local: selected items are local.\n" \
					"Defaults to \"*;\"." ),
				NAFD_TYPE_STRING_LIST,
				"*",
				TRUE,
				TRUE,
				FALSE,
				FALSE,
				"capabilities",
				"Capabilities",
				0,
				NULL,
				0,
				G_OPTION_ARG_NONE,
				NULL,
				NULL },

	{ NULL },
};
