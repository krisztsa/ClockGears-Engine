#pragma once

#include "clockgears_types.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/math/random_pcg.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "scene/2d/node_2d.h"

// ============================================================================
// Triggers estándar de animación (Fase 8A - Sección 9)
// ============================================================================
enum ClockGearsAnimTrigger {
	ANIM_TRIGGER_NONE = 0,
	ANIM_TRIGGER_ATTACK = 1,
	ANIM_TRIGGER_SKILL = 2,
	ANIM_TRIGGER_ITEM = 3,
	ANIM_TRIGGER_DAMAGE = 4,
	ANIM_TRIGGER_STATE_ADD = 5,
	ANIM_TRIGGER_DEATH = 6,
	ANIM_TRIGGER_TURN_START = 7,
	ANIM_TRIGGER_VICTORY = 8,
	ANIM_TRIGGER_MAP_WALK = 9,
	ANIM_TRIGGER_MAP_TALK = 10,
	ANIM_TRIGGER_CUSTOM = 11,
	ANIM_TRIGGER_IDLE = 12,
	ANIM_TRIGGER_DEFEND = 13,
	ANIM_TRIGGER_MAX = 14
};

// ============================================================================
// Definición de una animación individual (desde CAT_ANIMATIONS o dinámica)
// ============================================================================
struct ClockGearsAnimDef {
	int id = 0;
	String name;
	String graphic_path;
	int frame_count = 4;
	float frame_duration = 0.1f;
	bool loop = false;
	Color flash_color = Color(1, 1, 1, 0);
	float flash_duration = 0.0f;
	String sound_effect;
	int alignment = 0; // 0=Centro, 1=Cabeza, 2=Pies, 3=Pantalla Completa

	Dictionary to_dict() const {
		Dictionary d;
		d["id"] = id;
		d["name"] = name;
		d["graphic_path"] = graphic_path;
		d["frame_count"] = frame_count;
		d["frame_duration"] = frame_duration;
		d["loop"] = loop;
		d["flash_color"] = flash_color;
		d["flash_duration"] = flash_duration;
		d["sound_effect"] = sound_effect;
		d["alignment"] = alignment;
		return d;
	}

	void from_dict(const Dictionary &p_d) {
		id = int(p_d.get("id", id));
		name = p_d.get("name", name);
		graphic_path = p_d.get("graphic_path", graphic_path);
		frame_count = int(p_d.get("frame_count", frame_count));
		frame_duration = float(p_d.get("frame_duration", frame_duration));
		loop = bool(p_d.get("loop", loop));
		flash_color = p_d.get("flash_color", flash_color);
		flash_duration = float(p_d.get("flash_duration", flash_duration));
		sound_effect = p_d.get("sound_effect", sound_effect);
		alignment = int(p_d.get("alignment", p_d.get("type", alignment)));
	}
};

// ============================================================================
// Frame individual de Animación (Ruta de spritesheet + región propia)
// ============================================================================
struct ClockGearsAnimationFrame {
	String sheet_path;
	Rect2i region;
	float duration = 0.0f; // 0.0 = por defecto (1.0f / fps de la animación)
	bool is_random_duration = false;
	float duration_min = 0.1f;
	float duration_max = 0.5f;

	float get_duration(float p_default_fps = 6.0f) const {
		if (is_random_duration) {
			float dmin = MAX(0.01f, duration_min);
			float dmax = MAX(dmin, duration_max);
			return dmin + (float(Math::randf()) * (dmax - dmin));
		}
		if (duration > 0.0f) {
			return duration;
		}
		return 1.0f / MAX(0.1f, p_default_fps);
	}

	Dictionary to_dict() const {
		Dictionary d;
		d["sheet"] = sheet_path;
		d["x"] = region.position.x;
		d["y"] = region.position.y;
		d["w"] = region.size.x;
		d["h"] = region.size.y;
		d["duration"] = duration;
		d["is_random_duration"] = is_random_duration;
		d["duration_min"] = duration_min;
		d["duration_max"] = duration_max;
		return d;
	}

