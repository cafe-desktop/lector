/* ev-sidebar-layers.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos  <carlosgc@gnome.org>
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

#include <glib/gi18n.h>

#include "ev-document-layers.h"
#include "ev-sidebar-page.h"
#include "ev-jobs.h"
#include "ev-job-scheduler.h"
#include "ev-stock-icons.h"
#include "ev-sidebar-layers.h"

struct _EvSidebarLayersPrivate {
	CtkTreeView  *tree_view;

	EvDocument   *document;
	EvJob        *job;
};

enum {
	PROP_0,
	PROP_WIDGET
};

enum {
	LAYERS_VISIBILITY_CHANGED,
	N_SIGNALS
};

static void ev_sidebar_layers_page_iface_init (EvSidebarPageInterface *iface);
static void job_finished_callback             (EvJobLayers            *job,
					       EvSidebarLayers        *sidebar_layers);

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_EXTENDED (EvSidebarLayers,
                        ev_sidebar_layers,
                        CTK_TYPE_BOX,
                        0,
                        G_ADD_PRIVATE (EvSidebarLayers)
                        G_IMPLEMENT_INTERFACE (EV_TYPE_SIDEBAR_PAGE,
					       ev_sidebar_layers_page_iface_init))

static void
ev_sidebar_layers_dispose (GObject *object)
{
	EvSidebarLayers *sidebar = EV_SIDEBAR_LAYERS (object);

	if (sidebar->priv->job) {
		g_signal_handlers_disconnect_by_func (sidebar->priv->job,
						      job_finished_callback,
						      sidebar);
		ev_job_cancel (sidebar->priv->job);
		g_object_unref (sidebar->priv->job);
		sidebar->priv->job = NULL;
	}

	if (sidebar->priv->document) {
		g_object_unref (sidebar->priv->document);
		sidebar->priv->document = NULL;
	}

	G_OBJECT_CLASS (ev_sidebar_layers_parent_class)->dispose (object);
}

static void
ev_sidebar_layers_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	EvSidebarLayers *ev_sidebar_layers;

	ev_sidebar_layers = EV_SIDEBAR_LAYERS (object);

	switch (prop_id) {
	        case PROP_WIDGET:
			g_value_set_object (value, ev_sidebar_layers->priv->tree_view);
			break;
	        default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static CtkTreeModel *
ev_sidebar_layers_create_loading_model (void)
{
	CtkTreeModel *retval;
	CtkTreeIter   iter;
	gchar        *markup;

	/* Creates a fake model to indicate that we're loading */
	retval = (CtkTreeModel *)ctk_list_store_new (EV_DOCUMENT_LAYERS_N_COLUMNS,
						     G_TYPE_STRING,
						     G_TYPE_OBJECT,
						     G_TYPE_BOOLEAN,
						     G_TYPE_BOOLEAN,
						     G_TYPE_BOOLEAN,
						     G_TYPE_INT);

	ctk_list_store_append (CTK_LIST_STORE (retval), &iter);
	markup = g_strdup_printf ("<span size=\"larger\" style=\"italic\">%s</span>", _("Loading…"));
	ctk_list_store_set (CTK_LIST_STORE (retval), &iter,
			    EV_DOCUMENT_LAYERS_COLUMN_TITLE, markup,
			    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, FALSE,
			    EV_DOCUMENT_LAYERS_COLUMN_ENABLED, TRUE,
			    EV_DOCUMENT_LAYERS_COLUMN_SHOWTOGGLE, FALSE,
			    EV_DOCUMENT_LAYERS_COLUMN_RBGROUP, -1,
			    EV_DOCUMENT_LAYERS_COLUMN_LAYER, NULL,
			    -1);
	g_free (markup);

	return retval;
}

static gboolean
update_kids (CtkTreeModel *model,
	     CtkTreePath  *path,
	     CtkTreeIter  *iter,
	     CtkTreeIter  *parent)
{
	if (ctk_tree_store_is_ancestor (CTK_TREE_STORE (model), parent, iter)) {
		gboolean visible;

		ctk_tree_model_get (model, parent,
				    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, &visible,
				    -1);
		ctk_tree_store_set (CTK_TREE_STORE (model), iter,
				    EV_DOCUMENT_LAYERS_COLUMN_ENABLED, visible,
				    -1);
	}

	return FALSE;
}

static gboolean
clear_rb_group (CtkTreeModel *model,
		CtkTreePath  *path,
		CtkTreeIter  *iter,
		gint         *rb_group)
{
	gint group;

	ctk_tree_model_get (model, iter,
			    EV_DOCUMENT_LAYERS_COLUMN_RBGROUP, &group,
			    -1);

	if (group == *rb_group) {
		ctk_tree_store_set (CTK_TREE_STORE (model), iter,
				    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, FALSE,
				    -1);
	}

	return FALSE;
}

