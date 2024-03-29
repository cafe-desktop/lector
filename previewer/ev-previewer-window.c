/* ev-previewer-window.c:
 *  this file is part of lector, a cafe document viewer
 *
 * Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
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

#if CTKUNIXPRINT_ENABLED
#include <ctk/ctkunixprint.h>
#endif
#include <glib/gi18n.h>
#include <lector-view.h>
#include "ev-page-action.h"

#include "ev-previewer-window.h"

struct _EvPreviewerWindow {
	CtkWindow         base_instance;

	EvDocumentModel  *model;
	EvDocument       *document;

	CtkActionGroup   *action_group;
	CtkActionGroup   *accels_group;
	CtkUIManager     *ui_manager;

	CtkWidget        *swindow;
	EvView           *view;
	gdouble           dpi;

	/* Printing */
	CtkPrintSettings *print_settings;
	CtkPageSetup     *print_page_setup;
#if CTKUNIXPRINT_ENABLED
	CtkPrinter       *printer;
#endif
	gchar            *print_job_title;
	gchar            *source_file;
};

struct _EvPreviewerWindowClass {
	CtkWindowClass base_class;
};

enum {
	PROP_0,
	PROP_MODEL
};

#define MIN_SCALE 0.05409
#define MAX_SCALE 4.0

G_DEFINE_TYPE (EvPreviewerWindow, ev_previewer_window, CTK_TYPE_WINDOW)

static gdouble
get_monitor_dpi (EvPreviewerWindow *window)
{
	CdkWindow  *cdk_window;
	CdkMonitor *monitor;
	CdkDisplay *display;

	cdk_window = ctk_widget_get_window (CTK_WIDGET (window));
	display = cdk_window_get_display (cdk_window);
	monitor = cdk_display_get_monitor_at_window (display, cdk_window);

	return ev_document_misc_get_monitor_dpi (monitor);
}

#if CTKUNIXPRINT_ENABLED
static void
ev_previewer_window_error_dialog_run (EvPreviewerWindow *window,
				      GError            *error)
{
	CtkWidget *dialog;

	dialog = ctk_message_dialog_new (CTK_WINDOW (window),
					 CTK_DIALOG_MODAL |
					 CTK_DIALOG_DESTROY_WITH_PARENT,
					 CTK_MESSAGE_ERROR,
					 CTK_BUTTONS_CLOSE,
					 "%s", _("Failed to print document"));
	ctk_message_dialog_format_secondary_text (CTK_MESSAGE_DIALOG (dialog),
						  "%s", error->message);
	ctk_dialog_run (CTK_DIALOG (dialog));
	ctk_widget_destroy (dialog);
}
#endif

static void
ev_previewer_window_close (CtkAction         *action,
			   EvPreviewerWindow *window)
{
	ctk_widget_destroy (CTK_WIDGET (window));
}

static void
ev_previewer_window_previous_page (CtkAction         *action,
				   EvPreviewerWindow *window)
{
	ev_view_previous_page (window->view);
}

static void
ev_previewer_window_next_page (CtkAction         *action,
			       EvPreviewerWindow *window)
{
	ev_view_next_page (window->view);
}

static void
ev_previewer_window_zoom_in (CtkAction         *action,
			     EvPreviewerWindow *window)
{
	ev_document_model_set_sizing_mode (window->model, EV_SIZING_FREE);
	ev_view_zoom_in (window->view);
}

static void
ev_previewer_window_zoom_out (CtkAction         *action,
			      EvPreviewerWindow *window)
{
	ev_document_model_set_sizing_mode (window->model, EV_SIZING_FREE);
	ev_view_zoom_out (window->view);
}

static void
ev_previewer_window_zoom_reset (CtkAction         *action,
			      EvPreviewerWindow *window)
{
	ev_document_model_set_sizing_mode (window->model, EV_SIZING_FREE);
	ev_view_zoom_reset (window->view);
}

static void
ev_previewer_window_zoom_fit_page (CtkToggleAction   *action,
				   EvPreviewerWindow *window)
{
	ev_document_model_set_sizing_mode (window->model,
					   ctk_toggle_action_get_active (action) ?
					   EV_SIZING_FIT_PAGE : EV_SIZING_FREE);
}

static void
ev_previewer_window_zoom_fit_width (CtkToggleAction   *action,
                                    EvPreviewerWindow *window)
{
	ev_document_model_set_sizing_mode (window->model,
					   ctk_toggle_action_get_active (action) ?
					   EV_SIZING_FIT_WIDTH : EV_SIZING_FREE);
}

