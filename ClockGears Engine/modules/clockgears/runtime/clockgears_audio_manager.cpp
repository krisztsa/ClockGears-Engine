#include "clockgears_audio_manager.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/object/callable_mp.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/audio/audio_stream_wav.h"

ClockGearsAudioManager *ClockGearsAudioManager::singleton = nullptr;

void ClockGearsAudioManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("ensure_in_tree"), &ClockGearsAudioManager::ensure_in_tree);
	ClassDB::bind_method(D_METHOD("update_audio", "delta"), &ClockGearsAudioManager::update_audio);
	ClassDB::bind_method(D_METHOD("play_bgm", "path", "volume", "pitch", "crossfade_time", "pos"), &ClockGearsAudioManager::play_bgm, DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(0.5f), DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("stop_bgm", "fade_time"), &ClockGearsAudioManager::stop_bgm, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("pause_bgm"), &ClockGearsAudioManager::pause_bgm);
	ClassDB::bind_method(D_METHOD("resume_bgm_playback"), &ClockGearsAudioManager::resume_bgm_playback);
	ClassDB::bind_method(D_METHOD("is_bgm_playing"), &ClockGearsAudioManager::is_bgm_playing);
	ClassDB::bind_method(D_METHOD("get_current_bgm_path"), &ClockGearsAudioManager::get_current_bgm_path);
	ClassDB::bind_method(D_METHOD("get_bgm_playback_position"), &ClockGearsAudioManager::get_bgm_playback_position);
	ClassDB::bind_method(D_METHOD("set_bgm_loop", "start_sec", "end_sec"), &ClockGearsAudioManager::set_bgm_loop);

	ClassDB::bind_method(D_METHOD("play_stem", "idx", "path", "volume"), &ClockGearsAudioManager::play_stem, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("set_stem_volume", "idx", "volume", "fade_time"), &ClockGearsAudioManager::set_stem_volume, DEFVAL(0.3f));
	ClassDB::bind_method(D_METHOD("get_stem_volume", "idx"), &ClockGearsAudioManager::get_stem_volume);
	ClassDB::bind_method(D_METHOD("stop_all_stems", "fade_time"), &ClockGearsAudioManager::stop_all_stems, DEFVAL(0.3f));

	ClassDB::bind_method(D_METHOD("play_bgs", "path", "volume", "pitch", "fade_in"), &ClockGearsAudioManager::play_bgs, DEFVAL(1.0f), DEFVAL(1.0f), DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("stop_bgs", "fade_out"), &ClockGearsAudioManager::stop_bgs, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("is_bgs_playing"), &ClockGearsAudioManager::is_bgs_playing);
	ClassDB::bind_method(D_METHOD("get_current_bgs_path"), &ClockGearsAudioManager::get_current_bgs_path);

	ClassDB::bind_method(D_METHOD("play_me", "path", "volume", "pitch"), &ClockGearsAudioManager::play_me, DEFVAL(1.0f), DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("stop_me"), &ClockGearsAudioManager::stop_me);
	ClassDB::bind_method(D_METHOD("is_me_playing"), &ClockGearsAudioManager::is_me_playing);

	ClassDB::bind_method(D_METHOD("play_se", "path", "volume", "pitch"), &ClockGearsAudioManager::play_se, DEFVAL(1.0f), DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("stop_all_se"), &ClockGearsAudioManager::stop_all_se);

	ClassDB::bind_method(D_METHOD("play_voice", "path", "volume", "pitch"), &ClockGearsAudioManager::play_voice, DEFVAL(1.0f), DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("play_text_blip", "pitch_variation"), &ClockGearsAudioManager::play_text_blip, DEFVAL(0.05f));

	ClassDB::bind_method(D_METHOD("set_ducking_enabled", "enable"), &ClockGearsAudioManager::set_ducking_enabled);
	ClassDB::bind_method(D_METHOD("is_ducking_enabled"), &ClockGearsAudioManager::is_ducking_enabled);
	ClassDB::bind_method(D_METHOD("set_duck_factor", "factor"), &ClockGearsAudioManager::set_duck_factor);
	ClassDB::bind_method(D_METHOD("get_duck_factor"), &ClockGearsAudioManager::get_duck_factor);

	ClassDB::bind_method(D_METHOD("save_bgm"), &ClockGearsAudioManager::save_bgm);
	ClassDB::bind_method(D_METHOD("restore_bgm", "crossfade_time"), &ClockGearsAudioManager::restore_bgm, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("has_saved_bgm"), &ClockGearsAudioManager::has_saved_bgm);

	ClassDB::bind_method(D_METHOD("set_bus_volume", "bus", "volume"), &ClockGearsAudioManager::set_bus_volume);
	ClassDB::bind_method(D_METHOD("get_bus_volume", "bus"), &ClockGearsAudioManager::get_bus_volume);

	BIND_CONSTANT(BUS_MASTER);
	BIND_CONSTANT(BUS_BGM);
	BIND_CONSTANT(BUS_BGS);
	BIND_CONSTANT(BUS_ME);
	BIND_CONSTANT(BUS_SE);
	BIND_CONSTANT(BUS_VOICE);

	ADD_SIGNAL(MethodInfo("bgm_changed", PropertyInfo(Variant::STRING, "path")));
	ADD_SIGNAL(MethodInfo("me_finished"));
}

