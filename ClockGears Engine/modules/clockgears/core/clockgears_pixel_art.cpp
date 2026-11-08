#include "clockgears_pixel_art.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/math/random_pcg.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

ClockGearsPixelArt *ClockGearsPixelArt::singleton = nullptr;

Vector2 ClockGearsPixelArt::snap_to_pixel(const Vector2 &p_pos) {
	return Vector2(Math::round(p_pos.x), Math::round(p_pos.y));
}

int ClockGearsPixelArt::compute_integer_scale_for_screen(const Size2i &p_screen_size, const Size2i &p_base_res) {
	if (p_base_res.x <= 0 || p_base_res.y <= 0) {
		return 1;
	}
	int scale_x = p_screen_size.x / p_base_res.x;
	int scale_y = p_screen_size.y / p_base_res.y;
	return MAX(1, MIN(scale_x, scale_y));
}

void ClockGearsPixelArt::apply_project_pixel_art_defaults() {
	ProjectSettings *ps = ProjectSettings::get_singleton();
	if (!ps) {
		return;
	}
	// A4.1: Resolución base 960x540 (16:9), modo canvas_items, aspecto keep, escala entera, NEAREST y snap 2D a píxel
	ps->set_setting("display/window/size/viewport_width", base_resolution.x);
	ps->set_setting("display/window/size/viewport_height", base_resolution.y);
	ps->set_setting("display/window/stretch/mode", "canvas_items");
	ps->set_setting("display/window/stretch/aspect", "keep");
	ps->set_setting("display/window/stretch/scale_mode", "integer");
	ps->set_setting("rendering/textures/canvas_textures/default_texture_filter", 0); // Nearest
	ps->set_setting("rendering/2d/snap/snap_2d_transforms_to_pixel", true);
	ps->set_setting("rendering/2d/snap/snap_2d_vertices_to_pixel", true);
	ps->set_setting("gui/common/snap_controls_to_pixels", true);

	SceneTree *st = SceneTree::get_singleton();
	if (st && st->get_root()) {
		st->get_root()->set_snap_2d_transforms_to_pixel(true);
		st->get_root()->set_snap_2d_vertices_to_pixel(true);
		st->get_root()->set_default_canvas_item_texture_filter( Viewport::DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST );
	}
}

void ClockGearsPixelArt::set_base_resolution_preset(int p_preset_index) {
	switch (p_preset_index) {
		case 1:
			base_resolution = Size2i(640, 360); // 20 x 11.25 tiles (3x a 1080p, 4x a 1440p)
			break;
		case 2:
			base_resolution = Size2i(512, 288); // 16 x 9 tiles exactos de 32px
			break;
		case 0:
		default:
			base_resolution = Size2i(960, 540); // 30 x 16.875 tiles (2x a 1080p, 4x a 4K)
			break;
	}
	apply_project_pixel_art_defaults();
}

Array ClockGearsPixelArt::get_resolution_presets() const {
	Array arr;
	auto push_preset = [&arr](const char *p_label, int p_w, int p_h) {
		Dictionary d;
		d["label"] = String::utf8(p_label);
		d["width"] = p_w;
		d["height"] = p_h;
		arr.push_back(d);
	};
	push_preset("960x540 (Por defecto — 16:9, 2x en 1080p, 4x en 4K)", 960, 540);
	push_preset("640x360 (16:9 — 2x en 720p, 3x en 1080p, 4x en 1440p)", 640, 360);
	push_preset("512x288 (16:9 — 16x9 tiles exactos de 32px)", 512, 288);
	return arr;
}

void ClockGearsPixelArt::set_tile_size(int p_size) {
	tile_size = MAX(8, p_size);
}

