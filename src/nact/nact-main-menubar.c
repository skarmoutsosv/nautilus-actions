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

#include <runtime/na-iabout.h>
#include <runtime/na-ipivot-consumer.h>

#include <common/na-object-api.h>
#include <common/na-iprefs.h>

#include "nact-application.h"
#include "nact-assistant-export.h"
#include "nact-assistant-import.h"
#include "nact-preferences-editor.h"
#include "nact-iactions-list.h"
#include "nact-clipboard.h"
#include "nact-main-statusbar.h"
#include "nact-main-tab.h"
#include "nact-main-menubar.h"

#define MENUBAR_PROP_STATUS_CONTEXT		"nact-menubar-status-context"
#define MENUBAR_PROP_UI_MANAGER			"nact-menubar-ui-manager"
#define MENUBAR_PROP_ACTIONS_GROUP		"nact-menubar-actions-group"

/* GtkActivatable
 * gtk_action_get_tooltip are only available starting with Gtk 2.16
 * until this is a required level, we must have some code to do the
 * same thing
 */
#undef GTK_HAS_ACTIVATABLE
#if(( GTK_MAJOR_VERSION > 2 ) || ( GTK_MINOR_VERSION >= 16 ))
	#define GTK_HAS_ACTIVATABLE
#endif

#ifndef GTK_HAS_ACTIVATABLE
#define MENUBAR_PROP_ITEM_ACTION		"nact-menubar-item-action"
#endif

/* this structure is updated each time the user interacts in the
 * interface ; it is then used to update action sensitivities
 */
typedef struct {
	gint     selected_menus;
	gint     selected_actions;
	gint     selected_profiles;
	gint     clipboard_menus;
	gint     clipboard_actions;
	gint     clipboard_profiles;
	gint     list_menus;
	gint     list_actions;
	gint     list_profiles;
	gboolean is_modified;
	gboolean have_exportables;
}
	MenubarIndicatorsStruct;

#define MENUBAR_PROP_INDICATORS			"nact-menubar-indicators"

static void     on_iactions_list_count_updated( NactMainWindow *window, gint menus, gint actions, gint profiles );
static void     on_iactions_list_selection_changed( NactMainWindow *window, GList *selected );
static void     on_update_sensitivities( NactMainWindow *window, gpointer user_data );

static void     on_new_menu_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_action_activated( GtkAction *action, NactMainWindow *window );
static void     on_new_profile_activated( GtkAction *action, NactMainWindow *window );
static void     on_save_activated( GtkAction *action, NactMainWindow *window );
static void     save_items( NactMainWindow *window, NAPivot *pivot, GList *items );
static void     save_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *item );
static void     on_quit_activated( GtkAction *action, NactMainWindow *window );

static void     on_cut_activated( GtkAction *action, NactMainWindow *window );
static void     on_copy_activated( GtkAction *action, NactMainWindow *window );
static void     on_paste_activated( GtkAction *action, NactMainWindow *window );
static void     on_paste_into_activated( GtkAction *action, NactMainWindow *window );
static GList   *prepare_for_paste( NactMainWindow *window );
static void     on_duplicate_activated( GtkAction *action, NactMainWindow *window );
static void     on_delete_activated( GtkAction *action, NactMainWindow *window );
static void     update_clipboard_counters( NactMainWindow *window );
static void     on_reload_activated( GtkAction *action, NactMainWindow *window );
static void     on_preferences_activated( GtkAction *action, NactMainWindow *window );

static void     on_expand_all_activated( GtkAction *action, NactMainWindow *window );
static void     on_collapse_all_activated( GtkAction *action, NactMainWindow *window );

static void     on_import_activated( GtkAction *action, NactMainWindow *window );
static void     on_export_activated( GtkAction *action, NactMainWindow *window );

static void     on_help_activated( GtkAction *action, NactMainWindow *window );
static void     on_about_activated( GtkAction *action, NactMainWindow *window );

static void     enable_item( NactMainWindow *window, const gchar *name, gboolean enabled );
static gboolean on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window );
static void     on_destroy_callback( gpointer data );
static void     on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window );
static void     on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );
static void     on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window );