static void
ev_previewer_window_action_page_activated (CtkAction         *action,
					   EvLink            *link,
					   EvPreviewerWindow *window)
{
	ev_view_handle_link (window->view, link);
	ctk_widget_grab_focus (CTK_WIDGET (window->view));
}

static void
ev_previewer_window_focus_page_selector (CtkAction         *action,
					 EvPreviewerWindow *window)
{
	CtkAction *page_action;

	page_action = ctk_action_group_get_action (window->action_group,
						   "PageSelector");
	ev_page_action_grab_focus (EV_PAGE_ACTION (page_action));
}

#if CTKUNIXPRINT_ENABLED
static void
ev_previewer_window_print_finished (CtkPrintJob       *print_job,
				    EvPreviewerWindow *window,
				    GError            *error)
{
	if (error) {
		ev_previewer_window_error_dialog_run (window, error);
	}

	g_object_unref (print_job);
	ctk_widget_destroy (CTK_WIDGET (window));
}

static void
ev_previewer_window_do_print (EvPreviewerWindow *window)
{
	CtkPrintJob *job;
	GError      *error = NULL;

	job = ctk_print_job_new (window->print_job_title ?
				 window->print_job_title :
				 window->source_file,
				 window->printer,
				 window->print_settings,
				 window->print_page_setup);
	if (ctk_print_job_set_source_file (job, window->source_file, &error)) {
		ctk_print_job_send (job,
				    (CtkPrintJobCompleteFunc)ev_previewer_window_print_finished,
				    window, NULL);
	} else {
		ev_previewer_window_error_dialog_run (window, error);
		g_error_free (error);
	}

	ctk_widget_hide (CTK_WIDGET (window));
}

static void
ev_previewer_window_enumerate_finished (EvPreviewerWindow *window)
{
	if (window->printer) {
		ev_previewer_window_do_print (window);
	} else {
		GError *error = NULL;

		g_set_error (&error,
			     CTK_PRINT_ERROR,
			     CTK_PRINT_ERROR_GENERAL,
			     _("The selected printer '%s' could not be found"),
			     ctk_print_settings_get_printer (window->print_settings));

		ev_previewer_window_error_dialog_run (window, error);
		g_error_free (error);
	}
}

static gboolean
ev_previewer_window_enumerate_printers (CtkPrinter        *printer,
					EvPreviewerWindow *window)
{
	const gchar *printer_name;

	printer_name = ctk_print_settings_get_printer (window->print_settings);
	if ((printer_name
	     && strcmp (printer_name, ctk_printer_get_name (printer)) == 0) ||
	    (!printer_name && ctk_printer_is_default (printer))) {
		if (window->printer)
			g_object_unref (window->printer);
		window->printer = g_object_ref (printer);

		return TRUE; /* we're done */
	}

	return FALSE; /* continue the enumeration */
}

static void
ev_previewer_window_print (CtkAction         *action,
			   EvPreviewerWindow *window)
{
	if (!window->print_settings)
		window->print_settings = ctk_print_settings_new ();
	if (!window->print_page_setup)
		window->print_page_setup = ctk_page_setup_new ();
	ctk_enumerate_printers ((CtkPrinterFunc)ev_previewer_window_enumerate_printers,
				window,
				(GDestroyNotify)ev_previewer_window_enumerate_finished,
				FALSE);
}
#endif

static const CtkActionEntry action_entries[] = {
	{ "FileCloseWindow", "window-close", N_("_Close"), "<control>W",
	  NULL,
	  G_CALLBACK (ev_previewer_window_close) },
	{ "GoPreviousPage", "go-up", N_("_Previous Page"), "<control>Page_Up",
          N_("Go to the previous page"),
          G_CALLBACK (ev_previewer_window_previous_page) },
        { "GoNextPage", "go-down", N_("_Next Page"), "<control>Page_Down",
          N_("Go to the next page"),
          G_CALLBACK (ev_previewer_window_next_page) },
        { "ViewZoomIn", "zoom-in", N_("Zoom _In"), "<control>plus",
          N_("Enlarge the document"),
          G_CALLBACK (ev_previewer_window_zoom_in) },
        { "ViewZoomOut", "zoom-out", N_("Zoom _Out"), "<control>minus",
          N_("Shrink the document"),
          G_CALLBACK (ev_previewer_window_zoom_out) },
        { "ViewZoomReset", "zoom-original", NULL, "<control>0",
          N_("Reset zoom to 100\%"),
          G_CALLBACK (ev_previewer_window_zoom_reset) },
#if CTKUNIXPRINT_ENABLED
	/* translators: Print document currently shown in the Print Preview window */
	{ "PreviewPrint", "document-print", N_("Print"), NULL,
	  N_("Print this document"),
	  G_CALLBACK (ev_previewer_window_print) }
#endif
};

