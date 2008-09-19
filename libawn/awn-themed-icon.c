/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>

#include "awn-themed-icon.h"

G_DEFINE_TYPE (AwnThemedIcon, awn_themed_icon, AWN_TYPE_ICON);

#define AWN_THEMED_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THEMED_ICON, \
  AwnThemedIconPrivate))

#define LOAD_FLAGS GTK_ICON_LOOKUP_FORCE_SVG
#define AWN_ICON_THEME_NAME "awn-theme"

struct _AwnThemedIconPrivate
{
  GtkIconTheme *awn_theme;
  GtkIconTheme *override_theme;
  GtkIconTheme *gtk_theme;
  gchar        *icon_dir;
  
  gchar  *applet_name;
  gchar  *uid;
  gchar **states;
  gchar **icon_names;
  gint    n_states;

  gchar  *current_state;
  gint    current_size;
};

enum
{
  SCOPE_UID=0,
  SCOPE_APPLET,
  SCOPE_AWN_THEME,
  SCOPE_OVERRIDE_THEME,
  SCOPE_GTK_THEME,
  SCOPE_FALLBACK_STOP,
  SCOPE_FALLBACK_FILL,

  N_SCOPES
};

/* Forwards */
void on_icon_theme_changed (GtkIconTheme *theme, AwnThemedIcon *icon);

/* GObject stuff */
static void
awn_themed_icon_dispose (GObject *object)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (object));
  priv = AWN_THEMED_ICON (object)->priv;

  g_strfreev (priv->states);
  g_strfreev (priv->icon_names);
  g_free (priv->applet_name);
  g_free (priv->uid);
  g_free (priv->current_state);
  g_object_unref (priv->awn_theme);
  g_free (priv->icon_dir);

  G_OBJECT_CLASS (awn_themed_icon_parent_class)->dispose (object);
}

static void
awn_themed_icon_class_init (AwnThemedIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  
  obj_class->dispose = awn_themed_icon_dispose;

  g_type_class_add_private (obj_class, sizeof (AwnThemedIconPrivate));
}

static void 
check_dest_or_copy (const gchar *src, const gchar *dest)
{
  GFile  *from;
  GFile  *to;
  GError *error = NULL;

  if (g_file_test (dest, G_FILE_TEST_EXISTS))
    return;

  from = g_file_new_for_path (src);
  to = g_file_new_for_path (dest);

  g_file_copy (from, to, 0, NULL, NULL, NULL, &error);

  if (error)
  {
    g_warning ("Unable to copy %s to %s: %s", src, dest, error->message);
    g_error_free (error);
  }

  g_object_unref (to);
  g_object_unref (from);
}


static void
check_and_make_dir (const gchar *dir)
{
  if (!g_file_test (dir, G_FILE_TEST_EXISTS))
  {
    g_mkdir (dir, 0755);
  }
}

static void
awn_themed_icon_init (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  gchar                *icon_dir;
  gchar                *theme_dir;
  gchar                *scalable_dir;
  gchar                *index_src;
  gchar                *index_dest;

  priv = icon->priv = AWN_THEMED_ICON_GET_PRIVATE (icon);

  priv->applet_name = NULL;
  priv->uid = NULL;
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->current_state = NULL;
  priv->current_size = 48;

  /* Set-up the gtk-theme */
  priv->gtk_theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->gtk_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);

  /* 
   * Set-up our special theme. We need to check for all the dirs
   */

  /* First check all the directories */
  icon_dir = priv->icon_dir = g_strdup_printf ("%s/.icons", g_get_home_dir ());
  check_and_make_dir (icon_dir);

  theme_dir = g_strdup_printf ("%s/%s", icon_dir, AWN_ICON_THEME_NAME);
  check_and_make_dir (theme_dir);
  
  scalable_dir = g_strdup_printf ("%s/scalable", theme_dir);
  check_and_make_dir (scalable_dir);

  /* Copy over the index.theme if it's not already done */
  index_src = g_strdup (PKGDATADIR"/index.theme");
  index_dest = g_strdup_printf ("%s/index.theme", theme_dir);
  check_dest_or_copy (index_src, index_dest);
  g_free (index_src);
  g_free (index_dest);

  /* Now let's make our custom theme */
  priv->awn_theme = gtk_icon_theme_new ();
  gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);
  g_signal_connect (priv->awn_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);
  
  g_free (scalable_dir);
  g_free (theme_dir);
}

GtkWidget *
awn_themed_icon_new (void)

{
  GtkWidget *themed_icon = NULL;

  themed_icon = g_object_new (AWN_TYPE_THEMED_ICON, 
                              NULL);

  return themed_icon;
}

/*
 * Main function to get the correct icon for a size
 */