static const GtkActionEntry entries[] = {

		{ "FileMenu", NULL, N_( "_File" ) },
		{ "EditMenu", NULL, N_( "_Edit" ) },
		{ "ViewMenu", NULL, N_( "_View" ) },
		{ "ToolsMenu", NULL, N_( "_Tools" ) },
		{ "HelpMenu", NULL, N_( "_Help" ) },

		{ "NewMenuItem", NULL, N_( "New _menu" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New menu' item */
				N_( "Insert a new menu at the current position" ),
				G_CALLBACK( on_new_menu_activated ) },
		{ "NewActionItem", GTK_STOCK_NEW, N_( "_New action" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New action' item */
				N_( "Define a new action" ),
				G_CALLBACK( on_new_action_activated ) },
		{ "NewProfileItem", NULL, N_( "New _profile" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'New profile' item */
				N_( "Define a new profile attached to the current action" ),
				G_CALLBACK( on_new_profile_activated ) },
		{ "SaveItem", GTK_STOCK_SAVE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Save' item */
				N_( "Record all the modified actions. Invalid actions will be silently ignored" ),
				G_CALLBACK( on_save_activated ) },
		{ "QuitItem", GTK_STOCK_QUIT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting 'Quit' item */
				N_( "Quit the application" ),
				G_CALLBACK( on_quit_activated ) },
		{ "CutItem" , GTK_STOCK_CUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Cut item */
				N_( "Cut the selected item(s) to the clipboard" ),
				G_CALLBACK( on_cut_activated ) },
		{ "CopyItem" , GTK_STOCK_COPY, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Copy item */
				N_( "Copy the selected item(s) to the clipboard" ),
				G_CALLBACK( on_copy_activated ) },
		{ "PasteItem" , GTK_STOCK_PASTE, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Paste item */
				N_( "Insert the content of the clipboard just before the current position" ),
				G_CALLBACK( on_paste_activated ) },
		{ "PasteIntoItem" , NULL, N_( "Paste _into" ), "<Shift><Ctrl>V",
				/* i18n: tooltip displayed in the status bar when selecting the Paste Into item */
				N_( "Insert the content of the clipboard as first child of the current item" ),
				G_CALLBACK( on_paste_into_activated ) },
		{ "DuplicateItem" , NULL, N_( "D_uplicate" ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Duplicate item */
				N_( "Duplicate the selected item(s)" ),
				G_CALLBACK( on_duplicate_activated ) },
		{ "DeleteItem", GTK_STOCK_DELETE, NULL, "Delete",
				/* i18n: tooltip displayed in the status bar when selecting the Delete item */
				N_( "Delete the selected item(s)" ),
				G_CALLBACK( on_delete_activated ) },
		{ "ReloadActionsItem", GTK_STOCK_REFRESH, N_( "_Reload the list of actions" ), "F5",
				/* i18n: tooltip displayed in the status bar when selecting the 'Reload' item */
				N_( "Cancel your current modifications and reload the list of actions" ),
				G_CALLBACK( on_reload_activated ) },
		{ "PreferencesItem", GTK_STOCK_PREFERENCES, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the 'Preferences' item */
				N_( "Edit your preferences" ),
				G_CALLBACK( on_preferences_activated ) },
		{ "ExpandAllItem" , NULL, N_( "_Expand all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Expand all item */
				N_( "Entirely expand the items hierarchy" ),
				G_CALLBACK( on_expand_all_activated ) },
		{ "CollapseAllItem" , NULL, N_( "_Collapse all" ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Collapse all item */
				N_( "Entirely collapse the items hierarchy" ),
				G_CALLBACK( on_collapse_all_activated ) },
		{ "ImportItem" , GTK_STOCK_CONVERT, N_( "_Import assistant.." ), "",
				/* i18n: tooltip displayed in the status bar when selecting the Import item */
				N_( "Import one or more actions from external (XML) files into your configuration" ),
				G_CALLBACK( on_import_activated ) },
		{ "ExportItem", NULL, N_( "E_xport assistant.." ), NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Export item */
				N_( "Export one or more actions from your configuration to external XML files" ),
				G_CALLBACK( on_export_activated ) },
		{ "HelpItem" , GTK_STOCK_HELP, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the Help item */
				N_( "Display help about this program" ),
				G_CALLBACK( on_help_activated ) },
		{ "AboutItem", GTK_STOCK_ABOUT, NULL, NULL,
				/* i18n: tooltip displayed in the status bar when selecting the About item */
				N_( "Display informations about this program" ),
				G_CALLBACK( on_about_activated ) },
};

/**
 * nact_main_menubar_runtime_init:
 * @window: the #NactMainWindow to which the menubar is attached.
 *
 * Creates the menubar.
 * Connects to all possible signals which may have an impact on action
 * sensitivities.
 */
void
nact_main_menubar_runtime_init( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_init";
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GError *error = NULL;
	guint merge_id;
	GtkAccelGroup *accel_group;
	GtkWidget *menubar, *vbox;
	GtkWindow *toplevel;
	MenubarIndicatorsStruct *mis;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	/* create the menubar:
	 * - create action group, and insert list of actions in it
	 * - create ui_manager, and insert action group in it
	 * - merge inserted actions group with ui xml definition
	 * - install accelerators in the main window
	 * - pack menu bar in the main window
	 */
	ui_manager = gtk_ui_manager_new();
	g_object_set_data_full( G_OBJECT( window ), MENUBAR_PROP_UI_MANAGER, ui_manager, ( GDestroyNotify ) on_destroy_callback  );
	g_debug( "%s: ui_manager=%p", thisfn, ( void * ) ui_manager );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( ui_manager ),
			"connect-proxy",
			G_CALLBACK( on_proxy_connect ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( ui_manager ),
			"disconnect-proxy",
			G_CALLBACK( on_proxy_disconnect ));

	action_group = gtk_action_group_new( "MenubarActions" );
	g_object_set_data_full( G_OBJECT( window ), MENUBAR_PROP_ACTIONS_GROUP, action_group, ( GDestroyNotify ) on_destroy_callback );
	g_debug( "%s: action_group=%p", thisfn, ( void * ) action_group );

	gtk_action_group_set_translation_domain( action_group, GETTEXT_PACKAGE );
	gtk_action_group_add_actions( action_group, entries, G_N_ELEMENTS( entries ), window );
	gtk_ui_manager_insert_action_group( ui_manager, action_group, 0 );

	merge_id = gtk_ui_manager_add_ui_from_file( ui_manager, PKGDATADIR "/nautilus-actions-config-tool.actions", &error );
	if( merge_id == 0 || error ){
		g_warning( "%s: error=%s", thisfn, error->message );
		g_error_free( error );
	}

	toplevel = base_window_get_toplevel( BASE_WINDOW( window ));
	accel_group = gtk_ui_manager_get_accel_group( ui_manager );
	gtk_window_add_accel_group( toplevel, accel_group );

	menubar = gtk_ui_manager_get_widget( ui_manager, "/ui/MainMenubar" );
	vbox = base_window_get_widget( BASE_WINDOW( window ), "MenubarVBox" );
	gtk_box_pack_start( GTK_BOX( vbox ), menubar, FALSE, FALSE, 0 );

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( toplevel ),
			"delete-event",
			G_CALLBACK( on_delete_event ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_LIST_COUNT_UPDATED,
			G_CALLBACK( on_iactions_list_count_updated ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
			G_CALLBACK( on_iactions_list_selection_changed ));

	base_window_signal_connect(
			BASE_WINDOW( window ),
			G_OBJECT( window ),
			MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES,
			G_CALLBACK( on_update_sensitivities ));

	mis = g_new0( MenubarIndicatorsStruct, 1 );
	g_object_set_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS, mis );
}

/**
 * nact_main_menubar_dispose:
 * @window: this #NactMainWindow window.
 *
 * Release internally allocated resources.
 */
void
nact_main_menubar_dispose( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_dispose";
	MenubarIndicatorsStruct *mis;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	g_free( mis );
}

/*
 * when the IActionsList is refilled, update our internal counters so
 * that we are knowing if we have some exportables
 */
static void
on_iactions_list_count_updated( NactMainWindow *window, gint menus, gint actions, gint profiles )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_iactions_list_count_updated: menus=%u, actions=%u, profiles=%u", menus, actions, profiles );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->list_menus = menus;
	mis->list_actions = actions;
	mis->list_profiles = profiles;
	mis->have_exportables = ( mis->list_actions > 0 );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

/*
 * when the selection changes in IActionsList, see what is selected
 */
static void
on_iactions_list_selection_changed( NactMainWindow *window, GList *selected )
{
	MenubarIndicatorsStruct *mis;

	g_debug( "nact_main_menubar_on_iactions_list_selection_changed: selected=%p (%d)",
			( void * ) selected, g_list_length( selected ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );
	mis->selected_menus = 0;
	mis->selected_actions = 0;
	mis->selected_profiles = 0;
	na_object_item_count_items( selected, &mis->selected_menus, &mis->selected_actions, &mis->selected_profiles, FALSE );
	g_debug( "nact_main_menubar_on_iactions_list_selection_changed: menus=%d, actions=%d, profiles=%d",
			mis->selected_menus, mis->selected_actions, mis->selected_profiles );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_update_sensitivities( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_menubar_on_update_sensitivities";
	MenubarIndicatorsStruct *mis;
	NAObject *item;
	NAObject *profile;
	gboolean has_modified;
	gint count_list;
	gint count_selected;
	gboolean paste_enabled;
	gboolean paste_into_enabled;
	gboolean clipboard_is_empty;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &item,
			TAB_UPDATABLE_PROP_EDITED_PROFILE, &profile,
			NULL );
	g_return_if_fail( !item || NA_IS_OBJECT_ITEM( item ));
	g_return_if_fail( !profile || NA_IS_OBJECT_PROFILE( profile ));

	has_modified = nact_main_window_has_modified_items( window );
	count_list = mis->list_menus + mis->list_actions + mis->list_profiles;
	count_selected = mis->selected_menus + mis->selected_actions + mis->selected_profiles;

	clipboard_is_empty = ( mis->clipboard_menus + mis->clipboard_actions + mis->clipboard_profiles == 0 );

	paste_enabled = FALSE;
	if( !clipboard_is_empty ){
		if( mis->clipboard_profiles ){
			paste_enabled = profile && NA_IS_OBJECT_PROFILE( profile );
		} else {
			paste_enabled = ( item != NULL );
		}
	}

	paste_into_enabled = FALSE;
	if( !clipboard_is_empty ){
		if( mis->clipboard_profiles ){
			paste_into_enabled = item && NA_IS_OBJECT_ACTION( item );
		} else {
			paste_into_enabled = item && NA_IS_OBJECT_MENU( item );
		}
	}

	/* new menu always enabled */
	/* new action always enabled */
	/* new profile enabled if selection is relative to only one action */
	enable_item( window, "NewProfileItem", item != NULL && !NA_IS_OBJECT_MENU( item ));
	/* save enabled if at least one item has been modified */
	enable_item( window, "SaveItem", has_modified );
	/* quit always enabled */
	/* cut/copy enabled if selection not empty */
	enable_item( window, "CutItem", count_selected > 0 );
	enable_item( window, "CopyItem", count_selected > 0 );
	/* paste enabled if
	 * - clipboard contains only profiles, and current selection is a profile
	 * - clipboard contains actions or menus, and current selection is a menu or an action */
	enable_item( window, "PasteItem", count_selected <= 1 && paste_enabled );
	/* paste into enabled if
	 * - clipboard has profiles and current item is an action
	 * - or current item is a menu */
	enable_item( window, "PasteIntoItem", count_selected <= 1 && paste_into_enabled );
	/* duplicate/delete enabled if selection not empty */
	enable_item( window, "DuplicateItem", count_selected > 0 );
	enable_item( window, "DeleteItem", count_selected > 0 );
	/* reload items always enabled */
	/* preferences always enabled */
	/* expand all/collapse all requires at least one item in the list */
	enable_item( window, "ExpandAllItem", count_list > 0 );
	enable_item( window, "CollapseAllItem", count_list > 0 );
	/* import item always enabled */
	/* export item enabled if IActionsList store contains actions */
	enable_item( window, "ExportItem", mis->have_exportables );
	/* TODO: help temporarily disabled */
	enable_item( window, "HelpItem", FALSE );
	/* about always enabled */
}

static void
on_new_menu_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectMenu *menu;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	menu = na_object_menu_new();
	items = g_list_prepend( NULL, menu );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items_list( items );
}

static void
on_new_action_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	action = na_object_action_new_with_profile();
	items = g_list_prepend( NULL, action );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items_list( items );
}

static void
on_new_profile_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	NAObjectAction *action;
	NAObjectProfile *profile;
	gchar *name;
	GList *items;

	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_object_get(
			G_OBJECT( window ),
			TAB_UPDATABLE_PROP_EDITED_ACTION, &action,
			NULL );

	profile = na_object_profile_new();

	name = na_object_action_get_new_profile_name( action );
	na_object_set_parent( profile, action );
	na_object_set_id( profile, name );
	na_object_check_edition_status( profile );

	items = g_list_prepend( NULL, profile );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );

	na_object_free_items_list( items );
	g_free( name );
}

/*
 * saving is not only saving modified items, but also saving hierarchy
 * (and order if alpha order is not set)
 *
 * note that we only go down in the hierarchy if parent is valid and not
 * modified (or has been successfully saved)
 */
static void
on_save_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_save_activated";
	GList *items;
	NactApplication *application;
	NAPivot *pivot;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	/* delete the removed actions
	 * so that new actions with same id do not risk to be deleted later
	 */
	nact_main_window_remove_deleted( window );

	items = nact_iactions_list_get_items( NACT_IACTIONS_LIST( window ));
	nact_window_write_level_zero( NACT_WINDOW( window ), items );

	/* recursively save the valid modified items
	 */
	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );
	save_items( window, pivot, items );
	g_list_free( items );

	/* required as selection has not changed
	 */
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );

	/* get ride of notification messages of IOProviders
	 */
	na_ipivot_consumer_delay_notify( NA_IPIVOT_CONSUMER( window ));
}

