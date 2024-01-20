/* this file is part of lector, a cafe document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc.
 *
 *  Author:
 *    Jonathan Blandford <jrb@alum.mit.edu>
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

#include <string.h>

#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "ev-document-links.h"
#include "ev-job-scheduler.h"
#include "ev-sidebar-links.h"
#include "ev-sidebar-page.h"
#include "ev-window.h"

struct _EvSidebarLinksPrivate {
	GtkWidget *tree_view;

	/* Keep these ids around for blocking */
	guint selection_id;
	guint page_changed_id;
	guint row_activated_id;

	EvJob *job;
	GtkTreeModel *model;
	EvDocument *document;
	EvDocumentModel *doc_model;
};

enum {
	PROP_0,
	PROP_MODEL,
	PROP_WIDGET,
};

enum {
	LINK_ACTIVATED,
	N_SIGNALS
};

static void update_page_callback 			(EvSidebarLinks    *sidebar_links,
							 gint               old_page,
							 gint               current_page);
static void row_activated_callback 			(GtkTreeView *treeview,
		                                         GtkTreePath *arg1,
	                                                 GtkTreeViewColumn *arg2,
		                                         gpointer user_data);
static void ev_sidebar_links_set_links_model            (EvSidebarLinks *links,
							 GtkTreeModel   *model);
static void job_finished_callback 			(EvJobLinks     *job,
				    		         EvSidebarLinks *sidebar_links);
static void ev_sidebar_links_set_current_page           (EvSidebarLinks *sidebar_links,
							 gint            current_page);
static void ev_sidebar_links_page_iface_init 		(EvSidebarPageInterface *iface);
static gboolean ev_sidebar_links_support_document	(EvSidebarPage  *sidebar_page,
						         EvDocument     *document);
static const gchar* ev_sidebar_links_get_label 		(EvSidebarPage *sidebar_page);

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_EXTENDED (EvSidebarLinks,
                        ev_sidebar_links,
                        CTK_TYPE_BOX,
                        0,
                        G_ADD_PRIVATE (EvSidebarLinks)
                        G_IMPLEMENT_INTERFACE (EV_TYPE_SIDEBAR_PAGE,
					       ev_sidebar_links_page_iface_init))