static void
ev_sidebar_layers_visibility_changed (CtkCellRendererToggle *cell,
				      gchar                 *path_str,
				      EvSidebarLayers       *ev_layers)
{
	CtkTreeModel *model;
	CtkTreePath  *path;
	CtkTreeIter   iter;
	gboolean      visible;
	EvLayer      *layer;

	model = ctk_tree_view_get_model (ev_layers->priv->tree_view);

	path = ctk_tree_path_new_from_string (path_str);
	ctk_tree_model_get_iter (model, &iter, path);
	ctk_tree_model_get (model, &iter,
			    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, &visible,
			    EV_DOCUMENT_LAYERS_COLUMN_LAYER, &layer,
			    -1);

	visible = !visible;
	if (visible) {
		gint rb_group;

		ev_document_layers_show_layer (EV_DOCUMENT_LAYERS (ev_layers->priv->document),
					       layer);

		rb_group = ev_layer_get_rb_group (layer);
		if (rb_group) {
			ctk_tree_model_foreach (model,
						(CtkTreeModelForeachFunc)clear_rb_group,
						&rb_group);
		}
	} else {
		ev_document_layers_hide_layer (EV_DOCUMENT_LAYERS (ev_layers->priv->document),
					       layer);
	}

	ctk_tree_store_set (CTK_TREE_STORE (model), &iter,
			    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, visible,
			    -1);

	if (ev_layer_is_parent (layer)) {
		ctk_tree_model_foreach (model,
					(CtkTreeModelForeachFunc)update_kids,
					&iter);
	}

	ctk_tree_path_free (path);

	g_signal_emit (ev_layers, signals[LAYERS_VISIBILITY_CHANGED], 0);
}

static CtkTreeView *
ev_sidebar_layers_create_tree_view (EvSidebarLayers *ev_layers)
{
	CtkTreeView       *tree_view;
	CtkTreeViewColumn *column;
	CtkCellRenderer   *renderer;

	tree_view = CTK_TREE_VIEW (ctk_tree_view_new ());
	ctk_tree_view_set_headers_visible (tree_view, FALSE);
	ctk_tree_selection_set_mode (ctk_tree_view_get_selection (tree_view),
				     CTK_SELECTION_NONE);


	column = ctk_tree_view_column_new ();

	renderer = ctk_cell_renderer_toggle_new ();
	ctk_tree_view_column_pack_start (column, renderer, FALSE);
	ctk_tree_view_column_set_attributes (column, renderer,
					     "active", EV_DOCUMENT_LAYERS_COLUMN_VISIBLE,
					     "activatable", EV_DOCUMENT_LAYERS_COLUMN_ENABLED,
					     "visible", EV_DOCUMENT_LAYERS_COLUMN_SHOWTOGGLE,
					     "sensitive", EV_DOCUMENT_LAYERS_COLUMN_ENABLED,
					     NULL);
	g_object_set (G_OBJECT (renderer),
		      "xpad", 4,
		      "ypad", 0,
		      NULL);
	g_signal_connect (renderer, "toggled",
			  G_CALLBACK (ev_sidebar_layers_visibility_changed),
			  (gpointer)ev_layers);

	renderer = ctk_cell_renderer_text_new ();
	ctk_tree_view_column_pack_start (column, renderer, TRUE);
	ctk_tree_view_column_set_attributes (column, renderer,
					     "markup", EV_DOCUMENT_LAYERS_COLUMN_TITLE,
					     "sensitive", EV_DOCUMENT_LAYERS_COLUMN_ENABLED,
					     NULL);
	g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	ctk_tree_view_append_column (tree_view, column);

	return tree_view;
}

static void
ev_sidebar_layers_init (EvSidebarLayers *ev_layers)
{
	CtkWidget    *swindow;
	CtkTreeModel *model;

	ev_layers->priv = ev_sidebar_layers_get_instance_private (ev_layers);

	ctk_orientable_set_orientation (CTK_ORIENTABLE (ev_layers), CTK_ORIENTATION_VERTICAL);
	swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_widget_set_vexpand (swindow, TRUE);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
					CTK_POLICY_NEVER,
					CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow),
					     CTK_SHADOW_IN);
	/* Data Model */
	model = ev_sidebar_layers_create_loading_model ();

	/* Layers list */
	ev_layers->priv->tree_view = ev_sidebar_layers_create_tree_view (ev_layers);
	ctk_tree_view_set_model (ev_layers->priv->tree_view, model);
	g_object_unref (model);

	ctk_container_add (CTK_CONTAINER (swindow),
			   CTK_WIDGET (ev_layers->priv->tree_view));

	ctk_container_add (CTK_CONTAINER (ev_layers), swindow);
	ctk_widget_show_all (CTK_WIDGET (ev_layers));
}