/*
 * only recurse in menus
 */
static void
save_items( NactMainWindow *window, NAPivot *pivot, GList *items )
{
	GList *it;

	for( it = items ; it ; it = it->next ){
		save_item( window, pivot, NA_OBJECT_ITEM( it->data ));
	}
}

/*
 * iterates here on each and every row stored in the tree
 * - do not deal with profiles as they are directly managed by their
 *   action parent
 * - do not deal with not modified, or not valid, items, but allow
 *   for save their subitems
 */
static void
save_item( NactMainWindow *window, NAPivot *pivot, NAObjectItem *item )
{
	NAObjectItem *origin;
	NAObjectItem *dup_pivot;
	GList *subitems;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NA_IS_PIVOT( pivot ));
	g_return_if_fail( NA_IS_OBJECT_ITEM( item ));

	if( NA_IS_OBJECT_MENU( item )){
		subitems = na_object_get_items_list( item );
		save_items( window, pivot, subitems );
	}

	if( na_object_is_modified( item ) &&
		na_object_is_valid( item )){

		if( nact_window_save_item( NACT_WINDOW( window ), item )){

			if( NA_IS_OBJECT_ACTION( item )){
				na_object_action_reset_last_allocated( NA_OBJECT_ACTION( item ));
			}

			/* do not use NA_OBJECT_ITEM macro as this may return a
			 * (valid) NULL value
			 */
			origin = ( NAObjectItem * ) na_object_get_origin( item );

			if( origin ){
				na_pivot_remove_item( pivot, NA_OBJECT( origin ));
			}

			dup_pivot = NA_OBJECT_ITEM( na_object_duplicate( item ));
			na_object_reset_origin( item, dup_pivot );
			g_debug( "save_item: un" );
			na_object_dump( item );
			g_debug( "save_item: deux" );
			na_object_dump( dup_pivot );
			g_debug( "save_item: trois" );
			na_pivot_add_item( pivot, NA_OBJECT( dup_pivot ));

			na_object_check_edition_status( item );
		}
	}
}

