/*
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 *  Modified 2005 by James Bowes for use in lector.
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

#include "ephy-zoom-control.h"
#include "ephy-zoom.h"

#include <ctk/ctk.h>
#include <glib/gi18n.h>

struct _EphyZoomControlPrivate
{
	CtkComboBox *combo;
	float zoom;
	float min_zoom;
	float max_zoom;
	guint handler_id;
};

enum
{
	COL_TEXT,
	COL_IS_SEP
};

enum
{
	PROP_0,
	PROP_ZOOM,
	PROP_MIN_ZOOM,
	PROP_MAX_ZOOM
};

enum
{
	ZOOM_TO_LEVEL_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (EphyZoomControl, ephy_zoom_control, CTK_TYPE_TOOL_ITEM)

static void
combo_changed_cb (CtkComboBox *combo, EphyZoomControl *control)
{
	gint index;
	float zoom;

	index = ctk_combo_box_get_active (combo);
	zoom = zoom_levels[index].level;

	if (zoom != control->priv->zoom)
	{
		g_signal_emit (control, signals[ZOOM_TO_LEVEL_SIGNAL], 0, zoom);
	}
}

static void
sync_zoom_cb (EphyZoomControl *control, GParamSpec *pspec, gpointer data)
{
	EphyZoomControlPrivate *p = control->priv;
	guint index;

	index = ephy_zoom_get_zoom_level_index (p->zoom);

	g_signal_handler_block (p->combo, p->handler_id);
	ctk_combo_box_set_active (p->combo, index);
	g_signal_handler_unblock (p->combo, p->handler_id);
}

static void
sync_zoom_max_min_cb (EphyZoomControl *control, GParamSpec *pspec, gpointer data)
{
	EphyZoomControlPrivate *p = control->priv;
	CtkListStore *model = (CtkListStore *)ctk_combo_box_get_model (p->combo);
	CtkTreeIter iter;
	gint i;

	g_signal_handler_block (p->combo, p->handler_id);
	ctk_list_store_clear (model);

	for (i = 0; i < n_zoom_levels; i++)
	{
		if (zoom_levels[i].level > 0) {
			if (zoom_levels[i].level < p->min_zoom)
				continue;

			if (zoom_levels[i].level > p->max_zoom)
				break;
		}

		ctk_list_store_append (model, &iter);

		if (zoom_levels[i].name != NULL) {
			ctk_list_store_set (model, &iter,
					    COL_TEXT, _(zoom_levels[i].name),
					    -1);
		} else {
			ctk_list_store_set (model, &iter,
					    COL_IS_SEP, zoom_levels[i].name == NULL,
					    -1);
		}
	}

	ctk_combo_box_set_active (p->combo, ephy_zoom_get_zoom_level_index (p->zoom));
	g_signal_handler_unblock (p->combo, p->handler_id);
}

static gboolean
row_is_separator (CtkTreeModel *model,
		  CtkTreeIter  *iter,
		  gpointer      data)
{
	gboolean is_sep;
	ctk_tree_model_get (model, iter, COL_IS_SEP, &is_sep, -1);
	return is_sep;
}

static void
ephy_zoom_control_finalize (GObject *o)
{
	EphyZoomControl *control = EPHY_ZOOM_CONTROL (o);

	g_object_unref (control->priv->combo);

	G_OBJECT_CLASS (ephy_zoom_control_parent_class)->finalize (o);
}

static void
ephy_zoom_control_init (EphyZoomControl *control)
{
	EphyZoomControlPrivate *p;
	CtkWidget *vbox;
	CtkCellRenderer *renderer;
	CtkListStore    *store;
	CtkTreeIter      iter;
	guint i;

	p = ephy_zoom_control_get_instance_private (control);
	control->priv = p;

	p->zoom = 1.0;

	store = ctk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN);

	for (i = 0; i < n_zoom_levels; i++)
	{
		ctk_list_store_append (store, &iter);

		if (zoom_levels[i].name != NULL) {
			ctk_list_store_set (store, &iter,
				            COL_TEXT, _(zoom_levels[i].name),
				            -1);
		} else {
			ctk_list_store_set (store, &iter,
				            COL_IS_SEP, zoom_levels[i].name == NULL,
				            -1);
		}
	}

	p->combo = CTK_COMBO_BOX (ctk_combo_box_new_with_model (CTK_TREE_MODEL (store)));
	g_object_unref (store);

	renderer = ctk_cell_renderer_text_new ();
	ctk_cell_layout_pack_start     (CTK_CELL_LAYOUT (p->combo), renderer, TRUE);
	ctk_cell_layout_set_attributes (CTK_CELL_LAYOUT (p->combo), renderer,
					"text", COL_TEXT, NULL);
	ctk_combo_box_set_row_separator_func (p->combo,
					      (CtkTreeViewRowSeparatorFunc) row_is_separator,
					      NULL, NULL);

	ctk_widget_set_focus_on_click (CTK_WIDGET (p->combo), FALSE);
	g_object_ref_sink (G_OBJECT (p->combo));
	ctk_widget_show (CTK_WIDGET (p->combo));

	i = ephy_zoom_get_zoom_level_index (p->zoom);
	ctk_combo_box_set_active (p->combo, i);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);
	ctk_box_set_homogeneous (CTK_BOX (vbox), TRUE);
	ctk_box_pack_start (CTK_BOX (vbox), CTK_WIDGET (p->combo), TRUE, FALSE, 0);
	ctk_widget_show (vbox);

	ctk_container_add (CTK_CONTAINER (control), vbox);

	p->handler_id = g_signal_connect (p->combo, "changed",
					  G_CALLBACK (combo_changed_cb), control);

	g_signal_connect_object (control, "notify::zoom",
				 G_CALLBACK (sync_zoom_cb), NULL, 0);
	g_signal_connect_object (control, "notify::min-zoom",
				 G_CALLBACK (sync_zoom_max_min_cb), NULL, 0);
	g_signal_connect_object (control, "notify::max-zoom",
				 G_CALLBACK (sync_zoom_max_min_cb), NULL, 0);
}

static void
ephy_zoom_control_set_property (GObject *object,
				guint prop_id,
				const GValue *value,
				GParamSpec *pspec)
{
	EphyZoomControl *control;
	EphyZoomControlPrivate *p;

	control = EPHY_ZOOM_CONTROL (object);
	p = control->priv;

	switch (prop_id)
	{
		case PROP_ZOOM:
			p->zoom = g_value_get_float (value);
			break;
		case PROP_MIN_ZOOM:
			p->min_zoom = g_value_get_float (value);
			break;
		case PROP_MAX_ZOOM:
			p->max_zoom = g_value_get_float (value);
			break;
	}
}

static void
ephy_zoom_control_get_property (GObject *object,
				guint prop_id,
				GValue *value,
				GParamSpec *pspec)
{
	EphyZoomControl *control;
	EphyZoomControlPrivate *p;

	control = EPHY_ZOOM_CONTROL (object);
	p = control->priv;

	switch (prop_id)
	{
		case PROP_ZOOM:
			g_value_set_float (value, p->zoom);
			break;
		case PROP_MIN_ZOOM:
			g_value_set_float (value, p->min_zoom);
			break;
		case PROP_MAX_ZOOM:
			g_value_set_float (value, p->max_zoom);
			break;
	}
}

static void
ephy_zoom_control_class_init (EphyZoomControlClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;

	object_class->set_property = ephy_zoom_control_set_property;
	object_class->get_property = ephy_zoom_control_get_property;
	object_class->finalize = ephy_zoom_control_finalize;

	g_object_class_install_property (object_class,
					 PROP_ZOOM,
					 g_param_spec_float ("zoom",
							     "Zoom",
							     "Zoom level to display in the item.",
							     ZOOM_MINIMAL,
							     ZOOM_MAXIMAL,
							     1.0,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MIN_ZOOM,
					 g_param_spec_float ("min-zoom",
							     "MinZoom",
							     "The minimum zoom",
							     ZOOM_MINIMAL,
							     ZOOM_MAXIMAL,
							     ZOOM_MINIMAL,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_MAX_ZOOM,
					 g_param_spec_float ("max-zoom",
							     "MaxZoom",
							     "The maximum zoom",
							     ZOOM_MINIMAL,
							     ZOOM_MAXIMAL,
							     ZOOM_MAXIMAL,
							     G_PARAM_READWRITE));

	signals[ZOOM_TO_LEVEL_SIGNAL] =
		g_signal_new ("zoom_to_level",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EphyZoomControlClass,
					       zoom_to_level),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_FLOAT);
}

void
ephy_zoom_control_set_zoom_level (EphyZoomControl *control, float zoom)
{
	g_return_if_fail (EPHY_IS_ZOOM_CONTROL (control));

	if (zoom < ZOOM_MINIMAL || zoom > ZOOM_MAXIMAL) return;

	control->priv->zoom = zoom;
	g_object_notify (G_OBJECT (control), "zoom");
}

float
ephy_zoom_control_get_zoom_level (EphyZoomControl *control)
{
	g_return_val_if_fail (EPHY_IS_ZOOM_CONTROL (control), 1.0);

	return control->priv->zoom;
}
