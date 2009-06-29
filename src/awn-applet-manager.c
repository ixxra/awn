/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include "config.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

#include <libawn/libawn.h>
#include <libawn/awn-alignment.h>
#include <libawn/awn-config-bridge.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>


#include "awn-defines.h"
#include "awn-applet-manager.h"

#include "awn-applet-proxy.h"
#include "awn-throbber.h"

G_DEFINE_TYPE (AwnAppletManager, awn_applet_manager, GTK_TYPE_BOX) 

#define AWN_APPLET_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_MANAGER, AwnAppletManagerPrivate))

struct _AwnAppletManagerPrivate
{
  AwnConfigClient *client;

  AwnOrientation   orient;
  gint             offset;
  gint             size;
  
  /*TODO/FIXME  need to add ua_list and make requisite changes in functions.*/
  GSList          *applet_list;

  gboolean         expands;
  gint             expander_count;

  GHashTable      *applets;
  GHashTable      *extra_widgets;
  GQuark           touch_quark;
  GQuark           visibility_quark;

  /* Current box class */
  GtkWidgetClass  *klass;

};

typedef struct
{
  AwnAppletManager  * manager;

  double          ua_ratio;
  GtkWidget  *ua_alignment;
  GtkWidget  *socket;

  guint       notify_size_id;
  guint       notify_orient_id;
  guint       notify_offset_id;
}AwnUaInfo;

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_ORIENT,
  PROP_OFFSET,
  PROP_SIZE,
  PROP_APPLET_LIST,
  PROP_EXPANDS
};

enum
{
  APPLET_EMBEDDED,

  LAST_SIGNAL
};
static guint _applet_manager_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */
static void awn_applet_manager_set_size   (AwnAppletManager *manager,
                                           gint              size);
static void awn_applet_manager_set_orient (AwnAppletManager *manager, 
                                           gint              orient);
static void awn_applet_manager_set_offset (AwnAppletManager *manager,
                                           gint              offset);
static void free_list                     (GSList *list);

/*
 * GOBJECT CODE 
 */
static void
awn_applet_manager_constructed (GObject *object)
{
  AwnAppletManager        *manager;
  AwnAppletManagerPrivate *priv;
  AwnConfigBridge         *bridge;
  
  priv = AWN_APPLET_MANAGER_GET_PRIVATE (object);
  manager = AWN_APPLET_MANAGER (object);

  /* Hook everything up the config client */
  bridge = awn_config_bridge_get_default ();

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_ORIENT,
                          object, "orient");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                          object, "size");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                          object, "offset");
  awn_config_bridge_bind_list (bridge, priv->client,
                               AWN_GROUP_PANEL, AWN_PANEL_APPLET_LIST,
                               AWN_CONFIG_CLIENT_LIST_TYPE_STRING,
                               object, "applet_list");
}

static void
awn_applet_manager_size_request (GtkWidget *widget, GtkRequisition *requisition){
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER (widget)->priv;
  
  priv->klass->size_request (widget, requisition);
}

static void
awn_applet_manager_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER (widget)->priv;
  
  priv->klass->size_allocate (widget, allocation);
}

