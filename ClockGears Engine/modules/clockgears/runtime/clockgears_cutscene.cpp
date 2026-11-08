#include "clockgears_cutscene.h"
#include "clockgears_audio_manager.h"
#include "clockgears_picture_manager.h"
#include "clockgears_message_window.h"
#include "../core/clockgears_data_manager.h"
#include "../core/clockgears_time_manager.h"
#include "core/input/input.h"
#include "scene/main/viewport.h"

// ============================================================================
// ClockGearsCutscene (Recurso)
// ============================================================================
void ClockGearsCutscene::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cutscene_name", "name"), &ClockGearsCutscene::set_cutscene_name);
	ClassDB::bind_method(D_METHOD("get_cutscene_name"), &ClockGearsCutscene::get_cutscene_name);
	ClassDB::bind_method(D_METHOD("set_total_duration", "duration"), &ClockGearsCutscene::set_total_duration);
	ClassDB::bind_method(D_METHOD("get_total_duration"), &ClockGearsCutscene::get_total_duration);
	ClassDB::bind_method(D_METHOD("set_letterbox_enabled", "enable"), &ClockGearsCutscene::set_letterbox_enabled);
	ClassDB::bind_method(D_METHOD("is_letterbox_enabled"), &ClockGearsCutscene::is_letterbox_enabled);
	ClassDB::bind_method(D_METHOD("set_block_player", "block"), &ClockGearsCutscene::set_block_player);
	ClassDB::bind_method(D_METHOD("is_block_player"), &ClockGearsCutscene::is_block_player);
	ClassDB::bind_method(D_METHOD("set_restore_camera", "restore"), &ClockGearsCutscene::set_restore_camera);
	ClassDB::bind_method(D_METHOD("is_restore_camera"), &ClockGearsCutscene::is_restore_camera);
	ClassDB::bind_method(D_METHOD("set_allow_skip", "allow"), &ClockGearsCutscene::set_allow_skip);
	ClassDB::bind_method(D_METHOD("is_allow_skip"), &ClockGearsCutscene::is_allow_skip);

	ClassDB::bind_method(D_METHOD("add_keyframe", "time", "track", "target", "params", "easing"), &ClockGearsCutscene::add_keyframe, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_keyframe_count"), &ClockGearsCutscene::get_keyframe_count);
	ClassDB::bind_method(D_METHOD("get_keyframe", "idx"), &ClockGearsCutscene::get_keyframe);
	ClassDB::bind_method(D_METHOD("clear_keyframes"), &ClockGearsCutscene::clear_keyframes);

	ClassDB::bind_method(D_METHOD("to_dict"), &ClockGearsCutscene::to_dict);
	ClassDB::bind_method(D_METHOD("from_dict", "dict"), &ClockGearsCutscene::from_dict);

	BIND_CONSTANT(TRACK_CAMERA);
	BIND_CONSTANT(TRACK_CHARACTER);
	BIND_CONSTANT(TRACK_PICTURE);
	BIND_CONSTANT(TRACK_TEXT);
	BIND_CONSTANT(TRACK_AUDIO);
	BIND_CONSTANT(TRACK_SCREEN);
	BIND_CONSTANT(TRACK_WAIT);
	BIND_CONSTANT(TRACK_SWITCH_VAR);
}

ClockGearsCutscene::ClockGearsCutscene() {
}

ClockGearsCutscene::~ClockGearsCutscene() {
}

void ClockGearsCutscene::add_keyframe(float p_time, int p_track, int p_target, const Dictionary &p_params, int p_easing) {
	Keyframe kf;
	kf.time = MAX(0.0f, p_time);
	kf.track = (TrackType)CLAMP(p_track, 0, 7);
	kf.target_id = p_target;
	kf.params = p_params.duplicate(true);
	kf.easing = p_easing;

	// Inserción ordenada por tiempo
	int insert_idx = keyframes.size();
	for (int i = 0; i < keyframes.size(); i++) {
		if (keyframes[i].time > kf.time) {
			insert_idx = i;
			break;
		}
	}
	keyframes.insert(insert_idx, kf);

	if (kf.time > total_duration) {
		total_duration = kf.time + 0.5f;
	}
}

