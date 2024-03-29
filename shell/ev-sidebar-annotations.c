/* ev-sidebar-annotations.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2010 Carlos Garcia Campos  <carlosgc@gnome.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <glib/gi18n.h>

#include "ev-document-annotations.h"
#include "ev-sidebar-page.h"
#include "ev-sidebar-annotations.h"
#include "ev-jobs.h"
#include "ev-job-scheduler.h"
#include "ev-stock-icons.h"

enum {
	PROP_0,
	PROP_WIDGET
};

enum {
	COLUMN_MARKUP,
	COLUMN_ICON,
	COLUMN_ANNOT_MAPPING,
	N_COLUMNS
};

enum {
	ANNOT_ACTIVATED,
	BEGIN_ANNOT_ADD,
	ANNOT_ADD_CANCELLED,
	N_SIGNALS
};

struct _EvSidebarAnnotationsPrivate {
	EvDocument  *document;

	CtkWidget *notebook;
	CtkWidget *tree_view;
	CtkWidget *annot_text_item;

	EvJob *job;
	guint selection_changed_id;
};

static void ev_sidebar_annotations_page_iface_init (EvSidebarPageInterface *iface);
static void ev_sidebar_annotations_load            (EvSidebarAnnotations   *sidebar_annots);

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_EXTENDED (EvSidebarAnnotations,
                        ev_sidebar_annotations,
                        CTK_TYPE_BOX,
                        0,
                        G_ADD_PRIVATE (EvSidebarAnnotations)
                        G_IMPLEMENT_INTERFACE (EV_TYPE_SIDEBAR_PAGE,
					       ev_sidebar_annotations_page_iface_init))

static void
ev_sidebar_annotations_dispose (GObject *object)
{
	EvSidebarAnnotations *sidebar_annots = EV_SIDEBAR_ANNOTATIONS (object);
	EvSidebarAnnotationsPrivate *priv = sidebar_annots->priv;

	if (priv->document) {
		g_object_unref (priv->document);
		priv->document = NULL;
	}

	G_OBJECT_CLASS (ev_sidebar_annotations_parent_class)->dispose (object);
}

static CtkTreeModel *
ev_sidebar_annotations_create_simple_model (const gchar *message)
{
	CtkTreeModel *retval;
	CtkTreeIter iter;
	gchar *markup;

	/* Creates a fake model to indicate that we're loading */
	retval = (CtkTreeModel *)ctk_list_store_new (N_COLUMNS,
						     G_TYPE_STRING,
						     GDK_TYPE_PIXBUF,
						     G_TYPE_POINTER);

	ctk_list_store_append (CTK_LIST_STORE (retval), &iter);
	markup = g_strdup_printf ("<span size=\"larger\" style=\"italic\">%s</span>",
				  message);
	ctk_list_store_set (CTK_LIST_STORE (retval), &iter,
			    COLUMN_MARKUP, markup,
			    -1);
	g_free (markup);

	return retval;
}

static void
ev_sidebar_annotations_text_annot_button_toggled (CtkWidget  *button,
                                                  EvSidebarAnnotations *sidebar_annots)
{
	EvAnnotationType annot_type;

	if (!ctk_toggle_button_get_active (CTK_TOGGLE_BUTTON (button))) {
		g_signal_emit (sidebar_annots, signals[ANNOT_ADD_CANCELLED], 0, NULL);
		return;
	}

	if (button == sidebar_annots->priv->annot_text_item)
		annot_type = EV_ANNOTATION_TYPE_TEXT;
	else
		annot_type = EV_ANNOTATION_TYPE_UNKNOWN;

	g_signal_emit (sidebar_annots, signals[BEGIN_ANNOT_ADD], 0, annot_type);
}

