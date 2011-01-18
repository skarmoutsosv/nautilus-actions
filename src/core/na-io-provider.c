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

#include <glib/gi18n.h>
#include <string.h>

#include <api/na-iio-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>

#include "na-iprefs.h"
#include "na-io-provider.h"

/* private class data
 */
struct _NAIOProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NAIOProviderPrivate {
	gboolean       dispose_has_run;
	gchar         *id;
	NAIIOProvider *provider;
	gulong         item_changed_handler;
};

/* NAIOProvider properties
 */
enum {
	IO_PROVIDER_PROP_ID_ID = 1,
};

#define IO_PROVIDER_PROP_ID				"na-io-provider-prop-id"

static GObjectClass *st_parent_class = NULL;
static GList        *st_io_providers = NULL;

static GType  register_type( void );
static void   class_init( NAIOProviderClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_constructed( GObject *object );
static void   instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void   instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

#if 0
static void          dump( const NAIOProvider *provider );
static void          dump_providers_list( GList *providers );
#endif
static NAIOProvider *io_provider_new( const NAPivot *pivot, NAIIOProvider *module, const gchar *id );
static GList        *io_providers_list_add_from_plugins( const NAPivot *pivot, GList *list );
static GList        *io_providers_list_add_from_prefs( const NAPivot *pivot, GList *objects_list );
static GList        *io_providers_list_add_from_write_order( const NAPivot *pivot, GList *objects_list );
static GList        *io_providers_list_append_object( const NAPivot *pivot, GList *list, NAIIOProvider *module, const gchar *id );
static void          io_providers_list_set_module( const NAPivot *pivot, NAIOProvider *provider_object, NAIIOProvider *provider_module );
static gboolean      is_really_writable( const NAIOProvider *provider, const NAPivot *pivot );
static GList        *load_items_filter_unwanted_items( const NAPivot *pivot, GList *merged, guint loadable_set );
static GList        *load_items_filter_unwanted_items_rec( GList *merged, guint loadable_set );
static GList        *load_items_get_merged_list( const NAPivot *pivot, guint loadable_set, GSList **messages );
static GList        *load_items_hierarchy_build( GList **tree, GSList *level_zero, gboolean list_if_empty, NAObjectItem *parent );
static GList        *load_items_hierarchy_sort( const NAPivot *pivot, GList *tree, GCompareFunc fn );
static gint          peek_item_by_id_compare( const NAObject *obj, const gchar *id );
static NAIOProvider *peek_provider_by_id( const GList *providers, const gchar *id );

GType
na_io_provider_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "na_io_provider_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAIOProviderClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAIOProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAIOProvider", &info, 0 );

	return( type );
}