static void
on_quit_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_quit_activated";
	gboolean has_modified;

	g_debug( "%s: item=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ) || gtk_action == NULL );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	has_modified = nact_main_window_has_modified_items( window );
	if( !has_modified || nact_window_warn_modified( NACT_WINDOW( window ))){
		g_object_unref( window );
	}
}

/*
 * cuts the visible selection
 * - (tree) get new refs on selected items
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (menu) install in clipboard a copy of selected objects
 * - (tree) remove selected items, unreffing objects
 */
static void
on_cut_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_cut_activated";
	GList *items;
	NactClipboard *clipboard;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	nact_main_window_move_to_deleted( window, items );
	clipboard = nact_main_window_get_clipboard( window );
	nact_clipboard_primary_set( clipboard, items, CLIPBOARD_MODE_CUT );
	update_clipboard_counters( window );
	nact_iactions_list_delete( NACT_IACTIONS_LIST( window ), items );

	/* do not unref selected items as the list has been concatenated
	 * to main_deleted
	 */
	/*g_list_free( items );*/
}

/*
 * copies the visible selection
 * - (tree) get new refs on selected items
 * - (menu) install in clipboard a copy of selected objects
 *          renumbering actions/menus id to ensure unicity at paste time
 * - (menu) release refs on selected items
 * - (menu) refresh actions sensitivy (as selection doesn't change)
 */
