/* ev-sidebar-bookmarks.c
 *  this file is part of evince, a gnome document viewer
 *
 * Copyright (C) 2010 Carlos Garcia Campos  <carlosgc@gnome.org>
 *
 * Evince is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Evince is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "ev-sidebar-bookmarks.h"

#include "ev-document.h"
#include "ev-document-misc.h"
#include "ev-sidebar-page.h"
#include "ev-utils.h"

enum {
        PROP_0,
        PROP_WIDGET
};

enum {
        COLUMN_MARKUP,
        COLUMN_PAGE,
        N_COLUMNS
};

enum {
        ADD_BOOKMARK,
        N_SIGNALS
};

struct _EvSidebarBookmarksPrivate {
        EvDocumentModel *model;
        EvBookmarks     *bookmarks;

        CtkWidget       *tree_view;
        CtkWidget       *del_button;
        CtkWidget       *add_button;

        /* Popup menu */
        CtkWidget       *popup;
        CtkUIManager    *ui_manager;
        CtkActionGroup  *action_group;
};

static void ev_sidebar_bookmarks_page_iface_init (EvSidebarPageInterface *iface);

G_DEFINE_TYPE_EXTENDED (EvSidebarBookmarks,
                        ev_sidebar_bookmarks,
                        CTK_TYPE_BOX,
                        0,
                        G_ADD_PRIVATE (EvSidebarBookmarks)
                        G_IMPLEMENT_INTERFACE (EV_TYPE_SIDEBAR_PAGE,
                                               ev_sidebar_bookmarks_page_iface_init))

static guint signals[N_SIGNALS];

static const gchar popup_menu_ui[] =
        "<popup name=\"BookmarksPopup\" action=\"BookmarksPopupAction\">\n"
        "  <menuitem name=\"OpenBookmark\" action=\"OpenBookmark\"/>\n"
        "  <separator/>\n"
        "  <menuitem name=\"RenameBookmark\" action=\"RenameBookmark\"/>\n"
        "  <menuitem name=\"RemoveBookmark\" action=\"RemoveBookmark\"/>\n"
        "</popup>\n";

static gint
ev_sidebar_bookmarks_get_selected_page (EvSidebarBookmarks *sidebar_bookmarks,
                                        CtkTreeSelection   *selection)
{
        CtkTreeModel *model;
        CtkTreeIter   iter;

        if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
                guint page;

                ctk_tree_model_get (model, &iter,
                                    COLUMN_PAGE, &page,
                                    -1);
                return page;
        }

        return -1;
}

static void
ev_bookmarks_popup_cmd_open_bookmark (CtkAction          *action,
                                      EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeSelection          *selection;
        gint                       page;

        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
        page = ev_sidebar_bookmarks_get_selected_page (sidebar_bookmarks, selection);
        ev_document_model_set_page (priv->model, page);
}

static void
ev_bookmarks_popup_cmd_rename_bookmark (CtkAction          *action,
                                        EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeView               *tree_view = CTK_TREE_VIEW (priv->tree_view);
        CtkTreeSelection          *selection;
        CtkTreeModel              *model;
        CtkTreeIter                iter;


        selection = ctk_tree_view_get_selection (tree_view);
        if (ctk_tree_selection_get_selected (selection, &model, &iter)) {
                CtkTreePath *path;

                path = ctk_tree_model_get_path (model, &iter);
                ctk_tree_view_set_cursor (tree_view, path,
                                          ctk_tree_view_get_column (tree_view, 0),
                                          TRUE);
                ctk_tree_path_free (path);
        }
}

static void
ev_bookmarks_popup_cmd_remove_bookmark (CtkAction          *action,
                                        EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeSelection          *selection;
        gint                       page;
        EvBookmark                 bm;

        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
        page = ev_sidebar_bookmarks_get_selected_page (sidebar_bookmarks, selection);
        bm.page = page;
        bm.title = NULL;
        ev_bookmarks_delete (priv->bookmarks, &bm);
}

