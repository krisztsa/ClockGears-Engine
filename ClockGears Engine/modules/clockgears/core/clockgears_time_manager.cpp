#include "clockgears_time_manager.h"
#include "core/math/math_funcs.h"

ClockGearsTimeManager *ClockGearsTimeManager::singleton = nullptr;

void ClockGearsTimeManager::_init_default_periods() {
	day_periods.clear();
	// Periodos del día base (editables por el usuario, Sección A3.1)
	auto add_period = [this](const char *p_name, int p_start, int p_end) {
		Dictionary d;
		d["name"] = String::utf8(p_name);
		d["start_hour"] = p_start;
		d["end_hour"] = p_end;
		day_periods.push_back(d);
	};
	add_period("madrugada", 0, 5);
	add_period("amanecer", 5, 7);
	add_period("mañana", 7, 12);
	add_period("tarde", 12, 19);
	add_period("noche", 19, 24);
}

void ClockGearsTimeManager::_init_default_persistence_rules() {
	// A3.2: Por defecto TODO se reinicia salvo lo marcado explícitamente como persistente.
	persistent_rules.clear();
	persistent_rules["switches"] = Dictionary();
	persistent_rules["variables"] = Dictionary();
	persistent_rules["items"] = Dictionary();
	persistent_rules["equipment"] = Dictionary();
	persistent_rules["skills"] = Dictionary();
	persistent_rules["events_state"] = Dictionary();
	persistent_rules["npc_progress"] = Dictionary();

	Dictionary flags;
	flags["gold"] = false;
	flags["party_level_exp"] = false;
	flags["inventory_all"] = false;
	flags["equipment_all"] = false;
	flags["skills_all"] = false;
	persistent_rules["flags"] = flags;
}

void ClockGearsTimeManager::init_empty() {
	minutes_per_hour = 60;
	hours_per_day = 24;
	days_per_cycle = 3;
	cycle_start_hour = 6;
	cycle_mode_enabled = false;

	// Regla Cero: Nombres de días definidos por el usuario inician vacíos
	day_names.clear();
	_init_default_periods();
	_init_default_persistence_rules();

	total_game_minutes = int64_t(cycle_start_hour) * int64_t(minutes_per_hour);
	fractional_minutes_accum = 0.0;
	minutes_per_real_second = 1.0;
	current_cycle_count = 1;
	advance_mode = TIME_ADVANCE_REALTIME;
	paused = false;

	pause_in_dialogue = true;
	pause_in_menu = true;
	pause_in_cutscene = true;
	pause_in_battle = true;

	active_in_dialogue = false;
	active_in_menu = false;
	active_in_cutscene = false;
	active_in_battle = false;

	cycle_initial_snapshot.clear();
	cycle_reset_common_event_id = 0;
	cycle_spawn_map = "";
	cycle_spawn_x = 0;
	cycle_spawn_y = 0;
}

void ClockGearsTimeManager::set_minutes_per_hour(int p_minutes) {
	minutes_per_hour = MAX(1, p_minutes);
}

void ClockGearsTimeManager::set_hours_per_day(int p_hours) {
	hours_per_day = MAX(1, p_hours);
}

void ClockGearsTimeManager::set_days_per_cycle(int p_days) {
	days_per_cycle = MAX(1, p_days);
}

void ClockGearsTimeManager::set_cycle_start_hour(int p_hour) {
	cycle_start_hour = CLAMP(p_hour, 0, hours_per_day - 1);
}

void ClockGearsTimeManager::set_cycle_mode_enabled(bool p_enabled) {
	cycle_mode_enabled = p_enabled;
}

void ClockGearsTimeManager::set_day_names(const PackedStringArray &p_names) {
	day_names = p_names;
}

String ClockGearsTimeManager::get_day_name(int p_day_1based) const {
	int idx = p_day_1based - 1;
	if (idx >= 0 && idx < day_names.size() && !day_names[idx].is_empty()) {
		return day_names[idx];
	}
	return vformat(String::utf8("Día %d"), MAX(1, p_day_1based));
}

void ClockGearsTimeManager::set_day_periods(const Array &p_periods) {
	day_periods = p_periods.duplicate(true);
}

