/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-iimporter.h>

#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-export-format.h"
#include "nact-gtk-utils.h"
#include "nact-schemes-list.h"
#include "nact-providers-list.h"
#include "nact-preferences-editor.h"

/* private class data
 */
struct NactPreferencesEditorClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactPreferencesEditorPrivate {
	gboolean dispose_has_run;
	gboolean preferences_locked;

	/* first tab: runtime preferences */
	guint    order_mode;
	gboolean order_mode_mandatory;
	gboolean root_menu;
	gboolean root_menu_mandatory;
	gboolean about_item;
	gboolean about_item_mandatory;

	/* second tab: ui preferences */
	gboolean relabel_menu;
	gboolean relabel_menu_mandatory;
	gboolean relabel_action;
	gboolean relabel_action_mandatory;
	gboolean relabel_profile;
	gboolean relabel_profile_mandatory;
	gboolean esc_quit;
	gboolean esc_quit_mandatory;
	gboolean esc_confirm;
	gboolean esc_confirm_mandatory;
	gboolean auto_save;
	gboolean auto_save_mandatory;
	guint    auto_save_period;
	gboolean auto_save_period_mandatory;

	/* third tab: import mode */
	guint    import_mode;
	gboolean import_mode_mandatory;

	/* fourth tab: export format */
	gboolean export_format_mandatory;

	/* fifth tab: default list of available schemes */
	/* sixth tab: i/o providers */
};

static GObjectClass *st_parent_class = NULL;
static guint         st_last_tab     = 0;

static GType    register_type( void );
static void     class_init( NactPreferencesEditorClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactPreferencesEditor *preferences_editor_new( BaseWindow *parent );

static gchar   *base_get_iprefs_window_id( const BaseWindow *window );
static gchar   *base_get_dialog_name( const BaseWindow *window );
static gchar   *base_get_ui_filename( const BaseWindow *dialog );
static void     on_base_initial_load_dialog( NactPreferencesEditor *editor, gpointer user_data );
static void     on_base_runtime_init_dialog( NactPreferencesEditor *editor, gpointer user_data );
static void     on_base_all_widgets_showed( NactPreferencesEditor *editor, gpointer user_data );
static void     order_mode_setup( NactPreferencesEditor *editor, NAPivot *pivot );
static void     order_mode_on_alpha_asc_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     order_mode_on_alpha_desc_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     order_mode_on_manual_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     order_mode_on_toggled( NactPreferencesEditor *editor, GtkToggleButton *togglebutton, GCallback cb, guint order_mode );
static void     root_menu_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     root_menu_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     about_item_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     about_item_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     relabel_menu_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     relabel_menu_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     relabel_action_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     relabel_action_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     relabel_profile_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     relabel_profile_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     esc_quit_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     esc_quit_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     esc_confirm_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     esc_confirm_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     auto_save_setup( NactPreferencesEditor *editor, NASettings *settings );
static void     auto_save_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor );
static void     auto_save_period_on_change_value( GtkSpinButton *spinbutton, NactPreferencesEditor *editor );
static void     import_mode_setup( NactPreferencesEditor *editor, NAPivot *pivot );
static void     import_mode_on_ask_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     import_mode_on_override_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     import_mode_on_renumber_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     import_mode_on_noimport_toggled( GtkToggleButton *togglebutton, NactPreferencesEditor *editor );
static void     import_mode_on_toggled( NactPreferencesEditor *editor, GtkToggleButton *togglebutton, GCallback cb, guint import_mode );
static void     on_cancel_clicked( GtkButton *button, NactPreferencesEditor *editor );
static void     on_ok_clicked( GtkButton *button, NactPreferencesEditor *editor );
static void     save_preferences( NactPreferencesEditor *editor );

static gboolean base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_preferences_editor_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_preferences_editor_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactPreferencesEditorClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactPreferencesEditor ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactPreferencesEditor", &info, 0 );

	return( type );
}

