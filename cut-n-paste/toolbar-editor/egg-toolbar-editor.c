/*
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  $Id$
 */

#include "config.h"

#include "egg-toolbar-editor.h"
#include "egg-editable-toolbar.h"

#include <string.h>
#include <libxml/tree.h>
#include <ctk/ctk.h>
#include <glib/gi18n.h>

static const CtkTargetEntry dest_drag_types[] = {
  {EGG_TOOLBAR_ITEM_TYPE, CTK_TARGET_SAME_APP, 0},
};

static const CtkTargetEntry source_drag_types[] = {
  {EGG_TOOLBAR_ITEM_TYPE, CTK_TARGET_SAME_APP, 0},
};


static void egg_toolbar_editor_finalize         (GObject *object);
static void update_editor_sheet                 (EggToolbarEditor *editor);

enum
{
  PROP_0,
  PROP_UI_MANAGER,
  PROP_TOOLBARS_MODEL
};

enum
{
  SIGNAL_HANDLER_ITEM_ADDED,
  SIGNAL_HANDLER_ITEM_REMOVED,
  SIGNAL_HANDLER_TOOLBAR_REMOVED,
  SIGNAL_HANDLER_LIST_SIZE  /* Array size */
};

struct EggToolbarEditorPrivate
{
  CtkUIManager *manager;
  EggToolbarsModel *model;

  CtkWidget *grid;
  CtkWidget *scrolled_window;
  GList     *actions_list;
  GList     *factory_list;

  /* These handlers need to be sanely disconnected when switching models */
  gulong     sig_handlers[SIGNAL_HANDLER_LIST_SIZE];
};

G_DEFINE_TYPE_WITH_PRIVATE (EggToolbarEditor, egg_toolbar_editor, CTK_TYPE_BOX);

static gint
compare_items (gconstpointer a,
               gconstpointer b)
{
  const CtkWidget *item1 = a;
  const CtkWidget *item2 = b;

  char *key1 = g_object_get_data (G_OBJECT (item1),
                                  "egg-collate-key");
  char *key2 = g_object_get_data (G_OBJECT (item2),
                                  "egg-collate-key");

  return strcmp (key1, key2);
}

