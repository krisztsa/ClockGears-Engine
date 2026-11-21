#pragma once

#include "core/io/image.h"
#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"

class ClockGearsBootSplash {
public:
	static constexpr int BASE_DESIGN_WIDTH = 1920;
	static constexpr int BASE_DESIGN_HEIGHT = 1080;
	static constexpr int SPLASH_WIDTH = 1920;
	static constexpr int SPLASH_HEIGHT = 1080;
	static constexpr int TOTAL_STAGES = 7;
	static constexpr int MAX_PARTICLES = 120;
	static constexpr int MAX_AMBIENT_MOTES = 20;
	static constexpr int MAX_HEAD_SPARKS = 100;
	static constexpr float FRAME_BUDGET_MS = 16.7f;
	static constexpr uint64_t WATCHDOG_DEFAULT_TIMEOUT_MS = 15000;

	struct Palette {
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

	struct LayoutMetrics {
		Rect2i card_rect;
		Rect2i inner_frame_rect;
		Rect2i icon_rect;
		Rect2i badge_rect;
		Rect2i title_rect;
		Rect2i subtitle_rect;
		Rect2i credits_rect;
		Rect2i status_rect;
		Rect2i right_brand_rect;
		Rect2i progress_bar_rect;
		int corner_radius = 6;
		int frame_inset = 24;
		int icon_scale_multiple = 2;
		int ornament_gear_count = 0;
		int circuit_trace_count = 0;
		float layout_scale = 1.0f;
		float glow_breath_factor = 1.0f;
		float head_x = 0.0f;
		float head_y = 0.0f;
		bool indeterminate = false;
		bool glow_enabled = true;
		int active_particle_count = 0;
		int active_spark_count = 0;
		int active_ambient_count = 0;
		int particle_cap = MAX_PARTICLES;
		int pool_capacity = MAX_PARTICLES;
	};

	static bool is_saved_brass_theme();
	static bool are_glow_effects_enabled();
	static void set_glow_effects_override(bool p_has_override, bool p_enabled);
	static float compute_breathing_intensity(float p_time_sec);
	static void reset_particles(uint32_t p_seed = 0xC6E00004u, const Vector2i &p_screen_size = Vector2i(BASE_DESIGN_WIDTH, BASE_DESIGN_HEIGHT), bool p_brass = false);
	static void step_particles(float p_dt, float p_progress, bool p_completed, const Vector2i &p_screen_size = Vector2i(BASE_DESIGN_WIDTH, BASE_DESIGN_HEIGHT), bool p_brass = false, float p_last_frame_ms = 2.0f);
	static Dictionary get_particle_state();
	static Palette get_active_palette(bool p_force_brass = false, bool p_override_brass = false);
	static Color get_active_clear_color();
	static Vector<Vector2> get_rounded_window_polygon(int p_width = SPLASH_WIDTH, int p_height = SPLASH_HEIGHT, int p_radius = -1);
	static Vector2i get_primary_monitor_size(int *r_pos_x = nullptr, int *r_pos_y = nullptr, float *r_dpi_scale = nullptr);

	static Ref<Image> render_card(float p_progress, const String &p_status, float p_spinner_deg, bool p_completed, bool p_brass, LayoutMetrics *r_metrics = nullptr, const Vector2i &p_screen_size = Vector2i(BASE_DESIGN_WIDTH, BASE_DESIGN_HEIGHT), float p_dpi_scale = 1.0f, float p_anim_time_sec = -1.0f);
	static void present_stage(int p_stage, int p_total_stages, const String &p_status, bool p_completed = false);

	// Ventana propia independiente sin bordes, ciclo de vida y seguro anti-bloqueo (15 s).
	static void log_event(const String &p_event_line);
	static bool open_window(int p_screen = 0);
	static bool is_window_open();
	static void mark_ready();
	static void notify_first_frame_drawn(const String &p_context = "editor");
	static void close_with_fade(const String &p_reason = "first_frame_drawn");
	static void close_on_error(const String &p_error_reason);
	static void poll_watchdog();
	static void set_watchdog_timeout_ms(uint64_t p_timeout_ms);
	static Dictionary get_last_lifecycle_state();
};