static void
class_init( NactPreferencesEditorClass *klass )
{
	static const gchar *thisfn = "nact_preferences_editor_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactPreferencesEditorClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->get_ui_filename = base_get_ui_filename;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_init";
	NactPreferencesEditor *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_PREFERENCES_EDITOR( instance ));
	self = NACT_PREFERENCES_EDITOR( instance );

	self->private = g_new0( NactPreferencesEditorPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_dispose";
	NactPreferencesEditor *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_PREFERENCES_EDITOR( dialog ));
	self = NACT_PREFERENCES_EDITOR( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		nact_schemes_list_dispose( BASE_WINDOW( self ));
		nact_providers_list_dispose( BASE_WINDOW( self ));

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_preferences_editor_instance_finalize";
	NactPreferencesEditor *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_PREFERENCES_EDITOR( dialog ));
	self = NACT_PREFERENCES_EDITOR( dialog );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactPreferencesEditor object.
 *
 * @parent: the BaseWindow parent of this dialog (usually, the main
 * toplevel window of the application).
 */
static NactPreferencesEditor *
preferences_editor_new( BaseWindow *parent )
{
	NactPreferencesEditor *editor;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;
	gboolean are_locked, mandatory;

	editor = NACT_PREFERENCES_EDITOR( g_object_new( NACT_PREFERENCES_EDITOR_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));

	application = NACT_APPLICATION( base_window_get_application( parent ));
	updater = nact_application_get_updater( application );
	settings = na_pivot_get_settings( NA_PIVOT( updater ));

	are_locked = na_settings_get_boolean( settings, NA_IPREFS_ADMIN_PREFERENCES_LOCKED, NULL, &mandatory );
	editor->private->preferences_locked = are_locked && mandatory;
	g_debug( "nact_preferences_editor_new: are_locked=%s, mandatory=%s",
			are_locked ? "True":"False", mandatory ? "True":"False" );

	return( editor );
}

/**
 * nact_preferences_editor_run:
 * @parent: the BaseWindow parent of this dialog
 * (usually the NactMainWindow).
 *
 * Initializes and runs the dialog.
 */
void
nact_preferences_editor_run( BaseWindow *parent )
{
	static const gchar *thisfn = "nact_preferences_editor_run";
	NactPreferencesEditor *editor;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );
	g_return_if_fail( BASE_IS_WINDOW( parent ));

	editor = preferences_editor_new( parent );

	base_window_run( BASE_WINDOW( editor ));
}

static gchar *
base_get_iprefs_window_id( const BaseWindow *window )
{
	return( g_strdup( NA_IPREFS_PREFERENCES_WSP ));
}

static gchar *
base_get_dialog_name( const BaseWindow *window )
{
	return( g_strdup( "PreferencesDialog" ));
}

static gchar *
base_get_ui_filename( const BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-preferences.ui" ));
}

static void
on_base_initial_load_dialog( NactPreferencesEditor *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_preferences_editor_on_initial_load_dialog";
	NactApplication *application;
	NAUpdater *updater;
	GtkWidget *container;
	GtkTreeView *listview;

	g_return_if_fail( NACT_IS_PREFERENCES_EDITOR( editor ));

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
	updater = nact_application_get_updater( application );

	container = base_window_get_widget( BASE_WINDOW( editor ), "PreferencesExportFormatVBox" );
	nact_export_format_init_display(
			container, NA_PIVOT( updater ),
			EXPORT_FORMAT_DISPLAY_PREFERENCES, !editor->private->preferences_locked );

	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "SchemesTreeView" ));
	nact_schemes_list_create_model( listview, SCHEMES_LIST_FOR_PREFERENCES );

	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ProvidersTreeView" ));
	nact_providers_list_create_model( listview );
}