static void
class_init( NAIOProviderClass *klass )
{
	static const gchar *thisfn = "na_io_provider_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->constructed = instance_constructed;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	spec = g_param_spec_string(
			IO_PROVIDER_PROP_ID,
			"I/O Provider Id",
			"Internal identifier of the I/O provider (e.g. 'na-gconf')", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, IO_PROVIDER_PROP_ID_ID, spec );

	klass->private = g_new0( NAIOProviderClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_io_provider_instance_init";
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NA_IO_PROVIDER( instance );

	self->private = g_new0( NAIOProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->id = NULL;
	self->private->provider = NULL;
	self->private->item_changed_handler = 0;
}

static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_constructed";
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));

	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p, id=%s", thisfn, ( void * ) object, self->private->id );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}
	}
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_value_set_string( value, self->private->id );
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
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));
	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case IO_PROVIDER_PROP_ID_ID:
				g_free( self->private->id );
				self->private->id = g_value_dup_string( value );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_dispose";
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));

	self = NA_IO_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		if( self->private->provider ){
			if( g_signal_handler_is_connected( self->private->provider, self->private->item_changed_handler )){
				g_signal_handler_disconnect( self->private->provider, self->private->item_changed_handler );
			}
			g_object_unref( self->private->provider );
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_io_provider_instance_finalize";
	NAIOProvider *self;

	g_return_if_fail( NA_IS_IO_PROVIDER( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = NA_IO_PROVIDER( object );

	g_free( self->private->id );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * na_io_provider_find_writable_io_provider:
 * @pivot: the #NAPivot instance.
 *
 * Returns: the first willing and able to write I/O provider, or NULL.
 *
 * The returned provider is owned by NAIOProvider class, and should not
 * be released by the caller.
 */
NAIOProvider *
na_io_provider_find_writable_io_provider( const NAPivot *pivot )
{
	const GList *providers;
	const GList *ip;
	NAIOProvider *provider;

	providers = na_io_provider_get_io_providers_list( pivot );
	provider = NULL;

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		if( is_really_writable( NA_IO_PROVIDER( ip->data ), pivot )){
			provider = NA_IO_PROVIDER( ip->data );
		}
	}

	return( provider );
}

/*
 * na_io_provider_find_io_provider_by_id:
 * @pivot: the #NAPivot instance.
 * @id: the identifier of the searched I/O provider.
 *
 * Returns: the I/O provider, or NULL.
 *
 * The returned provider is owned by NAIOProvider class, and should not
 * be released by the caller.
 */
NAIOProvider *
na_io_provider_find_io_provider_by_id( const NAPivot *pivot, const gchar *id )
{
	const GList *providers;
	const GList *ip;
	NAIOProvider *provider;
	NAIOProvider *found;

	providers = na_io_provider_get_io_providers_list( pivot );
	found = NULL;

	for( ip = providers ; ip && !found ; ip = ip->next ){
		provider = NA_IO_PROVIDER( ip->data );
		if( !strcmp( provider->private->id, id )){
			found = provider;
		}
	}

	return( found );
}

/*
 * na_io_provider_get_io_providers_list:
 * @pivot: the current #NAPivot instance.
 *
 * Build (if not already done) the ordered list of currently avialable
 * NAIOProvider objects.
 *
 * A NAIOProvider object may be created:
 * - either because we have loaded a plugin which claims to implement the
 *   NAIIOProvider interface;
 * - or because an i/o provider identifier has been found in preferences.
 *
 * The objects in this list must be in write order:
 * - when loading items, items are ordered depending of menus items list
 *   and of level zero defined order;
 * - when writing a new item, there is a 'writable-order' preference.
 *
 * Returns: the list of I/O providers.
 *
 * The returned list is owned by #NAIOProvider class, and should not be
 * released by the caller.
 */
const GList *
na_io_provider_get_io_providers_list( const NAPivot *pivot )
{
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	if( !st_io_providers ){
		st_io_providers = io_providers_list_add_from_write_order( pivot, NULL );
		st_io_providers = io_providers_list_add_from_plugins( pivot, st_io_providers );
		st_io_providers = io_providers_list_add_from_prefs( pivot, st_io_providers );
	}

	return( st_io_providers );
}

/*
 * na_io_provider_unref_io_providers_list:
 *
 * Called by on #NAPivot dispose(), free here resources allocated to
 * the I/O providers.
 */
void
na_io_provider_unref_io_providers_list( void )
{
	g_list_foreach( st_io_providers, ( GFunc ) g_object_unref, NULL );
	g_list_free( st_io_providers );
	st_io_providers = NULL;
}

/*
 * na_io_provider_get_id:
 * @provider: this #NAIOProvider.
 *
 * Returns: the internal id of this #NAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_io_provider_get_id( const NAIOProvider *provider )
{
	gchar *id;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), NULL );

	id = NULL;

	if( !provider->private->dispose_has_run ){

		id = g_strdup( provider->private->id );
	}

	return( id );
}

/*
 * na_io_provider_get_name:
 * @provider: this #NAIOProvider.
 *
 * Returns: the displayable name of this #NAIIOProvider, as a newly
 * allocated string which should be g_free() by the caller.
 *
 * This function makes sure to never return %NULL. An empty string
 * may be returned if the NAIIOProvider is not present at runtime,
 * or does not implement the needed interface, or returns itself %NULL
 * or an empty string.
 */
gchar *
na_io_provider_get_name( const NAIOProvider *provider )
{
	static const gchar *thisfn = "na_io_provider_get_name";
	gchar *name;

	name = g_strdup( "" );

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), name );

	if( !provider->private->dispose_has_run ){
		if( na_io_provider_is_available( provider ) &&
			NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->get_name ){

				g_free( name );
				name = NULL;
				name = NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->get_name( provider->private->provider );
				if( !name ){
					g_warning( "%s: NAIIOProvider %s get_name() interface returns NULL", thisfn, provider->private->id );
					name = g_strdup( "" );
			}

		} else {
			g_warning( "%s: NAIIOProvider %s doesn't support get_name() interface", thisfn, provider->private->id );
		}
	}

	return( name );
}

/*
 * na_io_provider_is_available:
 * @provider: the #NAIOProvider object.
 *
 * Returns: %TRUE if the corresponding #NAIIOProvider module is available
 * at runtime, %FALSE else.
 */
gboolean
na_io_provider_is_available( const NAIOProvider *provider )
{
	gboolean is_available;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );

	is_available = FALSE;

	if( !provider->private->dispose_has_run ){

		is_available = ( provider->private->provider && NA_IS_IIO_PROVIDER( provider->private->provider ));
	}

	return( is_available );
}

/*
 * na_io_provider_is_conf_readable:
 * @provider: this #NAIOProvider.
 * @pivot: the #NAPivot application object.
 * @mandatory: a pointer to a boolean which will be set to %TRUE if the
 *  preference is mandatory; may be %NULL.
 *
 * Returns: %TRUE is this I/O provider should be read at startup, and so
 * may participate to the global list of menus and actions, %FALSE else.
 *
 * This is a configuration property, which defaults to %TRUE.
 *
 * Whether it is editable by the user or not depends on:
 * - whether the whole configuration has been locked down by an admin;
 * - whether this flag has been set as mandatory by an admin.
 */
gboolean
na_io_provider_is_conf_readable( const NAIOProvider *provider, const NAPivot *pivot, gboolean *mandatory )
{
	gboolean readable;
	gchar *group;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	readable = FALSE;

	if( !provider->private->dispose_has_run ){

		group = g_strdup_printf( "%s %s", NA_IPREFS_IO_PROVIDER_GROUP, provider->private->id );
		readable = na_settings_get_boolean_ex(
				na_pivot_get_settings( pivot ), group, NA_IPREFS_IO_PROVIDER_READABLE, NULL, mandatory );
		g_free( group );
	}

	return( readable );
}

/*
 * na_io_provider_is_able_to_write:
 * @provider: this #NAIOProvider.
 *
 * Whether the NAIIOProvider is able to write is a runtime condition.
 *
 * Returns: %TRUE is this I/O provider is able to write.
 */
gboolean
na_io_provider_is_able_to_write( const NAIOProvider *provider )
{
	gboolean is_able_to;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );

	is_able_to = FALSE;

	if( !provider->private->dispose_has_run ){

		if( provider->private->provider ){

			g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), FALSE );

			if( NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_able_to_write ){

				is_able_to = NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_able_to_write( provider->private->provider );
			}
		}
	}

	return( is_able_to );
}