static void
awn_applet_manager_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (object));
  priv = AWN_APPLET_MANAGER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_pointer (value, priv->client);
      break;
    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;

    case PROP_APPLET_LIST:
      g_value_set_pointer (value, priv->applet_list);
      break;
    case PROP_EXPANDS:
      g_value_set_boolean (value, priv->expands);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_applet_manager_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  AwnAppletManager *manager = AWN_APPLET_MANAGER (object);
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (object));
  priv = AWN_APPLET_MANAGER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      priv->client =  g_value_get_pointer (value);
      break;
    case PROP_ORIENT:
      awn_applet_manager_set_orient (manager, g_value_get_int (value));
      break;
    case PROP_OFFSET:
      awn_applet_manager_set_offset (manager, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_applet_manager_set_size (manager, g_value_get_int (value));
      break;
    case PROP_APPLET_LIST:
      free_list (priv->applet_list);
      priv->applet_list = g_value_get_pointer (value);
      awn_applet_manager_refresh_applets (manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
awn_applet_manager_dispose (GObject *object)
{ 
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER_GET_PRIVATE (object);

  if (priv->applets)
  {
    g_hash_table_destroy (priv->applets);
    priv->applets = NULL;
  }

  if (priv->extra_widgets)
  {
    g_hash_table_destroy (priv->extra_widgets);
    priv->extra_widgets = NULL;
  }

  if (priv->klass)
  {
    g_type_class_unref (priv->klass);
    priv->klass = NULL;
  }

  G_OBJECT_CLASS (awn_applet_manager_parent_class)->dispose (object);
}

#include "awn-applet-manager-glue.h"

static void
awn_applet_manager_class_init (AwnAppletManagerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  
  obj_class->constructed   = awn_applet_manager_constructed;
  obj_class->dispose       = awn_applet_manager_dispose;
  obj_class->get_property  = awn_applet_manager_get_property;
  obj_class->set_property  = awn_applet_manager_set_property;

  wid_class->size_request  = awn_applet_manager_size_request;
  wid_class->size_allocate = awn_applet_manager_size_allocate;
    
  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "The AwnConfigClient",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The orientation of the panel",
                      0, 3, AWN_ORIENTATION_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "The icon offset of the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_APPLET_LIST,
    g_param_spec_pointer ("applet_list",
                          "Applet List",
                          "The list of applets for this panel",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_EXPANDS,
    g_param_spec_boolean ("expands",
                          "Expands",
                          "Determines whether there's an expander",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /* Class signals */
  _applet_manager_signals[APPLET_EMBEDDED] =
    g_signal_new("applet-embedded",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletManagerClass, applet_embedded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
 
  g_type_class_add_private (obj_class, sizeof (AwnAppletManagerPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), 
                                   &dbus_glib_awn_ua_object_info);

}

static void
awn_applet_manager_init (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;
  DBusGConnection *connection;
  GError                *error = NULL;

  priv = manager->priv = AWN_APPLET_MANAGER_GET_PRIVATE (manager);

  priv->touch_quark = g_quark_from_string ("applets-touch-quark");
  priv->visibility_quark = g_quark_from_string ("visibility-quark");
  priv->applets = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, NULL);
  priv->extra_widgets = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_widget_show_all (GTK_WIDGET (manager));

  /* Grab a connection to the bus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL)
  {
    g_warning ("Unable to make connection to the D-Bus session bus: %s",
               error->message);
    g_error_free (error);
    gtk_main_quit ();
  }

 dbus_g_connection_register_g_object (connection, 
                                       AWN_DBUS_MANAGER_PATH, 
					G_OBJECT (GTK_WIDGET (manager)));

}

GtkWidget *
awn_applet_manager_new_from_config (AwnConfigClient *client)
{
  GtkWidget *manager;
  
  manager = g_object_new (AWN_TYPE_APPLET_MANAGER,
                         "homogeneous", FALSE,
                         "spacing", 0,
                         "client", client,
                         NULL);
  return manager;
}

void
awn_applet_manager_set_applet_flags (AwnAppletManager *manager,
                                     const gchar *uid,
                                     AwnAppletFlags flags)
{
  AwnAppletProxy *applet;
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  // FIXME: separators!
  //if (flags & (AWN_APPLET_IS_SEPARATOR | AWN_APPLET_IS_EXPANDER) == 0) return;
  if ((flags & AWN_APPLET_IS_EXPANDER) == 0) return;

  priv = manager->priv;

  applet = g_hash_table_lookup (priv->applets, uid);
  if (applet && AWN_IS_APPLET_PROXY (applet))
  {
    // dummy widget that will expand
    GtkWidget *image = gtk_image_new ();
    gtk_widget_show (image);
    gtk_box_pack_start (GTK_BOX (manager), image, TRUE, TRUE, 0);

    priv->expander_count++;

    g_hash_table_replace (priv->applets, g_strdup (uid), image);
    gtk_widget_destroy (GTK_WIDGET (applet));

    awn_applet_manager_refresh_applets (manager);

    if (!priv->expands)
    {
      priv->expands = TRUE;
      g_object_notify (G_OBJECT (manager), "expands");
    }
  }
}

gboolean
awn_applet_manager_get_expands (AwnAppletManager *manager)
{
  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager), FALSE);

  return manager->priv->expands;
}

/*
 * PROPERTY SETTERS
 */

static void
awn_manager_set_applets_size (gpointer key,
                              GtkWidget *applet,
                              AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "size"))
  {
    g_object_set (applet, "size", manager->priv->size, NULL);
  }
}

static void
awn_applet_manager_set_size (AwnAppletManager *manager,
                             gint              size)
{
  AwnAppletManagerPrivate *priv = manager->priv;

  priv->size = size;

  /* update size on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_size, manager);
}

static void
awn_manager_set_applets_offset (gpointer key,
                                GtkWidget *applet,
                                AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "offset"))
  {
    g_object_set (applet, "offset", manager->priv->offset, NULL);
  }
}

static void
awn_applet_manager_set_offset (AwnAppletManager *manager,
                               gint              offset)
{
  AwnAppletManagerPrivate *priv = manager->priv;

  priv->offset = offset;

  /* update size on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_offset, manager);
}

static void
awn_manager_set_applets_orient (gpointer key,
                                GtkWidget *applet,
                                AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "orient"))
  {
    g_object_set (applet, "orient", manager->priv->orient, NULL);
  }
}

/*
 * Update the box class
 */
static void 
awn_applet_manager_set_orient (AwnAppletManager *manager, 
                               gint              orient)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  
  priv->orient = orient;

  if (priv->klass)
  {
    g_type_class_unref (priv->klass);
    priv->klass = NULL;
  }
//  g_debug ("%s, orient = %d",__func__,priv->orient);
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
#if GTK_CHECK_VERSION(2, 15, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE(manager), GTK_ORIENTATION_HORIZONTAL);
#endif
      priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_HBOX));
      break;
    
    case AWN_ORIENTATION_RIGHT:
    case AWN_ORIENTATION_LEFT:
#if GTK_CHECK_VERSION(2, 15, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE(manager), GTK_ORIENTATION_VERTICAL);
#endif
      priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_VBOX));
      break;

    default:
      g_assert_not_reached ();
      priv->klass = NULL;
      break;
  }

  /* update orientation on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_orient, manager);
}

/*
 * UTIL
 */