static void
on_base_runtime_init_dialog( NactPreferencesEditor *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_preferences_editor_on_runtime_init_dialog";
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;
	GtkWidget *container;
	GQuark export_format;
	GtkTreeView *listview;
	GtkWidget *ok_button;

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
	updater = nact_application_get_updater( application );
	settings = na_pivot_get_settings( NA_PIVOT( updater ));

	/* first tab: runtime preferences
	 */
	order_mode_setup( editor, NA_PIVOT( updater ));
	root_menu_setup( editor, settings );
	about_item_setup( editor, settings );

	/* second tab: ui preferences
	 */
	relabel_menu_setup( editor, settings );
	relabel_action_setup( editor, settings );
	relabel_profile_setup( editor, settings );
	esc_quit_setup( editor, settings );
	esc_confirm_setup( editor, settings );
	auto_save_setup( editor, settings );

	/* third tab: import mode
	 */
	import_mode_setup( editor, NA_PIVOT( updater ));

	/* fourth tab: export format
	 */
	export_format = na_iprefs_get_export_format( NA_PIVOT( updater ), NA_IPREFS_EXPORT_PREFERRED_FORMAT, &editor->private->export_format_mandatory );
	container = base_window_get_widget( BASE_WINDOW( editor ), "PreferencesExportFormatVBox" );
	nact_export_format_select( container, !editor->private->export_format_mandatory, export_format );

	/* fifth tab: default schemes
	 */
	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "SchemesTreeView" ));
	nact_schemes_list_init_view( listview, BASE_WINDOW( editor ), NULL, NULL );

	/* sixth tab: I/O providers priorities
	 */
	listview = GTK_TREE_VIEW( base_window_get_widget( BASE_WINDOW( editor ), "ProvidersTreeView" ));
	nact_providers_list_init_view( listview, BASE_WINDOW( editor ));

	/* dialog buttons
	 */
	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ),
			"CancelButton",
			"clicked",
			G_CALLBACK( on_cancel_clicked ));

	ok_button = base_window_get_widget( BASE_WINDOW( editor ), "OKButton" );
	base_window_signal_connect(
			BASE_WINDOW( editor ),
			G_OBJECT( ok_button ),
			"clicked",
			G_CALLBACK( on_ok_clicked ));
	nact_gtk_utils_set_editable( G_OBJECT( ok_button ), !editor->private->preferences_locked );
}

static void
on_base_all_widgets_showed( NactPreferencesEditor *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_preferences_editor_on_all_widgets_showed";
	GtkNotebook *notebook;

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	notebook = GTK_NOTEBOOK( base_window_get_widget( BASE_WINDOW( editor ), "PreferencesNotebook" ));
	gtk_notebook_set_current_page( notebook, st_last_tab );

	nact_schemes_list_show_all( BASE_WINDOW( editor ));
}

/*
 * 'order mode' is editable if preferences in general are not locked,
 * and this one is not mandatory.
 *
 * This is not related in any way to the level zero order writability status
 *
 * The radio button group is insensitive if preferences are locked.
 * If preferences in general are not locked, but this one is mandatory,
 * then the radio group is sensitive, but not editable.
 */
