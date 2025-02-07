/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/***************************************************************************
 *
 * Copyright (C) 2008 Dan Williams, <dcbw@redhat.com>
 * Copyright (C) 2008 - 2011 Red Hat, Inc.
 * Based on work by David Zeuthen, <davidz@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 **************************************************************************/

#include "nm-default.h"

#include "nm-pptp-editor-plugin.h"
#include "nm-utils/nm-vpn-plugin-utils.h"

#define PPTP_PLUGIN_NAME    _("Point-to-Point Tunneling Protocol (PPTP)")
#define PPTP_PLUGIN_DESC    _("Compatible with Microsoft and other PPTP VPN servers.")

/*****************************************************************************/

static void pptp_plugin_ui_interface_init (NMVpnEditorPluginInterface *iface_class);

G_DEFINE_TYPE_EXTENDED (PptpPluginUi, pptp_plugin_ui, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (NM_TYPE_VPN_EDITOR_PLUGIN,
                                               pptp_plugin_ui_interface_init))

enum {
	PROP_0,
	PROP_NAME,
	PROP_DESC,
	PROP_SERVICE,

	LAST_PROP
};

/*****************************************************************************/

static NMConnection *
import (NMVpnEditorPlugin *iface, const char *path, GError **error)
{
	gs_free char *contents = NULL;
	gs_strfreev char **lines = NULL;
	char *ext;

	ext = strrchr (path, '.');
	if (!ext) {
		g_set_error (error,
		             NMV_EDITOR_PLUGIN_ERROR,
		             NMV_EDITOR_PLUGIN_ERROR_FILE_NOT_VPN,
		             "unknown PPTP file extension");
		return NULL;
	}

	if (strcmp (ext, ".conf") && strcmp (ext, ".cnf")) {
		g_set_error (error,
		             NMV_EDITOR_PLUGIN_ERROR,
		             NMV_EDITOR_PLUGIN_ERROR_FILE_NOT_VPN,
		             "unknown PPTP file extension");
		return NULL;
	}

	if (!g_file_get_contents (path, &contents, NULL, error))
		return NULL;

	lines = g_strsplit_set (contents, "\r\n", 0);
	if (g_strv_length (lines) <= 1) {
		g_set_error (error,
		             NMV_EDITOR_PLUGIN_ERROR,
		             NMV_EDITOR_PLUGIN_ERROR_FILE_NOT_READABLE,
		             "not a valid PPTP configuration file");
		return NULL;
	}

	g_set_error_literal (error,
	                     NMV_EDITOR_PLUGIN_ERROR,
	                     NMV_EDITOR_PLUGIN_ERROR_FAILED,
	                     "PPTP import is not implemented");
	return NULL;
}

static gboolean
export (NMVpnEditorPlugin *iface,
        const char *path,
        NMConnection *connection,
        GError **error)
{
	g_set_error_literal (error,
	                     NMV_EDITOR_PLUGIN_ERROR,
	                     NMV_EDITOR_PLUGIN_ERROR_FAILED,
	                     "PPTP export is not implemented");
	return FALSE;
}

static char *
get_suggested_filename (NMVpnEditorPlugin *iface, NMConnection *connection)
{
	NMSettingConnection *s_con;
	const char *id;

	g_return_val_if_fail (connection != NULL, NULL);

	s_con = nm_connection_get_setting_connection (connection);
	g_return_val_if_fail (s_con != NULL, NULL);

	id = nm_setting_connection_get_id (s_con);
	g_return_val_if_fail (id != NULL, NULL);

	return g_strdup_printf ("%s (pptp).conf", id);
}

static NMVpnEditorPluginCapability
get_capabilities (NMVpnEditorPlugin *iface)
{
	return NM_VPN_EDITOR_PLUGIN_CAPABILITY_NONE;
}

static NMVpnEditor *
_call_editor_factory (gpointer factory,
                      NMVpnEditorPlugin *editor_plugin,
                      NMConnection *connection,
                      gpointer user_data,
                      GError **error)
{
	return ((NMVpnEditorFactory) factory) (editor_plugin,
	                                       connection,
	                                       error);
}

static NMVpnEditor *
get_editor (NMVpnEditorPlugin *iface, NMConnection *connection, GError **error)
{
	gpointer gtk3_only_symbol;
	GModule *self_module;
	const char *editor;

	g_return_val_if_fail (PPTP_IS_PLUGIN_UI (iface), NULL);
	g_return_val_if_fail (NM_IS_CONNECTION (connection), NULL);
	g_return_val_if_fail (!error || !*error, NULL);


	self_module = g_module_open (NULL, 0);
	g_module_symbol (self_module, "gtk_container_add", &gtk3_only_symbol);
	g_module_close (self_module);

	if (gtk3_only_symbol) {
		editor = "libnm-vpn-plugin-pptp-editor.so";
	} else {
		editor = "libnm-gtk4-vpn-plugin-pptp-editor.so";
	}

	return nm_vpn_plugin_utils_load_editor (editor,
						"nm_vpn_editor_factory_pptp",
						_call_editor_factory,
						iface,
						connection,
						NULL,
						error);
}

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, PPTP_PLUGIN_NAME);
		break;
	case PROP_DESC:
		g_value_set_string (value, PPTP_PLUGIN_DESC);
		break;
	case PROP_SERVICE:
		g_value_set_string (value, NM_DBUS_SERVICE_PPTP);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
pptp_plugin_ui_class_init (PptpPluginUiClass *req_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (req_class);

	object_class->get_property = get_property;

	g_object_class_override_property (object_class,
	                                  PROP_NAME,
	                                  NM_VPN_EDITOR_PLUGIN_NAME);

	g_object_class_override_property (object_class,
	                                  PROP_DESC,
	                                  NM_VPN_EDITOR_PLUGIN_DESCRIPTION);

	g_object_class_override_property (object_class,
	                                  PROP_SERVICE,
	                                  NM_VPN_EDITOR_PLUGIN_SERVICE);
}

static void
pptp_plugin_ui_init (PptpPluginUi *plugin)
{
}

static void
pptp_plugin_ui_interface_init (NMVpnEditorPluginInterface *iface_class)
{
	iface_class->get_editor = get_editor;
	iface_class->get_capabilities = get_capabilities;
	iface_class->import_from_file = import;
	iface_class->export_to_file = export;
	iface_class->get_suggested_filename = get_suggested_filename;
}

/*****************************************************************************/

G_MODULE_EXPORT NMVpnEditorPlugin *
nm_vpn_editor_plugin_factory (GError **error)
{
	if (error)
		g_return_val_if_fail (*error == NULL, NULL);

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	return NM_VPN_EDITOR_PLUGIN (g_object_new (PPTP_TYPE_PLUGIN_UI, NULL));
}