	void from_dict(const Dictionary &p_d) {
		sheet_path = p_d.get("sheet", p_d.get("texture_path", p_d.get("spritesheet_path", "")));
		if (p_d.has("region")) {
			Rect2 r = p_d.get("region", Rect2());
			region = Rect2i(r);
		} else {
			int x = int(p_d.get("x", 0));
			int y = int(p_d.get("y", 0));
			int w = int(p_d.get("w", 32));
			int h = int(p_d.get("h", 32));
			region = Rect2i(x, y, w, h);
		}
		duration = float(p_d.get("duration", 0.0f));
		is_random_duration = bool(p_d.get("is_random_duration", false));
		duration_min = float(p_d.get("duration_min", 0.1f));
		duration_max = float(p_d.get("duration_max", 0.5f));
	}
};

// ============================================================================
// Datos de Animación de Sprite (SpriteFrames / AnimatedSprite2D style - Fase 8A)
// ============================================================================
struct ClockGearsSpriteAnimation {
	String name;
	String spritesheet_path;
	int frame_w = 32;
	int frame_h = 32;
	int cols = 4;
	int rows = 1;
	float fps = 6.0f;
	bool loop = true;
	bool random_order = false;
	bool behind_body = false;
	Vector<ClockGearsAnimationFrame> frames_data;
	Vector<int> frames;

	int get_frame_count() const {
		return !frames_data.is_empty() ? frames_data.size() : frames.size();
	}

	ClockGearsAnimationFrame get_frame(int p_idx, int p_tex_w = 0, int p_tex_h = 0) const {
		if (p_idx >= 0 && p_idx < frames_data.size()) {
			ClockGearsAnimationFrame f = frames_data[p_idx];
			if (f.sheet_path.is_empty()) {
				f.sheet_path = spritesheet_path;
			}
			int eff_cols = cols > 0 ? cols : 4;
			int eff_rows = rows > 0 ? rows : 1;
			bool needs_dynamic_region = (f.region.size.x <= 0 || f.region.size.y <= 0);
			if (!needs_dynamic_region && p_tex_w > 0 && p_tex_h > 0 && f.region.size == Vector2i(32, 32)) {
				if (p_tex_w != eff_cols * 32 || p_tex_h != eff_rows * 32) {
					needs_dynamic_region = true;
				}
			}
			if (needs_dynamic_region && p_tex_w > 0 && p_tex_h > 0) {
				int fn = (p_idx < frames.size()) ? frames[p_idx] : p_idx;
				int fw = MAX(1, p_tex_w / eff_cols);
				int fh = MAX(1, p_tex_h / eff_rows);
				int c = fn % eff_cols;
				int r = fn / eff_cols;
				f.region = Rect2i(c * fw, r * fh, fw, fh);
			}
			return f;
		}
		ClockGearsAnimationFrame f;
		f.sheet_path = spritesheet_path;
		int fn = (p_idx >= 0 && p_idx < frames.size()) ? frames[p_idx] : 0;
		f.region = get_frame_rect(fn, p_tex_w, p_tex_h);
		f.duration = 1.0f / MAX(0.1f, fps);
		return f;
	}

	Rect2i get_frame_rect(int p_frame_index, int p_tex_w, int p_tex_h) const {
		if (p_frame_index >= 0 && p_frame_index < frames_data.size()) {
			const Rect2i &r = frames_data[p_frame_index].region;
			if (r.size.x > 0 && r.size.y > 0) {
				return r;
			}
		}
		int effective_cols = cols > 0 ? cols : (frame_w > 0 && p_tex_w > 0 ? MAX(1, p_tex_w / frame_w) : 1);
		int effective_fw = frame_w > 0 ? frame_w : (cols > 0 && p_tex_w > 0 ? MAX(1, p_tex_w / cols) : p_tex_w);
		int effective_fh = frame_h > 0 ? frame_h : (rows > 0 && p_tex_h > 0 ? MAX(1, p_tex_h / rows) : p_tex_h);

		int c = p_frame_index % effective_cols;
		int r = p_frame_index / effective_cols;
		int x = c * effective_fw;
		int y = r * effective_fh;
		return Rect2i(x, y, effective_fw, effective_fh);
	}