Dictionary ClockGearsCutscene::get_keyframe(int p_idx) const {
	if (p_idx < 0 || p_idx >= keyframes.size()) return Dictionary();
	const Keyframe &kf = keyframes[p_idx];
	Dictionary d;
	d["time"] = kf.time;
	d["track"] = int(kf.track);
	d["target_id"] = kf.target_id;
	d["params"] = kf.params;
	d["easing"] = kf.easing;
	return d;
}

Dictionary ClockGearsCutscene::to_dict() const {
	Dictionary d;
	d["name"] = cutscene_name;
	d["total_duration"] = total_duration;
	d["letterbox"] = letterbox;
	d["block_player"] = block_player;
	d["restore_camera"] = restore_camera;
	d["allow_skip"] = allow_skip;

	Array kfs;
	for (int i = 0; i < keyframes.size(); i++) {
		kfs.push_back(get_keyframe(i));
	}
	d["keyframes"] = kfs;
	return d;
}

void ClockGearsCutscene::from_dict(const Dictionary &p_dict) {
	cutscene_name = p_dict.get("name", "");
	total_duration = float(p_dict.get("total_duration", 5.0f));
	letterbox = bool(p_dict.get("letterbox", true));
	block_player = bool(p_dict.get("block_player", true));
	restore_camera = bool(p_dict.get("restore_camera", true));
	allow_skip = bool(p_dict.get("allow_skip", true));

	keyframes.clear();
	Array kfs = p_dict.get("keyframes", Array());
	for (int i = 0; i < kfs.size(); i++) {
		Dictionary kd = kfs[i];
		add_keyframe(float(kd.get("time", 0.0f)), int(kd.get("track", 6)), int(kd.get("target_id", 0)), kd.get("params", Dictionary()), int(kd.get("easing", 0)));
	}
}

// ============================================================================
// ClockGearsCutscenePlayer (Reproductor de Cinemáticas en Runtime)
// ============================================================================
ClockGearsCutscenePlayer *ClockGearsCutscenePlayer::singleton = nullptr;

void ClockGearsCutscenePlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("play", "cutscene"), &ClockGearsCutscenePlayer::play);
	ClassDB::bind_method(D_METHOD("pause"), &ClockGearsCutscenePlayer::pause);
	ClassDB::bind_method(D_METHOD("resume"), &ClockGearsCutscenePlayer::resume);
	ClassDB::bind_method(D_METHOD("stop"), &ClockGearsCutscenePlayer::stop);
	ClassDB::bind_method(D_METHOD("skip"), &ClockGearsCutscenePlayer::skip);

	ClassDB::bind_method(D_METHOD("is_playing"), &ClockGearsCutscenePlayer::is_playing);
	ClassDB::bind_method(D_METHOD("is_paused"), &ClockGearsCutscenePlayer::is_paused);
	ClassDB::bind_method(D_METHOD("get_playback_time"), &ClockGearsCutscenePlayer::get_playback_time);
	ClassDB::bind_method(D_METHOD("get_active_cutscene"), &ClockGearsCutscenePlayer::get_active_cutscene);
	ClassDB::bind_method(D_METHOD("get_letterbox_height"), &ClockGearsCutscenePlayer::get_letterbox_height);
	ClassDB::bind_method(D_METHOD("is_letterbox_active"), &ClockGearsCutscenePlayer::is_letterbox_active);
	ClassDB::bind_method(D_METHOD("update_player", "delta"), &ClockGearsCutscenePlayer::update_player);

	ADD_SIGNAL(MethodInfo("cutscene_started", PropertyInfo(Variant::OBJECT, "cutscene")));
	ADD_SIGNAL(MethodInfo("cutscene_finished", PropertyInfo(Variant::OBJECT, "cutscene")));
	ADD_SIGNAL(MethodInfo("keyframe_reached", PropertyInfo(Variant::INT, "track"), PropertyInfo(Variant::FLOAT, "time")));
}

ClockGearsCutscenePlayer::ClockGearsCutscenePlayer() {
	if (!singleton) {
		singleton = this;
	}
	set_process(true);
	set_process_unhandled_input(true);
	set_anchors_preset(PRESET_FULL_RECT);
	set_mouse_filter(MOUSE_FILTER_IGNORE);

	// Barras de cine (Letterbox)
	letterbox_top = memnew(ColorRect);
	letterbox_top->set_color(Color(0, 0, 0, 1));
	letterbox_top->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(letterbox_top);

	letterbox_bottom = memnew(ColorRect);
	letterbox_bottom->set_color(Color(0, 0, 0, 1));
	letterbox_bottom->set_mouse_filter(MOUSE_FILTER_IGNORE);
	add_child(letterbox_bottom);

	letterbox_top->hide();
	letterbox_bottom->hide();
}

