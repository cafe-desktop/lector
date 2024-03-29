/* ev-annotation-window.c
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
 * Copyright (C) 2007 Iñigo Martinez <inigomartinez@gmail.com>
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

#include <string.h>

#include "ev-annotation-window.h"
#include "ev-stock-icons.h"
#include "ev-view-marshal.h"
#include "ev-document-misc.h"

enum {
	PROP_0,
	PROP_ANNOTATION,
	PROP_PARENT
};

enum {
	CLOSED,
	MOVED,
	N_SIGNALS
};

struct _EvAnnotationWindow {
	CtkWindow     base_instance;

	EvAnnotation *annotation;
	CtkWindow    *parent;

	CtkWidget    *title;
	CtkWidget    *close_button;
	CtkWidget    *text_view;
	CtkWidget    *resize_se;
	CtkWidget    *resize_sw;

	gboolean      is_open;
	EvRectangle   rect;

	gboolean      in_move;
	gint          x;
	gint          y;
	gint          orig_x;
	gint          orig_y;
};

struct _EvAnnotationWindowClass {
	CtkWindowClass base_class;

	void (* closed) (EvAnnotationWindow *window);
	void (* moved)  (EvAnnotationWindow *window,
			 gint                x,
			 gint                y);
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (EvAnnotationWindow, ev_annotation_window, CTK_TYPE_WINDOW)

/* Cut and paste from ctkwindow.c */
static void
send_focus_change (CtkWidget *widget,
		   gboolean   in)
{
	CdkEvent *fevent = cdk_event_new (CDK_FOCUS_CHANGE);

	fevent->focus_change.type = CDK_FOCUS_CHANGE;
	fevent->focus_change.window = ctk_widget_get_window (widget);
	fevent->focus_change.in = in;
	if (fevent->focus_change.window)
		g_object_ref (fevent->focus_change.window);

	ctk_widget_send_focus_change (widget, fevent);

	cdk_event_free (fevent);
}

static gdouble
get_monitor_dpi (EvAnnotationWindow *window)
{
	CdkWindow  *cdk_window;
	CdkMonitor *monitor;
	CdkDisplay *display;

	cdk_window = ctk_widget_get_window (CTK_WIDGET (window));
	display = cdk_window_get_display (cdk_window);
	monitor = cdk_display_get_monitor_at_window (display, cdk_window);

	return ev_document_misc_get_monitor_dpi (monitor);
}

static void
ev_annotation_window_sync_contents (EvAnnotationWindow *window)
{
	gchar         *contents;
	CtkTextIter    start, end;
	CtkTextBuffer *buffer;
	EvAnnotation  *annot = window->annotation;

	buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (window->text_view));
	ctk_text_buffer_get_bounds (buffer, &start, &end);
	contents = ctk_text_buffer_get_text (buffer, &start, &end, FALSE);
	ev_annotation_set_contents (annot, contents);
	g_free (contents);
}