/*
 * na_io_provider_is_conf_writable:
 * @provider: this #NAIOProvider.
 * @pivot: the #NAPivot application object.
 * @mandatory: a pointer to a boolean which will be set to %TRUE if the
 *  preference is mandatory; may be %NULL.
 *
 * Returns: %TRUE is this I/O provider is candidate to be edited.
 *
 * This is a configuration property, which defaults to %TRUE.
 *
 * Whether it is editable by the user or not depends on:
 * - whether the whole configuration has been locked down by an admin;
 * - whether this flag has been set as mandatory by an admin.
 *
 * This property does not say that an item can actually be written by this
 * NAIIOProvider module. See also is_willing_to() and is_able_to().
 */
gboolean
na_io_provider_is_conf_writable( const NAIOProvider *provider, const NAPivot *pivot, gboolean *mandatory )
{
	gboolean is_writable;
	gchar *group;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), FALSE );

	is_writable = FALSE;

	if( !provider->private->dispose_has_run ){

		group = g_strdup_printf( "%s %s", NA_IPREFS_IO_PROVIDER_GROUP, provider->private->id );
		is_writable = na_settings_get_boolean_ex(
				na_pivot_get_settings( pivot ), group, NA_IPREFS_IO_PROVIDER_WRITABLE, NULL, mandatory );
		g_free( group );
	}

	return( is_writable );
}

/*
 * na_io_provider_is_willing_to_write:
 * @provider: this #NAIOProvider.
 *
 * The 'willing_to_write' property is an intrinsic attribute of the NAIIOProvider
 * module. It depends on:
 * - whether it provides a 'willing_to_write' api which returns %TRUE (defaults
 *   to %FALSE)
 * - whether it has the needed API (is_able_to_write, write_item, delete_item)
 *
 * Returns: %TRUE is this I/O provider is willing to write.
 */
gboolean
na_io_provider_is_willing_to_write( const NAIOProvider *provider )
{
	gboolean is_willing_to;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );

	is_willing_to = FALSE;

	if( !provider->private->dispose_has_run &&
		provider->private->provider ){

		g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), FALSE );

		if( NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_willing_to_write &&
			NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_willing_to_write( provider->private->provider )){

			is_willing_to =
					NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->is_able_to_write &&
					NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item &&
					NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item;
		}
	}

	return( is_willing_to );
}

/*
 * na_io_provider_load_items:
 * @pivot: the #NAPivot object which owns the list of registered I/O
 *  storage providers.
 * @loadable_set: the set of loadable items
 *  (cf. NAPivotLoadableSet enumeration defined in core/na-pivot.h).
 * @messages: error messages.
 *
 * Loads the tree from I/O storage subsystems.
 *
 * Returns: a #GList of newly allocated objects as a hierarchical tree
 * in display order. This tree contains #NAObjectMenu menus, along with
 * #NAObjectAction actions and their #NAObjectProfile profiles.
 *
 * The returned list should be na_object_unref_items() by the caller.
 */
GList *
na_io_provider_load_items( const NAPivot *pivot, guint loadable_set, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_load_items";
	GList *flat, *hierarchy, *filtered;
	GSList *level_zero;
	guint order_mode;

	g_return_val_if_fail( NA_IS_PIVOT( pivot ), NULL );

	g_debug( "%s: pivot=%p, loadable_set=%d, messages=%p",
			thisfn, ( void * ) pivot, loadable_set, ( void * ) messages );

	/* get the global flat items list, as a merge of the list provided
	 * each available and readable i/o provider
	 */
	flat = load_items_get_merged_list( pivot, loadable_set, messages );

	/* build the items hierarchy
	 */
	level_zero = na_settings_get_string_list(
			na_pivot_get_settings( pivot ), NA_IPREFS_ITEMS_LEVEL_ZERO_ORDER, NULL, NULL );

	hierarchy = load_items_hierarchy_build( &flat, level_zero, TRUE, NULL );

	/* items that stay left in the global flat list are simply appended
	 * to the built hierarchy, and level zero is updated accordingly
	 */
	if( flat ){
		g_debug( "%s: %d items left appended to the hierarchy", thisfn, g_list_length( flat ));
		hierarchy = g_list_concat( hierarchy, flat );
	}

	if( flat || !level_zero || !g_slist_length( level_zero )){
		g_debug( "%s: rewriting level-zero", thisfn );
		if( !na_iprefs_write_level_zero( pivot, hierarchy, messages )){
			g_warning( "%s: unable to update level-zero", thisfn );
		}
	}

	na_core_utils_slist_free( level_zero );

	/* sort the hierarchy according to preferences
	 */
	order_mode = na_iprefs_get_order_mode( pivot, NULL );
	switch( order_mode ){
		case IPREFS_ORDER_ALPHA_ASCENDING:
			hierarchy = load_items_hierarchy_sort( pivot, hierarchy, ( GCompareFunc ) na_object_id_sort_alpha_asc );
			break;

		case IPREFS_ORDER_ALPHA_DESCENDING:
			hierarchy = load_items_hierarchy_sort( pivot, hierarchy, ( GCompareFunc ) na_object_id_sort_alpha_desc );
			break;

		case IPREFS_ORDER_MANUAL:
		default:
			break;
	}

	/* check status here...
	 */
	filtered = load_items_filter_unwanted_items( pivot, hierarchy, loadable_set );
	g_list_free( hierarchy );

	g_debug( "%s: tree after filtering and reordering (if any)", thisfn );
	na_object_dump_tree( filtered );
	g_debug( "%s: end of tree", thisfn );

	return( filtered );
}

