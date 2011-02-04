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

#include <string.h>

#include <api/na-object-api.h>

#include <core/na-iprefs.h>

#include "nact-application.h"
#include "nact-clipboard.h"
#include "base-gtk-utils.h"
#include "nact-tree-model.h"
#include "nact-tree-model-priv.h"

/* private class data
 */
struct _NactTreeModelClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* search for an object, setting the iter if found
 */
typedef struct {
	GtkTreeModel   *store;
	const NAObject *object;
	gboolean        found;
	GtkTreeIter    *iter;
}
	ntmSearchStruct;

/* getting the list of items
 * - mode is the indicator of the wished content
 * - list is the returned list
 */
typedef struct {
	guint  mode;
	GList *items;
}
	ntmGetItems;

/* when iterating while searching for an object
 * setting the iter if found
 */
typedef struct {
	gchar       *id;
	NAObject    *object;
	GtkTreeIter *iter;
}
	ntmFindObject;

/* dump the content of the tree
 */
typedef struct {
	gchar *fname;
	gchar *prefix;
}
	ntmDumpStruct;

/* drop formats, as defined in nact-tree-model-dnd.c
 */
extern GtkTargetEntry tree_model_dnd_dest_formats[];
extern guint          tree_model_dnd_dest_formats_count;

#define TREE_MODEL_ORDER_MODE			"nact-tree-model-order-mode"

static GtkTreeModelFilterClass *st_parent_class = NULL;

static GType          register_type( void );
static void           class_init( NactTreeModelClass *klass );
static void           imulti_drag_source_init( EggTreeMultiDragSourceIface *iface );
static void           idrag_dest_init( GtkTreeDragDestIface *iface );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_dispose( GObject *model );
static void           instance_finalize( GObject *model );

static void           on_initialize_model( BaseWindow *window, gpointer user_data );
static void           dump( NactTreeModel *model );

static void           append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object );
static void           display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object );
#if 0
static gboolean       dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm );
#endif
static void           fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, NAObject *object, GtkTreeIter *parent );
static gboolean       filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model );
static gboolean       find_item_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmFindObject *nfo );
static gboolean       get_items_iter( const NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmGetItems *ngi );
static void           iter_on_store( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data );
static gboolean       iter_on_store_item( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data );
static void           remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object );
static gboolean       remove_items( GtkTreeStore *store, GtkTreeIter *iter );
static gboolean       search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *iter );
static gboolean       search_for_object_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm );
static gint           sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data );

static void           on_finalizing_window( NactTreeModel *model, GObject *window );

GType
nact_tree_model_get_type( void )
{
	static GType model_type = 0;

	if( !model_type ){
		model_type = register_type();
	}

	return( model_type );
}

