/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of lector, a cafe document viewer
 *
 *  Copyright (C) 2009 Juanjo Marín <juanj.marin@juntadeandalucia.es>
 *  Copyright (C) 2005 Red Hat, Inc
 *
 * Lector is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Lector is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "ev-properties-license.h"

struct _EvPropertiesLicense {
	CtkBox base_instance;
};

struct _EvPropertiesLicenseClass {
	CtkBoxClass base_class;
};

G_DEFINE_TYPE (EvPropertiesLicense, ev_properties_license, CTK_TYPE_BOX)

static void
ev_properties_license_class_init (EvPropertiesLicenseClass *properties_license_class)
{
}

static CtkWidget *
get_license_text_widget (EvDocumentLicense *license)
{
	CtkWidget *swindow, *textview;
	CtkTextBuffer *buffer;

	textview = ctk_text_view_new ();
	ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (textview), CTK_WRAP_WORD);
	ctk_text_view_set_left_margin (CTK_TEXT_VIEW (textview), 8);
	ctk_text_view_set_right_margin (CTK_TEXT_VIEW (textview), 8);

	buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (textview));
	ctk_text_buffer_set_text (buffer, ev_document_license_get_text (license), -1);

	swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_widget_set_hexpand (swindow, TRUE);
	ctk_widget_set_margin_start (swindow, 12);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow),
					     CTK_SHADOW_IN);
	ctk_container_add (CTK_CONTAINER (swindow), textview);
	ctk_widget_show (textview);

	return swindow;
}

static CtkWidget *
get_license_uri_widget (const gchar *uri)
{
	CtkWidget *label;
	gchar     *checked_uri;
	gchar     *markup;

	label = ctk_label_new (NULL);
	ctk_widget_set_margin_start (label, 12);
	g_object_set (G_OBJECT (label),
		      "xalign", 0.0,
		      "width_chars", 25,
		      "selectable", TRUE,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);

	checked_uri = g_uri_parse_scheme (uri);
	if (checked_uri) {
		markup = g_markup_printf_escaped ("<a href=\"%s\">%s</a>", uri, uri);
		ctk_label_set_markup (CTK_LABEL (label), markup);
		g_free (markup);
		g_free (checked_uri);
	} else {
		ctk_label_set_text (CTK_LABEL (label), uri);
	}

	return label;
}

static void
ev_properties_license_add_section (EvPropertiesLicense *properties,
				   const gchar         *title_text,
				   CtkWidget           *contents)
{
	CtkWidget *title;
	gchar     *markup;

	title = ctk_label_new (NULL);
	ctk_label_set_xalign (CTK_LABEL (title), 0.0);
	ctk_label_set_use_markup (CTK_LABEL (title), TRUE);
	markup = g_strdup_printf ("<b>%s</b>", title_text);
	ctk_label_set_markup (CTK_LABEL (title), markup);
	g_free (markup);
	ctk_box_pack_start (CTK_BOX (properties), title, FALSE, FALSE, 0);
	ctk_widget_show (title);

	ctk_box_pack_start (CTK_BOX (properties), contents, FALSE, TRUE, 0);
	ctk_widget_show (contents);
}

void
ev_properties_license_set_license (EvPropertiesLicense *properties,
				   EvDocumentLicense   *license)
{
	const gchar *text = ev_document_license_get_text (license);
	const gchar *uri = ev_document_license_get_uri (license);
	const gchar *web_statement = ev_document_license_get_web_statement (license);

	if (text) {
		ev_properties_license_add_section (properties,
						   _("Usage terms"),
						   get_license_text_widget (license));
	}

	if (uri) {
		ev_properties_license_add_section (properties,
						   _("Text License"),
						   get_license_uri_widget (uri));
	}

	if (web_statement) {
		ev_properties_license_add_section (properties,
						   _("Further Information"),
						   get_license_uri_widget (web_statement));
	}
}

static void
ev_properties_license_init (EvPropertiesLicense *properties)
{
	ctk_orientable_set_orientation (CTK_ORIENTABLE (properties), CTK_ORIENTATION_VERTICAL);
	ctk_box_set_spacing (CTK_BOX (properties), 12);
	ctk_container_set_border_width (CTK_CONTAINER (properties), 12);
}

CtkWidget *
ev_properties_license_new (void)
{
	EvPropertiesLicense *properties_license;

	properties_license = g_object_new (EV_TYPE_PROPERTIES_LICENSE, NULL);

	return CTK_WIDGET (properties_license);
}
