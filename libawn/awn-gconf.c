/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#include <assert.h>
#include "awn-gconf.h"

#include "config.h"

#define AWN_PATH		"/apps/avant-window-navigator"
#define AWN_FORCE_MONITOR	AWN_PATH "/force_monitor"	/*bool*/
#define AWN_MONITOR_WIDTH	AWN_PATH "/monitor_width"	/*int*/
#define AWN_MONITOR_HEIGHT	AWN_PATH "/monitor_height"	/*int*/
#define AWN_PANEL_MODE		AWN_PATH "/panel_mode"		/*bool*/
#define AWN_AUTO_HIDE		AWN_PATH "/auto_hide"		/*bool*/
#define AWN_AUTO_HIDE_DELAY	AWN_PATH "/auto_hide_delay"	/*int*/
#define AWN_KEEP_BELOW		AWN_PATH "/keep_below"		/*bool*/
#define NO_BAR_RESIZE_ANI	AWN_PATH "/no_bar_resize_animation" /*bool*/

#define BAR_PATH		AWN_PATH "/bar"
#define BAR_ROUNDED_CORNERS	BAR_PATH "/rounded_corners"	/*bool*/
#define BAR_CORNER_RADIUS 	BAR_PATH "/corner_radius" 	/*float*/
#define BAR_RENDER_PATTERN	BAR_PATH "/render_pattern"	/*bool*/
#define BAR_PATTERN_URI		BAR_PATH "/pattern_uri" 	/*string*/
#define BAR_PATTERN_ALPHA 	BAR_PATH "/pattern_alpha" 	/*float*/
#define BAR_GLASS_STEP_1	BAR_PATH "/glass_step_1"	/*string*/
#define BAR_GLASS_STEP_2	BAR_PATH "/glass_step_2"	/*string*/
#define BAR_GLASS_HISTEP_1	BAR_PATH "/glass_histep_1"	/*string*/
#define BAR_GLASS_HISTEP_2	BAR_PATH "/glass_histep_2"	/*string*/
#define BAR_BORDER_COLOR	BAR_PATH "/border_color"	/*string*/
#define BAR_HILIGHT_COLOR	BAR_PATH "/hilight_color"	/*string*/
#define BAR_SHOW_SEPARATOR	BAR_PATH "/show_separator"	/*string*/
#define BAR_SEP_COLOR		BAR_PATH "/sep_color"		/*string*/
#define BAR_HEIGHT		BAR_PATH "/bar_height"		/*int*/
#define BAR_ANGLE		BAR_PATH "/bar_angle"		/*int, between 0 and 90*/
#define ICON_OFFSET		BAR_PATH "/icon_offset" 	/*float*/
#define BAR_POS			BAR_PATH "/bar_pos" 		/*float, between 0 and 1 */

#define WINMAN_PATH		AWN_PATH "/window_manager"
#define WINMAN_SHOW_ALL_WINS	WINMAN_PATH "/show_all_windows"	/*bool*/
#define WINMAN_LAUNCHERS	WINMAN_PATH "/launchers"	/*str list*/

#define APP_PATH		AWN_PATH "/app"
#define APP_ACTIVE_PNG		APP_PATH "/active_png"		/*string*/
#define APP_USE_PNG		APP_PATH "/use_png"		/*bool*/
#define APP_ARROW_COLOR		APP_PATH "/arrow_color"		/*color*/
#define APP_ARROW_OFFSET	APP_PATH "/arrow_offset"	/*offset*/
#define APP_TASKS_H_ARROWS	APP_PATH "/tasks_have_arrows"	/*bool*/
#define APP_NAME_CHANGE_NOTIFY	APP_PATH "/name_change_notify"	/*bool*/
#define APP_ALPHA_EFFECT	APP_PATH "/alpha_effect"	/*bool*/
#define ICON_EFFECT		APP_PATH "/icon_effect"		/*int*/
#define APP_HOVER_BOUNCE_EFFECT	APP_PATH "/hover_bounce_effect"	/*bool*/