static void
ev_sidebar_annotations_init (EvSidebarAnnotations *ev_annots)
{
	CtkWidget *swindow;
	CtkTreeModel *loading_model;
	CtkCellRenderer *renderer;
	CtkTreeViewColumn *column;
	CtkTreeSelection *selection;
	CtkWidget *hbox;
	CtkWidget *buttonarea;
	CtkWidget *image;
	CtkWidget *label;

	ev_annots->priv = ev_sidebar_annotations_get_instance_private (ev_annots);

	ctk_orientable_set_orientation (CTK_ORIENTABLE (ev_annots), CTK_ORIENTATION_VERTICAL);

	swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
	                                CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow), CTK_SHADOW_IN);
	ctk_box_pack_start (CTK_BOX (ev_annots), swindow, TRUE, TRUE, 0);
	ctk_widget_show (swindow);

	/* Create tree view */
	loading_model = ev_sidebar_annotations_create_simple_model (_("Loading…"));
	ev_annots->priv->tree_view = ctk_tree_view_new_with_model (loading_model);
	g_object_unref (loading_model);

	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (ev_annots->priv->tree_view), FALSE);
	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (ev_annots->priv->tree_view));
	ctk_tree_selection_set_mode (selection, CTK_SELECTION_NONE);

	column = ctk_tree_view_column_new ();

	renderer = ctk_cell_renderer_pixbuf_new ();
	ctk_tree_view_column_pack_start (column, renderer, FALSE);
	ctk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COLUMN_ICON,
					     NULL);

	renderer = ctk_cell_renderer_text_new ();
	ctk_tree_view_column_pack_start (column, renderer, TRUE);
	ctk_tree_view_column_set_attributes (column, renderer,
					     "markup", COLUMN_MARKUP,
					     NULL);
	ctk_tree_view_append_column (CTK_TREE_VIEW (ev_annots->priv->tree_view), column);

	ctk_container_add (CTK_CONTAINER (swindow), ev_annots->priv->tree_view);
	ctk_widget_show (ev_annots->priv->tree_view);

	hbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);
	ctk_widget_set_halign (hbox, CTK_ALIGN_START);
	ctk_widget_set_margin_top (hbox, 6);
	ctk_widget_show (hbox);

	ev_annots->priv->annot_text_item = ctk_toggle_button_new ();
	ctk_widget_set_halign (ev_annots->priv->annot_text_item, CTK_ALIGN_START);
	buttonarea = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_set_spacing (CTK_BOX (buttonarea), 2);
	ctk_widget_set_margin_start (buttonarea, 4);
	ctk_widget_set_margin_end (buttonarea, 4);
	ctk_container_add (CTK_CONTAINER (ev_annots->priv->annot_text_item), buttonarea);
	ctk_widget_show (buttonarea);

	image = ctk_image_new_from_icon_name ("list-add", CTK_ICON_SIZE_BUTTON);
	ctk_widget_show (image);
	ctk_box_pack_start (CTK_BOX (buttonarea), image, TRUE, TRUE, 0);

	label = ctk_label_new (_("Add"));
	ctk_widget_show (label);
	ctk_box_pack_start (CTK_BOX (buttonarea), label, TRUE, TRUE, 0);

	ctk_box_pack_start (CTK_BOX (hbox), ev_annots->priv->annot_text_item, TRUE, TRUE, 6);
	ctk_widget_set_tooltip_text (CTK_WIDGET (ev_annots->priv->annot_text_item),
	                             _("Add text annotation"));
	g_signal_connect (ev_annots->priv->annot_text_item, "toggled",
	                  G_CALLBACK (ev_sidebar_annotations_text_annot_button_toggled),
	                  ev_annots);
	ctk_widget_show (CTK_WIDGET (ev_annots->priv->annot_text_item));

	ctk_box_pack_end (CTK_BOX (ev_annots), hbox, FALSE, TRUE, 0);
	ctk_widget_show (CTK_WIDGET (ev_annots));
}

static void
ev_sidebar_annotations_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	EvSidebarAnnotations *ev_sidebar_annots;

	ev_sidebar_annots = EV_SIDEBAR_ANNOTATIONS (object);

	switch (prop_id) {
	        case PROP_WIDGET:
			g_value_set_object (value, ev_sidebar_annots->priv->notebook);
			break;
	        default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
ev_sidebar_annotations_class_init (EvSidebarAnnotationsClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_object_class->get_property = ev_sidebar_annotations_get_property;
	g_object_class->dispose = ev_sidebar_annotations_dispose;

	g_object_class_override_property (g_object_class, PROP_WIDGET, "main-widget");

	signals[ANNOT_ACTIVATED] =
		g_signal_new ("annot-activated",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvSidebarAnnotationsClass, annot_activated),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1,
			      G_TYPE_POINTER);
	signals[BEGIN_ANNOT_ADD] =
		g_signal_new ("begin-annot-add",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvSidebarAnnotationsClass, begin_annot_add),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE, 1,
			      EV_TYPE_ANNOTATION_TYPE);
	signals[ANNOT_ADD_CANCELLED] =
		g_signal_new ("annot-add-cancelled",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvSidebarAnnotationsClass, annot_add_cancelled),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0,
			      G_TYPE_NONE);
}

