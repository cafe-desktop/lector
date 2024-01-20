/*
 *  Copyright (C) 2003, 2004 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
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
 */

#include "config.h"

#include <glib/gi18n.h>
#include <ctk/ctk.h>

#include "ev-navigation-action.h"
#include "ev-navigation-action-widget.h"


enum
{
	WIDGET_ACTIVATE_LINK,
	WIDGET_N_SIGNALS
};

static guint widget_signals[WIDGET_N_SIGNALS] = {0, };

struct _EvNavigationActionPrivate
{
	EvHistory *history;
};

G_DEFINE_TYPE_WITH_PRIVATE (EvNavigationAction, ev_navigation_action, CTK_TYPE_ACTION)

#define MAX_LABEL_LENGTH 48

static void
ev_navigation_action_history_changed (EvHistory *history,
				      gpointer data)
{
	EvNavigationAction *action = EV_NAVIGATION_ACTION (data);

	ctk_action_set_sensitive (CTK_ACTION (action),
				  ev_history_get_n_links (history) > 0);
}

void
ev_navigation_action_set_history (EvNavigationAction *action,
				  EvHistory	     *history)
{
	action->priv->history = history;

	g_object_add_weak_pointer (G_OBJECT (action->priv->history),
				   (gpointer) &action->priv->history);

	g_signal_connect_object (history, "changed",
				 G_CALLBACK (ev_navigation_action_history_changed),
				 action, 0);
}

static void
activate_menu_item_cb (CtkWidget *widget, EvNavigationAction *action)
{
	int index;

	g_return_if_fail (EV_IS_HISTORY (action->priv->history));

	index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "index"));

	if (action->priv->history) {
		EvLink *link;

		link = ev_history_get_link_nth (action->priv->history, index);

		g_signal_emit (action, widget_signals[WIDGET_ACTIVATE_LINK], 0, link);
	}
}

static CtkWidget *
new_history_menu_item (EvNavigationAction *action,
		       EvLink             *link,
		       int                 index)
{
	CtkLabel *label;
	CtkWidget *item;
	const char *title;

	title = ev_link_get_title (link);
	item = ctk_image_menu_item_new_with_label (title);
	label = CTK_LABEL (ctk_bin_get_child (CTK_BIN (item)));
	ctk_label_set_use_markup (label, TRUE);
	g_object_set_data (G_OBJECT (item), "index",
			   GINT_TO_POINTER (index));

	ctk_label_set_ellipsize (label, PANGO_ELLIPSIZE_END);
	ctk_label_set_max_width_chars (label, MAX_LABEL_LENGTH);

	g_signal_connect (item, "activate",
			  G_CALLBACK (activate_menu_item_cb),
			  action);

	ctk_widget_show (item);

	return item;
}

static CtkWidget *
build_menu (EvNavigationAction *action)
{
	CtkMenuShell *menu;
	CtkWidget *item;
	EvLink *link;
	EvHistory *history = action->priv->history;
	int start, end, i;

	if (history == NULL || ev_history_get_n_links (history) <= 0) {
		return NULL;
	}

	menu = CTK_MENU_SHELL (ctk_menu_new ());

	start = 0;
	end = ev_history_get_n_links (history);

	for (i = start; i < end; i++) {
		link = ev_history_get_link_nth (history, i);
		item = new_history_menu_item (action, link, i);
		ctk_menu_shell_prepend (menu, item);
	}

	return CTK_WIDGET (menu);
}

static void
menu_activated_cb (EvNavigationActionWidget *button,
		   EvNavigationAction *action)
{
	CtkWidget *menu;

	menu = build_menu (action);
	ev_navigation_action_widget_set_menu (button, menu);
}

static void
connect_proxy (CtkAction *action, CtkWidget *proxy)
{
	CtkWidget *menu;

	if (CTK_IS_TOOL_ITEM (proxy)) {
		/* set dummy menu so the arrow gets sensitive */
		menu = ctk_menu_new ();
		ev_navigation_action_widget_set_menu (EV_NAVIGATION_ACTION_WIDGET (proxy), menu);

		g_signal_connect (proxy, "show-menu",
				  G_CALLBACK (menu_activated_cb), action);
	}

	CTK_ACTION_CLASS (ev_navigation_action_parent_class)->connect_proxy (action, proxy);
}

static CtkWidget *
create_tool_item (CtkAction *action)
{
	EvNavigationActionWidget *proxy;

	proxy = g_object_new (EV_TYPE_NAVIGATION_ACTION_WIDGET, NULL);
	ctk_widget_show (CTK_WIDGET (proxy));

	return CTK_WIDGET (proxy);
}

static CtkWidget *
create_menu_item (CtkAction *action)
{
	CtkWidget *menu;
	CtkWidget *menu_item;

	menu = build_menu (EV_NAVIGATION_ACTION (action));

        menu_item = CTK_ACTION_CLASS (ev_navigation_action_parent_class)->create_menu_item (action);

	ctk_menu_item_set_submenu (CTK_MENU_ITEM (menu_item), menu);

	ctk_widget_show (menu_item);

	return menu_item;
}

static void
ev_navigation_action_init (EvNavigationAction *action)
{
	action->priv = ev_navigation_action_get_instance_private (action);
}

static void
ev_navigation_action_finalize (GObject *object)
{
	EvNavigationAction *action = EV_NAVIGATION_ACTION (object);

	if (action->priv->history) {
		g_object_remove_weak_pointer (G_OBJECT (action->priv->history),
				  	     (gpointer) &action->priv->history);
		action->priv->history = NULL;
	}

	G_OBJECT_CLASS (ev_navigation_action_parent_class)->finalize (object);
}

static void
ev_navigation_action_class_init (EvNavigationActionClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	CtkActionClass *action_class = CTK_ACTION_CLASS (class);

	object_class->finalize = ev_navigation_action_finalize;

	action_class->toolbar_item_type = CTK_TYPE_TOOL_ITEM;
	action_class->create_tool_item = create_tool_item;
	action_class->connect_proxy = connect_proxy;
	action_class->create_menu_item = create_menu_item;

	widget_signals[WIDGET_ACTIVATE_LINK] = g_signal_new ("activate_link",
					       G_OBJECT_CLASS_TYPE (object_class),
					       G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
					       G_STRUCT_OFFSET (EvNavigationActionClass, activate_link),
					       NULL, NULL,
					       g_cclosure_marshal_VOID__OBJECT,
					       G_TYPE_NONE, 1,
					       G_TYPE_OBJECT);
}