#define TITLE_PATH		AWN_PATH "/title"
#define TITLE_TEXT_COLOR	TITLE_PATH "/text_color"	/*color*/
#define TITLE_SHADOW_COLOR	TITLE_PATH "/shadow_color"	/*color*/
#define TITLE_BACKGROUND	TITLE_PATH "/background"	/*color*/
#define TITLE_FONT_FACE		TITLE_PATH "/font_face"		/*bool*/

/* globals */
static AwnSettings *settings		= NULL;
static GConfClient *client 		= NULL;

/* prototypes */
static void awn_load_bool(GConfClient *client, const gchar* key, gboolean *data, gboolean def);
static void awn_load_string(GConfClient *client, const gchar* key, gchar **data, const char *def);
static void awn_load_float(GConfClient *client, const gchar* key, gfloat *data, float def);
static void awn_load_int(GConfClient *client, const gchar* key, int *data, int def);
static void awn_load_color(GConfClient *client, const gchar* key, AwnColor *color, const char * def);
static void awn_load_string_list(GConfClient *client, const gchar* key, GSList **data, GSList *def);

static void awn_notify_bool (GConfClient *client, guint cid, GConfEntry *entry, gboolean* data);
static void awn_notify_string (GConfClient *client, guint cid, GConfEntry *entry, gchar** data);
static void awn_notify_float (GConfClient *client, guint cid, GConfEntry *entry, gfloat* data);
static void awn_load_int (GConfClient *client, const gchar* key, int *data, int def);
static void awn_notify_color (GConfClient *client, guint cid, GConfEntry *entry, AwnColor *color);

static void hex2float(char* HexColor, float* FloatColor);

AwnSettings* awn_get_settings(void)
{
	//assert(settings != NULL);
	if (settings) return settings;
	else return awn_gconf_new();
}

