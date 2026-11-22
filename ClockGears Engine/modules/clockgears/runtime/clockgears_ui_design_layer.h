#ifndef CLOCKGEARS_UI_DESIGN_LAYER_H
#define CLOCKGEARS_UI_DESIGN_LAYER_H

#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"

class ClockGearsDataManager;
class ClockGearsUiDesignLayer;
class ScrollContainer;

// ============================================================================
// ClockGearsShapedClipControl — Contenedor con recorte real a forma geométrica
// (rect, rounded, pill, circle, ellipse) + overlay de borde/sombra/brillo (3F).
// ============================================================================
class ClockGearsShapedClipControl : public Control {
	GDCLASS(ClockGearsShapedClipControl, Control);

	String shape_type = "rounded";
	float corner_radius = 8.0f;
	bool clip_active = false;
	Color fill_color = Color(0.08f, 0.09f, 0.12f, 0.94f);
	bool gradient_enabled = false;
	Color gradient_top = Color(0.16f, 0.13f, 0.10f, 1.0f);
	Color gradient_bottom = Color(0.08f, 0.07f, 0.05f, 1.0f);
	String gradient_angle = "vertical";
	bool pixel_art = true;
	Ref<Texture2D> fill_texture;
	bool fill_image_enabled = false;

protected:
	void _notification(int p_what);

public:
	void configure(const Dictionary &p_shape, float p_scale, bool p_draw_fill);
	bool is_clip_active() const { return clip_active; }
	String get_shape_type() const { return shape_type; }
	bool contains_local_point(const Vector2 &p_local) const;
};

class ClockGearsShapedBorderOverlay : public Control {
	GDCLASS(ClockGearsShapedBorderOverlay, Control);

	String shape_type = "rounded";
	float corner_radius = 8.0f;
	int border_width = 2;
	Color border_color = Color(0.88f, 0.63f, 0.25f, 1.0f);
	bool shadow_enabled = false;
	Color shadow_color = Color(0, 0, 0, 0.45f);
	float shadow_size = 4.0f;
	Vector2 shadow_offset = Vector2(0, 2);
	bool glow_enabled = false;
	Color glow_color = Color(0.96f, 0.74f, 0.33f, 0.40f);
	float glow_size = 4.0f;
	bool pixel_art = true;

protected:
	void _notification(int p_what);

public:
	void configure(const Dictionary &p_shape, float p_scale);
};

// ============================================================================
// ClockGearsRadialCanvas — Menú radial prioritario dibujado con dibujo propio
// (0 nodos por ranura), estilo anillo de objetos Zelda 3D (3E).
// ============================================================================
class ClockGearsRadialCanvas : public Control {
	GDCLASS(ClockGearsRadialCanvas, Control);

public:
	struct RadialSlot {
		String id;
		String title;
		String description;
		String icon_path;
		bool pixel_art = true;
		String action;
		Array actions;
		String enabled_mode = "always";
		String enabled_switch;
		String visibility_mode = "always";
		String visibility_switch;
		bool visible = true;
		bool disabled = false;
		int quantity = 0;
		bool is_empty = false;
		String shape;
		String image_normal;
		String image_hover;
		String image_pressed;
		String image_disabled;
		bool has_fill_color = false;
		Color fill_color;
		bool has_border_color = false;
		Color border_color;
		bool has_font_color = false;
		Color font_color;
		float slot_scale = 1.0f;
		Ref<Texture2D> cached_icon;
		Ref<Texture2D> tex_normal;
		Ref<Texture2D> tex_hover;
		Ref<Texture2D> tex_pressed;
		Ref<Texture2D> tex_disabled;
	};

private:
	Dictionary layout_cfg;
	Vector<RadialSlot> all_slots;
	Vector<int> active_slot_indices;
	Vector<float> slot_visual_scales;
	Ref<Texture2D> cached_ring_tex;
	Ref<Texture2D> cached_center_tex;
	Ref<Texture2D> cached_selected_tex;
	Ref<Texture2D> cached_indicator_tex;
	int selected_pos = 0;
	float ui_scale = 1.0f;
	bool anim_active = false;
	bool interactive = true;
	String open_anim = "radial_spin";
	float open_anim_progress = 1.0f;
	bool open_anim_playing = false;
	int runtime_actor_id = 0;
	int runtime_equip_slot_idx = 0;
	bool use_editor_test_data = false;

