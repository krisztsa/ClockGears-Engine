#include "clockgears_picture_manager.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "scene/resources/image_texture.h"

ClockGearsPictureManager *ClockGearsPictureManager::singleton = nullptr;

void ClockGearsPictureManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_picture", "id", "path", "origin", "pos", "scale", "opacity", "blend", "duration_mode", "duration_sec", "auto_fadeout"), &ClockGearsPictureManager::show_picture, DEFVAL(0), DEFVAL(Vector2()), DEFVAL(Vector2(100, 100)), DEFVAL(1.0f), DEFVAL(0), DEFVAL(0), DEFVAL(0.0f), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("move_picture", "id", "origin", "pos", "scale", "opacity", "duration_sec", "easing"), &ClockGearsPictureManager::move_picture, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("rotate_picture", "id", "speed_deg_sec"), &ClockGearsPictureManager::rotate_picture);
	ClassDB::bind_method(D_METHOD("tint_picture", "id", "color", "duration_sec"), &ClockGearsPictureManager::tint_picture);
	ClassDB::bind_method(D_METHOD("erase_picture", "id"), &ClockGearsPictureManager::erase_picture);
	ClassDB::bind_method(D_METHOD("erase_all_pictures"), &ClockGearsPictureManager::erase_all_pictures);
	ClassDB::bind_method(D_METHOD("erase_pictures_on_message_close"), &ClockGearsPictureManager::erase_pictures_on_message_close);

	ClassDB::bind_method(D_METHOD("has_picture", "id"), &ClockGearsPictureManager::has_picture);
	ClassDB::bind_method(D_METHOD("get_picture_position", "id"), &ClockGearsPictureManager::get_picture_position);
	ClassDB::bind_method(D_METHOD("get_picture_opacity", "id"), &ClockGearsPictureManager::get_picture_opacity);
	ClassDB::bind_method(D_METHOD("get_picture_scale", "id"), &ClockGearsPictureManager::get_picture_scale);
	ClassDB::bind_method(D_METHOD("get_picture_rotation", "id"), &ClockGearsPictureManager::get_picture_rotation);
	ClassDB::bind_method(D_METHOD("get_picture_duration_mode", "id"), &ClockGearsPictureManager::get_picture_duration_mode);
	ClassDB::bind_method(D_METHOD("get_picture_life_timer", "id"), &ClockGearsPictureManager::get_picture_life_timer);
	ClassDB::bind_method(D_METHOD("is_picture_fading_out", "id"), &ClockGearsPictureManager::is_picture_fading_out);

	ClassDB::bind_method(D_METHOD("set_enforce_integer_scaling", "enforce"), &ClockGearsPictureManager::set_enforce_integer_scaling);
	ClassDB::bind_method(D_METHOD("is_integer_scaling_enforced"), &ClockGearsPictureManager::is_integer_scaling_enforced);

	ClassDB::bind_method(D_METHOD("start_screen_shake", "intensity", "speed", "duration_sec"), &ClockGearsPictureManager::start_screen_shake);
	ClassDB::bind_method(D_METHOD("stop_screen_shake"), &ClockGearsPictureManager::stop_screen_shake);
	ClassDB::bind_method(D_METHOD("is_screen_shaking"), &ClockGearsPictureManager::is_screen_shaking);
	ClassDB::bind_method(D_METHOD("start_screen_flash", "color", "duration_sec"), &ClockGearsPictureManager::start_screen_flash);
	ClassDB::bind_method(D_METHOD("set_screen_tint", "color", "duration_sec"), &ClockGearsPictureManager::set_screen_tint);
	ClassDB::bind_method(D_METHOD("update_pictures", "delta"), &ClockGearsPictureManager::update_pictures);
	ClassDB::bind_method(D_METHOD("set_weather", "type", "intensity"), &ClockGearsPictureManager::set_weather);
	ClassDB::bind_method(D_METHOD("get_weather_type"), &ClockGearsPictureManager::get_weather_type);

	BIND_CONSTANT(ORIGIN_TOP_LEFT);
	BIND_CONSTANT(ORIGIN_CENTER);

	BIND_CONSTANT(BLEND_NORMAL);
	BIND_CONSTANT(BLEND_ADDITIVE);
	BIND_CONSTANT(BLEND_MULTIPLY);
	BIND_CONSTANT(BLEND_SCREEN);

	BIND_CONSTANT(EASE_LINEAR);
	BIND_CONSTANT(EASE_IN);
	BIND_CONSTANT(EASE_OUT);
	BIND_CONSTANT(EASE_IN_OUT);

	BIND_CONSTANT(DUR_UNTIL_ERASED);
	BIND_CONSTANT(DUR_SECONDS);
	BIND_CONSTANT(DUR_NEXT_MESSAGE);

	ADD_SIGNAL(MethodInfo("picture_moved", PropertyInfo(Variant::INT, "id")));
}

