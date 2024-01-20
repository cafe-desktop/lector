/* ev-loading-message.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2010, 2012 Carlos Garcia Campos <carlosgc@gnome.org>
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
#include "ev-loading-message.h"

#include <string.h>
#include <glib/gi18n.h>

struct _EvLoadingMessage {
        CtkBox     base_instance;

        CtkWidget *spinner;
};

struct _EvLoadingMessageClass {
        CtkBoxClass base_class;
};

G_DEFINE_TYPE (EvLoadingMessage, ev_loading_message, CTK_TYPE_BOX)

static void
ev_loading_message_init (EvLoadingMessage *message)
{
        CtkWidget *label;

        ctk_container_set_border_width (CTK_CONTAINER (message), 10);

        message->spinner = ctk_spinner_new ();
        ctk_box_pack_start (CTK_BOX (message), message->spinner, FALSE, FALSE, 0);
        ctk_widget_show (message->spinner);

        label = ctk_label_new (_("Loading…"));
        ctk_box_pack_start (CTK_BOX (message), label, FALSE, FALSE, 0);
        ctk_widget_show (label);
}

static void
get_widget_padding (CtkWidget *widget,
                    CtkBorder *padding)
{
        CtkStyleContext *context;
        CtkStateFlags state;

        context = ctk_widget_get_style_context (widget);
        state = ctk_style_context_get_state (context);
        ctk_style_context_get_padding (context, state, padding);
}

static void
ev_loading_message_size_allocate (CtkWidget     *widget,
                                  CtkAllocation *allocation)
{
        CtkAllocation child_allocation;
        CtkBorder padding;

        get_widget_padding (widget, &padding);
        child_allocation.y = allocation->x + padding.left;
        child_allocation.x = allocation->y + padding.top;
        child_allocation.width = MAX (1, allocation->width - (padding.left + padding.right));
        child_allocation.height = MAX (1, allocation->height - (padding.top + padding.bottom));

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->size_allocate (widget, &child_allocation);
        ctk_widget_set_allocation (widget, allocation);
}

static void
ev_loading_message_get_preferred_width (CtkWidget *widget,
                                        gint      *minimum_size,
                                        gint      *natural_size)
{
        CtkBorder padding;

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->get_preferred_width (widget, minimum_size, natural_size);

        get_widget_padding (widget, &padding);
        *minimum_size += padding.left + padding.right;
        *natural_size += padding.left + padding.right;
}

static void
ev_loading_message_get_preferred_height (CtkWidget *widget,
                                         gint      *minimum_size,
                                         gint      *natural_size)
{
        CtkBorder padding;

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->get_preferred_height (widget, minimum_size, natural_size);

        get_widget_padding (widget, &padding);
        *minimum_size += padding.top + padding.bottom;
        *natural_size += padding.top + padding.bottom;
}

static gboolean
ev_loading_message_draw (CtkWidget *widget,
                         cairo_t   *cr)
{
        CtkStyleContext *context;
        gint             width, height;

        context = ctk_widget_get_style_context (widget);
        width = ctk_widget_get_allocated_width (widget);
        height = ctk_widget_get_allocated_height (widget);

        ctk_render_background (context, cr, 0, 0, width, height);
        ctk_render_frame (context, cr, 0, 0, width, height);

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->draw (widget, cr);

        return TRUE;
}

static void
ev_loading_message_hide (CtkWidget *widget)
{
        EvLoadingMessage *message = EV_LOADING_MESSAGE (widget);

        ctk_spinner_stop (CTK_SPINNER (message->spinner));

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->hide (widget);
}

static void
ev_loading_message_show (CtkWidget *widget)
{
        EvLoadingMessage *message = EV_LOADING_MESSAGE (widget);

        ctk_spinner_start (CTK_SPINNER (message->spinner));

        CTK_WIDGET_CLASS (ev_loading_message_parent_class)->show (widget);
}

static void
ev_loading_message_class_init (EvLoadingMessageClass *klass)
{
        CtkWidgetClass *ctk_widget_class = CTK_WIDGET_CLASS (klass);

        ctk_widget_class->size_allocate = ev_loading_message_size_allocate;
        ctk_widget_class->get_preferred_width = ev_loading_message_get_preferred_width;
        ctk_widget_class->get_preferred_height = ev_loading_message_get_preferred_height;
        ctk_widget_class->draw = ev_loading_message_draw;
        ctk_widget_class->show = ev_loading_message_show;
        ctk_widget_class->hide = ev_loading_message_hide;
}

/* Public methods */
CtkWidget *
ev_loading_message_new (void)
{
        CtkWidget *message;

        message = g_object_new (EV_TYPE_LOADING_MESSAGE,
                                "orientation", CTK_ORIENTATION_HORIZONTAL,
                                "spacing", 12,
                                NULL);
        return message;
}