static void
ev_sidebar_links_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	EvSidebarLinks *ev_sidebar_links = EV_SIDEBAR_LINKS (object);

	switch (prop_id)
	{
	case PROP_MODEL:
		ev_sidebar_links_set_links_model (ev_sidebar_links, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ev_sidebar_links_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
	EvSidebarLinks *ev_sidebar_links;

	ev_sidebar_links = EV_SIDEBAR_LINKS (object);

	switch (prop_id)
	{
	case PROP_MODEL:
		g_value_set_object (value, ev_sidebar_links->priv->model);
		break;
	case PROP_WIDGET:
		g_value_set_object (value, ev_sidebar_links->priv->tree_view);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ev_sidebar_links_dispose (GObject *object)
{
	EvSidebarLinks *sidebar = EV_SIDEBAR_LINKS (object);

	if (sidebar->priv->job) {
		g_signal_handlers_disconnect_by_func (sidebar->priv->job,
						      job_finished_callback, sidebar);
		ev_job_cancel (sidebar->priv->job);
		g_object_unref (sidebar->priv->job);
		sidebar->priv->job = NULL;
	}

	if (sidebar->priv->model) {
		g_object_unref (sidebar->priv->model);
		sidebar->priv->model = NULL;
	}

	if (sidebar->priv->document) {
		g_object_unref (sidebar->priv->document);
		sidebar->priv->document = NULL;
		sidebar->priv->doc_model = NULL;
	}

	G_OBJECT_CLASS (ev_sidebar_links_parent_class)->dispose (object);
}

static void
ev_sidebar_links_map (GtkWidget *widget)
{
	EvSidebarLinks *links;

	links = EV_SIDEBAR_LINKS (widget);

	CTK_WIDGET_CLASS (ev_sidebar_links_parent_class)->map (widget);

	if (links->priv->model) {
		ev_sidebar_links_set_current_page (links,
						   ev_document_model_get_page (links->priv->doc_model));
	}
}

static void
ev_sidebar_links_class_init (EvSidebarLinksClass *ev_sidebar_links_class)
{
	GObjectClass   *g_object_class;
	GtkWidgetClass *widget_class;

	g_object_class = G_OBJECT_CLASS (ev_sidebar_links_class);
	widget_class = CTK_WIDGET_CLASS (ev_sidebar_links_class);

	g_object_class->set_property = ev_sidebar_links_set_property;
	g_object_class->get_property = ev_sidebar_links_get_property;
	g_object_class->dispose = ev_sidebar_links_dispose;

	widget_class->map = ev_sidebar_links_map;

	signals[LINK_ACTIVATED] = g_signal_new ("link-activated",
			 G_TYPE_FROM_CLASS (g_object_class),
		         G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		         G_STRUCT_OFFSET (EvSidebarLinksClass, link_activated),
		         NULL, NULL,
		         g_cclosure_marshal_VOID__OBJECT,
		         G_TYPE_NONE, 1, G_TYPE_OBJECT);

	g_object_class_install_property (g_object_class,
					 PROP_MODEL,
					 g_param_spec_object ("model",
							      "Model",
							      "Current Model",
							      CTK_TYPE_TREE_MODEL,
							      G_PARAM_READWRITE));
	g_object_class_override_property (g_object_class,
					  PROP_WIDGET,
					  "main-widget");
}

static void
selection_changed_callback (GtkTreeSelection   *selection,
		            EvSidebarLinks     *ev_sidebar_links)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (ev_sidebar_links->priv->document != NULL);

	if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
		EvLink *link;

		ctk_tree_model_get (model, &iter,
				    EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
				    -1);

		if (link == NULL)
			return;

		g_signal_handler_block (ev_sidebar_links->priv->doc_model,
					ev_sidebar_links->priv->page_changed_id);
		g_signal_emit (ev_sidebar_links, signals[LINK_ACTIVATED], 0, link);
		g_signal_handler_unblock (ev_sidebar_links->priv->doc_model,
					  ev_sidebar_links->priv->page_changed_id);

		g_object_unref (link);
	}
}

static GtkTreeModel *
create_loading_model (void)
{
	GtkTreeModel *retval;
	GtkTreeIter iter;
	gchar *markup;

	/* Creates a fake model to indicate that we're loading */
	retval = (GtkTreeModel *)ctk_list_store_new (EV_DOCUMENT_LINKS_COLUMN_NUM_COLUMNS,
						     G_TYPE_STRING,
						     G_TYPE_OBJECT,
						     G_TYPE_BOOLEAN,
						     G_TYPE_STRING);

	ctk_list_store_append (CTK_LIST_STORE (retval), &iter);
	markup = g_strdup_printf ("<span size=\"larger\" style=\"italic\">%s</span>", _("Loading…"));
	ctk_list_store_set (CTK_LIST_STORE (retval), &iter,
			    EV_DOCUMENT_LINKS_COLUMN_MARKUP, markup,
			    EV_DOCUMENT_LINKS_COLUMN_EXPAND, FALSE,
			    EV_DOCUMENT_LINKS_COLUMN_LINK, NULL,
			    -1);
	g_free (markup);

	return retval;
}

static void
print_section_cb (GtkWidget *menuitem, EvSidebarLinks *sidebar)
{
	GtkWidget *window;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	selection = ctk_tree_view_get_selection
		(CTK_TREE_VIEW (sidebar->priv->tree_view));

	if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
		EvLink *link;
		int first_page, last_page = -1;
		EvDocumentLinks *document_links;

		ctk_tree_model_get (model, &iter,
				    EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
				    -1);

		if (!link)
			return;

		document_links = EV_DOCUMENT_LINKS (sidebar->priv->document);

		first_page = ev_document_links_get_link_page (document_links, link);
		if (first_page == -1) {
			g_object_unref (link);
			return;
		}

		first_page++;
		g_object_unref (link);

		if (ctk_tree_model_iter_next (model, &iter)) {
			ctk_tree_model_get (model, &iter,
					    EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
					    -1);

			if (link) {
				last_page = ev_document_links_get_link_page (document_links, link);;
				g_object_unref (link);
			}
		} else {
			last_page = ev_document_get_n_pages (sidebar->priv->document);
		}

		if (last_page == -1)
			last_page = ev_document_get_n_pages (sidebar->priv->document);

		window = ctk_widget_get_toplevel (CTK_WIDGET (sidebar));
		if (EV_IS_WINDOW (window)) {
			ev_window_print_range (EV_WINDOW (window), first_page, last_page);
		}
	}
}

