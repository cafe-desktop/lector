/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/* this file is part of lector, a cafe document viewer
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "ev-document-fonts.h"
#include "ev-properties-dialog.h"
#include "ev-properties-fonts.h"
#include "ev-properties-view.h"
#include "ev-properties-license.h"

struct _EvPropertiesDialog {
	CtkDialog base_instance;

	EvDocument *document;
	CtkWidget *notebook;
	CtkWidget *general_page;
	CtkWidget *fonts_page;
	CtkWidget *license_page;
};

struct _EvPropertiesDialogClass {
	CtkDialogClass base_class;
};

G_DEFINE_TYPE (EvPropertiesDialog, ev_properties_dialog, CTK_TYPE_DIALOG)

static void
ev_properties_dialog_class_init (EvPropertiesDialogClass *properties_class)
{
}

static void
ev_properties_dialog_init (EvPropertiesDialog *properties)
{
	CtkBox *content_area;

	content_area = CTK_BOX (ctk_dialog_get_content_area (CTK_DIALOG (properties)));

	ctk_window_set_title (CTK_WINDOW (properties), _("Properties"));
	ctk_window_set_destroy_with_parent (CTK_WINDOW (properties), TRUE);
	ctk_container_set_border_width (CTK_CONTAINER (properties), 5);
	ctk_box_set_spacing (content_area, 2);

	ctk_dialog_add_button (CTK_DIALOG (properties), "ctk-close",
			       CTK_RESPONSE_CANCEL);
	ctk_dialog_set_default_response (CTK_DIALOG (properties),
			                 CTK_RESPONSE_CANCEL);

	properties->notebook = ctk_notebook_new ();
	ctk_container_set_border_width (CTK_CONTAINER (properties->notebook), 5);
	ctk_box_pack_start (content_area, properties->notebook, TRUE, TRUE, 0);
	ctk_widget_show (properties->notebook);

	g_signal_connect (properties, "response",
			  G_CALLBACK (ctk_widget_destroy), NULL);
}

void
ev_properties_dialog_set_document (EvPropertiesDialog *properties,
				   const gchar        *uri,
			           EvDocument         *document)
{
	CtkWidget *label;
	const EvDocumentInfo *info;

	properties->document = document;

	info = ev_document_get_info (document);

	if (properties->general_page == NULL) {
		label = ctk_label_new (_("General"));
		properties->general_page = ev_properties_view_new (uri);
		ctk_notebook_append_page (CTK_NOTEBOOK (properties->notebook),
					  properties->general_page, label);
		ctk_widget_show (properties->general_page);
	}
	ev_properties_view_set_info (EV_PROPERTIES_VIEW (properties->general_page), info);

	if (EV_IS_DOCUMENT_FONTS (document)) {
		if (properties->fonts_page == NULL) {
			label = ctk_label_new (_("Fonts"));
			properties->fonts_page = ev_properties_fonts_new ();
			ctk_notebook_append_page (CTK_NOTEBOOK (properties->notebook),
						  properties->fonts_page, label);
			ctk_widget_show (properties->fonts_page);
		}

		ev_properties_fonts_set_document
			(EV_PROPERTIES_FONTS (properties->fonts_page), document);
	}

	if (info->fields_mask & EV_DOCUMENT_INFO_LICENSE && info->license) {
		if (properties->license_page == NULL) {
			label = ctk_label_new (_("Document License"));
			properties->license_page = ev_properties_license_new ();
			ctk_notebook_append_page (CTK_NOTEBOOK (properties->notebook),
						  properties->license_page, label);
			ctk_widget_show (properties->license_page);
		}

		ev_properties_license_set_license
			(EV_PROPERTIES_LICENSE (properties->license_page), info->license);
	}
}

CtkWidget *
ev_properties_dialog_new ()
{
	EvPropertiesDialog *properties;

	properties = g_object_new (EV_TYPE_PROPERTIES_DIALOG, NULL);

	return CTK_WIDGET (properties);
}
