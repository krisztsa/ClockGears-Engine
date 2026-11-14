#ifndef CLOCKGEARS_HUD_H
#define CLOCKGEARS_HUD_H

#include "scene/gui/box_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"

class ClockGearsDataManager;

class ClockGearsHudOverlay : public Control {
	GDCLASS(ClockGearsHudOverlay, Control);

public:
	static constexpr float LOGICAL_WIDTH = 960.0f;
	static constexpr float LOGICAL_HEIGHT = 540.0f;

private:
	struct WidgetEntry {
		Dictionary config;
		String id;
		String name;
		String type;
		String anchor;
		int offset_x = 0;
		int offset_y = 0;
		int width = 160;
		int height = 36;
		int layer_order = 0;
		String window_style_id;
		String visibility_mode = "always";
		String visibility_switch;
		bool hide_during_messages_and_menus = true;
		bool pixel_art = true;

		Control *root = nullptr;
		Panel *bg_panel = nullptr;
		Control *content_root = nullptr;

		// Bar subnodes
		ColorRect *bar_bg_rect = nullptr;
		ColorRect *bar_border_top = nullptr;
		ColorRect *bar_border_bottom = nullptr;
		ColorRect *bar_border_left = nullptr;
		ColorRect *bar_border_right = nullptr;
		TextureRect *bar_bg_tex = nullptr;
		Control *bar_fill_clip = nullptr;
		ColorRect *bar_fill_rect = nullptr;
		TextureRect *bar_fill_tex = nullptr;
		Label *bar_label = nullptr;

		// Hearts subnodes
		Control *hearts_container = nullptr;
		Vector<TextureRect *> heart_nodes;

		// Currency / Item subnodes
		HBoxContainer *hbox = nullptr;
		TextureRect *icon_rect = nullptr;
		Label *value_label = nullptr;

		// Text subnode
		Label *text_label = nullptr;

		// Image subnode
		TextureRect *image_rect = nullptr;

		// Snapshot / Metrics (event-driven only)
		int update_count = 0;
		Dictionary last_snapshot;
	};

	Dictionary hud_config;
	Vector<WidgetEntry> widgets;
	bool has_explicit_config = false;
	bool dm_signals_connected = false;
	ObjectID connected_viewport_id;

	bool context_blocked = false;
	bool preview_mode = false;
	bool log_updates = false;
	int leader_actor_id_override = 0;
	int total_update_count = 0;
	Size2 viewport_size_override;
	Size2 last_layout_size;
	float current_ui_scale = 1.0f;
	Rect2 current_stage_rect = Rect2(0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT);

	Ref<ImageTexture> fallback_heart_full;
	Ref<ImageTexture> fallback_heart_half;
	Ref<ImageTexture> fallback_heart_empty;
	Ref<ImageTexture> fallback_coin_tex;
	Ref<ImageTexture> fallback_item_tex;

	void _ensure_fallback_textures();
	void _connect_signals();
	void _disconnect_signals();
	void _clear_widgets();
	void _rebuild_widgets(const String &p_reason);
	void _build_widget_entry(WidgetEntry &r_entry, int p_index);
	void _apply_widget_window_style(WidgetEntry &r_entry, float p_scale);
	void _update_widget_data(WidgetEntry &r_entry, const String &p_reason, bool p_count_update);
	void _update_widget_visibility(WidgetEntry &r_entry);
	void _relayout_widgets(bool p_refresh_styles);

	Size2 _resolve_effective_viewport_size() const;
	int _resolve_leader_actor_id(ClockGearsDataManager *p_dm) const;
	bool _widget_matches_topic(const WidgetEntry &p_entry, const String &p_topic, const String &p_key) const;

	void _on_runtime_state_changed(const String &p_topic, const String &p_key);
	void _on_data_changed(const String &p_category, int p_id);
	void _on_viewport_resized();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void apply_hud_config(const Dictionary &p_cfg);
	void reload_from_data_manager();
	void refresh_all_widgets(const String &p_reason = "manual");
	void refresh_window_styles();

	void set_context_blocked(bool p_blocked);
	bool is_context_blocked() const { return context_blocked; }

	void set_preview_mode(bool p_preview);
	bool is_preview_mode() const { return preview_mode; }

	void set_leader_actor_id(int p_actor_id);
	int get_leader_actor_id() const { return leader_actor_id_override; }

	void set_viewport_size_override(const Size2 &p_size);
	Size2 get_viewport_size_override() const { return viewport_size_override; }

	void set_log_updates(bool p_enable) { log_updates = p_enable; }
	bool is_logging_updates() const { return log_updates; }

	int get_widget_count() const { return widgets.size(); }
	Control *get_widget_node(const String &p_id_or_name) const;
	Dictionary get_widget_state(const String &p_id_or_name) const;
	Array get_all_widget_states() const;
	int get_widget_update_count(const String &p_id_or_name) const;
	int get_total_update_count() const { return total_update_count; }
	void reset_update_counters();
	Dictionary get_layout_bounds_info() const;

	ClockGearsHudOverlay();
	~ClockGearsHudOverlay();
};

#endif // CLOCKGEARS_HUD_H