ClockGearsAudioManager::ClockGearsAudioManager() {
	if (!singleton) {
		singleton = this;
	}
	set_process(true);

	// Crear reproductores para Crossfade BGM
	bgm_a = memnew(AudioStreamPlayer);
	bgm_a->set_name("BGM_A");
	add_child(bgm_a);

	bgm_b = memnew(AudioStreamPlayer);
	bgm_b->set_name("BGM_B");
	add_child(bgm_b);

	// Crear reproductores para Stems
	for (int i = 0; i < MAX_STEMS; i++) {
		stem_players[i] = memnew(AudioStreamPlayer);
		stem_players[i]->set_name(vformat("Stem_%d", i));
		add_child(stem_players[i]);
		stem_volumes[i] = 0.0f;
		stem_target_volumes[i] = 0.0f;
	}

	// BGS
	bgs_player = memnew(AudioStreamPlayer);
	bgs_player->set_name("BGS");
	add_child(bgs_player);

	// ME
	me_player = memnew(AudioStreamPlayer);
	me_player->set_name("ME");
	me_player->connect("finished", callable_mp(this, &ClockGearsAudioManager::_on_me_finished));
	add_child(me_player);

	// Pool de SE
	for (int i = 0; i < MAX_SE_PLAYERS; i++) {
		se_players[i] = memnew(AudioStreamPlayer);
		se_players[i]->set_name(vformat("SE_%d", i));
		add_child(se_players[i]);
	}

	// Voz / Blips
	voice_player = memnew(AudioStreamPlayer);
	voice_player->set_name("Voice");
	add_child(voice_player);
}

ClockGearsAudioManager::~ClockGearsAudioManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void ClockGearsAudioManager::ensure_in_tree() {
	if (is_inside_tree() || get_parent() != nullptr) {
		return;
	}
	if (SceneTree::get_singleton() && SceneTree::get_singleton()->get_root()) {
		SceneTree::get_singleton()->get_root()->add_child(this);
	}
}

Ref<AudioStream> ClockGearsAudioManager::_load_audio_stream(const String &p_path) {
	if (p_path.is_empty()) {
		return Ref<AudioStream>();
	}
	if (ResourceLoader::exists(p_path)) {
		return ResourceLoader::load(p_path);
	}
	// Fallback de audio silencioso para testing / prototipos sin assets
	Ref<AudioStreamWAV> dummy;
	dummy.instantiate();
	dummy->set_format(AudioStreamWAV::FORMAT_8_BITS);
	dummy->set_mix_rate(8000);
	Vector<uint8_t> dummy_bytes;
	dummy_bytes.resize(8000);
	dummy_bytes.fill(0);
	dummy->set_data(dummy_bytes);
	dummy->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
	return dummy;
}

