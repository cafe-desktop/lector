/* Copyright (C) 2004 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Cafe Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <cdk/cdkkeysyms.h>

#include "eggfindbar.h"

struct _EggFindBarPrivate
{
  gchar *search_string;

  CtkToolItem *next_button;
  CtkToolItem *previous_button;
  CtkToolItem *status_separator;
  CtkToolItem *status_item;
  CtkToolItem *case_button;

  CtkWidget *find_entry;
  CtkWidget *status_label;

  gulong set_focus_handler;
  guint case_sensitive : 1;
};

enum {
    PROP_0,
    PROP_SEARCH_STRING,
    PROP_CASE_SENSITIVE
};

static void egg_find_bar_finalize      (GObject        *object);
static void egg_find_bar_get_property  (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);
static void egg_find_bar_set_property  (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void egg_find_bar_show          (CtkWidget *widget);
static void egg_find_bar_hide          (CtkWidget *widget);

G_DEFINE_TYPE_WITH_PRIVATE (EggFindBar, egg_find_bar, CTK_TYPE_TOOLBAR);

enum
  {
    NEXT,
    PREVIOUS,
    CLOSE,
    SCROLL,
    LAST_SIGNAL
  };

static guint find_bar_signals[LAST_SIGNAL] = { 0 };

static void
egg_find_bar_class_init (EggFindBarClass *klass)
{
  GObjectClass *object_class;
  CtkWidgetClass *widget_class;
  CtkBindingSet *binding_set;

  egg_find_bar_parent_class = g_type_class_peek_parent (klass);

  object_class = (GObjectClass *)klass;
  widget_class = (CtkWidgetClass *)klass;

  object_class->set_property = egg_find_bar_set_property;
  object_class->get_property = egg_find_bar_get_property;

  object_class->finalize = egg_find_bar_finalize;

  widget_class->show = egg_find_bar_show;
  widget_class->hide = egg_find_bar_hide;

  widget_class->grab_focus = egg_find_bar_grab_focus;

  find_bar_signals[NEXT] =
    g_signal_new ("next",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (EggFindBarClass, next),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  find_bar_signals[PREVIOUS] =
    g_signal_new ("previous",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (EggFindBarClass, previous),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  find_bar_signals[CLOSE] =
    g_signal_new ("close",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (EggFindBarClass, close),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  find_bar_signals[SCROLL] =
    g_signal_new ("scroll",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (EggFindBarClass, scroll),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__ENUM,
		  G_TYPE_NONE, 1,
		  CTK_TYPE_SCROLL_TYPE);

  /**
   * EggFindBar:search_string:
   *
   * The current string to search for. NULL or empty string
   * both mean no current string.
   *
   */
  g_object_class_install_property (object_class,
				   PROP_SEARCH_STRING,
				   g_param_spec_string ("search_string",
							"Search string",
							"The name of the string to be found",
							NULL,
							G_PARAM_READWRITE));

  /**
   * EggFindBar:case_sensitive:
   *
   * TRUE for a case sensitive search.
   *
   */
  g_object_class_install_property (object_class,
				   PROP_CASE_SENSITIVE,
				   g_param_spec_boolean ("case_sensitive",
                                                         "Case sensitive",
                                                         "TRUE for a case sensitive search",
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  binding_set = ctk_binding_set_by_class (klass);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Escape, 0,
				"close", 0);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Up, 0,
                                "scroll", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_BACKWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Down, 0,
                                "scroll", 1,
                                CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_STEP_FORWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Page_Up, 0,
				"scroll", 1,
				CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_BACKWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Page_Up, 0,
				"scroll", 1,
				CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_BACKWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Page_Down, 0,
				"scroll", 1,
				CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_FORWARD);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_KP_Page_Down, 0,
				"scroll", 1,
				CTK_TYPE_SCROLL_TYPE, CTK_SCROLL_PAGE_FORWARD);

  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Up, CDK_CONTROL_MASK,
                                "previous", 0);
  ctk_binding_entry_add_signal (binding_set, CDK_KEY_Down, CDK_CONTROL_MASK,
                                "next", 0);
}

static void
egg_find_bar_emit_next (EggFindBar *find_bar)
{
  g_signal_emit (find_bar, find_bar_signals[NEXT], 0);
}

static void
egg_find_bar_emit_previous (EggFindBar *find_bar)
{
  g_signal_emit (find_bar, find_bar_signals[PREVIOUS], 0);
}

static void
next_clicked_callback (CtkButton *button,
                       void      *data)
{
  EggFindBar *find_bar = EGG_FIND_BAR (data);

  egg_find_bar_emit_next (find_bar);
}

static void
previous_clicked_callback (CtkButton *button,
                           void      *data)
{
  EggFindBar *find_bar = EGG_FIND_BAR (data);

  egg_find_bar_emit_previous (find_bar);
}

