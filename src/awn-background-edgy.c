/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Michal Hruby <michal.mhr@gmail.com>
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-cairo-utils.h>
#include <libawn/awn-config-client.h>
#include <math.h>

#include "awn-background-edgy.h"
#include "awn-applet-manager.h"

G_DEFINE_TYPE (AwnBackgroundEdgy, awn_background_edgy, AWN_TYPE_BACKGROUND_FLAT)

#define AWN_BACKGROUND_EDGY_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (obj, AWN_TYPE_BACKGROUND_EDGY, \
  AwnBackgroundEdgyPrivate))

struct _AwnBackgroundEdgyPrivate
{
  gint radius;
  gint top_pad;

  gboolean in_corner;

  GtkWidget *separator;
};

static void awn_background_edgy_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      AwnOrientation  orient,
                                      GdkRectangle   *area);

static void awn_background_edgy_get_shape_mask (AwnBackground  *bg,
                                                cairo_t        *cr,
                                                AwnOrientation  orient,
                                                GdkRectangle   *area);

static void awn_background_edgy_padding_request (AwnBackground *bg,
                                                 AwnOrientation orient,
                                                 guint *padding_top,
                                                 guint *padding_bottom,
                                                 guint *padding_left,
                                                 guint *padding_right);

static void
awn_background_edgy_update_panel_size (AwnBackgroundEdgy *bg)
{
  gint size, offset;
  g_object_get (AWN_BACKGROUND (bg)->panel,
                "size", &size, "offset", &offset, NULL);

  const gint size_offset = size + offset;
  bg->priv->radius = sqrt (size_offset * size_offset * 2);
  bg->priv->top_pad = bg->priv->radius - size_offset + 1;
  const gint side_pad = bg->priv->top_pad + offset - 2;

  gtk_misc_set_padding (GTK_MISC (bg->priv->separator), side_pad/2, side_pad/2);
}

static void
awn_background_edgy_radius_changed (AwnBackgroundEdgy *bg)
{
  awn_background_edgy_update_panel_size (bg);
  awn_background_emit_padding_changed (AWN_BACKGROUND (bg));
}

static void
awn_background_edgy_align_changed (AwnBackgroundEdgy *bg) // has more params...
{
  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));
  gboolean in_corner = align == 0.0 || align == 1.0;
  if (bg->priv->in_corner != in_corner)
  {
    /* change from in_corner to in_the_middle or vice versa */
    bg->priv->in_corner = in_corner;
    if (in_corner)
    {
      gtk_widget_show (bg->priv->separator);
      /* we'll emit padding_changed after updating position in manager */
    }
    else
    {
      gtk_widget_hide (bg->priv->separator);
      awn_background_emit_padding_changed (AWN_BACKGROUND (bg));
    }
  }

  if (in_corner)
  {
    AwnAppletManager *manager;
    g_object_get (AWN_BACKGROUND (bg)->panel, "applet-manager", &manager, NULL);
    awn_applet_manager_add_widget (manager, bg->priv->separator,
                                   align == 0.0 ? 1: -1);
    awn_background_emit_padding_changed (AWN_BACKGROUND (bg));
  }
}

static void
awn_background_edgy_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_edgy_parent_class)->constructed (object);

  AwnBackgroundEdgy *bg = AWN_BACKGROUND_EDGY (object);
  gpointer panel = AWN_BACKGROUND (object)->panel;
  g_return_if_fail (panel);

  g_signal_connect_swapped (panel, "notify::size",
                            G_CALLBACK (awn_background_edgy_radius_changed),
                            object);
  g_signal_connect_swapped (panel, "notify::offset",
                            G_CALLBACK (awn_background_edgy_radius_changed),
                            object);

  AwnAppletManager *manager;
  g_object_get (panel, "applet-manager", &manager, NULL);
  awn_applet_manager_add_widget (manager, bg->priv->separator, 1);

  gpointer monitor = NULL;
  g_object_get (panel, "monitor", &monitor, NULL);

  g_return_if_fail (monitor);

  g_signal_connect_swapped (monitor, "notify::monitor-align",
                            G_CALLBACK (awn_background_edgy_align_changed),
                            object);

  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (object));
  bg->priv->in_corner = align == 0.0 || align == 1.0;

  if (bg->priv->in_corner)
  {
    if (align == 1.0)
    {
      awn_applet_manager_add_widget (manager, bg->priv->separator, -1);
    }
    gtk_widget_show (bg->priv->separator);
  }

  awn_background_edgy_update_panel_size (AWN_BACKGROUND_EDGY (object));
}

