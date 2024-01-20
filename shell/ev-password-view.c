/* this file is part of lector, a cafe document viewer
 *
 *  Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 *  Copyright (C) 2005 Red Hat, Inc
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <ctk/ctk.h>
#include <gio/gio.h>

#include "ev-keyring.h"
#include "ev-password-view.h"

enum {
	UNLOCK,
	LAST_SIGNAL
};
struct _EvPasswordViewPrivate {
	CtkWindow    *parent_window;
	CtkWidget    *label;
	CtkWidget    *password_entry;

	gchar        *password;
	GPasswordSave password_save;

	GFile        *uri_file;
};

static guint password_view_signals [LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (EvPasswordView, ev_password_view, CTK_TYPE_VIEWPORT)

static void
ev_password_view_finalize (GObject *object)
{
	EvPasswordView *password_view = EV_PASSWORD_VIEW (object);

	if (password_view->priv->password) {
		g_free (password_view->priv->password);
		password_view->priv->password = NULL;
	}

	password_view->priv->parent_window = NULL;

	if (password_view->priv->uri_file) {
		g_object_unref (password_view->priv->uri_file);
		password_view->priv->uri_file = NULL;
	}

	G_OBJECT_CLASS (ev_password_view_parent_class)->finalize (object);
}

static void
ev_password_view_class_init (EvPasswordViewClass *class)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS (class);

	password_view_signals[UNLOCK] =
		g_signal_new ("unlock",
			      G_TYPE_FROM_CLASS (g_object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvPasswordViewClass, unlock),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_object_class->finalize = ev_password_view_finalize;
}

static void
ev_password_view_clicked_cb (CtkWidget      *button,
			     EvPasswordView *password_view)
{
	ev_password_view_ask_password (password_view);
}

static void
ev_password_view_init (EvPasswordView *password_view)
{
	CtkWidget *vbox;
	CtkWidget *hbox;
	CtkWidget *image;
	CtkWidget *button;
	CtkWidget *label;
	gchar     *markup;

	password_view->priv = ev_password_view_get_instance_private (password_view);

	password_view->priv->password_save = G_PASSWORD_SAVE_NEVER;

	/* set ourselves up */
	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 24);
	ctk_widget_set_valign (vbox, CTK_ALIGN_CENTER);
	ctk_widget_set_hexpand (vbox, FALSE);
	ctk_widget_set_vexpand (vbox, FALSE);
	ctk_container_set_border_width (CTK_CONTAINER (vbox), 24);
	ctk_container_add (CTK_CONTAINER (password_view), vbox);

	password_view->priv->label =
		(CtkWidget *) g_object_new (CTK_TYPE_LABEL,
					    "wrap", TRUE,
					    "selectable", TRUE,
					    NULL);
	ctk_box_pack_start (CTK_BOX (vbox), password_view->priv->label, FALSE, FALSE, 0);

	image = ctk_image_new_from_icon_name ("dialog-password",
	                                      CTK_ICON_SIZE_DIALOG);
	ctk_box_pack_start (CTK_BOX (vbox), image, FALSE, FALSE, 0);

	label = ctk_label_new (NULL);
	ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
	markup = g_strdup_printf ("<span size=\"x-large\">%s</span>",
				  _("This document is locked and can only be read by entering the correct password."));
	ctk_label_set_markup (CTK_LABEL (label), markup);
	g_free (markup);

	ctk_box_pack_start (CTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 0);
	ctk_box_pack_start (CTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	button = ctk_button_new_with_mnemonic (_("_Unlock Document"));
	g_signal_connect (button, "clicked", G_CALLBACK (ev_password_view_clicked_cb), password_view);
	ctk_box_pack_end (CTK_BOX (hbox), button, FALSE, FALSE, 0);

	ctk_widget_show_all (vbox);
}

/* Public functions */
void
ev_password_view_set_uri (EvPasswordView *password_view,
			  const char     *uri)
{
	gchar *markup, *file_name;
	GFile *file;

	g_return_if_fail (EV_IS_PASSWORD_VIEW (password_view));
	g_return_if_fail (uri != NULL);

	file = g_file_new_for_uri (uri);
	if (password_view->priv->uri_file &&
	    g_file_equal (file, password_view->priv->uri_file)) {
		g_object_unref (file);
		return;
	}
	if (password_view->priv->uri_file)
		g_object_unref (password_view->priv->uri_file);
	password_view->priv->uri_file = file;

	file_name = g_file_get_basename (password_view->priv->uri_file);
	markup = g_markup_printf_escaped ("<span size=\"x-large\" weight=\"bold\">%s</span>",
					  file_name);
	g_free (file_name);

	ctk_label_set_markup (CTK_LABEL (password_view->priv->label), markup);
	g_free (markup);
}

