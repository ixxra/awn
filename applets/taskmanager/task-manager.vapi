/* task-manager.vapi generated by vapigen, do not modify. */

[CCode (cprefix = "Task", lower_case_cprefix = "task_")]
namespace Task {
	[CCode (cheader_filename = "task-icon.h")]
	public class Icon : Awn.ThemedIcon, Gtk.Buildable, Atk.Implementor, Awn.Overlayable {
		[CCode (type = "GtkWidget*", has_construct_function = false)]
		public Icon (Awn.Applet applet);
		public void append_ephemeral_item (Task.Item item);
		public void append_item (Task.Item item);
		public bool contains_launcher ();
		public uint count_ephemeral_items ();
		public uint count_items ();
		public uint count_tasklist_windows ();
		public void decrement_ephemeral_count ();
		public unowned string get_custom_name ();
		public unowned GLib.Object get_dbus_dispatcher ();
		public unowned Gtk.Widget get_dialog ();
		public unowned GLib.SList<Task.Item> get_items ();
		public unowned Task.Item get_launcher ();
		public unowned Task.Item get_main_item ();
		public void increment_ephemeral_count ();
		public bool is_visible ();
		public uint match_item (Task.Item item);
		public void moving_item (Task.Icon src, Task.Item item);
		public void refresh_icon (uint size);
		public void remove_item (Task.Item item);
		public void schedule_geometry_refresh ();
		public void set_draggable (bool draggable);
		public void set_inhibit_focus_loss (bool val);
		[NoAccessorMethod]
		public Awn.Applet applet { owned get; set construct; }
		[NoAccessorMethod]
		public int desktop_copy { get; set construct; }
		[NoAccessorMethod]
		public int drag_and_drop_hover_delay { get; set construct; }
		[NoAccessorMethod]
		public bool draggable { get; set construct; }
		[NoAccessorMethod]
		public bool enable_long_press { get; set construct; }
		[NoAccessorMethod]
		public int icon_change_behavior { get; set construct; }
		[NoAccessorMethod]
		public int max_indicators { get; set construct; }
		[NoAccessorMethod]
		public string menu_filename { owned get; set construct; }
		[NoAccessorMethod]
		public bool overlay_application_icons { get; set construct; }
		[NoAccessorMethod]
		public double overlay_application_icons_alpha { get; set construct; }
		[NoAccessorMethod]
		public double overlay_application_icons_scale { get; set construct; }
		[NoAccessorMethod]
		public bool overlay_application_icons_swap { get; set construct; }
		[NoAccessorMethod]
		public int txt_indicator_threshold { get; set construct; }
		public virtual signal void dest_drag_leave ();
		public virtual signal void dest_drag_motion (int p0, int p1);
		public virtual signal void source_drag_begin ();
		public virtual signal void source_drag_end ();
		public virtual signal void source_drag_fail ();
		public virtual signal void visible_changed ();
	}
	[CCode (cheader_filename = "task-item.h")]
	public class Item : Gtk.Button, Gtk.Activatable, Gtk.Buildable, Atk.Implementor {
		public Awn.OverlayPixbufFile icon_overlay;
		public Awn.OverlayProgressCircle progress_overlay;
		public Awn.OverlayText text_overlay;
		public void emit_icon_changed (Gdk.Pixbuf icon);
		public void emit_name_changed (string name);
		public void emit_visible_changed (bool visible);
		public virtual unowned Gdk.Pixbuf get_icon ();
		public virtual unowned Gtk.Widget get_image_widget ();
		public virtual unowned string get_name ();
		public unowned Task.Icon get_task_icon ();
		public virtual bool is_visible ();
		public virtual void left_click (Gdk.EventButton event);
		public virtual uint match (Task.Item item_to_match);
		public virtual void middle_click (Gdk.EventButton event);
		[NoWrapper]
		public virtual void name_change (string name);
		public virtual unowned Gtk.Widget right_click (Gdk.EventButton event);
		public void set_task_icon (Task.Icon icon);
		[NoAccessorMethod]
		public Awn.Applet applet { owned get; set construct; }
		[NoAccessorMethod]
		public bool ignore_wm_client_name { get; set construct; }
		public virtual signal void icon_changed (Gdk.Pixbuf icon);
		public virtual signal void name_changed (string name);
		public virtual signal void visible_changed (bool visible);
	}
	[CCode (cheader_filename = "task-launcher.h")]
	public class Launcher : Task.Item, Gtk.Activatable, Gtk.Buildable, Atk.Implementor {
		[CCode (type = "TaskItem*", has_construct_function = false)]
		public Launcher.for_desktop_file (Awn.Applet applet, string path);
		public unowned string get_desktop_path ();
		public static unowned string get_exec (Task.Item item);
		public static unowned string get_icon_name (Task.Item item);
		public void launch_with_data (GLib.SList list);
		[NoAccessorMethod]
		public string desktopfile { owned get; set; }
	}
	[CCode (cheader_filename = "task-manager.h")]
	public class Manager : Awn.Applet, Gtk.Buildable, Atk.Implementor, Awn.PanelConnector {
		[CCode (type = "AwnApplet*", has_construct_function = false)]
		public Manager (string name, string uid, int panel_id);
		public void add_icon (Task.Icon icon);
		public void append_launcher (string launcher_path);
		public bool get_capabilities (string[] supported_keys) throws GLib.Error;
		public unowned Task.Icon get_icon_by_xid (int64 xid);
		public unowned GLib.SList get_icons ();
		public GLib.SList<Task.Icon> get_icons_by_desktop (string desktop);
		public GLib.SList<Task.Icon> get_icons_by_pid (int pid);
		public GLib.SList<Task.Icon> get_icons_by_wmclass (string name);
		public void remove_task_icon (Gtk.Widget icon);
		public bool update (GLib.Value window, GLib.HashTable hints) throws GLib.Error;
		[NoAccessorMethod]
		public int attention_autohide_timer { get; set; }
		[NoAccessorMethod]
		public int attention_required_reminder { get; set; }
		[NoAccessorMethod]
		public bool drag_and_drop { get; set construct; }
		[NoAccessorMethod]
		public bool grouping { get; set construct; }
		[NoAccessorMethod]
		public bool icon_grouping { get; set construct; }
		[NoAccessorMethod]
		public bool intellihide { get; set; }
		[NoAccessorMethod]
		public int intellihide_mode { get; set construct; }
		[NoAccessorMethod]
		public GLib.ValueArray launcher_paths { owned get; set; }
		[NoAccessorMethod]
		public int match_strength { get; set; }
		[NoAccessorMethod]
		public bool only_show_launchers { get; set construct; }
		[NoAccessorMethod]
		public bool show_all_windows { get; set construct; }
		public virtual signal void grouping_changed (bool grouping);
	}
	[CCode (cheader_filename = "task-window.h")]
	public class Window : Task.Item, Gtk.Activatable, Gtk.Buildable, Atk.Implementor {
		[CCode (type = "TaskItem*", has_construct_function = false)]
		public Window (Awn.Applet applet, Wnck.Window window);
		public void activate (uint32 timestamp);
		public void close (uint32 timestamp);
		public unowned Wnck.Application get_application ();
		public unowned string get_client_name ();
		public Task.WinIconUse get_icon_changes ();
		public bool get_icon_is_fallback ();
		public bool get_is_running ();
		public unowned string get_message ();
		public bool get_needs_attention ();
		public int get_pid ();
		public float get_progress ();
		public unowned Wnck.Screen get_screen ();
		public Task.WinIconUse get_use_win_icon ();
		public unowned Wnck.Window get_window ();
		public bool get_wm_class (string res_name, string class_name);
		public bool get_wm_client (string client_name);
		public ulong get_xid ();
		public bool is_active ();
		public bool is_hidden ();
		public bool is_on_workspace (Wnck.Workspace space);
		public bool is_visible ();
		public void minimize ();
		public unowned Gtk.Widget popup_context_menu (Gdk.EventButton event);
		[NoWrapper]
		public virtual void running_changed (bool is_running);
		public void set_active_workspace (Wnck.Workspace space);
		public void set_hidden (bool hidden);
		public void set_highlighted (bool highlight_state);
		public void set_icon_geometry (int x, int y, int width, int height);
		public void set_is_active (bool is_active);
		public void set_use_win_icon (Task.WinIconUse win_use);
		[NoAccessorMethod]
		public int activate_behavior { get; set construct; }
		[NoAccessorMethod]
		public bool highlighted { get; set construct; }
		[NoAccessorMethod]
		public Wnck.Window taskwindow { owned get; set; }
		public int use_win_icon { get; set construct; }
		public virtual signal void active_changed (bool is_active);
		public virtual signal void hidden_changed (bool hidden);
		public virtual signal void message_changed (string message);
		public virtual signal void needs_attention (bool needs_attention);
		public virtual signal void progress_changed (float progress);
		public virtual signal void workspace_changed (Wnck.Workspace space);
	}
	[CCode (cprefix = "USE_", has_type_id = false, cheader_filename = "task-defines.h")]
	public enum WinIconUse {
		DEFAULT,
		ALWAYS,
		NEVER
	}
	[CCode (cheader_filename = "task-manager.h")]
	public const int MAX_TASK_ITEM_CHARS;
}
