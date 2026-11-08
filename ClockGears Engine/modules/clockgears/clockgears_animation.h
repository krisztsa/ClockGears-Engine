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
	Vector<ClockGearsAnimationFrame> frames_data;
	Vector<int> frames;

	int get_frame_count() const {
		return !frames_data.is_empty() ? frames_data.size() : frames.size();
	}

	ClockGearsAnimationFrame get_frame(int p_idx, int p_tex_w = 0, int p_tex_h = 0) const {
		if (p_idx >= 0 && p_idx < frames_data.size()) {
			ClockGearsAnimationFrame f = frames_data[p_idx];
			if (f.sheet_path.is_empty()) f.sheet_path = spritesheet_path;
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
		spritesheet_path = p_d.get("spritesheet_path", spritesheet_path);
		frame_w = int(p_d.get("frame_w", frame_w > 0 ? frame_w : 32));
		frame_h = int(p_d.get("frame_h", frame_h > 0 ? frame_h : 32));
		cols = int(p_d.get("cols", cols > 0 ? cols : 4));
		rows = int(p_d.get("rows", rows > 0 ? rows : 1));
		fps = float(p_d.get("fps", fps > 0 ? fps : 6.0f));
		loop = bool(p_d.get("loop", loop));
		random_order = bool(p_d.get("random_order", false));

		frames_data.clear();
		frames.clear();

		if (p_d.has("frames_data")) {
			Array arr = p_d.get("frames_data", Array());
			for (int i = 0; i < arr.size(); i++) {
				if (arr[i].get_type() == Variant::DICTIONARY) {
					ClockGearsAnimationFrame f;
					f.from_dict(Dictionary(arr[i]));
					frames_data.push_back(f);
				}
			}
		}

		Array legacy = p_d.get("frames", Array());
		for (int i = 0; i < legacy.size(); i++) {
			if (legacy[i].get_type() == Variant::DICTIONARY) {
				ClockGearsAnimationFrame f;
				f.from_dict(Dictionary(legacy[i]));
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
		animations.clear();
		Dictionary anims = p_d.get("animations", Dictionary());
		Array keys = anims.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			Dictionary ad = anims[k];
			ClockGearsSpriteAnimation a;
			a.name = k;
			a.from_dict(ad);
			animations[k] = a;
		}
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