	Dictionary to_dict() const {
		Dictionary d;
		d["name"] = name;
		d["spritesheet_path"] = spritesheet_path;
		d["frame_w"] = frame_w;
		d["frame_h"] = frame_h;
		d["cols"] = cols;
		d["rows"] = rows;
		d["fps"] = fps;
		d["loop"] = loop;
		d["random_order"] = random_order;
		d["behind_body"] = behind_body;

		Array arr;
		for (int i = 0; i < frames_data.size(); i++) {
			arr.push_back(frames_data[i].to_dict());
		}
		d["frames_data"] = arr;

		Array legacy_arr;
		for (int i = 0; i < frames.size(); i++) {
			legacy_arr.push_back(frames[i]);
		}
		d["frames"] = legacy_arr;
		return d;
	}

	void from_dict(const Dictionary &p_d) {
		name = p_d.get("name", name);
		String sp_in = String(p_d.get("spritesheet_path", "")).strip_edges();
		if (!sp_in.is_empty()) {
			spritesheet_path = sp_in;
		}
		frame_w = int(p_d.get("frame_w", frame_w > 0 ? frame_w : 32));
		frame_h = int(p_d.get("frame_h", frame_h > 0 ? frame_h : 32));
		cols = int(p_d.get("cols", cols > 0 ? cols : 4));
		rows = int(p_d.get("rows", rows > 0 ? rows : 1));
		fps = float(p_d.get("fps", fps > 0 ? fps : 6.0f));
		loop = bool(p_d.get("loop", loop));
		random_order = bool(p_d.get("random_order", false));
		behind_body = bool(p_d.get("behind_body", false));

		frames_data.clear();
		frames.clear();

		if (p_d.has("frames_data")) {
			Array arr = p_d.get("frames_data", Array());
			for (int i = 0; i < arr.size(); i++) {
				if (arr[i].get_type() == Variant::DICTIONARY) {
					ClockGearsAnimationFrame f;
					f.from_dict(Dictionary(arr[i]));
					if (f.sheet_path.is_empty()) {
						f.sheet_path = spritesheet_path;
					}
					frames_data.push_back(f);
				}
			}
		}

		Array legacy = p_d.get("frames", Array());
		for (int i = 0; i < legacy.size(); i++) {
			if (legacy[i].get_type() == Variant::DICTIONARY) {
				ClockGearsAnimationFrame f;
				f.from_dict(Dictionary(legacy[i]));
				if (f.sheet_path.is_empty()) {
					f.sheet_path = spritesheet_path;
				}
				if (frames_data.size() <= i) {
					frames_data.push_back(f);
				}
				frames.push_back(i);
			} else {
				int f_idx = int(legacy[i]);
				frames.push_back(f_idx);
				if (frames_data.size() <= i) {
					ClockGearsAnimationFrame f;
					f.sheet_path = spritesheet_path;
					int eff_cols = cols > 0 ? cols : 4;
					int eff_fw = frame_w > 0 ? frame_w : 32;
					int eff_fh = frame_h > 0 ? frame_h : 32;
					int c = f_idx % eff_cols;
					int r = f_idx / eff_cols;
					f.region = Rect2i(c * eff_fw, r * eff_fh, eff_fw, eff_fh);
					frames_data.push_back(f);
				}
			}
		}
		while (frames.size() < frames_data.size()) {
			frames.push_back(frames.size());
		}
	}
};

struct ClockGearsSpriteSheetData {
	int frame_w = 32;
	int frame_h = 32;
	int cols = 4;
	int rows = 1;
	int margin = 0;
	int spacing = 0;
	HashMap<String, ClockGearsSpriteAnimation> animations;

	Rect2i get_frame_rect(int p_frame_index, int p_tex_w, int p_tex_h, const String &p_anim = "") const {
		if (!p_anim.is_empty() && animations.has(p_anim)) {
			const ClockGearsSpriteAnimation &a = animations[p_anim];
			return a.get_frame_rect(p_frame_index, p_tex_w, p_tex_h);
		}
		if (animations.has("idle")) {
			const ClockGearsSpriteAnimation &a = animations["idle"];
			return a.get_frame_rect(p_frame_index, p_tex_w, p_tex_h);
		}
		int effective_cols = cols > 0 ? cols : (frame_w > 0 && p_tex_w > 0 ? MAX(1, p_tex_w / frame_w) : 1);
		int effective_fw = frame_w > 0 ? frame_w : (cols > 0 && p_tex_w > 0 ? MAX(1, p_tex_w / cols) : p_tex_w);
		int effective_fh = frame_h > 0 ? frame_h : (rows > 0 && p_tex_h > 0 ? MAX(1, p_tex_h / rows) : p_tex_h);

		int c = p_frame_index % effective_cols;
		int r = p_frame_index / effective_cols;
		int x = margin + c * (effective_fw + spacing);
		int y = margin + r * (effective_fh + spacing);
		return Rect2i(x, y, effective_fw, effective_fh);
	}