static CtkAction *
find_action (EggToolbarEditor *t,
             const char       *name)
{
  GList *l;
  CtkAction *action = NULL;

  l = ctk_ui_manager_get_action_groups (t->priv->manager);

  g_return_val_if_fail (EGG_IS_TOOLBAR_EDITOR (t), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (; l != NULL; l = l->next)
    {
      CtkAction *tmp;

      tmp = ctk_action_group_get_action (CTK_ACTION_GROUP (l->data), name);
      if (tmp)
        action = tmp;
    }

  return action;
}

static void
egg_toolbar_editor_set_ui_manager (EggToolbarEditor *t,
                                   CtkUIManager     *manager)
{
  g_return_if_fail (CTK_IS_UI_MANAGER (manager));

  t->priv->manager = g_object_ref (manager);
}

static void
item_added_or_removed_cb (EggToolbarsModel   *model,
                          int                 tpos,
                          int                 ipos,
                          EggToolbarEditor   *editor)
{
  update_editor_sheet (editor);
}

static void
toolbar_removed_cb (EggToolbarsModel   *model,
                    int                 position,
                    EggToolbarEditor   *editor)
{
  update_editor_sheet (editor);
}

static void
egg_toolbar_editor_disconnect_model (EggToolbarEditor *t)
{
  EggToolbarEditorPrivate *priv = t->priv;
  EggToolbarsModel *model = priv->model;
  gulong handler;
  int i;

  for (i = 0; i < SIGNAL_HANDLER_LIST_SIZE; i++)
    {
      handler = priv->sig_handlers[i];

      if (handler != 0)
        {
          if (g_signal_handler_is_connected (model, handler))
            {
              g_signal_handler_disconnect (model, handler);
            }

          priv->sig_handlers[i] = 0;
        }
    }
}

void
egg_toolbar_editor_set_model (EggToolbarEditor *t,
                              EggToolbarsModel *model)
{
  EggToolbarEditorPrivate *priv;

  g_return_if_fail (EGG_IS_TOOLBAR_EDITOR (t));
  g_return_if_fail (model != NULL);

  priv = t->priv;

  if (priv->model)
    {
      if (G_UNLIKELY (priv->model == model)) return;

      egg_toolbar_editor_disconnect_model (t);
      g_object_unref (priv->model);
    }

  priv->model = g_object_ref (model);

  update_editor_sheet (t);

  priv->sig_handlers[SIGNAL_HANDLER_ITEM_ADDED] =
    g_signal_connect_object (model, "item_added",
                             G_CALLBACK (item_added_or_removed_cb), t, 0);
  priv->sig_handlers[SIGNAL_HANDLER_ITEM_REMOVED] =
    g_signal_connect_object (model, "item_removed",
                             G_CALLBACK (item_added_or_removed_cb), t, 0);
  priv->sig_handlers[SIGNAL_HANDLER_TOOLBAR_REMOVED] =
    g_signal_connect_object (model, "toolbar_removed",
                             G_CALLBACK (toolbar_removed_cb), t, 0);
}

static void
egg_toolbar_editor_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  EggToolbarEditor *t = EGG_TOOLBAR_EDITOR (object);

  switch (prop_id)
    {
    case PROP_UI_MANAGER:
      egg_toolbar_editor_set_ui_manager (t, g_value_get_object (value));
      break;
    case PROP_TOOLBARS_MODEL:
      egg_toolbar_editor_set_model (t, g_value_get_object (value));
      break;
    }
}

static void
egg_toolbar_editor_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  EggToolbarEditor *t = EGG_TOOLBAR_EDITOR (object);

  switch (prop_id)
    {
    case PROP_UI_MANAGER:
      g_value_set_object (value, t->priv->manager);
      break;
    case PROP_TOOLBARS_MODEL:
      g_value_set_object (value, t->priv->model);
      break;
    }
}

static void
egg_toolbar_editor_class_init (EggToolbarEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = egg_toolbar_editor_finalize;
  object_class->set_property = egg_toolbar_editor_set_property;
  object_class->get_property = egg_toolbar_editor_get_property;

  g_object_class_install_property (object_class,
                                   PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        "UI-Manager",
                                                        "UI Manager",
                                                        CTK_TYPE_UI_MANAGER,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB |
                                                        G_PARAM_CONSTRUCT_ONLY));
 g_object_class_install_property (object_class,
                                  PROP_TOOLBARS_MODEL,
                                  g_param_spec_object ("model",
                                                       "Model",
                                                       "Toolbars Model",
                                                       EGG_TYPE_TOOLBARS_MODEL,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB |
                                                       G_PARAM_CONSTRUCT));

  CtkWidgetClass *widget_class  = CTK_WIDGET_CLASS (klass);
  ctk_widget_class_set_css_name (widget_class, "EggToolbarEditor");
}

static void
egg_toolbar_editor_finalize (GObject *object)
{
  EggToolbarEditor *editor = EGG_TOOLBAR_EDITOR (object);

  if (editor->priv->manager)
    {
      g_object_unref (editor->priv->manager);
    }

  if (editor->priv->model)
    {
      egg_toolbar_editor_disconnect_model (editor);
      g_object_unref (editor->priv->model);
    }

  g_list_free (editor->priv->actions_list);
  g_list_free (editor->priv->factory_list);

  G_OBJECT_CLASS (egg_toolbar_editor_parent_class)->finalize (object);
}

CtkWidget *
egg_toolbar_editor_new (CtkUIManager *manager,
                        EggToolbarsModel *model)
{
  return CTK_WIDGET (g_object_new (EGG_TYPE_TOOLBAR_EDITOR,
                                   "ui-manager", manager,
                                   "model", model,
                                   NULL));
}