	Vector2 _compute_center_local() const;
	float _compute_radius_px() const;
	float _compute_slot_angle(int p_visible_idx, int p_count) const;
	int _pick_slot_by_angle(float p_target_angle) const;
	void _rebuild_visible_indices();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_data_context(int p_actor_id, int p_equip_slot_idx, bool p_test_data);
	void configure(const Dictionary &p_layout, const Array &p_fallback_elements, float p_scale, bool p_interactive);
	void refresh_slot_states(const ClockGearsDataManager *p_dm);
	void play_open_anim();
	Array get_slot_actions_by_id(const String &p_id) const;

	int get_slot_count() const { return active_slot_indices.size(); }
	int get_selected_index() const { return selected_pos; }
	bool select_slot(int p_index);
	bool step_selection(int p_delta);
	bool select_by_angle_vector(const Vector2 &p_dir_vec);
	bool select_by_local_point(const Vector2 &p_local_pos);
	bool activate_selected_slot();

	Vector2 get_slot_local_pos(int p_index) const;
	float get_slot_visual_scale(int p_index) const;
	Dictionary get_center_displayed_info() const;
	Dictionary get_selected_slot_info() const;
	Dictionary get_radial_layout_config() const { return layout_cfg.duplicate(true); }
	Dictionary get_radial_slot_visual_config(int p_index) const;
	virtual String get_tooltip(const Point2 &p_pos) const override;

	ClockGearsRadialCanvas();
};

// ============================================================================
// ClockGearsMenuCursorOverlay — Cursor configurable (forma/imagen/animación 3G)
// ============================================================================
class ClockGearsMenuCursorOverlay : public Control {
	GDCLASS(ClockGearsMenuCursorOverlay, Control);

	Dictionary cursor_cfg;
	Rect2 target_rect;
	bool has_target = false;
	float ui_scale = 1.0f;
	double anim_phase = 0.0;
	Ref<Texture2D> cursor_tex;

protected:
	void _notification(int p_what);

public:
	void configure(const Dictionary &p_cursor_cfg, float p_scale);
	void set_target_rect(const Rect2 &p_rect, bool p_visible);
	Dictionary get_cursor_state() const;
	ClockGearsMenuCursorOverlay();
};