static void
on_copy_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_copy_activated";
	GList *items;
	NactClipboard *clipboard;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	clipboard = nact_main_window_get_clipboard( window );
	nact_clipboard_primary_set( clipboard, items, CLIPBOARD_MODE_COPY );
	update_clipboard_counters( window );
	na_object_free_items_list( items );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

/*
 * pastes the current content of the clipboard at the current position
 * (same path, same level)
 * - (menu) get from clipboard a copy of installed items
 *          the clipboard will return a new copy
 *          and renumber its own data for allowing a new paste
 * - (tree) insert new items, the tree store will ref them
 *          attaching each item to its parent
 *          recursively checking edition status of the topmost parent
 *          selecting the first item at end
 * - (menu) unreffing the copy got from clipboard
 */
static void
on_paste_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_paste_activated";
	GList *items;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );

	items = prepare_for_paste( window );
	nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), items, NULL );
	na_object_free_items_list( items );
}

/*
 * pastes the current content of the clipboard as the first child of
 * currently selected item
 * - (menu) get from clipboard a copy of installed items
 *          the clipboard will return a new copy
 *          and renumber its own data for allowing a new paste
 * - (tree) insert new items, the tree store will ref them
 *          attaching each item to its parent
 *          recursively checking edition status of the topmost parent
 *          selecting the first item at end
 * - (menu) unreffing the copy got from clipboard
 */