static GType
register_type (void)
{
	static const gchar *thisfn = "nact_tree_model_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NactTreeModelClass ),
		NULL,							/* base_init */
		NULL,							/* base_finalize */
		( GClassInitFunc ) class_init,
		NULL,							/* class_finalize */
		NULL,							/* class_data */
		sizeof( NactTreeModel ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo imulti_drag_source_info = {
		( GInterfaceInitFunc ) imulti_drag_source_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo idrag_dest_info = {
		( GInterfaceInitFunc ) idrag_dest_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( GTK_TYPE_TREE_MODEL_FILTER, "NactTreeModel", &info, 0 );

	g_type_add_interface_static( type, EGG_TYPE_TREE_MULTI_DRAG_SOURCE, &imulti_drag_source_info );

	g_type_add_interface_static( type, GTK_TYPE_TREE_DRAG_DEST, &idrag_dest_info );

	return( type );
}

static void
class_init( NactTreeModelClass *klass )
{
	static const gchar *thisfn = "nact_tree_model_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactTreeModelClassPrivate, 1 );
}

static void
imulti_drag_source_init( EggTreeMultiDragSourceIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_imulti_drag_source_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->row_draggable = nact_tree_model_dnd_imulti_drag_source_row_draggable;
	iface->drag_data_get = nact_tree_model_dnd_imulti_drag_source_drag_data_get;
	iface->drag_data_delete = nact_tree_model_dnd_imulti_drag_source_drag_data_delete;
	iface->get_target_list = nact_tree_model_dnd_imulti_drag_source_get_format_list;
	iface->free_target_list = NULL;
	iface->get_drag_actions = nact_tree_model_dnd_imulti_drag_source_get_drag_actions;
}

static void
idrag_dest_init( GtkTreeDragDestIface *iface )
{
	static const gchar *thisfn = "nact_tree_model_idrag_dest_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->drag_data_received = nact_tree_model_dnd_idrag_dest_drag_data_received;
	iface->row_drop_possible = nact_tree_model_dnd_idrag_dest_row_drop_possible;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_tree_model_instance_init";
	NactTreeModel *self;

	g_return_if_fail( NACT_IS_TREE_MODEL( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NACT_TREE_MODEL( instance );

	self->private = g_new0( NactTreeModelPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_dispose";
	NactTreeModel *self;
	GtkTreeStore *ts_model;

	g_return_if_fail( NACT_IS_TREE_MODEL( object ));

	self = NACT_TREE_MODEL( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		g_object_unref( self->private->clipboard );

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( self )));
		gtk_tree_store_clear( ts_model );
		g_debug( "%s: tree store cleared", thisfn );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "nact_tree_model_instance_finalize";
	NactTreeModel *self;

	g_return_if_fail( NACT_IS_TREE_MODEL( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = NACT_TREE_MODEL( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * nact_tree_model_new:
 * @window: a #BaseWindow window which embeds our NactTreeView
 * @treeview: the #GtkTreeView widget.
 * @mode: management mode.
 *
 * Returns: a newly created NactTreeModel object, which will take
 * care itself of its destroy by attaching a weak reference to the
 * @window.
 *
 * The returned reference is owned by the #GtkTreeView.
 */
NactTreeModel *
nact_tree_model_new( BaseWindow *window, GtkTreeView *treeview, NactTreeMode mode )
{
	static const gchar *thisfn = "nact_tree_model_new";
	GtkTreeStore *ts_model;
	NactTreeModel *model;
	NactApplication *application;
	NAUpdater *updater;
	gint order_mode;

	g_return_val_if_fail( BASE_IS_WINDOW( window ), NULL );
	g_return_val_if_fail( GTK_IS_TREE_VIEW( treeview ), NULL );

	g_debug( "%s: window=%p, treeview=%p, mode=%u", thisfn, ( void * ) window, ( void * ) treeview, mode );

	ts_model = gtk_tree_store_new(
			TREE_N_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, NA_OBJECT_TYPE );

	/* create the filter model
	 */
	model = g_object_new( NACT_TREE_MODEL_TYPE, "child-model", ts_model, NULL );

	gtk_tree_model_filter_set_visible_func(
			GTK_TREE_MODEL_FILTER( model ), ( GtkTreeModelFilterVisibleFunc ) filter_visible, model, NULL );

	/* initialize the sortable interface
	 */
	application = NACT_APPLICATION( base_window_get_application( window ));
	updater = nact_application_get_updater( application );
	order_mode = na_iprefs_get_order_mode( NA_PIVOT( updater ), NULL );
	nact_tree_model_display_order_change( model, order_mode );

	/* setup instanciation time data
	 */
	model->private->window = window;
	model->private->treeview = treeview;
	model->private->mode = mode;
	model->private->clipboard = nact_clipboard_new( window );

	/* connect to some signal of interest
	 */
	base_window_signal_connect( window,
			G_OBJECT( window ), BASE_SIGNAL_INITIALIZE_WINDOW, G_CALLBACK( on_initialize_model ));

	g_object_set_data( G_OBJECT( window ), WINDOW_DATA_TREE_MODEL, model );

	g_object_weak_ref( G_OBJECT( window ), ( GWeakNotify ) on_finalizing_window, model );

	/* attach the model to the tree view
	 */
	gtk_tree_view_set_model( treeview, GTK_TREE_MODEL( model ));
	g_object_unref( model );

	return( model );
}

/*
 * on_initialize_model:
 * @model: this #NactTreeModel instance.
 * @have_dnd: whether the tree model must implement drag and drop
 * interfaces.
 *
 * Initializes the tree model.
 *
 * We use drag and drop:
 * - inside of treeview, for duplicating items, or moving items between
 *   menus
 * - from treeview to the outside world (e.g. Nautilus) to export actions
 * - from outside world (e.g. Nautilus) to import actions
 */
static void
on_initialize_model( BaseWindow *window, gpointer user_data )
{
	static const gchar *thisfn = "nact_tree_model_on_initialize_model";

	WINDOW_MODEL_VOID( window );

	g_debug( "%s: window=%p (%s), user_data=%p, model=%p (%s)",
			thisfn, ( void * ) window, G_OBJECT_TYPE_NAME( window ),
			( void * ) user_data, ( void * ) model, G_OBJECT_TYPE_NAME( model ));

	if( !model->private->dispose_has_run ){

		if( model->private->mode == TREE_MODE_EDITION ){

			egg_tree_multi_drag_add_drag_support(
					EGG_TREE_MULTI_DRAG_SOURCE( model ), model->private->treeview );

			gtk_tree_view_enable_model_drag_dest( model->private->treeview,
				tree_model_dnd_dest_formats, tree_model_dnd_dest_formats_count,
				nact_tree_model_dnd_imulti_drag_source_get_drag_actions( EGG_TREE_MULTI_DRAG_SOURCE( model )));

			base_window_signal_connect( window,
					G_OBJECT( model->private->treeview ), "drag-begin", G_CALLBACK( nact_tree_model_dnd_on_drag_begin ));

			/* connect: implies that we have to do all hard stuff
			 * connect_after: no more triggers drag-drop signal
			 */
			/*base_window_signal_connect_after( window,
					G_OBJECT( model->private->treeview ), "drag-motion", G_CALLBACK( on_drag_motion ));*/

			/*base_window_signal_connect( window,
					G_OBJECT( model->private->treeview ), "drag-drop", G_CALLBACK( on_drag_drop ));*/

			base_window_signal_connect( window,
					G_OBJECT( model->private->treeview ), "drag-end", G_CALLBACK( nact_tree_model_dnd_on_drag_end ));
		}
	}
}

/*
 * dump:
 * @model: this #NactTreeModel instance.
 *
 * Briefly dumps the content of the tree.
 */
static void
dump( NactTreeModel *model )
{
	GList *items;

	items = nact_tree_model_get_items( model, TREE_LIST_ALL );
	na_object_dump_tree( items );
	na_object_free_items( items );
}

/**
 * nact_tree_model_display:
 * @model: this #NactTreeModel instance.
 * @object: the object whose display is to be refreshed.
 *
 * Refresh the display of a #NAObject.
 */
void
nact_tree_model_display( NactTreeModel *model, NAObject *object )
{
	static const gchar *thisfn = "nact_tree_model_display";
	GtkTreeStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	g_debug( "%s: model=%p (%s), object=%p (%s)",
			thisfn,
			( void * ) model, G_OBJECT_TYPE_NAME( model ),
			( void * ) object, G_OBJECT_TYPE_NAME( object ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			display_item( store, model->private->treeview, &iter, object );
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			gtk_tree_model_row_changed( GTK_TREE_MODEL( store ), path, &iter );
			gtk_tree_path_free( path );
		}

		/*gtk_tree_model_filter_refilter( GTK_TREE_MODEL_FILTER( model ));*/
	}
}

/**
 * nact_tree_model_display_order_change:
 * @model: this #NactTreeModel.
 * @order_mode: the new order mode.
 *
 * Setup the new order mode.
 */
void
nact_tree_model_display_order_change( NactTreeModel *model, gint order_mode )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		g_object_set_data( G_OBJECT( store ), TREE_MODEL_ORDER_MODE, GINT_TO_POINTER( order_mode ));

		switch( order_mode ){

			case IPREFS_ORDER_ALPHA_ASCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_ASCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case IPREFS_ORDER_ALPHA_DESCENDING:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						GTK_SORT_DESCENDING );

				gtk_tree_sortable_set_sort_func(
						GTK_TREE_SORTABLE( store ),
						IACTIONS_LIST_LABEL_COLUMN,
						( GtkTreeIterCompareFunc ) sort_actions_list,
						NULL,
						NULL );
				break;

			case IPREFS_ORDER_MANUAL:
			default:

				gtk_tree_sortable_set_sort_column_id(
						GTK_TREE_SORTABLE( store ),
						GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
						0 );
				break;
		}
	}
}

/**
 * nact_tree_model_fill:
 * @model: this #NactTreeModel instance.
 * @ŧreeview: the #GtkTreeView widget.
 * @items: this list of items, usually from #NAPivot, which will be used
 *  to fill up the tree store.
 *
 * Fill up the tree store with specified items.
 *
 * We enter with the GSList owned by NAPivot which contains the ordered
 * list of level-zero items. We want have a duplicate of this list in
 * tree store, so that we are able to freely edit it.
 */
void
nact_tree_model_fill( NactTreeModel *model, GList *items )
{
	static const gchar *thisfn = "nact_tree_model_fill";
	GtkTreeStore *ts_model;
	GList *it;
	NAObject *duplicate;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	g_debug( "%s: model=%p, items=%p (%d items)",
			thisfn, ( void * ) model, ( void * ) items, g_list_length( items ));

	if( !model->private->dispose_has_run ){

		ts_model = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		gtk_tree_store_clear( ts_model );

		for( it = items ; it ; it = it->next ){
			duplicate = ( NAObject * ) na_object_duplicate( it->data );
			na_object_check_status( duplicate );
			fill_tree_store( ts_model, model->private->treeview, duplicate, NULL );
			na_object_unref( duplicate );
		}
	}
}

/**
 * nact_tree_model_get_item_by_id:
 * @model: this #NactTreeModel object.
 * @id: the searched #NAObjectItem.
 *
 * Returns: a pointer on the searched #NAObjectItem if it exists, or %NULL.
 *
 * The returned pointer is owned by the underlying tree store, and should
 * not be released by the caller.
 */
NAObjectItem *
nact_tree_model_get_item_by_id( const NactTreeModel *model, const gchar *id )
{
	static const gchar *thisfn = "nact_tree_model_get_item_by_id";
	GtkTreeStore *store;
	ntmFindObject nfo;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	nfo.object = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, id=%s", thisfn, ( void * ) model, id );

		nfo.id = ( gchar * ) id;
		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, ( FnIterOnStore ) find_item_iter, &nfo );
	}

	return(( NAObjectItem * ) nfo.object );
}

/**
 * nact_tree_model_get_items:
 * @model: this #NactTreeModel object.
 * @mode: the content indicator for the returned list
 *
 * Returns: the content of the current store as a newly allocated list
 * which should be na_object_free_items() by the caller.
 */
GList *
nact_tree_model_get_items( const NactTreeModel *model, guint mode )
{
	static const gchar *thisfn = "nact_tree_model_get_items";
	GList *items;
	GtkTreeStore *store;
	ntmGetItems ngi;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	items = NULL;

	if( !model->private->dispose_has_run ){
		g_debug( "%s: model=%p, mode=%x", thisfn, ( void * ) model, mode );

		ngi.mode = mode;
		ngi.items = NULL;
		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, ( FnIterOnStore ) get_items_iter, &ngi );
	}

	return( items );
}

/**
 * nact_tree_model_insert:
 * @model: this #NactTreeModel instance.
 * @object: a #NAObject-derived object to be inserted.
 * @path: the #GtkTreePath which specifies the insertion path.
 * @parent: set to the parent or the object itself.
 *
 * Insert a new row at the given position.
 *
 * Gtk API uses to returns iter ; but at least when inserting a new
 * profile in an action, we may have store_iter_path="0:1" (good), but
 * iter_path="0:0" (bad) - so we'd rather return a string path.
 *
 * Returns: the actual insertion path, which may be different from the
 * asked insertion path if tree is sorted.
 */
GtkTreePath *
nact_tree_model_insert( NactTreeModel *model, const NAObject *object, GtkTreePath *path, NAObject **parent )
{
	static const gchar *thisfn = "nact_tree_model_insert";
	GtkTreeModel *store;
	gchar *path_str;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *parent_path;
	GtkTreePath *inserted_path;
	NAObject *parent_obj;
	gboolean has_parent;
	GtkTreeIter sibling_iter;
	NAObject *sibling_obj;
	gboolean has_sibling;

	path_str = gtk_tree_path_to_string( path );
	g_debug( "%s: model=%p, object=%p (%s, ref_count=%d), path=%p (%s), parent=%p",
			thisfn,
			( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count,
			( void * ) path, path_str, ( void * ) parent );
	g_free( path_str );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	inserted_path = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
		has_parent = FALSE;
		parent_obj = NULL;
		sibling_obj = NULL;

		remove_if_exists( model, store, object );

		/* may be FALSE when store is empty */
		has_sibling = gtk_tree_model_get_iter( store, &sibling_iter, path );
		if( has_sibling ){
			gtk_tree_model_get( store, &sibling_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &sibling_obj, -1 );
			g_object_unref( sibling_obj );
		}
		g_debug( "%s: has_sibling=%s, sibling_obj=%p", thisfn, has_sibling ? "True":"False", ( void * ) sibling_obj );

		if( gtk_tree_path_get_depth( path ) > 1 ){

			has_parent = TRUE;
			parent_path = gtk_tree_path_copy( path );
			gtk_tree_path_up( parent_path );
			gtk_tree_model_get_iter( store, &parent_iter, parent_path );
			gtk_tree_path_free( parent_path );

			gtk_tree_model_get( store, &parent_iter, IACTIONS_LIST_NAOBJECT_COLUMN, &parent_obj, -1 );
			g_object_unref( parent_obj );

			if( parent && !*parent ){
				*parent = parent_obj;
			}

			if( has_sibling ){
				na_object_insert_item( parent_obj, object, sibling_obj );
			} else {
				na_object_append_item( parent_obj, object );
			}

			na_object_set_parent( object, parent_obj );
		}
		g_debug( "%s: has_parent=%s, parent_obj=%p", thisfn, has_parent ? "True":"False", ( void * ) parent_obj );

		gtk_tree_store_insert_before(
				GTK_TREE_STORE( store ), &iter,
				has_parent ? &parent_iter : NULL,
				has_sibling ? &sibling_iter : NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );

		inserted_path = gtk_tree_model_get_path( store, &iter );
	}

	return( inserted_path );
}

/**
 * nact_tree_model_insert_into:
 * @model:
 * @object:
 * @path:
 * @parent:
 */
GtkTreePath *
nact_tree_model_insert_into( NactTreeModel *model, const NAObject *object, GtkTreePath *path, NAObject **parent )
{
	static const gchar *thisfn = "nact_tree_model_insert_into";
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	GtkTreePath *new_path;
	gchar *path_str;

	path_str = gtk_tree_path_to_string( path );
	g_debug( "%s: model=%p, object=%p (%s, ref_count=%d), path=%p (%s), parent=%p",
			thisfn,
			( void * ) model,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count,
			( void * ) path, path_str, ( void * ) parent );
	g_free( path_str );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );
	g_return_val_if_fail( NA_IS_OBJECT( object ), NULL );

	new_path = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		if( !gtk_tree_model_get_iter( store, &parent_iter, path )){
			path_str = gtk_tree_path_to_string( path );
			g_warning( "%s: unable to get iter at path %s", thisfn, path_str );
			g_free( path_str );
			return( NULL );
		}
		gtk_tree_model_get( store, &parent_iter, IACTIONS_LIST_NAOBJECT_COLUMN, parent, -1 );
		g_object_unref( *parent );

		na_object_insert_item( *parent, object, NULL );
		na_object_set_parent( object, *parent );

		gtk_tree_store_insert_after( GTK_TREE_STORE( store ), &iter, &parent_iter, NULL );
		gtk_tree_store_set( GTK_TREE_STORE( store ), &iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
		display_item( GTK_TREE_STORE( store ), model->private->treeview, &iter, object );

		new_path = gtk_tree_model_get_path( store, &iter );
	}

	return( new_path );
}

/**
 * nact_tree_model_iter:
 * @model: this #NactTreeModel instance.
 */
void
nact_tree_model_iter( NactTreeModel *model, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeStore *store;

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));
		iter_on_store( model, GTK_TREE_MODEL( store ), NULL, fn, user_data );
	}
}