ClockGearsPictureManager::ClockGearsPictureManager() {
	if (!singleton) {
		singleton = this;
	}
	set_process(true);
	set_mouse_filter(MOUSE_FILTER_IGNORE);

	for (int i = 0; i <= MAX_PICTURES; i++) {
		pictures[i].id = i;
		pictures[i].active = false;
	}

	// Tint de pantalla
	tint_rect = memnew(ColorRect);
	tint_rect->set_anchors_preset(PRESET_FULL_RECT);
	tint_rect->set_color(Color(0, 0, 0, 0));
	tint_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(tint_rect);

	// Flash de pantalla
	flash_rect = memnew(ColorRect);
	flash_rect->set_anchors_preset(PRESET_FULL_RECT);
	flash_rect->set_color(Color(1, 1, 1, 0));
	flash_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(flash_rect);
}

ClockGearsPictureManager::~ClockGearsPictureManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

float ClockGearsPictureManager::_calculate_easing(float t, EasingType easing) const {
	t = CLAMP(t, 0.0f, 1.0f);
	switch (easing) {
		case EASE_IN:
			return t * t;
		case EASE_OUT:
			return t * (2.0f - t);
		case EASE_IN_OUT:
			return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
		case EASE_LINEAR:
		default:
			return t;
	}
}

void ClockGearsPictureManager::_update_slot_transform(PictureSlot &slot) {
	if (!slot.texture_rect) return;

	Vector2 final_pos = slot.position + shake_offset;
	Vector2 final_scale = slot.scale;

	// Snap entero solo se aplica al ampliar (>= 1.0f), nunca al reducir
	if (enforce_integer_scaling) {
		if (final_scale.x >= 1.0f) {
			final_scale.x = Math::round(final_scale.x);
		}
		if (final_scale.y >= 1.0f) {
			final_scale.y = Math::round(final_scale.y);
		}
	}

	Ref<Texture2D> tex = slot.texture_rect->get_texture();
	Size2 tex_size = tex.is_valid() ? tex->get_size() : Size2(32, 32);

	if (slot.origin == ORIGIN_CENTER) {
		final_pos -= (tex_size * final_scale) * 0.5f;
	}

	slot.texture_rect->set_position(final_pos.floor());
	slot.texture_rect->set_scale(final_scale);
	slot.texture_rect->set_rotation_degrees(slot.rotation);

	Color mod = slot.tint;
	mod.a *= slot.opacity;
	slot.texture_rect->set_modulate(mod);
}

void ClockGearsPictureManager::show_picture(int p_id, const String &p_path, int p_origin, const Vector2 &p_pos, const Vector2 &p_scale, float p_opacity, int p_blend, int p_duration_mode, float p_duration_sec, bool p_auto_fadeout) {
	if (p_id < 1 || p_id > MAX_PICTURES) return;

	PictureSlot &slot = pictures[p_id];
	slot.active = true;
	slot.path = p_path;
	slot.origin = (OriginType)CLAMP(p_origin, 0, 1);
	slot.position = p_pos;
	slot.target_position = p_pos;
	slot.scale = p_scale / 100.0f;
	slot.target_scale = slot.scale;
	slot.opacity = CLAMP(p_opacity, 0.0f, 1.0f);
	slot.target_opacity = slot.opacity;
	slot.blend = (BlendMode)CLAMP(p_blend, 0, 3);
	slot.rotation = 0.0f;
	slot.rotation_speed = 0.0f;
	slot.tint = Color(1, 1, 1, 1);
	slot.target_tint = Color(1, 1, 1, 1);
	slot.move_duration = 0.0f;

	// Duración automática (Fase 7B)
	slot.duration_mode = (DurationMode)CLAMP(p_duration_mode, 0, 2);
	slot.life_duration = MAX(0.0f, p_duration_sec);
	slot.life_timer = slot.life_duration;
	slot.auto_fadeout = p_auto_fadeout;
	slot.fading_out = false;
	slot.fadeout_timer = 0.0f;

	if (!slot.texture_rect) {
		slot.texture_rect = memnew(TextureRect);
		slot.texture_rect->set_mouse_filter(MOUSE_FILTER_IGNORE);
		slot.texture_rect->set_texture_filter(TEXTURE_FILTER_NEAREST);
		add_child(slot.texture_rect);
	}

	Ref<Texture2D> tex;
	if (ResourceLoader::exists(p_path)) {
		tex = ResourceLoader::load(p_path);
	}
	if (tex.is_null() && FileAccess::exists(p_path)) {
		Ref<Image> img = Image::load_from_file(p_path);
		if (img.is_valid() && !img->is_empty()) {
			tex = ImageTexture::create_from_image(img);
		}
	}
	if (tex.is_null()) {
		// Fallback pixel art para test / prototipo rápido
		Ref<Image> img = Image::create_empty(32, 32, false, Image::FORMAT_RGBA8);
		img->fill(Color(0.8f, 0.4f, 0.2f, 1.0f));
		tex = ImageTexture::create_from_image(img);
	}

	if (tex.is_valid()) {
		slot.texture_rect->set_texture(tex);
	}
	slot.texture_rect->show();

	_update_slot_transform(slot);
}