static void
on_paste_into_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_paste_into_activated";
	GList *items;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );

	items = prepare_for_paste( window );
	nact_iactions_list_insert_into( NACT_IACTIONS_LIST( window ), items );
	na_object_free_items_list( items );
}

static GList *
prepare_for_paste( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_prepare_for_paste";
	GList *items, *it;
	NactClipboard *clipboard;
	NAObjectAction *action;
	gboolean renumber;
	NactApplication *application;
	NAPivot *pivot;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	clipboard = nact_main_window_get_clipboard( window );
	items = nact_clipboard_primary_get( clipboard, &renumber );
	action = NULL;

	/* if pasted items are profiles, then setup the target action
	 */
	for( it = items ; it ; it = it->next ){

		if( NA_IS_OBJECT_PROFILE( it->data )){
			if( !action ){
				g_object_get( G_OBJECT( window ), TAB_UPDATABLE_PROP_EDITED_ACTION, &action, NULL );
				g_return_val_if_fail( NA_IS_OBJECT_ACTION( action ), NULL );
			}
		}

		na_object_prepare_for_paste( it->data, pivot, renumber, action );
	}

	g_debug( "%s: action=%p (%s)",
			thisfn, ( void * ) action, action ? G_OBJECT_TYPE_NAME( action ): "(null)" );

	return( items );
}