	Dictionary to_dict() const {
		Dictionary d;
		d["frame_w"] = frame_w;
		d["frame_h"] = frame_h;
		d["cols"] = cols;
		d["rows"] = rows;
		d["margin"] = margin;
		d["spacing"] = spacing;
		Dictionary anims;
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : animations) {
			anims[E.key] = E.value.to_dict();
		}
		d["animations"] = anims;
		return d;
	}

	void from_dict(const Dictionary &p_d) {
		frame_w = int(p_d.get("frame_w", frame_w));
		frame_h = int(p_d.get("frame_h", frame_h));
		cols = int(p_d.get("cols", cols));
		rows = int(p_d.get("rows", rows));
		margin = int(p_d.get("margin", margin));
		spacing = int(p_d.get("spacing", spacing));
		String default_sheet = String(p_d.get("spritesheet_path", p_d.get("graphic_path", ""))).strip_edges();
		animations.clear();
		Dictionary anims = p_d.get("animations", Dictionary());
		Array keys = anims.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			Dictionary ad = anims[k];
			ClockGearsSpriteAnimation a;
			a.name = k;
			a.spritesheet_path = default_sheet;
			a.frame_w = frame_w;
			a.frame_h = frame_h;
			a.cols = cols;
			a.rows = rows;
			a.from_dict(ad);
			animations[k] = a;
		}
	}

	static String map_enemy_anim_to_fixed_slot(const String &p_anim_name) {
		String s = p_anim_name.strip_edges().to_lower();
		if (s == "idle" || s == "inactivo" || s == "reposo") {
			return "idle";
		}
		if (s == "attack" || s == "atacar" || s == "ataque") {
			return "attack";
		}
		if (s == "hurt" || s == String::utf8("daño") || s == "dano" || s == "damage" || s == "herido") {
			return "hurt";
		}
		if (s == "death" || s == "muerte" || s == "morir" || s == "dead") {
			return "death";
		}
		if (s == "buff" || s == "apoyo") {
			return "buff";
		}
		return "";
	}

	static String get_enemy_fixed_slot_label(const String &p_slot_id) {
		String s = p_slot_id.strip_edges().to_lower();
		if (s == "idle") return "Idle";
		if (s == "attack") return "Ataque";
		if (s == "hurt") return String::utf8("Daño");
		if (s == "death") return "Muerte";
		if (s == "buff") return "Buff";
		return p_slot_id;
	}

	static int get_anim_dict_frame_count(const Dictionary &p_ad) {
		int c1 = 0;
		int c2 = 0;
		if (p_ad.has("frames_data") && p_ad["frames_data"].get_type() == Variant::ARRAY) {
			c1 = Array(p_ad["frames_data"]).size();
		}
		if (p_ad.has("frames") && p_ad["frames"].get_type() == Variant::ARRAY) {
			c2 = Array(p_ad["frames"]).size();
		}
		return MAX(c1, c2);
	}

	static void migrate_enemy_record(Dictionary &r_enemy_dict) {
		String main_graphic = String(r_enemy_dict.get("graphic_path", "")).strip_edges();
		Dictionary sp = r_enemy_dict.get("sprite_data", Dictionary());
		int sp_fw = int(sp.get("frame_w", r_enemy_dict.get("frame_w", 32)));
		int sp_fh = int(sp.get("frame_h", r_enemy_dict.get("frame_h", 32)));
		int sp_cols = int(sp.get("cols", r_enemy_dict.get("battler_frames", 4)));
		int sp_rows = int(sp.get("rows", 1));
		if (sp_fw <= 0) sp_fw = 32;
		if (sp_fh <= 0) sp_fh = 32;
		if (sp_cols <= 0) sp_cols = 4;
		if (sp_rows <= 0) sp_rows = 1;
		sp["frame_w"] = sp_fw;
		sp["frame_h"] = sp_fh;
		sp["cols"] = sp_cols;
		sp["rows"] = sp_rows;

		Array casts = r_enemy_dict.get("cast_animations", Array());
		Vector<String> cast_names;
		for (int i = 0; i < casts.size(); i++) {
			if (casts[i].get_type() == Variant::DICTIONARY) {
				String cn = String(Dictionary(casts[i]).get("name", "")).strip_edges();
				if (!cn.is_empty()) {
					cast_names.push_back(cn);
				}
			}
		}

		Array others = r_enemy_dict.get("other_animations", Array());
		Dictionary old_anims = sp.get("animations", Dictionary());
		Dictionary new_anims;

		auto normalize_anim = [&](const Dictionary &p_ad, const String &p_target_name, bool p_def_loop) -> Dictionary {
			ClockGearsSpriteAnimation a;
			a.name = p_target_name;
			a.frame_w = sp_fw;
			a.frame_h = sp_fh;
			a.cols = sp_cols;
			a.rows = sp_rows;
			a.loop = p_def_loop;
			String anim_sheet = String(p_ad.get("spritesheet_path", "")).strip_edges();
			if (anim_sheet.is_empty() && !main_graphic.is_empty() && get_anim_dict_frame_count(p_ad) > 0) {
				anim_sheet = main_graphic;
			}
			a.spritesheet_path = anim_sheet;
			a.from_dict(p_ad);
			a.name = p_target_name;
			if (a.spritesheet_path.is_empty() && !anim_sheet.is_empty()) {
				a.spritesheet_path = anim_sheet;
			}
			for (int fi = 0; fi < a.frames_data.size(); fi++) {
				if (a.frames_data[fi].sheet_path.is_empty() && !a.spritesheet_path.is_empty()) {
					a.frames_data.write[fi].sheet_path = a.spritesheet_path;
				}
			}
			return a.to_dict();
		};

		Array keys = old_anims.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = String(keys[i]);
			if (old_anims[k].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary ad = old_anims[k];
			String fixed_slot = map_enemy_anim_to_fixed_slot(k);
			if (!fixed_slot.is_empty()) {
				bool def_loop = (fixed_slot == "idle");
				Dictionary norm = normalize_anim(ad, fixed_slot, def_loop);
				if (!new_anims.has(fixed_slot)) {
					new_anims[fixed_slot] = norm;
				} else {
					Dictionary existing = new_anims[fixed_slot];
					int old_cnt = get_anim_dict_frame_count(existing);
					int new_cnt = get_anim_dict_frame_count(norm);
					if (new_cnt > old_cnt || (new_cnt == old_cnt && String(existing.get("spritesheet_path", "")).is_empty() && !String(norm.get("spritesheet_path", "")).is_empty())) {
						new_anims[fixed_slot] = norm;
					}
				}
			} else if (cast_names.has(k)) {
				new_anims[k] = normalize_anim(ad, k, false);
			} else {
				Dictionary norm = normalize_anim(ad, k, bool(ad.get("loop", true)));
				new_anims[k] = norm;
				bool found_in_others = false;
				for (int oi = 0; oi < others.size(); oi++) {
					String oi_name = (others[oi].get_type() == Variant::DICTIONARY)
							? String(Dictionary(others[oi]).get("name", "")).strip_edges()
							: String(others[oi]).strip_edges();
					if (oi_name == k) {
						others[oi] = norm;
						found_in_others = true;
						break;
					}
				}
				if (!found_in_others) {
					others.push_back(norm);
				}
			}
		}

		// Ensure any entry in other_animations also exists in new_anims so it can be edited/previewed before reassigning
		for (int oi = 0; oi < others.size(); oi++) {
			if (others[oi].get_type() == Variant::DICTIONARY) {
				Dictionary od = others[oi];
				String ok = String(od.get("name", "")).strip_edges();
				if (!ok.is_empty() && !new_anims.has(ok)) {
					new_anims[ok] = normalize_anim(od, ok, bool(od.get("loop", true)));
				}
			}
		}

		const char *fixed_ids[] = { "idle", "attack", "hurt", "death", "buff" };
		for (int i = 0; i < 5; i++) {
			String fid = String::utf8(fixed_ids[i]);
			if (!new_anims.has(fid)) {
				ClockGearsSpriteAnimation empty_a;
				empty_a.name = fid;
				empty_a.frame_w = sp_fw;
				empty_a.frame_h = sp_fh;
				empty_a.cols = sp_cols;
				empty_a.rows = sp_rows;
				empty_a.fps = 6.0f;
				empty_a.loop = (fid == "idle");
				new_anims[fid] = empty_a.to_dict();
			}
		}

		for (int i = 0; i < cast_names.size(); i++) {
			String cn = cast_names[i];
			if (!new_anims.has(cn)) {
				ClockGearsSpriteAnimation empty_c;
				empty_c.name = cn;
				empty_c.frame_w = sp_fw;
				empty_c.frame_h = sp_fh;
				empty_c.cols = sp_cols;
				empty_c.rows = sp_rows;
				empty_c.fps = 6.0f;
				empty_c.loop = false;
				new_anims[cn] = empty_c.to_dict();
			}
		}

		sp["animations"] = new_anims;
		r_enemy_dict["sprite_data"] = sp;
		r_enemy_dict["other_animations"] = others;
	}
};