static void
free_list (GSList *list)
{
  GSList *l;

  for (l = list; l; l = l->next)
  {
    g_free (l->data);
  }
  g_slist_free (list);
}

/*
 * APPLET CONTROL
 */
static void
_applet_plug_added (AwnAppletManager *manager, GtkWidget *applet)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  g_signal_emit (manager, _applet_manager_signals[APPLET_EMBEDDED], 0, applet);
}

static GtkWidget *
create_applet (AwnAppletManager *manager, 
               const gchar      *path,
               const gchar      *uid)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GtkWidget               *applet;
  GtkWidget               *notifier;

  /*FIXME: Exception cases, i.e. separators */
  
  applet = awn_applet_proxy_new (path, uid, priv->orient,
                                 priv->offset, priv->size);
  g_signal_connect_swapped (applet, "plug-added",
                            G_CALLBACK (_applet_plug_added), manager);
  notifier = awn_applet_proxy_get_throbber (AWN_APPLET_PROXY (applet));

  gtk_box_pack_start (GTK_BOX (manager), applet, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (manager), notifier, FALSE, FALSE, 0);
  gtk_widget_show (notifier);

  g_object_set_qdata (G_OBJECT (applet), 
                      priv->touch_quark, GINT_TO_POINTER (0));
  g_hash_table_insert (priv->applets, g_strdup (uid), applet);

  awn_applet_proxy_schedule_execute (AWN_APPLET_PROXY (applet));

  return applet;
}

static void
zero_applets (gpointer key, GtkWidget *applet, AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet))
  {
    g_object_set_qdata (G_OBJECT (applet), 
                        manager->priv->touch_quark, GINT_TO_POINTER (0));
  }
}

static gboolean
delete_applets (gpointer key, GtkWidget *applet, AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  const gchar             *uid;
  gint                     touched;
  
  if (!G_IS_OBJECT (applet))
    return TRUE;
  
  touched = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (applet),
                                                 priv->touch_quark));

  if (!touched)
  {
    if (AWN_IS_APPLET_PROXY (applet))
    {
      g_object_get (applet, "uid", &uid, NULL);
      /* FIXME: Let the applet know it's about to be deleted ? */
    }
    else if (GTK_IS_IMAGE (applet)) // expander
    {
      priv->expander_count--;
      if (priv->expander_count == 0 && priv->expands)
      {
        priv->expands = FALSE;
        g_object_notify (G_OBJECT (manager), "expands");
      }
    }
    gtk_widget_destroy (applet);

    return TRUE; /* remove from hash table */
  }

  return FALSE;
}