/*
 * duplicate is just as paste, with the difference that content comes
 * from the current selection, instead of coming from the clipboard
 *
 * this is nonetheless a bit more complicated because when we duplicate
 * some items (e.g. a multiple selection), we expect to see the new
 * items just besides the original ones...
 */
static void
on_duplicate_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_duplicate_activated";
	NactApplication *application;
	NAPivot *pivot;
	NAObjectAction *action;
	GList *items, *it;
	GList *dup;
	NAObject *obj;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	pivot = nact_application_get_pivot( application );

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		obj = NA_OBJECT( na_object_duplicate( it->data ));
		action = NULL;

		/* duplicating a profile
		 * as we insert in sibling mode, the parent doesn't change
		 */
		if( NA_IS_OBJECT_PROFILE( obj )){
			action = NA_OBJECT_ACTION( na_object_get_parent( NA_OBJECT_PROFILE( obj )));
		}

		na_object_prepare_for_paste( obj, pivot, TRUE, action );
		na_object_set_origin( obj, NULL );
		dup = g_list_prepend( NULL, obj );
		nact_iactions_list_insert_items( NACT_IACTIONS_LIST( window ), dup, it->data );
		na_object_free_items_list( dup );
	}

	na_object_free_items_list( items );
}

/*
 * deletes the visible selection
 * - (tree) get new refs on selected items
 * - (tree) remove selected items, unreffing objects
 * - (main) add selected items to main list of deleted,
 *          moving newref from list_from_tree to main_list_of_deleted
 * - (tree) select next row (if any, or previous if any, or none)
 */
static void
on_delete_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_delete_activated";
	GList *items;
	GList *it;

	g_debug( "%s: gtk_action=%p, window=%p", thisfn, ( void * ) gtk_action, ( void * ) window );
	g_return_if_fail( GTK_IS_ACTION( gtk_action ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	items = nact_iactions_list_get_selected_items( NACT_IACTIONS_LIST( window ));
	for( it = items ; it ; it = it->next ){
		g_debug( "%s: item=%p (%s)", thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ));
	}
	nact_main_window_move_to_deleted( window, items );
	nact_iactions_list_delete( NACT_IACTIONS_LIST( window ), items );

	/* do not unref selected items as the list has been concatenated
	 * to main_deleted
	 */
	/*g_list_free( items );*/
}

/*
 * as we are coming from cut or copy to clipboard, report selection
 * counters to clipboard ones
 */
static void
update_clipboard_counters( NactMainWindow *window )
{
	MenubarIndicatorsStruct *mis;

	mis = ( MenubarIndicatorsStruct * ) g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_INDICATORS );

	mis->clipboard_menus = mis->selected_menus;
	mis->clipboard_actions = mis->selected_actions;
	mis->clipboard_profiles = mis->selected_profiles;

	g_debug( "nact_main_menubar_update_clipboard_counters: menus=%d, actions=%d, profiles=%d",
			mis->clipboard_menus, mis->clipboard_actions, mis->clipboard_profiles );

	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES, NULL );
}

static void
on_reload_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_main_window_reload( window );
}

static void
on_preferences_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_preferences_editor_run( BASE_WINDOW( window ));
}

static void
on_expand_all_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_expand_all( NACT_IACTIONS_LIST( window ));
}

static void
on_collapse_all_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_iactions_list_collapse_all( NACT_IACTIONS_LIST( window ));
}

static void
on_import_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_assistant_import_run( BASE_WINDOW( window ));
}

static void
on_export_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	nact_assistant_export_run( BASE_WINDOW( window ));
}

static void
on_help_activated( GtkAction *gtk_action, NactMainWindow *window )
{
}