static void
order_mode_setup( NactPreferencesEditor *editor, NAPivot *pivot )
{
	gboolean editable;
	GtkWidget *alpha_asc_button, *alpha_desc_button, *manual_button;
	GtkWidget *active_button;
	GCallback active_handler;

	editor->private->order_mode = na_iprefs_get_order_mode( pivot, &editor->private->order_mode_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->order_mode_mandatory;

	alpha_asc_button = base_window_get_widget( BASE_WINDOW( editor ), "OrderAlphaAscButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( alpha_asc_button ), "toggled", G_CALLBACK( order_mode_on_alpha_asc_toggled ));

	alpha_desc_button = base_window_get_widget( BASE_WINDOW( editor ), "OrderAlphaDescButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( alpha_desc_button ), "toggled", G_CALLBACK( order_mode_on_alpha_desc_toggled ));

	manual_button = base_window_get_widget( BASE_WINDOW( editor ), "OrderManualButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( manual_button ), "toggled", G_CALLBACK( order_mode_on_manual_toggled ));

	switch( editor->private->order_mode ){
		case IPREFS_ORDER_ALPHA_ASCENDING:
			active_button = alpha_asc_button;
			active_handler = G_CALLBACK( order_mode_on_alpha_asc_toggled );
			break;
		case IPREFS_ORDER_ALPHA_DESCENDING:
			active_button = alpha_desc_button;
			active_handler = G_CALLBACK( order_mode_on_alpha_desc_toggled );
			break;
		case IPREFS_ORDER_MANUAL:
		default:
			active_button = manual_button;
			active_handler = G_CALLBACK( order_mode_on_manual_toggled );
			break;
	}

	nact_gtk_utils_radio_set_initial_state(
			GTK_RADIO_BUTTON( active_button ),
			active_handler, editor, editable, !editor->private->preferences_locked );
}

static void
order_mode_on_alpha_asc_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	order_mode_on_toggled( editor, toggle_button, G_CALLBACK( order_mode_on_alpha_asc_toggled ), IPREFS_ORDER_ALPHA_ASCENDING );
}

static void
order_mode_on_alpha_desc_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	order_mode_on_toggled( editor, toggle_button, G_CALLBACK( order_mode_on_alpha_desc_toggled ), IPREFS_ORDER_ALPHA_DESCENDING );
}

static void
order_mode_on_manual_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	order_mode_on_toggled( editor, toggle_button, G_CALLBACK( order_mode_on_manual_toggled ), IPREFS_ORDER_MANUAL );
}

static void
order_mode_on_toggled( NactPreferencesEditor *editor, GtkToggleButton *toggle_button, GCallback cb, guint order_mode )
{
	gboolean editable;
	gboolean active;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( toggle_button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		active = gtk_toggle_button_get_active( toggle_button );
		if( active ){
			editor->private->order_mode = order_mode;
		}
	} else {
		nact_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), cb );
	}
}

/*
 * create a root menu
 */
static void
root_menu_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->root_menu = na_settings_get_boolean( settings, NA_IPREFS_ITEMS_CREATE_ROOT_MENU, NULL, &editor->private->root_menu_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->root_menu_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"CreateRootMenuButton", G_CALLBACK( root_menu_on_toggled ),
			editor->private->root_menu, editable, !editor->private->preferences_locked );
}

static void
root_menu_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->root_menu = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * add an about item
 */
static void
about_item_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->about_item = na_settings_get_boolean( settings, NA_IPREFS_ITEMS_ADD_ABOUT_ITEM, NULL, &editor->private->about_item_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->about_item_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"AddAboutButton", G_CALLBACK( about_item_on_toggled ),
			editor->private->about_item, editable, !editor->private->preferences_locked );
}

static void
about_item_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->about_item = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * add an about item
 */
static void
relabel_menu_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->relabel_menu = na_settings_get_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_MENU, NULL, &editor->private->relabel_menu_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->relabel_menu_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"RelabelMenuButton", G_CALLBACK( relabel_menu_on_toggled ),
			editor->private->relabel_menu, editable, !editor->private->preferences_locked );
}

static void
relabel_menu_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->relabel_menu = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * add an about item
 */
static void
relabel_action_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->relabel_action = na_settings_get_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_ACTION, NULL, &editor->private->relabel_action_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->relabel_action_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"RelabelActionButton", G_CALLBACK( relabel_action_on_toggled ),
			editor->private->relabel_action, editable, !editor->private->preferences_locked );
}

static void
relabel_action_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->relabel_action = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * add an about item
 */
static void
relabel_profile_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->relabel_profile = na_settings_get_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_PROFILE, NULL, &editor->private->relabel_profile_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->relabel_profile_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"RelabelProfileButton", G_CALLBACK( relabel_profile_on_toggled ),
			editor->private->relabel_profile, editable, !editor->private->preferences_locked );
}