void    
awn_applet_manager_refresh_applets  (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GSList                  *a;
  GList                   *w;
  gint                     i = 0;
  gint                     applet_num = 0;

  g_debug ("%s",__func__);
  if (!GTK_WIDGET_REALIZED (manager))
    return;

  if (priv->applet_list == NULL)
  {
    g_debug ("No applets");
    return;
  }

  guint applet_count = g_slist_length (priv->applet_list);
  gint back_pos = (gint)applet_count;
  back_pos = -back_pos;

  /* Set each of the current apps as "untouched" */
  g_hash_table_foreach (priv->applets, (GHFunc)zero_applets, manager);

  GList *widgets = g_hash_table_get_keys (priv->extra_widgets);
  /* Go through the list of applets. Re-order those that are already active, 
   * and create those that are not
   */
  for (a = priv->applet_list; a; a = a->next)
  {
    GtkWidget *applet = NULL;
    gchar     **tokens;

    /* Get the two tokens from the saved string, where:
     * tokens[0] == path to applet desktop file &
     * tokens[1] == uid of applet
     */
    tokens = g_strsplit (a->data, "::", 2);

    if (tokens == NULL || g_strv_length (tokens) != 2)
    {
      g_warning ("Bad applet key: %s", (gchar*)a->data);
      continue;
    }

//    g_debug ("%s:  %s, %s",__func__,tokens[0],tokens[1]);
    /* See if the applet already exists */
    applet = g_hash_table_lookup (priv->applets, tokens[1]);

    /* If not, create it */
    if (applet == NULL)
    {
      g_debug ("applet does not exist");
      applet = create_applet (manager, tokens[0], tokens[1]);
      if (!applet)
      {
        g_strfreev (tokens);
        continue;
      }
    }

    /* insert extra widgets on correct position */
    w = widgets;
    while (w)
    {
      gint pos = GPOINTER_TO_INT (g_hash_table_lookup (priv->extra_widgets,
                                                       w->data));

      if (pos == applet_num || pos == back_pos)
      {
        gtk_box_reorder_child (GTK_BOX (manager), w->data, i++);
        /* widget placed, remove from list */
        widgets = g_list_remove (widgets, w->data);
        w = widgets;
        continue;
      }

      w = w->next;
    }
    applet_num++; back_pos++;

    /* Order the applet correctly */
    gtk_box_reorder_child (GTK_BOX (manager), applet, i++);
    if (AWN_IS_APPLET_PROXY (applet))
    {
      gtk_box_reorder_child (GTK_BOX (manager),
               awn_applet_proxy_get_throbber (AWN_APPLET_PROXY (applet)), i++);
    }
    
    /* Make sure we don't kill it during clean up */
    g_object_set_qdata (G_OBJECT (applet), 
                        priv->touch_quark, GINT_TO_POINTER (1));

    g_strfreev (tokens);
  }

  g_list_free (widgets);

  /* Delete applets that have been removed from the list */
  g_hash_table_foreach_remove (priv->applets, (GHRFunc)delete_applets, manager);
}

void
awn_applet_manager_add_widget (AwnAppletManager *manager,
                               GtkWidget *widget, gint pos)
{
  gpointer key, val;
  AwnAppletManagerPrivate *priv = manager->priv;

  if (!g_hash_table_lookup_extended (priv->extra_widgets, widget, &key, &val))
  {
    gtk_box_pack_start (GTK_BOX (manager), widget, FALSE, FALSE, 0);
    /* caller is supposed to call gtk_widget_show! */
  }
  g_print ("Gint : %i : ", pos);
  g_hash_table_replace (priv->extra_widgets, widget, GINT_TO_POINTER (pos));

  awn_applet_manager_refresh_applets (manager);
}

void
awn_applet_manager_remove_widget (AwnAppletManager *manager, GtkWidget *widget)
{
  AwnAppletManagerPrivate *priv = manager->priv;

  if (g_hash_table_remove (priv->extra_widgets, widget))
  {
    gtk_container_remove (GTK_CONTAINER (manager), widget);
  }
}


/*UA*/