static const CtkActionEntry popup_entries[] = {
        { "OpenBookmark", "document-open", N_("_Open Bookmark"), NULL,
          NULL, G_CALLBACK (ev_bookmarks_popup_cmd_open_bookmark) },
        { "RenameBookmark", NULL, N_("_Rename Bookmark"), NULL,
          NULL, G_CALLBACK (ev_bookmarks_popup_cmd_rename_bookmark) },
        { "RemoveBookmark", NULL, N_("_Remove Bookmark"), NULL,
          NULL, G_CALLBACK (ev_bookmarks_popup_cmd_remove_bookmark) }
};

static gint
compare_bookmarks (EvBookmark *a,
                   EvBookmark *b)
{
        if (a->page < b->page)
                return -1;
        if (a->page > b->page)
                return 1;
        return 0;
}

static void
ev_sidebar_bookmarks_update (EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkListStore              *model;
        GList                     *items, *l;
        CtkTreeIter                iter;

        model = CTK_LIST_STORE (ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view)));
        ctk_list_store_clear (model);

        if (!priv->bookmarks) {
                g_object_set (priv->tree_view, "has-tooltip", FALSE, NULL);
                return;
        }

        items = ev_bookmarks_get_bookmarks (priv->bookmarks);
        items = g_list_sort (items, (GCompareFunc)compare_bookmarks);
        for (l = items; l; l = g_list_next (l)) {
                EvBookmark *bm = (EvBookmark *)l->data;

                ctk_list_store_append (model, &iter);
                ctk_list_store_set (model, &iter,
                                    COLUMN_MARKUP, bm->title,
                                    COLUMN_PAGE, bm->page,
                                    -1);
        }
        g_list_free (items);
        g_object_set (priv->tree_view, "has-tooltip", TRUE, NULL);
}

static void
ev_sidebar_bookmarks_changed (EvBookmarks        *bookmarks,
                              EvSidebarBookmarks *sidebar_bookmarks)
{
        ev_sidebar_bookmarks_update (sidebar_bookmarks);
}

static void
ev_sidebar_bookmarks_selection_changed (CtkTreeSelection   *selection,
                                        EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        gint                       page;

        page = ev_sidebar_bookmarks_get_selected_page (sidebar_bookmarks, selection);
        if (page >= 0) {
                ev_document_model_set_page (priv->model, page);
                ctk_widget_set_sensitive (priv->del_button, TRUE);
        } else {
                ctk_widget_set_sensitive (priv->del_button, FALSE);
        }
}

static void
ev_sidebar_bookmarks_add_clicked (CtkWidget          *button,
                                  EvSidebarBookmarks *sidebar_bookmarks)
{
        /* Let the window add the bookmark since
         * since we don't know the page title
         */
        g_signal_emit (sidebar_bookmarks, signals[ADD_BOOKMARK], 0);
}

static void
ev_sidebar_bookmarks_del_clicked (CtkWidget          *button,
                                  EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeSelection          *selection;
        gint                       page;
        EvBookmark                 bm;

        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
        page = ev_sidebar_bookmarks_get_selected_page (sidebar_bookmarks, selection);
        if (page < 0)
                return;

        bm.page = page;
        bm.title = NULL;
        ev_bookmarks_delete (priv->bookmarks, &bm);
}

static void
ev_sidebar_bookmarks_bookmark_renamed (CtkCellRendererText *renderer,
                                       const gchar         *path_string,
                                       const gchar         *new_text,
                                       EvSidebarBookmarks  *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreePath               *path = ctk_tree_path_new_from_string (path_string);
        CtkTreeModel              *model;
        CtkTreeIter                iter;
        guint                      page;
        EvBookmark                 bm;

        if (!new_text || new_text[0] == '\0')
                return;

        model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));
        ctk_tree_model_get_iter (model, &iter, path);
        ctk_tree_model_get (model, &iter,
                            COLUMN_PAGE, &page,
                            -1);
        ctk_tree_path_free (path);

        bm.page = page;
        bm.title = g_strdup (new_text);
        ev_bookmarks_update (priv->bookmarks, &bm);
}