static void
relabel_profile_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->relabel_profile = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * whether Esc key quits the assistants
 */
static void
esc_quit_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->esc_quit = na_settings_get_boolean( settings, NA_IPREFS_ASSISTANT_ESC_QUIT, NULL, &editor->private->esc_quit_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->esc_quit_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"EscCloseButton", G_CALLBACK( esc_quit_on_toggled ),
			editor->private->esc_quit, editable, !editor->private->preferences_locked );
}

static void
esc_quit_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;
	GtkWidget *confirm_button;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->esc_quit = gtk_toggle_button_get_active( button );
		confirm_button = base_window_get_widget( BASE_WINDOW( editor ), "EscConfirmButton" );
		gtk_widget_set_sensitive( confirm_button, editor->private->esc_quit );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * whether we should ask for a user confirmation when quitting an assistant
 * on 'Esc' key
 */
static void
esc_confirm_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;

	editor->private->esc_confirm = na_settings_get_boolean( settings, NA_IPREFS_ASSISTANT_ESC_CONFIRM, NULL, &editor->private->esc_confirm_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->esc_confirm_mandatory;

	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"EscConfirmButton", G_CALLBACK( esc_confirm_on_toggled ),
			editor->private->esc_confirm, editable, !editor->private->preferences_locked );
}

static void
esc_confirm_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->esc_confirm = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}
}

/*
 * add an about item
 */
static void
auto_save_setup( NactPreferencesEditor *editor, NASettings *settings )
{
	gboolean editable;
	GtkWidget *spin_button;
	GtkAdjustment *adjustment;

	g_debug( "nact_preferences_editor_auto_save_setup" );
	editor->private->auto_save = na_settings_get_boolean( settings, NA_IPREFS_MAIN_SAVE_AUTO, NULL, &editor->private->auto_save_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->auto_save_mandatory;

	editor->private->auto_save_period = na_settings_get_uint( settings, NA_IPREFS_MAIN_SAVE_PERIOD, NULL, &editor->private->auto_save_period_mandatory );
	spin_button = base_window_get_widget( BASE_WINDOW( editor ), "AutoSavePeriodicitySpinButton" );
	adjustment = gtk_spin_button_get_adjustment( GTK_SPIN_BUTTON( spin_button ));
	gtk_adjustment_configure( adjustment, editor->private->auto_save_period, 1, 999, 1, 10, 0 );

	editable = !editor->private->preferences_locked && !editor->private->auto_save_period_mandatory;
	gtk_editable_set_editable( GTK_EDITABLE( spin_button ), editable );
	base_window_signal_connect( BASE_WINDOW( editor ),
			G_OBJECT( spin_button ), "value-changed", G_CALLBACK( auto_save_period_on_change_value ));

	editable = !editor->private->preferences_locked && !editor->private->auto_save_mandatory;
	nact_gtk_utils_toggle_set_initial_state( BASE_WINDOW( editor ),
			"AutoSaveCheckButton", G_CALLBACK( auto_save_on_toggled ),
			editor->private->auto_save, editable, !editor->private->preferences_locked );
}

static void
auto_save_on_toggled( GtkToggleButton *button, NactPreferencesEditor *editor )
{
	gboolean editable;
	GtkWidget *widget;
	gboolean sensitive;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		editor->private->auto_save = gtk_toggle_button_get_active( button );

	} else {
		nact_gtk_utils_toggle_reset_initial_state( button );
	}

	sensitive = editor->private->auto_save && !editor->private->preferences_locked;

	widget = base_window_get_widget( BASE_WINDOW( editor ), "AutoSavePeriodicitySpinButton" );
	gtk_widget_set_sensitive( widget, sensitive );

	widget = base_window_get_widget( BASE_WINDOW( editor ), "AutoSaveLabel1" );
	gtk_widget_set_sensitive( widget, sensitive  );

	widget = base_window_get_widget( BASE_WINDOW( editor ), "AutoSaveLabel2" );
	gtk_widget_set_sensitive( widget, sensitive  );
}

