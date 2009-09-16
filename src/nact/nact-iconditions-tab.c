/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
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
#include <string.h>

#include <common/na-object-api.h>
#include <common/na-obj-action-class.h>
#include <common/na-obj-profile.h>
#include <common/na-utils.h>

#include "base-iprefs.h"
#include "base-window.h"
#include "nact-main-tab.h"
#include "nact-iconditions-tab.h"

/* private interface data
 */
struct NactIConditionsTabInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GType      register_type( void );
static void       interface_base_init( NactIConditionsTabInterface *klass );
static void       interface_base_finalize( NactIConditionsTabInterface *klass );

static void       on_tab_updatable_selection_updated( NactIConditionsTab *instance, gint count_selected );
static GtkWidget *get_basenames_entry( NactIConditionsTab *instance );
static GtkButton *get_both_button( NactIConditionsTab *instance );
static GtkButton *get_isdir_button( NactIConditionsTab *instance );
static GtkButton *get_isfile_button( NactIConditionsTab *instance );
static GtkButton *get_matchcase_button( NactIConditionsTab *instance );
static GtkWidget *get_mimetypes_entry( NactIConditionsTab *instance );
static GtkButton *get_multiple_button( NactIConditionsTab *instance );
static void       on_basenames_changed( GtkEntry *entry, NactIConditionsTab *instance );
static void       on_isfiledir_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       on_matchcase_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       on_mimetypes_changed( GtkEntry *entry, NactIConditionsTab *instance );
static void       on_multiple_toggled( GtkToggleButton *button, NactIConditionsTab *instance );
static void       set_isfiledir( NactIConditionsTab *instance, gboolean isfile, gboolean isdir );

GType
nact_iconditions_tab_get_type( void )
{
	static GType iface_type = 0;

	if( !iface_type ){
		iface_type = register_type();
	}

	return( iface_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_iconditions_tab_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactIConditionsTabInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "NactIConditionsTab", &info, 0 );

	g_type_interface_add_prerequisite( type, BASE_WINDOW_TYPE );

	return( type );
}

static void
interface_base_init( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_init";
	static gboolean initialized = FALSE;

	if( !initialized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		klass->private = g_new0( NactIConditionsTabInterfacePrivate, 1 );

		initialized = TRUE;
	}
}

static void
interface_base_finalize( NactIConditionsTabInterface *klass )
{
	static const gchar *thisfn = "nact_iconditions_tab_interface_base_finalize";
	static gboolean finalized = FALSE ;

	if( !finalized ){
		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );

		finalized = TRUE;
	}
}

void
nact_iconditions_tab_initial_load_toplevel( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_initial_load_toplevel";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

void
nact_iconditions_tab_runtime_init_toplevel( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_runtime_init_toplevel";
	GtkWidget *basenames_widget;
	GtkButton *matchcase_button;
	GtkWidget *mimetypes_widget;
	GtkButton *isfile_button, *isdir_button, *both_button;
	GtkButton *multiple_button;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	basenames_widget = get_basenames_entry( instance );
	g_signal_connect(
			G_OBJECT( basenames_widget ),
			"changed",
			G_CALLBACK( on_basenames_changed ),
			instance );

	matchcase_button = get_matchcase_button( instance );
	g_signal_connect(
			G_OBJECT( matchcase_button ),
			"toggled",
			G_CALLBACK( on_matchcase_toggled ),
			instance );

	mimetypes_widget = get_mimetypes_entry( instance );
	g_signal_connect(
			G_OBJECT( mimetypes_widget ),
			"changed",
			G_CALLBACK( on_mimetypes_changed ),
			instance );

	isfile_button = get_isfile_button( instance );
	g_signal_connect(
			G_OBJECT( isfile_button ),
			"toggled",
			G_CALLBACK( on_isfiledir_toggled ),
			instance );

	isdir_button = get_isdir_button( instance );
	g_signal_connect(
			G_OBJECT( isdir_button ),
			"toggled",
			G_CALLBACK( on_isfiledir_toggled ),
			instance );

	both_button = get_both_button( instance );
	g_signal_connect(
			G_OBJECT( both_button ),
			"toggled",
			G_CALLBACK( on_isfiledir_toggled ),
			instance );

	multiple_button = get_multiple_button( instance );
	g_signal_connect(
			G_OBJECT( multiple_button ),
			"toggled",
			G_CALLBACK( on_multiple_toggled ),
			instance );

	g_signal_connect(
			G_OBJECT( instance ),
			TAB_UPDATABLE_SIGNAL_SELECTION_UPDATED,
			G_CALLBACK( on_tab_updatable_selection_updated ),
			instance );
}

void
nact_iconditions_tab_all_widgets_showed( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_all_widgets_showed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

void
nact_iconditions_tab_dispose( NactIConditionsTab *instance )
{
	static const gchar *thisfn = "nact_iconditions_tab_dispose";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );
}

void
nact_iconditions_tab_get_isfiledir( NactIConditionsTab *instance, gboolean *isfile, gboolean *isdir )
{
	gboolean both;

	g_assert( isfile );
	g_assert( isdir );

	both = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_both_button( instance )));
	if( both ){
		*isfile = TRUE;
		*isdir = TRUE;
	} else {
		*isfile = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isfile_button( instance )));
		*isdir = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( get_isdir_button( instance )));
	}
}

gboolean
nact_iconditions_tab_get_multiple( NactIConditionsTab *instance )
{
	GtkButton *button = get_multiple_button( instance );
	return( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button )));
}