static void
awn_background_edgy_dispose (GObject *object)
{
  gpointer panel = AWN_BACKGROUND (object)->panel;
  gpointer monitor = NULL;
  gpointer manager = NULL;

  if (panel)
  {
    g_signal_handlers_disconnect_by_func (panel,
        G_CALLBACK (awn_background_edgy_radius_changed), object);
    g_object_get (panel, "monitor", &monitor, "applet-manager", &manager, NULL);
  }

  if (monitor)
    g_signal_handlers_disconnect_by_func (monitor, 
        G_CALLBACK (awn_background_edgy_align_changed), object);

  GtkWidget *widget = AWN_BACKGROUND_EDGY_GET_PRIVATE (object)->separator;

  if (manager && widget)
  {
    awn_applet_manager_remove_widget (manager, widget);
    /* removing the widget from it's container will destroy it
     *  - no need to call gtk_widget_destroy
     */
    AWN_BACKGROUND_EDGY_GET_PRIVATE (object)->separator = NULL;
  }

  G_OBJECT_CLASS (awn_background_edgy_parent_class)->dispose (object);
}

static void
awn_background_edgy_class_init (AwnBackgroundEdgyClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_background_edgy_constructed;
  obj_class->dispose = awn_background_edgy_dispose;

  bg_class->draw = awn_background_edgy_draw;
  bg_class->padding_request = awn_background_edgy_padding_request;
  bg_class->get_shape_mask = awn_background_edgy_get_shape_mask;

  g_type_class_add_private (obj_class, sizeof (AwnBackgroundEdgyPrivate));
}


static void
awn_background_edgy_init (AwnBackgroundEdgy *bg)
{
  bg->priv = AWN_BACKGROUND_EDGY_GET_PRIVATE (bg);

  GtkWidget *image = gtk_image_new ();
  bg->priv->separator = image;
}

AwnBackground * 
awn_background_edgy_new (AwnConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_EDGY,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}


/*
 * Drawing functions
 */
static void
draw_path (cairo_t *cr, gdouble radius, gint width, gint height,
           gboolean bottom_left)
{
  if (bottom_left)
    cairo_arc (cr, 0, height, radius, -M_PI / 2.0, 0.0);
  else
    cairo_arc_negative (cr, width, height, radius, -M_PI / 2.0, -M_PI);
}

static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            gint            width,
                            gint            height)
{
  cairo_pattern_t *pat;

  gfloat align = awn_background_get_panel_alignment (bg);
  if (align != 0.0 && align != 1.0) return;

  gboolean bottom_left = align == 0.0;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if (gtk_widget_is_composited (GTK_WIDGET (bg->panel)) == FALSE)
  {
    goto paint_lines;
  }

  /*
   * [0,0]
   *  __
   * |  '-.
   * |     '.
   * |       \
   * |        ;
   * '--------'  [width, height]
   * [0,height]
   */

  /* Draw the background */
  pat = cairo_pattern_create_radial (bottom_left ? 0 : width, height, 1,
                                     bottom_left ? 0 : width, height, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_2);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_1);
  draw_path(cr, height - 1.0, width, height, bottom_left);
  cairo_line_to (cr, bottom_left ? 0.0 : width, height);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_radial (bottom_left ? 0 : width, height,
                                     height * 3/4,
                                     bottom_left ? 0 : width, height,
                                     height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_histep_2);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_histep_1);

  draw_path (cr, height * 3/4, width, height, bottom_left);
  if (bottom_left)
    cairo_arc_negative (cr, 0, height, height-2.0, 0.0, -M_PI / 2.0);
  else
    cairo_arc (cr, width, height, height-2.0, -M_PI, -M_PI / 2.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  paint_lines:

  /* Internal border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  draw_path(cr, height - 2.0, width, height, bottom_left);
  cairo_stroke (cr);

  /* External border */
  awn_cairo_set_source_color (cr, bg->border_color);
  draw_path(cr, height - 1.0, width, height, bottom_left);
  cairo_stroke (cr);
}

static
void awn_background_edgy_padding_request (AwnBackground *bg,
                                          AwnOrientation orient,
                                          guint *padding_top,
                                          guint *padding_bottom,
                                          guint *padding_left,
                                          guint *padding_right)
{
  if (AWN_BACKGROUND_EDGY (bg)->priv->in_corner == FALSE)
  {
    /* if we're not in the corner behave as standard flat bg */
    AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->padding_request (
      bg, orient, padding_top, padding_bottom, padding_left, padding_right);
    return;
  }

  const gint req = AWN_BACKGROUND_EDGY (bg)->priv->top_pad;
  gboolean bottom_left = awn_background_get_panel_alignment (bg) == 0.0;

  gint base_side_pad;
  guint dummy, left, right;
  AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->padding_request (
      bg, AWN_ORIENTATION_BOTTOM, &dummy, &dummy, &left, &right);
  base_side_pad = MAX (left, right);

  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      *padding_top  = 0; *padding_bottom = req;
      *padding_left = bottom_left ? 0 : base_side_pad;
      *padding_right = bottom_left ? base_side_pad : 0;
      break;
    case AWN_ORIENTATION_BOTTOM:
      *padding_top  = req; *padding_bottom = 0;
      *padding_left = bottom_left ? 0 : base_side_pad;
      *padding_right = bottom_left ? base_side_pad : 0;
      break;
    case AWN_ORIENTATION_LEFT:
      *padding_top  = bottom_left ? 0 : base_side_pad;
      *padding_bottom = bottom_left ? base_side_pad : 0;
      *padding_left = 0; *padding_right = req;
      break;
    case AWN_ORIENTATION_RIGHT:
      *padding_top  = bottom_left ? 0 : base_side_pad;
      *padding_bottom = bottom_left ? base_side_pad : 0;
      *padding_left = req; *padding_right = 0;
      break;
    default:
      break;
  }
}