static void
auto_save_period_on_change_value( GtkSpinButton *spinbutton, NactPreferencesEditor *editor )
{
	g_debug( "nact_preferences_editor_auto_save_period_on_change_value" );
	editor->private->auto_save_period = gtk_spin_button_get_value_as_int( spinbutton );
}

/*
 * preferred import mode
 */
static void
import_mode_setup( NactPreferencesEditor *editor, NAPivot *pivot )
{
	gboolean editable;
	GtkWidget *ask_button, *override_button, *renumber_button, *noimport_button;
	GtkWidget *active_button;
	GCallback active_handler;

	editor->private->import_mode = na_iprefs_get_import_mode(
			pivot, NA_IPREFS_IMPORT_PREFERRED_MODE, &editor->private->import_mode_mandatory );
	editable = !editor->private->preferences_locked && !editor->private->import_mode_mandatory;

	ask_button = base_window_get_widget( BASE_WINDOW( editor ), "PrefsAskButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( ask_button ), "toggled", G_CALLBACK( import_mode_on_ask_toggled ));

	override_button = base_window_get_widget( BASE_WINDOW( editor ), "PrefsOverrideButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( override_button ), "toggled", G_CALLBACK( import_mode_on_override_toggled ));

	renumber_button = base_window_get_widget( BASE_WINDOW( editor ), "PrefsRenumberButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( renumber_button ), "toggled", G_CALLBACK( import_mode_on_renumber_toggled ));

	noimport_button = base_window_get_widget( BASE_WINDOW( editor ), "PrefsNoImportButton" );
	base_window_signal_connect( BASE_WINDOW( editor ), G_OBJECT( noimport_button ), "toggled", G_CALLBACK( import_mode_on_noimport_toggled ));

	switch( editor->private->import_mode ){
		case IMPORTER_MODE_ASK:
			active_button = ask_button;
			active_handler = G_CALLBACK( import_mode_on_ask_toggled );
			break;
		case IMPORTER_MODE_OVERRIDE:
			active_button = override_button;
			active_handler = G_CALLBACK( import_mode_on_override_toggled );
			break;
		case IMPORTER_MODE_RENUMBER:
			active_button = renumber_button;
			active_handler = G_CALLBACK( import_mode_on_renumber_toggled );
			break;
		case IMPORTER_MODE_NO_IMPORT:
		default:
			active_button = noimport_button;
			active_handler = G_CALLBACK( import_mode_on_noimport_toggled );
			break;
	}

	nact_gtk_utils_radio_set_initial_state(
			GTK_RADIO_BUTTON( active_button ),
			active_handler, editor, editable, !editor->private->preferences_locked );
}

static void
import_mode_on_ask_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	import_mode_on_toggled( editor, toggle_button, G_CALLBACK( import_mode_on_ask_toggled ), IMPORTER_MODE_ASK );
}

static void
import_mode_on_override_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	import_mode_on_toggled( editor, toggle_button, G_CALLBACK( import_mode_on_override_toggled ), IMPORTER_MODE_OVERRIDE );
}

static void
import_mode_on_renumber_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	import_mode_on_toggled( editor, toggle_button, G_CALLBACK( import_mode_on_renumber_toggled ), IMPORTER_MODE_RENUMBER );
}

static void
import_mode_on_noimport_toggled( GtkToggleButton *toggle_button, NactPreferencesEditor *editor )
{
	import_mode_on_toggled( editor, toggle_button, G_CALLBACK( import_mode_on_noimport_toggled ), IMPORTER_MODE_NO_IMPORT );
}