static void
ev_annotation_window_set_color (EvAnnotationWindow *window,
                                CdkRGBA           *color)
{
        CtkStyleProperties *properties;
        CtkStyleProvider   *provider;

        properties = ctk_style_properties_new ();
        ctk_style_properties_set (properties, 0,
                                  "background-color", color,
                                  NULL);

        provider = CTK_STYLE_PROVIDER (properties);
        ctk_style_context_add_provider (ctk_widget_get_style_context (CTK_WIDGET (window)),
                                        provider, CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        ctk_style_context_add_provider (ctk_widget_get_style_context (window->close_button),
                                        provider, CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        ctk_style_context_add_provider (ctk_widget_get_style_context (window->resize_se),
                                        provider, CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        ctk_style_context_add_provider (ctk_widget_get_style_context (window->resize_sw),
                                        provider, CTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref (properties);
}

static void
ev_annotation_window_set_opacity (EvAnnotationWindow *window,
                                  gdouble             opacity)
{
	ctk_widget_set_opacity (CTK_WIDGET (window), opacity);
	ctk_widget_set_opacity (CTK_WIDGET (window->text_view), opacity);
}

static void
ev_annotation_window_label_changed (EvAnnotationMarkup *annot,
				    GParamSpec         *pspec,
				    EvAnnotationWindow *window)
{
	const gchar *label = ev_annotation_markup_get_label (annot);

	ctk_window_set_title (CTK_WINDOW (window), label);
	ctk_label_set_text (CTK_LABEL (window->title), label);
}

static void
ev_annotation_window_color_changed (EvAnnotation       *annot,
				    GParamSpec         *pspec,
				    EvAnnotationWindow *window)
{
	CdkRGBA rgba;

	ev_annotation_get_rgba (annot, &rgba);
	ev_annotation_window_set_color (window, &rgba);
}

static void
ev_annotation_window_opacity_changed (EvAnnotation       *annot,
                                      GParamSpec         *pspec,
                                      EvAnnotationWindow *window)
{
	gdouble opacity;

	opacity = ev_annotation_markup_get_opacity (EV_ANNOTATION_MARKUP (annot));
	ev_annotation_window_set_opacity (window, opacity);
}

static void
ev_annotation_window_dispose (GObject *object)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (object);

	if (window->annotation) {
		ev_annotation_window_sync_contents (window);
		g_object_unref (window->annotation);
		window->annotation = NULL;
	}

	(* G_OBJECT_CLASS (ev_annotation_window_parent_class)->dispose) (object);
}

static void
ev_annotation_window_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (object);

	switch (prop_id) {
	case PROP_ANNOTATION:
		window->annotation = g_value_dup_object (value);
		break;
	case PROP_PARENT:
		window->parent = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static gboolean
ev_annotation_window_resize (EvAnnotationWindow *window,
			     CdkEventButton     *event,
			     CtkWidget          *ebox)
{
	if (event->type == CDK_BUTTON_PRESS && event->button == 1) {
		ctk_window_begin_resize_drag (CTK_WINDOW (window),
					      window->resize_sw == ebox ?
					      CDK_WINDOW_EDGE_SOUTH_WEST :
					      CDK_WINDOW_EDGE_SOUTH_EAST,
					      event->button, event->x_root,
					      event->y_root, event->time);
		return TRUE;
	}

	return FALSE;
}

static void
ev_annotation_window_set_resize_cursor (CtkWidget          *widget,
					EvAnnotationWindow *window)
{
	CdkWindow *cdk_window = ctk_widget_get_window (widget);

	if (!cdk_window)
		return;

	if (ctk_widget_is_sensitive (widget)) {
		CdkDisplay *display = ctk_widget_get_display (widget);
		CdkCursor  *cursor;

		cursor = cdk_cursor_new_for_display (display,
						     widget == window->resize_sw ?
						     CDK_BOTTOM_LEFT_CORNER :
						     CDK_BOTTOM_RIGHT_CORNER);
		cdk_window_set_cursor (cdk_window, cursor);
		g_object_unref (cursor);
	} else {
		cdk_window_set_cursor (cdk_window, NULL);
	}
}

static void
text_view_state_flags_changed (CtkWidget     *widget,
                               CtkStateFlags  previous_flags)
{
	CtkStateFlags current_flags = ctk_widget_get_state_flags (widget);

	if (current_flags & CTK_STATE_FLAG_BACKDROP)
	    ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (widget), FALSE);
}

static void
ev_annotation_window_close (EvAnnotationWindow *window)
{
	ctk_widget_hide (CTK_WIDGET (window));
	g_signal_emit (window, signals[CLOSED], 0);
}

static gboolean
ev_annotation_window_button_press_event (CtkWidget      *widget,
					 CdkEventButton *event)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (widget);

	if (event->type == CDK_BUTTON_PRESS && event->button == 1) {
		window->in_move = TRUE;
		window->x = event->x_root - event->x;
		window->y = event->y_root - event->y;
		ctk_window_begin_move_drag (CTK_WINDOW (widget),
					    event->button,
					    event->x_root,
					    event->y_root,
					    event->time);
		return TRUE;
	}

	return FALSE;
}

static void
ev_annotation_window_init (EvAnnotationWindow *window)
{
	CtkWidget *vbox, *hbox;
	CtkWidget *icon;
	CtkWidget *swindow;
	CtkWidget *header;
	CtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf;

	icon_theme = ctk_icon_theme_get_default ();

	ctk_widget_set_can_focus (CTK_WIDGET (window), TRUE);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	/* Title bar */
	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

	icon = ctk_image_new (); /* FIXME: use the annot icon */
	ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 0);
	ctk_widget_show (icon);

	header = ctk_event_box_new ();
	ctk_widget_add_events (header, CDK_BUTTON_PRESS_MASK);
	g_signal_connect_swapped (header, "button-press-event",
	                          G_CALLBACK (ev_annotation_window_button_press_event),
	                          window);

	window->title = ctk_label_new (NULL);
	ctk_container_add (CTK_CONTAINER (header), window->title);
	ctk_widget_show (window->title);

	ctk_box_pack_start (CTK_BOX (hbox), header, TRUE, TRUE, 0);
	ctk_widget_show (header);

	window->close_button = ctk_button_new ();
	ctk_button_set_relief (CTK_BUTTON (window->close_button), CTK_RELIEF_NONE);
	ctk_container_set_border_width (CTK_CONTAINER (window->close_button), 0);
	g_signal_connect_swapped (window->close_button, "clicked",
				  G_CALLBACK (ev_annotation_window_close),
				  window);
	pixbuf = ctk_icon_theme_load_icon (icon_theme, EV_STOCK_CLOSE, 8, CTK_ICON_LOOKUP_FORCE_SIZE, NULL);
	icon = ctk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	ctk_container_add (CTK_CONTAINER (window->close_button), icon);
	ctk_widget_show (icon);

	ctk_box_pack_start (CTK_BOX (hbox), window->close_button, FALSE, FALSE, 0);
	ctk_widget_show (window->close_button);

	ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	ctk_widget_show (hbox);

	/* Contents */
	swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (swindow),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);
	window->text_view = ctk_text_view_new ();
	ctk_text_view_set_wrap_mode (CTK_TEXT_VIEW (window->text_view), CTK_WRAP_WORD);
	g_signal_connect (window->text_view, "state-flags-changed",
	                  G_CALLBACK (text_view_state_flags_changed),
	                  window);
	ctk_container_add (CTK_CONTAINER (swindow), window->text_view);
	ctk_widget_show (window->text_view);

	ctk_box_pack_start (CTK_BOX (vbox), swindow, TRUE, TRUE, 0);
	ctk_widget_show (swindow);

	/* Resize bar */
	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);

	window->resize_sw = ctk_event_box_new ();
	ctk_widget_add_events (window->resize_sw, CDK_BUTTON_PRESS_MASK);
	g_signal_connect_swapped (window->resize_sw, "button-press-event",
				  G_CALLBACK (ev_annotation_window_resize),
				  window);
	g_signal_connect (window->resize_sw, "realize",
			  G_CALLBACK (ev_annotation_window_set_resize_cursor),
			  window);

	pixbuf = ctk_icon_theme_load_icon (icon_theme, EV_STOCK_RESIZE_SW, 8, CTK_ICON_LOOKUP_FORCE_SIZE, NULL);
	icon = ctk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	ctk_container_add (CTK_CONTAINER (window->resize_sw), icon);
	ctk_widget_show (icon);
	ctk_box_pack_start (CTK_BOX (hbox), window->resize_sw, FALSE, FALSE, 0);
	ctk_widget_show (window->resize_sw);

	window->resize_se = ctk_event_box_new ();
	ctk_widget_add_events (window->resize_se, CDK_BUTTON_PRESS_MASK);
	g_signal_connect_swapped (window->resize_se, "button-press-event",
				  G_CALLBACK (ev_annotation_window_resize),
				  window);
	g_signal_connect (window->resize_se, "realize",
			  G_CALLBACK (ev_annotation_window_set_resize_cursor),
			  window);

	pixbuf = ctk_icon_theme_load_icon (icon_theme, EV_STOCK_RESIZE_SE, 8, CTK_ICON_LOOKUP_FORCE_SIZE, NULL);
	icon = ctk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	ctk_container_add (CTK_CONTAINER (window->resize_se), icon);
	ctk_widget_show (icon);
	ctk_box_pack_end (CTK_BOX (hbox), window->resize_se, FALSE, FALSE, 0);
	ctk_widget_show (window->resize_se);

	ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	ctk_widget_show (hbox);

	ctk_container_add (CTK_CONTAINER (window), vbox);
	ctk_widget_show (vbox);

	ctk_widget_add_events (CTK_WIDGET (window),
			       CDK_BUTTON_PRESS_MASK |
			       CDK_KEY_PRESS_MASK);

	ctk_container_set_border_width (CTK_CONTAINER (window), 2);

	ctk_window_set_decorated (CTK_WINDOW (window), FALSE);
	ctk_window_set_skip_taskbar_hint (CTK_WINDOW (window), TRUE);
	ctk_window_set_skip_pager_hint (CTK_WINDOW (window), TRUE);
	ctk_window_set_resizable (CTK_WINDOW (window), TRUE);
}