void ClockGearsPictureManager::move_picture(int p_id, int p_origin, const Vector2 &p_pos, const Vector2 &p_scale, float p_opacity, float p_duration_sec, int p_easing) {
	if (p_id < 1 || p_id > MAX_PICTURES) return;
	PictureSlot &slot = pictures[p_id];
	if (!slot.active) return;

	slot.origin = (OriginType)CLAMP(p_origin, 0, 1);
	slot.start_position = slot.position;
	slot.target_position = p_pos;

	slot.start_scale = slot.scale;
	slot.target_scale = p_scale / 100.0f;

	slot.start_opacity = slot.opacity;
	slot.target_opacity = CLAMP(p_opacity, 0.0f, 1.0f);

	slot.move_easing = (EasingType)CLAMP(p_easing, 0, 3);
	slot.move_duration = MAX(0.001f, p_duration_sec);
	slot.move_timer = 0.0f;
}

void ClockGearsPictureManager::rotate_picture(int p_id, float p_speed_deg_sec) {
	if (p_id < 1 || p_id > MAX_PICTURES) return;
	PictureSlot &slot = pictures[p_id];
	if (!slot.active) return;
	slot.rotation_speed = p_speed_deg_sec;
}

void ClockGearsPictureManager::tint_picture(int p_id, const Color &p_color, float p_duration_sec) {
	if (p_id < 1 || p_id > MAX_PICTURES) return;
	PictureSlot &slot = pictures[p_id];
	if (!slot.active) return;

	slot.start_tint = slot.tint;
	slot.target_tint = p_color;
	slot.tint_duration = MAX(0.001f, p_duration_sec);
	slot.tint_timer = 0.0f;
}

void ClockGearsPictureManager::erase_picture(int p_id) {
	if (p_id < 1 || p_id > MAX_PICTURES) return;
	PictureSlot &slot = pictures[p_id];
	slot.active = false;
	if (slot.texture_rect) {
		slot.texture_rect->hide();
	}
}

void ClockGearsPictureManager::erase_all_pictures() {
	for (int i = 1; i <= MAX_PICTURES; i++) {
		erase_picture(i);
	}
}

void ClockGearsPictureManager::erase_pictures_on_message_close() {
	for (int i = 1; i <= MAX_PICTURES; i++) {
		if (pictures[i].active && pictures[i].duration_mode == DUR_NEXT_MESSAGE) {
			erase_picture(i);
		}
	}
}

bool ClockGearsPictureManager::has_picture(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return false;
	return pictures[p_id].active;
}

Vector2 ClockGearsPictureManager::get_picture_position(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return Vector2();
	return pictures[p_id].position;
}

float ClockGearsPictureManager::get_picture_opacity(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return 0.0f;
	return pictures[p_id].opacity;
}

Vector2 ClockGearsPictureManager::get_picture_scale(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return Vector2(1, 1);
	return pictures[p_id].scale;
}

float ClockGearsPictureManager::get_picture_rotation(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return 0.0f;
	return pictures[p_id].rotation;
}

int ClockGearsPictureManager::get_picture_duration_mode(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return 0;
	return int(pictures[p_id].duration_mode);
}

float ClockGearsPictureManager::get_picture_life_timer(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return 0.0f;
	return pictures[p_id].life_timer;
}

bool ClockGearsPictureManager::is_picture_fading_out(int p_id) const {
	if (p_id < 1 || p_id > MAX_PICTURES) return false;
	return pictures[p_id].fading_out;
}

void ClockGearsPictureManager::start_screen_shake(float p_intensity, float p_speed, float p_duration_sec) {
	shake_intensity = p_intensity;
	shake_speed = p_speed;
	shake_duration = p_duration_sec;
	shake_timer = p_duration_sec;
}

void ClockGearsPictureManager::stop_screen_shake() {
	shake_timer = 0.0f;
	shake_offset = Vector2();
}

