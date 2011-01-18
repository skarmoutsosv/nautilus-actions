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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <api/na-object-api.h>

#include <core/na-iabout.h>
#include <core/na-ipivot-consumer.h>
#include <core/na-iprefs.h>
#include <core/na-pivot.h>

#include "base-iprefs.h"
#include "nact-application.h"
#include "nact-iactions-list.h"
#include "nact-iaction-tab.h"
#include "nact-icommand-tab.h"
#include "nact-ibasenames-tab.h"
#include "nact-imimetypes-tab.h"
#include "nact-ifolders-tab.h"
#include "nact-ischemes-tab.h"
#include "nact-icapabilities-tab.h"
#include "nact-ienvironment-tab.h"
#include "nact-iexecution-tab.h"
#include "nact-iproperties-tab.h"
#include "nact-main-tab.h"
#include "nact-main-menubar.h"
#include "nact-main-menubar-file.h"
#include "nact-main-statusbar.h"
#include "nact-marshal.h"
#include "nact-main-window.h"
#include "nact-confirm-logout.h"
#include "nact-sort-buttons.h"

/* private class data
 */
struct NactMainWindowClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactMainWindowPrivate {
	gboolean         dispose_has_run;

	/* TODO: this will have to be replaced with undo-manager */
	GList           *deleted;

	/**
	 * Currently selected action or menu.
	 *
	 * This is the action or menu which is displayed in tabs Action/Menu
	 * and Properties ; it may be different of the exact row being currently
	 * selected, e.g. when a sub-profile is edited.
	 *
	 * Can be null, and this implies that @selected_profile is also null,
	 * e.g. when the list is empty or in case of multiple selection.
	 *
	 * 'editable' property is computed on selection change;
	 * This is the real writability status of the item at this time.
	 */
	NAObjectItem    *selected_item;
	gboolean         editable;
	gint             reason;

	/**
	 * Currently selected profile.
	 *
	 * This is the profile which is displayed in tab Command;
	 * it may be different of the exact row being currently selected.
	 *
	 * Can be null if @selected_item is a menu, or an action with more
	 * than one profile and action is selected, or an action without
	 * any profile, or the list is empty, or in case of multiple selection.
	 */
	NAObjectProfile *selected_profile;

	/**
	 * The convenience clipboard object.
	 */
	NactClipboard   *clipboard;
};

/* action properties
 * these are set when selection changes as an optimization try
 */
enum {
	PROP_EDITED_ITEM = 1,
	PROP_EDITED_PROFILE,
	PROP_EDITABLE,
	PROP_REASON
};

/* signals
 */
enum {
	PROVIDER_CHANGED,
	SELECTION_CHANGED,
	ITEM_UPDATED,
	UPDATE_SENSITIVITIES,
	ORDER_CHANGED,
	LAST_SIGNAL
};

static NactWindowClass *st_parent_class = NULL;
static gint             st_signals[ LAST_SIGNAL ] = { 0 };

static GType    register_type( void );
static void     class_init( NactMainWindowClass *klass );
static void     iactions_list_iface_init( NactIActionsListInterface *iface );
static void     iaction_tab_iface_init( NactIActionTabInterface *iface );
static void     icommand_tab_iface_init( NactICommandTabInterface *iface );
static void     ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface );
static void     imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface );
static void     ifolders_tab_iface_init( NactIFoldersTabInterface *iface );
static void     ischemes_tab_iface_init( NactISchemesTabInterface *iface );
static void     icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface );
static void     ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface );
static void     iexecution_tab_iface_init( NactIExecutionTabInterface *iface );
static void     iproperties_tab_iface_init( NactIPropertiesTabInterface *iface );
static void     iabout_iface_init( NAIAboutInterface *iface );
static void     ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface );
static void     iprefs_base_iface_init( BaseIPrefsInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static gboolean actually_delete_item( NactMainWindow *window, NAObject *item, NAUpdater *updater, GList **not_deleted, GSList **messages );

static gchar   *base_get_toplevel_name( const BaseWindow *window );
static gchar   *base_get_iprefs_window_id( const BaseWindow *window );
static gboolean base_is_willing_to_quit( const BaseWindow *window );
static void     on_base_initial_load_toplevel( NactMainWindow *window, gpointer user_data );
static void     on_base_runtime_init_toplevel( NactMainWindow *window, gpointer user_data );
static void     on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data );

static void     on_main_window_level_zero_order_changed( NactMainWindow *window, gpointer user_data );
static void     on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items );
static void     on_iactions_list_status_changed( NactMainWindow *window, gpointer user_data );
static void     raz_main_properties( NactMainWindow *window );
static void     setup_current_selection( NactMainWindow *window, NAObjectId *selected_row );
static void     setup_dialog_title( NactMainWindow *window );
static void     setup_writability_status( NactMainWindow *window );
static gchar   *iactions_list_get_treeview_name( NactIActionsList *instance );

static void     on_tab_updatable_item_updated( NactMainWindow *window, gpointer user_data, gboolean force_display );

static gboolean confirm_for_giveup_from_menu( NactMainWindow *window );
static gboolean confirm_for_giveup_from_pivot( NactMainWindow *window );
static void     install_autosave( NactMainWindow *window );
static void     ipivot_consumer_on_autosave_changed( NAIPivotConsumer *instance, gboolean enabled, guint period );
static void     ipivot_consumer_on_items_changed( NAIPivotConsumer *instance, gpointer user_data );
static void     ipivot_consumer_on_display_order_changed( NAIPivotConsumer *instance, gint order_mode );
static void     ipivot_consumer_on_io_provider_prefs_changed( NAIPivotConsumer *instance );
static void     ipivot_consumer_on_mandatory_prefs_changed( NAIPivotConsumer *instance );
static void     update_ui_after_provider_change( NactMainWindow *window );
static void     reload( NactMainWindow *window );