// ============================================================================
// Secuencias de animación: Encadenar A -> B con repeticiones, pausas y bucle
// ============================================================================
struct ClockGearsAnimSequenceStep {
	String anim_name;
	int repeats = 1;
	float delay_after = 0.0f; // Pausa intermedia en segundos
};

struct ClockGearsAnimSequence {
	String name;
	Vector<ClockGearsAnimSequenceStep> steps;
	bool loop = false;

	Array to_array() const {
		Array arr;
		for (int i = 0; i < steps.size(); i++) {
			Dictionary step;
			step["anim_name"] = steps[i].anim_name;
			step["repeats"] = steps[i].repeats;
			step["delay_after"] = steps[i].delay_after;
			arr.push_back(step);
		}
		return arr;
	}

	void from_array(const String &p_name, const Array &p_steps, bool p_loop = false) {
		name = p_name;
		loop = p_loop;
		steps.clear();
		for (int i = 0; i < p_steps.size(); i++) {
			Dictionary d = p_steps[i];
			ClockGearsAnimSequenceStep st;
			st.anim_name = d.get("anim_name", "");
			st.repeats = MAX(1, int(d.get("repeats", 1)));
			st.delay_after = float(d.get("delay_after", 0.0f));
			steps.push_back(st);
		}
	}
};

