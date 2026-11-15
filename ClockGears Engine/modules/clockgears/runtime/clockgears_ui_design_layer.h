#ifndef CLOCKGEARS_UI_DESIGN_LAYER_H
#define CLOCKGEARS_UI_DESIGN_LAYER_H

#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/texture_rect.h"

class ClockGearsDataManager;

// ============================================================================
// ClockGearsUiDesignLayer — Renderizador Runtime de Diseños Visuales 16:9 (9B Tanda 6A)
// Renderiza layouts personalizados por pantalla sobre base lógica 960x540 con
// anclaje de 9 puntos, estilos de ventana, enlaces de datos en vivo y orden
// de foco configurable para teclado/ratón.
// ============================================================================
class ClockGearsUiDesignLayer : public Control {
	GDCLASS(ClockGearsUiDesignLayer, Control);

public:
	static constexpr float LOGICAL_WIDTH = 960.0f;
	static constexpr float LOGICAL_HEIGHT = 540.0f;

private:
	struct ElementEntry {
		String id;
		String name;
		String type;
		String anchor = "top_left";
		int offset_x = 0;
		int offset_y = 0;
		int width = 120;
		int height = 36;
		int layer_order = 0;
		bool visible = true;
		String window_style_id;
		String data_binding = "none";
		String action;
		int focus_order = 0;
		int array_order = 0;
		Dictionary config;

		Control *root = nullptr;
		PanelContainer *bg_panel = nullptr;
		Control *content_root = nullptr;
		Label *label = nullptr;
		Button *button = nullptr;
		TextureRect *image_rect = nullptr;
		TextureRect *icon_rect = nullptr;

		// Bar subnodes
		ColorRect *bar_bg_rect = nullptr;
		Control *bar_fill_clip = nullptr;
		ColorRect *bar_fill_rect = nullptr;
		Label *bar_label = nullptr;

		// Hearts subnode
		Control *hearts_container = nullptr;
		int last_hearts_key = -1;
	};

	Vector<ElementEntry> elements;
	String screen_id = "title";
	String window_type_fallback = "game_menu";
	Dictionary active_design;
	Dictionary disabled_actions;
	bool preview_mode = false;
	Size2 viewport_size_override;
	Size2 last_layout_size = Vector2(-1, -1);
	Rect2 current_stage_rect;
	float current_ui_scale = 1.0f;

	Size2 _resolve_effective_viewport_size() const;
	void _rebuild_from_design(const Dictionary &p_design);
	void _build_element_entry(ElementEntry &r_entry);
	void _apply_element_style(ElementEntry &r_entry, float p_scale);
	void _relayout_elements(bool p_refresh_styles);
	void _on_button_pressed(const String &p_action, const String &p_elem_id);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static Vector2 compute_anchor_factors(const String &p_anchor);
	static Rect2 compute_stage_rect(const Vector2 &p_viewport_size);
	static Rect2 compute_element_rect(const Dictionary &p_elem, const Rect2 &p_stage_rect, float p_scale);
	static String resolve_element_text(const Dictionary &p_elem, const ClockGearsDataManager *p_dm);
	static Ref<Texture2D> make_builtin_heart_texture(int p_mode);
	static Ref<Texture2D> make_builtin_coin_texture();

	void set_screen_id(const String &p_screen_id) { screen_id = p_screen_id; }
	String get_screen_id() const { return screen_id; }

	void set_window_type_fallback(const String &p_win_type) { window_type_fallback = p_win_type; }
	String get_window_type_fallback() const { return window_type_fallback; }

	void set_preview_mode(bool p_preview);
	bool is_preview_mode() const { return preview_mode; }

	void set_viewport_size_override(const Vector2 &p_size);
	Vector2 get_viewport_size_override() const { return viewport_size_override; }

	void set_action_disabled(const String &p_action, bool p_disabled);
	void clear_disabled_actions();

	void apply_screen_design(const String &p_screen_id, const Dictionary &p_design_override = Dictionary());
	void clear_elements();
	void refresh_live_values();

	Vector<Button *> get_focus_ordered_buttons() const;
	Array get_focus_order_actions() const;
	Control *get_element_control(const String &p_elem_id) const;
	String get_element_displayed_text(const String &p_elem_id) const;
	int get_element_count() const { return elements.size(); }

	bool trigger_action(const String &p_action);
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;

	ClockGearsUiDesignLayer();
};

#endif // CLOCKGEARS_UI_DESIGN_LAYER_H
