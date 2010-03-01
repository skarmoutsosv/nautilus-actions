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

#include <string.h>

#include <api/na-data-def.h>
#include <api/na-data-types.h>
#include <api/na-iio-provider.h>
#include <api/na-ifactory-provider.h>
#include <api/na-object-api.h>
#include <api/na-core-utils.h>
#include <api/na-gconf-utils.h>

#include "nagp-gconf-provider.h"
#include "nagp-writer.h"
#include "nagp-keys.h"

typedef struct {
	gchar *parent_id;
}
	WriterData;

/*
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_willing_to_write( const NAIIOProvider *provider )
{
	return( TRUE );
}

/*
 * Rationale: gconf reads its storage path from /etc/gconf/2/path ;
 * there is there a 'xml:readwrite:$(HOME)/.gconf' line, but I do not
 * known any way to get it programatically, so an admin may have set a
 * readwrite space elsewhere..
 *
 * So, we try to write a 'foo' key somewhere: if it is ok, then the
 * provider is supposed able to write...
 *
 * API function: should only be called through NAIIOProvider interface
 */
gboolean
nagp_iio_provider_is_able_to_write( const NAIIOProvider *provider )
{
	/*static const gchar *thisfn = "nagp_iio_provider_is_able_to_write";*/
	static const gchar *path = "/apps/nautilus-actions/foo";
	NagpGConfProvider *self;
	gboolean able_to = FALSE;

	/*g_debug( "%s: provider=%p", thisfn, ( void * ) provider );*/
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), FALSE );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), FALSE );

	self = NAGP_GCONF_PROVIDER( provider );

	if( !self->private->dispose_has_run ){

		if( !na_gconf_utils_write_string( self->private->gconf, path, "1", NULL )){
			able_to = FALSE;

		} else if( !gconf_client_recursive_unset( self->private->gconf, path, 0, NULL )){
			able_to = FALSE;

		} else {
			able_to = TRUE;
		}
	}

	/*g_debug( "%s: provider=%p, able_to=%s", thisfn, ( void * ) provider, able_to ? "True":"False" );*/
	return( able_to );
}

/*
 * update an existing item or write a new one
 * in all cases, it is much more easy to delete the existing  entries
 * before trying to write the new ones
 */
guint
nagp_iio_provider_write_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_write_item";
	NagpGConfProvider *self;
	guint ret;
	WriterData *data;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = nagp_iio_provider_delete_item( provider, item, messages );

	if( ret == NA_IIO_PROVIDER_CODE_OK ){

		data = g_new0( WriterData, 1 );

		na_ifactory_provider_write_item( NA_IFACTORY_PROVIDER( provider ), data, NA_IFACTORY_OBJECT( item ), messages );

		g_free( data );
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	return( ret );
}

/*
 * also delete the schema which may be directly attached to this action
 * cf. http://bugzilla.gnome.org/show_bug.cgi?id=325585
 */
guint
nagp_iio_provider_delete_item( const NAIIOProvider *provider, const NAObjectItem *item, GSList **messages )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_delete_item";
	NagpGConfProvider *self;
	guint ret;
	gchar *uuid, *path;
	GError *error = NULL;

	g_debug( "%s: provider=%p (%s), item=%p (%s), messages=%p",
			thisfn,
			( void * ) provider, G_OBJECT_TYPE_NAME( provider ),
			( void * ) item, G_OBJECT_TYPE_NAME( item ),
			( void * ) messages );

	ret = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;

	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), ret );
	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), ret );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( item ), ret );

	self = NAGP_GCONF_PROVIDER( provider );

	if( self->private->dispose_has_run ){
		return( NA_IIO_PROVIDER_CODE_NOT_WILLING_TO_RUN );
	}

	ret = NA_IIO_PROVIDER_CODE_OK;
	uuid = na_object_get_id( NA_OBJECT( item ));

	path = gconf_concat_dir_and_key( NAGP_SCHEMAS_PATH, uuid );
	gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
	if( error ){
		g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
		*messages = g_slist_append( *messages, g_strdup( error->message ));
		g_error_free( error );
		error = NULL;
		ret = NA_IIO_PROVIDER_CODE_DELETE_SCHEMAS_ERROR;
	}
	g_free( path );

	if( ret == NA_IIO_PROVIDER_CODE_OK ){
		path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH, uuid );
		gconf_client_recursive_unset( self->private->gconf, path, 0, &error );
		if( error ){
			g_warning( "%s: path=%s, error=%s", thisfn, path, error->message );
			*messages = g_slist_append( *messages, g_strdup( error->message ));
			g_error_free( error );
			error = NULL;
			ret = NA_IIO_PROVIDER_CODE_DELETE_CONFIG_ERROR;
		}
	}

	gconf_client_suggest_sync( self->private->gconf, NULL );

	g_free( path );
	g_free( uuid );

	return( ret );
}

