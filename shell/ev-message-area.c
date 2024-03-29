/* ev-message-area.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2007 Carlos Garcia Campos
 *
 * Author:
 *   Carlos Garcia Campos <carlosgc@gnome.org>
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

#include "ev-message-area.h"

struct _EvMessageAreaPrivate {
	CtkWidget *main_box;
	CtkWidget *image;
	CtkWidget *label;
	CtkWidget *secondary_label;

	guint      message_type : 3;
};

enum {
	PROP_0,
	PROP_TEXT,
	PROP_SECONDARY_TEXT,
	PROP_IMAGE
};

static void ev_message_area_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec);
static void ev_message_area_get_property (GObject      *object,
					  guint         prop_id,
					  GValue       *value,
					  GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (EvMessageArea, ev_message_area, CTK_TYPE_INFO_BAR)

static void
ev_message_area_class_init (EvMessageAreaClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->set_property = ev_message_area_set_property;
	gobject_class->get_property = ev_message_area_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
							      "Text",
							      "The primary text of the message dialog",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_SECONDARY_TEXT,
					 g_param_spec_string ("secondary-text",
							      "Secondary Text",
							      "The secondary text of the message dialog",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_IMAGE,
					 g_param_spec_object ("image",
							      "Image",
							      "The image",
							      CTK_TYPE_WIDGET,
							      G_PARAM_READWRITE));
}

static void
ev_message_area_init (EvMessageArea *area)
{
	CtkWidget *hbox, *vbox;
	CtkWidget *content_area;

	area->priv = ev_message_area_get_instance_private (area);

	area->priv->main_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 12);

	area->priv->label = ctk_label_new (NULL);
	ctk_label_set_use_markup (CTK_LABEL (area->priv->label), TRUE);
	ctk_label_set_line_wrap (CTK_LABEL (area->priv->label), TRUE);
	ctk_label_set_selectable (CTK_LABEL (area->priv->label), TRUE);
	ctk_label_set_xalign (CTK_LABEL (area->priv->label), 0.0);
	ctk_widget_set_can_focus (area->priv->label, TRUE);
	ctk_box_pack_start (CTK_BOX (vbox), area->priv->label, TRUE, TRUE, 0);
	ctk_widget_show (area->priv->label);

	area->priv->secondary_label = ctk_label_new (NULL);
	ctk_label_set_use_markup (CTK_LABEL (area->priv->secondary_label), TRUE);
	ctk_label_set_line_wrap (CTK_LABEL (area->priv->secondary_label), TRUE);
	ctk_label_set_selectable (CTK_LABEL (area->priv->secondary_label), TRUE);
	ctk_label_set_xalign (CTK_LABEL (area->priv->secondary_label), 0.0);
	ctk_widget_set_can_focus (area->priv->secondary_label, TRUE);
	ctk_box_pack_start (CTK_BOX (vbox), area->priv->secondary_label, TRUE, TRUE, 0);

	area->priv->image = ctk_image_new_from_icon_name (NULL, CTK_ICON_SIZE_DIALOG);
	ctk_widget_set_halign (area->priv->image, CTK_ALIGN_CENTER);
	ctk_widget_set_valign (area->priv->image, CTK_ALIGN_START);
	ctk_box_pack_start (CTK_BOX (hbox), area->priv->image, FALSE, FALSE, 0);
	ctk_widget_show (area->priv->image);

	ctk_box_pack_start (CTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	ctk_widget_show (vbox);

	ctk_box_pack_start (CTK_BOX (area->priv->main_box), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	content_area = ctk_info_bar_get_content_area (CTK_INFO_BAR (area));
	ctk_container_add (CTK_CONTAINER (content_area), area->priv->main_box);
	ctk_widget_show (area->priv->main_box);
}

static void
ev_message_area_set_image_for_type (EvMessageArea *area,
				    CtkMessageType type)
{
	const gchar *icon_name = NULL;
	AtkObject   *atk_obj;

	switch (type) {
	case CTK_MESSAGE_INFO:
		icon_name = "dialog-information";
		break;
	case CTK_MESSAGE_QUESTION:
		icon_name = "dialog-question";
		break;
	case CTK_MESSAGE_WARNING:
		icon_name = "dialog-warning";
		break;
	case CTK_MESSAGE_ERROR:
		icon_name = "dialog-error";
		break;
	case CTK_MESSAGE_OTHER:
		break;
	default:
		g_warning ("Unknown CtkMessageType %u", type);
		break;
	}

	if (icon_name)
		ctk_image_set_from_icon_name (CTK_IMAGE (area->priv->image), icon_name,
					  CTK_ICON_SIZE_DIALOG);

	atk_obj = ctk_widget_get_accessible (CTK_WIDGET (area));
	if (CTK_IS_ACCESSIBLE (atk_obj)) {
		atk_object_set_role (atk_obj, ATK_ROLE_ALERT);
		if (icon_name) {
			atk_object_set_name (atk_obj, icon_name);
		}
	}
}

static void
ev_message_area_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	EvMessageArea *area = EV_MESSAGE_AREA (object);

	switch (prop_id) {
	case PROP_TEXT:
		ev_message_area_set_text (area, g_value_get_string (value));
		break;
	case PROP_SECONDARY_TEXT:
		ev_message_area_set_secondary_text (area, g_value_get_string (value));
		break;
	case PROP_IMAGE:
		ev_message_area_set_image (area, (CtkWidget *)g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
ev_message_area_get_property (GObject     *object,
			      guint        prop_id,
			      GValue      *value,
			      GParamSpec  *pspec)
{
	EvMessageArea *area = EV_MESSAGE_AREA (object);

	switch (prop_id) {
	case PROP_TEXT:
		g_value_set_string (value, ctk_label_get_label (CTK_LABEL (area->priv->label)));
		break;
	case PROP_SECONDARY_TEXT:
		g_value_set_string (value, ctk_label_get_label (CTK_LABEL (area->priv->secondary_label)));
		break;
	case PROP_IMAGE:
		g_value_set_object (value, area->priv->image);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

void
_ev_message_area_add_buttons_valist (EvMessageArea *area,
				     const gchar   *first_button_text,
				     va_list        args)
{
	const gchar* text;
	gint response_id;

	if (first_button_text == NULL)
		return;

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		ctk_info_bar_add_button (CTK_INFO_BAR (area), text, response_id);

		text = va_arg (args, gchar*);
		if (text == NULL)
			break;

		response_id = va_arg (args, int);
	}
}

CtkWidget *
_ev_message_area_get_main_box (EvMessageArea *area)
{
	return area->priv->main_box;
}

CtkWidget *
ev_message_area_new (CtkMessageType type,
		     const gchar   *text,
		     const gchar   *first_button_text,
		     ...)
{
	CtkWidget *widget;

	widget = g_object_new (EV_TYPE_MESSAGE_AREA,
			       "message-type", type,
			       "text", text,
			       NULL);
	ev_message_area_set_image_for_type (EV_MESSAGE_AREA (widget), type);
	if (first_button_text) {
		va_list args;

		va_start (args, first_button_text);
		_ev_message_area_add_buttons_valist (EV_MESSAGE_AREA (widget),
						     first_button_text, args);
		va_end (args);
	}

	return widget;
}

void
ev_message_area_set_image (EvMessageArea *area,
			   CtkWidget     *image)
{
	CtkWidget *parent;

	g_return_if_fail (EV_IS_MESSAGE_AREA (area));

	area->priv->message_type = CTK_MESSAGE_OTHER;

	parent = ctk_widget_get_parent (area->priv->image);
	ctk_container_add (CTK_CONTAINER (parent), image);
	ctk_container_remove (CTK_CONTAINER (parent), area->priv->image);
	ctk_box_reorder_child (CTK_BOX (parent), image, 0);

	area->priv->image = image;

	g_object_notify (G_OBJECT (area), "image");
}

void
ev_message_area_set_image_from_stock (EvMessageArea *area,
				      const gchar   *icon_name)
{
	g_return_if_fail (EV_IS_MESSAGE_AREA (area));
	g_return_if_fail (icon_name != NULL);

	ctk_image_set_from_icon_name (CTK_IMAGE (area->priv->image),
				  icon_name,
				  CTK_ICON_SIZE_DIALOG);
}

void
ev_message_area_set_text (EvMessageArea *area,
			  const gchar   *str)
{
	g_return_if_fail (EV_IS_MESSAGE_AREA (area));

	if (str) {
		gchar *msg;

		msg = g_strdup_printf ("<b>%s</b>", str);
		ctk_label_set_markup (CTK_LABEL (area->priv->label), msg);
		g_free (msg);
	} else {
		ctk_label_set_markup (CTK_LABEL (area->priv->label), NULL);
	}

	g_object_notify (G_OBJECT (area), "text");
}

void
ev_message_area_set_secondary_text (EvMessageArea *area,
				    const gchar   *str)
{
	g_return_if_fail (EV_IS_MESSAGE_AREA (area));

	if (str) {
		gchar *msg;

		msg = g_strdup_printf ("<small>%s</small>", str);
		ctk_label_set_markup (CTK_LABEL (area->priv->secondary_label), msg);
		g_free (msg);
		ctk_widget_show (area->priv->secondary_label);
	} else {
		ctk_label_set_markup (CTK_LABEL (area->priv->secondary_label), NULL);
		ctk_widget_hide (area->priv->secondary_label);
	}

	g_object_notify (G_OBJECT (area), "secondary-text");
}