/**
 * nact_tree_model_object_at_path:
 * @model: this #NactTreeModel instance.
 * @path: the #GtkTreePath to be searched.
 *
 * Returns: the #NAObject at the given @path if any, or NULL.
 *
 * The reference count of the object is not modified.
 */
NAObject *
nact_tree_model_object_at_path( NactTreeModel *model, GtkTreePath *path )
{
	NAObject *object;
	GtkTreeModel *store;
	GtkTreeIter iter;

	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	object = NULL;

	if( !model->private->dispose_has_run ){

		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		if( gtk_tree_model_get_iter( store, &iter, path )){
			gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
			g_object_unref( object );
		}
	}

	return( object );
}

/**
 * nact_tree_model_remove:
 * @model: this #NactTreeModel instance.
 * @object: the #NAObject to be deleted.
 *
 * Recursively deletes the specified object.
 *
 * Returns: a path which may be suitable for the next selection.
 */
GtkTreePath *
nact_tree_model_remove( NactTreeModel *model, NAObject *object )
{
	static const gchar *thisfn = "nact_tree_model_remove";
	GtkTreeIter iter;
	GtkTreeStore *store;
	NAObjectItem *parent;
	GtkTreePath *path = NULL;

	g_debug( "%s: model=%p, object=%p (%s)",
			thisfn, ( void * ) model, ( void * ) object, object ? G_OBJECT_TYPE_NAME( object ) : "(null)" );
	g_return_val_if_fail( NACT_IS_TREE_MODEL( model ), NULL );

	if( !model->private->dispose_has_run ){

		store = GTK_TREE_STORE( gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model )));

		if( search_for_object( model, GTK_TREE_MODEL( store ), object, &iter )){
			parent = na_object_get_parent( object );
			g_debug( "%s: object=%p, parent=%p", thisfn, ( void * ) object, ( void * ) parent );
			if( parent ){
				na_object_remove_item( parent, object );
				na_object_check_status_up( parent );
			}
			path = gtk_tree_model_get_path( GTK_TREE_MODEL( store ), &iter );
			remove_items( store, &iter );
		}
	}

	return( path );
}