Dictionary ClockGearsPixelArt::validate_texture_dimensions(const Ref<Texture2D> &p_tex, bool p_require_tile_multiple) const {
	Dictionary res;
	res["valid"] = true;
	res["warnings"] = PackedStringArray();

	if (p_tex.is_null()) {
		res["valid"] = false;
		return res;
	}

	int w = p_tex->get_width();
	int h = p_tex->get_height();
	PackedStringArray warnings;

	if ((w % 2) != 0 || (h % 2) != 0) {
		warnings.push_back(vformat(String::utf8("La imagen (%dx%d px) tiene dimensiones impares; puede desalinearse en pixel art."), w, h));
	}
	if (p_require_tile_multiple && tile_size > 0) {
		if ((w % tile_size) != 0 || (h % tile_size) != 0) {
			warnings.push_back(vformat(String::utf8("La hoja (%dx%d px) no es múltiplo del tamaño de tile (%dx%d px)."), w, h, tile_size, tile_size));
		}
	}

	res["width"] = w;
	res["height"] = h;
	res["valid"] = warnings.is_empty();
	res["warnings"] = warnings;
	return res;
}

void ClockGearsPixelArt::_bind_methods() {
	ClassDB::bind_method(D_METHOD("apply_project_pixel_art_defaults"), &ClockGearsPixelArt::apply_project_pixel_art_defaults);
	ClassDB::bind_method(D_METHOD("set_base_resolution_preset", "preset_index"), &ClockGearsPixelArt::set_base_resolution_preset);
	ClassDB::bind_method(D_METHOD("get_resolution_presets"), &ClockGearsPixelArt::get_resolution_presets);
	ClassDB::bind_method(D_METHOD("set_tile_size", "size"), &ClockGearsPixelArt::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &ClockGearsPixelArt::get_tile_size);
	ClassDB::bind_method(D_METHOD("get_base_resolution"), &ClockGearsPixelArt::get_base_resolution);
	ClassDB::bind_method(D_METHOD("validate_texture_dimensions", "texture", "require_tile_multiple"), &ClockGearsPixelArt::validate_texture_dimensions, DEFVAL(false));
}

ClockGearsPixelArt::ClockGearsPixelArt() {
	singleton = this;
}

ClockGearsPixelArt::~ClockGearsPixelArt() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

// ============================================================================
// ClockGearsPixelCamera2D
// ============================================================================

void ClockGearsPixelCamera2D::_sync_pixel_position() {
	// A4.1: Redondear siempre la posición efectiva de la cámara al píxel entero
	Vector2 snapped(Math::round(logical_position.x) + current_shake_offset.x,
			Math::round(logical_position.y) + current_shake_offset.y);
	set_position(snapped);
}

void ClockGearsPixelCamera2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			logical_position = get_position();
			_sync_pixel_position();
			set_process(true);
		} break;
		case NOTIFICATION_PROCESS: {
			double dt = get_process_delta_time();
			if (!follow_target_path.is_empty()) {
				Node2D *target = Object::cast_to<Node2D>(get_node_or_null(follow_target_path));
				if (target) {
					Vector2 target_pos = target->get_global_position();
					if (smooth_follow_enabled && follow_speed > 0.0f) {
						float t = CLAMP(float(dt) * follow_speed, 0.0f, 1.0f);
						logical_position = logical_position.lerp(target_pos, t);
					} else {
						logical_position = target_pos;
					}
				}
			}

			if (shake_remaining_sec > 0.0f) {
				shake_remaining_sec = MAX(0.0f, shake_remaining_sec - float(dt));
				float ratio = (shake_total_sec > 0.0f) ? (shake_remaining_sec / shake_total_sec) : 0.0f;
				int max_off = int(Math::round(shake_intensity_px * ratio));
				if (max_off > 0) {
					static RandomPCG shake_rng;
					int ox = int(shake_rng.rand() % uint32_t(max_off * 2 + 1)) - max_off;
					int oy = int(shake_rng.rand() % uint32_t(max_off * 2 + 1)) - max_off;
					current_shake_offset = Vector2i(ox, oy);
				} else {
					current_shake_offset = Vector2i();
				}
			} else {
				current_shake_offset = Vector2i();
			}

			_sync_pixel_position();
		} break;
	}
}

void ClockGearsPixelCamera2D::set_logical_position(const Vector2 &p_pos) {
	logical_position = p_pos;
	_sync_pixel_position();
}

void ClockGearsPixelCamera2D::set_follow_target_path(const NodePath &p_path) {
	follow_target_path = p_path;
}

void ClockGearsPixelCamera2D::set_smooth_follow_enabled(bool p_enabled) {
	smooth_follow_enabled = p_enabled;
}