static GtkMenu *
build_popup_menu (EvSidebarLinks *sidebar)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = ctk_menu_new ();
	item = ctk_image_menu_item_new_from_stock ("ctk-print", NULL);
	ctk_label_set_label (CTK_LABEL (ctk_bin_get_child (CTK_BIN (item))), _("Print…"));
	ctk_widget_show (item);
	ctk_menu_shell_append (CTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate",
			  G_CALLBACK (print_section_cb), sidebar);

	return CTK_MENU (menu);
}

static void
popup_menu_cb (GtkWidget *treeview, EvSidebarLinks *sidebar)
{
	GtkMenu *menu = build_popup_menu (sidebar);

	ctk_menu_popup_at_pointer (menu, NULL);
	ctk_menu_shell_select_first (CTK_MENU_SHELL (menu), FALSE);
}

static gboolean
button_press_cb (GtkWidget *treeview,
                 GdkEventButton *event,
                 EvSidebarLinks *sidebar)
{
	GtkTreePath *path = NULL;

	if (event->button == 3) {
	        if (ctk_tree_view_get_path_at_pos (CTK_TREE_VIEW (treeview),
        	                                   event->x,
                	                           event->y,
	                                           &path,
        	                                   NULL, NULL, NULL)) {
			ctk_tree_view_set_cursor (CTK_TREE_VIEW (treeview),
						  path, NULL, FALSE);
			ctk_menu_popup_at_pointer (build_popup_menu (sidebar),
			                           (const GdkEvent*) event);
			ctk_tree_path_free (path);

			return TRUE;
		}
	}

	return FALSE;
}


static void
ev_sidebar_links_construct (EvSidebarLinks *ev_sidebar_links)
{
	EvSidebarLinksPrivate *priv;
	GtkWidget *swindow;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeModel *loading_model;

	priv = ev_sidebar_links->priv;

	swindow = ctk_scrolled_window_new (NULL, NULL);

	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
					CTK_POLICY_AUTOMATIC, CTK_POLICY_AUTOMATIC);
	ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow),
					     CTK_SHADOW_IN);

	/* Create tree view */
	loading_model = create_loading_model ();
	priv->tree_view = ctk_tree_view_new_with_model (loading_model);
	g_object_unref (loading_model);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
	ctk_tree_selection_set_mode (selection, CTK_SELECTION_NONE);
	ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->tree_view), FALSE);
	ctk_container_add (CTK_CONTAINER (swindow), priv->tree_view);

	ctk_box_pack_start (CTK_BOX (ev_sidebar_links), swindow, TRUE, TRUE, 0);
	ctk_widget_show_all (CTK_WIDGET (ev_sidebar_links));

	column = ctk_tree_view_column_new ();
	ctk_tree_view_column_set_expand (CTK_TREE_VIEW_COLUMN (column), TRUE);
	ctk_tree_view_append_column (CTK_TREE_VIEW (priv->tree_view), column);

	renderer = (GtkCellRenderer*)
		g_object_new (CTK_TYPE_CELL_RENDERER_TEXT,
			      "ellipsize", PANGO_ELLIPSIZE_END,
			      NULL);
	ctk_tree_view_column_pack_start (CTK_TREE_VIEW_COLUMN (column), renderer, TRUE);
	ctk_tree_view_column_set_attributes (CTK_TREE_VIEW_COLUMN (column), renderer,
					     "markup", EV_DOCUMENT_LINKS_COLUMN_MARKUP,
					     NULL);


	renderer = ctk_cell_renderer_text_new ();
	ctk_tree_view_column_pack_end (CTK_TREE_VIEW_COLUMN (column), renderer, FALSE);
	ctk_tree_view_column_set_attributes (CTK_TREE_VIEW_COLUMN (column), renderer,
					     "text", EV_DOCUMENT_LINKS_COLUMN_PAGE_LABEL,
					     NULL);
	g_object_set (G_OBJECT (renderer),
                      "style", PANGO_STYLE_ITALIC,
                      "xalign", 1.0,
                      NULL);

	g_signal_connect (priv->tree_view,
			  "button_press_event",
			  G_CALLBACK (button_press_cb),
			  ev_sidebar_links);
	g_signal_connect (priv->tree_view,
			  "popup_menu",
			  G_CALLBACK (popup_menu_cb),
			  ev_sidebar_links);
}