// ============================================================================
// Variantes aleatorias: Elegir entre varias con pesos
// ============================================================================
struct ClockGearsWeightedChoice {
	String anim_name;
	float weight = 1.0f;
};

struct ClockGearsRandomVariant {
	String name;
	Vector<ClockGearsWeightedChoice> choices;

	void from_array(const String &p_name, const Array &p_choices) {
		name = p_name;
		choices.clear();
		for (int i = 0; i < p_choices.size(); i++) {
			Dictionary d = p_choices[i];
			ClockGearsWeightedChoice c;
			c.anim_name = d.get("anim_name", "");
			c.weight = MAX(0.0001f, float(d.get("weight", 1.0f)));
			choices.push_back(c);
		}
	}

	String pick_choice(RandomPCG &p_rng) const {
		if (choices.is_empty()) return "";
		float total_weight = 0.0f;
		for (int i = 0; i < choices.size(); i++) {
			total_weight += choices[i].weight;
		}
		if (total_weight <= 0.0f) return choices[0].anim_name;

		float r = p_rng.randf() * total_weight;
		float accum = 0.0f;
		for (int i = 0; i < choices.size(); i++) {
			accum += choices[i].weight;
			if (r <= accum) {
				return choices[i].anim_name;
			}
		}
		return choices[choices.size() - 1].anim_name;
	}
};

// ============================================================================
// Tabla de Triggers (Mapea trigger -> animación/secuencia/variante)
// ============================================================================
class ClockGearsAnimationTriggerTable : public RefCounted {
	GDCLASS(ClockGearsAnimationTriggerTable, RefCounted);

private:
	HashMap<int64_t, String> trigger_bindings; // key = (trigger << 32) | sub_id