static void
case_sensitive_toggled_callback (CtkToggleToolButton  *button,
                                 void                 *data)
{
  EggFindBar *find_bar = EGG_FIND_BAR (data);

  egg_find_bar_set_case_sensitive (find_bar,
                                   ctk_toggle_tool_button_get_active (button));
}

static void
entry_activate_callback (CtkEntry *entry,
                          void     *data)
{
  EggFindBar *find_bar = EGG_FIND_BAR (data);

  if (find_bar->priv->search_string != NULL)
    egg_find_bar_emit_next (find_bar);
}

static void
entry_changed_callback (CtkEntry *entry,
                        void     *data)
{
  EggFindBar *find_bar = EGG_FIND_BAR (data);
  char *text;

  /* paranoid strdup because set_search_string() sets
   * the entry text
   */
  text = g_strdup (ctk_entry_get_text (entry));

  egg_find_bar_set_search_string (find_bar, text);

  g_free (text);
}

static void
set_focus_cb (CtkWidget *window,
	      CtkWidget *widget,
	      EggFindBar *bar)
{
  CtkWidget *wbar = CTK_WIDGET (bar);

  while (widget != NULL && widget != wbar)
    {
      widget = ctk_widget_get_parent (widget);
    }

  /* if widget == bar, the new focus widget is in the bar, so we
   * don't deactivate.
   */
  if (widget != wbar)
    {
      g_signal_emit (bar, find_bar_signals[CLOSE], 0);
    }
}

static void
egg_find_bar_init (EggFindBar *find_bar)
{
  EggFindBarPrivate *priv;
  CtkWidget *label;
  CtkWidget *box;
  CtkToolItem *item;
  CtkWidget *arrow;

  /* Data */
  priv = egg_find_bar_get_instance_private (find_bar);

  find_bar->priv = priv;
  priv->search_string = NULL;

  ctk_toolbar_set_style (CTK_TOOLBAR (find_bar), CTK_TOOLBAR_BOTH_HORIZ);

  /* Find: |_____| */
  item = ctk_tool_item_new ();
  box = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);

  label = ctk_label_new_with_mnemonic (_("Find:"));
  ctk_widget_set_halign (label, CTK_ALIGN_START);
  ctk_widget_set_hexpand (label, TRUE);
  ctk_widget_set_margin_start (label, 2);
  ctk_widget_set_margin_top (label, 4);
  ctk_widget_set_margin_bottom (label, 4);
  ctk_widget_show (label);

  priv->find_entry = ctk_entry_new ();
  ctk_entry_set_width_chars (CTK_ENTRY (priv->find_entry), 32);
  ctk_entry_set_max_length (CTK_ENTRY (priv->find_entry), 512);
  ctk_widget_set_halign (priv->find_entry, CTK_ALIGN_START);
  ctk_widget_set_hexpand (priv->find_entry, TRUE);
  ctk_widget_set_margin_end (priv->find_entry, 2);
  ctk_widget_set_margin_top (priv->find_entry, 4);
  ctk_widget_set_margin_bottom (priv->find_entry, 4);
  ctk_label_set_mnemonic_widget (CTK_LABEL (label), priv->find_entry);
  ctk_widget_show (priv->find_entry);

  /* Prev */
  arrow = ctk_image_new_from_icon_name ("pan-start-symbolic", CTK_ICON_SIZE_BUTTON);
  priv->previous_button = ctk_tool_button_new (arrow, Q_("Find Pre_vious"));
  ctk_tool_button_set_use_underline (CTK_TOOL_BUTTON (priv->previous_button), TRUE);
  ctk_tool_item_set_is_important (priv->previous_button, TRUE);
  ctk_widget_set_tooltip_text (CTK_WIDGET (priv->previous_button),
			       _("Find previous occurrence of the search string"));

  /* Next */
  arrow = ctk_image_new_from_icon_name ("pan-end-symbolic", CTK_ICON_SIZE_BUTTON);
  priv->next_button = ctk_tool_button_new (arrow, Q_("Find Ne_xt"));
  ctk_tool_button_set_use_underline (CTK_TOOL_BUTTON (priv->next_button), TRUE);
  ctk_tool_item_set_is_important (priv->next_button, TRUE);
  ctk_widget_set_tooltip_text (CTK_WIDGET (priv->next_button),
			       _("Find next occurrence of the search string"));

  /* Separator*/
  priv->status_separator = ctk_separator_tool_item_new();

  /* Case button */
  priv->case_button = ctk_toggle_tool_button_new ();
  g_object_set (G_OBJECT (priv->case_button), "label", _("Case Sensitive"), NULL);
  ctk_tool_item_set_is_important (priv->case_button, TRUE);
  ctk_widget_set_tooltip_text (CTK_WIDGET (priv->case_button),
			       _("Toggle case sensitive search"));

  /* Status */
  priv->status_item = ctk_tool_item_new();
  ctk_tool_item_set_expand (priv->status_item, TRUE);
  priv->status_label = ctk_label_new (NULL);
  ctk_label_set_ellipsize (CTK_LABEL (priv->status_label),
                           PANGO_ELLIPSIZE_END);
  ctk_label_set_xalign (CTK_LABEL (priv->status_label), 0.0);

  g_signal_connect (priv->find_entry, "changed",
                    G_CALLBACK (entry_changed_callback),
                    find_bar);
  g_signal_connect (priv->find_entry, "activate",
                    G_CALLBACK (entry_activate_callback),
                    find_bar);
  g_signal_connect (priv->next_button, "clicked",
                    G_CALLBACK (next_clicked_callback),
                    find_bar);
  g_signal_connect (priv->previous_button, "clicked",
                    G_CALLBACK (previous_clicked_callback),
                    find_bar);
  g_signal_connect (priv->case_button, "toggled",
                    G_CALLBACK (case_sensitive_toggled_callback),
                    find_bar);

  ctk_box_pack_start (CTK_BOX (box), label, FALSE, FALSE, 0);
  ctk_box_pack_start (CTK_BOX (box), priv->find_entry, TRUE, TRUE, 0);
  ctk_container_add (CTK_CONTAINER (item), box);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), item, -1);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), priv->previous_button, -1);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), priv->next_button, -1);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), priv->case_button, -1);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), priv->status_separator, -1);
  ctk_container_add  (CTK_CONTAINER (priv->status_item), priv->status_label);
  ctk_toolbar_insert (CTK_TOOLBAR (find_bar), priv->status_item, -1);

  /* don't show status separator/label until they are set */

  ctk_widget_show_all (CTK_WIDGET (item));
  ctk_widget_show_all (CTK_WIDGET (priv->next_button));
  ctk_widget_show_all (CTK_WIDGET (priv->previous_button));
  ctk_widget_show_all (CTK_WIDGET (priv->case_button));
  ctk_widget_show (priv->status_label);
}

