#pragma once

#include "core/math/color.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/vector.h"
#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

class ClockGearsBrassStyleBox : public StyleBoxFlat {
	GDCLASS(ClockGearsBrassStyleBox, StyleBoxFlat);

public:
	enum GradientMode {
		GRADIENT_NONE = 0,
		GRADIENT_PANEL = 1,
		GRADIENT_HEADER = 2,
		GRADIENT_BUTTON = 3,
		GRADIENT_BUTTON_HOVER = 4,
	};

private:
	int gradient_mode = GRADIENT_NONE;
	bool is_sunken = false;
	bool is_gear_separator = false;
	float top_highlight_alpha = 0.0f;

protected:
	static void _bind_methods();

public:
	void set_gradient_mode(int p_mode) { gradient_mode = p_mode; emit_changed(); }
	int get_gradient_mode() const { return gradient_mode; }

	void set_sunken_style(bool p_sunken) { is_sunken = p_sunken; emit_changed(); }
	bool is_sunken_style() const { return is_sunken; }

	void set_gear_separator_style(bool p_sep) { is_gear_separator = p_sep; emit_changed(); }
	bool is_gear_separator_style() const { return is_gear_separator; }

	void set_top_highlight_alpha(float p_alpha) { top_highlight_alpha = p_alpha; emit_changed(); }
	float get_top_highlight_alpha() const { return top_highlight_alpha; }

	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const override;
};

class ClockGearsTheme : public Object {
	GDCLASS(ClockGearsTheme, Object);

public:
	// Paleta oficial ClockGears Dark (Sección 11.2)
	static Color color_bg_dark() { return Color::html("#1b1d23"); }
	static Color color_panel() { return Color::html("#23262e"); }
	static Color color_accent_amber() { return Color::html("#e0a040"); }
	static Color color_text_primary() { return Color::html("#e6e6e6"); }
	static Color color_text_secondary() { return Color::html("#9aa0ab"); }
	static Color color_error() { return Color::html("#e5534b"); }
	static Color color_success() { return Color::html("#4cc38a"); }
	static Color color_menu_pill_border(bool p_brass, bool p_light = false);
	static Color color_sunken_list_bg(bool p_brass, bool p_light = false);
	static Color color_sunken_list_border(bool p_brass, bool p_light = false);
	static Color get_active_container_border_color();
	static int get_container_border_width() { return 1; }
	static int get_container_corner_radius(float p_scale = 1.0f) { return int(6 * p_scale); }

	enum GlowLevel {
		GLOW_DISABLED = 0, // Desactivado: sin halos ni transiciones, conserva los bordes
		GLOW_SOFT = 1,     // Suave (por defecto): halos 25..40 % y radio 6..8 px, transiciones 120..150 ms
		GLOW_INTENSE = 2,  // Intenso: halos 36..40 % y radio 8..10 px, transiciones 120..150 ms
	};

	struct NeonCopperTokens {
		// Superficies
		Color surface_base;     // Fondo base (el más oscuro)
		Color surface_panel;    // Panel
		Color surface_elevated; // Panel elevado
		Color surface_sunken;   // Campo hundido
		// Bordes
		Color border_copper;        // Borde cobrizo (el actual de los botones)
		Color border_copper_bright; // Borde cobrizo brillante (hover, ~15 % más claro)
		Color border_subtle;        // Borde sutil (cobre con baja opacidad, para separadores y rejillas)
		// Acento neón dorado-cobre y halo
		Color accent_neon;          // Entre el ámbar y el cobre, algo más claro y saturado que el borde
		Color halo_color;           // Color del acento con opacidad de 25 a 40 % (0 % si Desactivado)
		int halo_radius_px = 7;     // Radio de 6 a 10 px (0 si Desactivado)
		int transition_ms = 120;    // 120 a 150 ms (0 si Desactivado)
	};

	static NeonCopperTokens get_neon_copper_tokens(bool p_brass = false, bool p_light = false, float p_scale = 1.0f, int p_glow_level = -1);
	static int get_glow_effects_level();
	static void set_glow_effects_level(int p_level);
	Dictionary get_theme_tokens_dict(const String &p_theme_id = "") const;

	struct SplashPalette {
		Color bg_top;
		Color bg_bottom;
		Color card_border;
		Color accent_amber;
		Color halo_amber;
		Color ornament_copper;
		Color text_primary;
		Color text_secondary;
		Color badge_bg;
		Color progress_track;
		Color progress_fill;
		int corner_radius = 6;
		int border_width = 1;
	};
	static SplashPalette get_splash_palette(bool p_brass = false);

	// Paleta oficial ClockGears Latón (Etapa 2 - 9B-Ext)
	static Color color_brass_bg() { return Color::html("#171512"); }
	static Color color_brass_panel() { return Color::html("#201c17"); }
	static Color color_brass_header() { return Color::html("#28221a"); }
	static Color color_brass_input() { return Color::html("#13110e"); }
	static Color color_brass_border() { return Color::html("#c9923e"); }
	static Color color_brass_accent() { return Color::html("#f5bc54"); }
	static Color color_brass_text_primary() { return Color::html("#f6ecd9"); }
	static Color color_brass_text_secondary() { return Color::html("#ccbca0"); }
	static Color color_brass_text_disabled() { return Color::html("#a8987c"); }