static void
awn_ua_offset_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUaInfo * ua_info = user_data;  
  AwnAppletManager * manager = ua_info->manager;  
  AwnAppletManagerPrivate *priv = manager->priv;  
  GtkWidget * align = ua_info->ua_alignment;
  g_debug ("%s",__func__);  
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      gtk_alignment_set_padding (GTK_ALIGNMENT(align), priv->offset, 0, 0, 0);
      break;
    case AWN_ORIENTATION_BOTTOM:
      gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, priv->offset, 0, 0);
      break;
    case AWN_ORIENTATION_LEFT:
      gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, 0, priv->offset, 0);
      break;
    case AWN_ORIENTATION_RIGHT:
      gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, 0, 0, priv->offset);
      break;
    default:
      g_assert_not_reached();
  }
}

static void
awn_ua_orient_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUaInfo * ua_info = user_data;
  AwnAppletManager * manager = ua_info->manager;
  AwnAppletManagerPrivate *priv = manager->priv;
  GtkWidget * align = ua_info->ua_alignment; 
  gint orient = priv->orient;
  g_debug ("%s",__func__);  
  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      gtk_alignment_set (GTK_ALIGNMENT(align), 0.0, 0.0, 1.0, 0.5);
      break;
    case AWN_ORIENTATION_BOTTOM:
      gtk_alignment_set (GTK_ALIGNMENT(align), 0.0, 1.0, 1.0, 0.5);
      break;
    case AWN_ORIENTATION_LEFT:
      gtk_alignment_set (GTK_ALIGNMENT(align), 0.0, 0.0, 0.5, 1.0);
      break;
    case AWN_ORIENTATION_RIGHT:
      gtk_alignment_set (GTK_ALIGNMENT(align), 1.0, 0.0, 0.5, 1.0);
      break;
    default:
      g_assert_not_reached();      
  }
  awn_ua_offset_change (object,param_spec,user_data);    
  gtk_widget_show_all (align);
}

static void
awn_ua_size_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUaInfo * ua_info = user_data;  
  AwnAppletManager * manager = ua_info->manager;  
  AwnAppletManagerPrivate *priv = manager->priv;  
  GtkRequisition  req;
  gint size = priv->size;
  g_debug ("%s",__func__);  
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:      
      req.width = size * ua_info->ua_ratio;
      req.height = size;
      break;
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
      req.width = size;
      req.height = size  * 1.0 / ua_info->ua_ratio;            
      break;
    default:
      g_assert_not_reached();
  }
  gtk_widget_set_size_request (ua_info->socket,req.width,req.height);
}

static gboolean
awn_ua_plug_removed (GtkSocket *socket,AwnUaInfo * info)
{
  g_debug ("%s: plug removed",__func__);\
  awn_applet_manager_remove_widget(info->manager, GTK_WIDGET (info->ua_alignment));
  g_signal_handler_disconnect (info->manager,info->notify_size_id);
  g_signal_handler_disconnect (info->manager,info->notify_orient_id);
  g_signal_handler_disconnect (info->manager,info->notify_offset_id);
  g_free (info);  
  return FALSE;
}
/*DBUS*/
/*
 	@action(IFACE)
	def add_applet (self, id, plug_id, width, height, size_type):
		"""
		Add an applet.
		
		id: A unique string used to identify the applet.
		plug_id: The applet's gtk.Plug's xid.
		width: A recommended width. This will be interpreted according to size_type.
		height A recommended height. This will be interpreted according to size_type.
		size_type: Determines the meaning of width and height.
			May be one of the following values:
			"scalable"- The applet may be resized as long as the width/height ratio is kept.
			"static"- The applet should be displayed at exactly the size requested.
			"static-width"-	The applet's width should remain static, and the server may change the height.
			"static-height"- The applet's height should remain static, and the server may change the width.
			"dynamic"- The applet may be resized to any size.
		desktop_path: Path to the desktop file.
		"""
		# NOTE: Melange currently ignores the size_type parameter.
		container = ToplevelContainer(plug_id, id, self, width, height,
			size_type, backend=self.backend)
		self.containers.append(container)
*/

