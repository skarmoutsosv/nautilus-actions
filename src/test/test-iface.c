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

/* We verify here that derived class can also use interfaces implemented
 * in base class.
 */

#include <glib.h>

#include "test-iface-derived.h"
#include "test-iface-iface.h"

int
main( int argc, char **argv )
{
	TestBase *base, *base2;
	TestDerived *derived;

	g_type_init();

	g_debug( "allocating TestBase -------------------------------------" );
	base = test_base_new();
	g_debug( "calling test_iface_fna on Base object -------------------" );
	test_iface_fna( TEST_IFACE( base ));
	g_debug( "calling test_iface_fnb on Base object -------------------" );
	test_iface_fnb( TEST_IFACE( base ));

	g_debug( "allocating TestDerived ----------------------------------" );
	derived = test_derived_new();
	g_debug( "calling test_iface_fna on Derived object ----------------" );
	test_iface_fna( TEST_IFACE( derived ));
	g_debug( "calling test_iface_fnb on Derived object ----------------" );
	test_iface_fnb( TEST_IFACE( derived ));

	g_debug( "allocating TestBase -------------------------------------" );
	base2 = test_base_new();
	g_debug( "calling test_iface_fna on Base object -------------------" );
	test_iface_fna( TEST_IFACE( base2 ));
	g_debug( "calling test_iface_fnb on Base object -------------------" );
	test_iface_fnb( TEST_IFACE( base2 ));

	g_debug( "end -----------------------------------------------------" );

	return( 0 );
}