ClockGearsCutscenePlayer::~ClockGearsCutscenePlayer() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void ClockGearsCutscenePlayer::play(const Ref<ClockGearsCutscene> &p_cutscene) {
	if (p_cutscene.is_null()) return;

	active_cutscene = p_cutscene;
	is_playing_cutscene = true;
	is_paused_cutscene = false;
	playback_time = 0.0f;
	next_keyframe_idx = 0;

	// Pausar reloj de tiempo en cinemáticas según A3.1
	ClockGearsTimeManager *tm = ClockGearsTimeManager::get_singleton();
	if (tm) {
		tm->set_context_state(false, false, true, false);
	}

	if (active_cutscene->is_letterbox_enabled()) {
		target_letterbox_height = 54.0f; // Altura estándar letterbox
		letterbox_top->show();
		letterbox_bottom->show();
	} else {
		target_letterbox_height = 0.0f;
	}

	emit_signal("cutscene_started", active_cutscene);
}

void ClockGearsCutscenePlayer::pause() {
	is_paused_cutscene = true;
}

void ClockGearsCutscenePlayer::resume() {
	is_paused_cutscene = false;
}

void ClockGearsCutscenePlayer::stop() {
	if (!is_playing_cutscene) return;

	is_playing_cutscene = false;
	is_paused_cutscene = false;
	target_letterbox_height = 0.0f;

	ClockGearsTimeManager *tm = ClockGearsTimeManager::get_singleton();
	if (tm) {
		tm->set_context_state(false, false, false, false);
	}

	Ref<ClockGearsCutscene> finished = active_cutscene;
	active_cutscene = Ref<ClockGearsCutscene>();
	emit_signal("cutscene_finished", finished);
}

void ClockGearsCutscenePlayer::skip() {
	if (!is_playing_cutscene || active_cutscene.is_null() || !active_cutscene->is_allow_skip()) {
		return;
	}

	// Regla A1.2 & A6: aplicar el estado final de todos los keyframes restantes de golpe
	const Vector<ClockGearsCutscene::Keyframe> &kfs = active_cutscene->get_keyframes_raw();
	for (int i = next_keyframe_idx; i < kfs.size(); i++) {
		_apply_keyframe(kfs[i], true);
	}
	playback_time = active_cutscene->get_total_duration();
	stop();
}