void ClockGearsAudioManager::play_bgm(const String &p_path, float p_volume, float p_pitch, float p_crossfade_time, float p_pos) {
	ensure_in_tree();
	if (p_path.is_empty()) {
		stop_bgm(p_crossfade_time);
		return;
	}

	// Regla A1.4: si es la misma pista ya sonando, no reiniciar
	if (current_bgm_path == p_path && is_bgm_playing()) {
		bgm_target_volume = p_volume;
		return;
	}

	current_bgm_path = p_path;
	bgm_target_volume = p_volume;
	bgm_pitch = p_pitch;
	bgm_playing = true;
	bgm_paused = false;

	AudioStreamPlayer *next_player = using_bgm_a ? bgm_b : bgm_a;
	using_bgm_a = !using_bgm_a;

	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		next_player->set_stream(stream);
	}
	next_player->set_pitch_scale(p_pitch);

	if (p_crossfade_time > 0.01f) {
		crossfade_duration = p_crossfade_time;
		crossfade_timer = 0.0f;
		next_player->set_volume_db(-80.0f); // Iniciar silencioso para crossfade
	} else {
		crossfade_duration = 0.0f;
		float eff_vol = bgm_target_volume * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER];
		next_player->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	}

	if (next_player->is_inside_tree()) {
		next_player->play(p_pos);
	}
	emit_signal("bgm_changed", p_path);
}

void ClockGearsAudioManager::stop_bgm(float p_fade_time) {
	current_bgm_path = "";
	bgm_target_volume = 0.0f;
	bgm_playing = false;
	bgm_paused = false;
	if (p_fade_time <= 0.01f) {
		if (bgm_a) bgm_a->stop();
		if (bgm_b) bgm_b->stop();
		for (int i = 0; i < MAX_STEMS; i++) {
			if (stem_players[i]) stem_players[i]->stop();
		}
	} else {
		crossfade_duration = p_fade_time;
		crossfade_timer = 0.0f;
	}
}

void ClockGearsAudioManager::pause_bgm() {
	bgm_paused = true;
	if (bgm_a && bgm_a->is_playing()) bgm_a->set_stream_paused(true);
	if (bgm_b && bgm_b->is_playing()) bgm_b->set_stream_paused(true);
	for (int i = 0; i < MAX_STEMS; i++) {
		if (stem_players[i] && stem_players[i]->is_playing()) {
			stem_players[i]->set_stream_paused(true);
		}
	}
}

void ClockGearsAudioManager::resume_bgm_playback() {
	bgm_paused = false;
	if (bgm_a && bgm_a->get_stream_paused()) bgm_a->set_stream_paused(false);
	if (bgm_b && bgm_b->get_stream_paused()) bgm_b->set_stream_paused(false);
	for (int i = 0; i < MAX_STEMS; i++) {
		if (stem_players[i] && stem_players[i]->get_stream_paused()) {
			stem_players[i]->set_stream_paused(false);
		}
	}
}

bool ClockGearsAudioManager::is_bgm_playing() const {
	if (!bgm_playing || bgm_paused) {
		return false;
	}
	if (bgm_a && bgm_a->is_inside_tree()) {
		bool a_plays = bgm_a->is_playing() && !bgm_a->get_stream_paused();
		bool b_plays = bgm_b && bgm_b->is_playing() && !bgm_b->get_stream_paused();
		return a_plays || b_plays;
	}
	return true;
}

float ClockGearsAudioManager::get_bgm_playback_position() const {
	AudioStreamPlayer *active = using_bgm_a ? bgm_a : bgm_b;
	return active ? active->get_playback_position() : 0.0f;
}

void ClockGearsAudioManager::set_bgm_loop(float p_start_sec, float p_end_sec) {
	bgm_loop_start = MAX(0.0f, p_start_sec);
	bgm_loop_end = MAX(0.0f, p_end_sec);
}

void ClockGearsAudioManager::play_stem(int p_idx, const String &p_path, float p_volume) {
	ensure_in_tree();
	if (p_idx < 0 || p_idx >= MAX_STEMS) return;
	stem_paths[p_idx] = p_path;
	stem_volumes[p_idx] = p_volume;
	stem_target_volumes[p_idx] = p_volume;

	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		stem_players[p_idx]->set_stream(stream);
	}
	float cur_pos = get_bgm_playback_position();
	if (stem_players[p_idx]->is_inside_tree()) {
		stem_players[p_idx]->play(cur_pos); // Sincronizado exactamente con la BGM
	}
}

