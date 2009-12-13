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

#ifndef __NA_TRACKER_H__
#define __NA_TRACKER_H__

/**
 * SECTION: na_tracker
 * @short_description: #NATracker class definition.
 * @include: tracker/na-tracker.h
 *
 * There is only one #NATracker object in the process. As any Nautilus
 * extension, it is instantiated when the module is loaded by the file
 * manager at startup time.
 *
 * The #NATracker object initiates the DBus connection, and allocates
 * the object responsable of effective tracking.
 */

#include <glib-object.h>

G_BEGIN_DECLS

#define NA_TRACKER_TYPE					( na_tracker_get_type())
#define NA_TRACKER( object )			( G_TYPE_CHECK_INSTANCE_CAST(( object ), NA_TRACKER_TYPE, NATracker ))
#define NA_TRACKER_CLASS( klass )		( G_TYPE_CHECK_CLASS_CAST(( klass ), NA_TRACKER_TYPE, NATrackerClass ))
#define NA_IS_TRACKER( object )			( G_TYPE_CHECK_INSTANCE_TYPE(( object ), NA_TRACKER_TYPE ))
#define NA_IS_TRACKER_CLASS( klass )	( G_TYPE_CHECK_CLASS_TYPE(( klass ), NA_TRACKER_TYPE ))
#define NA_TRACKER_GET_CLASS( object )	( G_TYPE_INSTANCE_GET_CLASS(( object ), NA_TRACKER_TYPE, NATrackerClass ))

typedef struct NATrackerPrivate NATrackerPrivate;

typedef struct
{
	GObject           parent;
	NATrackerPrivate *private;
}
	NATracker;

typedef struct NATrackerClassPrivate NATrackerClassPrivate;

typedef struct
{
	GObjectClass           parent;
	NATrackerClassPrivate *private;
}
	NATrackerClass;

GType na_tracker_get_type( void );
void  na_tracker_register_type( GTypeModule *module );

G_END_DECLS

#endif /* __NA_TRACKER_H__ */