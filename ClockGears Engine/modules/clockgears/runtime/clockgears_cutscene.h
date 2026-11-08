#ifndef CLOCKGEARS_CUTSCENE_H
#define CLOCKGEARS_CUTSCENE_H

#include "core/io/resource.h"
#include "core/object/class_db.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"

// ============================================================================
// ClockGearsCutscene — Recurso de Cinemática / Timeline (Anexo A1.2)
// Pistas: Cámara, Personajes, Imágenes, Texto, Audio, Pantalla, Espera.
// ============================================================================
class ClockGearsCutscene : public Resource {
	GDCLASS(ClockGearsCutscene, Resource);

public:
	enum TrackType {
		TRACK_CAMERA = 0,
		TRACK_CHARACTER = 1,
		TRACK_PICTURE = 2,
		TRACK_TEXT = 3,
		TRACK_AUDIO = 4,
		TRACK_SCREEN = 5,
		TRACK_WAIT = 6,
		TRACK_SWITCH_VAR = 7
	};

	struct Keyframe {
		float time = 0.0f;
		TrackType track = TRACK_WAIT;
		int target_id = 0;
		Dictionary params;
		int easing = 0;
	};

private:
	String cutscene_name;
	float total_duration = 5.0f;
	bool letterbox = true;
	bool block_player = true;
	bool restore_camera = true;
	bool allow_skip = true;
	Vector<Keyframe> keyframes;

protected:
	static void _bind_methods();

public:
	void set_cutscene_name(const String &p_name) { cutscene_name = p_name; }
	String get_cutscene_name() const { return cutscene_name; }

	void set_total_duration(float p_dur) { total_duration = MAX(0.1f, p_dur); }
	float get_total_duration() const { return total_duration; }

	void set_letterbox_enabled(bool p_enable) { letterbox = p_enable; }
	bool is_letterbox_enabled() const { return letterbox; }

	void set_block_player(bool p_block) { block_player = p_block; }
	bool is_block_player() const { return block_player; }

	void set_restore_camera(bool p_restore) { restore_camera = p_restore; }
	bool is_restore_camera() const { return restore_camera; }

	void set_allow_skip(bool p_allow) { allow_skip = p_allow; }
	bool is_allow_skip() const { return allow_skip; }

	void add_keyframe(float p_time, int p_track, int p_target, const Dictionary &p_params, int p_easing = 0);
	int get_keyframe_count() const { return keyframes.size(); }
	Dictionary get_keyframe(int p_idx) const;
	void clear_keyframes() { keyframes.clear(); }
	const Vector<Keyframe> &get_keyframes_raw() const { return keyframes; }

	Dictionary to_dict() const;
	void from_dict(const Dictionary &p_dict);

	ClockGearsCutscene();
	~ClockGearsCutscene();
};

// ============================================================================
// ClockGearsCutscenePlayer — Reproductor de Cinemáticas en Runtime (Anexo A1.2)
// Barras de cine (Letterbox), salto de cinemática con estado final consistente,
// bloqueo y restauración de cámara y controles.
// ============================================================================
class ClockGearsCutscenePlayer : public Control {
	GDCLASS(ClockGearsCutscenePlayer, Control);

private:
	static ClockGearsCutscenePlayer *singleton;

	Ref<ClockGearsCutscene> active_cutscene;
	bool is_playing_cutscene = false;
	bool is_paused_cutscene = false;
	float playback_time = 0.0f;
	int next_keyframe_idx = 0;

	// Barras de cine (Letterbox)
	ColorRect *letterbox_top = nullptr;
	ColorRect *letterbox_bottom = nullptr;
	float letterbox_height = 0.0f;
	float target_letterbox_height = 0.0f;

	void _apply_keyframe(const ClockGearsCutscene::Keyframe &kf, bool is_skipping = false);
	void _update_letterbox(float p_delta);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event);

public:
	static ClockGearsCutscenePlayer *get_singleton() { return singleton; }

	void play(const Ref<ClockGearsCutscene> &p_cutscene);
	void pause();
	void resume();
	void stop();
	void skip(); // Salto con aplicación instantánea de estado final (A1.2 & A6)

	bool is_playing() const { return is_playing_cutscene; }
	bool is_paused() const { return is_paused_cutscene; }
	float get_playback_time() const { return playback_time; }
	Ref<ClockGearsCutscene> get_active_cutscene() const { return active_cutscene; }
	float get_letterbox_height() const { return letterbox_height; }
	bool is_letterbox_active() const { return letterbox_height > 0.1f; }

	void update_player(float p_delta);

	ClockGearsCutscenePlayer();
	~ClockGearsCutscenePlayer();
};

#endif // CLOCKGEARS_CUTSCENE_H
