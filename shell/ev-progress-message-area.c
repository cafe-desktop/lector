/* ev-progress-message-area.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
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

#include <config.h>

#include <ctk/ctk.h>

#include "ev-progress-message-area.h"

struct _EvProgressMessageAreaPrivate {
	CtkWidget *label;
	CtkWidget *progress_bar;
};

enum {
	PROP_0,
	PROP_STATUS,
	PROP_FRACTION
};

static void ev_progress_message_area_set_property (GObject      *object,
						   guint         prop_id,
						   const GValue *value,
						   GParamSpec   *pspec);
static void ev_progress_message_area_get_property (GObject      *object,
						   guint         prop_id,
						   GValue       *value,
						   GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (EvProgressMessageArea, ev_progress_message_area, EV_TYPE_MESSAGE_AREA)

static void
ev_progress_message_area_class_init (EvProgressMessageAreaClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->set_property = ev_progress_message_area_set_property;
	gobject_class->get_property = ev_progress_message_area_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_STATUS,
					 g_param_spec_string ("status",
							      "Status",
							      "The status text of the progress area",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_FRACTION,
					 g_param_spec_double ("fraction",
							      "Fraction",
							      "The fraction of total work that has been completed",
							      0.0, 1.0, 0.0,
							      G_PARAM_READWRITE));
}

static void
ev_progress_message_area_init (EvProgressMessageArea *area)
{
	CtkWidget *contents;
	CtkWidget *vbox;

	area->priv = ev_progress_message_area_get_instance_private (area);

	contents = _ev_message_area_get_main_box (EV_MESSAGE_AREA (area));

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);

	area->priv->label = ctk_label_new (NULL);
	ctk_label_set_use_markup (CTK_LABEL (area->priv->label), TRUE);
	ctk_label_set_ellipsize (CTK_LABEL (area->priv->label),
				 PANGO_ELLIPSIZE_END);
	ctk_label_set_xalign (CTK_LABEL (area->priv->label), 0.0);
	ctk_box_pack_start (CTK_BOX (vbox), area->priv->label, TRUE, TRUE, 0);
	ctk_widget_show (area->priv->label);

	area->priv->progress_bar = ctk_progress_bar_new ();
	ctk_widget_set_size_request (area->priv->progress_bar, -1, 15);
	ctk_box_pack_start (CTK_BOX (vbox), area->priv->progress_bar, TRUE, FALSE, 0);
	ctk_widget_show (area->priv->progress_bar);

	ctk_box_pack_start (CTK_BOX (contents), vbox, TRUE, TRUE, 0);
	ctk_widget_show (vbox);
}

static void
ev_progress_message_area_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
	EvProgressMessageArea *area = EV_PROGRESS_MESSAGE_AREA (object);

	switch (prop_id) {
	case PROP_STATUS:
		ev_progress_message_area_set_status (area, g_value_get_string (value));
		break;
	case PROP_FRACTION:
		ev_progress_message_area_set_fraction (area, g_value_get_double (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
ev_progress_message_area_get_property (GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec)
{
	EvProgressMessageArea *area = EV_PROGRESS_MESSAGE_AREA (object);

	switch (prop_id) {
	case PROP_STATUS:
		g_value_set_string (value, ctk_label_get_label (CTK_LABEL (area->priv->label)));
		break;
	case PROP_FRACTION: {
		gdouble fraction;

		fraction = ctk_progress_bar_get_fraction (CTK_PROGRESS_BAR (area->priv->progress_bar));
		g_value_set_double (value, fraction);
	}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

CtkWidget *
ev_progress_message_area_new (const gchar *stock_id,
			      const gchar *text,
			      const gchar *first_button_text,
			      ...)
{
	CtkWidget *widget;

	widget = g_object_new (EV_TYPE_PROGRESS_MESSAGE_AREA,
			       "message-type", CTK_MESSAGE_OTHER,
			       "text", text,
			       NULL);
	if (first_button_text) {
		va_list args;

		va_start (args, first_button_text);
		_ev_message_area_add_buttons_valist (EV_MESSAGE_AREA (widget),
						     first_button_text,
						     args);
		va_end (args);
	}

	ev_message_area_set_image_from_stock (EV_MESSAGE_AREA (widget), stock_id);

	return widget;
}

void
ev_progress_message_area_set_status (EvProgressMessageArea *area,
				     const gchar           *str)
{
	g_return_if_fail (EV_IS_PROGRESS_MESSAGE_AREA (area));

	ctk_label_set_text (CTK_LABEL (area->priv->label), str);

	g_object_notify (G_OBJECT (area), "status");
}

void
ev_progress_message_area_set_fraction (EvProgressMessageArea *area,
				       gdouble                fraction)
{
	g_return_if_fail (EV_IS_PROGRESS_MESSAGE_AREA (area));

	ctk_progress_bar_set_fraction (CTK_PROGRESS_BAR (area->priv->progress_bar),
				       fraction);
	g_object_notify (G_OBJECT (area), "fraction");
}
