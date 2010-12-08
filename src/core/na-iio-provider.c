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

#include <api/na-iio-provider.h>

/**
 * SECTION: iio-provider
 * @title: NAIIOProvider
 * @short_description: The I/O Provider Interface
 * @include: nautilus-actions/na-iio-provider.h
 *
 * The #NAIIOProvider interface is defined in order to let internal
 * and external plugins provide read and write accesses
 * to an alternate storage subsystem.
 *
 * The #NAIIOProvider interface provides three types of services:
 * <itemizedlist>
 *  <listitem>
 *   <para>
 *    load all items at startup;
 *   </para>
 *  </listitem>
 *  <listitem>
 *   <para>
 *    create, update or delete items via the management user interface;
 *   </para>
 *  </listitem>
 *  <listitem>
 *   <para>
 *    inform Nautilus-Actions when an item has been modified on the
 *    underlying storage subsystem.
 *   </para>
 *  </listitem>
 * </itemizedlist>
 *
 * These services may be fully implemented by the I/O provider itself.
 * Or, the I/O provider may also prefer to take advantage of the data
 * factory management (see #NAIFactoryObject and #NAIFactoryProvider interfaces).
 *
 * Since Nautilus-Actions v 2.30 (API version 1)
 */

/* private interface data
 */
struct _NAIIOProviderInterfacePrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* signals
 */
enum {
	ITEM_CHANGED,
	LAST_SIGNAL
};

static gboolean st_initialized            = FALSE;
static gboolean st_finalized              = FALSE;
static gint     st_signals[ LAST_SIGNAL ] = { 0 };

static GType    register_type( void );
static void     interface_base_init( NAIIOProviderInterface *klass );
static void     interface_base_finalize( NAIIOProviderInterface *klass );

static gboolean do_is_willing_to_write( const NAIIOProvider *instance );
static gboolean do_is_able_to_write( const NAIIOProvider *instance );

/**
 * na_iio_provider_get_type:
 *
 * Returns: the #GType type of this interface.
 */
GType
na_iio_provider_get_type( void )
{
	static GType type = 0;

	if( !type ){
		type = register_type();
	}

	return( type );
}

/*
 * na_iio_provider_register_type:
 *
 * Registers this interface.
 */
static GType
register_type( void )
{
	static const gchar *thisfn = "na_iio_provider_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( NAIIOProviderInterface ),
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

	type = g_type_register_static( G_TYPE_INTERFACE, "NAIIOProvider", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_init";

	if( !st_initialized ){

		g_debug( "%s: klass%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( NAIIOProviderInterfacePrivate, 1 );

		klass->get_id = NULL;
		klass->get_version = NULL;
		klass->read_items = NULL;
		klass->is_willing_to_write = do_is_willing_to_write;
		klass->is_able_to_write = do_is_able_to_write;
		klass->write_item = NULL;
		klass->delete_item = NULL;
		klass->duplicate_data = NULL;

		/* register the signal (without any default handler)
		 * this signal should be sent by the #NAIIOProvider instance when
		 * an item has changed in the underlying storage subsystem
		 * #NAPivot is connected to this signal
		 */
		st_signals[ ITEM_CHANGED ] = g_signal_new(
					IIO_PROVIDER_SIGNAL_ITEM_CHANGED,
					NA_IIO_PROVIDER_TYPE,
					G_SIGNAL_RUN_LAST,
					0,
					NULL,
					NULL,
					g_cclosure_marshal_VOID__POINTER,
					G_TYPE_NONE,
					1,
					G_TYPE_POINTER );

		st_initialized = TRUE;
	}
}

static void
interface_base_finalize( NAIIOProviderInterface *klass )
{
	static const gchar *thisfn = "na_iio_provider_interface_base_finalize";

	if( st_initialized && !st_finalized ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		st_finalized = TRUE;

		g_free( klass->private );
	}
}

static gboolean
do_is_willing_to_write( const NAIIOProvider *instance )
{
	return( FALSE );
}

static gboolean
do_is_able_to_write( const NAIIOProvider *instance )
{
	return( FALSE );
}

/**
 * na_iio_provider_item_changed:
 * @instance: the calling #NAIIOProvider.
 *
 * Advertises Nautilus-Actions that this #NAIIOProvider @instance has
 * detected a modification in one of its configurations (menu or action).
 *
 * This function should be triggered for each and every #NAObjectItem-
 * derived modified objects, but (if possible) only once for each one.
 *
 * When receiving this signal, the current &prodname; program will
 * automatically ask its I/O providers for a current list of menus and
 * actions, or ask the user if he is willing to reload such a current
 * list, depending of the exact running &prodname; program.
 */
void
na_iio_provider_item_changed( const NAIIOProvider *instance )
{
	static const gchar *thisfn = "na_iio_provider_item_changed";

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	g_signal_emit_by_name(( gpointer ) instance, IIO_PROVIDER_SIGNAL_ITEM_CHANGED, NULL );
}