void ClockGearsCutscenePlayer::_apply_keyframe(const ClockGearsCutscene::Keyframe &kf, bool is_skipping) {
	emit_signal("keyframe_reached", int(kf.track), kf.time);

	switch (kf.track) {
		case ClockGearsCutscene::TRACK_AUDIO: {
			ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
			if (am) {
				String type = kf.params.get("type", "bgm");
				String file = kf.params.get("file", "");
				float vol = float(kf.params.get("vol", 1.0f));
				float pitch = float(kf.params.get("pitch", 1.0f));
				if (type == "bgm") am->play_bgm(file, vol, pitch);
				else if (type == "se" && !is_skipping) am->play_se(file, vol, pitch);
				else if (type == "bgs") am->play_bgs(file, vol, pitch);
				else if (type == "me" && !is_skipping) am->play_me(file, vol, pitch);
			}
		} break;

		case ClockGearsCutscene::TRACK_PICTURE: {
			ClockGearsPictureManager *pm = ClockGearsPictureManager::get_singleton();
			if (pm) {
				String act = kf.params.get("action", "show");
				int pic_id = kf.target_id;
				if (act == "show") {
					pm->show_picture(pic_id, kf.params.get("path", ""), int(kf.params.get("origin", 0)),
							kf.params.get("pos", Vector2()), kf.params.get("scale", Vector2(100, 100)),
							float(kf.params.get("opacity", 1.0f)));
				} else if (act == "erase") {
					pm->erase_picture(pic_id);
				} else if (act == "move") {
					if (is_skipping) {
						pm->show_picture(pic_id, pm->has_picture(pic_id) ? "" : "", int(kf.params.get("origin", 0)),
								kf.params.get("pos", Vector2()), kf.params.get("scale", Vector2(100, 100)),
								float(kf.params.get("opacity", 1.0f)));
					} else {
						pm->move_picture(pic_id, int(kf.params.get("origin", 0)), kf.params.get("pos", Vector2()),
								kf.params.get("scale", Vector2(100, 100)), float(kf.params.get("opacity", 1.0f)),
								float(kf.params.get("dur", 1.0f)), kf.easing);
					}
				}
			}
		} break;

		case ClockGearsCutscene::TRACK_SCREEN: {
			ClockGearsPictureManager *pm = ClockGearsPictureManager::get_singleton();
			if (pm && !is_skipping) {
				String eff = kf.params.get("effect", "flash");
				if (eff == "shake") {
					pm->start_screen_shake(float(kf.params.get("power", 5.0f)), float(kf.params.get("speed", 20.0f)), float(kf.params.get("dur", 0.5f)));
				} else if (eff == "flash") {
					pm->start_screen_flash(kf.params.get("color", Color(1, 1, 1, 0.8f)), float(kf.params.get("dur", 0.3f)));
				}
			}
		} break;

		case ClockGearsCutscene::TRACK_SWITCH_VAR: {
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			if (dm) {
				String type = kf.params.get("type", "switch");
				String name = kf.params.get("name", "");
				if (type == "switch") {
					dm->set_switch(name, bool(kf.params.get("val", true)));
				} else if (type == "variable") {
					dm->set_variable(name, int(kf.params.get("val", 0)));
				}
			}
		} break;

		case ClockGearsCutscene::TRACK_TEXT: {
			ClockGearsMessageWindow *mw = ClockGearsMessageWindow::get_singleton();
			if (mw && !is_skipping) {
				String text = kf.params.get("text", "");
				String speaker = kf.params.get("speaker", "");
				String face = kf.params.get("face", "");
				int pos = int(kf.params.get("pos", 0));
				mw->show_message(text, speaker, face, pos);
			}
		} break;

		case ClockGearsCutscene::TRACK_CHARACTER: {
			// Soporte de posición/dirección de personaje
			if (kf.params.has("cell")) {
				// Señalizado para que el mapa o intérprete lo aplique
			}
		} break;

		default:
			break;
	}
}

void ClockGearsCutscenePlayer::unhandled_input(const Ref<InputEvent> &p_event) {
	if (!is_playing_cutscene) return;

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::ESCAPE || k->get_keycode() == Key::TAB) {
			skip();
			get_viewport()->set_input_as_handled();
		}
	}
}

void ClockGearsCutscenePlayer::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		update_player(get_process_delta_time());
	}
}

void ClockGearsCutscenePlayer::_update_letterbox(float p_delta) {
	letterbox_height = Math::lerp(letterbox_height, target_letterbox_height, CLAMP(p_delta * 6.0f, 0.0f, 1.0f));

	Size2 vp_sz = get_viewport() ? get_viewport()->get_visible_rect().size : Size2(960, 540);
	if (letterbox_top && letterbox_bottom) {
		letterbox_top->set_position(Vector2(0, 0));
		letterbox_top->set_size(Size2(vp_sz.x, letterbox_height));

		letterbox_bottom->set_position(Vector2(0, vp_sz.y - letterbox_height));
		letterbox_bottom->set_size(Size2(vp_sz.x, letterbox_height));

		if (letterbox_height < 0.5f && target_letterbox_height < 0.5f) {
			letterbox_top->hide();
			letterbox_bottom->hide();
		}
	}
}

void ClockGearsCutscenePlayer::update_player(float p_delta) {
	_update_letterbox(p_delta);

	if (!is_playing_cutscene || is_paused_cutscene || active_cutscene.is_null()) {
		return;
	}

	playback_time += p_delta;

	const Vector<ClockGearsCutscene::Keyframe> &kfs = active_cutscene->get_keyframes_raw();
	while (next_keyframe_idx < kfs.size() && playback_time >= kfs[next_keyframe_idx].time) {
		_apply_keyframe(kfs[next_keyframe_idx++], false);
	}

	if (playback_time >= active_cutscene->get_total_duration()) {
		stop();
	}
}