static void
drag_begin_cb (CtkWidget          *widget,
               CdkDragContext     *context)
{
  ctk_widget_hide (widget);
}

static void
drag_end_cb (CtkWidget          *widget,
             CdkDragContext     *context)
{
  ctk_widget_show (widget);
}

static void
drag_data_get_cb (CtkWidget          *widget,
                  CdkDragContext     *context,
                  CtkSelectionData   *selection_data,
                  guint               info,
                  guint32             time,
                  EggToolbarEditor   *editor)
{
  const char *target;

  target = g_object_get_data (G_OBJECT (widget), "egg-item-name");
  g_return_if_fail (target != NULL);

  ctk_selection_data_set (selection_data,
                          ctk_selection_data_get_target (selection_data), 8,
                          (const guchar *) target, strlen (target));
}

static gchar *
elide_underscores (const gchar *original)
{
  gchar *q, *result;
  const gchar *p;
  gboolean last_underscore;

  q = result = g_malloc (strlen (original) + 1);
  last_underscore = FALSE;

  for (p = original; *p; p++)
    {
      if (!last_underscore && *p == '_')
        last_underscore = TRUE;
      else
        {
          last_underscore = FALSE;
          *q++ = *p;
        }
    }

  *q = '\0';

  return result;
}

static void
set_drag_cursor (CtkWidget *widget)
{
  CdkCursor *cursor;
  CdkScreen *screen;

  screen = ctk_widget_get_screen (widget);

  cursor = cdk_cursor_new_for_display (cdk_screen_get_display (screen),
                                       CDK_HAND2);
  cdk_window_set_cursor (ctk_widget_get_window (widget), cursor);
  g_object_unref (cursor);
}

static void
event_box_realize_cb (CtkWidget *widget, CtkImage *icon)
{
  CtkImageType type;

  set_drag_cursor (widget);

  type = ctk_image_get_storage_type (icon);
  if (type == CTK_IMAGE_STOCK)
    {
      gchar *stock_id;
      CdkPixbuf *pixbuf;

      ctk_image_get_stock (icon, &stock_id, NULL);
      pixbuf = ctk_widget_render_icon_pixbuf (widget, stock_id,
                                              CTK_ICON_SIZE_LARGE_TOOLBAR);
      ctk_drag_source_set_icon_pixbuf (widget, pixbuf);
      g_object_unref (pixbuf);
    }
  else if (type == CTK_IMAGE_ICON_NAME)
    {
      const gchar *icon_name;
      CdkScreen *screen;
      CtkIconTheme *icon_theme;
      gint width, height;
      CdkPixbuf *pixbuf;

      ctk_image_get_icon_name (icon, &icon_name, NULL);
      screen = ctk_widget_get_screen (widget);
      icon_theme = ctk_icon_theme_get_for_screen (screen);

      if (!ctk_icon_size_lookup (CTK_ICON_SIZE_LARGE_TOOLBAR,
                                 &width, &height))
        {
          width = height = 24;
        }

      pixbuf = ctk_icon_theme_load_icon (icon_theme, icon_name,
                                         MIN (width, height), 0, NULL);
      if (G_UNLIKELY (!pixbuf))
        return;

      ctk_drag_source_set_icon_pixbuf (widget, pixbuf);
      g_object_unref (pixbuf);

    }
  else if (type == CTK_IMAGE_PIXBUF)
    {
      CdkPixbuf *pixbuf = ctk_image_get_pixbuf (icon);
      ctk_drag_source_set_icon_pixbuf (widget, pixbuf);
    }
}