// ============================================================================
// ClockGearsUiDesignLayer — Renderizador Runtime de Diseños Visuales 16:9
// Ampliado en Etapa 3 con: menús ilimitados, distribuciones (lista/grid/tabs/
// carrusel/radial propio), formas recortadas, foco espacial y cursor (3D-3G).
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
		String widget_kind;
		String action;
		Array actions;
		int focus_order = 0;
		int array_order = 0;
		int tab_index = 0;
		String parent_id;
		bool clip_children = false;
		String clip_ancestor_id;
		Dictionary config;

		Control *root = nullptr;
		ClockGearsShapedClipControl *clip_host = nullptr;
		ClockGearsShapedBorderOverlay *border_overlay = nullptr;
		PanelContainer *bg_panel = nullptr;
		Control *content_root = nullptr;
		Label *label = nullptr;
		Button *button = nullptr;
		TextureRect *image_rect = nullptr;
		TextureRect *icon_rect = nullptr;
		Control *manual_btn_img_host = nullptr;

		// Bar subnodes
		ColorRect *bar_bg_rect = nullptr;
		Control *bar_fill_clip = nullptr;
		ColorRect *bar_fill_rect = nullptr;
		Label *bar_label = nullptr;

		// Hearts subnode
		Control *hearts_container = nullptr;
		int last_hearts_key = -1;

		// Data-driven container subnodes (Editor de UI 2/3)
		ScrollContainer *data_scroll = nullptr;
		Control *data_items_canvas = nullptr;
		Vector<Button *> data_cell_buttons;
		Array data_resolved_entries;
		Vector<Dictionary> data_tpl_children;
		Size2 data_tpl_cell_base_size = Size2(140, 120);
		int focused_data_slot_idx = 0;
		int data_carousel_offset = 0;

		// Standalone rotatable item preview support
		bool rotatable_dragging = false;
		float rotatable_angle_deg = 0.0f;
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

	// Fondo por menú (color, degradado, imagen con desplazamiento en bucle, velo oscuro 4F) y transición
	ColorRect *bg_veil_rect = nullptr;
	Control *bg_backdrop_ctrl = nullptr;
	TextureRect *bg_image_rect = nullptr;
	TextureRect *bg_blur_rect = nullptr;
	Ref<Texture2D> cached_bg_texture;
	String cached_bg_path;
	Ref<Image> captured_backdrop_img;
	Ref<ImageTexture> cached_blur_texture;
	int last_blur_intensity = -1;
	bool last_blur_preview_mode = false;
	Vector2 bg_scroll_offset;
	bool transition_active = false;
	bool transition_is_out = false;
	double transition_elapsed = 0.0;
	double transition_duration = 0.0;
	String transition_type = "none";
	String transition_curve = "ease_out";

	// Soporte para contenedores 3E (paginación / pestañas / carrusel / radial) y cursor 3G
	int current_layout_page = 0;
	int current_layout_tab = 0;
	int current_carousel_index = 0;
	Vector<Button *> layout_aux_buttons;
	ClockGearsRadialCanvas *radial_canvas = nullptr;
	ClockGearsMenuCursorOverlay *cursor_overlay = nullptr;

	// Estado vivo de contenedores con datos y objeto enfocado
	int runtime_actor_id = -1;
	int runtime_equip_slot_idx = 0;
	bool use_editor_test_data = false;
	String last_focused_container_id;
	Dictionary last_focused_data_entry;
	Dictionary focused_entry_by_container;
	Dictionary dynamic_slot_actions_by_id;

	Size2 _resolve_effective_viewport_size() const;
	void _clear_aux_layout_controls();
	void _ensure_background_nodes();
	void _rebuild_cached_blur_texture(int p_intensity);
	void _apply_screen_background(const Dictionary &p_design, const Size2 &p_vp);
	static Ref<Font> _load_cached_font(const String &p_path);
	float _eval_transition_curve(float p_t) const;
	void _apply_transition_pose(float p_progress);
	void _rebuild_from_design(const Dictionary &p_design);
	void _build_element_entry(ElementEntry &r_entry);
	static void _update_element_image_rect(ElementEntry &r_entry, const Size2 &p_target_sz);
	void _apply_element_style(ElementEntry &r_entry, float p_scale);
	void _relayout_elements(bool p_refresh_styles);
	void _apply_container_layout(const Dictionary &p_layout, float p_scale);
	void _on_button_pressed(const String &p_action, const String &p_elem_id);
	void _on_button_hover(const String &p_elem_id);
	void _on_button_focus_changed(ObjectID p_btn_id, const String &p_elem_id);
	void _on_radial_slot_activated(int p_slot_idx);
	void _on_radial_selection_changed(int p_slot_idx);
	void _on_layout_page_delta(int p_delta);
	void _on_layout_tab_selected(int p_tab_idx);
	void _on_carousel_step(int p_delta);
	void _build_or_refresh_data_container(ElementEntry &r_entry, float p_scale, bool p_rebuild_nodes);
	void _apply_data_cell_state_styles(ElementEntry &r_entry, float p_scale);
	void _refresh_focused_entry_elements();
	void _on_data_cell_focused(const String &p_container_id, int p_slot_idx);
	void _on_data_cell_pressed(const String &p_container_id, int p_slot_idx);
	void _on_data_container_carousel_delta(const String &p_container_id, int p_delta);
	void _on_rotatable_preview_gui_input(const Ref<InputEvent> &p_event, const String &p_elem_id);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static Vector2 compute_anchor_factors(const String &p_anchor);
	static Rect2 compute_stage_rect(const Vector2 &p_viewport_size);
	static Rect2 compute_manual_image_rect(const Rect2 &p_frame, const Vector2 &p_tex_size, const Dictionary &p_cfg);
	static Rect2 compute_element_rect(const Dictionary &p_elem, const Rect2 &p_stage_rect, float p_scale);
	static HashMap<String, float> compute_all_element_scales(const Array &p_elems);
	static HashMap<String, Rect2> compute_all_element_rects(const Array &p_elems, const Rect2 &p_stage_rect, float p_scale);
	static Rect2 compute_element_global_rect(const String &p_elem_id, const Array &p_elems, const Rect2 &p_stage_rect, float p_scale);
	static bool is_element_effectively_visible(const String &p_elem_id, const Array &p_elems);
	static bool is_element_effectively_locked(const String &p_elem_id, const Array &p_elems);
	static String resolve_element_text(const Dictionary &p_elem, const ClockGearsDataManager *p_dm);
	static Ref<Texture2D> make_builtin_heart_texture(int p_mode);
	static Ref<Texture2D> make_builtin_coin_texture();

	void set_screen_id(const String &p_screen_id) { screen_id = p_screen_id; }
	String get_screen_id() const { return screen_id; }

	void set_window_type_fallback(const String &p_win_type) { window_type_fallback = p_win_type; }
	String get_window_type_fallback() const { return window_type_fallback; }

	void set_preview_mode(bool p_preview);
	bool is_preview_mode() const { return preview_mode; }
	void set_use_editor_test_data(bool p_use) { use_editor_test_data = p_use; }
	bool is_using_editor_test_data() const { return use_editor_test_data; }
	void set_runtime_actor_context(int p_actor_id, int p_equip_slot_idx = 0);
	Dictionary get_focused_data_entry() const { return last_focused_data_entry.duplicate(true); }
	bool focus_data_slot(const String &p_container_id, int p_slot_idx);
	bool activate_data_slot(const String &p_container_id, int p_slot_idx);
	Dictionary get_data_container_info(const String &p_container_id) const;

	void set_viewport_size_override(const Vector2 &p_size);
	Vector2 get_viewport_size_override() const { return viewport_size_override; }

	void set_action_disabled(const String &p_action, bool p_disabled);
	void capture_backdrop_from_viewport();
	void clear_disabled_actions();

	void apply_screen_design(const String &p_screen_id, const Dictionary &p_design_override = Dictionary());
	void apply_prebuilt_screen_design(const String &p_screen_id, const Dictionary &p_norm_design);
	bool update_element_geometry_fast(const String &p_elem_id, const Dictionary &p_elem);
	void play_transition_in();
	void play_transition_out();
	bool is_transition_active() const { return transition_active; }
	void clear_elements();
	void refresh_live_values();

	Vector<Button *> get_focus_ordered_buttons() const;
	Button *find_next_focus_button(Button *p_current, int p_dir) const;
	void update_cursor_for_button(Button *p_button);
	Array get_focus_order_actions() const;
	Control *get_element_control(const String &p_elem_id) const;
	Control *get_widget_mount(const String &p_widget_kind) const;
	bool has_button_action(const String &p_action) const;
	Array get_element_actions(const String &p_elem_id) const;
	String get_element_displayed_text(const String &p_elem_id) const;
	int get_element_count() const { return elements.size(); }

	ClockGearsRadialCanvas *get_radial_canvas() const { return radial_canvas; }
	bool is_radial_mode() const { return radial_canvas != nullptr && radial_canvas->is_visible(); }
	Dictionary get_active_design_copy() const { return active_design.duplicate(true); }
	Dictionary get_layout_state_info() const;

	bool trigger_action(const String &p_action);
	bool owns_control(const Control *p_ctrl) const;
	static bool execute_common_action_step(const Dictionary &p_step, ClockGearsUiDesignLayer *p_layer = nullptr);
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;

	ClockGearsUiDesignLayer();
};

#endif // CLOCKGEARS_UI_DESIGN_LAYER_H