	static int64_t _make_key(int p_trigger, int p_sub_id) {
		return (int64_t(p_trigger) << 32) | (int64_t(p_sub_id) & 0xFFFFFFFF);
	}

protected:
	static void _bind_methods();

public:
	static int string_to_trigger(const String &p_name);
	static String trigger_to_string(int p_trigger);

	void bind_trigger(int p_trigger, const String &p_anim_name, int p_sub_id = 0);
	void bind_trigger_named(const String &p_trigger_name, const String &p_anim_name, int p_sub_id = 0);

	String get_animation(int p_trigger, int p_sub_id = 0) const;
	String get_animation_named(const String &p_trigger_name, int p_sub_id = 0) const;

	bool has_trigger(int p_trigger, int p_sub_id = 0) const;
	bool has_trigger_named(const String &p_trigger_name, int p_sub_id = 0) const;

	void unbind_trigger(int p_trigger, int p_sub_id = 0);
	void clear();

	Dictionary serialize() const;
	void deserialize(const Dictionary &p_data);

	ClockGearsAnimationTriggerTable();
};

// ============================================================================
// Gestor Central de Animaciones (ClockGearsAnimationManager)
// ============================================================================
class ClockGearsAnimationManager : public Object {
	GDCLASS(ClockGearsAnimationManager, Object);

public:
	struct ActivePlayback {
		ObjectID target_id;
		String anim_name;
		bool is_sequence = false;
		int seq_step_idx = 0;
		int seq_repeat_count = 0;
		float timer = 0.0f;
		float pause_timer = 0.0f;
		int frame_index = 0;
		bool finished = false;
	};

private:
	static ClockGearsAnimationManager *singleton;

	mutable RandomPCG rng;
	HashMap<String, ClockGearsAnimDef> registered_animations;
	HashMap<String, ClockGearsAnimSequence> registered_sequences;
	HashMap<String, ClockGearsRandomVariant> registered_variants;
	Ref<ClockGearsAnimationTriggerTable> default_trigger_table;

	Vector<ActivePlayback> active_playbacks;

	void _apply_visual_feedback(Node2D *p_target, int p_trigger, const String &p_anim_name);

protected:
	static void _bind_methods();

public:
	static ClockGearsAnimationManager *get_singleton() { return singleton; }

	void set_rng_seed(uint64_t p_seed);
	uint64_t get_rng_seed() const;

	// Registro de animaciones
	void register_animation(const String &p_name, const Dictionary &p_data);
	bool has_animation(const String &p_name) const;
	Dictionary get_animation(const String &p_name) const;

	// Registro de secuencias
	void register_sequence(const String &p_name, const Array &p_steps, bool p_loop = false);
	bool has_sequence(const String &p_name) const;

	// Registro de variantes aleatorias
	void register_random_variant(const String &p_name, const Array &p_choices);
	bool has_random_variant(const String &p_name) const;

	// Resolución de nombre (resuelve variantes aleatorias a nombre concreto)
	String resolve_animation_name(const String &p_name_or_variant);

	// Tabla de triggers global por defecto
	Ref<ClockGearsAnimationTriggerTable> get_default_trigger_table();
	void set_default_trigger_table(const Ref<ClockGearsAnimationTriggerTable> &p_table);

	// Disparo de triggers (Regla Cero: si no está asignado, se ignora sin error)
	bool trigger_animation(Node2D *p_target, int p_trigger, int p_sub_id = 0, const Ref<ClockGearsAnimationTriggerTable> &p_custom_table = Ref<ClockGearsAnimationTriggerTable>());
	bool trigger_animation_named(Node2D *p_target, const String &p_trigger_name, int p_sub_id = 0, const Ref<ClockGearsAnimationTriggerTable> &p_custom_table = Ref<ClockGearsAnimationTriggerTable>());

	// Reproducción directa
	bool play_animation(Node2D *p_target, const String &p_anim_name);

	// Actualización paso a paso (step)
	void step(float p_delta);
	bool is_playing(Node2D *p_target) const;
	void stop(Node2D *p_target);
	void clear_all();

	ClockGearsAnimationManager();
	~ClockGearsAnimationManager();
};