static const CtkActionEntry accel_entries[] = {
	{ "p", "go-up", "", "p", NULL,
	  G_CALLBACK (ev_previewer_window_previous_page) },
	{ "n", "go-down", "", "n", NULL,
	  G_CALLBACK (ev_previewer_window_next_page) },
	{ "Plus", "zoom-in", NULL, "plus", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_in) },
	{ "CtrlEqual", "zoom-in", NULL, "<control>equal", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_in) },
	{ "Equal", "zoom-in", NULL, "equal", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_in) },
	{ "Minus", "zoom-out", NULL, "minus", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_out) },
	{ "KpPlus", "zoom-in", NULL, "KP_Add", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_in) },
	{ "KpMinus", "zoom-out", NULL, "KP_Subtract", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_out) },
	{ "CtrlKpPlus", "zoom-in", NULL, "<control>KP_Add", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_in) },
	{ "CtrlKpMinus", "zoom-out", NULL, "<control>KP_Subtract", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_out) },
	{ "CtrlKpZero", "zoom-original", NULL, "<control>KP_0", NULL,
	  G_CALLBACK (ev_previewer_window_zoom_reset) },
	{ "FocusPageSelector", NULL, "", "<control>l", NULL,
	  G_CALLBACK (ev_previewer_window_focus_page_selector) }

};

static const CtkToggleActionEntry toggle_action_entries[] = {
	{ "ViewFitPage", EV_STOCK_ZOOM_PAGE, N_("Fit Pa_ge"), NULL,
	  N_("Make the current document fill the window"),
	  G_CALLBACK (ev_previewer_window_zoom_fit_page) },
	{ "ViewFitWidth", EV_STOCK_ZOOM_WIDTH, N_("Fit _Width"), NULL,
	  N_("Make the current document fill the window width"),
	  G_CALLBACK (ev_previewer_window_zoom_fit_width) }
};

static gboolean
view_focus_changed (CtkWidget         *widget,
		    CdkEventFocus     *event,
		    EvPreviewerWindow *window)
{
	if (window->accels_group)
		ctk_action_group_set_sensitive (window->accels_group, event->in);

	return FALSE;
}

static void
view_sizing_mode_changed (EvDocumentModel   *model,
			  GParamSpec        *pspec,
			  EvPreviewerWindow *window)
{
	EvSizingMode sizing_mode = ev_document_model_get_sizing_mode (model);
	CtkAction   *action;

	action = ctk_action_group_get_action (window->action_group, "ViewFitPage");
	g_signal_handlers_block_by_func (action,
					 G_CALLBACK (ev_previewer_window_zoom_fit_page),
					 window);
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
				      sizing_mode == EV_SIZING_FIT_PAGE);
	g_signal_handlers_unblock_by_func (action,
					   G_CALLBACK (ev_previewer_window_zoom_fit_page),
					   window);

	action = ctk_action_group_get_action (window->action_group, "ViewFitWidth");
	g_signal_handlers_block_by_func (action,
					 G_CALLBACK (ev_previewer_window_zoom_fit_width),
					 window);
	ctk_toggle_action_set_active (CTK_TOGGLE_ACTION (action),
				      sizing_mode == EV_SIZING_FIT_WIDTH);
	g_signal_handlers_unblock_by_func (action,
					   G_CALLBACK (ev_previewer_window_zoom_fit_width),
					   window);
}

static void
ev_previewer_window_set_document (EvPreviewerWindow *window,
				  GParamSpec        *pspec,
				  EvDocumentModel   *model)
{
	EvDocument *document = ev_document_model_get_document (model);

	window->document = g_object_ref (document);

	g_signal_connect (model, "notify::sizing-mode",
			  G_CALLBACK (view_sizing_mode_changed),
			  window);
	ctk_action_group_set_sensitive (window->action_group, TRUE);
	ctk_action_group_set_sensitive (window->accels_group, TRUE);
}

static void
ev_previewer_window_connect_action_accelerators (EvPreviewerWindow *window)
{
	GList *actions;

	ctk_ui_manager_ensure_update (window->ui_manager);

	actions = ctk_action_group_list_actions (window->action_group);
	g_list_foreach (actions, (GFunc)ctk_action_connect_accelerator, NULL);
	g_list_free (actions);
}