CtkWidget *
ev_sidebar_annotations_new (void)
{
	return CTK_WIDGET (g_object_new (EV_TYPE_SIDEBAR_ANNOTATIONS, NULL));
}

void
ev_sidebar_annotations_annot_added (EvSidebarAnnotations *sidebar_annots,
				    EvAnnotation         *annot)
{
	CtkWidget *toggle_button;

	if (EV_IS_ANNOTATION_TEXT (annot)) {
		toggle_button = sidebar_annots->priv->annot_text_item;
		g_signal_handlers_block_by_func (toggle_button,
						 ev_sidebar_annotations_text_annot_button_toggled,
						 sidebar_annots);
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (toggle_button), FALSE);
		g_signal_handlers_unblock_by_func (toggle_button,
						   ev_sidebar_annotations_text_annot_button_toggled,
						   sidebar_annots);
	}

	ev_sidebar_annotations_load (sidebar_annots);
}

void
ev_sidebar_annotations_annot_removed (EvSidebarAnnotations *sidebar_annots)
{
	ev_sidebar_annotations_load (sidebar_annots);
}

static void
selection_changed_cb (CtkTreeSelection     *selection,
		      EvSidebarAnnotations *sidebar_annots)
{
	CtkTreeModel *model;
	CtkTreeIter   iter;

	if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
		EvMapping *mapping = NULL;

		ctk_tree_model_get (model, &iter,
				    COLUMN_ANNOT_MAPPING, &mapping,
				    -1);
		if (mapping)
			g_signal_emit (sidebar_annots, signals[ANNOT_ACTIVATED], 0, mapping);
	}
}

static void
job_finished_callback (EvJobAnnots          *job,
		       EvSidebarAnnotations *sidebar_annots)
{
	EvSidebarAnnotationsPrivate *priv;
	CtkTreeStore *model;
	CtkTreeSelection *selection;
	GList *l;
	GdkPixbuf *text_icon = NULL;
	GdkPixbuf *attachment_icon = NULL;

	priv = sidebar_annots->priv;

	if (!job->annots) {
		CtkTreeModel *list;

		list = ev_sidebar_annotations_create_simple_model (_("Document contains no annotations"));
		ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), list);
		g_object_unref (list);

		g_object_unref (job);
		priv->job = NULL;

		return;
	}

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
	ctk_tree_selection_set_mode (selection, CTK_SELECTION_SINGLE);
	if (priv->selection_changed_id == 0) {
		priv->selection_changed_id =
			g_signal_connect (selection, "changed",
					  G_CALLBACK (selection_changed_cb),
					  sidebar_annots);
	}

	model = ctk_tree_store_new (N_COLUMNS,
				    G_TYPE_STRING,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_POINTER);

	for (l = job->annots; l; l = g_list_next (l)) {
		EvMappingList *mapping_list;
		GList         *ll;
		gchar         *page_label;
		CtkTreeIter    iter;
		gboolean       found = FALSE;

		mapping_list = (EvMappingList *)l->data;
		page_label = g_strdup_printf (_("Page %d"),
					      ev_mapping_list_get_page (mapping_list) + 1);
		ctk_tree_store_append (model, &iter, NULL);
		ctk_tree_store_set (model, &iter,
				    COLUMN_MARKUP, page_label,
				    -1);
		g_free (page_label);

		for (ll = ev_mapping_list_get_list (mapping_list); ll; ll = g_list_next (ll)) {
			EvAnnotation *annot;
			const gchar  *label;
			const gchar  *modified;
			gchar        *markup;
			CtkTreeIter   child_iter;
			GdkPixbuf    *pixbuf = NULL;
			CtkIconTheme *icon_theme;

			annot = ((EvMapping *)(ll->data))->data;
			if (!EV_IS_ANNOTATION_MARKUP (annot))
				continue;

			label = ev_annotation_markup_get_label (EV_ANNOTATION_MARKUP (annot));
			modified = ev_annotation_get_modified (annot);
			if (modified) {
				markup = g_strdup_printf ("<span weight=\"bold\">%s</span>\n%s",
							  label, modified);
			} else {
				markup = g_strdup_printf ("<span weight=\"bold\">%s</span>", label);
			}

			icon_theme = ctk_icon_theme_get_default();

			if (EV_IS_ANNOTATION_TEXT (annot)) {
				if (!text_icon) {
					text_icon = ctk_icon_theme_load_icon (icon_theme,
					                                      "starred",
					                                      CTK_ICON_SIZE_BUTTON,
					                                      CTK_ICON_LOOKUP_FORCE_REGULAR,
					                                      NULL);
				}
				pixbuf = text_icon;
			} else if (EV_IS_ANNOTATION_ATTACHMENT (annot)) {
				if (!attachment_icon) {
					attachment_icon = ctk_icon_theme_load_icon (icon_theme,
					                                            "mail-attachment",
					                                            CTK_ICON_SIZE_BUTTON,
					                                            CTK_ICON_LOOKUP_FORCE_REGULAR,
					                                            NULL);
				}
				pixbuf = attachment_icon;
			}

			ctk_tree_store_append (model, &child_iter, &iter);
			ctk_tree_store_set (model, &child_iter,
					    COLUMN_MARKUP, markup,
					    COLUMN_ICON, pixbuf,
					    COLUMN_ANNOT_MAPPING, ll->data,
					    -1);
			g_free (markup);
			found = TRUE;
		}

		if (!found)
			ctk_tree_store_remove (model, &iter);
	}

	ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view),
				 CTK_TREE_MODEL (model));
	g_object_unref (model);

	if (text_icon)
		g_object_unref (text_icon);
	if (attachment_icon)
		g_object_unref (attachment_icon);

	g_object_unref (job);
	priv->job = NULL;
}