static void
ev_sidebar_layers_class_init (EvSidebarLayersClass *ev_layers_class)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (ev_layers_class);

	g_object_class->get_property = ev_sidebar_layers_get_property;
	g_object_class->dispose = ev_sidebar_layers_dispose;

	g_object_class_override_property (g_object_class, PROP_WIDGET, "main-widget");

	signals[LAYERS_VISIBILITY_CHANGED] =
		g_signal_new ("layers_visibility_changed",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvSidebarLayersClass, layers_visibility_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0, G_TYPE_NONE);
}

CtkWidget *
ev_sidebar_layers_new (void)
{
	return CTK_WIDGET (g_object_new (EV_TYPE_SIDEBAR_LAYERS, NULL));
}

static void
update_layers_state (CtkTreeModel     *model,
		     CtkTreeIter      *iter,
		     EvDocumentLayers *document_layers)
{
	EvLayer    *layer;
	gboolean    visible;
	CtkTreeIter child_iter;

	do {
		ctk_tree_model_get (model, iter,
				    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, &visible,
				    EV_DOCUMENT_LAYERS_COLUMN_LAYER, &layer,
				    -1);
		if (layer) {
			gboolean layer_visible;

			layer_visible = ev_document_layers_layer_is_visible (document_layers, layer);
			if (layer_visible != visible) {
				ctk_tree_store_set (CTK_TREE_STORE (model), iter,
						    EV_DOCUMENT_LAYERS_COLUMN_VISIBLE, layer_visible,
						    -1);
			}
		}

		if (ctk_tree_model_iter_children (model, &child_iter, iter))
			update_layers_state (model, &child_iter, document_layers);
	} while (ctk_tree_model_iter_next (model, iter));
}

void
ev_sidebar_layers_update_layers_state (EvSidebarLayers *sidebar_layers)
{
	CtkTreeModel     *model;
	CtkTreeIter       iter;
	EvDocumentLayers *document_layers;

	document_layers = EV_DOCUMENT_LAYERS (sidebar_layers->priv->document);
	model = ctk_tree_view_get_model (CTK_TREE_VIEW (sidebar_layers->priv->tree_view));
	if (ctk_tree_model_get_iter_first (model, &iter))
		update_layers_state (model, &iter, document_layers);
}

static void
job_finished_callback (EvJobLayers     *job,
		       EvSidebarLayers *sidebar_layers)
{
	EvSidebarLayersPrivate *priv;

	priv = sidebar_layers->priv;

	ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), job->model);

	g_object_unref (job);
	priv->job = NULL;
}

static void
ev_sidebar_layers_document_changed_cb (EvDocumentModel *model,
				       GParamSpec      *pspec,
				       EvSidebarLayers *sidebar_layers)
{
	EvDocument *document = ev_document_model_get_document (model);
	EvSidebarLayersPrivate *priv = sidebar_layers->priv;

	if (!EV_IS_DOCUMENT_LAYERS (document))
		return;

	if (priv->document) {
		ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), NULL);
		g_object_unref (priv->document);
	}

	priv->document = g_object_ref (document);

	if (priv->job) {
		g_signal_handlers_disconnect_by_func (priv->job,
						      job_finished_callback,
						      sidebar_layers);
		g_object_unref (priv->job);
	}

	priv->job = ev_job_layers_new (document);
	g_signal_connect (priv->job, "finished",
			  G_CALLBACK (job_finished_callback),
			  sidebar_layers);
	/* The priority doesn't matter for this job */
	ev_job_scheduler_push_job (priv->job, EV_JOB_PRIORITY_NONE);
}

static void
ev_sidebar_layers_set_model (EvSidebarPage   *sidebar_page,
			     EvDocumentModel *model)
{
	g_signal_connect (model, "notify::document",
			  G_CALLBACK (ev_sidebar_layers_document_changed_cb),
			  sidebar_page);
}

static gboolean
ev_sidebar_layers_support_document (EvSidebarPage *sidebar_page,
				    EvDocument    *document)
{
	return (EV_IS_DOCUMENT_LAYERS (document) &&
		ev_document_layers_has_layers (EV_DOCUMENT_LAYERS (document)));
}

static const gchar*
ev_sidebar_layers_get_label (EvSidebarPage *sidebar_page)
{
	return _("Layers");
}

static void
ev_sidebar_layers_page_iface_init (EvSidebarPageInterface *iface)
{
	iface->support_document = ev_sidebar_layers_support_document;
	iface->set_model = ev_sidebar_layers_set_model;
	iface->get_label = ev_sidebar_layers_get_label;
}

