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
#include "ev-job-scheduler.h"
#include "ev-jobs.h"
#include "ev-properties-fonts.h"

struct _EvPropertiesFonts {
	CtkBox base_instance;

	CtkWidget *fonts_treeview;
	CtkWidget *fonts_progress_label;
	EvJob     *fonts_job;

	EvDocument *document;
};

struct _EvPropertiesFontsClass {
	CtkBoxClass base_class;
};

static void
job_fonts_finished_cb (EvJob *job, EvPropertiesFonts *properties);

G_DEFINE_TYPE (EvPropertiesFonts, ev_properties_fonts, CTK_TYPE_BOX)

static void
ev_properties_fonts_dispose (GObject *object)
{
	EvPropertiesFonts *properties = EV_PROPERTIES_FONTS (object);

	if (properties->fonts_job) {
		g_signal_handlers_disconnect_by_func (properties->fonts_job,
						      job_fonts_finished_cb,
						      properties);
		ev_job_cancel (properties->fonts_job);

		g_object_unref (properties->fonts_job);
		properties->fonts_job = NULL;
	}

	G_OBJECT_CLASS (ev_properties_fonts_parent_class)->dispose (object);
}

static void
ev_properties_fonts_class_init (EvPropertiesFontsClass *properties_class)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (properties_class);

	g_object_class->dispose = ev_properties_fonts_dispose;
}

static void
font_cell_data_func (CtkTreeViewColumn *col, CtkCellRenderer *renderer,
		     CtkTreeModel *model, CtkTreeIter *iter, gpointer user_data)
{
	char *name;
	char *details;
	char *markup;

	ctk_tree_model_get (model, iter,
			    EV_DOCUMENT_FONTS_COLUMN_NAME, &name,
			    EV_DOCUMENT_FONTS_COLUMN_DETAILS, &details,
			    -1);

	if (details) {
		markup = g_strdup_printf ("<b><big>%s</big></b>\n<small>%s</small>",
					  name, details);
	} else {
		markup = g_strdup_printf ("<b><big>%s</big></b>", name);
	}

	g_object_set (renderer, "markup", markup, NULL);

	g_free (markup);
	g_free (details);
	g_free (name);
}

static void
ev_properties_fonts_init (EvPropertiesFonts *properties)
{
	CtkWidget         *swindow;
	CtkCellRenderer   *renderer;
	CtkTreeViewColumn *column;

	ctk_orientable_set_orientation (CTK_ORIENTABLE (properties), CTK_ORIENTATION_VERTICAL);
	ctk_container_set_border_width (CTK_CONTAINER (properties), 12);
	ctk_box_set_spacing (CTK_BOX (properties), 6);

	swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow),
					     CTK_SHADOW_IN);

	properties->fonts_treeview = ctk_tree_view_new ();
	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (properties->fonts_treeview),
					   FALSE);
	column = ctk_tree_view_column_new ();
	ctk_tree_view_column_set_expand (CTK_TREE_VIEW_COLUMN (column), TRUE);
	ctk_tree_view_append_column (CTK_TREE_VIEW (properties->fonts_treeview),
				     column);

	renderer = CTK_CELL_RENDERER (g_object_new (CTK_TYPE_CELL_RENDERER_TEXT,
						    "ypad", 6, NULL));
	ctk_tree_view_column_pack_start (CTK_TREE_VIEW_COLUMN (column),
					 renderer, FALSE);
	ctk_tree_view_column_set_title (CTK_TREE_VIEW_COLUMN (column),
					_("Font"));
	ctk_tree_view_column_set_cell_data_func (column, renderer,
						 font_cell_data_func,
						 NULL, NULL);

	ctk_container_add (CTK_CONTAINER (swindow), properties->fonts_treeview);
	ctk_widget_show (properties->fonts_treeview);

	ctk_box_pack_start (CTK_BOX (properties), swindow,
			    TRUE, TRUE, 0);
	ctk_widget_show (swindow);

	properties->fonts_progress_label = ctk_label_new (NULL);
	g_object_set (G_OBJECT (properties->fonts_progress_label),
		      "xalign", 0.0,
		      NULL);
	ctk_box_pack_start (CTK_BOX (properties),
			    properties->fonts_progress_label,
			    FALSE, FALSE, 0);
	ctk_widget_show (properties->fonts_progress_label);
}

static void
update_progress_label (CtkWidget *label, double progress)
{
	if (progress > 0) {
		char *progress_text;
		progress_text = g_strdup_printf (_("Gathering font information… %3d%%"),
						 (int) (progress * 100));
		ctk_label_set_text (CTK_LABEL (label), progress_text);
		g_free (progress_text);
		ctk_widget_show (label);
	} else {
		ctk_widget_hide (label);
	}
}

static void
job_fonts_finished_cb (EvJob *job, EvPropertiesFonts *properties)
{
	g_signal_handlers_disconnect_by_func (job, job_fonts_finished_cb, properties);
	g_object_unref (properties->fonts_job);
	properties->fonts_job = NULL;
}

static void
job_fonts_updated_cb (EvJobFonts *job, gdouble progress, EvPropertiesFonts *properties)
{
	CtkTreeModel *model;
	EvDocumentFonts *document_fonts = EV_DOCUMENT_FONTS (properties->document);

	update_progress_label (properties->fonts_progress_label, progress);

	model = ctk_tree_view_get_model (CTK_TREE_VIEW (properties->fonts_treeview));
	/* Documen lock is already held by the jop */
	ev_document_fonts_fill_model (document_fonts, model);
}

void
ev_properties_fonts_set_document (EvPropertiesFonts *properties,
				  EvDocument        *document)
{
	CtkTreeView *tree_view = CTK_TREE_VIEW (properties->fonts_treeview);
	CtkListStore *list_store;

	properties->document = document;

	list_store = ctk_list_store_new (EV_DOCUMENT_FONTS_COLUMN_NUM_COLUMNS,
					 G_TYPE_STRING, G_TYPE_STRING);
	ctk_tree_view_set_model (tree_view, CTK_TREE_MODEL (list_store));

	properties->fonts_job = ev_job_fonts_new (properties->document);
	g_signal_connect (properties->fonts_job, "updated",
			  G_CALLBACK (job_fonts_updated_cb),
			  properties);
	g_signal_connect (properties->fonts_job, "finished",
			  G_CALLBACK (job_fonts_finished_cb),
			  properties);
	ev_job_scheduler_push_job (properties->fonts_job, EV_JOB_PRIORITY_NONE);
}

CtkWidget *
ev_properties_fonts_new ()
{
	EvPropertiesFonts *properties;

	properties = g_object_new (EV_TYPE_PROPERTIES_FONTS, NULL);

	return CTK_WIDGET (properties);
}
