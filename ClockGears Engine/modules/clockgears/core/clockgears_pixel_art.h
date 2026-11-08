#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/dictionary.h"
#include "scene/2d/camera_2d.h"
#include "scene/resources/texture.h"

// ============================================================================
// ClockGearsPixelArt — Configuración de Pixel Art del Proyecto y Validadores (Anexo A4)
// ============================================================================
class ClockGearsPixelArt : public Object {
	GDCLASS(ClockGearsPixelArt, Object);

public:
	static constexpr int DEFAULT_TILE_SIZE = 32;
	static constexpr int DEFAULT_VIEWPORT_WIDTH = 960;  // 30 tiles de 32 px (16:9)
	static constexpr int DEFAULT_VIEWPORT_HEIGHT = 540; // 16.875 tiles de 32 px (16:9)

private:
	static ClockGearsPixelArt *singleton;
	int tile_size = DEFAULT_TILE_SIZE;
	Size2i base_resolution = Size2i(DEFAULT_VIEWPORT_WIDTH, DEFAULT_VIEWPORT_HEIGHT);
	bool integer_scaling_only = true;

protected:
	static void _bind_methods();

public:
	static ClockGearsPixelArt *get_singleton() { return singleton; }

	void apply_project_pixel_art_defaults();
	void set_base_resolution_preset(int p_preset_index); // 0 = 960x540, 1 = 640x360, 2 = 512x288
	Array get_resolution_presets() const;

	void set_tile_size(int p_size);
	int get_tile_size() const { return tile_size; }

	Size2i get_base_resolution() const { return base_resolution; }
	static Vector2 snap_to_pixel(const Vector2 &p_pos);
	static int compute_integer_scale_for_screen(const Size2i &p_screen_size, const Size2i &p_base_res = Size2i(DEFAULT_VIEWPORT_WIDTH, DEFAULT_VIEWPORT_HEIGHT));

	Dictionary validate_texture_dimensions(const Ref<Texture2D> &p_tex, bool p_require_tile_multiple = false) const;

	ClockGearsPixelArt();
	~ClockGearsPixelArt();
};

// ============================================================================
// ClockGearsPixelCamera2D — Cámara Pixel-Perfect 16:9 (Anexo A4.1)
// Redondea la posición de cámara al píxel entero, maneja la fila parcial
// en 960x540 (16.875 tiles de alto) y restringe el zoom a enteros (1x, 2x, 3x).
// ============================================================================
class ClockGearsPixelCamera2D : public Camera2D {
	GDCLASS(ClockGearsPixelCamera2D, Camera2D);

private:
	Vector2 logical_position;
	NodePath follow_target_path;
	bool smooth_follow_enabled = true;
	float follow_speed = 8.0f;
	int integer_zoom = 1;
	bool allow_fractional_zoom = false;

	// Efecto de temblor pixel-perfect (A1.3 / A2.4)
	float shake_intensity_px = 0.0f;
	float shake_remaining_sec = 0.0f;
	float shake_total_sec = 0.0f;
	Vector2i current_shake_offset;

	void _sync_pixel_position();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_logical_position(const Vector2 &p_pos);
	Vector2 get_logical_position() const { return logical_position; }

	void set_follow_target_path(const NodePath &p_path);
	NodePath get_follow_target_path() const { return follow_target_path; }

	void set_smooth_follow_enabled(bool p_enabled);
	bool is_smooth_follow_enabled() const { return smooth_follow_enabled; }

	void set_follow_speed(float p_speed);
	float get_follow_speed() const { return follow_speed; }

	void set_integer_zoom(int p_zoom);
	int get_integer_zoom() const { return integer_zoom; }

	void set_allow_fractional_zoom(bool p_allow);
	bool is_allow_fractional_zoom() const { return allow_fractional_zoom; }

	void start_shake(float p_intensity_px, float p_duration_sec);
	void stop_shake();

	ClockGearsPixelCamera2D();
};