	static constexpr int BRASS_TRANSITION_MS = 120;

private:
	static ClockGearsTheme *singleton;
	Ref<Theme> dark_theme;
	Ref<Theme> light_theme;
	Ref<Theme> brass_theme;
	bool dark_has_editor_base = false;
	bool light_has_editor_base = false;
	bool brass_has_editor_base = false;
	bool use_light_mode = false;
	String active_theme_id = "dark";
	bool simulate_brass_load_failure = false;

	Vector<ObjectID> registered_roots;
	mutable Ref<ImageTexture> cached_gradient_textures[5];
	mutable Ref<ImageTexture> cached_gear_texture;
	mutable int gradient_generation_count = 0;
	mutable int gear_generation_count = 0;

	Ref<Theme> _build_theme(bool p_light, float p_scale, bool *r_has_editor_base = nullptr) const;
	Ref<Theme> _build_brass_theme(float p_scale, bool *r_has_editor_base = nullptr) const;

	void _on_brass_control_hover_entered(ObjectID p_id);
	void _on_brass_tab_changed(int p_tab, ObjectID p_id);
	void _on_input_hover_entered(ObjectID p_id);
	void _on_input_hover_exited(ObjectID p_id);

protected:
	static void _bind_methods();

public:
	static ClockGearsTheme *get_singleton() { return singleton; }

	Ref<Theme> get_theme(bool p_light_mode = false);
	Ref<Theme> get_brass_theme();
	Ref<Theme> get_theme_by_id(const String &p_theme_id);
	Ref<Theme> get_active_theme();
	Ref<Theme> get_game_pixel_theme(int p_integer_scale = 1);

	void set_light_mode(bool p_light);
	bool is_light_mode() const { return use_light_mode; }

	PackedStringArray get_available_theme_ids() const;
	String get_theme_display_name(const String &p_theme_id) const;
	String get_active_theme_id() const { return active_theme_id; }
	bool set_active_theme_id(const String &p_theme_id);
	void sync_from_project_settings();

	void set_simulate_brass_load_failure(bool p_fail);
	bool is_simulating_brass_load_failure() const { return simulate_brass_load_failure; }

	Ref<ImageTexture> get_cached_brass_gradient_texture(int p_mode) const;
	Ref<ImageTexture> get_cached_brass_gear_texture() const;
	int get_cached_gradient_generation_count() const { return gradient_generation_count; }
	int get_cached_gear_generation_count() const { return gear_generation_count; }

	void register_themed_root(Node *p_root);
	void apply_theme_to_node_tree(Node *p_root);
	void refresh_all_themed_roots();

	void apply_to_control(Control *p_control);
	void apply_to_window(Window *p_window);

	static float compute_contrast_ratio(const Color &p_fg, const Color &p_bg);
	Dictionary inspect_theme_metrics(const String &p_theme_id = "");
	Dictionary check_tree_legibility_and_layout(Node *p_root, const Vector2i &p_viewport_size = Vector2i(1366, 768));
	Dictionary audit_inputs_and_popups_in_tree(Node *p_root);
	static Dictionary inspect_boot_splash(int p_stage, int p_total_stages, const String &p_status, const String &p_save_png_path = "", bool p_brass = false, const Vector2i &p_screen_size = Vector2i(1920, 1080), float p_anim_time_sec = -1.0f);
	static Dictionary simulate_boot_splash_particles(int p_frames = 600, int p_seed = 12345, float p_simulated_frame_ms = 2.0f, bool p_glow_enabled = true, const Vector2i &p_screen_size = Vector2i(1920, 1080));

	static Ref<StyleBoxFlat> make_container_stylebox(float p_scale = 1.0f, int p_margin_h = 12, int p_margin_v = 12, int p_variant = 0);
	static Ref<StyleBoxFlat> make_sunken_list_stylebox(float p_scale = 1.0f);
	static Ref<StyleBoxFlat> make_card_stylebox(float p_scale = 1.0f);
	static Ref<StyleBoxFlat> make_input_stylebox(bool p_has_error, bool p_focused, float p_scale = 1.0f);

	// Variante instancia para llamadas desde GDScript / CanvasItemEditor
	Ref<StyleBoxFlat> get_container_stylebox(float p_scale = 1.0f, int p_margin_h = 12, int p_margin_v = 12, int p_variant = 0) const {
		return make_container_stylebox(p_scale, p_margin_h, p_margin_v, p_variant);
	}
	Ref<StyleBoxFlat> get_sunken_list_stylebox(float p_scale = 1.0f) const {
		return make_sunken_list_stylebox(p_scale);
	}

	ClockGearsTheme();
	~ClockGearsTheme();
};