static void
ev_password_dialog_got_response (CtkDialog      *dialog,
				 gint            response_id,
				 EvPasswordView *password_view)
{
	ctk_widget_set_sensitive (CTK_WIDGET (password_view), TRUE);

	if (response_id == CTK_RESPONSE_OK) {
		g_free (password_view->priv->password);
		password_view->priv->password =
			g_strdup (ctk_entry_get_text (CTK_ENTRY (password_view->priv->password_entry)));

		g_signal_emit (password_view, password_view_signals[UNLOCK], 0);
	}

	ctk_widget_destroy (CTK_WIDGET (dialog));
}

static void
ev_password_dialog_remember_button_toggled (CtkToggleButton *button,
					    EvPasswordView  *password_view)
{
	if (ctk_toggle_button_get_active (button)) {
		gpointer data;

		data = g_object_get_data (G_OBJECT (button), "password-save");
		password_view->priv->password_save = GPOINTER_TO_INT (data);
	}
}

static void
ev_password_dialog_entry_changed_cb (CtkEditable *editable,
				     CtkDialog   *dialog)
{
	const char *text;

	text = ctk_entry_get_text (CTK_ENTRY (editable));

	ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog), CTK_RESPONSE_OK,
					   (text != NULL && *text != '\0'));
}

static void
ev_password_dialog_entry_activated_cb (CtkEntry  *entry,
				       CtkDialog *dialog)
{
	ctk_dialog_response (CTK_DIALOG (dialog), CTK_RESPONSE_OK);
}