gboolean
awn_ua_add_applet (	AwnAppletManager *manager,
			gchar     *name,
      glong		  xid,
			gint	    width,
			gint      height,
			gchar     *size_type,
      GError   **error)
{
  g_debug ("size type = '%s'",size_type); 
  g_return_val_if_fail ( (g_strcmp0(size_type,"scalable")==0 ) || 
                     (g_strcmp0(size_type,"dynamic")==0 ), FALSE );
  AwnUaInfo * ua_info = g_malloc (sizeof (AwnUaInfo) );
  GtkWidget *socket = gtk_socket_new ();
  static gint pos = 2;
  GdkWindow* plugwin; 
  AwnAppletManagerPrivate *priv = manager->priv;  
  GdkNativeWindow native_window = (GdkNativeWindow) xid;

  ua_info->socket = socket;
  ua_info->manager = manager;
  ua_info->ua_ratio = width / (double) height;
  ua_info->ua_alignment = gtk_alignment_new(0.0, 0.0, 0.0, 0.0); 

  awn_ua_orient_change (NULL, NULL, ua_info);

  socket = gtk_socket_new ();
  g_signal_connect_swapped (socket, "plug-added",
                          G_CALLBACK (_applet_plug_added), manager);

  gtk_container_add (GTK_CONTAINER(ua_info->ua_alignment),socket);  
  awn_applet_manager_add_widget(manager, GTK_WIDGET (ua_info->ua_alignment), pos);  
  gtk_socket_add_id (GTK_SOCKET(socket), native_window);
  plugwin = gtk_socket_get_plug_window (GTK_SOCKET(socket));  
  g_object_set_qdata (G_OBJECT (ua_info->ua_alignment), 
                      priv->touch_quark, GINT_TO_POINTER (0));    
  gtk_widget_realize (GTK_WIDGET (socket));  
  gtk_widget_set_size_request (ua_info->ua_alignment,
                               priv->size * ua_info->ua_ratio,
                               priv->size);
  gtk_widget_show_all (ua_info->ua_alignment);
  if (!plugwin)
  {
    gtk_widget_destroy (socket);
    g_warning ("UA Plug was not created within socket.");
    g_free (ua_info);
  }
  ua_info->notify_offset_id = g_signal_connect (manager,"notify::offset",G_CALLBACK(awn_ua_offset_change),ua_info);
  ua_info->notify_orient_id = g_signal_connect_after (manager,"notify::orient",G_CALLBACK(awn_ua_orient_change),ua_info);
  ua_info->notify_size_id = g_signal_connect_after (manager,"notify::size",G_CALLBACK(awn_ua_size_change),ua_info);  
  g_signal_connect (socket,"plug-removed",G_CALLBACK(awn_ua_plug_removed),ua_info);

  return TRUE;
}

gboolean
awn_ua_get_all_server_flags (	AwnAppletManager *manager,
				GHashTable *hash,
				gchar     *name,
				GError   **error)
{
/* Furtur function to return capability of the server
For now, it return nothing*/
/*hash = g_hash_table_new (NULL, NULL);*/
return TRUE;

}


/*End DBUS*/
void
awn_applet_manager_show_applets (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GList *list = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *it = list; it != NULL; it = it->next)
  {
    if (AWN_IS_THROBBER (it->data)) continue;
    if (AWN_IS_APPLET_PROXY (it->data))
    {
      AwnAppletProxy *proxy = AWN_APPLET_PROXY (it->data);
      if (gtk_socket_get_plug_window (GTK_SOCKET (proxy)))
        gtk_widget_show (GTK_WIDGET (proxy));
      else
        gtk_widget_show (awn_applet_proxy_get_throbber (proxy));
    }
    else
    {
      int was_visible = GPOINTER_TO_INT (g_object_get_qdata (it->data,
                                         priv->visibility_quark));
      if (was_visible) gtk_widget_show (GTK_WIDGET (it->data));
    }
  }

  g_list_free (list);
}

void
awn_applet_manager_hide_applets (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GList *list = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *it = list; it != NULL; it = it->next)
  {
    if (!AWN_IS_THROBBER (it->data) && !AWN_IS_APPLET_PROXY (it->data))
    {
      g_object_set_qdata (it->data, priv->visibility_quark,
                          GINT_TO_POINTER (GTK_WIDGET_VISIBLE (it->data) ?
                            1 : 0));
    }

    gtk_widget_hide (GTK_WIDGET (it->data));
  }

  g_list_free (list);
}