guint
nagp_writer_write_start( const NAIFactoryProvider *writer, void *writer_data,
							const NAIFactoryObject *object, GSList **messages  )
{
	guint code;
	gchar *id, *parent_path, *path;
	gchar *msg;

	code = NA_IIO_PROVIDER_CODE_OK;

	if( NA_IS_OBJECT_ITEM( object )){
		g_return_val_if_fail(
				(( WriterData * ) writer_data )->parent_id == NULL,
				NA_IIO_PROVIDER_CODE_PROGRAM_ERROR );

		id = na_object_get_id( object );
		parent_path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH, id );
		path = gconf_concat_dir_and_key( parent_path, NAGP_ENTRY_TYPE );

		msg = NULL;
		na_gconf_utils_write_string(
				NAGP_GCONF_PROVIDER( writer )->private->gconf,
				path,
				NA_IS_OBJECT_ACTION( object ) ? NAGP_VALUE_TYPE_ACTION : NAGP_VALUE_TYPE_MENU,
				&msg );
		if( msg ){
			*messages = g_slist_append( *messages, msg );
			code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
		}

		g_free( path );
		g_free( parent_path );
		g_free( id );
	}

	return( code );
}

guint
nagp_writer_write_data( const NAIFactoryProvider *provider, void *writer_data,
									const NAIFactoryObject *object, const NADataBoxed *boxed,
									GSList **messages )
{
	static const gchar *thisfn = "nagp_writer_write_data";
	guint code;
	NADataDef *def;
	gchar *id;
	gchar *parent_path, *dir_path, *path;
	gchar *msg;
	gchar *str_value;
	gboolean bool_value;
	GSList *slist_value;
	guint uint_value;
	GConfClient *gconf;

	/*g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));*/

	msg = NULL;
	code = NA_IIO_PROVIDER_CODE_OK;
	def = na_data_boxed_get_data_def( boxed );

	id = na_object_get_id( object );

	parent_path = gconf_concat_dir_and_key( NAGP_CONFIGURATIONS_PATH,
			(( WriterData * ) writer_data )->parent_id ?
					(( WriterData * ) writer_data )->parent_id : id );

	dir_path = (( WriterData * ) writer_data )->parent_id ?
					gconf_concat_dir_and_key( parent_path, id ) : g_strdup( parent_path );

	path = gconf_concat_dir_and_key( dir_path, def->gconf_entry );

	gconf = NAGP_GCONF_PROVIDER( provider )->private->gconf;

	switch( def->type ){

		case NAFD_TYPE_STRING:
			str_value = na_data_boxed_get_as_string( boxed );
			if( str_value && strlen( str_value )){
				na_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
			}
			g_free( str_value );
			break;

		case NAFD_TYPE_LOCALE_STRING:
			str_value = na_data_boxed_get_as_string( boxed );
			if( str_value && g_utf8_strlen( str_value, -1 )){
				na_gconf_utils_write_string( gconf, path, str_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
			}
			g_free( str_value );
			break;

		case NAFD_TYPE_BOOLEAN:
			bool_value = GPOINTER_TO_UINT( na_data_boxed_get_as_void( boxed ));
			na_gconf_utils_write_bool( gconf, path, bool_value, &msg );
			if( msg ){
				*messages = g_slist_append( *messages, msg );
				code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
			}
			break;

		case NAFD_TYPE_STRING_LIST:
			slist_value = ( GSList * ) na_data_boxed_get_as_void( boxed );
			if( slist_value && g_slist_length( slist_value )){
				na_gconf_utils_write_string_list( gconf, path, slist_value, &msg );
				if( msg ){
					*messages = g_slist_append( *messages, msg );
					code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
				}
			}
			na_core_utils_slist_free( slist_value );
			break;

		case NAFD_TYPE_UINT:
			uint_value = GPOINTER_TO_UINT( na_data_boxed_get_as_void( boxed ));
			na_gconf_utils_write_int( gconf, path, uint_value, &msg );
			if( msg ){
				*messages = g_slist_append( *messages, msg );
				code = NA_IIO_PROVIDER_CODE_WRITE_ERROR;
			}
			break;

		default:
			g_warning( "%s: unknown type=%u for %s", thisfn, def->type, def->name );
			code = NA_IIO_PROVIDER_CODE_PROGRAM_ERROR;
	}

	/*g_debug( "%s: gconf=%p, code=%u, path=%s", thisfn, ( void * ) gconf, code, path );*/

	g_free( msg );
	g_free( path );
	g_free( parent_path );
	g_free( id );

	return( code );
}

guint
nagp_writer_write_done( const NAIFactoryProvider *writer, void *writer_data,
									const NAIFactoryObject *object,
									GSList **messages  )
{
	guint code;
	GSList *children_slist, *ic;
	WriterData *data;
	NAObjectProfile *profile;

	code = NA_IIO_PROVIDER_CODE_OK;

	if( NA_IS_OBJECT_ACTION( object )){
		children_slist = na_object_get_items_slist( object );

		for( ic = children_slist ; ic && code == NA_IIO_PROVIDER_CODE_OK ; ic = ic->next ){
			data = g_new0( WriterData, 1 );
			data->parent_id = na_object_get_id( object );

			profile = NA_OBJECT_PROFILE( na_object_get_item( object, ic->data ));

			code = na_ifactory_provider_write_item( writer, data, NA_IFACTORY_OBJECT( profile ), messages );

			g_free( data->parent_id );
			g_free( data );
		}
	}

	return( code );
}