static gchar     *iabout_get_application_name( NAIAbout *instance );
static GtkWindow *iabout_get_toplevel( NAIAbout *instance );

GType
nact_main_window_get_type( void )
{
	static GType window_type = 0;

	if( !window_type ){
		window_type = register_type();
	}

	return( window_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_main_window_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactMainWindowClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactMainWindow ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iactions_list_iface_info = {
		( GInterfaceInitFunc ) iactions_list_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iaction_tab_iface_info = {
		( GInterfaceInitFunc ) iaction_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icommand_tab_iface_info = {
		( GInterfaceInitFunc ) icommand_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ibasenames_tab_iface_info = {
		( GInterfaceInitFunc ) ibasenames_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo imimetypes_tab_iface_info = {
		( GInterfaceInitFunc ) imimetypes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ifolders_tab_iface_info = {
		( GInterfaceInitFunc ) ifolders_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ischemes_tab_iface_info = {
		( GInterfaceInitFunc ) ischemes_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo icapabilities_tab_iface_info = {
		( GInterfaceInitFunc ) icapabilities_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ienvironment_tab_iface_info = {
		( GInterfaceInitFunc ) ienvironment_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iexecution_tab_iface_info = {
		( GInterfaceInitFunc ) iexecution_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iproperties_tab_iface_info = {
		( GInterfaceInitFunc ) iproperties_tab_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iabout_iface_info = {
		( GInterfaceInitFunc ) iabout_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ipivot_consumer_iface_info = {
		( GInterfaceInitFunc ) ipivot_consumer_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo iprefs_base_iface_info = {
		( GInterfaceInitFunc ) iprefs_base_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( NACT_WINDOW_TYPE, "NactMainWindow", &info, 0 );

	g_type_add_interface_static( type, NACT_IACTIONS_LIST_TYPE, &iactions_list_iface_info );

	g_type_add_interface_static( type, NACT_IACTION_TAB_TYPE, &iaction_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICOMMAND_TAB_TYPE, &icommand_tab_iface_info );

	g_type_add_interface_static( type, NACT_IBASENAMES_TAB_TYPE, &ibasenames_tab_iface_info );

	g_type_add_interface_static( type, NACT_IMIMETYPES_TAB_TYPE, &imimetypes_tab_iface_info );

	g_type_add_interface_static( type, NACT_IFOLDERS_TAB_TYPE, &ifolders_tab_iface_info );

	g_type_add_interface_static( type, NACT_ISCHEMES_TAB_TYPE, &ischemes_tab_iface_info );

	g_type_add_interface_static( type, NACT_ICAPABILITIES_TAB_TYPE, &icapabilities_tab_iface_info );

	g_type_add_interface_static( type, NACT_IENVIRONMENT_TAB_TYPE, &ienvironment_tab_iface_info );

	g_type_add_interface_static( type, NACT_IEXECUTION_TAB_TYPE, &iexecution_tab_iface_info );

	g_type_add_interface_static( type, NACT_IPROPERTIES_TAB_TYPE, &iproperties_tab_iface_info );

	g_type_add_interface_static( type, NA_IABOUT_TYPE, &iabout_iface_info );

	g_type_add_interface_static( type, NA_IPIVOT_CONSUMER_TYPE, &ipivot_consumer_iface_info );

	g_type_add_interface_static( type, BASE_IPREFS_TYPE, &iprefs_base_iface_info );

	return( type );
}

static void
class_init( NactMainWindowClass *klass )
{
	static const gchar *thisfn = "nact_main_window_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_pointer(
			TAB_UPDATABLE_PROP_SELECTED_ITEM,
			"Edited NAObjectItem",
			"A pointer to the edited NAObjectItem, an action or a menu",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_EDITED_ITEM, spec );

	spec = g_param_spec_pointer(
			TAB_UPDATABLE_PROP_SELECTED_PROFILE,
			"Edited NAObjectProfile",
			"A pointer to the edited NAObjectProfile",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_EDITED_PROFILE, spec );

	spec = g_param_spec_boolean(
			TAB_UPDATABLE_PROP_EDITABLE,
			"Editable item ?",
			"Whether the item will be able to be updated against its I/O provider", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_EDITABLE, spec );

	spec = g_param_spec_int(
			TAB_UPDATABLE_PROP_REASON,
			"No edition reason",
			"Why is this item not editable", 0, 255, 0,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, PROP_REASON, spec );

	klass->private = g_new0( NactMainWindowClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->get_toplevel_name = base_get_toplevel_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->is_willing_to_quit = base_is_willing_to_quit;

	/**
	 * nact-tab-updatable-provider-changed:
	 *
	 * This signal is emitted at save time, when we are noticing that
	 * the save operation has led to a modification of the I/O provider.
	 * This signal may be caught by a tab in order to display the
	 * new provider's name.
	 */
	st_signals[ PROVIDER_CHANGED ] = g_signal_new(
			TAB_UPDATABLE_SIGNAL_PROVIDER_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * main-window-selection-changed:
	 *
	 * This signal is emitted by this main window, in response of a
	 * change of the selection in IActionsList, after having updated
	 * its properties.
	 * Notebook tabs should connect to this signal and update their
	 * display to reflect the content of the new selection (if applyable).
	 *
	 * Note also that, where this main window will receive from
	 * IActionsList the full list of currently selected items, this
	 * signal only carries to the tabs the count of selected items.
	 *
	 * See #iactions_list_selection_changed().
	 */
	st_signals[ SELECTION_CHANGED ] = g_signal_new(
			MAIN_WINDOW_SIGNAL_SELECTION_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * nact-tab-updatable-item-updated:
	 *
	 * This signal is emitted by the notebook tabs, when any property
	 * of an item has been modified.
	 *
	 * This main window is rather the only consumer of this message,
	 * does its tricks (title, etc.), and then reforward an item-updated
	 * message to IActionsList.
	 */
	st_signals[ ITEM_UPDATED ] = g_signal_new(
			TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			nact_marshal_VOID__POINTER_BOOLEAN,
			G_TYPE_NONE,
			2,
			G_TYPE_POINTER,
			G_TYPE_BOOLEAN );

	/**
	 * main-window-update-sensitivities:
	 *
	 * This signal is emitted each time a user interaction may led the
	 * action sensitivities to be updated.
	 */
	st_signals[ UPDATE_SENSITIVITIES ] = g_signal_new(
			MAIN_WINDOW_SIGNAL_UPDATE_ACTION_SENSITIVITIES,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );

	/**
	 * main-window-level-zero-order-changed:
	 *
	 * This signal is emitted each time a user interaction may led the
	 * action sensitivities to be updated.
	 */
	st_signals[ ORDER_CHANGED ] = g_signal_new(
			MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED,
			G_TYPE_OBJECT,
			G_SIGNAL_RUN_LAST,
			0,					/* no default handler */
			NULL,
			NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1,
			G_TYPE_POINTER );
}

static void
iactions_list_iface_init( NactIActionsListInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iactions_list_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_treeview_name = iactions_list_get_treeview_name;
}

static void
iaction_tab_iface_init( NactIActionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iaction_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
icommand_tab_iface_init( NactICommandTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icommand_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ibasenames_tab_iface_init( NactIBasenamesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ibasenames_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
imimetypes_tab_iface_init( NactIMimetypesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_imimetypes_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ifolders_tab_iface_init( NactIFoldersTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ifolders_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ischemes_tab_iface_init( NactISchemesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ischemes_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
icapabilities_tab_iface_init( NactICapabilitiesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_icapabilities_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
ienvironment_tab_iface_init( NactIEnvironmentTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ienvironment_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iexecution_tab_iface_init( NactIExecutionTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iexecution_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iproperties_tab_iface_init( NactIPropertiesTabInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iproperties_tab_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
iabout_iface_init( NAIAboutInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iabout_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_application_name = iabout_get_application_name;
	iface->get_toplevel = iabout_get_toplevel;
}

static void
ipivot_consumer_iface_init( NAIPivotConsumerInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->on_autosave_changed = ipivot_consumer_on_autosave_changed;
	iface->on_create_root_menu_changed = NULL;
	iface->on_display_about_changed = NULL;
	iface->on_display_order_changed = ipivot_consumer_on_display_order_changed;
	iface->on_io_provider_prefs_changed = ipivot_consumer_on_io_provider_prefs_changed;
	iface->on_items_changed = ipivot_consumer_on_items_changed;
	iface->on_mandatory_prefs_changed = ipivot_consumer_on_mandatory_prefs_changed;
}

static void
iprefs_base_iface_init( BaseIPrefsInterface *iface )
{
	static const gchar *thisfn = "nact_main_window_iprefs_base_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_main_window_instance_init";
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_MAIN_WINDOW( instance );

	self->private = g_new0( NactMainWindowPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_toplevel ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_toplevel ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			TAB_UPDATABLE_SIGNAL_ITEM_UPDATED,
			G_CALLBACK( on_tab_updatable_item_updated ));

	self->private->dispose_has_run = FALSE;

	na_ipivot_consumer_allow_notify( NA_IPIVOT_CONSUMER( instance ), TRUE, 0 );
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case PROP_EDITED_ITEM:
				g_value_set_pointer( value, self->private->selected_item );
				break;

			case PROP_EDITED_PROFILE:
				g_value_set_pointer( value, self->private->selected_profile );
				break;

			case PROP_EDITABLE:
				g_value_set_boolean( value, self->private->editable );
				break;

			case PROP_REASON:
				g_value_set_int( value, self->private->reason );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( object ));
	self = NACT_MAIN_WINDOW( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case PROP_EDITED_ITEM:
				self->private->selected_item = g_value_get_pointer( value );
				break;

			case PROP_EDITED_PROFILE:
				self->private->selected_profile = g_value_get_pointer( value );
				break;

			case PROP_EDITABLE:
				self->private->editable = g_value_get_boolean( value );
				break;

			case PROP_REASON:
				self->private->reason = g_value_get_int( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_dispose";
	NactMainWindow *self;
	GtkWidget *pane;
	gint pos;
	GList *it;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_MAIN_WINDOW( window );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( self )));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));

		pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
		pos = gtk_paned_get_position( GTK_PANED( pane ));
		na_settings_set_uint( settings, NA_IPREFS_MAIN_PANED, pos );

		for( it = self->private->deleted ; it ; it = it->next ){
			g_debug( "nact_main_window_instance_dispose: deleted=%p (%s)", ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ));
		}
		na_object_unref_items( self->private->deleted );

		nact_iactions_list_dispose( NACT_IACTIONS_LIST( window ));
		nact_sort_buttons_dispose( self );
		nact_iaction_tab_dispose( NACT_IACTION_TAB( window ));
		nact_icommand_tab_dispose( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_dispose( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_dispose( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_dispose( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_dispose( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_dispose( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_dispose( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_dispose( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_dispose( NACT_IPROPERTIES_TAB( window ));
		nact_main_menubar_dispose( self );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( window );
		}
	}
}

static void
instance_finalize( GObject *window )
{
	static const gchar *thisfn = "nact_main_window_instance_finalize";
	NactMainWindow *self;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	g_debug( "%s: window=%p (%s)", thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ));

	self = NACT_MAIN_WINDOW( window );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( window );
	}
}

/**
 * Returns a newly allocated NactMainWindow object.
 */
NactMainWindow *
nact_main_window_new( BaseApplication *application )
{
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	return( g_object_new( NACT_MAIN_WINDOW_TYPE, BASE_WINDOW_PROP_APPLICATION, application, NULL ));
}

/**
 * nact_main_window_get_clipboard:
 * @window: this #NactMainWindow instance.
 *
 * Returns: the #nactClipboard convenience object.
 */
NactClipboard *
nact_main_window_get_clipboard( const NactMainWindow *window )
{
	NactClipboard *clipboard = NULL;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), NULL );

	if( !window->private->dispose_has_run ){
		clipboard = window->private->clipboard;
	}

	return( clipboard );
}

/**
 * nact_main_window_get_item:
 * @window: this #NactMainWindow instance.
 * @id: the identifier to check for existancy.
 *
 * Returns: a pointer to the #NAObjectItem if it exists in the current
 * tree, or %NULL else.
 *
 * Do not check in NAPivot: actions which are not displayed in the user
 * interface are not considered as existing.
 *
 * Also note that the returned object may be an action, but also a menu.
 */
NAObjectItem *
nact_main_window_get_item( const NactMainWindow *window, const gchar *id )
{
	NAObjectItem *exists;
	NactApplication *application;
	NAUpdater *updater;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );

	exists = NULL;

	if( !window->private->dispose_has_run ){

		/* leave here this dead code, in case I change of opinion later */
		if( 0 ){
			application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
			updater = nact_application_get_updater( application );
			exists = na_pivot_get_item( NA_PIVOT( updater ), id );
		}

		if( !exists ){
			exists = NA_OBJECT_ITEM( nact_iactions_list_bis_get_item( NACT_IACTIONS_LIST( window ), id ));
		}
	}

	return( exists );
}

/**
 * nact_main_window_has_modified_items:
 * @window: this #NactMainWindow instance.
 *
 * Returns: %TRUE if there is at least one modified item in IActionsList.
 *
 * Note that exact count of modified actions is subject to some
 * approximation:
 * 1. counting the modified actions currently in the list is ok
 * 2. but what about deleted actions ?
 *    we can create any new actions, deleting them, and so on
 *    if we have eventually deleted all newly created actions, then the
 *    final count of modified actions should be zero... don't it ?
 */
gboolean
nact_main_window_has_modified_items( const NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_has_modified_items";
	GList *ia;
	gint count_deleted = 0;
	gboolean has_modified = FALSE;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );
	g_return_val_if_fail( NACT_IS_IACTIONS_LIST( window ), FALSE );

	if( !window->private->dispose_has_run ){

		for( ia = window->private->deleted ; ia ; ia = ia->next ){
			if( na_object_get_origin( NA_OBJECT( ia->data )) != NULL ){
				count_deleted += 1;
			}
		}
		g_debug( "%s: count_deleted=%d", thisfn, count_deleted );

		has_modified = nact_iactions_list_has_modified_items( NACT_IACTIONS_LIST( window ));
		g_debug( "%s: has_modified=%s", thisfn, has_modified ? "True":"False" );
	}

	return( count_deleted > 0 || has_modified || nact_main_menubar_is_level_zero_order_changed( window ));
}

/**
 * nact_main_window_move_to_deleted:
 * @window: this #NactMainWindow instance.
 * @items: list of deleted objects.
 *
 * Adds the given list to the deleted one.
 *
 * Note that we move the ref from @items list to our own deleted list.
 * So that the caller should not try to na_object_free_items_list() the
 * provided list.
 */
void
nact_main_window_move_to_deleted( NactMainWindow *window, GList *items )
{
	static const gchar *thisfn = "nact_main_window_move_to_deleted";
	GList *it;

	g_debug( "%s: window=%p, items=%p (%d items)",
			thisfn, ( void * ) window, ( void * ) items, g_list_length( items ));
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		for( it = items ; it ; it = it->next ){
			g_debug( "%s: %p (%s, ref_count=%d)", thisfn,
					( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), G_OBJECT( it->data )->ref_count );
		}

		window->private->deleted = g_list_concat( window->private->deleted, items );
		g_debug( "%s: main_deleted has %d items", thisfn, g_list_length( window->private->deleted ));
	}
}

/**
 * nact_main_window_reload:
 * @window: this #NactMainWindow instance.
 *
 * Refresh the list of items.
 * If there is some non-yet saved modifications, a confirmation is
 * required before giving up with them.
 */
void
nact_main_window_reload( NactMainWindow *window )
{
	gboolean reload_ok;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		reload_ok = confirm_for_giveup_from_menu( window );

		if( reload_ok ){
			reload( window );
		}
	}
}

/**
 * nact_main_window_remove_deleted:
 * @window: this #NactMainWindow instance.
 * @messages: a pointer to a #GSList of error messages.
 *
 * Removes the deleted items from the underlying I/O storage subsystem.
 *
 * Each item of <structfield>NactMainWindow::private::deleted</structfield>
 * #GList may actually itself be a tree (e.g. a #NAObjectMenu). The embedded
 * items will be recursivelly removed, starting from the root.
 *
 * Returns: %TRUE if all candidate items have been successfully deleted,
 * %FALSE else.
 *
 * @messages #GSList is only filled up in case of an error has occured.
 * If there is no error (nact_main_window_remove_deleted() returns %TRUE),
 * then the caller may safely assume that @messages is returned in the
 * same state that it has been provided.
 */
gboolean
nact_main_window_remove_deleted( NactMainWindow *window, GSList **messages )
{
	gboolean delete_ok = TRUE;
	NactApplication *application;
	NAUpdater *updater;
	GList *it;
	NAObject *item;
	GList *not_deleted;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( window ), FALSE );

	if( !window->private->dispose_has_run ){

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );
		not_deleted = NULL;

		for( it = window->private->deleted ; it ; it = it->next ){
			item = NA_OBJECT( it->data );
			delete_ok = actually_delete_item( window, item, updater, &not_deleted, messages );
		}

		na_object_unref_items( window->private->deleted );
		window->private->deleted = NULL;

		setup_dialog_title( window );

		if( g_list_length( not_deleted )){
			nact_iactions_list_bis_insert_items( NACT_IACTIONS_LIST( window ), not_deleted, NULL );
			na_object_unref_items( not_deleted );
		}
	}

	return( delete_ok );
}

/*
 * If the deleted item is a profile, then do nothing because the parent
 * action has been marked as modified when the profile has been deleted,
 * and thus updated in the storage subsystem as well as in the pivot
 */
static gboolean
actually_delete_item( NactMainWindow *window, NAObject *item, NAUpdater *updater, GList **not_deleted, GSList **messages )
{
	gboolean delete_ok = TRUE;
	GList *items, *it;
	NAObject *origin;
	gchar *msg;

	g_debug( "nact_main_window_actually_delete_item: item=%p (%s)",
			( void * ) item, G_OBJECT_TYPE_NAME( item ));

	if( NA_IS_OBJECT_ITEM( item )){
		msg = NULL;
		delete_ok = nact_window_delete_item( NACT_WINDOW( window ), NA_OBJECT_ITEM( item ), &msg );

		if( !delete_ok ){
			*messages = g_slist_append( *messages, msg );
			*not_deleted = g_list_append( *not_deleted, na_object_ref( item ));
			g_free( msg );

		} else if( NA_IS_OBJECT_MENU( item )){
			items = na_object_get_items( item );
			for( it = items ; delete_ok && it ; it = it->next ){
				delete_ok &= actually_delete_item( window, NA_OBJECT( it->data ), updater, not_deleted, messages );
			}
		}

		if( delete_ok ){
			origin = ( NAObject * ) na_object_get_origin( item );
			if( origin ){
				na_updater_remove_item( updater, origin );
				g_object_unref( origin );
			}
		}
	}

	return( delete_ok );
}

static gchar *
base_get_toplevel_name( const BaseWindow *window )
{
	return( g_strdup( "MainWindow" ));
}

static gchar *
base_get_iprefs_window_id( const BaseWindow *window )
{
	return( g_strdup( NA_IPREFS_MAIN_WINDOW_WSP ));
}

static gboolean
base_is_willing_to_quit( const BaseWindow *window )
{
	static const gchar *thisfn = "nact_main_window_is_willing_to_quit";
	gboolean willing_to;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	willing_to = TRUE;
	if( nact_main_window_has_modified_items( NACT_MAIN_WINDOW( window ))){
		willing_to = nact_confirm_logout_run( NACT_MAIN_WINDOW( window ));
	}

	return( willing_to );
}

/*
 * note that for this NactMainWindow, on_initial_load_toplevel and
 * on_runtime_init_toplevel are equivalent, as there is only one
 * occurrence on this window in the application : closing this window
 * is the same than quitting the application
 */
static void
on_base_initial_load_toplevel( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_initial_load_toplevel";
	gint pos;
	GtkWidget *pane;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
		g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );
		settings = na_pivot_get_settings( NA_PIVOT( updater ));

		pos = na_settings_get_uint( settings, NA_IPREFS_MAIN_PANED, NULL, NULL );
		if( pos ){
			pane = base_window_get_widget( BASE_WINDOW( window ), "MainPaned" );
			gtk_paned_set_position( GTK_PANED( pane ), pos );
		}

		nact_iactions_list_set_management_mode( NACT_IACTIONS_LIST( window ), IACTIONS_LIST_MANAGEMENT_MODE_EDITION );
		nact_iactions_list_initial_load_toplevel( NACT_IACTIONS_LIST( window ));
		nact_sort_buttons_initial_load( window );

		nact_iaction_tab_initial_load_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_initial_load_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_initial_load_toplevel( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_initial_load_toplevel( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_initial_load_toplevel( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_initial_load_toplevel( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_initial_load_toplevel( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_initial_load_toplevel( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_initial_load_toplevel( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_initial_load_toplevel( NACT_IPROPERTIES_TAB( window ));

		nact_main_statusbar_initial_load_toplevel( window );
	}
}

static void
on_base_runtime_init_toplevel( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_runtime_init_toplevel";
	NactApplication *application;
	NAUpdater *updater;
	GList *tree;
	gint order_mode;

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		window->private->clipboard = nact_clipboard_new( BASE_WINDOW( window ));

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( window ),
				IACTIONS_LIST_SIGNAL_SELECTION_CHANGED,
				G_CALLBACK( on_iactions_list_selection_changed ));

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );
		tree = na_pivot_get_items( NA_PIVOT( updater ));
		g_debug( "%s: pivot_tree=%p", thisfn, ( void * ) tree );

		nact_iaction_tab_runtime_init_toplevel( NACT_IACTION_TAB( window ));
		nact_icommand_tab_runtime_init_toplevel( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_runtime_init_toplevel( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_runtime_init_toplevel( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_runtime_init_toplevel( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_runtime_init_toplevel( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_runtime_init_toplevel( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_runtime_init_toplevel( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_runtime_init_toplevel( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_runtime_init_toplevel( NACT_IPROPERTIES_TAB( window ));

		nact_main_menubar_runtime_init( window );

		order_mode = na_iprefs_get_order_mode( NA_PIVOT( updater ), NULL );
		ipivot_consumer_on_display_order_changed( NA_IPIVOT_CONSUMER( window ), order_mode );

		/* fill the IActionsList at last so that all signals are connected
		 */
		nact_iactions_list_runtime_init_toplevel( NACT_IACTIONS_LIST( window ), tree );
		nact_sort_buttons_runtime_init( window );

		/* this to update the title when an item is modified
		 */
		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( window ),
				IACTIONS_LIST_SIGNAL_STATUS_CHANGED,
				G_CALLBACK( on_iactions_list_status_changed ));

		base_window_signal_connect(
				BASE_WINDOW( window ),
				G_OBJECT( window ),
				MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED,
				G_CALLBACK( on_main_window_level_zero_order_changed ));
	}
}

static void
on_base_all_widgets_showed( NactMainWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_on_base_all_widgets_showed";

	g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));
	g_return_if_fail( NACT_IS_IACTIONS_LIST( window ));
	g_return_if_fail( NACT_IS_IACTION_TAB( window ));
	g_return_if_fail( NACT_IS_ICOMMAND_TAB( window ));
	g_return_if_fail( NACT_IS_IBASENAMES_TAB( window ));
	g_return_if_fail( NACT_IS_IMIMETYPES_TAB( window ));
	g_return_if_fail( NACT_IS_IFOLDERS_TAB( window ));
	g_return_if_fail( NACT_IS_ISCHEMES_TAB( window ));
	g_return_if_fail( NACT_IS_IENVIRONMENT_TAB( window ));
	g_return_if_fail( NACT_IS_IEXECUTION_TAB( window ));
	g_return_if_fail( NACT_IS_IPROPERTIES_TAB( window ));

	if( !window->private->dispose_has_run ){

		raz_main_properties( window );

		nact_iaction_tab_all_widgets_showed( NACT_IACTION_TAB( window ));
		nact_icommand_tab_all_widgets_showed( NACT_ICOMMAND_TAB( window ));
		nact_ibasenames_tab_all_widgets_showed( NACT_IBASENAMES_TAB( window ));
		nact_imimetypes_tab_all_widgets_showed( NACT_IMIMETYPES_TAB( window ));
		nact_ifolders_tab_all_widgets_showed( NACT_IFOLDERS_TAB( window ));
		nact_ischemes_tab_all_widgets_showed( NACT_ISCHEMES_TAB( window ));
		nact_icapabilities_tab_all_widgets_showed( NACT_ICAPABILITIES_TAB( window ));
		nact_ienvironment_tab_all_widgets_showed( NACT_IENVIRONMENT_TAB( window ));
		nact_iexecution_tab_all_widgets_showed( NACT_IEXECUTION_TAB( window ));
		nact_iproperties_tab_all_widgets_showed( NACT_IPROPERTIES_TAB( window ));

		nact_iactions_list_all_widgets_showed( NACT_IACTIONS_LIST( window ));
		nact_sort_buttons_all_widgets_showed( window );

		install_autosave( window );
	}
}

static void
on_main_window_level_zero_order_changed( NactMainWindow *window, gpointer user_data )
{
	g_debug( "nact_main_window_on_main_window_level_zero_order_changed" );

	setup_dialog_title( window );
}

/*
 * iactions_list_selection_changed:
 * @window: this #NactMainWindow instance.
 * @selected_items: the currently selected items in ActionsList
 */
static void
on_iactions_list_selection_changed( NactIActionsList *instance, GSList *selected_items )
{
	static const gchar *thisfn = "nact_main_window_on_iactions_list_selection_changed";
	NactMainWindow *window;
	gint count;

	count = g_slist_length( selected_items );

	g_debug( "%s: instance=%p, selected_items=%p, count=%d",
			thisfn, ( void * ) instance, ( void * ) selected_items, count );

	window = NACT_MAIN_WINDOW( instance );

	if( window->private->dispose_has_run ){
		return;
	}

	raz_main_properties( window );

	if( count == 1 ){
		g_return_if_fail( NA_IS_OBJECT_ID( selected_items->data ));
		setup_current_selection( window, NA_OBJECT_ID( selected_items->data ));
		setup_writability_status( window );
	}

	setup_dialog_title( window );
	g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_SELECTION_CHANGED, GINT_TO_POINTER( count ));
}

static void
on_iactions_list_status_changed( NactMainWindow *window, gpointer user_data )
{
	g_debug( "nact_main_window_on_iactions_list_status_changed" );

	setup_dialog_title( window );
}

static void
raz_main_properties( NactMainWindow *window )
{
	window->private->selected_item = NULL;
	window->private->selected_profile = NULL;
	window->private->editable = FALSE;
	window->private->reason = 0;

	nact_main_statusbar_set_locked( window, FALSE, 0 );
}

/*
 * enter after raz_properties
 * only called when only one selected row
 */
static void
setup_current_selection( NactMainWindow *window, NAObjectId *selected_row )
{
	guint nb_profiles;
	GList *profiles;

	if( NA_IS_OBJECT_PROFILE( selected_row )){
		window->private->selected_profile = NA_OBJECT_PROFILE( selected_row );
		window->private->selected_item = NA_OBJECT_ITEM( na_object_get_parent( selected_row ));

	} else {
		g_return_if_fail( NA_IS_OBJECT_ITEM( selected_row ));
		window->private->selected_item = NA_OBJECT_ITEM( selected_row );

		if( NA_IS_OBJECT_ACTION( selected_row )){
			nb_profiles = na_object_get_items_count( selected_row );

			if( nb_profiles == 1 ){
				profiles = na_object_get_items( selected_row );
				window->private->selected_profile = NA_OBJECT_PROFILE( profiles->data );
			}
		}
	}
}

/*
 * the title bar of the main window brings up three informations:
 * - the name of the application
 * - the name of the currently selected item if there is only one
 * - an asterisk if anything has been modified
 */
static void
setup_dialog_title( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_setup_dialog_title";
	GtkWindow *toplevel;
	NactApplication *application;
	gchar *title;
	gchar *label;
	gchar *tmp;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	title = base_application_get_application_name( BASE_APPLICATION( application ));

	if( window->private->selected_item ){
		label = na_object_get_label( window->private->selected_item );
		tmp = g_strdup_printf( "%s - %s", label, title );
		g_free( label );
		g_free( title );
		title = tmp;
	}

	if( nact_main_window_has_modified_items( window )){
		tmp = g_strdup_printf( "*%s", title );
		g_free( title );
		title = tmp;
	}

	toplevel = base_window_get_toplevel( BASE_WINDOW( window ));
	gtk_window_set_title( toplevel, title );
	g_free( title );
}

static void
setup_writability_status( NactMainWindow *window )
{
	NactApplication *application;
	NAUpdater *updater;

	g_return_if_fail( NA_IS_OBJECT_ITEM( window->private->selected_item ));

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	window->private->editable = na_updater_is_item_writable( updater, window->private->selected_item, &window->private->reason );
	nact_main_statusbar_set_locked( window, !window->private->editable, window->private->reason );
}

static gchar *
iactions_list_get_treeview_name( NactIActionsList *instance )
{
	gchar *name = NULL;

	g_return_val_if_fail( NACT_IS_MAIN_WINDOW( instance ), NULL );

	name = g_strdup( "ActionsList" );

	return( name );
}

static void
on_tab_updatable_item_updated( NactMainWindow *window, gpointer user_data, gboolean force_display )
{
	/*static const gchar *thisfn = "nact_main_window_on_tab_updatable_item_updated";*/

	/*g_debug( "%s: window=%p, user_data=%p", thisfn, ( void * ) window, ( void * ) user_data );*/
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){
	}
}

/*
 * requires a confirmation from the user when is has asked for reloading
 * the actions via the Edit menu
 */
static gboolean
confirm_for_giveup_from_menu( NactMainWindow *window )
{
	gboolean reload_ok = TRUE;
	gchar *first, *second;

	if( nact_main_window_has_modified_items( window )){

		first = g_strdup(
					_( "Reloading a fresh list of actions requires "
						"that you give up with your current modifications." ));

		second = g_strdup( _( "Do you really want to do this ?" ));

		reload_ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, second );

		g_free( second );
		g_free( first );
	}

	return( reload_ok );
}

/*
 * informs the user that the actions in underlying storage subsystem
 * have changed, and propose for reloading
 *
 */
static gboolean
confirm_for_giveup_from_pivot( NactMainWindow *window )
{
	gboolean reload_ok;
	gchar *first, *second;

	first = g_strdup(
				_( "One or more actions have been modified in the filesystem.\n"
					"You could keep to work with your current list of actions, "
					"or you may want to reload a fresh one." ));

	if( nact_main_window_has_modified_items( window )){

		gchar *tmp = g_strdup_printf( "%s\n\n%s", first,
				_( "Note that reloading a fresh list of actions requires "
					"that you give up with your current modifications." ));
		g_free( first );
		first = tmp;
	}

	second = g_strdup( _( "Do you want to reload a fresh list of actions ?" ));

	reload_ok = base_window_yesno_dlg( BASE_WINDOW( window ), GTK_MESSAGE_QUESTION, first, second );

	g_free( second );
	g_free( first );

	return( reload_ok );
}

static void
install_autosave( NactMainWindow *window )
{
	gboolean autosave_on;
	guint autosave_period;
	NactApplication *application;
	NAUpdater *updater;
	NASettings *settings;

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
	updater = nact_application_get_updater( application );
	settings = na_pivot_get_settings( NA_PIVOT( updater ));

	autosave_on = na_settings_get_boolean( settings, NA_IPREFS_MAIN_SAVE_AUTO, NULL, NULL );
	autosave_period = na_settings_get_uint( settings, NA_IPREFS_MAIN_SAVE_PERIOD, NULL, NULL );

	nact_main_menubar_file_set_autosave( window, autosave_on, autosave_period );
}

/*
 * called by NAPivot because this window implements the IIOConsumer
 * interface, i.e. it wish to be advertised when the list of actions
 * changes in the underlying I/O storage subsystem (typically, when we
 * save the modifications)
 *
 * note that we only reload the full list of actions when asking for a
 * reset - saving is handled on a per-action basis.
 */
static void
ipivot_consumer_on_items_changed( NAIPivotConsumer *instance, gpointer user_data )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_on_items_changed";
	NactMainWindow *window;
	gboolean reload_ok;

	g_debug( "%s: instance=%p, user_data=%p", thisfn, ( void * ) instance, ( void * ) user_data );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));
	window = NACT_MAIN_WINDOW( instance );

	if( !window->private->dispose_has_run ){

		reload_ok = confirm_for_giveup_from_pivot( window );

		if( reload_ok ){
			reload( window );
		}
	}
}

/*
 * called by NAPivot via NAIPivotConsumer whenever the
 * autosave preferences have been modified.
 */
static void
ipivot_consumer_on_autosave_changed( NAIPivotConsumer *instance, gboolean enabled, guint period )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_on_autosave_changed";

	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));
	g_debug( "%s: instance=%p, enabled=%s, period=%d",
			thisfn, ( void * ) instance, enabled ? "True":"False", period );

	nact_main_menubar_file_set_autosave( NACT_MAIN_WINDOW( instance ), enabled, period );
}

/*
 * called by NAPivot via NAIPivotConsumer whenever the
 * "sort in alphabetical order" preference is modified.
 */
static void
ipivot_consumer_on_display_order_changed( NAIPivotConsumer *instance, gint order_mode )
{
	static const gchar *thisfn = "nact_main_window_ipivot_consumer_on_display_order_changed";
	NactApplication *application;
	NAUpdater *updater;
	GList *tree;

	g_return_if_fail( NACT_IS_MAIN_WINDOW( instance ));
	g_debug( "%s: instance=%p, order_mode=%d", thisfn, ( void * ) instance, order_mode );

	nact_iactions_list_display_order_change( NACT_IACTIONS_LIST( instance ), order_mode );
	nact_sort_buttons_display_order_change( NACT_MAIN_WINDOW( instance ), order_mode );

	application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( instance )));
	updater = nact_application_get_updater( application );
	tree = na_pivot_get_items( NA_PIVOT( updater ));

	if( g_list_length( tree )){
		g_signal_emit_by_name(
				NACT_MAIN_WINDOW( instance ),
				MAIN_WINDOW_SIGNAL_LEVEL_ZERO_ORDER_CHANGED,
				GINT_TO_POINTER( TRUE ));
	}
}

static void
ipivot_consumer_on_io_provider_prefs_changed( NAIPivotConsumer *instance )
{
	update_ui_after_provider_change( NACT_MAIN_WINDOW( instance ));
}

static void
ipivot_consumer_on_mandatory_prefs_changed( NAIPivotConsumer *instance )
{
	update_ui_after_provider_change( NACT_MAIN_WINDOW( instance ));
}

static void
update_ui_after_provider_change( NactMainWindow *window )
{
	nact_sort_buttons_level_zero_writability_change( window );

	if( window->private->selected_item ){
		setup_writability_status( window );
		g_signal_emit_by_name( window, MAIN_WINDOW_SIGNAL_SELECTION_CHANGED, GINT_TO_POINTER( 1 ));
	}
}

static void
reload( NactMainWindow *window )
{
	static const gchar *thisfn = "nact_main_window_reload";
	NactApplication *application;
	NAUpdater *updater;

	g_debug( "%s: window=%p", thisfn, ( void * ) window );
	g_return_if_fail( NACT_IS_MAIN_WINDOW( window ));

	if( !window->private->dispose_has_run ){

		window->private->selected_item = NULL;
		window->private->selected_profile = NULL;

		na_object_unref_items( window->private->deleted );
		window->private->deleted = NULL;

		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( window )));
		updater = nact_application_get_updater( application );
		na_pivot_load_items( NA_PIVOT( updater ));
		nact_iactions_list_fill( NACT_IACTIONS_LIST( window ), na_pivot_get_items( NA_PIVOT( updater )));
		nact_iactions_list_bis_select_first_row( NACT_IACTIONS_LIST( window ));
	}
}

static gchar *
iabout_get_application_name( NAIAbout *instance )
{
	BaseApplication *application;

	g_return_val_if_fail( NA_IS_IABOUT( instance ), NULL );
	g_return_val_if_fail( BASE_IS_WINDOW( instance ), NULL );

	application = base_window_get_application( BASE_WINDOW( instance ));
	return( base_application_get_application_name( application ));
}

static GtkWindow *
iabout_get_toplevel( NAIAbout *instance )
{
	g_return_val_if_fail( NA_IS_IABOUT( instance ), NULL );
	g_return_val_if_fail( BASE_IS_WINDOW( instance ), NULL );

	return( base_window_get_toplevel( BASE_WINDOW( instance )));
}