static CtkWidget *
editor_create_item (EggToolbarEditor *editor,
                    CtkImage         *icon,
                    const char       *label_text,
                    CdkDragAction    action)
{
  CtkWidget *event_box;
  CtkWidget *vbox;
  CtkWidget *label;
  gchar *label_no_mnemonic = NULL;

  event_box = ctk_event_box_new ();
  ctk_event_box_set_visible_window (CTK_EVENT_BOX (event_box), FALSE);
  ctk_widget_show (event_box);
  ctk_drag_source_set (event_box,
                       CDK_BUTTON1_MASK,
                       source_drag_types, G_N_ELEMENTS (source_drag_types), action);
  g_signal_connect (event_box, "drag_data_get",
                    G_CALLBACK (drag_data_get_cb), editor);
  g_signal_connect_after (event_box, "realize",
                          G_CALLBACK (event_box_realize_cb), icon);

  if (action == CDK_ACTION_MOVE)
    {
      g_signal_connect (event_box, "drag_begin",
                        G_CALLBACK (drag_begin_cb), NULL);
      g_signal_connect (event_box, "drag_end",
                        G_CALLBACK (drag_end_cb), NULL);
    }

  vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
  ctk_widget_show (vbox);
  ctk_container_add (CTK_CONTAINER (event_box), vbox);

  ctk_widget_show (CTK_WIDGET (icon));
  ctk_box_pack_start (CTK_BOX (vbox), CTK_WIDGET (icon), FALSE, TRUE, 0);
  label_no_mnemonic = elide_underscores (label_text);
  label = ctk_label_new (label_no_mnemonic);
  g_free (label_no_mnemonic);
  ctk_widget_show (label);
  ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, TRUE, 0);

  return event_box;
}

static CtkWidget *
editor_create_item_from_name (EggToolbarEditor *editor,
                              const char *      name,
                              CdkDragAction     drag_action)
{
  CtkWidget *item;
  const char *item_name;
  char *short_label;
  const char *collate_key;

  if (strcmp (name, "_separator") == 0)
    {
      CtkWidget *icon;

      icon = _egg_editable_toolbar_new_separator_image ();
      short_label = _("Separator");
      item_name = g_strdup (name);
      collate_key = g_utf8_collate_key (short_label, -1);
      item = editor_create_item (editor, CTK_IMAGE (icon),
                                 short_label, drag_action);
    }
  else
    {
      CtkAction *action;
      CtkWidget *icon;
      char *stock_id, *icon_name = NULL;

      action = find_action (editor, name);
      g_return_val_if_fail (action != NULL, NULL);

      g_object_get (action,
                    "icon-name", &icon_name,
                    "stock-id", &stock_id,
                    "short-label", &short_label,
                    NULL);

      /* This is a workaround to catch named icons. */
      if (icon_name)
        icon = ctk_image_new_from_icon_name (icon_name,
                                             CTK_ICON_SIZE_LARGE_TOOLBAR);
      else
        icon = ctk_image_new_from_icon_name (stock_id ? stock_id : "ctk-dnd",
                                             CTK_ICON_SIZE_LARGE_TOOLBAR);

      item_name = g_strdup (name);
      collate_key = g_utf8_collate_key (short_label, -1);
      item = editor_create_item (editor, CTK_IMAGE (icon),
                                 short_label, drag_action);

      g_free (short_label);
      g_free (stock_id);
      g_free (icon_name);
    }

  g_object_set_data_full (G_OBJECT (item), "egg-collate-key",
                          (gpointer) collate_key, g_free);
  g_object_set_data_full (G_OBJECT (item), "egg-item-name",
                          (gpointer) item_name, g_free);

  return item;
}

static gint
append_grid (CtkGrid *grid, GList *items, gint y, gint width)
{
  if (items != NULL)
    {
      gint x = 0;
      CtkWidget *item;

      if (y > 0)
        {
          item = ctk_separator_new (CTK_ORIENTATION_HORIZONTAL);
          ctk_widget_set_hexpand (item, TRUE);
          ctk_widget_set_vexpand (item, FALSE);
          ctk_widget_show (item);

          ctk_grid_attach (grid, item, 0, y, width, 1);
          y++;
        }

      for (; items != NULL; items = items->next)
        {
          item = items->data;
          ctk_widget_set_hexpand (item, FALSE);
          ctk_widget_set_vexpand (item, FALSE);
          ctk_widget_show (item);

          if (x >= width)
            {
              x = 0;
              y++;
            }
          ctk_grid_attach (grid, item, x, y, 1, 1);
          x++;
        }

      y++;
    }
  return y;
}