// ============================================================================
// Acciones y Estados Registrables de Personaje (Tanda 1A - Secciones 4, 5, 6)
// ============================================================================
struct ClockGearsCharacterStateDef {
	String id;
	String slot_prefix;
	String fallback_state_id;
	bool fallback_freeze_frame0 = false;
	bool blocks_movement = false;
	bool default_loop = true;

	Dictionary to_dict() const {
		Dictionary d;
		d["id"] = id;
		d["slot_prefix"] = slot_prefix;
		d["fallback_state_id"] = fallback_state_id;
		d["fallback_freeze_frame0"] = fallback_freeze_frame0;
		d["blocks_movement"] = blocks_movement;
		d["default_loop"] = default_loop;
		return d;
	}
};

struct ClockGearsResolvedEquipLayer {
	bool visible = false;
	bool is_completo = true;
	String texture_path;
	Rect2i region;
	int offset_x = 0;
	int offset_y = 0;
	bool behind_body = false;
	int layer_order = 0;
};

class ClockGearsCharacterStateRegistry : public Object {
	GDCLASS(ClockGearsCharacterStateRegistry, Object);

private:
	static ClockGearsCharacterStateRegistry *singleton;
	HashMap<String, ClockGearsCharacterStateDef> states;
	Vector<String> state_order;

protected:
	static void _bind_methods();

public:
	static ClockGearsCharacterStateRegistry *get_singleton() { return singleton; }

	// Utilidades de dirección compartidas (4/8 direcciones y orden de filas 4/8)
	static bool is_valid_direction(int p_dir);
	static bool is_diagonal_direction(int p_dir);
	static String direction_to_suffix(int p_dir);
	static int direction_to_nearest_horizontal(int p_dir);
	static int direction_to_sheet_row(int p_dir, int p_rows);
	static int vector_to_direction(const Vector2i &p_vec);
	static PackedStringArray get_actor_slot_ids(int p_movement_directions, bool p_directional_idle);
	static PackedStringArray get_category_slot_ids(int p_category, int p_movement_directions, bool p_directional_idle, int p_combat_mode);
	static PackedStringArray get_category_slot_ids_by_key(const String &p_category_key, int p_movement_directions, bool p_directional_idle, int p_combat_mode);
	static bool is_actor_fixed_directional_slot(const String &p_slot_id);
	static Ref<Texture2D> get_missing_question_marker_texture();

	// Resolución compartida de capas de equipo ARPG (DB Preview + F5 Jugador)
	static int get_default_arpg_layer_order(int p_category);
	static int get_arpg_category_for_slot(const String &p_slot_id);
	static String get_arpg_slot_label(const String &p_slot_id);
	static String get_arpg_simple_group_for_state(const String &p_state_id);
	static Vector2 compute_layer_sprite_position(bool p_is_completo, int p_tile_size, int p_fw, int p_fh, int p_offset_x, int p_offset_y);
	static int compute_layer_z_index(bool p_behind_body, int p_layer_order, int p_slot_index);

	bool resolve_arpg_piece_layer(
			const Dictionary &p_piece,
			int p_category,
			const String &p_state_id,
			int p_direction,
			int p_body_frame_idx,
			bool p_directional_idle,
			int p_movement_directions,
			ClockGearsResolvedEquipLayer &r_out) const;

	// Registro extensible de estados de acción (sin modificar estados ya registrados)
	void register_state(const String &p_id, const String &p_slot_prefix, bool p_blocks_movement = false, const String &p_fallback_state_id = "", bool p_fallback_freeze_frame0 = false, bool p_default_loop = true);
	bool has_state(const String &p_id) const;
	Dictionary get_state_info(const String &p_id) const;
	bool does_state_block_movement(const String &p_id) const;
	PackedStringArray get_registered_state_ids() const;
	String resolve_slot_for_state(const String &p_state_id, int p_direction) const;

	bool resolve_actor_animation(
			const ClockGearsSpriteSheetData &p_sheet,
			bool p_directional_idle,
			const String &p_state_id,
			int p_direction,
			ClockGearsSpriteAnimation &r_anim,
			bool &r_freeze_frame0,
			String &r_resolved_slot) const;

	ClockGearsCharacterStateRegistry();
	~ClockGearsCharacterStateRegistry();
};