#if 0
static void
dump( const NAIOProvider *provider )
{
	static const gchar *thisfn = "na_io_provider_dump";

	g_debug( "%s:                   id=%s", thisfn, provider->private->id );
	g_debug( "%s:             provider=%p", thisfn, ( void * ) provider->private->provider );
	g_debug( "%s: item_changed_handler=%lu", thisfn, provider->private->item_changed_handler );
}

static void
dump_providers_list( GList *providers )
{
	static const gchar *thisfn = "na_io_provider_dump_providers_list";
	GList *ip;

	g_debug( "%s: providers=%p (count=%d)", thisfn, ( void * ) providers, g_list_length( providers ));

	for( ip = providers ; ip ; ip = ip->next ){
		dump( NA_IO_PROVIDER( ip->data ));
	}
}
#endif

/*
 * allocate a new NAIOProvider object for the specified module and id
 *
 * id is mandatory here
 * module may be NULL
 */
static NAIOProvider *
io_provider_new( const NAPivot *pivot, NAIIOProvider *module, const gchar *id )
{
	NAIOProvider *object;

	g_return_val_if_fail( id && strlen( id ), NULL );

	object = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );

	if( module ){
		io_providers_list_set_module( pivot, object, module );
	}

	return( object );
}

/*
 * add to the list a NAIOProvider object for each loaded plugin which claim
 * to implement the NAIIOProvider interface
 */
static GList *
io_providers_list_add_from_plugins( const NAPivot *pivot, GList *objects_list )
{
	static const gchar *thisfn = "na_io_provider_io_providers_list_add_from_plugins";
	GList *merged;
	GList *modules_list, *im;
	gchar *id;
	NAIIOProvider *provider_module;

	merged = objects_list;
	modules_list = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );

	for( im = modules_list ; im ; im = im->next ){

		id = NULL;
		provider_module = NA_IIO_PROVIDER( im->data );

		if( NA_IIO_PROVIDER_GET_INTERFACE( provider_module )->get_id ){
			id = NA_IIO_PROVIDER_GET_INTERFACE( provider_module )->get_id( provider_module );
			if( !id || !strlen( id )){
				g_warning( "%s: NAIIOProvider %p get_id() interface returns null or empty id", thisfn, ( void * ) im->data );
				g_free( id );
				id = NULL;
			}
		} else {
			g_warning( "%s: NAIIOProvider %p doesn't support get_id() interface", thisfn, ( void * ) im->data );
		}

		if( id ){
			merged = io_providers_list_append_object( pivot, merged, provider_module, id );
			g_free( id );
		}
	}

	na_pivot_free_providers( modules_list );

	return( merged );
}

/*
 * add to the list NAIOProvider objects for the identifiers we may find
 * in preferences without having found the plugin itself
 *
 * preferences may come from the 'write-order' list, or from the io-providers
 * status.
 */
static GList *
io_providers_list_add_from_prefs( const NAPivot *pivot, GList *objects_list )
{
	GList *merged;
	const gchar *id;
	GSList *io_providers, *it;

	merged = objects_list;
	io_providers = na_iprefs_get_io_providers( pivot );

	for( it = io_providers ; it ; it = it->next ){
		id = ( const gchar * ) it->data;
		merged = io_providers_list_append_object( pivot, merged, NULL, id );
	}

	na_core_utils_slist_free( io_providers );

	return( merged );
}

/*
 * adding from write-order means we only create NAIOProvider objects
 * without having any pointer to the underlying NAIIOProvider (if it exists)
 */
static GList *
io_providers_list_add_from_write_order( const NAPivot *pivot, GList *objects_list )
{
	GList *merged;
	NASettings *settings;
	GSList *io_providers, *it;
	const gchar *id;

	merged = objects_list;
	settings = na_pivot_get_settings( pivot );
	io_providers = na_settings_get_string_list( settings, NA_IPREFS_IO_PROVIDERS_WRITE_ORDER, NULL, NULL );

	for( it = io_providers ; it ; it = it->next ){
		id = ( const gchar * ) it->data;
		merged = io_providers_list_append_object( pivot, merged, NULL, id );
	}

	na_core_utils_slist_free( io_providers );

	return( merged );
}

/*
 * add to the list a NAIOProvider object for the specified module and id
 * if it does not have been already registered
 */
static GList *
io_providers_list_append_object( const NAPivot *pivot, GList *list, NAIIOProvider *module, const gchar *id )
{
	GList *merged;
	NAIOProvider *object;

	merged = list;
	object = peek_provider_by_id( list, id );

	if( !object ){
		object = io_provider_new( pivot, module, id );
		merged = g_list_append( merged, object );

	} else if( module && !object->private->provider ){
		io_providers_list_set_module( pivot, object, module );
	}

	return( merged );
}

static void
io_providers_list_set_module( const NAPivot *pivot, NAIOProvider *provider_object, NAIIOProvider *provider_module )
{
	provider_object->private->provider = g_object_ref( provider_module );
	provider_object->private->item_changed_handler =
			g_signal_connect(
					provider_module, IO_PROVIDER_SIGNAL_ITEM_CHANGED,
					( GCallback ) na_pivot_on_item_changed_handler, ( gpointer ) pivot );
}

/*
 * is_really_writable:
 * @provider: the #NAIOProvider provider.
 * @pivot: the #NAPivot instance.
 *
 * Returns: %TRUE if the @provider will be able to write proposed items,
 * %FALSE else.
 *
 * Note thay almost all checked conditions are themselves subject to race
 * conditions. Unless that (!), the caller can be (almost) sure that writings
 * will be possible when this function returns %TRUE.
 */