static void
update_editor_sheet (EggToolbarEditor *editor)
{
  gint y;
  GPtrArray *items;
  GList *to_move = NULL, *to_copy = NULL;
  CtkWidget *grid;
  CtkWidget *viewport;

  g_return_if_fail (EGG_IS_TOOLBAR_EDITOR (editor));

  /* Create new grid. */
  grid = ctk_grid_new ();
  editor->priv->grid = grid;
  ctk_container_set_border_width (CTK_CONTAINER (grid), 12);
  ctk_grid_set_row_spacing (CTK_GRID (grid), 24);
  ctk_widget_show (grid);
  ctk_drag_dest_set (grid, CTK_DEST_DEFAULT_ALL,
                     dest_drag_types, G_N_ELEMENTS (dest_drag_types),
                     CDK_ACTION_MOVE | CDK_ACTION_COPY);

  /* Build two lists of items (one for copying, one for moving). */
  items = egg_toolbars_model_get_name_avail (editor->priv->model);
  while (items->len > 0)
    {
      CtkWidget *item;
      const char *name;
      gint flags;

      name = g_ptr_array_index (items, 0);
      g_ptr_array_remove_index_fast (items, 0);

      flags = egg_toolbars_model_get_name_flags (editor->priv->model, name);
      if ((flags & EGG_TB_MODEL_NAME_INFINITE) == 0)
        {
          item = editor_create_item_from_name (editor, name, CDK_ACTION_MOVE);
          if (item != NULL)
            to_move = g_list_insert_sorted (to_move, item, compare_items);
        }
      else
        {
          item = editor_create_item_from_name (editor, name, CDK_ACTION_COPY);
          if (item != NULL)
            to_copy = g_list_insert_sorted (to_copy, item, compare_items);
        }
    }

  /* Add them to the sheet. */
  y = 0;
  y = append_grid (CTK_GRID (grid), to_move, y, 4);
  y = append_grid (CTK_GRID (grid), to_copy, y, 4);

  g_list_free (to_move);
  g_list_free (to_copy);
  g_ptr_array_free (items, TRUE);

  /* Delete old grid. */
  viewport = ctk_bin_get_child (CTK_BIN (editor->priv->scrolled_window));
  if (viewport)
    {
      ctk_container_remove (CTK_CONTAINER (viewport),
                            ctk_bin_get_child (CTK_BIN (viewport)));
    }

  /* Add grid to window. */
  ctk_scrolled_window_add_with_viewport
    (CTK_SCROLLED_WINDOW (editor->priv->scrolled_window), grid);
}

static void
setup_editor (EggToolbarEditor *editor)
{
  CtkWidget *scrolled_window;

  ctk_container_set_border_width (CTK_CONTAINER (editor), 12);
  scrolled_window = ctk_scrolled_window_new (NULL, NULL);
  editor->priv->scrolled_window = scrolled_window;
  ctk_widget_show (scrolled_window);
  ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (scrolled_window),
                                  CTK_POLICY_NEVER, CTK_POLICY_AUTOMATIC);
  ctk_box_pack_start (CTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);
}

static void
egg_toolbar_editor_init (EggToolbarEditor *t)
{
  ctk_orientable_set_orientation (CTK_ORIENTABLE (t), CTK_ORIENTATION_VERTICAL);

  t->priv = egg_toolbar_editor_get_instance_private (t);

  t->priv->manager = NULL;
  t->priv->actions_list = NULL;

  setup_editor (t);
}