static void
ev_sidebar_annotations_load (EvSidebarAnnotations *sidebar_annots)
{
	EvSidebarAnnotationsPrivate *priv = sidebar_annots->priv;

	if (priv->job) {
		g_signal_handlers_disconnect_by_func (priv->job,
						      job_finished_callback,
						      sidebar_annots);
		g_object_unref (priv->job);
	}

	priv->job = ev_job_annots_new (priv->document);
	g_signal_connect (priv->job, "finished",
			  G_CALLBACK (job_finished_callback),
			  sidebar_annots);
	/* The priority doesn't matter for this job */
	ev_job_scheduler_push_job (priv->job, EV_JOB_PRIORITY_NONE);
}

static void
ev_sidebar_annotations_document_changed_cb (EvDocumentModel      *model,
					    GParamSpec           *pspec,
					    EvSidebarAnnotations *sidebar_annots)
{
	EvDocument *document = ev_document_model_get_document (model);
	EvSidebarAnnotationsPrivate *priv = sidebar_annots->priv;
	gboolean enable_add;

	if (!EV_IS_DOCUMENT_ANNOTATIONS (document))
		return;

	if (priv->document)
		g_object_unref (priv->document);
	priv->document = g_object_ref (document);

	enable_add = ev_document_annotations_can_add_annotation (EV_DOCUMENT_ANNOTATIONS (document));
	ctk_widget_set_sensitive (priv->annot_text_item, enable_add);

	ev_sidebar_annotations_load (sidebar_annots);
}

/* EvSidebarPageIface */
static void
ev_sidebar_annotations_set_model (EvSidebarPage   *sidebar_page,
				  EvDocumentModel *model)
{
	g_signal_connect (model, "notify::document",
			  G_CALLBACK (ev_sidebar_annotations_document_changed_cb),
			  sidebar_page);
}

static gboolean
ev_sidebar_annotations_support_document (EvSidebarPage *sidebar_page,
					 EvDocument    *document)
{
	return (EV_IS_DOCUMENT_ANNOTATIONS (document));
}

static const gchar *
ev_sidebar_annotations_get_label (EvSidebarPage *sidebar_page)
{
	return _("Annotations");
}

static void
ev_sidebar_annotations_page_iface_init (EvSidebarPageInterface *iface)
{
	iface->support_document = ev_sidebar_annotations_support_document;
	iface->set_model = ev_sidebar_annotations_set_model;
	iface->get_label = ev_sidebar_annotations_get_label;
}