static gboolean
is_really_writable( const NAIOProvider *provider, const NAPivot *pivot )
{

	return( na_io_provider_is_willing_to_write( provider ) &&
			na_io_provider_is_able_to_write( provider ) &&
			na_io_provider_is_conf_writable( provider, pivot, NULL ) &&
			!na_pivot_is_configuration_locked_by_admin( pivot ));
}

static GList *
load_items_filter_unwanted_items( const NAPivot *pivot, GList *hierarchy, guint loadable_set )
{
	GList *it;
	GList *filtered;

	for( it = hierarchy ; it ; it = it->next ){
		na_object_check_status( it->data );
	}

	filtered = load_items_filter_unwanted_items_rec( hierarchy, loadable_set );

	return( filtered );
}

/*
 * build a dest tree from a source tree, removing filtered items
 * an item is filtered if it is invalid (and not loading invalid ones)
 * or disabled (and not loading disabled ones)
 */
static GList *
load_items_filter_unwanted_items_rec( GList *hierarchy, guint loadable_set )
{
	static const gchar *thisfn = "na_io_provider_load_items_filter_unwanted_items_rec";
	GList *subitems, *subitems_f;
	GList *it, *itnext;
	GList *filtered;
	gboolean selected;
	gchar *label;
	gboolean load_invalid, load_disabled;

	filtered = NULL;
	load_invalid = loadable_set & PIVOT_LOAD_INVALID;
	load_disabled = loadable_set & PIVOT_LOAD_DISABLED;

	for( it = hierarchy ; it ; it = itnext ){

		itnext = it->next;
		selected = FALSE;

		if( NA_IS_OBJECT_PROFILE( it->data )){
			if( na_object_is_valid( it->data ) || load_invalid ){
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( NA_IS_OBJECT_ITEM( it->data )){
			if(( na_object_is_enabled( it->data ) || load_disabled ) &&
				( na_object_is_valid( it->data ) || load_invalid )){

				subitems = na_object_get_items( it->data );
				subitems_f = load_items_filter_unwanted_items_rec( subitems, loadable_set );
				na_object_set_items( it->data, subitems_f );
				filtered = g_list_append( filtered, it->data );
				selected = TRUE;
			}
		}

		if( !selected ){
			label = na_object_get_label( it->data );
			g_debug( "%s: filtering %p (%s) '%s'", thisfn, ( void * ) it->data, G_OBJECT_TYPE_NAME( it->data ), label );
			g_free( label );
			na_object_unref( it->data );
		}
	}

	return( filtered );
}

/*
 * returns a concatened flat list of readen actions / menus
 * we take care here of:
 * - i/o providers which appear unavailable at runtime
 * - i/o providers marked as unreadable
 * - items (actions or menus) which do not satisfy the defined loadable set
 */
static GList *
load_items_get_merged_list( const NAPivot *pivot, guint loadable_set, GSList **messages )
{
	const GList *providers;
	const GList *ip;
	GList *merged, *items, *it;
	const NAIOProvider *provider_object;
	const NAIIOProvider *provider_module;

	merged = NULL;
	providers = na_io_provider_get_io_providers_list( pivot );

	for( ip = providers ; ip ; ip = ip->next ){
		provider_object = NA_IO_PROVIDER( ip->data );
		provider_module = provider_object->private->provider;

		if( provider_module &&
			NA_IIO_PROVIDER_GET_INTERFACE( provider_module )->read_items &&
			na_io_provider_is_conf_readable( provider_object, pivot, NULL )){

			items = NA_IIO_PROVIDER_GET_INTERFACE( provider_module )->read_items( provider_module, messages );

			for( it = items ; it ; it = it->next ){
				na_object_set_provider( it->data, provider_object );
				na_object_dump( it->data );
			}

			merged = g_list_concat( merged, items );
		}
	}

	return( merged );
}

/*
 * builds the hierarchy
 *
 * this is a recursive function which _moves_ items from input 'tree' to
 * output list.
 */
static GList *
load_items_hierarchy_build( GList **tree, GSList *level_zero, gboolean list_if_empty, NAObjectItem *parent )
{
	static const gchar *thisfn = "na_io_provider_load_items_hierarchy_build";
	GList *hierarchy, *it;
	GSList *ilevel;
	GSList *subitems_ids;
	GList *subitems;

	hierarchy = NULL;

	if( g_slist_length( level_zero )){
		for( ilevel = level_zero ; ilevel ; ilevel = ilevel->next ){
			/*g_debug( "%s: id=%s", thisfn, ( gchar * ) ilevel->data );*/
			it = g_list_find_custom( *tree, ilevel->data, ( GCompareFunc ) peek_item_by_id_compare );
			if( it ){
				hierarchy = g_list_append( hierarchy, it->data );
				na_object_set_parent( it->data, parent );

				g_debug( "%s: id=%s: %s (%p) appended to hierarchy %p",
						thisfn, ( gchar * ) ilevel->data, G_OBJECT_TYPE_NAME( it->data ), ( void * ) it->data, ( void * ) hierarchy );

				*tree = g_list_remove_link( *tree, it );

				if( NA_IS_OBJECT_MENU( it->data )){
					subitems_ids = na_object_get_items_slist( it->data );
					subitems = load_items_hierarchy_build( tree, subitems_ids, FALSE, NA_OBJECT_ITEM( it->data ));
					na_object_set_items( it->data, subitems );
					na_core_utils_slist_free( subitems_ids );
				}
			}
		}
	}
	/* if level-zero list is empty,
	 * we consider that all items are at the same level
	 */
	else if( list_if_empty ){
		for( it = *tree ; it ; it = it->next ){
			hierarchy = g_list_append( hierarchy, it->data );
			na_object_set_parent( it->data, parent );
		}
		g_list_free( *tree );
		*tree = NULL;
	}

	return( hierarchy );
}

static GList *
load_items_hierarchy_sort( const NAPivot *pivot, GList *tree, GCompareFunc fn )
{
	GList *sorted;
	GList *items, *it;

	sorted = g_list_sort( tree, fn );

	/* recursively sort each level of the tree
	 */
	for( it = sorted ; it ; it = it->next ){
		if( NA_IS_OBJECT_MENU( it->data )){
			items = na_object_get_items( it->data );
			items = load_items_hierarchy_sort( pivot, items, fn );
			na_object_set_items( it->data, items );
		}
	}

	return( sorted );
}

/*
 * returns zero when @obj has the required @id
 */
static gint
peek_item_by_id_compare( const NAObject *obj, const gchar *id )
{
	gchar *obj_id;
	gint ret = 1;

	if( NA_IS_OBJECT_ITEM( obj )){
		obj_id = na_object_get_id( obj );
		ret = strcmp( obj_id, id );
		g_free( obj_id );
	}

	return( ret );
}

static NAIOProvider *
peek_provider_by_id( const GList *providers, const gchar *id )
{
	NAIOProvider *provider = NULL;
	const GList *ip;

	for( ip = providers ; ip && !provider ; ip = ip->next ){
		if( !strcmp( NA_IO_PROVIDER( ip->data )->private->id, id )){
			provider = NA_IO_PROVIDER( ip->data );
		}
	}

	return( provider );
}

#if 0
/*
 * @priority: the internal ids of IO providers in descending order of
 * priority for writing new elements, as a string list
 *
 * build the static list of I/O providers, depending of setup of NAPivot
 * doing required initializations
 */
static void
setup_io_providers( const NAPivot *pivot, GSList *priority )
{
	GList *ordered_providers;
	GList *merged_providers;
	GList *all_providers;

	g_return_if_fail( st_io_providers == NULL );

	/* allocate the ordered list */
	ordered_providers = allocate_ordered_providers( priority );

	/* merge with available I/O providers */
	merged_providers = merge_available_io_providers( pivot, ordered_providers );

	/* add providers found in prefs */
	all_providers = add_io_providers_from_prefs( pivot, merged_providers );

	st_io_providers = all_providers;
}

static GList *
allocate_ordered_providers( GSList *priority )
{
	GSList *ip;
	NAIOProvider *provider;
	GList *providers;

	providers = NULL;

	for( ip = priority ; ip ; ip = ip->next ){

		provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, ( const gchar * ) ip->data, NULL );
		providers = g_list_prepend( providers, provider );
	}

	return( g_list_reverse( providers ));
}

/*
 * merge the ordered list of I/O providers (which have only Id)
 * with those found available at runtime
 */
static GList *
merge_available_io_providers( const NAPivot *pivot, GList *ordered )
{
	static const gchar *thisfn = "na_io_provider_merge_available_io_providers";
	GList *merged;
	GList *module_objects, *im;
	gchar *id;
	NAIOProvider *provider;

	merged = ordered;

	module_objects = na_pivot_get_providers( pivot, NA_IIO_PROVIDER_TYPE );
	for( im = module_objects ; im ; im = im->next ){

		id = NULL;
		if( NA_IIO_PROVIDER_GET_INTERFACE( NA_IIO_PROVIDER( im->data ))->get_id ){
			id = NA_IIO_PROVIDER_GET_INTERFACE( NA_IIO_PROVIDER( im->data ))->get_id( NA_IIO_PROVIDER( im->data ));

		} else {
			g_warning( "%s: NAIIOProvider %p doesn't support get_id() interface", thisfn, ( void * ) im->data );
		}

		provider = NULL;
		if( id ){
			provider = na_io_provider_find_provider_by_id( merged, id );

			if( !provider ){
				g_debug( "%s: no provider already allocated in ordered list for id=%s", thisfn, id );
				provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );
				merged = g_list_append( merged, provider );

			} else {
				g_debug( "%s: found NAIOProvider=%p (NAIIOProvider=%p) for id=%s",
						thisfn, ( void * ) provider, ( void * ) im->data, id );
			}

			io_provider_set_provider( provider, NA_IIO_PROVIDER( im->data ), pivot );

			g_free( id );
		}
	}

	na_pivot_free_providers( module_objects );

	return( merged );
}