static void
on_tab_updatable_selection_updated( NactIConditionsTab *instance, gint count_selected )
{
	static const gchar *thisfn = "nact_iconditions_tab_on_tab_updatable_selection_updated";
	NAObjectProfile *profile;
	GtkWidget *basenames_widget, *mimetypes_widget;
	GSList *basenames, *mimetypes;
	gchar *basenames_text, *mimetypes_text;
	GtkButton *matchcase_button;
	gboolean matchcase;
	gboolean isfile, isdir;
	GtkButton *files_button, *dir_button, *both_button;
	GtkButton *multiple_button;
	gboolean multiple;

	g_debug( "%s: instance=%p, count_selected=%d", thisfn, ( void * ) instance, count_selected );

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );

	basenames_widget = get_basenames_entry( instance );
	basenames = profile ? na_object_profile_get_basenames( profile ) : NULL;
	basenames_text = profile ? na_utils_string_list_to_text( basenames ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( basenames_widget ), basenames_text );
	g_free( basenames_text );
	na_utils_free_string_list( basenames );
	gtk_widget_set_sensitive( basenames_widget, profile != NULL );

	matchcase_button = get_matchcase_button( instance );
	matchcase = profile ? na_object_profile_get_matchcase( profile ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( matchcase_button ), matchcase );
	gtk_widget_set_sensitive( GTK_WIDGET( matchcase_button ), profile != NULL );

	mimetypes_widget = get_mimetypes_entry( instance );
	mimetypes = profile ? na_object_profile_get_mimetypes( profile ) : NULL;
	mimetypes_text = profile ? na_utils_string_list_to_text( mimetypes ) : g_strdup( "" );
	gtk_entry_set_text( GTK_ENTRY( mimetypes_widget ), mimetypes_text );
	g_free( mimetypes_text );
	na_utils_free_string_list( mimetypes );
	gtk_widget_set_sensitive( mimetypes_widget, profile != NULL );

	isfile = profile ? na_object_profile_get_is_file( profile ) : FALSE;
	isdir = profile ? na_object_profile_get_is_dir( profile ) : FALSE;
	set_isfiledir( instance, isfile, isdir );
	files_button = get_isfile_button( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( files_button ), profile != NULL );
	dir_button = get_isdir_button( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( dir_button ), profile != NULL );
	both_button = get_both_button( instance );
	gtk_widget_set_sensitive( GTK_WIDGET( both_button ), profile != NULL );

	multiple_button = get_multiple_button( instance );
	multiple = profile ? na_object_profile_get_multiple( profile ) : FALSE;
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( multiple_button ), multiple );
	gtk_widget_set_sensitive( GTK_WIDGET( multiple_button ), profile != NULL );
}

static GtkWidget *
get_basenames_entry( NactIConditionsTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsFilenamesEntry" ));
}

static GtkButton *
get_both_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsBothButton" )));
}

static GtkButton *
get_isdir_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsOnlyFoldersButton" )));
}

static GtkButton *
get_isfile_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsOnlyFilesButton" )));
}

static GtkButton *
get_matchcase_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMatchcaseButton" )));
}

static GtkWidget *
get_mimetypes_entry( NactIConditionsTab *instance )
{
	return( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMimetypesEntry" ));
}

static GtkButton *
get_multiple_button( NactIConditionsTab *instance )
{
	return( GTK_BUTTON( base_window_get_widget( BASE_WINDOW( instance ), "ConditionsMultipleButton" )));
}

static void
on_basenames_changed( GtkEntry *entry, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	const gchar *text;
	GSList *basenames;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		text = gtk_entry_get_text( entry );
		basenames = na_utils_text_to_string_list( text );
		na_object_profile_set_basenames( edited, basenames );
		na_utils_free_string_list( basenames );
		/*g_signal_emit_by_name( G_OBJECT( instance ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

/*
 * Note that this callback is triggered twice: first, for the
 * deactivated button, then a second time for the newly activated one.
 * I don't know what to do to be triggered only once..?
 */
static void
on_isfiledir_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	/*static const gchar *thisfn = "nact_iconditions_tab_on_isfiledir_toggled";*/
	NAObjectProfile *edited;
	gboolean isfile, isdir;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		nact_iconditions_tab_get_isfiledir( instance, &isfile, &isdir );
		na_object_profile_set_isfiledir( edited, isfile, isdir );
		/*g_signal_emit_by_name( G_OBJECT( instance ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

static void
on_matchcase_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	gboolean matchcase;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		matchcase = gtk_toggle_button_get_active( button );
		na_object_profile_set_matchcase( edited, matchcase );
		/*g_signal_emit_by_name( G_OBJECT( instance ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

static void
on_mimetypes_changed( GtkEntry *entry, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	const gchar *text;
	GSList *mimetypes;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		text = gtk_entry_get_text( entry );
		mimetypes = na_utils_text_to_string_list( text );
		na_object_profile_set_mimetypes( edited, mimetypes );
		na_utils_free_string_list( mimetypes );
		/*g_signal_emit_by_name( G_OBJECT( instance ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

static void
on_multiple_toggled( GtkToggleButton *button, NactIConditionsTab *instance )
{
	NAObjectProfile *edited;
	gboolean multiple;

	g_object_get(
			G_OBJECT( instance ),
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &edited,
			NULL );

	if( edited ){
		multiple = gtk_toggle_button_get_active( button );
		na_object_profile_set_multiple( edited, multiple );
		/*g_signal_emit_by_name( G_OBJECT( instance ), NACT_SIGNAL_MODIFIED_FIELD );*/
	}
}

static void
set_isfiledir( NactIConditionsTab *instance, gboolean isfile, gboolean isdir )
{
	if( isfile && isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_both_button( instance )), TRUE );

	} else if( isfile ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isfile_button( instance )), TRUE );

	} else if( isdir ){
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( get_isdir_button( instance )), TRUE );
	}
}
