#ifndef CLOCKGEARS_AUDIO_MANAGER_H
#define CLOCKGEARS_AUDIO_MANAGER_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/node.h"

// ============================================================================
// ClockGearsAudioManager — Gestor Integral de Música y Audio (Anexo A1.4)
// BGM con crossfade, bucles sin cortes, capas/stems sincronizadas, BGS, ME con
// retorno de BGM, ducking automático, guardar/restaurar música.
// ============================================================================
class ClockGearsAudioManager : public Node {
	GDCLASS(ClockGearsAudioManager, Node);

public:
	enum BusType {
		BUS_MASTER = 0,
		BUS_BGM,
		BUS_BGS,
		BUS_ME,
		BUS_SE,
		BUS_VOICE,
		BUS_COUNT
	};

	struct SavedBGMState {
		String path;
		float volume = 1.0f;
		float pitch = 1.0f;
		float position = 0.0f;
		bool valid = false;
	};

private:
	static ClockGearsAudioManager *singleton;

	// BGM con Crossfade (A y B)
	AudioStreamPlayer *bgm_a = nullptr;
	AudioStreamPlayer *bgm_b = nullptr;
	bool using_bgm_a = true;
	bool bgm_playing = false;
	bool bgm_paused = false;
	String current_bgm_path;
	float bgm_target_volume = 1.0f;
	float bgm_current_volume = 1.0f;
	float bgm_pitch = 1.0f;
	float crossfade_timer = 0.0f;
	float crossfade_duration = 0.0f;
	float bgm_loop_start = 0.0f;
	float bgm_loop_end = 0.0f;

	// Stems / Capas musicales sincronizadas (hasta 8)
	static constexpr int MAX_STEMS = 8;
	AudioStreamPlayer *stem_players[MAX_STEMS] = { nullptr };
	float stem_volumes[MAX_STEMS] = { 0.0f };
	float stem_target_volumes[MAX_STEMS] = { 0.0f };
	String stem_paths[MAX_STEMS];

	// BGS (Ambiente en bucle)
	AudioStreamPlayer *bgs_player = nullptr;
	bool bgs_playing = false;
	String current_bgs_path;
	float bgs_volume = 1.0f;

	// ME (Efecto musical / jingle que pausa o atenúa la BGM y luego retorna)
	AudioStreamPlayer *me_player = nullptr;
	bool me_active = false;
	bool bgm_was_playing_before_me = false;
	float bgm_pre_me_pos = 0.0f;

	// Pool de SE (Efectos de sonido simultáneos sin cortes)
	static constexpr int MAX_SE_PLAYERS = 16;
	AudioStreamPlayer *se_players[MAX_SE_PLAYERS] = { nullptr };
	int next_se_idx = 0;

	// Voz / Blips de texto
	AudioStreamPlayer *voice_player = nullptr;

	// Ducking automático
	bool ducking_active = false;
	float duck_factor = 0.4f; // Reducción de BGM al 40% durante voz/ME
	float duck_current_lerp = 1.0f;

	// Estado guardado para restaurar tras batallas/cinemáticas
	SavedBGMState saved_bgm;

	// Volúmenes maestros por bus
	float bus_volumes[BUS_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

	Ref<AudioStream> _load_audio_stream(const String &p_path);
	void _on_me_finished();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static ClockGearsAudioManager *get_singleton() { return singleton; }
	void ensure_in_tree();

	// --- BGM (Música de fondo) ---
	void play_bgm(const String &p_path, float p_volume = 1.0f, float p_pitch = 1.0f, float p_crossfade_time = 0.5f, float p_pos = 0.0f);
	void stop_bgm(float p_fade_time = 0.5f);
	void pause_bgm();
	void resume_bgm_playback();
	bool is_bgm_playing() const;
	String get_current_bgm_path() const { return current_bgm_path; }
	float get_bgm_playback_position() const;
	void set_bgm_loop(float p_start_sec, float p_end_sec);

	// --- Stems (Capas musicales sincronizadas) ---
	void play_stem(int p_idx, const String &p_path, float p_volume = 1.0f);
	void set_stem_volume(int p_idx, float p_volume, float p_fade_time = 0.3f);
	float get_stem_volume(int p_idx) const;
	void stop_all_stems(float p_fade_time = 0.3f);

	// --- BGS (Sonido ambiental) ---
	void play_bgs(const String &p_path, float p_volume = 1.0f, float p_pitch = 1.0f, float p_fade_in = 0.0f);
	void stop_bgs(float p_fade_out = 0.5f);
	bool is_bgs_playing() const;
	String get_current_bgs_path() const { return current_bgs_path; }

	// --- ME (Efecto musical con retorno de BGM) ---
	void play_me(const String &p_path, float p_volume = 1.0f, float p_pitch = 1.0f);
	void stop_me();
	bool is_me_playing() const { return me_active; }

	// --- SE (Efectos de sonido) ---
	void play_se(const String &p_path, float p_volume = 1.0f, float p_pitch = 1.0f);
	void stop_all_se();

	// --- Voz y Blips de texto ---
	void play_voice(const String &p_path, float p_volume = 1.0f, float p_pitch = 1.0f);
	void play_text_blip(float p_pitch_variation = 0.05f);

	// --- Ducking ---
	void set_ducking_enabled(bool p_enable) { ducking_active = p_enable; }
	bool is_ducking_enabled() const { return ducking_active; }
	void set_duck_factor(float p_factor) { duck_factor = CLAMP(p_factor, 0.0f, 1.0f); }
	float get_duck_factor() const { return duck_factor; }

	// --- Guardar y Restaurar BGM (Batallas y Cinemáticas) ---
	void save_bgm();
	void restore_bgm(float p_crossfade_time = 0.5f);
	bool has_saved_bgm() const { return saved_bgm.valid; }

	// --- Control de Volúmenes por Bus ---
	void set_bus_volume(int p_bus, float p_volume);
	float get_bus_volume(int p_bus) const;

	void update_audio(float p_delta);

	ClockGearsAudioManager();
	~ClockGearsAudioManager();
};

#endif // CLOCKGEARS_AUDIO_MANAGER_H