static void 
chain_draw (AwnBackground  *bg,
            cairo_t        *cr, 
            AwnOrientation  orient,
            GdkRectangle   *area)
{
  guint top, bot, left, right;

  cairo_save (cr);

  if (AWN_BACKGROUND_EDGY (bg)->priv->in_corner)
  {
    gint temp;
    gint x = area->x, y = area->y;
    gint width = area->width, height = area->height;

    gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));
    gboolean bottom_left = align == 0.0;

    cairo_rectangle (cr, x, y, width, height);

    switch (orient)
    {
      case AWN_ORIENTATION_RIGHT:
        cairo_translate (cr, x, y);
        cairo_rotate (cr, M_PI * 0.5);
        cairo_scale (cr, 1.0, -1.0);
        temp = width;
        width = height; height = temp;
        break;
      case AWN_ORIENTATION_LEFT:
        cairo_translate (cr, x+width, y);
        cairo_rotate (cr, M_PI * 0.5);
        temp = width;
        width = height; height = temp;
        break;
      case AWN_ORIENTATION_TOP:
        cairo_translate (cr, x, height - y);
        cairo_scale (cr, 1.0, -1.0);
        break;
      default:
        cairo_translate (cr, x, y);
        break;
    }
    cairo_move_to (cr, bottom_left ? 0.0 : width, height);
    draw_path (cr, height - 1.0, width, height, bottom_left);
    cairo_line_to (cr, bottom_left ? 0.0 : width, height);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip (cr);

    cairo_identity_matrix (cr);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);

    AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->padding_request (
      bg, orient, &top, &bot, &left, &right);
    const gint modifier = AWN_BACKGROUND_EDGY (bg)->priv->top_pad;
  
    switch (orient)
    {
      case AWN_ORIENTATION_RIGHT:
        area->x += modifier - left;
        area->width -= modifier - left;
        break;
      case AWN_ORIENTATION_LEFT:
        area->width -= modifier - right;
        break;
      case AWN_ORIENTATION_TOP:
        area->height -= modifier - bot;
        break;
      default:
        area->y += modifier - top;
        area->height -= modifier - top;
        break;
    }
  }

  AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)-> draw (
    bg, cr, orient, area);

  cairo_restore (cr);
}

static void 
awn_background_edgy_draw (AwnBackground  *bg,
                          cairo_t        *cr, 
                          AwnOrientation  orient,
                          GdkRectangle   *area)
{
  const gboolean in_corner = AWN_BACKGROUND_EDGY (bg)->priv->in_corner;
  gint base_size = area->width;

  if (in_corner)
  {
    gint temp;
    gint x = area->x, y = area->y;
    gint width = area->width, height = area->height;
    cairo_save (cr);

    switch (orient)
    {
      case AWN_ORIENTATION_RIGHT:
        cairo_translate (cr, x, y);
        cairo_rotate (cr, M_PI * 0.5);
        cairo_scale (cr, 1.0, -1.0);
        temp = width;
        width = height; height = temp;
        break;
      case AWN_ORIENTATION_LEFT:
        cairo_translate (cr, x+width, y);
        cairo_rotate (cr, M_PI * 0.5);
        temp = width;
        width = height; height = temp;
        break;
      case AWN_ORIENTATION_TOP:
        cairo_translate (cr, x, height - y);
        cairo_scale (cr, 1.0, -1.0);
        break;
      default:
        cairo_translate (cr, x, y);
        break;
    }

    draw_top_bottom_background (bg, cr, width, height);

    cairo_restore (cr);

    base_size = width; // will be in fact height for VERTICAL orients
  }

  if (!in_corner || base_size > AWN_BACKGROUND_EDGY (bg)->priv->radius * 4/3)
    chain_draw (bg, cr, orient, area);
}

static void 
awn_background_edgy_get_shape_mask (AwnBackground  *bg,
                                    cairo_t        *cr, 
                                    AwnOrientation  orient,
                                    GdkRectangle   *area)
{
  if (AWN_BACKGROUND_EDGY (bg)->priv->in_corner == FALSE)
  {
    AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->get_shape_mask (
      bg, cr, orient, area);
    return;
  }

  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  cairo_save (cr);

  // FIXME: doesn't work yet

  switch (orient)
  {
    case AWN_ORIENTATION_RIGHT:
      cairo_translate (cr, x, y+height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_LEFT:
      cairo_translate (cr, x+width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_TOP:
      cairo_translate (cr, x+width, y+height);
      cairo_rotate (cr, M_PI);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }

  //draw_rect (bg, cr, 0, 0, width, height+3);
  //cairo_fill (cr);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