static void
io_provider_set_provider( NAIOProvider *provider, NAIIOProvider *instance, const NAPivot *pivot )
{
	static const gchar *thisfn = "na_io_provider_set_provider";

	g_return_if_fail( NA_IS_IO_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IIO_PROVIDER( instance ));

	provider->private->provider = g_object_ref( instance );

	if( NA_IIO_PROVIDER_GET_INTERFACE( instance )->get_name ){
		provider->private->name = NA_IIO_PROVIDER_GET_INTERFACE( instance )->get_name( instance );
	} else {
		g_warning( "%s: NAIIOProvider %p doesn't support get_name() interface", thisfn, ( void * ) instance );
	}

	provider->private->item_changed_handler =
			g_signal_connect(
					instance,
					IO_PROVIDER_SIGNAL_ITEM_CHANGED,
					( GCallback ) na_pivot_item_changed_handler,
					( gpointer ) pivot );
}

static GList *
add_io_providers_from_prefs( const NAPivot *pivot, GList *runtime_providers )
{
	gchar *path, *id;
	GSList *ids, *iid;
	GList *providers;
	NAIOProvider *provider;

	providers = runtime_providers;
	path = gconf_concat_dir_and_key( IPREFS_GCONF_BASEDIR, IO_PROVIDER_KEY_ROOT );
	gconf = na_iprefs_get_gconf_client( NA_IPREFS( pivot ));

	ids = na_gconf_utils_get_subdirs( gconf, path );

	for( iid = ids ; iid ; iid = iid->next ){
		id = g_path_get_basename(( const gchar * ) iid->data );
		provider = na_io_provider_find_provider_by_id( providers, id );

		if( !provider ){
			provider = g_object_new( NA_IO_PROVIDER_TYPE, IO_PROVIDER_PROP_ID, id, NULL );
			providers = g_list_append( providers, provider );
		}

		g_free( id );
	}

	na_gconf_utils_free_subdirs( ids );
	g_free( path );

	return( providers );
}