static gboolean
ev_sidebar_bookmarks_query_tooltip (CtkWidget          *widget,
                                    gint                x,
                                    gint                y,
                                    gboolean            keyboard_tip,
                                    CtkTooltip         *tooltip,
                                    EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeModel              *model;
        CtkTreeIter                iter;
        CtkTreePath               *path = NULL;
        EvDocument                *document;
        guint                      page;
        gchar                     *page_label;
        gchar                     *text;

        model = ctk_tree_view_get_model (CTK_TREE_VIEW (priv->tree_view));
        if (!ctk_tree_view_get_tooltip_context (CTK_TREE_VIEW (priv->tree_view),
                                                &x, &y, keyboard_tip,
                                                &model, &path, &iter))
                return FALSE;

        ctk_tree_model_get (model, &iter,
                            COLUMN_PAGE, &page,
                            -1);

        document = ev_document_model_get_document (priv->model);
        page_label = ev_document_get_page_label (document, page);
        text = g_strdup_printf (_("Page %s"), page_label);
        ctk_tooltip_set_text (tooltip, text);
        g_free (text);
        g_free (page_label);

        ctk_tree_view_set_tooltip_row (CTK_TREE_VIEW (priv->tree_view),
                                       tooltip, path);
        ctk_tree_path_free (path);

        return TRUE;
}

static gboolean
ev_sidebar_bookmarks_popup_menu_show (EvSidebarBookmarks *sidebar_bookmarks,
                                      gint                x,
                                      gint                y,
                                      gboolean            keyboard_mode)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;
        CtkTreeView               *tree_view = CTK_TREE_VIEW (sidebar_bookmarks->priv->tree_view);
        CtkTreeSelection          *selection = ctk_tree_view_get_selection (tree_view);

        if (keyboard_mode) {
                if (!ctk_tree_selection_get_selected (selection, NULL, NULL))
                        return FALSE;
        } else {
                CtkTreePath *path;

                if (!ctk_tree_view_get_path_at_pos (tree_view, x, y, &path, NULL, NULL, NULL))
                        return FALSE;

                g_signal_handlers_block_by_func (selection,
                                                 ev_sidebar_bookmarks_selection_changed,
                                                 sidebar_bookmarks);
                ctk_tree_view_set_cursor (tree_view, path, NULL, FALSE);
                g_signal_handlers_unblock_by_func (selection,
                                                   ev_sidebar_bookmarks_selection_changed,
                                                   sidebar_bookmarks);
                ctk_tree_path_free (path);
        }

        if (!priv->popup)
                priv->popup = ctk_ui_manager_get_widget (priv->ui_manager, "/BookmarksPopup");

        ctk_menu_popup_at_pointer (CTK_MENU (priv->popup), NULL);
        return TRUE;
}

static gboolean
ev_sidebar_bookmarks_button_press (CtkWidget          *widget,
                                   CdkEventButton     *event,
                                   EvSidebarBookmarks *sidebar_bookmarks)
{
        if (event->button != 3)
                return FALSE;

        return ev_sidebar_bookmarks_popup_menu_show (sidebar_bookmarks, event->x, event->y, FALSE);
}

static gboolean
ev_sidebar_bookmarks_popup_menu (CtkWidget *widget)
{
        EvSidebarBookmarks *sidebar_bookmarks = EV_SIDEBAR_BOOKMARKS (widget);
        gint                x, y;

        ev_document_misc_get_pointer_position (widget, &x, &y);
        return ev_sidebar_bookmarks_popup_menu_show (sidebar_bookmarks, x, y, TRUE);
}

static void
ev_sidebar_bookmarks_dispose (GObject *object)
{
        EvSidebarBookmarks *sidebar_bookmarks = EV_SIDEBAR_BOOKMARKS (object);
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;

        if (priv->model) {
                g_object_unref (priv->model);
                priv->model = NULL;
        }

        if (priv->bookmarks) {
                g_object_unref (priv->bookmarks);
                priv->bookmarks = NULL;
        }

        if (priv->action_group) {
                g_object_unref (priv->action_group);
                priv->action_group = NULL;
        }

        if (priv->ui_manager) {
                g_object_unref (priv->ui_manager);
                priv->ui_manager = NULL;
        }

        G_OBJECT_CLASS (ev_sidebar_bookmarks_parent_class)->dispose (object);
}

