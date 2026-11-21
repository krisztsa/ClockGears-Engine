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
	static constexpr int SPLASH_WIDTH = 800;
	static constexpr int SPLASH_HEIGHT = 450;
	static constexpr int TOTAL_STAGES = 7;
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
		Rect2i icon_rect;
		Rect2i badge_rect;
		Rect2i title_rect;
		Rect2i subtitle_rect;
		Rect2i credits_rect;
		Rect2i status_rect;
		Rect2i right_brand_rect;
		Rect2i progress_bar_rect;
		int corner_radius = 6;
	};

	static bool is_saved_brass_theme();
	static Palette get_active_palette(bool p_force_brass = false, bool p_override_brass = false);
	static Color get_active_clear_color();
	static Vector<Vector2> get_rounded_window_polygon(int p_width = SPLASH_WIDTH, int p_height = SPLASH_HEIGHT, int p_radius = -1);

	static Ref<Image> render_card(float p_progress, const String &p_status, float p_spinner_deg, bool p_completed, bool p_brass, LayoutMetrics *r_metrics = nullptr);
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