void ClockGearsAudioManager::set_stem_volume(int p_idx, float p_volume, float p_fade_time) {
	if (p_idx < 0 || p_idx >= MAX_STEMS) return;
	stem_target_volumes[p_idx] = CLAMP(p_volume, 0.0f, 1.0f);
	if (p_fade_time <= 0.01f) {
		stem_volumes[p_idx] = stem_target_volumes[p_idx];
		float eff_vol = stem_volumes[p_idx] * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER];
		stem_players[p_idx]->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	}
}

float ClockGearsAudioManager::get_stem_volume(int p_idx) const {
	if (p_idx < 0 || p_idx >= MAX_STEMS) return 0.0f;
	return stem_volumes[p_idx];
}

void ClockGearsAudioManager::stop_all_stems(float p_fade_time) {
	for (int i = 0; i < MAX_STEMS; i++) {
		set_stem_volume(i, 0.0f, p_fade_time);
	}
}

void ClockGearsAudioManager::play_bgs(const String &p_path, float p_volume, float p_pitch, float p_fade_in) {
	ensure_in_tree();
	current_bgs_path = p_path;
	bgs_volume = p_volume;
	bgs_playing = true;
	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		bgs_player->set_stream(stream);
	}
	bgs_player->set_pitch_scale(p_pitch);
	float eff_vol = bgs_volume * bus_volumes[BUS_BGS] * bus_volumes[BUS_MASTER];
	bgs_player->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	if (bgs_player->is_inside_tree()) {
		bgs_player->play();
	}
}

void ClockGearsAudioManager::stop_bgs(float p_fade_out) {
	current_bgs_path = "";
	bgs_playing = false;
	if (bgs_player) {
		bgs_player->stop();
	}
}

bool ClockGearsAudioManager::is_bgs_playing() const {
	if (!bgs_playing) return false;
	if (bgs_player && bgs_player->is_inside_tree()) {
		return bgs_player->is_playing();
	}
	return true;
}

void ClockGearsAudioManager::play_me(const String &p_path, float p_volume, float p_pitch) {
	ensure_in_tree();
	if (is_bgm_playing()) {
		bgm_was_playing_before_me = true;
		bgm_pre_me_pos = get_bgm_playback_position();
		pause_bgm();
	} else {
		bgm_was_playing_before_me = false;
	}

	me_active = true;
	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		me_player->set_stream(stream);
	}
	me_player->set_pitch_scale(p_pitch);
	float eff_vol = p_volume * bus_volumes[BUS_ME] * bus_volumes[BUS_MASTER];
	me_player->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	if (me_player->is_inside_tree()) {
		me_player->play();
	}
}

void ClockGearsAudioManager::stop_me() {
	if (me_player) {
		me_player->stop();
	}
	_on_me_finished();
}

void ClockGearsAudioManager::_on_me_finished() {
	me_active = false;
	emit_signal("me_finished");

	// Regla A1.4: al terminar el ME, la BGM retorna sola
	if (bgm_was_playing_before_me) {
		bgm_was_playing_before_me = false;
		resume_bgm_playback();
	}
}

void ClockGearsAudioManager::play_se(const String &p_path, float p_volume, float p_pitch) {
	ensure_in_tree();
	if (p_path.is_empty()) return;
	AudioStreamPlayer *pl = se_players[next_se_idx];
	next_se_idx = (next_se_idx + 1) % MAX_SE_PLAYERS;

	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		pl->set_stream(stream);
	}
	pl->set_pitch_scale(p_pitch);
	float eff_vol = p_volume * bus_volumes[BUS_SE] * bus_volumes[BUS_MASTER];
	pl->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	if (pl->is_inside_tree()) {
		pl->play();
	}
}

void ClockGearsAudioManager::stop_all_se() {
	for (int i = 0; i < MAX_SE_PLAYERS; i++) {
		if (se_players[i]) se_players[i]->stop();
	}
}

void ClockGearsAudioManager::play_voice(const String &p_path, float p_volume, float p_pitch) {
	ensure_in_tree();
	if (!voice_player) return;
	Ref<AudioStream> stream = _load_audio_stream(p_path);
	if (stream.is_valid()) {
		voice_player->set_stream(stream);
	}
	voice_player->set_pitch_scale(p_pitch);
	float eff_vol = p_volume * bus_volumes[BUS_VOICE] * bus_volumes[BUS_MASTER];
	voice_player->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
	if (voice_player->is_inside_tree()) {
		voice_player->play();
	}
}