String ClockGearsTimeManager::get_current_period_name() const {
	int h = get_current_hour();
	for (int i = 0; i < day_periods.size(); i++) {
		if (day_periods[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary d = day_periods[i];
		int sh = int(d.get("start_hour", 0));
		int eh = int(d.get("end_hour", 24));
		if (sh <= eh) {
			if (h >= sh && h < eh) {
				return d.get("name", "");
			}
		} else {
			if (h >= sh || h < eh) {
				return d.get("name", "");
			}
		}
	}
	return "";
}

void ClockGearsTimeManager::set_advance_mode(int p_mode) {
	advance_mode = CLAMP(p_mode, TIME_ADVANCE_REALTIME, TIME_ADVANCE_MANUAL);
}

void ClockGearsTimeManager::set_time_scale(double p_minutes_per_real_sec) {
	minutes_per_real_second = MAX(0.0, p_minutes_per_real_sec);
}

void ClockGearsTimeManager::set_paused(bool p_paused) {
	paused = p_paused;
}

bool ClockGearsTimeManager::is_effectively_paused() const {
	if (paused) {
		return true;
	}
	if (pause_in_dialogue && active_in_dialogue) {
		return true;
	}
	if (pause_in_menu && active_in_menu) {
		return true;
	}
	if (pause_in_cutscene && active_in_cutscene) {
		return true;
	}
	if (pause_in_battle && active_in_battle) {
		return true;
	}
	return false;
}

void ClockGearsTimeManager::set_context_state(bool p_dialogue, bool p_menu, bool p_cutscene, bool p_battle) {
	active_in_dialogue = p_dialogue;
	active_in_menu = p_menu;
	active_in_cutscene = p_cutscene;
	active_in_battle = p_battle;
}

void ClockGearsTimeManager::tick(double p_delta_seconds) {
	if (p_delta_seconds <= 0.0 || advance_mode != TIME_ADVANCE_REALTIME || is_effectively_paused()) {
		return;
	}
	fractional_minutes_accum += p_delta_seconds * minutes_per_real_second;
	int64_t whole_minutes = int64_t(Math::floor(fractional_minutes_accum));
	if (whole_minutes > 0) {
		fractional_minutes_accum -= double(whole_minutes);
		_apply_minutes_delta(whole_minutes);
	}
}

void ClockGearsTimeManager::_apply_minutes_delta(int64_t p_delta_minutes) {
	if (p_delta_minutes <= 0) {
		return;
	}

	int prev_day = get_current_day();
	int prev_hour = get_current_hour();
	String prev_period = get_current_period_name();

	total_game_minutes += p_delta_minutes;

	int64_t mins_per_day = int64_t(minutes_per_hour) * int64_t(hours_per_day);
	int64_t max_cycle_minutes = mins_per_day * int64_t(days_per_cycle);

	if (cycle_mode_enabled && max_cycle_minutes > 0 && total_game_minutes >= max_cycle_minutes) {
		reset_cycle();
		return;
	}

	int new_day = get_current_day();
	int new_hour = get_current_hour();
	int new_min = get_current_minute();
	String new_period = get_current_period_name();

	emit_signal("minute_changed", total_game_minutes, new_day, new_hour, new_min);

	if (new_hour != prev_hour || new_day != prev_day) {
		emit_signal("hour_changed", new_day, new_hour);
	}
	if (new_period != prev_period) {
		emit_signal("period_changed", new_period);
	}
	if (new_day != prev_day) {
		emit_signal("day_changed", new_day, get_day_name(new_day));
	}
}

void ClockGearsTimeManager::set_total_minutes(int64_t p_total_minutes) {
	int prev_day = get_current_day();
	int prev_hour = get_current_hour();
	String prev_period = get_current_period_name();

	total_game_minutes = MAX(int64_t(0), p_total_minutes);
	fractional_minutes_accum = 0.0;

	int new_day = get_current_day();
	int new_hour = get_current_hour();
	int new_min = get_current_minute();
	String new_period = get_current_period_name();

	emit_signal("minute_changed", total_game_minutes, new_day, new_hour, new_min);
	if (new_hour != prev_hour || new_day != prev_day) {
		emit_signal("hour_changed", new_day, new_hour);
	}
	if (new_period != prev_period) {
		emit_signal("period_changed", new_period);
	}
	if (new_day != prev_day) {
		emit_signal("day_changed", new_day, get_day_name(new_day));
	}
}

void ClockGearsTimeManager::set_game_time(int p_day_1based, int p_hour, int p_minute) {
	int d = MAX(1, p_day_1based) - 1;
	int h = CLAMP(p_hour, 0, hours_per_day - 1);
	int m = CLAMP(p_minute, 0, minutes_per_hour - 1);
	int64_t mins_per_day = int64_t(minutes_per_hour) * int64_t(hours_per_day);
	set_total_minutes(int64_t(d) * mins_per_day + int64_t(h) * int64_t(minutes_per_hour) + int64_t(m));
}

void ClockGearsTimeManager::advance_minutes(int64_t p_minutes) {
	if (p_minutes > 0) {
		_apply_minutes_delta(p_minutes);
	}
}

void ClockGearsTimeManager::skip_to_hour(int p_target_hour, int p_target_minute) {
	int target_h = CLAMP(p_target_hour, 0, hours_per_day - 1);
	int target_m = CLAMP(p_target_minute, 0, minutes_per_hour - 1);
	int cur_h = get_current_hour();
	int cur_m = get_current_minute();

	int64_t cur_day_mins = int64_t(cur_h) * minutes_per_hour + cur_m;
	int64_t target_day_mins = int64_t(target_h) * minutes_per_hour + target_m;
	int64_t mins_per_day = int64_t(minutes_per_hour) * int64_t(hours_per_day);

	int64_t delta = target_day_mins - cur_day_mins;
	if (delta <= 0) {
		delta += mins_per_day;
	}
	_apply_minutes_delta(delta);
}

int ClockGearsTimeManager::get_current_minute() const {
	return int(total_game_minutes % int64_t(MAX(1, minutes_per_hour)));
}

int ClockGearsTimeManager::get_current_hour() const {
	int64_t total_hours = total_game_minutes / int64_t(MAX(1, minutes_per_hour));
	return int(total_hours % int64_t(MAX(1, hours_per_day)));
}

int ClockGearsTimeManager::get_current_day() const {
	int64_t mins_per_day = int64_t(MAX(1, minutes_per_hour)) * int64_t(MAX(1, hours_per_day));
	return int(total_game_minutes / mins_per_day) + 1;
}

String ClockGearsTimeManager::get_formatted_time(bool p_include_day) const {
	String hhmm = vformat("%02d:%02d", get_current_hour(), get_current_minute());
	if (!p_include_day) {
		return hhmm;
	}
	return vformat("%s - %s", get_day_name(get_current_day()), hhmm);
}

bool ClockGearsTimeManager::is_time_between(int p_start_hour, int p_start_min, int p_end_hour, int p_end_min) const {
	int cur_mins = get_current_hour() * minutes_per_hour + get_current_minute();
	int start_mins = p_start_hour * minutes_per_hour + p_start_min;
	int end_mins = p_end_hour * minutes_per_hour + p_end_min;

	if (start_mins <= end_mins) {
		return cur_mins >= start_mins && cur_mins < end_mins;
	}
	// Cruza la medianoche (ej. 22:00 -> 06:00)
	return cur_mins >= start_mins || cur_mins < end_mins;
}

bool ClockGearsTimeManager::is_on_day(int p_day_1based) const {
	return get_current_day() == p_day_1based;
}

bool ClockGearsTimeManager::is_in_period(const String &p_period_name) const {
	return get_current_period_name().to_lower() == p_period_name.strip_edges().to_lower();
}

void ClockGearsTimeManager::set_persistence_rule(const String &p_category, const String &p_key, bool p_persistent) {
	if (!persistent_rules.has(p_category) || persistent_rules[p_category].get_type() != Variant::DICTIONARY) {
		persistent_rules[p_category] = Dictionary();
	}
	Dictionary cat_dict = persistent_rules[p_category];
	if (p_persistent) {
		cat_dict[p_key] = true;
	} else {
		cat_dict.erase(p_key);
	}
	persistent_rules[p_category] = cat_dict;
}

bool ClockGearsTimeManager::is_persistent(const String &p_category, const String &p_key) const {
	if (!persistent_rules.has(p_category) || persistent_rules[p_category].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary cat_dict = persistent_rules[p_category];
	return bool(cat_dict.get(p_key, false));
}

void ClockGearsTimeManager::set_global_persistence_flag(const String &p_flag_name, bool p_persistent) {
	Dictionary flags = persistent_rules.get("flags", Dictionary());
	flags[p_flag_name] = p_persistent;
	persistent_rules["flags"] = flags;
}

bool ClockGearsTimeManager::get_global_persistence_flag(const String &p_flag_name) const {
	Dictionary flags = persistent_rules.get("flags", Dictionary());
	return bool(flags.get(p_flag_name, false));
}

void ClockGearsTimeManager::set_persistence_rules(const Dictionary &p_rules) {
	persistent_rules = p_rules.duplicate(true);
}

Dictionary ClockGearsTimeManager::get_persistence_summary() const {
	Dictionary summary;
	PackedStringArray conserved;
	PackedStringArray reset_list;

	const char *cats[] = { "switches", "variables", "items", "equipment", "skills", "events_state", "npc_progress" };
	for (int i = 0; i < 7; i++) {
		String c = cats[i];
		Dictionary d = persistent_rules.get(c, Dictionary());
		Array keys = d.keys();
		int count = 0;
		for (int k = 0; k < keys.size(); k++) {
			if (bool(d[keys[k]])) {
				count++;
			}
		}
		summary[c + "_persistent_count"] = count;
		if (count > 0) {
			conserved.push_back(vformat("%s (%d)", c, count));
		} else {
			reset_list.push_back(c);
		}
	}

	Dictionary flags = persistent_rules.get("flags", Dictionary());
	summary["gold_persistent"] = bool(flags.get("gold", false));
	summary["party_level_exp_persistent"] = bool(flags.get("party_level_exp", false));
	summary["conserved_categories"] = conserved;
	summary["reset_categories"] = reset_list;
	return summary;
}

void ClockGearsTimeManager::set_cycle_reset_transition(int p_common_event_id, const String &p_spawn_map, int p_spawn_x, int p_spawn_y) {
	cycle_reset_common_event_id = MAX(0, p_common_event_id);
	cycle_spawn_map = p_spawn_map;
	cycle_spawn_x = p_spawn_x;
	cycle_spawn_y = p_spawn_y;
}

void ClockGearsTimeManager::capture_cycle_initial_snapshot(const Dictionary &p_runtime_state) {
	cycle_initial_snapshot = p_runtime_state.duplicate(true);
}

Dictionary ClockGearsTimeManager::apply_cycle_reset_filter(const Dictionary &p_current_runtime_state) const {
	// A3.2: Partimos del estado inicial del ciclo (todo se reinicia por defecto)
	// y copiamos únicamente las entradas marcadas como persistentes desde p_current_runtime_state.
	Dictionary result = cycle_initial_snapshot.duplicate(true);
	Dictionary flags = persistent_rules.get("flags", Dictionary());

	auto merge_category = [&](const String &p_cat_key, bool p_keep_all) {
		if (!p_current_runtime_state.has(p_cat_key) || p_current_runtime_state[p_cat_key].get_type() != Variant::DICTIONARY) {
			return;
		}
		Dictionary cur_cat = p_current_runtime_state[p_cat_key];
		if (p_keep_all) {
			result[p_cat_key] = cur_cat.duplicate(true);
			return;
		}
		Dictionary base_cat = result.get(p_cat_key, Dictionary()).duplicate(true);
		Dictionary rules_cat = persistent_rules.get(p_cat_key, Dictionary());
		Array keys = cur_cat.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k_str = String(keys[i]);
			if (bool(rules_cat.get(k_str, false)) || bool(rules_cat.get(keys[i], false))) {
				base_cat[keys[i]] = cur_cat[keys[i]];
			}
		}
		result[p_cat_key] = base_cat;
	};

	merge_category("switches", false);
	merge_category("variables", false);
	merge_category("items", bool(flags.get("inventory_all", false)));
	merge_category("equipment", bool(flags.get("equipment_all", false)));
	merge_category("skills", bool(flags.get("skills_all", false)));
	merge_category("events_state", false);
	merge_category("npc_progress", false);

	if (bool(flags.get("gold", false)) && p_current_runtime_state.has("gold")) {
		result["gold"] = p_current_runtime_state["gold"];
	}
	if (bool(flags.get("party_level_exp", false)) && p_current_runtime_state.has("party_level_exp")) {
		result["party_level_exp"] = p_current_runtime_state["party_level_exp"];
	}

	result["spawn_map"] = cycle_spawn_map;
	result["spawn_x"] = cycle_spawn_x;
	result["spawn_y"] = cycle_spawn_y;
	result["transition_common_event_id"] = cycle_reset_common_event_id;
	return result;
}

Dictionary ClockGearsTimeManager::reset_cycle(const Dictionary &p_current_runtime_state) {
	Dictionary filtered_state = apply_cycle_reset_filter(p_current_runtime_state);
	current_cycle_count++;
	total_game_minutes = int64_t(cycle_start_hour) * int64_t(minutes_per_hour);
	fractional_minutes_accum = 0.0;

	emit_signal("cycle_reset", current_cycle_count, filtered_state);
	emit_signal("day_changed", 1, get_day_name(1));
	emit_signal("hour_changed", 1, get_current_hour());
	emit_signal("minute_changed", total_game_minutes, 1, get_current_hour(), 0);
	return filtered_state;
}

Dictionary ClockGearsTimeManager::to_dict() const {
	Dictionary d;
	d["minutes_per_hour"] = minutes_per_hour;
	d["hours_per_day"] = hours_per_day;
	d["days_per_cycle"] = days_per_cycle;
	d["cycle_start_hour"] = cycle_start_hour;
	d["cycle_mode_enabled"] = cycle_mode_enabled;

	Array dn;
	for (int i = 0; i < day_names.size(); i++) {
		dn.push_back(day_names[i]);
	}
	d["day_names"] = dn;
	d["day_periods"] = day_periods.duplicate(true);
	d["total_game_minutes"] = total_game_minutes;
	d["minutes_per_real_second"] = minutes_per_real_second;
	d["current_cycle_count"] = current_cycle_count;
	d["advance_mode"] = advance_mode;
	d["paused"] = paused;
	d["pause_in_dialogue"] = pause_in_dialogue;
	d["pause_in_menu"] = pause_in_menu;
	d["pause_in_cutscene"] = pause_in_cutscene;
	d["pause_in_battle"] = pause_in_battle;
	d["persistent_rules"] = persistent_rules.duplicate(true);
	d["cycle_reset_common_event_id"] = cycle_reset_common_event_id;
	d["cycle_spawn_map"] = cycle_spawn_map;
	d["cycle_spawn_x"] = cycle_spawn_x;
	d["cycle_spawn_y"] = cycle_spawn_y;
	return d;
}

void ClockGearsTimeManager::from_dict(const Dictionary &p_dict) {
	minutes_per_hour = MAX(1, int(p_dict.get("minutes_per_hour", 60)));
	hours_per_day = MAX(1, int(p_dict.get("hours_per_day", 24)));
	days_per_cycle = MAX(1, int(p_dict.get("days_per_cycle", 3)));
	cycle_start_hour = CLAMP(int(p_dict.get("cycle_start_hour", 6)), 0, hours_per_day - 1);
	cycle_mode_enabled = bool(p_dict.get("cycle_mode_enabled", false));

	day_names.clear();
	if (p_dict.has("day_names") && p_dict["day_names"].get_type() == Variant::ARRAY) {
		Array dn = p_dict["day_names"];
		for (int i = 0; i < dn.size(); i++) {
			day_names.push_back(String(dn[i]));
		}
	}

	if (p_dict.has("day_periods") && p_dict["day_periods"].get_type() == Variant::ARRAY) {
		day_periods = Array(p_dict["day_periods"]).duplicate(true);
	}

	total_game_minutes = int64_t(p_dict.get("total_game_minutes", int64_t(cycle_start_hour) * minutes_per_hour));
	minutes_per_real_second = MAX(0.0, double(p_dict.get("minutes_per_real_second", 1.0)));
	current_cycle_count = MAX(1, int(p_dict.get("current_cycle_count", 1)));
	advance_mode = CLAMP(int(p_dict.get("advance_mode", TIME_ADVANCE_REALTIME)), TIME_ADVANCE_REALTIME, TIME_ADVANCE_MANUAL);
	paused = bool(p_dict.get("paused", false));

	pause_in_dialogue = bool(p_dict.get("pause_in_dialogue", true));
	pause_in_menu = bool(p_dict.get("pause_in_menu", true));
	pause_in_cutscene = bool(p_dict.get("pause_in_cutscene", true));
	pause_in_battle = bool(p_dict.get("pause_in_battle", true));

	if (p_dict.has("persistent_rules") && p_dict["persistent_rules"].get_type() == Variant::DICTIONARY) {
		persistent_rules = Dictionary(p_dict["persistent_rules"]).duplicate(true);
	}

	cycle_reset_common_event_id = int(p_dict.get("cycle_reset_common_event_id", 0));
	cycle_spawn_map = String(p_dict.get("cycle_spawn_map", ""));
	cycle_spawn_x = int(p_dict.get("cycle_spawn_x", 0));
	cycle_spawn_y = int(p_dict.get("cycle_spawn_y", 0));
}

void ClockGearsTimeManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init_empty"), &ClockGearsTimeManager::init_empty);
	ClassDB::bind_method(D_METHOD("set_minutes_per_hour", "minutes"), &ClockGearsTimeManager::set_minutes_per_hour);
	ClassDB::bind_method(D_METHOD("get_minutes_per_hour"), &ClockGearsTimeManager::get_minutes_per_hour);
	ClassDB::bind_method(D_METHOD("set_hours_per_day", "hours"), &ClockGearsTimeManager::set_hours_per_day);
	ClassDB::bind_method(D_METHOD("get_hours_per_day"), &ClockGearsTimeManager::get_hours_per_day);
	ClassDB::bind_method(D_METHOD("set_days_per_cycle", "days"), &ClockGearsTimeManager::set_days_per_cycle);
	ClassDB::bind_method(D_METHOD("get_days_per_cycle"), &ClockGearsTimeManager::get_days_per_cycle);
	ClassDB::bind_method(D_METHOD("set_cycle_start_hour", "hour"), &ClockGearsTimeManager::set_cycle_start_hour);
	ClassDB::bind_method(D_METHOD("get_cycle_start_hour"), &ClockGearsTimeManager::get_cycle_start_hour);
	ClassDB::bind_method(D_METHOD("set_cycle_mode_enabled", "enabled"), &ClockGearsTimeManager::set_cycle_mode_enabled);
	ClassDB::bind_method(D_METHOD("is_cycle_mode_enabled"), &ClockGearsTimeManager::is_cycle_mode_enabled);
	ClassDB::bind_method(D_METHOD("set_day_names", "names"), &ClockGearsTimeManager::set_day_names);
	ClassDB::bind_method(D_METHOD("get_day_names"), &ClockGearsTimeManager::get_day_names);
	ClassDB::bind_method(D_METHOD("get_day_name", "day_1based"), &ClockGearsTimeManager::get_day_name);
	ClassDB::bind_method(D_METHOD("set_day_periods", "periods"), &ClockGearsTimeManager::set_day_periods);
	ClassDB::bind_method(D_METHOD("get_day_periods"), &ClockGearsTimeManager::get_day_periods);
	ClassDB::bind_method(D_METHOD("get_current_period_name"), &ClockGearsTimeManager::get_current_period_name);

	ClassDB::bind_method(D_METHOD("set_advance_mode", "mode"), &ClockGearsTimeManager::set_advance_mode);
	ClassDB::bind_method(D_METHOD("get_advance_mode"), &ClockGearsTimeManager::get_advance_mode);
	ClassDB::bind_method(D_METHOD("set_time_scale", "minutes_per_real_sec"), &ClockGearsTimeManager::set_time_scale);
	ClassDB::bind_method(D_METHOD("get_time_scale"), &ClockGearsTimeManager::get_time_scale);
	ClassDB::bind_method(D_METHOD("set_paused", "paused"), &ClockGearsTimeManager::set_paused);
	ClassDB::bind_method(D_METHOD("is_paused"), &ClockGearsTimeManager::is_paused);
	ClassDB::bind_method(D_METHOD("is_effectively_paused"), &ClockGearsTimeManager::is_effectively_paused);
	ClassDB::bind_method(D_METHOD("set_context_state", "dialogue", "menu", "cutscene", "battle"), &ClockGearsTimeManager::set_context_state);
	ClassDB::bind_method(D_METHOD("tick", "delta_seconds"), &ClockGearsTimeManager::tick);

	ClassDB::bind_method(D_METHOD("set_total_minutes", "total_minutes"), &ClockGearsTimeManager::set_total_minutes);
	ClassDB::bind_method(D_METHOD("get_total_minutes"), &ClockGearsTimeManager::get_total_minutes);
	ClassDB::bind_method(D_METHOD("set_game_time", "day_1based", "hour", "minute"), &ClockGearsTimeManager::set_game_time);
	ClassDB::bind_method(D_METHOD("advance_minutes", "minutes"), &ClockGearsTimeManager::advance_minutes);
	ClassDB::bind_method(D_METHOD("skip_to_hour", "target_hour", "target_minute"), &ClockGearsTimeManager::skip_to_hour, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("get_current_minute"), &ClockGearsTimeManager::get_current_minute);
	ClassDB::bind_method(D_METHOD("get_current_hour"), &ClockGearsTimeManager::get_current_hour);
	ClassDB::bind_method(D_METHOD("get_current_day"), &ClockGearsTimeManager::get_current_day);
	ClassDB::bind_method(D_METHOD("get_current_cycle_count"), &ClockGearsTimeManager::get_current_cycle_count);
	ClassDB::bind_method(D_METHOD("get_formatted_time", "include_day"), &ClockGearsTimeManager::get_formatted_time, DEFVAL(true));

	ClassDB::bind_method(D_METHOD("is_time_between", "start_hour", "start_min", "end_hour", "end_min"), &ClockGearsTimeManager::is_time_between);
	ClassDB::bind_method(D_METHOD("is_on_day", "day_1based"), &ClockGearsTimeManager::is_on_day);
	ClassDB::bind_method(D_METHOD("is_in_period", "period_name"), &ClockGearsTimeManager::is_in_period);

	ClassDB::bind_method(D_METHOD("set_persistence_rule", "category", "key", "persistent"), &ClockGearsTimeManager::set_persistence_rule);
	ClassDB::bind_method(D_METHOD("is_persistent", "category", "key"), &ClockGearsTimeManager::is_persistent);
	ClassDB::bind_method(D_METHOD("set_global_persistence_flag", "flag_name", "persistent"), &ClockGearsTimeManager::set_global_persistence_flag);
	ClassDB::bind_method(D_METHOD("get_global_persistence_flag", "flag_name"), &ClockGearsTimeManager::get_global_persistence_flag);
	ClassDB::bind_method(D_METHOD("set_persistence_rules", "rules"), &ClockGearsTimeManager::set_persistence_rules);
	ClassDB::bind_method(D_METHOD("get_persistence_rules"), &ClockGearsTimeManager::get_persistence_rules);
	ClassDB::bind_method(D_METHOD("get_persistence_summary"), &ClockGearsTimeManager::get_persistence_summary);
	ClassDB::bind_method(D_METHOD("set_cycle_reset_transition", "common_event_id", "spawn_map", "spawn_x", "spawn_y"), &ClockGearsTimeManager::set_cycle_reset_transition);
	ClassDB::bind_method(D_METHOD("capture_cycle_initial_snapshot", "runtime_state"), &ClockGearsTimeManager::capture_cycle_initial_snapshot);
	ClassDB::bind_method(D_METHOD("apply_cycle_reset_filter", "current_runtime_state"), &ClockGearsTimeManager::apply_cycle_reset_filter);
	ClassDB::bind_method(D_METHOD("reset_cycle", "current_runtime_state"), &ClockGearsTimeManager::reset_cycle, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("to_dict"), &ClockGearsTimeManager::to_dict);
	ClassDB::bind_method(D_METHOD("from_dict", "dict"), &ClockGearsTimeManager::from_dict);

	ADD_SIGNAL(MethodInfo("minute_changed", PropertyInfo(Variant::INT, "total_minutes"), PropertyInfo(Variant::INT, "day"), PropertyInfo(Variant::INT, "hour"), PropertyInfo(Variant::INT, "minute")));
	ADD_SIGNAL(MethodInfo("hour_changed", PropertyInfo(Variant::INT, "day"), PropertyInfo(Variant::INT, "hour")));
	ADD_SIGNAL(MethodInfo("day_changed", PropertyInfo(Variant::INT, "day"), PropertyInfo(Variant::STRING, "day_name")));
	ADD_SIGNAL(MethodInfo("period_changed", PropertyInfo(Variant::STRING, "period_name")));
	ADD_SIGNAL(MethodInfo("cycle_reset", PropertyInfo(Variant::INT, "cycle_number"), PropertyInfo(Variant::DICTIONARY, "preserved_state")));
}

ClockGearsTimeManager::ClockGearsTimeManager() {
	singleton = this;
	init_empty();
}

ClockGearsTimeManager::~ClockGearsTimeManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