static GObject *
ev_annotation_window_constructor (GType                  type,
				  guint                  n_construct_properties,
				  GObjectConstructParam *construct_params)
{
	GObject            *object;
	EvAnnotationWindow *window;
	EvAnnotation       *annot;
	EvAnnotationMarkup *markup;
	const gchar        *contents;
	const gchar        *label;
	CdkRGBA             color;
	EvRectangle        *rect;
	gdouble             scale;
	gdouble             opacity;

	object = G_OBJECT_CLASS (ev_annotation_window_parent_class)->constructor (type,
										  n_construct_properties,
										  construct_params);
	window = EV_ANNOTATION_WINDOW (object);
	annot = window->annotation;
	markup = EV_ANNOTATION_MARKUP (annot);

	ctk_window_set_transient_for (CTK_WINDOW (window), window->parent);
	ctk_window_set_destroy_with_parent (CTK_WINDOW (window), FALSE);

	label = ev_annotation_markup_get_label (markup);
	window->is_open = ev_annotation_markup_get_popup_is_open (markup);
	ev_annotation_markup_get_rectangle (markup, &window->rect);

	rect = &window->rect;

	/* Rectangle is at doc resolution (72.0) */
	scale = get_monitor_dpi (window) / 72.0;
	ctk_window_resize (CTK_WINDOW (window),
			   (gint)((rect->x2 - rect->x1) * scale),
			   (gint)((rect->y2 - rect->y1) * scale));

	ev_annotation_get_rgba (annot, &color);
	ev_annotation_window_set_color (window, &color);

	opacity = ev_annotation_markup_get_opacity (markup);
	ev_annotation_window_set_opacity (window, opacity);

	ctk_widget_set_name (CTK_WIDGET (window), ev_annotation_get_name (annot));
	ctk_window_set_title (CTK_WINDOW (window), label);
	ctk_label_set_text (CTK_LABEL (window->title), label);

	contents = ev_annotation_get_contents (annot);
	if (contents) {
		CtkTextBuffer *buffer;

		buffer = ctk_text_view_get_buffer (CTK_TEXT_VIEW (window->text_view));
		ctk_text_buffer_set_text (buffer, contents, -1);
	}

	g_signal_connect (annot, "notify::label",
			  G_CALLBACK (ev_annotation_window_label_changed),
			  window);
	g_signal_connect (annot, "notify::rgba",
			  G_CALLBACK (ev_annotation_window_color_changed),
			  window);
	g_signal_connect (annot, "notify::opacity",
			  G_CALLBACK (ev_annotation_window_opacity_changed),
			  window);

	return object;
}