static void
append_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *parent, GtkTreeIter *iter, const NAObject *object )
{
	/*g_debug( "nact_tree_model_append_item: object=%p (ref_count=%d), parent=%p",
					( void * ) object, G_OBJECT( object )->ref_count, ( void * ) parent );*/

	gtk_tree_store_append( model, iter, parent );
	gtk_tree_store_set( model, iter, IACTIONS_LIST_NAOBJECT_COLUMN, object, -1 );
	display_item( model, treeview, iter, object );
}

static void
display_item( GtkTreeStore *model, GtkTreeView *treeview, GtkTreeIter *iter, const NAObject *object )
{
	gchar *label = na_object_get_label( object );
	gtk_tree_store_set( model, iter, IACTIONS_LIST_LABEL_COLUMN, label, -1 );
	g_free( label );

	if( NA_IS_OBJECT_ITEM( object )){
		gchar *icon_name = na_object_get_icon( object );
		GdkPixbuf *icon = base_gtk_utils_get_pixbuf( icon_name, GTK_WIDGET( treeview ), GTK_ICON_SIZE_MENU );
		gtk_tree_store_set( model, iter, IACTIONS_LIST_ICON_COLUMN, icon, -1 );
		g_object_unref( icon );
	}
}

#if 0
static gboolean
dump_store( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmDumpStruct *ntm )
{
	gint depth;
	gint i;
	GString *prefix;
	gchar *id, *label;
	NAObjectItem *origin;

	depth = gtk_tree_path_get_depth( path );
	prefix = g_string_new( ntm->prefix );
	for( i=1 ; i<depth ; ++i ){
		g_string_append_printf( prefix, "  " );
	}

	id = na_object_get_id( object );
	label = na_object_get_label( object );
	origin = ( NAObjectItem * ) na_object_get_origin( object );
	g_debug( "%s: %s%s at %p (ref_count=%d) \"[%s] %s\" origin=%p (%s)",
			ntm->fname, prefix->str,
			G_OBJECT_TYPE_NAME( object ), ( void * ) object, G_OBJECT( object )->ref_count, id, label,
			( void * ) origin, origin ? G_OBJECT_TYPE_NAME( object ) : "null" );
	g_free( label );
	g_free( id );

	g_string_free( prefix, TRUE );

	/* don't stop iteration */
	return( FALSE );
}
#endif