/*
 * na_io_provider_reorder_providers_list:
 * @pivot: the #NAPivot instance of the application.
 *
 * Reorder our global list of #NAIOProviders,after the user has reordered
 * them in user preferences.
 */
void
na_io_provider_reorder_providers_list( const NAPivot *pivot )
{
	GSList *order, *io;
	GList *new_list;
	NAIOProvider *provider;

	new_list = NULL;
	order = na_iprefs_read_string_list( NA_IPREFS( pivot ), IO_PROVIDER_KEY_ORDER, NULL );

	for( io = order ; io ; io = io->next ){
		provider = na_io_provider_find_provider_by_id( st_io_providers, ( const gchar * ) io->data );
		if( provider ){
			st_io_providers = g_list_remove( st_io_providers, provider );
			new_list = g_list_prepend( new_list, provider );
		}
	}

	st_io_providers = g_list_reverse( new_list );

	na_core_utils_slist_free( order );
}

/*
 * na_io_provider_is_user_readable_at_startup:
 * @provider: this #NAIOProvider.
 * @iprefs: an implementor of the #NAIPrefs interface.
 *
 * Returns: %TRUE is this I/O provider should be read at startup, and so
 * may participate to the global list of menus and actions.
 *
 * This is a user preference.
 * If the preference is not recorded, then it defaults to %TRUE.
 * This means that the user may adjust its personal configuration to
 * fully ignore menu/action items from a NAIIOProvider, just by setting
 * the corresponding flag to %FALSE.
 */
gboolean
na_io_provider_is_user_readable_at_startup( const NAIOProvider *provider, const NAIPrefs *iprefs )
{
	gboolean to_be_read;
	GConfClient *gconf;
	gchar *path, *key, *entry;

	to_be_read = FALSE;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IPREFS( iprefs ), FALSE );

	if( !provider->private->dispose_has_run ){

		gconf = na_iprefs_get_gconf_client( iprefs );

		path = gconf_concat_dir_and_key( IPREFS_GCONF_BASEDIR, IO_PROVIDER_KEY_ROOT );
		key = gconf_concat_dir_and_key( path, provider->private->id );
		entry = gconf_concat_dir_and_key( key, NA_IPREFS_IO_PROVIDER_READABLE );

		to_be_read = na_gconf_utils_read_bool( gconf, entry, FALSE, TRUE );

		g_free( entry );
		g_free( key );
		g_free( path );
	}

	return( to_be_read );
}

/*
 * na_io_provider_find_provider_by_id:
 * @providers: the current list of #NAIOProvider.
 * @id: the searched internal id.
 *
 * Returns: the searched #NAIOProvider, or %NULL if not found.
 *
 * The returned object is owned by #NAIOProvider class, and should not
 * be g_object_unref() by the user.
 */
NAIOProvider *
na_io_provider_find_provider_by_id( GList *providers, const gchar *id )
{
	NAIOProvider *provider;
	GList *ip;

	provider = NULL;

	for( ip = providers ; ip && !provider ; ip = ip->next ){

		if( !strcmp( NA_IO_PROVIDER( ip->data )->private->id, id )){

			provider = NA_IO_PROVIDER( ip->data );
		}
	}

	return( provider );
}

/*
 * na_io_provider_is_locked_by_admin:
 * @provider: this #NAIOProvider.
 * @iprefs: an implementor of the #NAIPrefs interface.
 *
 * Returns: %TRUE is this I/O provider has been locked by an admin.
 */
gboolean
na_io_provider_is_locked_by_admin( const NAIOProvider *provider, const NAIPrefs *iprefs )
{
	gboolean locked;
	GConfClient *gconf;
	gchar *path;

	locked = FALSE;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IPREFS( iprefs ), FALSE );

	if( !provider->private->dispose_has_run ){

		gconf = na_iprefs_get_gconf_client( iprefs );

		path = g_strdup_printf( "%s/mandatory/%s/locked", IPREFS_GCONF_BASEDIR, provider->private->id );

		locked = na_gconf_utils_read_bool( gconf, path, FALSE, FALSE );

		g_free( path );
	}

	return( locked );
}

/*
 * na_io_provider_has_write_api:
 * @provider: this #NAIOProvider.
 *
 * Returns: %TRUE is the NAIIOProvider implements write and delete api.
 */
gboolean
na_io_provider_has_write_api( const NAIOProvider *provider )
{
	gboolean has_api;

	has_api = FALSE;
	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), has_api );

	if( !provider->private->dispose_has_run ){

		if( provider->private->provider ){

			g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), FALSE );

			has_api =
					NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item &&
					NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item;
		}
	}

	return( has_api );
}
#endif