static void
ev_previewer_window_dispose (GObject *object)
{
	EvPreviewerWindow *window = EV_PREVIEWER_WINDOW (object);

	if (window->model) {
		g_object_unref (window->model);
		window->model = NULL;
	}

	if (window->document) {
		g_object_unref (window->document);
		window->document = NULL;
	}

	if (window->action_group) {
		g_object_unref (window->action_group);
		window->action_group = NULL;
	}

	if (window->accels_group) {
		g_object_unref (window->accels_group);
		window->accels_group = NULL;
	}

	if (window->ui_manager) {
		g_object_unref (window->ui_manager);
		window->ui_manager = NULL;
	}

	if (window->print_settings) {
		g_object_unref (window->print_settings);
		window->print_settings = NULL;
	}

	if (window->print_page_setup) {
		g_object_unref (window->print_page_setup);
		window->print_page_setup = NULL;
	}

#if CTKUNIXPRINT_ENABLED
	if (window->printer) {
		g_object_unref (window->printer);
		window->printer = NULL;
	}
#endif

	if (window->print_job_title) {
		g_free (window->print_job_title);
		window->print_job_title = NULL;
	}

	if (window->source_file) {
		g_free (window->source_file);
		window->source_file = NULL;
	}

	G_OBJECT_CLASS (ev_previewer_window_parent_class)->dispose (object);
}

static void
ev_previewer_window_init (EvPreviewerWindow *window)
{
	CtkStyleContext *context;

	context = ctk_widget_get_style_context (CTK_WIDGET (window));
	ctk_style_context_add_class (context, "lector-previewer-window");

	ctk_window_set_default_size (CTK_WINDOW (window), 600, 600);
}

static void
ev_previewer_window_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	EvPreviewerWindow *window = EV_PREVIEWER_WINDOW (object);

	switch (prop_id) {
	case PROP_MODEL:
		window->model = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static GObject *
ev_previewer_window_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_params)
{
	GObject           *object;
	EvPreviewerWindow *window;
	CtkWidget         *vbox;
	CtkWidget         *toolbar;
	CtkAction         *action;
	GError            *error = NULL;
	gdouble            dpi;

	object = G_OBJECT_CLASS (ev_previewer_window_parent_class)->constructor (type,
										 n_construct_properties,
										 construct_params);
	window = EV_PREVIEWER_WINDOW (object);

	dpi = get_monitor_dpi (window);
	ev_document_model_set_min_scale (window->model, MIN_SCALE * dpi / 72.0);
	ev_document_model_set_max_scale (window->model, MAX_SCALE * dpi / 72.0);
	ev_document_model_set_sizing_mode (window->model, EV_SIZING_FIT_WIDTH);
	g_signal_connect_swapped (window->model, "notify::document",
				  G_CALLBACK (ev_previewer_window_set_document),
				  window);

	window->action_group = ctk_action_group_new ("PreviewerActions");
	ctk_action_group_set_translation_domain (window->action_group, NULL);
	ctk_action_group_add_actions (window->action_group, action_entries,
				      G_N_ELEMENTS (action_entries),
				      window);
	ctk_action_group_add_toggle_actions (window->action_group, toggle_action_entries,
					     G_N_ELEMENTS (toggle_action_entries),
					     window);
	ctk_action_group_set_sensitive (window->action_group, FALSE);

	action = g_object_new (EV_TYPE_PAGE_ACTION,
			       "name", "PageSelector",
			       "label", _("Page"),
			       "tooltip", _("Select Page"),
			       "icon_name", "text-x-generic",
			       "visible_overflown", FALSE,
			       NULL);
	ev_page_action_set_model (EV_PAGE_ACTION (action), window->model);
	g_signal_connect (action, "activate_link",
			  G_CALLBACK (ev_previewer_window_action_page_activated),
			  window);
	ctk_action_group_add_action (window->action_group, action);
	g_object_unref (action);

	window->accels_group = ctk_action_group_new ("PreviewerAccelerators");
	ctk_action_group_add_actions (window->accels_group, accel_entries,
				      G_N_ELEMENTS (accel_entries),
				      window);
	ctk_action_group_set_sensitive (window->accels_group, FALSE);

	window->ui_manager = ctk_ui_manager_new ();
	ctk_ui_manager_insert_action_group (window->ui_manager,
					    window->action_group, 0);
	ctk_ui_manager_insert_action_group (window->ui_manager,
					    window->accels_group, 1);
	ctk_window_add_accel_group (CTK_WINDOW (window),
				    ctk_ui_manager_get_accel_group (window->ui_manager));

	ctk_ui_manager_add_ui_from_resource (window->ui_manager, "/org/cafe/lector/previewer/ui/previewer.xml", &error);
	g_assert_no_error (error);

	/* CTKUIManager connects actions accels only for menu items,
	 * but not for tool items. See bug #612972.
	 */
	ev_previewer_window_connect_action_accelerators (window);

	view_sizing_mode_changed (window->model, NULL, window);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 0);

	toolbar = ctk_ui_manager_get_widget (window->ui_manager, "/PreviewToolbar");
	ctk_box_pack_start (CTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
	ctk_widget_show (toolbar);

	window->swindow = ctk_scrolled_window_new (NULL, NULL);
	ctk_scrolled_window_set_policy (CTK_SCROLLED_WINDOW (window->swindow),
					CTK_POLICY_AUTOMATIC,
					CTK_POLICY_AUTOMATIC);

	window->view = EV_VIEW (ev_view_new ());
	g_signal_connect_object (window->view, "focus_in_event",
				 G_CALLBACK (view_focus_changed),
				 window, 0);
	g_signal_connect_object (window->view, "focus_out_event",
				 G_CALLBACK (view_focus_changed),
				 window, 0);
	ev_view_set_model (window->view, window->model);
	ev_document_model_set_continuous (window->model, FALSE);

	ctk_container_add (CTK_CONTAINER (window->swindow), CTK_WIDGET (window->view));
	ctk_widget_show (CTK_WIDGET (window->view));

	ctk_box_pack_start (CTK_BOX (vbox), window->swindow, TRUE, TRUE, 0);
	ctk_widget_show (window->swindow);

	ctk_container_add (CTK_CONTAINER (window), vbox);
	ctk_widget_show (vbox);

	return object;
}