static void
fill_tree_store( GtkTreeStore *model, GtkTreeView *treeview, NAObject *object, GtkTreeIter *parent )
{
	static const gchar *thisfn = "nact_tree_model_fill_tree_store";
	GList *subitems, *it;
	GtkTreeIter iter;

	g_debug( "%s entering: object=%p (%s, ref_count=%d)", thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

	/* an action or a menu
	 */
	if( NA_IS_OBJECT_ITEM( object )){
		append_item( model, treeview, parent, &iter, object );
		subitems = na_object_get_items( object );
		for( it = subitems ; it ; it = it->next ){
			fill_tree_store( model, treeview, it->data, &iter );
		}

	} else {
		g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
		append_item( model, treeview, parent, &iter, object );
	}

	/*g_debug( "%s quitting: object=%p (%s, ref_count=%d)", thisfn,
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );*/
}

/*
 * only display profiles when we are in edition mode
 */
static gboolean
filter_visible( GtkTreeModel *store, GtkTreeIter *iter, NactTreeModel *model )
{
	/*static const gchar *thisfn = "nact_tree_model_filter_visible";*/
	NAObject *object;
	NAObjectAction *action;
	gint count;

	/*g_debug( "%s: model=%p, iter=%p, window=%p", thisfn, ( void * ) model, ( void * ) iter, ( void * ) window );*/
	/*g_debug( "%s at %p", G_OBJECT_TYPE_NAME( model ), ( void * ) model );*/
	/* is a GtkTreeStore */

	gtk_tree_model_get( store, iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );
	g_object_unref( object );
	/*na_object_dump( object );*/

	/* an action or a menu
	 */
	if( NA_IS_OBJECT_ITEM( object )){
		return( TRUE );
	}

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( object ), FALSE );

	if( NACT_TREE_MODEL( model )->private->mode != TREE_MODE_EDITION ){
		return( FALSE );
	}

	action = NA_OBJECT_ACTION( na_object_get_parent( object ));
	count = na_object_get_items_count( action );
	/*g_debug( "action=%p: count=%d", ( void * ) action, count );*/
	/*return( TRUE );*/
	return( count > 1 );

}