static void
ev_sidebar_links_init (EvSidebarLinks *ev_sidebar_links)
{
	ev_sidebar_links->priv = ev_sidebar_links_get_instance_private (ev_sidebar_links);

	ctk_orientable_set_orientation (CTK_ORIENTABLE (ev_sidebar_links), CTK_ORIENTATION_VERTICAL);

	ev_sidebar_links_construct (ev_sidebar_links);
}

/* Public Functions */

GtkWidget *
ev_sidebar_links_new (void)
{
	GtkWidget *ev_sidebar_links;

	ev_sidebar_links = g_object_new (EV_TYPE_SIDEBAR_LINKS, NULL);

	return ev_sidebar_links;
}

static gboolean
update_page_callback_foreach (GtkTreeModel *model,
			      GtkTreePath  *path,
			      GtkTreeIter  *iter,
			      gpointer      data)
{
	EvSidebarLinks *sidebar_links = (data);
	EvLink *link;

	ctk_tree_model_get (model, iter,
			    EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
			    -1);

	if (link) {
		int current_page;
		int dest_page;
		EvDocumentLinks *document_links = EV_DOCUMENT_LINKS (sidebar_links->priv->document);

		dest_page = ev_document_links_get_link_page (document_links, link);
		g_object_unref (link);

		current_page = ev_document_model_get_page (sidebar_links->priv->doc_model);

		if (dest_page == current_page) {
			ctk_tree_view_expand_to_path (CTK_TREE_VIEW (sidebar_links->priv->tree_view),
						      path);
			ctk_tree_view_set_cursor (CTK_TREE_VIEW (sidebar_links->priv->tree_view),
						  path, NULL, FALSE);

			return TRUE;
		}
	}

	return FALSE;
}

static void
ev_sidebar_links_set_current_page (EvSidebarLinks *sidebar_links,
				   gint            current_page)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	/* Widget is not currently visible */
	if (!ctk_widget_get_mapped (CTK_WIDGET (sidebar_links)))
		return;

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (sidebar_links->priv->tree_view));

	if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
		EvLink *link;

		ctk_tree_model_get (model, &iter,
				    EV_DOCUMENT_LINKS_COLUMN_LINK, &link,
				    -1);
		if (link) {
			gint dest_page;
			EvDocumentLinks *document_links = EV_DOCUMENT_LINKS (sidebar_links->priv->document);

			dest_page = ev_document_links_get_link_page (document_links, link);
			g_object_unref (link);

			if (dest_page == current_page)
				return;
		}
	}

	/* We go through the tree linearly looking for the first page that
	 * matches.  This is pretty inefficient.  We can do something neat with
	 * a GtkTreeModelSort here to make it faster, if it turns out to be
	 * slow.
	 */
	g_signal_handler_block (selection, sidebar_links->priv->selection_id);
	g_signal_handler_block (sidebar_links->priv->tree_view, sidebar_links->priv->row_activated_id);

	ctk_tree_model_foreach (model,
				update_page_callback_foreach,
				sidebar_links);

	g_signal_handler_unblock (selection, sidebar_links->priv->selection_id);
	g_signal_handler_unblock (sidebar_links->priv->tree_view, sidebar_links->priv->row_activated_id);
}

static void
update_page_callback (EvSidebarLinks *sidebar_links,
		      gint            old_page,
		      gint            new_page)
{
	ev_sidebar_links_set_current_page (sidebar_links, new_page);
}

static void
row_activated_callback (GtkTreeView       *treeview,
			GtkTreePath       *arg1,
			GtkTreeViewColumn *arg2,
			gpointer           user_data)
{
	if (ctk_tree_view_row_expanded (CTK_TREE_VIEW (treeview), arg1)) {
		ctk_tree_view_collapse_row (CTK_TREE_VIEW (treeview), arg1);
	} else {
		ctk_tree_view_expand_row (CTK_TREE_VIEW (treeview), arg1, FALSE);
	}
}

static void
expand_open_links (GtkTreeView *tree_view, GtkTreeModel *model, GtkTreeIter *parent)
{
	GtkTreeIter iter;
	gboolean expand;

	if (ctk_tree_model_iter_children (model, &iter, parent)) {
		do {
			ctk_tree_model_get (model, &iter,
					    EV_DOCUMENT_LINKS_COLUMN_EXPAND, &expand,
					    -1);
			if (expand) {
				GtkTreePath *path;

				path = ctk_tree_model_get_path (model, &iter);
				ctk_tree_view_expand_row (tree_view, path, FALSE);
				ctk_tree_path_free (path);
			}

			expand_open_links (tree_view, model, &iter);
		} while (ctk_tree_model_iter_next (model, &iter));
	}
}