/*
 * na_io_provider_write_item:
 * @provider: this #NAIOProvider object.
 * @item: a #NAObjectItem to be written to the storage subsystem.
 * @messages: error messages.
 *
 * Writes an @item to the storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 *
 * #NAPivot class, which should be the only caller of this function,
 * has already check that this item is writable, i.e. that all conditions
 * are met to be able to successfully write the item down to the
 * storage subsystem.
 */
guint
na_io_provider_write_item( const NAIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_write_item";
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), ret );

	ret = NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->write_item( provider->private->provider, item, messages );

	return( ret );
}

/*
 * na_io_provider_delete_item:
 * @provider: this #NAIOProvider object.
 * @item: the #NAObjectItem item to be deleted.
 * @messages: error messages.
 *
 * Deletes an item (action or menu) from the storage subsystem.
 *
 * Returns: the NAIIOProvider return code.
 */
guint
na_io_provider_delete_item( const NAIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_delete_item";
	guint ret;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), ret );

	ret = NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->delete_item( provider->private->provider, item, messages );

	return( ret );
}

/*
 * na_io_provider_duplicate_data:
 * @provider: this #NAIOProvider object.
 * @dest: the target #NAObjectItem item.
 * @source: the source #NAObjectItem item.
 * @messages: error messages.
 *
 * Duplicates provider data (if any) from @source to @dest.
 *
 * Returns: the NAIIOProvider return code.
 */
guint
na_io_provider_duplicate_data( const NAIOProvider *provider, NAObjectItem *dest, const NAObjectItem *source, GSList **messages )
{
	static const gchar *thisfn = "na_io_provider_duplicate_data";
	guint ret;
	void *provider_data;

	g_debug( "%s: provider=%p (%s), dest=%p (%s), source=%p (%s), messages=%p", thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) dest, G_OBJECT_TYPE_NAME( dest ),
			( void * ) source, G_OBJECT_TYPE_NAME( source ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( dest ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( source ), ret );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider->private->provider ), ret );

	na_object_set_provider_data( dest, NULL );
	provider_data = na_object_get_provider_data( source );

	if( provider_data &&
		NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->duplicate_data ){
			ret = NA_IIO_PROVIDER_GET_INTERFACE( provider->private->provider )->duplicate_data( provider->private->provider, dest, source, messages );
	}

	return( ret );
}

/*
 * na_io_provider_get_readonly_tooltip:
 * @reason: the reason for why an item is not writable.
 *
 * Returns: the associated tooltip, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
na_io_provider_get_readonly_tooltip( guint reason )
{
	gchar *tooltip;

	tooltip = NULL;

	switch( reason ){
		case NA_IIO_PROVIDER_STATUS_ITEM_READONLY:
			tooltip = g_strdup( _( "Item is read-only." ));
			break;

		case NA_IIO_PROVIDER_STATUS_PROVIDER_NOT_WILLING_TO:
			tooltip = g_strdup( _( "I/O provider is not willing to write." ));
			break;

		case NA_IIO_PROVIDER_STATUS_NO_PROVIDER_FOUND:
			tooltip = g_strdup( _( "No writable I/O provider found." ));
			break;

		case NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_ADMIN:
			tooltip = g_strdup( _( "I/O provider has been locked down by an administrator." ));
			break;

		case NA_IIO_PROVIDER_STATUS_PROVIDER_LOCKED_BY_USER:
			tooltip = g_strdup( _( "I/O provider has been locked down by the user." ));
			break;

		case NA_IIO_PROVIDER_STATUS_NO_API:
			tooltip = g_strdup( _( "I/O provider implementation lacks of required API." ));
			break;

		case NA_IIO_PROVIDER_STATUS_CONFIGURATION_LOCKED_BY_ADMIN:
			tooltip = g_strdup( _( "The whole configuration has been locked down by an administrator." ));
			break;

		/* item is writable, so tooltip is empty */
		case 0:
			tooltip = g_strdup( "" );
			break;

		default:
			tooltip = g_strdup_printf( _( "Item is not writable for an unknown reason (%d).\n" \
					"Please, be kind enough to fill out a bug report on http://bugzilla.gnome.org." ), reason );
			break;
	}

	return( tooltip );
}

/*
 * na_io_provider_get_return_code_label:
 * @code: the return code of an operation.
 *
 * Returns: the associated label, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
na_io_provider_get_return_code_label( guint code )
{
	gchar *label;

	label = NULL;

	switch( code ){
		case NA_IIO_PROVIDER_CODE_OK:
			label = g_strdup( _( "OK." ));
			break;

		case NA_IIO_PROVIDER_CODE_PROGRAM_ERROR:
			label = g_strdup( _( "Program flow error.\n" \
					"Please, be kind enough to fill out a bug report on http://bugzilla.gnome.org." ));
			break;

		case NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN:
			label = g_strdup( _( "The I/O provider is not willing to do that." ));
			break;

		case NA_IIO_PROVIDER_CODE_WRITE_ERROR:
			label = g_strdup( _( "Write error in I/O provider." ));
			break;

		case NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR:
			label = g_strdup( _( "Unable to delete GConf schemas." ));
			break;

		case NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR:
			label = g_strdup( _( "Unable to delete configuration." ));
			break;

		default:
			label = g_strdup_printf( _( "Unknown return code (%d).\n" \
					"Please, be kind enough to fill out a bug report on http://bugzilla.gnome.org." ), code );
			break;
	}

	return( label );
}