void ClockGearsAudioManager::play_text_blip(float p_pitch_variation) {
	if (!voice_player) return;
	float pitch = 1.0f + ((float)Math::randf() * 2.0f - 1.0f) * p_pitch_variation;
	voice_player->set_pitch_scale(pitch);
	// Si no hay archivo cargado en voz, usar SE predeterminado o simplemente pitch
	if (voice_player->is_playing()) {
		voice_player->stop();
	}
	if (voice_player->is_inside_tree()) {
		voice_player->play();
	}
}

void ClockGearsAudioManager::save_bgm() {
	saved_bgm.path = current_bgm_path;
	saved_bgm.volume = bgm_target_volume;
	saved_bgm.pitch = bgm_pitch;
	saved_bgm.position = get_bgm_playback_position();
	saved_bgm.valid = !saved_bgm.path.is_empty();
}

void ClockGearsAudioManager::restore_bgm(float p_crossfade_time) {
	if (!saved_bgm.valid) return;
	play_bgm(saved_bgm.path, saved_bgm.volume, saved_bgm.pitch, p_crossfade_time, saved_bgm.position);
	saved_bgm.valid = false;
}

void ClockGearsAudioManager::set_bus_volume(int p_bus, float p_volume) {
	if (p_bus >= 0 && p_bus < BUS_COUNT) {
		bus_volumes[p_bus] = CLAMP(p_volume, 0.0f, 1.0f);
	}
}

float ClockGearsAudioManager::get_bus_volume(int p_bus) const {
	if (p_bus >= 0 && p_bus < BUS_COUNT) {
		return bus_volumes[p_bus];
	}
	return 1.0f;
}

void ClockGearsAudioManager::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		update_audio(get_process_delta_time());
	}
}

void ClockGearsAudioManager::update_audio(float p_delta) {
	// 1. Ducking suave si hay voz activa
	bool should_duck = (voice_player && voice_player->is_playing()) || me_active;
	float target_duck = (should_duck && ducking_active) ? duck_factor : 1.0f;
	duck_current_lerp = Math::lerp(duck_current_lerp, target_duck, CLAMP(p_delta * 8.0f, 0.0f, 1.0f));

	// 2. Crossfade BGM
	AudioStreamPlayer *active = using_bgm_a ? bgm_a : bgm_b;
	AudioStreamPlayer *fading = using_bgm_a ? bgm_b : bgm_a;

	if (crossfade_duration > 0.001f) {
		crossfade_timer += p_delta;
		float t = CLAMP(crossfade_timer / crossfade_duration, 0.0f, 1.0f);

		float in_vol = t * bgm_target_volume * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER] * duck_current_lerp;
		if (active && active->is_playing()) {
			active->set_volume_db(Math::linear_to_db(MAX(0.0001f, in_vol)));
		}

		float out_vol = (1.0f - t) * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER];
		if (fading && fading->is_playing()) {
			fading->set_volume_db(Math::linear_to_db(MAX(0.0001f, out_vol)));
		}

		if (t >= 1.0f) {
			crossfade_duration = 0.0f;
			if (fading) fading->stop();
		}
	} else {
		float eff_vol = bgm_target_volume * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER] * duck_current_lerp;
		if (active && active->is_playing()) {
			active->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
		}
	}

	// 3. Loop points de BGM
	if (bgm_loop_end > bgm_loop_start && active && active->is_playing()) {
		if (active->get_playback_position() >= bgm_loop_end) {
			active->seek(bgm_loop_start);
		}
	}

	// 4. Stems volumen fade
	for (int i = 0; i < MAX_STEMS; i++) {
		if (Math::abs(stem_volumes[i] - stem_target_volumes[i]) > 0.001f) {
			stem_volumes[i] = Math::lerp(stem_volumes[i], stem_target_volumes[i], CLAMP(p_delta * 4.0f, 0.0f, 1.0f));
			float eff_vol = stem_volumes[i] * bus_volumes[BUS_BGM] * bus_volumes[BUS_MASTER] * duck_current_lerp;
			stem_players[i]->set_volume_db(Math::linear_to_db(MAX(0.0001f, eff_vol)));
		}
	}
}