void ClockGearsPixelCamera2D::set_follow_speed(float p_speed) {
	follow_speed = MAX(0.1f, p_speed);
}

void ClockGearsPixelCamera2D::set_integer_zoom(int p_zoom) {
	integer_zoom = CLAMP(p_zoom, 1, 8);
	set_zoom(Vector2(real_t(integer_zoom), real_t(integer_zoom)));
}

void ClockGearsPixelCamera2D::set_allow_fractional_zoom(bool p_allow) {
	allow_fractional_zoom = p_allow;
	if (!allow_fractional_zoom) {
		set_integer_zoom(MAX(1, int(Math::round(get_zoom().x))));
	}
}

void ClockGearsPixelCamera2D::start_shake(float p_intensity_px, float p_duration_sec) {
	shake_intensity_px = MAX(0.0f, p_intensity_px);
	shake_total_sec = MAX(0.0f, p_duration_sec);
	shake_remaining_sec = shake_total_sec;
}

void ClockGearsPixelCamera2D::stop_shake() {
	shake_remaining_sec = 0.0f;
	current_shake_offset = Vector2i();
	_sync_pixel_position();
}

void ClockGearsPixelCamera2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_logical_position", "pos"), &ClockGearsPixelCamera2D::set_logical_position);
	ClassDB::bind_method(D_METHOD("get_logical_position"), &ClockGearsPixelCamera2D::get_logical_position);
	ClassDB::bind_method(D_METHOD("set_follow_target_path", "path"), &ClockGearsPixelCamera2D::set_follow_target_path);
	ClassDB::bind_method(D_METHOD("get_follow_target_path"), &ClockGearsPixelCamera2D::get_follow_target_path);
	ClassDB::bind_method(D_METHOD("set_smooth_follow_enabled", "enabled"), &ClockGearsPixelCamera2D::set_smooth_follow_enabled);
	ClassDB::bind_method(D_METHOD("is_smooth_follow_enabled"), &ClockGearsPixelCamera2D::is_smooth_follow_enabled);
	ClassDB::bind_method(D_METHOD("set_follow_speed", "speed"), &ClockGearsPixelCamera2D::set_follow_speed);
	ClassDB::bind_method(D_METHOD("get_follow_speed"), &ClockGearsPixelCamera2D::get_follow_speed);
	ClassDB::bind_method(D_METHOD("set_integer_zoom", "zoom"), &ClockGearsPixelCamera2D::set_integer_zoom);
	ClassDB::bind_method(D_METHOD("get_integer_zoom"), &ClockGearsPixelCamera2D::get_integer_zoom);
	ClassDB::bind_method(D_METHOD("set_allow_fractional_zoom", "allow"), &ClockGearsPixelCamera2D::set_allow_fractional_zoom);
	ClassDB::bind_method(D_METHOD("is_allow_fractional_zoom"), &ClockGearsPixelCamera2D::is_allow_fractional_zoom);
	ClassDB::bind_method(D_METHOD("start_shake", "intensity_px", "duration_sec"), &ClockGearsPixelCamera2D::start_shake);
	ClassDB::bind_method(D_METHOD("stop_shake"), &ClockGearsPixelCamera2D::stop_shake);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "logical_position"), "set_logical_position", "get_logical_position");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "follow_target_path"), "set_follow_target_path", "get_follow_target_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "smooth_follow_enabled"), "set_smooth_follow_enabled", "is_smooth_follow_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "follow_speed", PROPERTY_HINT_RANGE, "0.5,60.0,0.5"), "set_follow_speed", "get_follow_speed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "integer_zoom", PROPERTY_HINT_RANGE, "1,8,1"), "set_integer_zoom", "get_integer_zoom");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_fractional_zoom"), "set_allow_fractional_zoom", "is_allow_fractional_zoom");
}

ClockGearsPixelCamera2D::ClockGearsPixelCamera2D() {
	set_anchor_mode(ANCHOR_MODE_DRAG_CENTER);
	set_position_smoothing_enabled(false); // Manejado por nuestra interpolación pixel-perfect
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	set_integer_zoom(1);
}
