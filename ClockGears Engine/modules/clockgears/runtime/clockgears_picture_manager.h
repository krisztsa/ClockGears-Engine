#ifndef CLOCKGEARS_PICTURE_MANAGER_H
#define CLOCKGEARS_PICTURE_MANAGER_H

#include "core/object/class_db.h"
#include "scene/gui/control.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/color_rect.h"
#include "core/templates/hash_map.h"

// ============================================================================
// ClockGearsPictureManager — Gestor de Imágenes y Pantalla (Anexo A1.3)
// Soporte de 100 imágenes simultáneas, escalado pixel-art entero, mezclas,
// temblor, flash, tintes y efectos climáticos en pixel art.
// ============================================================================
class ClockGearsPictureManager : public Control {
	GDCLASS(ClockGearsPictureManager, Control);

public:
	enum OriginType {
		ORIGIN_TOP_LEFT = 0,
		ORIGIN_CENTER = 1
	};

	enum BlendMode {
		BLEND_NORMAL = 0,
		BLEND_ADDITIVE = 1,
		BLEND_MULTIPLY = 2,
		BLEND_SCREEN = 3
	};

	enum EasingType {
		EASE_LINEAR = 0,
		EASE_IN = 1,
		EASE_OUT = 2,
		EASE_IN_OUT = 3
	};

	enum DurationMode {
		DUR_UNTIL_ERASED = 0,
		DUR_SECONDS = 1,
		DUR_NEXT_MESSAGE = 2
	};

	struct PictureSlot {
		int id = 0;
		bool active = false;
		String path;
		OriginType origin = ORIGIN_TOP_LEFT;
		Vector2 position;
		Vector2 target_position;
		Vector2 start_position;

		Vector2 scale = Vector2(1, 1);
		Vector2 target_scale = Vector2(1, 1);
		Vector2 start_scale = Vector2(1, 1);

		float opacity = 1.0f;
		float target_opacity = 1.0f;
		float start_opacity = 1.0f;

		float rotation = 0.0f;
		float rotation_speed = 0.0f; // Grados por segundo

		Color tint = Color(1, 1, 1, 1);
		Color target_tint = Color(1, 1, 1, 1);
		Color start_tint = Color(1, 1, 1, 1);

		BlendMode blend = BLEND_NORMAL;
		int layer = 1; // 0=Detrás mapa, 1=Sobre mapa / bajo UI, 2=Sobre UI

		float move_timer = 0.0f;
		float move_duration = 0.0f;
		EasingType move_easing = EASE_LINEAR;

		float tint_timer = 0.0f;
		float tint_duration = 0.0f;

		// Duración automática (Fase 7B)
		DurationMode duration_mode = DUR_UNTIL_ERASED;
		float life_duration = 0.0f;
		float life_timer = 0.0f;
		bool auto_fadeout = false;
		bool fading_out = false;
		float fadeout_timer = 0.0f;

		TextureRect *texture_rect = nullptr;
	};

	static constexpr int MAX_PICTURES = 100;

private:
	static ClockGearsPictureManager *singleton;

	PictureSlot pictures[MAX_PICTURES + 1];
	bool enforce_integer_scaling = false;

	// Efectos de pantalla (Shake, Flash, Tint, Viñeta)
	float shake_timer = 0.0f;
	float shake_duration = 0.0f;
	float shake_intensity = 0.0f;
	float shake_speed = 20.0f;
	Vector2 shake_offset;

	ColorRect *flash_rect = nullptr;
	float flash_timer = 0.0f;
	float flash_duration = 0.0f;
	Color flash_color;

	ColorRect *tint_rect = nullptr;
	Color current_tint = Color(0, 0, 0, 0);
	Color target_tint = Color(0, 0, 0, 0);
	float tint_timer = 0.0f;
	float tint_duration = 0.0f;

	// Clima (Partículas en pixel art)
	String current_weather = "none";
	float weather_intensity = 0.0f;

	void _update_slot_transform(PictureSlot &slot);
	float _calculate_easing(float t, EasingType easing) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static ClockGearsPictureManager *get_singleton() { return singleton; }

	// Operaciones con Imágenes
	void show_picture(int p_id, const String &p_path, int p_origin = 0, const Vector2 &p_pos = Vector2(), const Vector2 &p_scale = Vector2(100, 100), float p_opacity = 1.0f, int p_blend = 0, int p_duration_mode = 0, float p_duration_sec = 0.0f, bool p_auto_fadeout = false);
	void move_picture(int p_id, int p_origin, const Vector2 &p_pos, const Vector2 &p_scale, float p_opacity, float p_duration_sec, int p_easing = 0);
	void rotate_picture(int p_id, float p_speed_deg_sec);
	void tint_picture(int p_id, const Color &p_color, float p_duration_sec);
	void erase_picture(int p_id);
	void erase_all_pictures();
	void erase_pictures_on_message_close();

	bool has_picture(int p_id) const;
	Vector2 get_picture_position(int p_id) const;
	float get_picture_opacity(int p_id) const;
	Vector2 get_picture_scale(int p_id) const;
	float get_picture_rotation(int p_id) const;
	int get_picture_duration_mode(int p_id) const;
	float get_picture_life_timer(int p_id) const;
	bool is_picture_fading_out(int p_id) const;

	void set_enforce_integer_scaling(bool p_enforce) { enforce_integer_scaling = p_enforce; }
	bool is_integer_scaling_enforced() const { return enforce_integer_scaling; }

	// Efectos de Pantalla
	void start_screen_shake(float p_intensity, float p_speed, float p_duration_sec);
	void stop_screen_shake();
	bool is_screen_shaking() const { return shake_timer > 0.0f; }

	void start_screen_flash(const Color &p_color, float p_duration_sec);
	void set_screen_tint(const Color &p_color, float p_duration_sec);
	void set_weather(const String &p_type, float p_intensity);
	String get_weather_type() const { return current_weather; }

	void update_pictures(float p_delta);

	ClockGearsPictureManager();
	~ClockGearsPictureManager();
};

#endif // CLOCKGEARS_PICTURE_MANAGER_H