static void
ev_sidebar_bookmarks_init (EvSidebarBookmarks *sidebar_bookmarks)
{
        EvSidebarBookmarksPrivate *priv;
        CtkWidget                 *swindow;
        CtkWidget                 *hbox;
        CtkListStore              *model;
        CtkCellRenderer           *renderer;
        CtkTreeSelection          *selection;

        sidebar_bookmarks->priv = ev_sidebar_bookmarks_get_instance_private (sidebar_bookmarks);
        priv = sidebar_bookmarks->priv;

        ctk_orientable_set_orientation (CTK_ORIENTABLE (sidebar_bookmarks), CTK_ORIENTATION_VERTICAL);
        ctk_box_set_spacing (CTK_BOX (sidebar_bookmarks), 6);

        swindow = ctk_scrolled_window_new (NULL, NULL);
        ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
                                        CTK_POLICY_AUTOMATIC,
                                        CTK_POLICY_AUTOMATIC);
        ctk_scrolled_window_set_shadow_type (CTK_SCROLLED_WINDOW (swindow),
                                             CTK_SHADOW_IN);
        ctk_box_pack_start (CTK_BOX (sidebar_bookmarks), swindow, TRUE, TRUE, 0);
        ctk_widget_show (swindow);

        model = ctk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT);
        priv->tree_view = ctk_tree_view_new_with_model (CTK_TREE_MODEL (model));
        g_object_unref (model);
        g_signal_connect (priv->tree_view, "query-tooltip",
                          G_CALLBACK (ev_sidebar_bookmarks_query_tooltip),
                          sidebar_bookmarks);
        g_signal_connect (priv->tree_view,
                          "button-press-event",
                          G_CALLBACK (ev_sidebar_bookmarks_button_press),
                          sidebar_bookmarks);
        ctk_tree_view_set_headers_visible (CTK_TREE_VIEW (priv->tree_view), FALSE);
        selection = ctk_tree_view_get_selection (CTK_TREE_VIEW (priv->tree_view));
        g_signal_connect (selection, "changed",
                          G_CALLBACK (ev_sidebar_bookmarks_selection_changed),
                          sidebar_bookmarks);

        renderer = ctk_cell_renderer_text_new ();
        g_object_set (renderer,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      "editable", TRUE,
                      NULL);
        g_signal_connect (renderer, "edited",
                          G_CALLBACK (ev_sidebar_bookmarks_bookmark_renamed),
                          sidebar_bookmarks);
        ctk_tree_view_insert_column_with_attributes (CTK_TREE_VIEW (priv->tree_view),
                                                     0, NULL, renderer,
                                                     "markup", COLUMN_MARKUP,
                                                     NULL);
        ctk_container_add (CTK_CONTAINER (swindow), priv->tree_view);
        ctk_widget_show (priv->tree_view);

        hbox = ctk_button_box_new (CTK_ORIENTATION_HORIZONTAL);

        priv->add_button = ctk_button_new_with_mnemonic (_("_Add"));
        ctk_button_set_image (CTK_BUTTON (priv->add_button), ctk_image_new_from_icon_name ("list-add", CTK_ICON_SIZE_BUTTON));

        g_signal_connect (priv->add_button, "clicked",
                          G_CALLBACK (ev_sidebar_bookmarks_add_clicked),
                          sidebar_bookmarks);
        ctk_widget_set_sensitive (priv->add_button, FALSE);
        ctk_box_pack_start (CTK_BOX (hbox), priv->add_button, TRUE, TRUE, 6);
        ctk_widget_show (priv->add_button);

        priv->del_button = ctk_button_new_with_mnemonic (_("_Remove"));
        ctk_button_set_image (CTK_BUTTON (priv->del_button), ctk_image_new_from_icon_name ("list-remove", CTK_ICON_SIZE_BUTTON));

        g_signal_connect (priv->del_button, "clicked",
                          G_CALLBACK (ev_sidebar_bookmarks_del_clicked),
                          sidebar_bookmarks);
        ctk_widget_set_sensitive (priv->del_button, FALSE);
        ctk_box_pack_start (CTK_BOX (hbox), priv->del_button, TRUE, TRUE, 6);
        ctk_widget_show (priv->del_button);

        ctk_box_pack_end (CTK_BOX (sidebar_bookmarks), hbox, FALSE, TRUE, 0);
        ctk_widget_show (hbox);
        ctk_widget_show (CTK_WIDGET (sidebar_bookmarks));

        /* Popup menu */
        priv->action_group = ctk_action_group_new ("BookmarsPopupActions");
        ctk_action_group_set_translation_domain (priv->action_group, NULL);
        ctk_action_group_add_actions (priv->action_group, popup_entries,
                                      G_N_ELEMENTS (popup_entries),
                                      sidebar_bookmarks);
        priv->ui_manager = ctk_ui_manager_new ();
        ctk_ui_manager_insert_action_group (priv->ui_manager,
                                            priv->action_group, 0);
        ctk_ui_manager_add_ui_from_string (priv->ui_manager, popup_menu_ui, -1, NULL);
}