/*
 * search for an object, given its id
 */
static gboolean
find_item_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmFindObject *nfo )
{
	gchar *id;
	gboolean found = FALSE;

	if( NA_IS_OBJECT_ITEM( object )){
		id = na_object_get_id( object );
		found = ( g_ascii_strcasecmp( id, nfo->id ) == 0 );
		g_free( id );

		if( found ){
			nfo->object = object;
		}
	}

	/* stop iteration if found */
	return( found );
}

/*
 * Builds the tree by iterating on the store
 * we may want selected, modified or both, or a combination of these modes
 *
 * This function is called from iter_on_store_item();
 */
static gboolean
get_items_iter( const NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmGetItems *ngi )
{
	if( ngi->mode == TREE_LIST_ALL ){
		if( gtk_tree_path_get_depth( path ) == 1 ){
			ngi->items = g_list_prepend( ngi->items, na_object_ref( object ));
		}
	}

	/* don't stop iteration */
	return( FALSE );
}

static void
iter_on_store( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *parent, FnIterOnStore fn, gpointer user_data )
{
	GtkTreeIter iter;
	gboolean stop;

	if( gtk_tree_model_iter_children( store, &iter, parent )){
		stop = iter_on_store_item( model, store, &iter, fn, user_data );

		while( !stop && gtk_tree_model_iter_next( store, &iter )){
			stop = iter_on_store_item( model, store, &iter, fn, user_data );
		}
	}
}

