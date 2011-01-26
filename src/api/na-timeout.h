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

#ifndef __NAUTILUS_ACTIONS_API_NA_TIMEOUT_H__
#define __NAUTILUS_ACTIONS_API_NA_TIMEOUT_H__

/**
 * SECTION: timeout
 * @title: NATimeout
 * @short_description: The NATimeout Structure
 * @include: nautilus-actions/na-timeout.h
 *
 * The NATimeout structure is a convenience structure to manage timeout
 * functions.
 *
 * Since: 3.1.0
 */

#include <glib-object.h>

G_BEGIN_DECLS

typedef void ( *NATimeoutFunc )( void *user_data );

/**
 * NATimeout:
 * @timeout:   (i) timeout configurable parameter (ms)
 * @handler:   (i) handler function
 * @user_data: (i) user data
 * @private:   private data
 *
 * This structure let the user (i.e. the code which uses it) manage functions
 * which should only be called after some time of inactivity, which is typically
 * the case of 'item-change' handlers.
 *
 * The structure is supposed to be initialized at construction time with
 * @timeout in milliseconds, @handler and @user_data input parameters.
 * The @private pointer identifier must be set to %NULL.
 *
 * Such a structure must be allocated for each managed event.
 *
 * When an event is detected, the na_timeout_event() function must be called
 * with this structure. The function makes sure that the @handler callback
 * will be triggered as soon as no event will be recorded after @timeout
 * milliseconds of inactivity.
 *
 * Since: 3.1.0
 */
typedef struct {
	/*< public >*/
	guint         timeout;
	NATimeoutFunc handler;
	gpointer      user_data;
	/*< private >*/
	GTimeVal      last_time;
	guint         source_id;
}
	NATimeout;

void na_timeout_event( NATimeout *timeout );

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_API_NA_TIMEOUT_H__ */