static void
import_mode_on_toggled( NactPreferencesEditor *editor, GtkToggleButton *toggle_button, GCallback cb, guint import_mode )
{
	gboolean editable;
	gboolean active;

	editable = ( gboolean ) GPOINTER_TO_UINT( g_object_get_data( G_OBJECT( toggle_button ), NACT_PROP_TOGGLE_EDITABLE ));

	if( editable ){
		active = gtk_toggle_button_get_active( toggle_button );
		if( active ){
			editor->private->import_mode = import_mode;
		}
	} else {
		nact_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), cb );
	}
}

static void
on_cancel_clicked( GtkButton *button, NactPreferencesEditor *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactPreferencesEditor *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static void
save_preferences( NactPreferencesEditor *editor )
{
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;
	GtkWidget *container;
	NAExportFormat *export_format;

	if( !editor->private->preferences_locked ){
		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));

		/* first tab: runtime preferences
		 */
		if( !editor->private->order_mode_mandatory ){
			na_iprefs_set_order_mode( NA_PIVOT( updater ), editor->private->order_mode );
		}
		if( !editor->private->root_menu_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_ITEMS_CREATE_ROOT_MENU, editor->private->root_menu );
		}
		if( !editor->private->about_item_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_ITEMS_ADD_ABOUT_ITEM, editor->private->about_item );
		}

		/* second tab: runtime preferences
		 */
		if( !editor->private->relabel_menu_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_MENU, editor->private->relabel_menu );
		}
		if( !editor->private->relabel_action_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_ACTION, editor->private->relabel_action );
		}
		if( !editor->private->relabel_profile_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_RELABEL_DUPLICATE_PROFILE, editor->private->relabel_profile );
		}
		if( !editor->private->esc_quit_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_ASSISTANT_ESC_QUIT, editor->private->esc_quit );
		}
		if( !editor->private->esc_confirm_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_ASSISTANT_ESC_CONFIRM, editor->private->esc_confirm );
		}
		if( !editor->private->auto_save_mandatory ){
			na_settings_set_boolean( settings, NA_IPREFS_MAIN_SAVE_AUTO, editor->private->auto_save );
		}
		if( !editor->private->auto_save_period_mandatory ){
			na_settings_set_uint( settings, NA_IPREFS_MAIN_SAVE_PERIOD, editor->private->auto_save_period );
		}

		/* third tab: import mode
		 */
		if( !editor->private->import_mode_mandatory ){
			na_iprefs_set_import_mode( NA_PIVOT( updater ), NA_IPREFS_IMPORT_PREFERRED_MODE, editor->private->import_mode );
		}

		/* fourth tab: export format
		 */
		if( !editor->private->export_format_mandatory ){
			container = base_window_get_widget( BASE_WINDOW( editor ), "PreferencesExportFormatVBox" );
			export_format = nact_export_format_get_selected( container );
			na_iprefs_set_export_format(
					NA_PIVOT( updater ), NA_IPREFS_EXPORT_PREFERRED_FORMAT, na_export_format_get_quark( export_format ));
		}

		/* fifth tab: list of default schemes
		 */
		nact_schemes_list_save_defaults( BASE_WINDOW( editor ));

		/* sixth tab: priorities of I/O providers
		 */
		nact_providers_list_save( BASE_WINDOW( editor ));
	}
}

static gboolean
base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_preferences_editor_on_dialog_response";
	NactPreferencesEditor *editor;
	gboolean stop;
	GtkNotebook *notebook;

	g_return_val_if_fail( NACT_IS_PREFERENCES_EDITOR( window ), FALSE );

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );

	stop = FALSE;
	editor = NACT_PREFERENCES_EDITOR( window );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:

			stop = TRUE;
			break;

		case GTK_RESPONSE_OK:
			save_preferences( editor );
			stop = TRUE;
			break;
	}

	if( stop ){
		notebook = GTK_NOTEBOOK( base_window_get_widget( window, "PreferencesNotebook" ));
		st_last_tab = gtk_notebook_get_current_page( notebook );
		g_object_unref( editor );
	}


	return( stop );
}
