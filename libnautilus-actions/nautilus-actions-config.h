/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifndef __NAUTILUS_ACTIONS_CONFIG_H__
#define __NAUTILUS_ACTIONS_CONFIG_H__

#include <glib/glist.h>
#include <glib-object.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

G_BEGIN_DECLS

/* Error data */
#define NAUTILUS_ACTIONS_CONFIG_ERROR g_quark_from_string ("nautilus_actions_config")

typedef enum
{
	NAUTILUS_ACTIONS_CONFIG_ERROR_FAILED
} NautilusActionsConfigError;

#define NAUTILUS_ACTIONS_TYPE_CONFIG            (nautilus_actions_config_get_type())
#define NAUTILUS_ACTIONS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, NAUTILUS_ACTIONS_TYPE_CONFIG, NautilusActionsConfig))
#define NAUTILUS_ACTIONS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, NAUTILUS_ACTIONS_TYPE_CONFIG, NautilusActionsConfigClass))
#define NAUTILUS_ACTIONS_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, NAUTILUS_ACTIONS_TYPE_CONFIG))
#define NAUTILUS_ACTIONS_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), NAUTILUS_ACTIONS_TYPE_CONFIG))
#define NAUTILUS_ACTIONS_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), NAUTILUS_ACTIONS_TYPE_CONFIG, NautilusActionsConfigClass))

/* i18n notes : default profile name displayed in the profile list in
 * the action edition dialog
 */
#define NAUTILUS_ACTIONS_DEFAULT_PROFILE_DESC_NAME _("Main")
#define NAUTILUS_ACTIONS_DEFAULT_PROFILE_NAME "main"

/* i18n notes : default profile name displayed in the profile list in
 * the action edition dialog when more than one profile is created
 * (incremented each time) (please keep the string lowercase if possible)
 */
#define NAUTILUS_ACTIONS_DEFAULT_OTHER_PROFILE_DESC_NAME _("Profile %d")
#define NAUTILUS_ACTIONS_DEFAULT_OTHER_PROFILE_NAME "profile%d"

typedef struct {
	gchar *desc_name;
	gchar *path;
	gchar *parameters;
	gboolean match_case;
	GSList *basenames;
	GSList *mimetypes;
	gboolean is_dir;
	gboolean is_file;
	gboolean accept_multiple_files;
	GSList *schemes;
} NautilusActionsConfigActionProfile;

typedef struct {
	gchar *conf_section;
	gchar *uuid;
	gchar *label;
	gchar *tooltip;
	gchar *icon;
	GHashTable *profiles; /* Hash of NautilusActionsConfigActionProfile* */
	gchar *version;
} NautilusActionsConfigAction;

typedef struct _NautilusActionsConfig NautilusActionsConfig;
typedef struct _NautilusActionsConfigClass NautilusActionsConfigClass;

struct _NautilusActionsConfig {
	GObject parent;

	/* Private data, don't access */
	GHashTable *actions;
};

struct _NautilusActionsConfigClass {
	GObjectClass parent_class;

	/* Virtual private function */
	gboolean (* save_action) (NautilusActionsConfig *config, NautilusActionsConfigAction *action);
	gboolean (* remove_action) (NautilusActionsConfig *config, NautilusActionsConfigAction *action);

	/* Signals handler signature */
	void (* action_added) (NautilusActionsConfig *config, NautilusActionsConfigAction *action, gpointer user_data);
	void (* action_changed) (NautilusActionsConfig *config, NautilusActionsConfigAction *action, gpointer user_data);
	void (* action_removed) (NautilusActionsConfig *config, NautilusActionsConfigAction *action, gpointer user_data);
};

GType                        nautilus_actions_config_get_type (void);

NautilusActionsConfigAction *nautilus_actions_config_get_action (NautilusActionsConfig *config, const gchar *uuid);
GSList                      *nautilus_actions_config_get_actions (NautilusActionsConfig *config);

/* function to free a list returned by nautilus_actions_config_get_actions () */
void                         nautilus_actions_config_free_actions_list (GSList *list);
gboolean                     nautilus_actions_config_add_action (NautilusActionsConfig *config,
								 NautilusActionsConfigAction *action, GError** error);
gboolean                     nautilus_actions_config_update_action (NautilusActionsConfig *config,
								    NautilusActionsConfigAction *action);
gboolean                     nautilus_actions_config_remove_action (NautilusActionsConfig *config,
								    const gchar *label);

/* function to clear the actions list stored in the nautilus_actions_config object */
gboolean		     nautilus_actions_config_clear (NautilusActionsConfig *config);