static GdkPixbuf *
get_pixbuf_at_size (AwnThemedIcon *icon, gint size)
{
  AwnThemedIconPrivate *priv;
  GdkPixbuf            *pixbuf = NULL;
  gint                  index;

  priv = icon->priv;

  /* Find the index of the current state in states */
  for (index = 0; priv->states[index]; index++)
  {
    if (strcmp (priv->states[index], priv->current_state) == 0)
    {
      const gchar *applet_name;
      const gchar *icon_name;
      const gchar *uid;
      gint         i;
      
      applet_name = priv->applet_name;
      icon_name = priv->icon_names[index];
      uid = priv->uid;
      
      /* Go through all the possible outcomes until we get a pixbuf */
      for (i = 0; i < N_SCOPES; i++)
      {
        gchar *name = NULL;
        switch (i)
        {
          case SCOPE_UID:
            name = g_strdup_printf ("%s-%s-%s", icon_name, applet_name, uid);
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_APPLET:
            name = g_strdup_printf ("%s-%s", icon_name, applet_name);
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_AWN_THEME:
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, icon_name, 
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_OVERRIDE_THEME:
            pixbuf = NULL;
            if (priv->override_theme)
              pixbuf = gtk_icon_theme_load_icon (priv->override_theme,
                                                 icon_name, 
                                                 size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_GTK_THEME:
            pixbuf = gtk_icon_theme_load_icon (priv->gtk_theme, icon_name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_FALLBACK_STOP:
            pixbuf = gtk_icon_theme_load_icon (priv->gtk_theme,
                                               GTK_STOCK_MISSING_IMAGE,
                                               size, LOAD_FLAGS, NULL);
            break;

          default:
            pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 0, size, size);
            gdk_pixbuf_fill (pixbuf, 0xee221155);
            break;
        }

        /* Check if we got a valid pixbuf on this run */
        g_free (name);

        if (pixbuf)
        {
          /* FIXME: Should we make this orientation-aware? */
          if (gdk_pixbuf_get_height (pixbuf) > size)
          {
            GdkPixbuf *temp = pixbuf;
            gint       width, height;

            width = gdk_pixbuf_get_width (temp);
            height = gdk_pixbuf_get_height (temp);

            pixbuf = gdk_pixbuf_scale_simple (temp, width*size/height, size,
                                              GDK_INTERP_HYPER);
            g_object_unref (temp);
          }
          break;
        }
      }

    }
    g_warning ("state does not exist: %s", priv->current_state);
  }

  return pixbuf;
}


/*
 * Main function to ensure the icon
 */
static void
ensure_icon (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  GdkPixbuf            *pixbuf;

  priv = icon->priv;

  if (!priv->n_states || !priv->states || !priv->icon_names 
      || !priv->current_state || !priv->current_size)
  {
    /* We're not ready yet */
    return;
  }

  /* Get the icon first */
  pixbuf = get_pixbuf_at_size (icon, priv->current_size);

  awn_icon_set_from_pixbuf (AWN_ICON (icon), pixbuf);

  g_object_unref (pixbuf);
}

/*
 * Public functions
 */

void  
awn_themed_icon_set_state (AwnThemedIcon *icon,
                           const gchar   *state)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  if (priv->current_state)
    g_free (priv->current_state);

  priv->current_state = g_strdup (state);
  ensure_icon (icon);
}

void 
awn_themed_icon_set_size (AwnThemedIcon *icon,
                          gint           size)
{
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  icon->priv->current_size = size;
  ensure_icon (icon);
}

void
awn_themed_icon_set_info (AwnThemedIcon  *icon,
                          const gchar    *applet_name,
                          const gchar    *uid,
                          gchar         **states,
                          gchar         **icon_names)
{
  AwnThemedIconPrivate *priv;
  gint n_states;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  g_return_if_fail (applet_name);
  g_return_if_fail (uid);
  g_return_if_fail (states);
  g_return_if_fail (icon_names);
  priv = icon->priv;

  /* Check number of strings>0 and n_strings (states)==n_strings (icon_names)*/
  n_states = g_strv_length (states);
  if (n_states < 1 || n_states != g_strv_length (icon_names))
  {
    g_warning ("%s", n_states ? 
                       "Length of states must match length of icon_names" 
                       : "Length of states must be greater than 0");
    return;
  }
  
  /* Free the old states & icon_names */
  g_strfreev (priv->states);
  g_strfreev (priv->icon_names);
  priv->states = NULL;
  priv->icon_names = NULL;

  /* Copy states & icon_names internally */
  priv->states = g_strdupv (states);
  priv->icon_names = g_strdupv (icon_names);
  priv->n_states = n_states;
  
  /* Now add the rest of the entries */
  g_free (priv->uid);
  priv->uid = g_strdup (uid);

  /* Finally set-up the applet name & theme information */
  if (priv->applet_name && strcmp (priv->applet_name, applet_name) == 0)
  {
    /* Already appended the search path to the GtkIconTheme, so we skip this */
  }
  else
  {
    gchar *search_dir;

    g_free (priv->applet_name);
    priv->applet_name = g_strdup (applet_name);

    /* Add the applet's system-wide icon dir first */
    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/icons", applet_name);
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir);

    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/themes", applet_name);
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir); 
  }

  /*FIXME: Should we ensure_icon here? The current_state variable is probably
   * invalid at this moment...
   */
}

void
awn_themed_icon_set_info_simple (AwnThemedIcon  *icon,
                                 const gchar    *applet_name,
                                 const gchar    *uid,
                                 const gchar    *icon_name)
{
  gchar *states[]   = { "__SINGULAR__", NULL };
  gchar *icon_names[] = { NULL, NULL };

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  icon_names[0] = g_strdup (icon_name);
  
  awn_themed_icon_set_info (icon, applet_name, uid, states, icon_names);

  g_free (icon_names[0]);
  icon_names[0] = NULL;

  awn_themed_icon_set_state (icon, states[0]);
}

void
awn_themed_icon_set_override_theme (AwnThemedIcon *icon,
                                    gboolean       override)
{
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  ensure_icon (icon);
}

GdkPixbuf * 
awn_themed_icon_get_icon_at_size (AwnThemedIcon *icon,
                                  const gchar   *state,
                                  guint          size)
{
  return NULL;
}

void 
on_icon_theme_changed (GtkIconTheme *theme, AwnThemedIcon *icon)
{
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  ensure_icon (icon);
}