void
ev_password_view_ask_password (EvPasswordView *password_view)
{
	CtkDialog *dialog;
	CtkWidget *content_area, *action_area;
	CtkWidget *hbox, *main_vbox, *vbox, *icon;
	CtkWidget *grid;
	CtkWidget *label;
	gchar     *text, *markup, *file_name;

	ctk_widget_set_sensitive (CTK_WIDGET (password_view), FALSE);

	dialog = CTK_DIALOG (ctk_dialog_new ());
	content_area = ctk_dialog_get_content_area (dialog);
	action_area = ctk_dialog_get_action_area (dialog);

	/* Set the dialog up with HIG properties */
	ctk_container_set_border_width (CTK_CONTAINER (dialog), 5);
	ctk_box_set_spacing (CTK_BOX (content_area), 2); /* 2 * 5 + 2 = 12 */
	ctk_container_set_border_width (CTK_CONTAINER (action_area), 5);
	ctk_box_set_spacing (CTK_BOX (action_area), 6);

	ctk_window_set_title (CTK_WINDOW (dialog), _("Enter password"));
	ctk_window_set_resizable (CTK_WINDOW (dialog), FALSE);
	ctk_window_set_icon_name (CTK_WINDOW (dialog), "dialog-password");
	ctk_window_set_transient_for (CTK_WINDOW (dialog), password_view->priv->parent_window);
	ctk_window_set_modal (CTK_WINDOW (dialog), TRUE);

	ctk_dialog_add_buttons (dialog,
				"ctk-cancel", CTK_RESPONSE_CANCEL,
				_("_Unlock Document"), CTK_RESPONSE_OK,
				NULL);
	ctk_dialog_set_default_response (dialog, CTK_RESPONSE_OK);
	ctk_dialog_set_response_sensitive (CTK_DIALOG (dialog),
					   CTK_RESPONSE_OK, FALSE);

	/* Build contents */
	hbox = ctk_box_new (CTK_ORIENTATION_HORIZONTAL, 12);
	ctk_container_set_border_width (CTK_CONTAINER (hbox), 5);
	ctk_box_pack_start (CTK_BOX (content_area), hbox, TRUE, TRUE, 0);
	ctk_widget_show (hbox);

	icon = ctk_image_new_from_icon_name ("dialog-password",
	                                      CTK_ICON_SIZE_DIALOG);

	ctk_widget_set_halign (icon, CTK_ALIGN_CENTER);
	ctk_widget_set_valign (icon, CTK_ALIGN_START);
	ctk_box_pack_start (CTK_BOX (hbox), icon, FALSE, FALSE, 0);
	ctk_widget_show (icon);

	main_vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 18);
	ctk_box_pack_start (CTK_BOX (hbox), main_vbox, TRUE, TRUE, 0);
	ctk_widget_show (main_vbox);

	label = ctk_label_new (NULL);
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);
	ctk_label_set_line_wrap (CTK_LABEL (label), TRUE);
	file_name = g_file_get_basename (password_view->priv->uri_file);
	text = g_markup_printf_escaped (_("The document “%s” is locked and requires a password before it can be opened."),
                                        file_name);
	markup = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n\n%s",
				  _("Password required"),
                                  text);
	ctk_label_set_markup (CTK_LABEL (label), markup);
	g_free (text);
	g_free (markup);
	g_free (file_name);
	ctk_box_pack_start (CTK_BOX (main_vbox), label,
			    FALSE, FALSE, 0);
	ctk_widget_show (label);

	vbox = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
	ctk_box_pack_start (CTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
	ctk_widget_show (vbox);

	/* The grid that holds the entries */
	grid = ctk_grid_new ();
	ctk_grid_set_column_spacing (CTK_GRID (grid), 12);
	ctk_grid_set_row_spacing (CTK_GRID (grid), 6);
	ctk_widget_set_valign (grid, CTK_ALIGN_START);
	ctk_widget_set_hexpand (grid, TRUE);
	ctk_widget_set_vexpand (grid, TRUE);
	ctk_widget_set_margin_top (grid, 0);
	ctk_widget_set_margin_bottom (grid, 0);
	ctk_widget_set_margin_start (grid, 0);
	ctk_widget_set_margin_end (grid, 0);
	ctk_widget_show (grid);
	ctk_box_pack_start (CTK_BOX (vbox),
	                    grid,
	                    FALSE, FALSE, 0);

	label = ctk_label_new_with_mnemonic (_("_Password:"));
	ctk_label_set_xalign (CTK_LABEL (label), 0.0);

	password_view->priv->password_entry = ctk_entry_new ();
	ctk_entry_set_visibility (CTK_ENTRY (password_view->priv->password_entry), FALSE);
	g_signal_connect (password_view->priv->password_entry, "changed",
			  G_CALLBACK (ev_password_dialog_entry_changed_cb),
			  dialog);
	g_signal_connect (password_view->priv->password_entry, "activate",
			  G_CALLBACK (ev_password_dialog_entry_activated_cb),
			  dialog);

	ctk_grid_attach (CTK_GRID (grid), label, 0, 0, 1, 1);
	ctk_widget_show (label);

	ctk_grid_attach (CTK_GRID (grid), password_view->priv->password_entry, 1, 0, 1, 1);
	ctk_widget_set_hexpand (password_view->priv->password_entry, TRUE);
	ctk_widget_show (password_view->priv->password_entry);

	ctk_label_set_mnemonic_widget (CTK_LABEL (label),
				       password_view->priv->password_entry);

	if (ev_keyring_is_available ()) {
		CtkWidget  *choice;
		CtkWidget  *remember_box;
		GSList     *group;

		remember_box = ctk_box_new (CTK_ORIENTATION_VERTICAL, 6);
		ctk_box_pack_start (CTK_BOX (vbox), remember_box,
				    FALSE, FALSE, 0);
		ctk_widget_show (remember_box);

		choice = ctk_radio_button_new_with_mnemonic (NULL, _("Forget password _immediately"));
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
					      password_view->priv->password_save == G_PASSWORD_SAVE_NEVER);
		g_object_set_data (G_OBJECT (choice), "password-save",
				   GINT_TO_POINTER (G_PASSWORD_SAVE_NEVER));
		g_signal_connect (choice, "toggled",
				  G_CALLBACK (ev_password_dialog_remember_button_toggled),
				  password_view);
		ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);
		ctk_widget_show (choice);

		group = ctk_radio_button_get_group (CTK_RADIO_BUTTON (choice));
		choice = ctk_radio_button_new_with_mnemonic (group, _("Remember password until you _log out"));
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
					      password_view->priv->password_save == G_PASSWORD_SAVE_FOR_SESSION);
		g_object_set_data (G_OBJECT (choice), "password-save",
				   GINT_TO_POINTER (G_PASSWORD_SAVE_FOR_SESSION));
		g_signal_connect (choice, "toggled",
				  G_CALLBACK (ev_password_dialog_remember_button_toggled),
				  password_view);
		ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);
		ctk_widget_show (choice);

		group = ctk_radio_button_get_group (CTK_RADIO_BUTTON (choice));
		choice = ctk_radio_button_new_with_mnemonic (group, _("Remember _forever"));
		ctk_toggle_button_set_active (CTK_TOGGLE_BUTTON (choice),
					      password_view->priv->password_save == G_PASSWORD_SAVE_PERMANENTLY);
		g_object_set_data (G_OBJECT (choice), "password-save",
				   GINT_TO_POINTER (G_PASSWORD_SAVE_PERMANENTLY));
		g_signal_connect (choice, "toggled",
				  G_CALLBACK (ev_password_dialog_remember_button_toggled),
				  password_view);
		ctk_box_pack_start (CTK_BOX (remember_box), choice, FALSE, FALSE, 0);
		ctk_widget_show (choice);
	}

	g_signal_connect (dialog, "response",
			  G_CALLBACK (ev_password_dialog_got_response),
			  password_view);

	ctk_widget_show (CTK_WIDGET (dialog));
}

const gchar *
ev_password_view_get_password (EvPasswordView *password_view)
{
	return password_view->priv->password;
}

GPasswordSave
ev_password_view_get_password_save_flags (EvPasswordView *password_view)
{
	return password_view->priv->password_save;
}

CtkWidget *
ev_password_view_new (CtkWindow *parent)
{
	EvPasswordView *retval;

	retval = EV_PASSWORD_VIEW (g_object_new (EV_TYPE_PASSWORD_VIEW, NULL));

	retval->priv->parent_window = parent;

	return CTK_WIDGET (retval);
}