AwnSettings* 
awn_gconf_new()
{
	GdkScreen *screen;
	gint width = 1024;
	gint height = 768;
	
	screen = gdk_screen_get_default();
	if (screen) {
		width = gdk_screen_get_width(screen);
		height = gdk_screen_get_height(screen);
	}

	if (settings)
	        return settings;
	AwnSettings *s = NULL;
	
	s = g_new(AwnSettings, 1);
	settings = s;
	client = gconf_client_get_default();
	
	s->icon_theme = gtk_icon_theme_get_default();
	/* general settings */
	gconf_client_add_dir(client, AWN_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	awn_load_bool(client, AWN_FORCE_MONITOR, &s->force_monitor, FALSE);
	awn_load_int(client, AWN_MONITOR_WIDTH, &s->monitor_width, width);
	awn_load_int(client, AWN_MONITOR_HEIGHT, &s->monitor_height, height);
	awn_load_bool(client, AWN_PANEL_MODE, &s->panel_mode, FALSE);
	awn_load_bool(client, AWN_AUTO_HIDE, &s->auto_hide, FALSE);
	awn_load_int(client, AWN_AUTO_HIDE_DELAY, &s->auto_hide_delay, 1000);
	awn_load_bool(client, AWN_KEEP_BELOW, &s->keep_below, FALSE);
	awn_load_bool(client, NO_BAR_RESIZE_ANI, &s->no_bar_resize_ani, FALSE);
	s->hidden = FALSE;
	
	/* Bar settings */
	gconf_client_add_dir(client, BAR_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);

	awn_load_int(client, BAR_HEIGHT, &s->bar_height,48);
	awn_load_float(client, BAR_POS, &s->bar_pos,0.5);
	awn_load_int(client, BAR_ANGLE, &s->bar_angle,0);
	awn_load_int(client, ICON_OFFSET, &s->icon_offset,10);
	awn_load_bool(client, BAR_ROUNDED_CORNERS, &s->rounded_corners, TRUE);
	awn_load_float(client, BAR_CORNER_RADIUS, &s->corner_radius, 10.0);	
	awn_load_bool(client, BAR_RENDER_PATTERN, &s->render_pattern, FALSE);	
	awn_load_string(client, BAR_PATTERN_URI, &s->pattern_uri, "~");
	awn_load_float(client, BAR_PATTERN_ALPHA, &s->pattern_alpha, 1.0);
	
	awn_load_color(client, BAR_GLASS_STEP_1, &s->g_step_1, "454545C8");
	awn_load_color(client, BAR_GLASS_STEP_2, &s->g_step_2, "010101BE");
	awn_load_color(client, BAR_GLASS_HISTEP_1, &s->g_histep_1, "FFFFFF0B");
	awn_load_color(client, BAR_GLASS_HISTEP_2, &s->g_histep_2, "FFFFFF0A");
	
	awn_load_color(client, BAR_BORDER_COLOR, &s->border_color, "000000CC");
	awn_load_color(client, BAR_HILIGHT_COLOR, &s->hilight_color, "FFFFFF11");
	
	awn_load_bool(client, BAR_SHOW_SEPARATOR, &s->show_separator, TRUE);
	awn_load_color(client, BAR_SEP_COLOR, &s->sep_color, "FFFFFF00");
	
	/* Window Manager settings */
	gconf_client_add_dir(client, WINMAN_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	awn_load_bool(client, WINMAN_SHOW_ALL_WINS, &s->show_all_windows, TRUE);
	//s->launchers = gconf_client_get_list ( client, WINMAN_LAUNCHERS, GCONF_VALUE_STRING, NULL);
	awn_load_string_list(client, WINMAN_LAUNCHERS, &s->launchers, NULL);
	
	/* App settings */
	gconf_client_add_dir(client, APP_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	awn_load_string(client, APP_ACTIVE_PNG, &s->active_png, "~");
	awn_load_bool(client, APP_USE_PNG, &s->use_png, FALSE);
	awn_load_color(client, APP_ARROW_COLOR, &s->arrow_color, "FFFFFF66");
	awn_load_int(client, APP_ARROW_OFFSET, &s->arrow_offset, 2);
	awn_load_bool(client, APP_TASKS_H_ARROWS, &s->tasks_have_arrows, FALSE);
	awn_load_bool(client, APP_NAME_CHANGE_NOTIFY, &s->name_change_notify, FALSE);
	awn_load_bool(client, APP_ALPHA_EFFECT, &s->alpha_effect, FALSE);
 	awn_load_int(client, ICON_EFFECT, &s->icon_effect,0);

	/* Title settings */
	gconf_client_add_dir(client, TITLE_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	awn_load_color(client, TITLE_TEXT_COLOR, &s->text_color, "FFFFFFFF");
	awn_load_color(client, TITLE_SHADOW_COLOR, &s->shadow_color, "1B3B12E1");
	awn_load_color(client, TITLE_BACKGROUND, &s->background, "000000AA");
	awn_load_string(client, TITLE_FONT_FACE, &s->font_face, "Sans 11");	
	
	s->task_width = settings->bar_height + 12;
	
	/* make the custom icons directory */
	gchar *path = g_build_filename (g_get_home_dir (),
					".config/awn/custom-icons",
					NULL);
	g_mkdir_with_parents (path, 0755);
	g_free (path);
	
	return s;
}

static void 
awn_notify_bool (GConfClient *client, guint cid, GConfEntry *entry, gboolean* data)
{
	GConfValue *value = NULL;
	
	value = gconf_entry_get_value(entry);
	*data = gconf_value_get_bool(value);
	
	if (*data)
		g_print("%s is true\n", gconf_entry_get_key(entry));
}

static void 
awn_notify_string (GConfClient *client, guint cid, GConfEntry *entry, gchar** data)
{
	GConfValue *value = NULL;
	
	value = gconf_entry_get_value(entry);
	*data = (gchar *) gconf_value_get_string(value);
	
	//g_print("%s is %s\n", gconf_entry_get_key(entry), *data);
}

static void 
awn_notify_float (GConfClient *client, guint cid, GConfEntry *entry, gfloat* data)
{
	GConfValue *value = NULL;
	
	value = gconf_entry_get_value(entry);
	*data = gconf_value_get_float(value);
	//g_print("%s is %f\n", gconf_entry_get_key(entry), *data);
}

static void 
awn_notify_int (GConfClient *client, guint cid, GConfEntry *entry, int* data)
{
	GConfValue *value = NULL;
	
	value = gconf_entry_get_value(entry);
	*data = gconf_value_get_int(value);
	//g_print("%s is %f\n", gconf_entry_get_key(entry), *data);
}

static void 
awn_notify_color (GConfClient *client, guint cid, GConfEntry *entry, AwnColor* color)
{
	GConfValue *value = NULL;
	float colors[4];
	
	value = gconf_entry_get_value(entry);
	hex2float( (gchar *)gconf_value_get_string(value), colors);
	
	color->red = colors[0];
	color->green = colors[1];
	color->blue = colors[2];
	color->alpha = colors[3];
}


static void
awn_load_bool(GConfClient *client, const gchar* key, gboolean *data, gboolean def)
{
	GConfValue *value = NULL;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		*data = gconf_client_get_bool(client, key, NULL);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_bool (client, key, def, NULL);
		*data = def;
	}
	gconf_client_notify_add (client, key, (GConfClientNotifyFunc)awn_notify_bool, data, NULL, NULL);
}

static void
awn_load_string(GConfClient *client, const gchar* key, gchar **data, const char *def)
{
	GConfValue *value = NULL;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		*data = gconf_client_get_string(client, key, NULL);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_string (client, key, def, NULL);
		*data = g_strdup(def);
	}
	
	gconf_client_notify_add (client, key, (GConfClientNotifyFunc)awn_notify_string, data, NULL, NULL);
}

static void
awn_load_float(GConfClient *client, const gchar* key, gfloat *data, float def)
{
	GConfValue *value = NULL;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		*data = gconf_client_get_float(client, key, NULL);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_float (client, key, def, NULL);
		*data = def;
	}
	
	gconf_client_notify_add (client, key, (GConfClientNotifyFunc)awn_notify_float, data, NULL, NULL);
}

static void
awn_load_int(GConfClient *client, const gchar* key, int *data, int def)
{
	GConfValue *value = NULL;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		*data = gconf_client_get_int(client, key, NULL);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_int (client, key, def, NULL);
		*data = def;
	}
	
	gconf_client_notify_add (client, key, (GConfClientNotifyFunc)awn_notify_int, data, NULL, NULL);
}

