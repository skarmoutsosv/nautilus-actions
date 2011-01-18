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

#include <core/na-exporter.h>
#include <core/na-export-format.h>

#include "nact-export-format.h"
#include "nact-gtk-utils.h"

typedef struct {
	GtkWidget      *container_vbox;
	GtkRadioButton *button;
	NAExportFormat *format;
	gulong          handler;	/* 'toggled' signal handler id */
}
	VBoxData;

#define EXPORT_FORMAT_PROP_CONTAINER_FORMAT		"nact-export-format-prop-container-format"
#define EXPORT_FORMAT_PROP_CONTAINER_EDITABLE	"nact-export-format-prop-container-editable"
#define EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE	"nact-export-format-prop-container-sensitive"
#define EXPORT_FORMAT_PROP_VBOX_DATA			"nact-export-format-prop-vbox-data"

#define ASKME_LABEL						N_( "_Ask me" )
#define ASKME_DESCRIPTION				N_( "You will be asked for the format to choose each time an item is about to be exported." )

static const NAIExporterFormat st_ask_str = { "Ask", ASKME_LABEL, ASKME_DESCRIPTION };

static void draw_in_vbox( GtkWidget *container_vbox, const NAExportFormat *format, guint mode, gint id );
static void format_weak_notify( VBoxData *vbox_data, GObject *vbox );
static void select_default_iter( GtkWidget *widget, void *quark_ptr );
static void export_format_on_toggled( GtkToggleButton *toggle_button, VBoxData *vbox_data );
static void get_selected_iter( GtkWidget *widget, NAExportFormat **format );

/**
 * nact_export_format_init_display:
 * @container_vbox: the #GtkVBox container in which the display must be drawn.
 * @pivot: the #NAPivot instance.
 * @mode: the type of the display.
 * @sensitive: whether the whole radio button group is sensitive.
 *
 * Displays the available export formats in the VBox.
 *
 * Should only be called once per dialog box instance (e.g. on initial load
 * of the GtkWindow).
 */
void
nact_export_format_init_display( GtkWidget *container_vbox, const NAPivot *pivot, guint mode, gboolean sensitive )
{
	static const gchar *thisfn = "nact_export_format_init_display";
	GList *formats, *ifmt;
	NAExportFormat *format;

	g_debug( "%s: container_vbox=%p, pivot=%p, mode=%u, sensitive=%s",
			thisfn, ( void * ) container_vbox, ( void * ) pivot, mode,
			sensitive ? "True":"False" );

	formats = na_exporter_get_formats( pivot );

	for( ifmt = formats ; ifmt ; ifmt = ifmt->next ){
		draw_in_vbox( container_vbox, NA_EXPORT_FORMAT( ifmt->data ), mode, -1 );
	}

	na_exporter_free_formats( formats );

	switch( mode ){

		/* this is the mode to be used when we are about to export an item
		 * and the user preference is 'Ask me'; obviously, we don't propose
		 * here to ask him another time :)
		 */
		case EXPORT_FORMAT_DISPLAY_ASK:
			break;

		case EXPORT_FORMAT_DISPLAY_PREFERENCES:
		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			format = na_export_format_new( &st_ask_str, NULL );
			draw_in_vbox( container_vbox, format, mode, IPREFS_EXPORT_FORMAT_ASK );
			g_object_unref( format );
			break;

		default:
			g_warning( "%s: mode=%d: unknown mode", thisfn, mode );
	}

	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( 0 ));
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE, GUINT_TO_POINTER( sensitive ));
}

/*
 * container_box
 *  +- vbox                 new
 *  |   +- radio button     new
 *  |   +- hbox             new
 *  |   |   +- description  new
 */
static void
draw_in_vbox( GtkWidget *container_vbox, const NAExportFormat *format, guint mode, gint id )
{
	static GtkRadioButton *first_button = NULL;
	GtkVBox *vbox;
	gchar *description;
	GtkHBox *hbox;
	GtkRadioButton *button;
	guint size, spacing;
	gchar *markup, *label;
	GtkLabel *desc_label;
	VBoxData *vbox_data;

	vbox = GTK_VBOX( gtk_vbox_new( FALSE, 0 ));
	gtk_box_pack_start( GTK_BOX( container_vbox ), GTK_WIDGET( vbox ), FALSE, TRUE, 0 );
	description = na_export_format_get_description( format );
	g_object_set( G_OBJECT( vbox ), "tooltip-text", description, NULL );
	g_object_set( G_OBJECT( vbox ), "spacing", 6, NULL );

	button = GTK_RADIO_BUTTON( gtk_radio_button_new( NULL ));
	if( first_button ){
		g_object_set( G_OBJECT( button ), "group", first_button, NULL );
	} else {
		first_button = button;
	}
	gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( button ), FALSE, TRUE, 0 );

	label = NULL;
	markup = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASK:
		case EXPORT_FORMAT_DISPLAY_PREFERENCES:
		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			label = na_export_format_get_label( format );
			markup = g_markup_printf_escaped( "%s", label );
			gtk_button_set_label( GTK_BUTTON( button ), label );
			g_object_set( G_OBJECT( button ), "use_underline", TRUE, NULL );
			break;

		/* this work fine, but it appears that this is not consistant with import assistant */
		/*case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			radio_label = GTK_LABEL( gtk_label_new( NULL ));
			label = na_export_format_get_label( format );
			markup = g_markup_printf_escaped( "<b>%s</b>", label );
			gtk_label_set_markup( radio_label, markup );
			gtk_container_vbox_add( GTK_CONTAINER( button ), GTK_WIDGET( radio_label ));
			break;*/
	}

	desc_label = NULL;
	switch( mode ){

		case EXPORT_FORMAT_DISPLAY_ASSISTANT:
			hbox = GTK_HBOX( gtk_hbox_new( TRUE, 0 ));
			gtk_box_pack_start( GTK_BOX( vbox ), GTK_WIDGET( hbox ), FALSE, TRUE, 0 );

			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-size", &size, NULL );
			gtk_widget_style_get( GTK_WIDGET( button ), "indicator-spacing", &spacing, NULL );
			size += 2*spacing;

			desc_label = GTK_LABEL( gtk_label_new( description ));
			g_object_set( G_OBJECT( desc_label ), "xpad", size, NULL );
			g_object_set( G_OBJECT( desc_label ), "xalign", 0, NULL );
			gtk_box_pack_start( GTK_BOX( hbox ), GTK_WIDGET( desc_label ), TRUE, TRUE, 4 );
			break;
	}

	vbox_data = g_new0( VBoxData, 1 );
	vbox_data->container_vbox = container_vbox;
	vbox_data->button = button;
	vbox_data->format = g_object_ref(( gpointer ) format );

	g_object_set_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_VBOX_DATA, vbox_data );
	g_object_weak_ref( G_OBJECT( vbox ), ( GWeakNotify ) format_weak_notify, ( gpointer ) vbox_data );

	g_free( markup );
	g_free( label );
	g_free( description );
}