NautilusActionsConfigActionProfile *nautilus_actions_config_action_profile_new (void);
NautilusActionsConfigActionProfile *nautilus_actions_config_action_profile_new_default (void);
gboolean                     nautilus_actions_config_action_profile_exists (NautilusActionsConfigAction *action,
									 const gchar* profile_name);
GSList				*nautilus_actions_config_action_get_all_profile_names (NautilusActionsConfigAction *action);
void                 nautilus_actions_config_action_free_all_profile_names( GSList *list );
gchar               *nautilus_actions_config_action_get_first_profile_name( const NautilusActionsConfigAction *action );
void				 nautilus_actions_config_action_get_new_default_profile_name (NautilusActionsConfigAction *action,
									gchar** new_profile_name,
									gchar** new_profile_desc_name);
NautilusActionsConfigActionProfile *nautilus_actions_config_action_get_profile (NautilusActionsConfigAction *action,
									 const gchar* profile_name);
NautilusActionsConfigActionProfile *nautilus_actions_config_action_get_or_create_profile (NautilusActionsConfigAction *action,
									 const gchar* profile_name);
gboolean                     nautilus_actions_config_action_add_profile (NautilusActionsConfigAction *action,
									 const gchar* profile_name,
								 	 NautilusActionsConfigActionProfile* profile,
									 GError** error);
void                         nautilus_actions_config_action_replace_profile (NautilusActionsConfigAction *action,
									 const gchar* profile_name,
								 	 NautilusActionsConfigActionProfile* profile);
gboolean                     nautilus_actions_config_action_remove_profile (NautilusActionsConfigAction *action,
									 const gchar* profile_name);
gboolean                     nautilus_actions_config_action_rename_profile (NautilusActionsConfigAction *action,
									 const gchar* old_profile_name,
								 	 const gchar* new_profile_name,
									 GError** error);
guint                        nautilus_actions_config_action_get_profiles_count( const NautilusActionsConfigAction *action );


NautilusActionsConfigAction *nautilus_actions_config_action_new (void);
NautilusActionsConfigAction *nautilus_actions_config_action_new_default (void);

void                         nautilus_actions_config_action_set_uuid (NautilusActionsConfigAction *action,
								       const gchar *uuid);
void                         nautilus_actions_config_action_set_label (NautilusActionsConfigAction *action,
								       const gchar *label);
void                         nautilus_actions_config_action_set_tooltip (NautilusActionsConfigAction *action,
									 const gchar *tooltip);
void                         nautilus_actions_config_action_set_icon (NautilusActionsConfigAction *action,
									 const gchar *icon);
void                         nautilus_actions_config_action_profile_set_desc_name (NautilusActionsConfigActionProfile *action_profile,
								      const gchar *desc_name);
void                         nautilus_actions_config_action_profile_set_path (NautilusActionsConfigActionProfile *action_profile,
								      const gchar *path);
void                         nautilus_actions_config_action_profile_set_parameters (NautilusActionsConfigActionProfile *action_profile,
									    const gchar *parameters);
void                         nautilus_actions_config_action_profile_set_basenames (NautilusActionsConfigActionProfile *action_profile,
										 GSList *basenames);
void                         nautilus_actions_config_action_profile_set_mimetypes (NautilusActionsConfigActionProfile *action_profile,
										 GSList *mimetypes);
void                         nautilus_actions_config_action_profile_set_schemes (NautilusActionsConfigActionProfile *action_profile,
										 GSList *schemes);

#define nautilus_actions_config_action_profile_set_match_case(action_profile, b) { if ((action_profile)) (action_profile)->match_case = b; }
#define nautilus_actions_config_action_profile_set_is_dir(action_profile, b) { if ((action_profile)) (action_profile)->is_dir = b; }
#define nautilus_actions_config_action_profile_set_is_file(action_profile, b) { if ((action_profile)) (action_profile)->is_file = b; }
#define nautilus_actions_config_action_profile_set_accept_multiple(action_profile, b) { if ((action_profile)) (action_profile)->accept_multiple_files = b; }

NautilusActionsConfigActionProfile *nautilus_actions_config_action_profile_dup (NautilusActionsConfigActionProfile *action_profile);
void                         nautilus_actions_config_action_profile_free (NautilusActionsConfigActionProfile *action_profile);

NautilusActionsConfigAction *nautilus_actions_config_action_dup (NautilusActionsConfigAction *action);
NautilusActionsConfigAction *nautilus_actions_config_action_dup_new (NautilusActionsConfigAction *action);
void                         nautilus_actions_config_action_free (NautilusActionsConfigAction *action);
void                         nautilus_actions_config_action_dump (NautilusActionsConfigAction *action);

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_CONFIG_H__ */
