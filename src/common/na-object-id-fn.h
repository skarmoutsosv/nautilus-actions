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

#ifndef __NA_OBJECT_ID_FN_H__
#define __NA_OBJECT_ID_FN_H__

/**
 * SECTION: na_object_id
 * @short_description: #NAObjectId class definition.
 * @include: common/na-object-id-fn.h
 *
 * A #NAObjectId object is characterized by :
 * - an internal identifiant (ASCII, case insensitive)
 * - a libelle (UTF8, localizable).
 *
 * The #NAObjectId class is a pure virtual class.
 */

#include "na-object-id-class.h"

G_BEGIN_DECLS

gchar *na_object_id_get_id( const NAObjectId *object );
gchar *na_object_id_get_label( const NAObjectId *object );

void   na_object_id_set_id( NAObjectId *object, const gchar *id );
void   na_object_id_set_label( NAObjectId *object, const gchar *label );

G_END_DECLS

#endif /* __NA_OBJECT_ID_FN_H__ */