static void
on_about_activated( GtkAction *gtk_action, NactMainWindow *window )
{
	na_iabout_display( NA_IABOUT( window ));
}

static void
enable_item( NactMainWindow *window, const gchar *name, gboolean enabled )
{
	GtkActionGroup *group;
	GtkAction *action;

	group = g_object_get_data( G_OBJECT( window ), MENUBAR_PROP_ACTIONS_GROUP );
	action = gtk_action_group_get_action( group, name );
	gtk_action_set_sensitive( action, enabled );
}

static gboolean
on_delete_event( GtkWidget *toplevel, GdkEvent *event, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_delete_event";

	g_debug( "%s: toplevel=%p, event=%p, window=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) window );

	on_quit_activated( NULL, window );

	return( TRUE );
}

/*
 * this callback is declared when attaching ui_manager and actions_group
 * as data to the window ; it is so triggered on NactMainWindow::finalize()
 */
static void
on_destroy_callback( gpointer data )
{
	static const gchar *thisfn = "nact_main_menubar_on_destroy_callback";

	g_debug( "%s: data=%p (%s)", thisfn,
			( void * ) data, G_OBJECT_TYPE_NAME( data ));

	g_object_unref( G_OBJECT( data ));
}

/*
 * gtk_activatable_get_related_action() and gtk_action_get_tooltip()
 * are only available starting with Gtk 2.16
 */
static void
on_menu_item_selected( GtkMenuItem *proxy, NactMainWindow *window )
{
	GtkAction *action;
	gchar *tooltip;

	/*g_debug( "nact_main_menubar_on_menu_item_selected: proxy=%p (%s), window=%p (%s)",
			( void * ) proxy, G_OBJECT_TYPE_NAME( proxy ),
			( void * ) window, G_OBJECT_TYPE_NAME( window ));*/

	tooltip = NULL;

#ifdef GTK_HAS_ACTIVATABLE
	action = gtk_activatable_get_related_action( GTK_ACTIVATABLE( proxy ));
	if( action ){
		tooltip = ( gchar * ) gtk_action_get_tooltip( action );
	}
#else
	action = GTK_ACTION( g_object_get_data( G_OBJECT( proxy ), MENUBAR_PROP_ITEM_ACTION ));
	if( action ){
		g_object_get( G_OBJECT( action ), "tooltip", &tooltip, NULL );
	}
#endif

	if( tooltip ){
		nact_main_statusbar_display_status( window, MENUBAR_PROP_STATUS_CONTEXT, tooltip );
	}

#ifndef GTK_HAS_ACTIVATABLE
	g_free( tooltip );
#endif
}

static void
on_menu_item_deselected( GtkMenuItem *proxy, NactMainWindow *window )
{
	nact_main_statusbar_hide_status( window, MENUBAR_PROP_STATUS_CONTEXT );
}

static void
on_proxy_connect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_menubar_on_proxy_connect";

	g_debug( "%s: action_group=%p (%s), action=%p (%s), proxy=%p (%s), window=%p (%s)",
			thisfn,
			( void * ) action_group, G_OBJECT_TYPE_NAME( action_group ),
			( void * ) action, G_OBJECT_TYPE_NAME( action ),
			( void * ) proxy, G_OBJECT_TYPE_NAME( proxy ),
			( void * ) window, G_OBJECT_TYPE_NAME( window ));

	if( GTK_IS_MENU_ITEM( proxy )){

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( proxy ),
				"select",
				G_CALLBACK( on_menu_item_selected ));

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( proxy ),
				"deselect",
				G_CALLBACK( on_menu_item_deselected ));

#ifndef GTK_HAS_ACTIVATABLE
		g_object_set_data( G_OBJECT( proxy ), MENUBAR_PROP_ITEM_ACTION, action );
#endif
	}
}

static void
on_proxy_disconnect( GtkActionGroup *action_group, GtkAction *action, GtkWidget *proxy, NactMainWindow *window )
{
	/* signal handlers will be automagically disconnected on NactWindow::dispose */
}