static gboolean
iter_on_store_item( const NactTreeModel *model, GtkTreeModel *store, GtkTreeIter *iter, FnIterOnStore fn, gpointer user_data )
{
	NAObject *object;
	GtkTreePath *path;
	gboolean stop;

	/* unreffing as soon as we got the pointer so that the ref count is
	 * unchanged in dump_store
	 */
	gtk_tree_model_get( store, iter, TREE_COLUMN_NAOBJECT, &object, -1 );
	g_object_unref( object );

	/*
	g_debug( "nact_tree_model_iter_on_store_item: object=%p (%s, ref_count=%d)",
			( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );
			*/

	path = gtk_tree_model_get_path( store, iter );
	stop = ( *fn )( model, path, object, user_data );
	gtk_tree_path_free( path );

	if( !stop ){
		iter_on_store( model, store, iter, fn, user_data );
	}

	return( stop );
}

/*
 * if the object, identified by its id (historically a uuid), already exists,
 * then remove it first
 */
static void
remove_if_exists( NactTreeModel *model, GtkTreeModel *store, const NAObject *object )
{
	ntmFindObject nfo;
	GtkTreeIter iter;

	if( NA_IS_OBJECT_ITEM( object )){

		nfo.id = na_object_get_id( object );
		nfo.object = NULL;
		nfo.iter = &iter;

		iter_on_store( model, store, NULL, ( FnIterOnStore ) find_item_iter, &nfo );

		if( nfo.object ){
			g_debug( "nact_tree_model_remove_if_exists: removing %s %p",
					G_OBJECT_TYPE_NAME( object ), ( void * ) object );
			gtk_tree_store_remove( GTK_TREE_STORE( store ), nfo.iter );
		}

		g_free( nfo.id );
	}
}