static void
egg_find_bar_finalize (GObject *object)
{
  EggFindBar *find_bar = EGG_FIND_BAR (object);
  EggFindBarPrivate *priv = (EggFindBarPrivate *)find_bar->priv;

  g_free (priv->search_string);

  G_OBJECT_CLASS (egg_find_bar_parent_class)->finalize (object);
}

static void
egg_find_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  EggFindBar *find_bar = EGG_FIND_BAR (object);

  switch (prop_id)
    {
    case PROP_SEARCH_STRING:
      egg_find_bar_set_search_string (find_bar, g_value_get_string (value));
      break;
    case PROP_CASE_SENSITIVE:
      egg_find_bar_set_case_sensitive (find_bar, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_find_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  EggFindBar *find_bar = EGG_FIND_BAR (object);
  EggFindBarPrivate *priv = (EggFindBarPrivate *)find_bar->priv;

  switch (prop_id)
    {
    case PROP_SEARCH_STRING:
      g_value_set_string (value, priv->search_string);
      break;
    case PROP_CASE_SENSITIVE:
      g_value_set_boolean (value, priv->case_sensitive);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_find_bar_show (CtkWidget *widget)
{
  EggFindBar *bar = EGG_FIND_BAR (widget);
  EggFindBarPrivate *priv = bar->priv;

  CTK_WIDGET_CLASS (egg_find_bar_parent_class)->show (widget);

  if (priv->set_focus_handler == 0)
    {
      CtkWidget *toplevel;

      toplevel = ctk_widget_get_toplevel (widget);

      priv->set_focus_handler =
	g_signal_connect (toplevel, "set-focus",
			  G_CALLBACK (set_focus_cb), bar);
    }
}

static void
egg_find_bar_hide (CtkWidget *widget)
{
  EggFindBar *bar = EGG_FIND_BAR (widget);
  EggFindBarPrivate *priv = bar->priv;

  if (priv->set_focus_handler != 0)
    {
      CtkWidget *toplevel;

      toplevel = ctk_widget_get_toplevel (widget);

      g_signal_handlers_disconnect_by_func
	(toplevel, (void (*)) G_CALLBACK (set_focus_cb), bar);
      priv->set_focus_handler = 0;
    }

  CTK_WIDGET_CLASS (egg_find_bar_parent_class)->hide (widget);
}

void
egg_find_bar_grab_focus (CtkWidget *widget)
{
  EggFindBar *find_bar = EGG_FIND_BAR (widget);
  EggFindBarPrivate *priv = find_bar->priv;

  ctk_widget_grab_focus (priv->find_entry);
}

/**
 * egg_find_bar_new:
 *
 * Creates a new #EggFindBar.
 *
 * Returns: a newly created #EggFindBar
 *
 * Since: 2.6
 */
CtkWidget *
egg_find_bar_new (void)
{
  EggFindBar *find_bar;

  find_bar = g_object_new (EGG_TYPE_FIND_BAR, NULL);

  return CTK_WIDGET (find_bar);
}

/**
 * egg_find_bar_set_search_string:
 *
 * Sets the string that should be found/highlighted in the document.
 * Empty string is converted to NULL.
 *
 * Since: 2.6
 */
void
egg_find_bar_set_search_string  (EggFindBar *find_bar,
                                 const char *search_string)
{
  EggFindBarPrivate *priv;

  g_return_if_fail (EGG_IS_FIND_BAR (find_bar));

  priv = (EggFindBarPrivate *)find_bar->priv;

  g_object_freeze_notify (G_OBJECT (find_bar));

  if (priv->search_string != search_string)
    {
      char *old;

      old = priv->search_string;

      if (search_string && *search_string == '\0')
        search_string = NULL;

      /* Only update if the string has changed; setting the entry
       * will emit changed on the entry which will re-enter
       * this function, but we'll handle that fine with this
       * short-circuit.
       */
      if ((old && search_string == NULL) ||
          (old == NULL && search_string) ||
          (old && search_string &&
           strcmp (old, search_string) != 0))
        {
	  gboolean not_empty;

          priv->search_string = g_strdup (search_string);
          g_free (old);

          ctk_entry_set_text (CTK_ENTRY (priv->find_entry),
                              priv->search_string ?
                              priv->search_string :
                              "");

          not_empty = (search_string == NULL) ? FALSE : TRUE;

          ctk_widget_set_sensitive (CTK_WIDGET (find_bar->priv->next_button), not_empty);
          ctk_widget_set_sensitive (CTK_WIDGET (find_bar->priv->previous_button), not_empty);

          g_object_notify (G_OBJECT (find_bar),
                           "search_string");
        }
    }

  g_object_thaw_notify (G_OBJECT (find_bar));
}


/**
 * egg_find_bar_get_search_string:
 *
 * Gets the string that should be found/highlighted in the document.
 *
 * Returns: the string
 *
 * Since: 2.6
 */
const char*
egg_find_bar_get_search_string  (EggFindBar *find_bar)
{
  EggFindBarPrivate *priv;

  g_return_val_if_fail (EGG_IS_FIND_BAR (find_bar), NULL);

  priv = find_bar->priv;

  return priv->search_string ? priv->search_string : "";
}

/**
 * egg_find_bar_set_case_sensitive:
 *
 * Sets whether the search is case sensitive
 *
 * Since: 2.6
 */
void
egg_find_bar_set_case_sensitive (EggFindBar *find_bar,
                                 gboolean    case_sensitive)
{
  EggFindBarPrivate *priv;

  g_return_if_fail (EGG_IS_FIND_BAR (find_bar));

  priv = (EggFindBarPrivate *)find_bar->priv;

  g_object_freeze_notify (G_OBJECT (find_bar));

  case_sensitive = case_sensitive != FALSE;

  if (priv->case_sensitive != case_sensitive)
    {
      priv->case_sensitive = case_sensitive;

      ctk_toggle_tool_button_set_active (CTK_TOGGLE_TOOL_BUTTON (priv->case_button),
                                         priv->case_sensitive);

      g_object_notify (G_OBJECT (find_bar),
                       "case_sensitive");
    }

  g_object_thaw_notify (G_OBJECT (find_bar));
}

/**
 * egg_find_bar_get_case_sensitive:
 *
 * Gets whether the search is case sensitive
 *
 * Returns: TRUE if it's case sensitive
 *
 * Since: 2.6
 */
gboolean
egg_find_bar_get_case_sensitive (EggFindBar *find_bar)
{
  EggFindBarPrivate *priv;

  g_return_val_if_fail (EGG_IS_FIND_BAR (find_bar), FALSE);

  priv = (EggFindBarPrivate *)find_bar->priv;

  return priv->case_sensitive;
}

/**
 * egg_find_bar_set_status_text:
 *
 * Sets some text to display if there's space; typical text would
 * be something like "5 results on this page" or "No results"
 *
 * @text: the text to display
 *
 * Since: 2.6
 */
void
egg_find_bar_set_status_text (EggFindBar *find_bar,
                              const char *text)
{
  EggFindBarPrivate *priv;

  g_return_if_fail (EGG_IS_FIND_BAR (find_bar));

  priv = (EggFindBarPrivate *)find_bar->priv;

  ctk_label_set_text (CTK_LABEL (priv->status_label), text);
  g_object_set (priv->status_separator, "visible", text != NULL && *text != '\0', NULL);
  g_object_set (priv->status_item, "visible", text != NULL && *text !='\0', NULL);
}