static void
ev_previewer_window_class_init (EvPreviewerWindowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructor = ev_previewer_window_constructor;
	gobject_class->set_property = ev_previewer_window_set_property;
	gobject_class->dispose = ev_previewer_window_dispose;

	g_object_class_install_property (gobject_class,
					 PROP_MODEL,
					 g_param_spec_object ("model",
							      "Model",
							      "The document model",
							      EV_TYPE_DOCUMENT_MODEL,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY));
}

/* Public methods */
CtkWidget *
ev_previewer_window_new (EvDocumentModel *model)
{
	return CTK_WIDGET (g_object_new (EV_TYPE_PREVIEWER_WINDOW, "model", model, NULL));
}

void
ev_previewer_window_set_print_settings (EvPreviewerWindow *window,
					const gchar       *print_settings)
{
	if (window->print_settings)
		g_object_unref (window->print_settings);
	if (window->print_page_setup)
		g_object_unref (window->print_page_setup);
	if (window->print_job_title)
		g_free (window->print_job_title);

	if (print_settings && g_file_test (print_settings, G_FILE_TEST_IS_REGULAR)) {
		GKeyFile *key_file;
		GError   *error = NULL;

		key_file = g_key_file_new ();
		g_key_file_load_from_file (key_file,
					   print_settings,
					   G_KEY_FILE_KEEP_COMMENTS |
					   G_KEY_FILE_KEEP_TRANSLATIONS,
					   &error);
		if (!error) {
			CtkPrintSettings *psettings;
			CtkPageSetup     *psetup;
			gchar            *job_name;

			psettings = ctk_print_settings_new_from_key_file (key_file,
									  "Print Settings",
									  NULL);
			window->print_settings = psettings ? psettings : ctk_print_settings_new ();

			psetup = ctk_page_setup_new_from_key_file (key_file,
								   "Page Setup",
								   NULL);
			window->print_page_setup = psetup ? psetup : ctk_page_setup_new ();

			job_name = g_key_file_get_string (key_file,
							  "Print Job", "title",
							  NULL);
			if (job_name) {
				window->print_job_title = job_name;
				ctk_window_set_title (CTK_WINDOW (window), job_name);
			}
		} else {
			window->print_settings = ctk_print_settings_new ();
			window->print_page_setup = ctk_page_setup_new ();
			g_error_free (error);
		}

		g_key_file_free (key_file);
	} else {
		window->print_settings = ctk_print_settings_new ();
		window->print_page_setup = ctk_page_setup_new ();
	}
}

void
ev_previewer_window_set_source_file (EvPreviewerWindow *window,
				     const gchar       *source_file)
{
	if (window->source_file)
		g_free (window->source_file);
	window->source_file = g_strdup (source_file);
}