/*
 * recursively remove child items starting with iter
 * returns TRUE if iter is always valid after the remove
 */
static gboolean
remove_items( GtkTreeStore *store, GtkTreeIter *iter )
{
	GtkTreeIter child;
	gboolean valid;

	while( gtk_tree_model_iter_children( GTK_TREE_MODEL( store ), &child, iter )){
		remove_items( store, &child );
	}
	valid = gtk_tree_store_remove( store, iter );

	return( valid );
}

static gboolean
search_for_object( NactTreeModel *model, GtkTreeModel *store, const NAObject *object, GtkTreeIter *result_iter )
{
	gboolean found = FALSE;
	ntmSearchStruct *ntm;
	GtkTreeIter iter;

	ntm = g_new0( ntmSearchStruct, 1 );
	ntm->store = store;
	ntm->object = object;
	ntm->found = FALSE;
	ntm->iter = &iter;

	iter_on_store( model, store, NULL, ( FnIterOnStore ) search_for_object_iter, ntm );

	if( ntm->found ){
		found = TRUE;
		memcpy( result_iter, ntm->iter, sizeof( GtkTreeIter ));
	}

	g_free( ntm );
	return( found );
}

static gboolean
search_for_object_iter( NactTreeModel *model, GtkTreePath *path, NAObject *object, ntmSearchStruct *ntm )
{
	if( object == ntm->object ){
		if( gtk_tree_model_get_iter( ntm->store, ntm->iter, path )){
			ntm->found = TRUE;
		}
	}

	/* stop iteration when found */
	return( ntm->found );
}

static gint
sort_actions_list( GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data )
{
	/*static const gchar *thisfn = "nact_tree_model_sort_actions_list";*/
	NAObjectId *obj_a, *obj_b;
	gint ret;

	/*g_debug( "%s: model=%p, a=%p, b=%p, window=%p", thisfn, ( void * ) model, ( void * ) a, ( void * ) b, ( void * ) window );*/

	gtk_tree_model_get( model, a, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_a, -1 );
	gtk_tree_model_get( model, b, IACTIONS_LIST_NAOBJECT_COLUMN, &obj_b, -1 );

	g_object_unref( obj_b );
	g_object_unref( obj_a );

	if( NA_IS_OBJECT_PROFILE( obj_a )){
		ret = 0;
	} else {
		ret = na_object_sort_alpha_asc( obj_a, obj_b );
	}

	/*g_debug( "%s: ret=%d", thisfn, ret );*/
	return( ret );
}

static void
on_finalizing_window( NactTreeModel *model, GObject *window )
{
	static const gchar *thisfn = "nact_tree_model_on_finalizing_window";

	g_return_if_fail( NACT_IS_TREE_MODEL( model ));

	g_debug( "%s: model=%p (%s), window=%p",
			thisfn, ( void * ) model, G_OBJECT_TYPE_NAME( model ), ( void * ) window );

	dump( model );

	g_object_unref( model );
}