static gboolean
ev_annotation_window_configure_event (CtkWidget         *widget,
				      CdkEventConfigure *event)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (widget);

	if (window->in_move &&
	    (window->x != event->x || window->y != event->y)) {
		window->x = event->x;
		window->y = event->y;
	}

	return CTK_WIDGET_CLASS (ev_annotation_window_parent_class)->configure_event (widget, event);
}

static gboolean
ev_annotation_window_focus_in_event (CtkWidget     *widget,
				     CdkEventFocus *event)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (widget);

	if (window->in_move) {
		if (window->orig_x != window->x || window->orig_y != window->y) {
			window->orig_x = window->x;
			window->orig_y = window->y;
			g_signal_emit (window, signals[MOVED], 0, window->x, window->y);
		}
		window->in_move = FALSE;
	}

	ctk_widget_grab_focus (window->text_view);
	send_focus_change (window->text_view, TRUE);
	ctk_text_view_set_cursor_visible (CTK_TEXT_VIEW (window->text_view), TRUE);

	return FALSE;
}

static gboolean
ev_annotation_window_focus_out_event (CtkWidget     *widget,
				      CdkEventFocus *event)
{
	EvAnnotationWindow *window = EV_ANNOTATION_WINDOW (widget);

	ev_annotation_window_sync_contents (window);

	return FALSE;
}