void ClockGearsPictureManager::start_screen_flash(const Color &p_color, float p_duration_sec) {
	flash_color = p_color;
	flash_duration = MAX(0.001f, p_duration_sec);
	flash_timer = flash_duration;
	if (flash_rect) {
		flash_rect->set_color(flash_color);
	}
}

void ClockGearsPictureManager::set_screen_tint(const Color &p_color, float p_duration_sec) {
	target_tint = p_color;
	tint_duration = MAX(0.001f, p_duration_sec);
	tint_timer = 0.0f;
}

void ClockGearsPictureManager::set_weather(const String &p_type, float p_intensity) {
	current_weather = p_type;
	weather_intensity = CLAMP(p_intensity, 0.0f, 1.0f);
}

void ClockGearsPictureManager::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		update_pictures(get_process_delta_time());
	}
}

void ClockGearsPictureManager::update_pictures(float p_delta) {
	// 1. Temblor de pantalla (Screen Shake)
	if (shake_timer > 0.0f) {
		shake_timer -= p_delta;
		float progress = CLAMP(shake_timer / shake_duration, 0.0f, 1.0f);
		float current_amp = shake_intensity * 2.5f * progress;
		if (current_amp >= 0.5f) {
			float sx = Math::sin(shake_timer * shake_speed * 12.0f) * current_amp;
			float sy = Math::cos(shake_timer * shake_speed * 15.0f) * (current_amp * 0.7f);
			shake_offset = Vector2(Math::round(sx), Math::round(sy));
		} else {
			shake_offset = Vector2();
		}
		if (shake_timer <= 0.0f) {
			shake_offset = Vector2();
		}
	}

	// 2. Destello de pantalla (Screen Flash)
	if (flash_timer > 0.0f && flash_rect) {
		flash_timer -= p_delta;
		float alpha = CLAMP(flash_timer / flash_duration, 0.0f, 1.0f);
		Color fc = flash_color;
		fc.a = flash_color.a * alpha;
		flash_rect->set_color(fc);
	}

	// 3. Tinte de pantalla (Screen Tint)
	if (tint_duration > 0.001f && tint_rect) {
		tint_timer += p_delta;
		float t = CLAMP(tint_timer / tint_duration, 0.0f, 1.0f);
		current_tint = current_tint.lerp(target_tint, t);
		tint_rect->set_color(current_tint);
		if (t >= 1.0f) {
			tint_duration = 0.0f;
		}
	}

	// 4. Actualizar las 100 imágenes
	for (int i = 1; i <= MAX_PICTURES; i++) {
		PictureSlot &slot = pictures[i];
		if (!slot.active) continue;

		// Duración automática (DUR_SECONDS)
		if (slot.duration_mode == DUR_SECONDS) {
			if (!slot.fading_out) {
				slot.life_timer -= p_delta;
				if (slot.life_timer <= 0.0f) {
					if (slot.auto_fadeout) {
						slot.fading_out = true;
						slot.fadeout_timer = 0.3f;
					} else {
						erase_picture(i);
						continue;
					}
				}
			} else {
				slot.fadeout_timer -= p_delta;
				if (slot.fadeout_timer <= 0.0f) {
					erase_picture(i);
					continue;
				} else {
					float fade_ratio = CLAMP(slot.fadeout_timer / 0.3f, 0.0f, 1.0f);
					slot.opacity = slot.target_opacity * fade_ratio;
				}
			}
		}

		// Movimiento interpolado
		if (slot.move_duration > 0.001f) {
			slot.move_timer += p_delta;
			float t = CLAMP(slot.move_timer / slot.move_duration, 0.0f, 1.0f);
			float ease_t = _calculate_easing(t, slot.move_easing);

			slot.position = slot.start_position.lerp(slot.target_position, ease_t);
			slot.scale = slot.start_scale.lerp(slot.target_scale, ease_t);
			slot.opacity = Math::lerp(slot.start_opacity, slot.target_opacity, ease_t);

			if (t >= 1.0f) {
				slot.move_duration = 0.0f;
				emit_signal("picture_moved", slot.id);
			}
		}

		// Rotación continua
		if (Math::abs(slot.rotation_speed) > 0.01f) {
			slot.rotation += slot.rotation_speed * p_delta;
		}

		// Tinte interpolado
		if (slot.tint_duration > 0.001f) {
			slot.tint_timer += p_delta;
			float t = CLAMP(slot.tint_timer / slot.tint_duration, 0.0f, 1.0f);
			slot.tint = slot.start_tint.lerp(slot.target_tint, t);
			if (t >= 1.0f) {
				slot.tint_duration = 0.0f;
			}
		}

		_update_slot_transform(slot);
	}
}