static void
format_weak_notify( VBoxData *vbox_data, GObject *vbox )
{
	static const gchar *thisfn = "nact_export_format_weak_notify";

	g_debug( "%s: vbox_data=%p, vbox=%p", thisfn, ( void * ) vbox_data, ( void * ) vbox );

	g_signal_handler_disconnect( vbox_data->button, vbox_data->handler );

	g_object_unref( vbox_data->format );

	g_free( vbox_data );
}

/**
 * nact_export_format_select:
 * @container_vbox: the #GtkVBox in which the display has been drawn.
 * @editable: whether the whole radio button group is activatable.
 * @format: the #GQuark which must be used as a default value.
 *
 * Select the default value.
 *
 * This is to be ran from runtime initialization dialog.
 *
 * Data for each format has been set on the new embedding vbox, previously
 * created in nact_export_format_init_display(). We are iterating on these
 * vbox to setup the initially active radio button.
 */
void
nact_export_format_select( const GtkWidget *container_vbox, gboolean editable, GQuark format )
{
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE, GUINT_TO_POINTER( editable ));
	g_object_set_data( G_OBJECT( container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( format ));

	gtk_container_foreach( GTK_CONTAINER( container_vbox ), ( GtkCallback ) select_default_iter, GUINT_TO_POINTER( format ));
}

static void
select_default_iter( GtkWidget *vbox, void *quark_ptr )
{
	VBoxData *vbox_data;
	GQuark format_quark;
	GtkRadioButton *button;
	gboolean editable, sensitive;

	vbox_data = ( VBoxData * )
			g_object_get_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_VBOX_DATA );

	editable = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE ));

	sensitive = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_SENSITIVE ));

	vbox_data->handler = g_signal_connect( G_OBJECT( vbox_data->button ), "toggled", G_CALLBACK( export_format_on_toggled ), vbox_data );

	button = NULL;
	format_quark = ( GQuark ) GPOINTER_TO_UINT( quark_ptr );

	if( na_export_format_get_quark( vbox_data->format ) == format_quark ){
		button = vbox_data->button;
	}

	if( button ){
		nact_gtk_utils_radio_set_initial_state( button,
				G_CALLBACK( export_format_on_toggled ), vbox_data, editable, sensitive );
	}
}

static void
export_format_on_toggled( GtkToggleButton *toggle_button, VBoxData *vbox_data )
{
	gboolean editable, active;
	GQuark format;

	editable = ( gboolean ) GPOINTER_TO_UINT(
			g_object_get_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_EDITABLE ));

	if( editable ){
		active = gtk_toggle_button_get_active( toggle_button );
		if( active ){
			format = na_export_format_get_quark( vbox_data->format );
			g_object_set_data( G_OBJECT( vbox_data->container_vbox ), EXPORT_FORMAT_PROP_CONTAINER_FORMAT, GUINT_TO_POINTER( format ));
		}
	} else {
		nact_gtk_utils_radio_reset_initial_state( GTK_RADIO_BUTTON( toggle_button ), NULL );
	}
}

/**
 * nact_export_format_get_selected:
 * @container_vbox: the #GtkVBox in which the display has been drawn.
 *
 * Returns: the currently selected value, as a #NAExportFormat object.
 *
 * The returned #NAExportFormat is owned by #NactExportFormat, and
 * should not be released by the caller.
 */
NAExportFormat *
nact_export_format_get_selected( const GtkWidget *container_vbox )
{
	NAExportFormat *format;

	format = NULL;
	gtk_container_foreach( GTK_CONTAINER( container_vbox ), ( GtkCallback ) get_selected_iter, &format );
	g_debug( "nact_export_format_get_selected: format=%p", ( void * ) format );

	return( format );
}

static void
get_selected_iter( GtkWidget *vbox, NAExportFormat **format )
{
	VBoxData *vbox_data;

	if( !( *format  )){
		vbox_data = ( VBoxData * ) g_object_get_data( G_OBJECT( vbox ), EXPORT_FORMAT_PROP_VBOX_DATA );
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( vbox_data->button ))){
			*format = vbox_data->format;
		}
	}
}