static void
ev_sidebar_links_set_links_model (EvSidebarLinks *sidebar_links,
				  GtkTreeModel   *model)
{
	EvSidebarLinksPrivate *priv = sidebar_links->priv;

	if (priv->model == model)
		return;

	if (priv->model)
		g_object_unref (priv->model);
	priv->model = g_object_ref (model);

	g_object_notify (G_OBJECT (sidebar_links), "model");
}

static void
job_finished_callback (EvJobLinks     *job,
		       EvSidebarLinks *sidebar_links)
{
	EvSidebarLinksPrivate *priv = sidebar_links->priv;
	GtkTreeSelection *selection;

	ev_sidebar_links_set_links_model (sidebar_links, job->model);

	ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), job->model);

	g_object_unref (job);
	priv->job = NULL;

	expand_open_links (CTK_TREE_VIEW (priv->tree_view), priv->model, NULL);

	selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
	ctk_tree_selection_set_mode (selection, CTK_SELECTION_SINGLE);

	if (priv->selection_id <= 0) {
		priv->selection_id =
			g_signal_connect (selection, "changed",
					  G_CALLBACK (selection_changed_callback),
					  sidebar_links);
	}
	priv->page_changed_id =
		g_signal_connect_swapped (priv->doc_model, "page-changed",
					  G_CALLBACK (update_page_callback),
					  sidebar_links);
	if (priv->row_activated_id <= 0) {
		priv->row_activated_id =
			g_signal_connect (priv->tree_view, "row-activated",
					  G_CALLBACK (row_activated_callback),
					  sidebar_links);
	}

	ev_sidebar_links_set_current_page (sidebar_links,
					   ev_document_model_get_page (priv->doc_model));
}

static void
ev_sidebar_links_document_changed_cb (EvDocumentModel *model,
				      GParamSpec      *pspec,
				      EvSidebarLinks  *sidebar_links)
{
	EvDocument *document = ev_document_model_get_document (model);
	EvSidebarLinksPrivate *priv = sidebar_links->priv;

	if (!EV_IS_DOCUMENT_LINKS (document))
		return;

	if (!ev_document_links_has_document_links (EV_DOCUMENT_LINKS (document)))
		return;

	if (priv->document) {
		ctk_tree_view_set_model (CTK_TREE_VIEW (priv->tree_view), NULL);
		g_object_unref (priv->document);
	}

	priv->document = g_object_ref (document);

	if (priv->job) {
		g_signal_handlers_disconnect_by_func (priv->job,
						      job_finished_callback,
						      sidebar_links);
		g_object_unref (priv->job);
	}

	priv->job = ev_job_links_new (document);
	g_signal_connect (priv->job,
			  "finished",
			  G_CALLBACK (job_finished_callback),
			  sidebar_links);
	/* The priority doesn't matter for this job */
	ev_job_scheduler_push_job (priv->job, EV_JOB_PRIORITY_NONE);
}

static void
ev_sidebar_links_set_model (EvSidebarPage   *sidebar_page,
			    EvDocumentModel *model)
{
	EvSidebarLinks *sidebar_links = EV_SIDEBAR_LINKS (sidebar_page);
	EvSidebarLinksPrivate *priv = sidebar_links->priv;

	if (priv->doc_model == model)
		return;

	priv->doc_model = model;
	g_signal_connect (model, "notify::document",
			  G_CALLBACK (ev_sidebar_links_document_changed_cb),
			  sidebar_page);
}

static gboolean
ev_sidebar_links_support_document (EvSidebarPage  *sidebar_page,
				   EvDocument *document)
{
	return (EV_IS_DOCUMENT_LINKS (document) &&
		    ev_document_links_has_document_links (EV_DOCUMENT_LINKS (document)));
}

static const gchar*
ev_sidebar_links_get_label (EvSidebarPage *sidebar_page)
{
    return _("Index");
}

static void
ev_sidebar_links_page_iface_init (EvSidebarPageInterface *iface)
{
	iface->support_document = ev_sidebar_links_support_document;
	iface->set_model = ev_sidebar_links_set_model;
	iface->get_label = ev_sidebar_links_get_label;
}