static void
awn_load_color(GConfClient *client, const gchar* key, AwnColor *color, const char * def)
{
	float colors[4];
	GConfValue *value = NULL;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		hex2float (gconf_client_get_string(client, key, NULL), colors);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_string (client, key, def, NULL);
		hex2float ( (gchar*)def, colors);
	}
	color->red = colors[0];
	color->green = colors[1];
	color->blue = colors[2];
	color->alpha = colors[3];
		
	gconf_client_notify_add (client, key, (GConfClientNotifyFunc)awn_notify_color, color, NULL, NULL);
}

static void
awn_load_string_list(GConfClient *client, const gchar* key, GSList **data, GSList *def)
{
	GConfValue *value = NULL;
	//GSList *slist = def;
	
	value = gconf_client_get(client, key, NULL);
	if (value) {
		*data = gconf_client_get_list ( client, key, GCONF_VALUE_STRING, NULL);
	} else {
		g_print("%s unset, setting now\n", key);
		gconf_client_set_list (client, key, GCONF_VALUE_STRING, NULL, NULL);
		*data = NULL;
	}
}

static int 
getdec(char hexchar)
{
   if ((hexchar >= '0') && (hexchar <= '9')) return hexchar - '0';
   if ((hexchar >= 'A') && (hexchar <= 'F')) return hexchar - 'A' + 10;
   if ((hexchar >= 'a') && (hexchar <= 'f')) return hexchar - 'a' + 10;

   return -1; // Wrong character

}

static void 
hex2float(char* HexColor, float* FloatColor)
{
   char* HexColorPtr = HexColor;

   int i = 0;
   for (i = 0; i < 4; i++)
   {
     int IntColor = (getdec(HexColorPtr[0]) * 16) +
                     getdec(HexColorPtr[1]);

     FloatColor[i] = (float) IntColor / 255.0;
     HexColorPtr += 2;
   }

}