static void
ev_sidebar_bookmarks_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        EvSidebarBookmarks *sidebar_bookmarks;

        sidebar_bookmarks = EV_SIDEBAR_BOOKMARKS (object);

        switch (prop_id) {
        case PROP_WIDGET:
                g_value_set_object (value, sidebar_bookmarks->priv->tree_view);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ev_sidebar_bookmarks_class_init (EvSidebarBookmarksClass *klass)
{
        GObjectClass   *g_object_class = G_OBJECT_CLASS (klass);
        CtkWidgetClass *widget_class = CTK_WIDGET_CLASS (klass);

        g_object_class->get_property = ev_sidebar_bookmarks_get_property;
        g_object_class->dispose = ev_sidebar_bookmarks_dispose;

        widget_class->popup_menu = ev_sidebar_bookmarks_popup_menu;

        g_object_class_override_property (g_object_class, PROP_WIDGET, "main-widget");

        signals[ADD_BOOKMARK] =
                g_signal_new ("add-bookmark",
                              G_TYPE_FROM_CLASS (g_object_class),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                              G_STRUCT_OFFSET (EvSidebarBookmarksClass, add_bookmark),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0,
                              G_TYPE_NONE);
}

CtkWidget *
ev_sidebar_bookmarks_new (void)
{
        return CTK_WIDGET (g_object_new (EV_TYPE_SIDEBAR_BOOKMARKS, NULL));
}

void
ev_sidebar_bookmarks_set_bookmarks (EvSidebarBookmarks *sidebar_bookmarks,
                                    EvBookmarks        *bookmarks)
{
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;

        g_return_if_fail (EV_IS_BOOKMARKS (bookmarks));

        if (priv->bookmarks == bookmarks)
                return;

        if (priv->bookmarks)
                g_object_unref (priv->bookmarks);
        priv->bookmarks = g_object_ref (bookmarks);
        g_signal_connect (priv->bookmarks, "changed",
                          G_CALLBACK (ev_sidebar_bookmarks_changed),
                          sidebar_bookmarks);

        ctk_widget_set_sensitive (priv->add_button, TRUE);
        ev_sidebar_bookmarks_update (sidebar_bookmarks);
}

/* EvSidebarPageIface */
static void
ev_sidebar_bookmarks_set_model (EvSidebarPage   *sidebar_page,
                                EvDocumentModel *model)
{
        EvSidebarBookmarks *sidebar_bookmarks = EV_SIDEBAR_BOOKMARKS (sidebar_page);
        EvSidebarBookmarksPrivate *priv = sidebar_bookmarks->priv;

        if (priv->model == model)
                return;

        if (priv->model)
                g_object_unref (priv->model);
        priv->model = g_object_ref (model);
}

static gboolean
ev_sidebar_bookmarks_support_document (EvSidebarPage *sidebar_page,
                                       EvDocument    *document)
{
        return TRUE;
}

static const gchar *
ev_sidebar_bookmarks_get_label (EvSidebarPage *sidebar_page)
{
        return _("Bookmarks");
}

static void
ev_sidebar_bookmarks_page_iface_init (EvSidebarPageInterface *iface)
{
        iface->support_document = ev_sidebar_bookmarks_support_document;
        iface->set_model = ev_sidebar_bookmarks_set_model;
        iface->get_label = ev_sidebar_bookmarks_get_label;
}