static void
ev_annotation_window_class_init (EvAnnotationWindowClass *klass)
{
	GObjectClass   *g_object_class = G_OBJECT_CLASS (klass);
	CtkWidgetClass *ctk_widget_class = CTK_WIDGET_CLASS (klass);

	g_object_class->constructor = ev_annotation_window_constructor;
	g_object_class->set_property = ev_annotation_window_set_property;
	g_object_class->dispose = ev_annotation_window_dispose;

	ctk_widget_class->configure_event = ev_annotation_window_configure_event;
	ctk_widget_class->focus_in_event = ev_annotation_window_focus_in_event;
	ctk_widget_class->focus_out_event = ev_annotation_window_focus_out_event;

	g_object_class_install_property (g_object_class,
					 PROP_ANNOTATION,
					 g_param_spec_object ("annotation",
							      "Annotation",
							      "The annotation associated to the window",
							      EV_TYPE_ANNOTATION_MARKUP,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (g_object_class,
					 PROP_PARENT,
					 g_param_spec_object ("parent",
							      "Parent",
							      "The parent window",
							      CTK_TYPE_WINDOW,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY));
	signals[CLOSED] =
		g_signal_new ("closed",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvAnnotationWindowClass, closed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0, G_TYPE_NONE);
	signals[MOVED] =
		g_signal_new ("moved",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvAnnotationWindowClass, moved),
			      NULL, NULL,
			      ev_view_marshal_VOID__INT_INT,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT, G_TYPE_INT);
}

/* Public methods */
CtkWidget *
ev_annotation_window_new (EvAnnotation *annot,
			  CtkWindow    *parent)
{
	CtkWidget *window;

	g_return_val_if_fail (EV_IS_ANNOTATION_MARKUP (annot), NULL);
	g_return_val_if_fail (CTK_IS_WINDOW (parent), NULL);

	window = g_object_new (EV_TYPE_ANNOTATION_WINDOW,
			       "annotation", annot,
			       "parent", parent,
			       NULL);
	return window;
}

EvAnnotation *
ev_annotation_window_get_annotation (EvAnnotationWindow *window)
{
	g_return_val_if_fail (EV_IS_ANNOTATION_WINDOW (window), NULL);

	return window->annotation;
}

void
ev_annotation_window_set_annotation (EvAnnotationWindow *window,
				     EvAnnotation       *annot)
{
	g_return_if_fail (EV_IS_ANNOTATION_WINDOW (window));
	g_return_if_fail (EV_IS_ANNOTATION (annot));

	if (annot == window->annotation)
		return;

	g_object_unref (window->annotation);
	window->annotation = g_object_ref (annot);
	ev_annotation_window_sync_contents (window);
	g_object_notify (G_OBJECT (window), "annotation");
}

gboolean
ev_annotation_window_is_open (EvAnnotationWindow *window)
{
	g_return_val_if_fail (EV_IS_ANNOTATION_WINDOW (window), FALSE);

	return window->is_open;
}

void
ev_annotation_window_get_rectangle (EvAnnotationWindow *window,
				    EvRectangle        *rect)
{
	g_return_if_fail (EV_IS_ANNOTATION_WINDOW (window));
	g_return_if_fail (rect != NULL);

	*rect = window->rect;
}

void
ev_annotation_window_set_rectangle (EvAnnotationWindow *window,
				    const EvRectangle  *rect)
{
	g_return_if_fail (EV_IS_ANNOTATION_WINDOW (window));
	g_return_if_fail (rect != NULL);

	window->rect = *rect;
}

void
ev_annotation_window_grab_focus (EvAnnotationWindow *window)
{
	g_return_if_fail (EV_IS_ANNOTATION_WINDOW (window));

	if (!ctk_widget_has_focus (window->text_view)) {
		ctk_widget_grab_focus (CTK_WIDGET (window));
		send_focus_change (window->text_view, TRUE);
	}
}

void
ev_annotation_window_ungrab_focus (EvAnnotationWindow *window)
{
	g_return_if_fail (EV_IS_ANNOTATION_WINDOW (window));

	if (ctk_widget_has_focus (window->text_view)) {
		send_focus_change (window->text_view, FALSE);
	}

	ev_annotation_window_sync_contents (window);
}
