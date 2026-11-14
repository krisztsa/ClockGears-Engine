#include "clockgears_battle.h"
#include "clockgears_debug_panel.h"

// ============================================================================
// ClockGearsBattleUnit
// ============================================================================

void ClockGearsBattleUnit::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &ClockGearsBattleUnit::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &ClockGearsBattleUnit::get_id);

	ClassDB::bind_method(D_METHOD("set_name", "name"), &ClockGearsBattleUnit::set_name);
	ClassDB::bind_method(D_METHOD("get_name"), &ClockGearsBattleUnit::get_name);

	ClassDB::bind_method(D_METHOD("set_is_actor", "actor"), &ClockGearsBattleUnit::set_is_actor);
	ClassDB::bind_method(D_METHOD("get_is_actor"), &ClockGearsBattleUnit::get_is_actor);

	ClassDB::bind_method(D_METHOD("set_level", "level"), &ClockGearsBattleUnit::set_level);
	ClassDB::bind_method(D_METHOD("get_level"), &ClockGearsBattleUnit::get_level);

	ClassDB::bind_method(D_METHOD("set_exp", "exp"), &ClockGearsBattleUnit::set_exp);
	ClassDB::bind_method(D_METHOD("get_exp"), &ClockGearsBattleUnit::get_exp);

	ClassDB::bind_method(D_METHOD("set_hp", "hp"), &ClockGearsBattleUnit::set_hp);
	ClassDB::bind_method(D_METHOD("get_hp"), &ClockGearsBattleUnit::get_hp);

	ClassDB::bind_method(D_METHOD("set_max_hp", "max_hp"), &ClockGearsBattleUnit::set_max_hp);
	ClassDB::bind_method(D_METHOD("get_max_hp"), &ClockGearsBattleUnit::get_max_hp);

	ClassDB::bind_method(D_METHOD("set_mp", "mp"), &ClockGearsBattleUnit::set_mp);
	ClassDB::bind_method(D_METHOD("get_mp"), &ClockGearsBattleUnit::get_mp);

	ClassDB::bind_method(D_METHOD("set_max_mp", "max_mp"), &ClockGearsBattleUnit::set_max_mp);
	ClassDB::bind_method(D_METHOD("get_max_mp"), &ClockGearsBattleUnit::get_max_mp);

	ClassDB::bind_method(D_METHOD("set_atk", "atk"), &ClockGearsBattleUnit::set_atk);
	ClassDB::bind_method(D_METHOD("get_atk"), &ClockGearsBattleUnit::get_atk);

	ClassDB::bind_method(D_METHOD("set_def", "def"), &ClockGearsBattleUnit::set_def);
	ClassDB::bind_method(D_METHOD("get_def"), &ClockGearsBattleUnit::get_def);

	ClassDB::bind_method(D_METHOD("set_mat", "mat"), &ClockGearsBattleUnit::set_mat);
	ClassDB::bind_method(D_METHOD("get_mat"), &ClockGearsBattleUnit::get_mat);

	ClassDB::bind_method(D_METHOD("set_mdf", "mdf"), &ClockGearsBattleUnit::set_mdf);
	ClassDB::bind_method(D_METHOD("get_mdf"), &ClockGearsBattleUnit::get_mdf);

	ClassDB::bind_method(D_METHOD("set_agi", "agi"), &ClockGearsBattleUnit::set_agi);
	ClassDB::bind_method(D_METHOD("get_agi"), &ClockGearsBattleUnit::get_agi);

	ClassDB::bind_method(D_METHOD("set_luk", "luk"), &ClockGearsBattleUnit::set_luk);
	ClassDB::bind_method(D_METHOD("get_luk"), &ClockGearsBattleUnit::get_luk);

	ClassDB::bind_method(D_METHOD("set_exp_reward", "exp"), &ClockGearsBattleUnit::set_exp_reward);
	ClassDB::bind_method(D_METHOD("get_exp_reward"), &ClockGearsBattleUnit::get_exp_reward);

	ClassDB::bind_method(D_METHOD("set_gold_reward", "gold"), &ClockGearsBattleUnit::set_gold_reward);
	ClassDB::bind_method(D_METHOD("get_gold_reward"), &ClockGearsBattleUnit::get_gold_reward);

	ClassDB::bind_method(D_METHOD("set_drops", "drops"), &ClockGearsBattleUnit::set_drops);
	ClassDB::bind_method(D_METHOD("get_drops"), &ClockGearsBattleUnit::get_drops);

	ClassDB::bind_method(D_METHOD("set_states", "states"), &ClockGearsBattleUnit::set_states);
	ClassDB::bind_method(D_METHOD("get_states"), &ClockGearsBattleUnit::get_states);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &ClockGearsBattleUnit::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &ClockGearsBattleUnit::get_traits);
	ClassDB::bind_method(D_METHOD("get_element_rate", "element_id", "element_name"), &ClockGearsBattleUnit::get_element_rate, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("set_actions", "actions"), &ClockGearsBattleUnit::set_actions);
	ClassDB::bind_method(D_METHOD("get_actions"), &ClockGearsBattleUnit::get_actions);
	ClassDB::bind_method(D_METHOD("set_cast_animations", "casts"), &ClockGearsBattleUnit::set_cast_animations);
	ClassDB::bind_method(D_METHOD("get_cast_animations"), &ClockGearsBattleUnit::get_cast_animations);
	ClassDB::bind_method(D_METHOD("get_current_anim_slot"), &ClockGearsBattleUnit::get_current_anim_slot);
	ClassDB::bind_method(D_METHOD("set_current_anim_slot", "slot"), &ClockGearsBattleUnit::set_current_anim_slot);
	ClassDB::bind_method(D_METHOD("get_last_played_anim_slot"), &ClockGearsBattleUnit::get_last_played_anim_slot);
	ClassDB::bind_method(D_METHOD("resolve_slot_animation_name", "slot_type", "element_id", "element_name"), &ClockGearsBattleUnit::resolve_slot_animation_name, DEFVAL(0), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("play_slot_animation", "slot_name", "duration"), &ClockGearsBattleUnit::play_slot_animation, DEFVAL(0.5f));
	ClassDB::bind_method(D_METHOD("get_active_effect_anim_id"), &ClockGearsBattleUnit::get_active_effect_anim_id);
	ClassDB::bind_method(D_METHOD("has_state", "state_id"), &ClockGearsBattleUnit::has_state);
	ClassDB::bind_method(D_METHOD("apply_state", "state_id", "name", "turns", "restriction"), &ClockGearsBattleUnit::apply_state, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("remove_state", "state_id"), &ClockGearsBattleUnit::remove_state);

	ClassDB::bind_method(D_METHOD("set_is_defending", "defending"), &ClockGearsBattleUnit::set_is_defending);
	ClassDB::bind_method(D_METHOD("get_is_defending"), &ClockGearsBattleUnit::get_is_defending);

	ClassDB::bind_method(D_METHOD("set_is_dead", "dead"), &ClockGearsBattleUnit::set_is_dead);
	ClassDB::bind_method(D_METHOD("get_is_dead"), &ClockGearsBattleUnit::get_is_dead);

	ClassDB::bind_method(D_METHOD("set_action_speed", "speed"), &ClockGearsBattleUnit::set_action_speed);
	ClassDB::bind_method(D_METHOD("get_action_speed"), &ClockGearsBattleUnit::get_action_speed);

	ClassDB::bind_method(D_METHOD("set_graphic_path", "path"), &ClockGearsBattleUnit::set_graphic_path);
	ClassDB::bind_method(D_METHOD("get_graphic_path"), &ClockGearsBattleUnit::get_graphic_path);

	ClassDB::bind_method(D_METHOD("set_trigger_table", "table"), &ClockGearsBattleUnit::set_trigger_table);
	ClassDB::bind_method(D_METHOD("get_trigger_table"), &ClockGearsBattleUnit::get_trigger_table);

	ClassDB::bind_method(D_METHOD("set_battle_pos", "pos"), &ClockGearsBattleUnit::set_battle_pos);
	ClassDB::bind_method(D_METHOD("get_battle_pos"), &ClockGearsBattleUnit::get_battle_pos);

	ClassDB::bind_method(D_METHOD("set_sprite_data_dict", "data"), &ClockGearsBattleUnit::set_sprite_data_dict);
	ClassDB::bind_method(D_METHOD("get_sprite_data_dict"), &ClockGearsBattleUnit::get_sprite_data_dict);

	ClassDB::bind_method(D_METHOD("to_formula_dict"), &ClockGearsBattleUnit::to_formula_dict);
}

bool ClockGearsBattleUnit::has_state(int p_state_id) const {
	for (int i = 0; i < states.size(); i++) {
		Dictionary st = states[i];
		if (int(st.get("id", 0)) == p_state_id) {
			return true;
		}
	}
	return false;
}

void ClockGearsBattleUnit::apply_state(int p_state_id, const String &p_name, int p_turns, int p_restriction) {
	for (int i = 0; i < states.size(); i++) {
		Dictionary st = states[i];
		if (int(st.get("id", 0)) == p_state_id) {
			st["turns"] = p_turns;
			states[i] = st;
			return;
		}
	}
	Dictionary st;
	st["id"] = p_state_id;
	st["name"] = p_name;
	st["turns"] = p_turns;
	st["restriction"] = p_restriction;
	states.push_back(st);

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		am->trigger_animation(nullptr, ANIM_TRIGGER_STATE_ADD, p_state_id, trigger_table);
	}
}

void ClockGearsBattleUnit::remove_state(int p_state_id) {
	for (int i = states.size() - 1; i >= 0; i--) {
		Dictionary st = states[i];
		if (int(st.get("id", 0)) == p_state_id) {
			states.remove_at(i);
		}
	}
}

float ClockGearsBattleUnit::get_element_rate(int p_element_id, const String &p_element_name) const {
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	int resolved_id = p_element_id;
	if (resolved_id <= 0 && !p_element_name.strip_edges().is_empty() && cat) {
		resolved_id = cat->resolve_element_id(p_element_name);
	}
	String target_key = (cat && resolved_id > 0) ? cat->get_element_key(resolved_id).to_lower() : p_element_name.strip_edges().to_lower();
	String target_name = (cat && resolved_id > 0) ? cat->get_element_name(resolved_id).to_lower() : p_element_name.strip_edges().to_lower();

	if (resolved_id <= 0 && target_key.is_empty()) {
		return 1.0f;
	}

	float rate = 1.0f;
	bool found = false;
	for (int i = 0; i < traits.size(); i++) {
		if (traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = traits[i];
		int code = int(tr.get("code", 0));
		if (code != CG_TRAIT_ELEMENT_RATE) {
			continue;
		}
		int d_id = int(tr.get("data_id", -1));
		String tr_key = String(tr.get("element_key", tr.get("element", ""))).strip_edges().to_lower();
		bool match = false;
		if (!tr_key.is_empty() && (tr_key == target_key || tr_key == target_name)) {
			match = true;
		} else if (resolved_id > 0 && d_id == resolved_id) {
			match = true;
		} else if (tr_key.is_empty() && cat && resolved_id > 0 && d_id >= 0 && (d_id + 1) == resolved_id && !bool(tr.get("one_based", false))) {
			// Compatibilidad con rasgos antiguos guardados con índice 0-based
			bool has_exact_one_based = false;
			for (int j = 0; j < traits.size(); j++) {
				if (traits[j].get_type() == Variant::DICTIONARY) {
					Dictionary tj = traits[j];
					if (int(tj.get("code", 0)) == CG_TRAIT_ELEMENT_RATE && int(tj.get("data_id", -1)) == resolved_id) {
						has_exact_one_based = true;
						break;
					}
				}
			}
			if (!has_exact_one_based) {
				match = true;
			}
		}
		if (match) {
			float pct = float(tr.get("value", 100.0));
			rate = MAX(0.0f, pct / 100.0f);
			found = true;
		}
	}
	return found ? rate : 1.0f;
}

int ClockGearsBattleUnit::get_attack_element_id() const {
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	for (int i = 0; i < traits.size(); i++) {
		if (traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = traits[i];
		if (int(tr.get("code", 0)) == CG_TRAIT_ATTACK_ELEMENT) {
			String ek = String(tr.get("element_key", tr.get("element", ""))).strip_edges();
			if (!ek.is_empty() && cat) {
				int rid = cat->resolve_element_id(ek);
				if (rid > 0) return rid;
			}
			int did = int(tr.get("data_id", 0));
			if (did > 0) return did;
		}
	}
	return 0;
}

static String _find_anim_key_ci(const ClockGearsSpriteSheetData &p_sp, const String &p_candidate) {
	if (p_candidate.is_empty()) return "";
	if (p_sp.animations.has(p_candidate)) return p_candidate;
	String low = p_candidate.to_lower();
	for (const KeyValue<String, ClockGearsSpriteAnimation> &E : p_sp.animations) {
		if (E.key.to_lower() == low) {
			return E.key;
		}
	}
	return "";
}

String ClockGearsBattleUnit::resolve_slot_animation_name(const String &p_slot_type, int p_element_id, const String &p_element_name) const {
	String st = p_slot_type.strip_edges().to_lower();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	int resolved_elem = p_element_id;
	if (resolved_elem <= 0 && !p_element_name.strip_edges().is_empty() && cat) {
		resolved_elem = cat->resolve_element_id(p_element_name);
	}
	String elem_key = (cat && resolved_elem > 0) ? cat->get_element_key(resolved_elem).to_lower() : p_element_name.strip_edges().to_lower();
	String elem_name = (cat && resolved_elem > 0) ? cat->get_element_name(resolved_elem).to_lower() : p_element_name.strip_edges().to_lower();

	if (st == "cast" || st == "lanzamiento") {
		String first_cast_anim;
		for (int i = 0; i < cast_animations.size(); i++) {
			if (cast_animations[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary cd = cast_animations[i];
			String c_name = String(cd.get("anim_name", cd.get("name", ""))).strip_edges();
			if (c_name.is_empty()) continue;
			if (first_cast_anim.is_empty()) {
				first_cast_anim = c_name;
			}
			int c_elem_id = int(cd.get("element_id", 0));
			String c_elem_str = String(cd.get("element_key", cd.get("element", ""))).strip_edges().to_lower();
			if (c_elem_id <= 0 && !c_elem_str.is_empty() && cat) {
				c_elem_id = cat->resolve_element_id(c_elem_str);
			}
			bool elem_match = false;
			if (resolved_elem > 0 && c_elem_id == resolved_elem) {
				elem_match = true;
			} else if (!elem_key.is_empty() && !c_elem_str.is_empty() && (c_elem_str == elem_key || c_elem_str == elem_name)) {
				elem_match = true;
			}
			if (elem_match) {
				String found = _find_anim_key_ci(sprite_data, c_name);
				return !found.is_empty() ? found : c_name;
			}
		}
		if (!first_cast_anim.is_empty()) {
			String found = _find_anim_key_ci(sprite_data, first_cast_anim);
			return !found.is_empty() ? found : first_cast_anim;
		}
		// Si no hay ningún Lanzamiento, usar Ataque
		st = "attack";
	}

	if (st == "idle" || st == "reposo") {
		String cand = get_combat_animation(ANIM_TRIGGER_IDLE, "");
		String f = _find_anim_key_ci(sprite_data, cand);
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "Idle");
		if (!f.is_empty()) return f;
		return !cand.is_empty() ? cand : "idle";
	}
	if (st == "attack" || st == "ataque") {
		String cand = get_combat_animation(ANIM_TRIGGER_ATTACK, "");
		String f = _find_anim_key_ci(sprite_data, cand);
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "Ataque");
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "attack");
		if (!f.is_empty()) return f;
		return !cand.is_empty() ? cand : "attack";
	}
	if (st == "damage" || st == "hurt" || st == String::utf8("daño") || st == "dano") {
		String cand = get_combat_animation(ANIM_TRIGGER_DAMAGE, "");
		String f = _find_anim_key_ci(sprite_data, cand);
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, String::utf8("Daño"));
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "damage");
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "hurt");
		if (!f.is_empty()) return f;
		return !cand.is_empty() ? cand : "damage";
	}
	if (st == "death" || st == "muerte") {
		String cand = get_combat_animation(ANIM_TRIGGER_DEATH, "");
		String f = _find_anim_key_ci(sprite_data, cand);
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "Muerte");
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "death");
		if (!f.is_empty()) return f;
		return !cand.is_empty() ? cand : "death";
	}
	if (st == "buff") {
		String cand = get_combat_animation(ANIM_TRIGGER_SKILL, "");
		String f = _find_anim_key_ci(sprite_data, "Buff");
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, "buff");
		if (!f.is_empty()) return f;
		f = _find_anim_key_ci(sprite_data, cand);
		if (!f.is_empty()) return f;
		return "buff";
	}

	String direct = _find_anim_key_ci(sprite_data, p_slot_type);
	return !direct.is_empty() ? direct : p_slot_type;
}

void ClockGearsBattleUnit::play_slot_animation(const String &p_slot_name, float p_duration) {
	if (p_slot_name.is_empty()) {
		return;
	}
	current_anim_slot = p_slot_name;
	last_played_anim_slot = p_slot_name;
	anim_slot_timer = MAX(0.0f, p_duration);
}

Dictionary ClockGearsBattleUnit::to_formula_dict() const {
	Dictionary d;
	d["atk"] = atk;
	d["def"] = def;
	d["mat"] = mat;
	d["matk"] = mat;
	d["mdf"] = mdf;
	d["mdef"] = mdf;
	d["agi"] = agi;
	d["luk"] = luk;
	d["hp"] = hp;
	d["max_hp"] = max_hp;
	d["mhp"] = max_hp;
	d["mp"] = mp;
	d["max_mp"] = max_mp;
	d["mmp"] = max_mp;
	d["level"] = level;
	return d;
}

ClockGearsBattleUnit::ClockGearsBattleUnit() {
	trigger_table.instantiate();
}

// ============================================================================
// ClockGearsBattleController
// ============================================================================

void ClockGearsBattleController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_battle", "troop_id", "can_escape", "can_lose", "bgm"), &ClockGearsBattleController::setup_battle, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_party_unit", "unit"), &ClockGearsBattleController::add_party_unit);
	ClassDB::bind_method(D_METHOD("add_enemy_unit", "unit"), &ClockGearsBattleController::add_enemy_unit);
	ClassDB::bind_method(D_METHOD("get_party_units"), &ClockGearsBattleController::get_party_units);
	ClassDB::bind_method(D_METHOD("get_enemy_units"), &ClockGearsBattleController::get_enemy_units);

	ClassDB::bind_method(D_METHOD("calculate_initiative"), &ClockGearsBattleController::calculate_initiative);
	ClassDB::bind_method(D_METHOD("get_turn_order"), &ClockGearsBattleController::get_turn_order);
	ClassDB::bind_method(D_METHOD("get_active_unit"), &ClockGearsBattleController::get_active_unit);
	ClassDB::bind_method(D_METHOD("get_round"), &ClockGearsBattleController::get_round);

	ClassDB::bind_method(D_METHOD("action_attack", "target_idx"), &ClockGearsBattleController::action_attack);
	ClassDB::bind_method(D_METHOD("action_skill", "skill_id", "target_idx"), &ClockGearsBattleController::action_skill);
	ClassDB::bind_method(D_METHOD("action_item", "item_name", "target_idx"), &ClockGearsBattleController::action_item);
	ClassDB::bind_method(D_METHOD("action_defend"), &ClockGearsBattleController::action_defend);
	ClassDB::bind_method(D_METHOD("action_flee"), &ClockGearsBattleController::action_flee);

	ClassDB::bind_method(D_METHOD("execute_enemy_turn", "enemy"), &ClockGearsBattleController::execute_enemy_turn);
	ClassDB::bind_method(D_METHOD("start_turn_cycle"), &ClockGearsBattleController::start_turn_cycle);
	ClassDB::bind_method(D_METHOD("advance_turn"), &ClockGearsBattleController::advance_turn);
	ClassDB::bind_method(D_METHOD("tick_states", "unit"), &ClockGearsBattleController::tick_states);
	ClassDB::bind_method(D_METHOD("check_battle_end"), &ClockGearsBattleController::check_battle_end);

	ClassDB::bind_method(D_METHOD("get_result"), &ClockGearsBattleController::get_result);
	ClassDB::bind_method(D_METHOD("is_finished"), &ClockGearsBattleController::is_finished);
	ClassDB::bind_method(D_METHOD("get_total_exp"), &ClockGearsBattleController::get_total_exp);
	ClassDB::bind_method(D_METHOD("get_total_gold"), &ClockGearsBattleController::get_total_gold);
	ClassDB::bind_method(D_METHOD("get_dropped_items"), &ClockGearsBattleController::get_dropped_items);
	ClassDB::bind_method(D_METHOD("get_level_up_messages"), &ClockGearsBattleController::get_level_up_messages);
	ClassDB::bind_method(D_METHOD("get_log_history"), &ClockGearsBattleController::get_log_history);
	ClassDB::bind_method(D_METHOD("get_last_hit_log"), &ClockGearsBattleController::get_last_hit_log);
	ClassDB::bind_method(D_METHOD("get_last_element_multiplier"), &ClockGearsBattleController::get_last_element_multiplier);
	ClassDB::bind_method(D_METHOD("get_troop_bg_path"), &ClockGearsBattleController::get_troop_bg_path);
	ClassDB::bind_method(D_METHOD("set_troop_bg_path", "path"), &ClockGearsBattleController::set_troop_bg_path);
	ClassDB::bind_method(D_METHOD("get_troop_floor_path"), &ClockGearsBattleController::get_troop_floor_path);
	ClassDB::bind_method(D_METHOD("set_troop_floor_path", "path"), &ClockGearsBattleController::set_troop_floor_path);

	ADD_SIGNAL(MethodInfo("hit_executed", PropertyInfo(Variant::STRING, "log")));

	BIND_CONSTANT(RESULT_NONE);
	BIND_CONSTANT(RESULT_VICTORY);
	BIND_CONSTANT(RESULT_DEFEAT);
	BIND_CONSTANT(RESULT_ESCAPED);
	BIND_CONSTANT(RESULT_GAME_OVER);
}

static String _format_elem_mult(float p_mult) {
	if (Math::is_equal_approx(p_mult, 0.0f)) return "x0";
	if (Math::is_equal_approx(p_mult, 0.5f)) return "x0.5";
	if (Math::is_equal_approx(p_mult, 1.0f)) return "x1";
	if (Math::is_equal_approx(p_mult, 2.0f)) return "x2";
	if (Math::is_equal_approx(p_mult, Math::round(p_mult))) {
		return vformat("x%d", int(Math::round(p_mult)));
	}
	String s = String::num(p_mult, 2);
	while (s.ends_with("0") && s.find(".") != -1) {
		s = s.substr(0, s.length() - 1);
	}
	if (s.ends_with(".")) {
		s = s.substr(0, s.length() - 1);
	}
	return "x" + s;
}

void ClockGearsBattleController::_log_hit(const String &p_attacker, int p_a_atk, const String &p_target, int p_b_def, const String &p_action, const String &p_formula, int64_t p_damage, bool p_crit, bool p_hit, const String &p_elem_info) {
	String entry;
	String elem_suffix = p_elem_info.is_empty() ? "" : (" | " + p_elem_info);
	if (!p_hit) {
		entry = vformat(String::utf8("[Batalla] %s (ATK: %d) usa %s en %s (DEF: %d) -> ¡FALLÓ!%s"), p_attacker, p_a_atk, p_action, p_target, p_b_def, elem_suffix);
	} else {
		entry = vformat(String::utf8("[Batalla] %s (ATK: %d) usa %s -> %s (DEF: %d) | Fórmula: '%s'%s | Daño final: %d (Crítico: %s)"),
				p_attacker, p_a_atk, p_action, p_target, p_b_def, p_formula, elem_suffix, p_damage, p_crit ? "Sí" : "No");
	}
	print_line(entry);
	log_history.push_back(entry);
	last_hit_log = entry;
	ClockGearsDebugOverlay::log_battle(entry);
	emit_signal("hit_executed", entry);
}

void ClockGearsBattleController::setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm) {
	troop_id = p_troop_id;
	can_escape = p_can_escape;
	can_lose = p_can_lose;
	battle_bgm = p_bgm;

	party.clear();
	enemies.clear();
	turn_queue.clear();
	dropped_items.clear();
	level_up_messages.clear();
	log_history.clear();
	total_exp_reward = 0;
	total_gold_reward = 0;
	round_counter = 1;
	current_queue_idx = 0;
	battle_result = RESULT_NONE;
	phase = PHASE_INIT;
	last_hit_log = "";

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();

	// 1. Cargar Grupo del Jugador (CAT_ACTORS o Héroe por defecto)
	Array db_actors = dm ? dm->get_category_array(CAT_ACTORS) : Array();
	if (!db_actors.is_empty()) {
		for (int i = 0; i < db_actors.size() && i < 4; i++) {
			Dictionary d = db_actors[i];
			Ref<ClockGearsBattleUnit> u;
			u.instantiate();
			u->set_id(int(d.get("id", i + 1)));
			u->set_name(d.get("name", "Heroe"));
			u->set_is_actor(true);
			u->set_level(int(d.get("initial_level", 1)));
			u->set_max_hp(MAX(1, int(d.get("hp", 500))));
			u->set_hp(u->get_max_hp());
			u->set_max_mp(MAX(0, int(d.get("mp", 50))));
			u->set_mp(u->get_max_mp());
			u->set_atk(MAX(1, int(d.get("atk", 30))));
			u->set_def(MAX(0, int(d.get("def", 20))));
			u->set_mat(MAX(1, int(d.get("mat", 15))));
			u->set_mdf(MAX(0, int(d.get("mdf", 15))));
			u->set_agi(MAX(1, int(d.get("agi", 25))));
			u->set_luk(MAX(1, int(d.get("luk", 15))));
			u->set_traits(d.get("traits", Array()));
			u->set_graphic_path(d.get("graphic_path", ""));
			if (d.has("sprite_data")) {
				u->set_sprite_data_dict(d.get("sprite_data", Dictionary()));
			} else if (d.has("animations")) {
				Dictionary sp;
				sp["animations"] = d.get("animations", Dictionary());
				sp["cols"] = int(d.get("cols", 4));
				sp["rows"] = 1;
				u->set_sprite_data_dict(sp);
			}
			if (d.has("trigger_table")) {
				Ref<ClockGearsAnimationTriggerTable> tt;
				tt.instantiate();
				tt->deserialize(d.get("trigger_table", Dictionary()));
				u->set_trigger_table(tt);
			} else if (d.has("combat_animations")) {
				Ref<ClockGearsAnimationTriggerTable> tt;
				tt.instantiate();
				Dictionary ca = d.get("combat_animations", Dictionary());
				Array keys = ca.keys();
				for (int k = 0; k < keys.size(); k++) {
					tt->bind_trigger_named(keys[k], ca[keys[k]]);
				}
				u->set_trigger_table(tt);
			}
			party.push_back(u);
		}
	} else {
		Ref<ClockGearsBattleUnit> hero;
		hero.instantiate();
		hero->set_name("Heroe");
		hero->set_is_actor(true);
		hero->set_level(1);
		hero->set_max_hp(500);
		hero->set_hp(500);
		hero->set_max_mp(50);
		hero->set_mp(50);
		hero->set_atk(30);
		hero->set_def(20);
		hero->set_mat(15);
		hero->set_mdf(15);
		hero->set_agi(25);
		hero->set_luk(15);
		party.push_back(hero);
	}

	// 2. Cargar Enemigos de la Tropa (CAT_TROOPS -> CAT_ENEMIES)
	Dictionary troop_rec;
	if (dm) {
		troop_rec = dm->get_record_by_id(CAT_TROOPS, troop_id);
		if (troop_rec.is_empty()) {
			Array troops_all = dm->get_category_array(CAT_TROOPS);
			for (int t = 0; t < troops_all.size(); t++) {
				Dictionary tr = troops_all[t];
				if (int(tr.get("id", t + 1)) == troop_id) {
					troop_rec = tr;
					break;
				}
			}
			if (troop_rec.is_empty() && !troops_all.is_empty()) {
				troop_rec = troops_all[0];
			}
		}
	}

	troop_bg_path = String(troop_rec.get("battleback1", troop_rec.get("bg_path", ""))).strip_edges();
	troop_floor_path = String(troop_rec.get("battleback2", troop_rec.get("floor_path", ""))).strip_edges();

	Array troop_members = troop_rec.get("members", Array());
	if (troop_members.is_empty() && troop_rec.has("type") && int(troop_rec["type"]) > 0) {
		Dictionary m;
		m["enemy_id"] = troop_rec["type"];
		m["x"] = 300;
		m["y"] = 260;
		troop_members.push_back(m);
	}

	if (!troop_members.is_empty() && dm) {
		for (int i = 0; i < troop_members.size() && i < 8; i++) {
			Variant item = troop_members[i];
			int e_id = 1;
			int pos_x = 200 + i * 160;
			int pos_y = 260;
			if (item.get_type() == Variant::DICTIONARY) {
				Dictionary md = item;
				e_id = int(md.get("enemy_id", md.get("id", 1)));
				pos_x = int(md.get("x", pos_x));
				pos_y = int(md.get("y", pos_y));
			} else {
				e_id = int(item);
			}

			Dictionary en_data = dm->get_record_by_id(CAT_ENEMIES, e_id);
			if (en_data.is_empty()) {
				Array all_e = dm->get_category_array(CAT_ENEMIES);
				for (int e = 0; e < all_e.size(); e++) {
					Dictionary ed = all_e[e];
					if (int(ed.get("id", e + 1)) == e_id) {
						en_data = ed;
						break;
					}
				}
			}

			Ref<ClockGearsBattleUnit> en;
			en.instantiate();
			en->set_id(e_id);
			String base_name = en_data.get("name", vformat("Enemigo_%d", e_id));
			if (base_name.strip_edges().is_empty()) {
				base_name = String::utf8("Murciélago");
			}
			en->set_name(base_name);
			en->set_is_actor(false);
			en->set_level(MAX(1, int(en_data.get("level", 1))));
			en->set_battle_pos(Vector2(pos_x, pos_y));
			en->set_max_hp(MAX(1, int(en_data.get("hp", 120))));
			en->set_hp(en->get_max_hp());
			en->set_max_mp(MAX(0, int(en_data.get("mp", 20))));
			en->set_mp(en->get_max_mp());
			en->set_atk(MAX(1, int(en_data.get("atk", 18))));
			en->set_def(MAX(0, int(en_data.get("def", 10))));
			en->set_mat(MAX(1, int(en_data.get("mat", 10))));
			en->set_mdf(MAX(0, int(en_data.get("mdf", 10))));
			en->set_agi(MAX(1, int(en_data.get("agi", 15))));
			en->set_luk(MAX(1, int(en_data.get("luk", 10))));
			en->set_exp_reward(int(en_data.get("exp", 30)));
			en->set_gold_reward(int(en_data.get("gold", 20)));
			en->set_drops(en_data.get("drops", Array()));
			en->set_traits(en_data.get("traits", Array()));
			en->set_actions(en_data.get("actions", en_data.get("action_patterns", Array())));
			en->set_cast_animations(en_data.get("cast_animations", en_data.get("lanzamientos", Array())));
			en->set_graphic_path(en_data.get("graphic_path", ""));
			if (en_data.has("sprite_data") && en_data["sprite_data"].get_type() == Variant::DICTIONARY && !Dictionary(en_data["sprite_data"]).is_empty()) {
				en->set_sprite_data_dict(en_data.get("sprite_data", Dictionary()));
			} else if (en_data.has("animations") && en_data["animations"].get_type() == Variant::DICTIONARY && !Dictionary(en_data["animations"]).is_empty()) {
				Dictionary sp;
				sp["animations"] = en_data.get("animations", Dictionary());
				sp["cols"] = int(en_data.get("cols", 4));
				sp["rows"] = 1;
				en->set_sprite_data_dict(sp);
			} else if (en_data.has("battler_frames") || en_data.has("battler_fps")) {
				// Migración de datos antiguos de Frames Battler / FPS Battler
				int old_frames = MAX(1, int(en_data.get("battler_frames", 1)));
				float old_fps = MAX(1.0f, float(en_data.get("battler_fps", 8.0f)));
				ClockGearsSpriteSheetData sp_mig;
				sp_mig.cols = old_frames;
				sp_mig.rows = 1;
				ClockGearsSpriteAnimation idle_mig;
				idle_mig.name = "idle";
				idle_mig.spritesheet_path = en->get_graphic_path();
				idle_mig.fps = old_fps;
				idle_mig.loop = true;
				for (int f = 0; f < old_frames; f++) {
					idle_mig.frames.push_back(f);
				}
				sp_mig.animations["idle"] = idle_mig;
				en->set_sprite_data(sp_mig);
			}
			if (en_data.has("trigger_table")) {
				Ref<ClockGearsAnimationTriggerTable> tt;
				tt.instantiate();
				tt->deserialize(en_data.get("trigger_table", Dictionary()));
				en->set_trigger_table(tt);
			} else if (en_data.has("combat_animations")) {
				Ref<ClockGearsAnimationTriggerTable> tt;
				tt.instantiate();
				Dictionary ca = en_data.get("combat_animations", Dictionary());
				Array keys = ca.keys();
				for (int k = 0; k < keys.size(); k++) {
					tt->bind_trigger_named(keys[k], ca[keys[k]]);
				}
				en->set_trigger_table(tt);
			}
			en->set_current_anim_slot(en->resolve_slot_animation_name("idle"));
			enemies.push_back(en);
		}

		// Disambiguar nombres duplicados
		for (int i = 0; i < enemies.size(); i++) {
			int same_count = 0;
			for (int j = 0; j < enemies.size(); j++) {
				if (enemies[j]->get_id() == enemies[i]->get_id()) same_count++;
			}
			if (same_count > 1) {
				int occ = 0;
				for (int j = 0; j <= i; j++) {
					if (enemies[j]->get_id() == enemies[i]->get_id()) occ++;
				}
				char letter = 'A' + (occ - 1);
				enemies.write[i]->set_name(vformat("%s %c", enemies[i]->get_name(), letter));
			}
		}
	} else {
		// Fallback si no hay tropas en la base de datos
		Ref<ClockGearsBattleUnit> bat;
		bat.instantiate();
		bat->set_id(1);
		bat->set_name(String::utf8("Murciélago"));
		bat->set_is_actor(false);
		bat->set_battle_pos(Vector2(300, 150));
		bat->set_max_hp(120);
		bat->set_hp(120);
		bat->set_max_mp(20);
		bat->set_mp(20);
		bat->set_atk(18);
		bat->set_def(10);
		bat->set_mat(10);
		bat->set_mdf(10);
		bat->set_agi(15);
		bat->set_luk(10);
		bat->set_exp_reward(40);
		bat->set_gold_reward(25);
		bat->set_graphic_path("res://img/battlers/cañón_musgoso_down.png");
		ClockGearsSpriteAnimation idle_a;
		idle_a.name = "idle";
		idle_a.frames.push_back(0);
		idle_a.frames.push_back(1);
		idle_a.frames.push_back(2);
		idle_a.frames.push_back(3);
		ClockGearsSpriteSheetData sp_fb;
		sp_fb.animations["idle"] = idle_a;
		bat->set_sprite_data(sp_fb);
		Array drs;
		Dictionary d1;
		d1["kind"] = 0;
		d1["item_id"] = 1;
		d1["prob"] = 100;
		drs.push_back(d1);
		bat->set_drops(drs);
		enemies.push_back(bat);
	}

	calculate_initiative();
	start_turn_cycle();
}

void ClockGearsBattleController::add_party_unit(const Ref<ClockGearsBattleUnit> &p_unit) {
	if (p_unit.is_valid()) {
		party.push_back(p_unit);
	}
}

void ClockGearsBattleController::add_enemy_unit(const Ref<ClockGearsBattleUnit> &p_unit) {
	if (p_unit.is_valid()) {
		enemies.push_back(p_unit);
	}
}

Array ClockGearsBattleController::get_party_units() const {
	Array arr;
	for (int i = 0; i < party.size(); i++) arr.push_back(party[i]);
	return arr;
}

Array ClockGearsBattleController::get_enemy_units() const {
	Array arr;
	for (int i = 0; i < enemies.size(); i++) arr.push_back(enemies[i]);
	return arr;
}

void ClockGearsBattleController::calculate_initiative() {
	turn_queue.clear();
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();

	Vector<Ref<ClockGearsBattleUnit>> living;
	for (int i = 0; i < party.size(); i++) {
		if (!party[i]->get_is_dead()) living.push_back(party[i]);
	}
	for (int i = 0; i < enemies.size(); i++) {
		if (!enemies[i]->get_is_dead()) living.push_back(enemies[i]);
	}

	for (int i = 0; i < living.size(); i++) {
		int64_t spd = fm ? fm->calculate_action_speed(living[i]->get_agi()) : (living[i]->get_agi() * 10);
		living.write[i]->set_action_speed(spd);
		living.write[i]->set_is_defending(false); // Quitar defensa al iniciar nueva ronda
	}

	// Ordenamiento por velocidad descendente (Mayor AGI primero)
	for (int i = 0; i < living.size(); i++) {
		for (int j = i + 1; j < living.size(); j++) {
			if (living[j]->get_action_speed() > living[i]->get_action_speed()) {
				Ref<ClockGearsBattleUnit> tmp = living[i];
				living.write[i] = living[j];
				living.write[j] = tmp;
			}
		}
	}

	turn_queue = living;
	current_queue_idx = 0;
	phase = PHASE_TURN_INPUT;
}

Array ClockGearsBattleController::get_turn_order() const {
	Array arr;
	if (!turn_queue.is_empty()) {
		for (int i = 0; i < turn_queue.size(); i++) {
			if (turn_queue[i].is_valid() && !turn_queue[i]->get_is_dead()) {
				arr.push_back(turn_queue[i]);
			}
		}
	} else {
		for (int i = 0; i < party.size(); i++) {
			if (party[i].is_valid() && !party[i]->get_is_dead()) arr.push_back(party[i]);
		}
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].is_valid() && !enemies[i]->get_is_dead()) arr.push_back(enemies[i]);
		}
	}
	return arr;
}

Ref<ClockGearsBattleUnit> ClockGearsBattleController::get_active_unit() const {
	if (current_queue_idx >= 0 && current_queue_idx < turn_queue.size()) {
		return turn_queue[current_queue_idx];
	}
	return Ref<ClockGearsBattleUnit>();
}

void ClockGearsBattleController::tick_states(const Ref<ClockGearsBattleUnit> &p_unit) {
	if (!p_unit.is_valid()) return;
	Array sts = p_unit->get_states();
	Array remaining;
	for (int i = 0; i < sts.size(); i++) {
		Dictionary st = sts[i];
		int turns = int(st.get("turns", 1)) - 1;
		if (turns > 0) {
			st["turns"] = turns;
			remaining.push_back(st);
		}
	}
	p_unit->set_states(remaining);
}

bool ClockGearsBattleController::action_attack(int p_target_idx) {
	Ref<ClockGearsBattleUnit> attacker = get_active_unit();
	if (!attacker.is_valid() || !attacker->get_is_actor()) return false;
	if (p_target_idx < 0 || p_target_idx >= enemies.size()) return false;

	Ref<ClockGearsBattleUnit> target = enemies[p_target_idx];
	if (!target.is_valid() || target->get_is_dead()) return false;

	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	bool is_hit = true;
	bool is_crit = false;

	if (fm) {
		double hit_chance = fm->calculate_hit_chance(100, 1.0, 0.05, 1);
		is_hit = (fm->rand_float() <= hit_chance);
		double crit_chance = fm->calculate_critical_chance(attacker->get_luk() * 0.005, 0.0);
		is_crit = (fm->rand_float() <= crit_chance);
	}

	int64_t dmg = 0;
	String formula = "max(a.atk * 4 - b.def * 2, 0)";
	int atk_elem_id = attacker->get_attack_element_id();
	float elem_mult = (atk_elem_id > 0) ? target->get_element_rate(atk_elem_id) : 1.0f;
	last_element_multiplier = elem_mult;
	String elem_info;
	if (atk_elem_id > 0) {
		ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
		String ename = cat ? cat->get_element_name(atk_elem_id) : vformat("Elemento %d", atk_elem_id);
		elem_info = vformat("Elemento: %s (%s)", ename, _format_elem_mult(elem_mult));
	}

	if (is_hit && fm) {
		Dictionary ctx;
		ctx["is_critical"] = is_crit;
		ctx["critical_multiplier"] = 2.0;
		ctx["variance"] = 20.0;
		dmg = fm->calculate_damage(formula, attacker->to_formula_dict(), target->to_formula_dict(), ctx);
		if (target->get_is_defending()) {
			dmg = dmg / 2;
		}
		if (dmg <= 0) {
			dmg = 1;
		}
		if (atk_elem_id > 0) {
			dmg = int64_t(Math::round(double(dmg) * double(elem_mult)));
		}
	}

	attacker->play_slot_animation(attacker->resolve_slot_animation_name("attack"), 0.45f);
	target->set_hp(target->get_hp() - dmg);
	if (target->get_is_dead()) {
		target->play_slot_animation(target->resolve_slot_animation_name("death"), 999.0f);
	} else {
		target->play_slot_animation(target->resolve_slot_animation_name("damage"), 0.45f);
	}
	_log_hit(attacker->get_name(), attacker->get_atk(), target->get_name(), target->get_def(), String::utf8("Ataque"), formula, dmg, is_crit, is_hit, elem_info);

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		am->trigger_animation(nullptr, ANIM_TRIGGER_ATTACK, 0, attacker->get_trigger_table());
		if (target->get_is_dead()) {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DEATH, 0, target->get_trigger_table());
		} else {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DAMAGE, 0, target->get_trigger_table());
		}
	}

	if (check_battle_end()) {
		return true;
	}

	advance_turn();
	return true;
}

bool ClockGearsBattleController::action_skill(int p_skill_id, int p_target_idx) {
	Ref<ClockGearsBattleUnit> attacker = get_active_unit();
	if (!attacker.is_valid() || !attacker->get_is_actor()) return false;
	if (p_target_idx < 0 || p_target_idx >= enemies.size()) return false;

	Ref<ClockGearsBattleUnit> target = enemies[p_target_idx];
	if (!target.is_valid() || target->get_is_dead()) return false;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary sk = dm ? dm->get_record_by_id(CAT_SKILLS, p_skill_id) : Dictionary();
	if (sk.is_empty() && dm) {
		Array all_sk = dm->get_category_array(CAT_SKILLS);
		for (int i = 0; i < all_sk.size(); i++) {
			Dictionary sd = all_sk[i];
			if (int(sd.get("id", i + 1)) == p_skill_id) {
				sk = sd;
				break;
			}
		}
	}

	int mp_cost = int(sk.get("mp_cost", 10));
	if (attacker->get_mp() < mp_cost) {
		return false; // MP insuficiente
	}
	attacker->set_mp(attacker->get_mp() - mp_cost);

	String skill_name = sk.get("name", String::utf8("Habilidad"));
	String formula = sk.get("formula", "max(a.mat * 3 - b.mdf * 1.5, 0)");
	float variance = float(sk.get("variance", 20.0f));

	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	String elem_str = String(sk.get("element_key", sk.get("element", ""))).strip_edges();
	int elem_id = int(sk.get("element_id", sk.get("secondary_type", 0)));
	if (!elem_str.is_empty() && cat) {
		int rid = cat->resolve_element_id(elem_str);
		if (rid > 0) elem_id = rid;
	}
	float elem_mult = (elem_id > 0 || !elem_str.is_empty()) ? target->get_element_rate(elem_id, elem_str) : 1.0f;
	last_element_multiplier = elem_mult;
	String elem_info;
	if (elem_id > 0 || !elem_str.is_empty()) {
		String ename = (cat && elem_id > 0) ? cat->get_element_name(elem_id) : elem_str;
		if (ename.is_empty()) ename = vformat("Elemento %d", elem_id);
		elem_info = vformat("Elemento: %s (%s)", ename, _format_elem_mult(elem_mult));
	}

	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	bool is_hit = true;
	bool is_crit = false;

	if (fm) {
		double hit_chance = fm->calculate_hit_chance(int(sk.get("success_rate", 100)), 1.0, 0.05, 2);
		is_hit = (fm->rand_float() <= hit_chance);
		double crit_chance = bool(sk.get("critical", false)) ? fm->calculate_critical_chance(attacker->get_luk() * 0.01, 0.0) : 0.0;
		is_crit = (fm->rand_float() <= crit_chance);
	}

	int64_t dmg = 0;
	if (is_hit && fm) {
		Dictionary ctx;
		ctx["is_critical"] = is_crit;
		ctx["critical_multiplier"] = 2.0;
		ctx["variance"] = variance;
		dmg = fm->calculate_damage(formula, attacker->to_formula_dict(), target->to_formula_dict(), ctx);
		if (target->get_is_defending()) {
			dmg = dmg / 2;
		}
		if (dmg <= 0) {
			dmg = 1;
		}
		if (elem_id > 0 || !elem_str.is_empty()) {
			dmg = int64_t(Math::round(double(dmg) * double(elem_mult)));
		}
	}

	int fx_anim_id = int(sk.get("attack_anim_id", sk.get("animation_id", 0)));
	if (fx_anim_id > 0) {
		target->trigger_target_effect_anim(fx_anim_id, 0.55f);
	}

	attacker->play_slot_animation(attacker->resolve_slot_animation_name("cast", elem_id, elem_str), 0.45f);
	target->set_hp(target->get_hp() - dmg);
	if (target->get_is_dead()) {
		target->play_slot_animation(target->resolve_slot_animation_name("death"), 999.0f);
	} else {
		target->play_slot_animation(target->resolve_slot_animation_name("damage"), 0.45f);
	}
	_log_hit(attacker->get_name(), attacker->get_mat(), target->get_name(), target->get_mdf(), skill_name, formula, dmg, is_crit, is_hit, elem_info);

	// Aplicar efectos de estado
	Array effects = sk.get("effects", Array());
	for (int i = 0; i < effects.size(); i++) {
		Dictionary ef = effects[i];
		int ef_code = int(ef.get("code", 0));
		if (ef_code == 21) { // Añadir estado
			int st_id = int(ef.get("data_id", 1));
			target->apply_state(st_id, "Estado", 3);
		}
	}

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		am->trigger_animation(nullptr, ANIM_TRIGGER_SKILL, p_skill_id, attacker->get_trigger_table());
		if (target->get_is_dead()) {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DEATH, 0, target->get_trigger_table());
		} else {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DAMAGE, 0, target->get_trigger_table());
		}
	}

	if (check_battle_end()) {
		return true;
	}

	advance_turn();
	return true;
}

bool ClockGearsBattleController::action_item(const String &p_item_name, int p_target_idx) {
	Ref<ClockGearsBattleUnit> attacker = get_active_unit();
	if (!attacker.is_valid() || !attacker->get_is_actor()) return false;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->has_item(p_item_name, 1)) {
		return false;
	}

	// Por defecto los objetos curan o asisten a aliados (party)
	Ref<ClockGearsBattleUnit> target;
	if (p_target_idx >= 0 && p_target_idx < party.size()) {
		target = party[p_target_idx];
	} else if (!party.is_empty()) {
		target = party[0];
	}
	if (!target.is_valid()) return false;

	dm->change_item(p_item_name, -1);

	// Efecto estándar: Curar 150 HP
	int heal = 150;
	target->set_hp(target->get_hp() + heal);

	String entry = vformat("[Batalla] %s usa %s en %s -> ¡Recupera %d HP!", attacker->get_name(), p_item_name, target->get_name(), heal);
	print_line(entry);
	log_history.push_back(entry);
	ClockGearsDebugOverlay::log_battle(entry);

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		am->trigger_animation(nullptr, ANIM_TRIGGER_ITEM, 0, attacker->get_trigger_table());
	}

	advance_turn();
	return true;
}

bool ClockGearsBattleController::action_defend() {
	Ref<ClockGearsBattleUnit> attacker = get_active_unit();
	if (!attacker.is_valid() || !attacker->get_is_actor()) return false;

	attacker->set_is_defending(true);
	String entry = vformat("[Batalla] %s adopta una postura defensiva.", attacker->get_name());
	print_line(entry);
	log_history.push_back(entry);
	ClockGearsDebugOverlay::log_battle(entry);

	advance_turn();
	return true;
}

bool ClockGearsBattleController::action_flee() {
	if (!can_escape) {
		String entry = String::utf8("[Batalla] ¡No puedes huir de esta batalla!");
		print_line(entry);
		log_history.push_back(entry);
		ClockGearsDebugOverlay::log_battle(entry);
		return false;
	}

	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	double party_agi = 0.0;
	int p_cnt = 0;
	for (int i = 0; i < party.size(); i++) {
		if (!party[i]->get_is_dead()) {
			party_agi += party[i]->get_agi();
			p_cnt++;
		}
	}
	if (p_cnt > 0) party_agi /= p_cnt;

	double enemy_agi = 0.0;
	int e_cnt = 0;
	for (int i = 0; i < enemies.size(); i++) {
		if (!enemies[i]->get_is_dead()) {
			enemy_agi += enemies[i]->get_agi();
			e_cnt++;
		}
	}
	if (e_cnt > 0) enemy_agi /= e_cnt;

	double escape_ratio = fm ? fm->calculate_escape_ratio(party_agi, enemy_agi) : 0.5;
	bool success = fm ? (fm->rand_float() <= escape_ratio) : true;

	if (success) {
		battle_result = RESULT_ESCAPED;
		String entry = String::utf8("[Batalla] ¡El grupo ha escapado con éxito!");
		print_line(entry);
		log_history.push_back(entry);
		ClockGearsDebugOverlay::log_battle(entry);
		return true;
	} else {
		String entry = String::utf8("[Batalla] ¡No pudiste huir!");
		print_line(entry);
		log_history.push_back(entry);
		ClockGearsDebugOverlay::log_battle(entry);
		advance_turn();
		return false;
	}
}

void ClockGearsBattleController::execute_enemy_turn(const Ref<ClockGearsBattleUnit> &p_enemy) {
	if (!p_enemy.is_valid() || p_enemy->get_is_dead()) return;

	// Elegir un objetivo vivo del grupo del jugador
	Vector<Ref<ClockGearsBattleUnit>> living_party;
	for (int i = 0; i < party.size(); i++) {
		if (!party[i]->get_is_dead()) living_party.push_back(party[i]);
	}
	if (living_party.is_empty()) return;

	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();

	int target_idx = fm ? fm->rand_range_int(0, living_party.size() - 1) : 0;
	Ref<ClockGearsBattleUnit> target = living_party[target_idx];

	// Evaluar Patrones de Acción del Enemigo
	Array actions = p_enemy->get_actions();
	int chosen_skill_id = 0;
	int best_rating = -1;
	Dictionary chosen_skill;

	for (int i = 0; i < actions.size(); i++) {
		if (actions[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary act = actions[i];
		int sk_id = int(act.get("skill_id", 0));
		int cond_type = int(act.get("cond_type", CG_AI_COND_ALWAYS));
		int cond_a = int(act.get("cond_a", 0));
		int cond_b = int(act.get("cond_b", 100));
		int rating = int(act.get("rating", 5));

		bool cond_ok = false;
		switch (cond_type) {
			case CG_AI_COND_ALWAYS:
				cond_ok = true;
				break;
			case CG_AI_COND_TURN:
				if (cond_a <= 0) {
					cond_ok = true;
				} else if (cond_b <= 0) {
					cond_ok = (round_counter == cond_a);
				} else {
					cond_ok = (round_counter >= cond_a) && (((round_counter - cond_a) % cond_b) == 0);
				}
				break;
			case CG_AI_COND_HP_RANGE: {
				double hp_pct = (p_enemy->get_max_hp() > 0) ? (double(p_enemy->get_hp()) * 100.0 / double(p_enemy->get_max_hp())) : 100.0;
				// Soporta "Vida por debajo de X%" (guardado en cond_a o en cond_b con cond_a=0) y rango [cond_a, cond_b]
				if (cond_a > 0 && (cond_b <= 0 || cond_b == 100)) {
					cond_ok = (hp_pct <= double(cond_a));
				} else {
					cond_ok = (hp_pct >= double(cond_a) && hp_pct <= double(cond_b));
				}
			} break;
			case CG_AI_COND_MP_RANGE: {
				double mp_pct = (p_enemy->get_max_mp() > 0) ? (double(p_enemy->get_mp()) * 100.0 / double(p_enemy->get_max_mp())) : 100.0;
				cond_ok = (mp_pct >= double(cond_a) && mp_pct <= double(cond_b));
			} break;
			case CG_AI_COND_STATE:
				cond_ok = p_enemy->has_state(cond_a);
				break;
			case CG_AI_COND_SWITCH:
				if (dm) {
					String sw_key = String(act.get("cond_switch", "")).strip_edges();
					cond_ok = dm->get_switch(itos(cond_a)) || (!sw_key.is_empty() && dm->get_switch(sw_key));
				}
				break;
			default:
				cond_ok = true;
				break;
		}

		if (!cond_ok || sk_id <= 0) {
			continue;
		}

		Dictionary sk_rec = dm ? dm->get_record_by_id(CAT_SKILLS, sk_id) : Dictionary();
		if (sk_rec.is_empty() && dm) {
			Array all_sk = dm->get_category_array(CAT_SKILLS);
			for (int s = 0; s < all_sk.size(); s++) {
				Dictionary sd = all_sk[s];
				if (int(sd.get("id", s + 1)) == sk_id) {
					sk_rec = sd;
					break;
				}
			}
		}
		if (sk_rec.is_empty()) {
			continue;
		}
		int mp_cost = int(sk_rec.get("mp_cost", 0));
		if (p_enemy->get_mp() < mp_cost) {
			continue;
		}

		if (rating > best_rating) {
			best_rating = rating;
			chosen_skill_id = sk_id;
			chosen_skill = sk_rec;
		}
	}

	if (chosen_skill_id > 0 && !chosen_skill.is_empty()) {
		int mp_cost = int(chosen_skill.get("mp_cost", 0));
		p_enemy->set_mp(p_enemy->get_mp() - mp_cost);

		String skill_name = chosen_skill.get("name", String::utf8("Habilidad"));
		String formula = chosen_skill.get("formula", "max(a.mat * 3 - b.mdf * 1.5, 0)");
		float variance = float(chosen_skill.get("variance", 20.0f));
		int scope = int(chosen_skill.get("type", chosen_skill.get("scope", 1)));

		String elem_str = String(chosen_skill.get("element_key", chosen_skill.get("element", ""))).strip_edges();
		int elem_id = int(chosen_skill.get("element_id", chosen_skill.get("secondary_type", 0)));
		if (!elem_str.is_empty() && cat) {
			int rid = cat->resolve_element_id(elem_str);
			if (rid > 0) elem_id = rid;
		}

		// Determinar si es habilidad de apoyo/buff (scope aliado/usuario o efecto de buff sin daño)
		Array effects = chosen_skill.get("effects", Array());
		bool is_buff_skill = (scope >= 7 && scope <= 11);
		if (!is_buff_skill && (formula.strip_edges().is_empty() || formula.strip_edges() == "0")) {
			for (int e = 0; e < effects.size(); e++) {
				Dictionary ef = effects[e];
				int c = int(ef.get("code", 0));
				if (c == CG_EFFECT_ADD_BUFF || c == CG_EFFECT_RECOVER_HP || c == CG_EFFECT_RECOVER_MP) {
					is_buff_skill = true;
					break;
				}
			}
		}

		if (is_buff_skill) {
			p_enemy->play_slot_animation(p_enemy->resolve_slot_animation_name("buff"), 0.6f);
			int fx_anim_id = int(chosen_skill.get("attack_anim_id", chosen_skill.get("animation_id", 0)));
			if (fx_anim_id > 0) {
				p_enemy->trigger_target_effect_anim(fx_anim_id, 0.55f);
			}
			String entry = vformat(String::utf8("[Batalla] %s usa %s."), p_enemy->get_name(), skill_name);
			print_line(entry);
			log_history.push_back(entry);
			last_hit_log = entry;
			ClockGearsDebugOverlay::log_battle(entry);
			emit_signal("hit_executed", entry);
			check_battle_end();
			return;
		}

		// Habilidad ofensiva: animar Lanzamiento por elemento (o primer Lanzamiento, o Ataque)
		if (elem_id > 0 || !elem_str.is_empty() || !p_enemy->get_cast_animations().is_empty()) {
			p_enemy->play_slot_animation(p_enemy->resolve_slot_animation_name("cast", elem_id, elem_str), 0.6f);
		} else {
			p_enemy->play_slot_animation(p_enemy->resolve_slot_animation_name("attack"), 0.6f);
		}

		float elem_mult = (elem_id > 0 || !elem_str.is_empty()) ? target->get_element_rate(elem_id, elem_str) : 1.0f;
		last_element_multiplier = elem_mult;
		String elem_info;
		if (elem_id > 0 || !elem_str.is_empty()) {
			String ename = (cat && elem_id > 0) ? cat->get_element_name(elem_id) : elem_str;
			if (ename.is_empty()) ename = vformat("Elemento %d", elem_id);
			elem_info = vformat("Elemento: %s (%s)", ename, _format_elem_mult(elem_mult));
		}

		bool is_hit = true;
		bool is_crit = false;
		if (fm) {
			double hit_chance = fm->calculate_hit_chance(int(chosen_skill.get("success_rate", 100)), 1.0, 0.05, 2);
			is_hit = (fm->rand_float() <= hit_chance);
			double crit_chance = bool(chosen_skill.get("critical", false)) ? fm->calculate_critical_chance(p_enemy->get_luk() * 0.01, 0.0) : 0.0;
			is_crit = (fm->rand_float() <= crit_chance);
		}

		int64_t dmg = 0;
		if (is_hit && fm) {
			Dictionary ctx;
			ctx["is_critical"] = is_crit;
			ctx["critical_multiplier"] = 2.0;
			ctx["variance"] = variance;
			dmg = fm->calculate_damage(formula, p_enemy->to_formula_dict(), target->to_formula_dict(), ctx);
			if (target->get_is_defending()) {
				dmg = dmg / 2;
			}
			if (dmg <= 0) {
				dmg = 1;
			}
			if (elem_id > 0 || !elem_str.is_empty()) {
				dmg = int64_t(Math::round(double(dmg) * double(elem_mult)));
			}
		}

		int fx_anim_id = int(chosen_skill.get("attack_anim_id", chosen_skill.get("animation_id", 0)));
		if (fx_anim_id > 0) {
			target->trigger_target_effect_anim(fx_anim_id, 0.55f);
		}

		target->set_hp(target->get_hp() - dmg);
		if (target->get_is_dead()) {
			target->play_slot_animation(target->resolve_slot_animation_name("death"), 999.0f);
		} else {
			target->play_slot_animation(target->resolve_slot_animation_name("damage"), 0.45f);
		}

		for (int i = 0; i < effects.size(); i++) {
			Dictionary ef = effects[i];
			if (int(ef.get("code", 0)) == CG_EFFECT_ADD_STATE) {
				target->apply_state(int(ef.get("data_id", 1)), "Estado", 3);
			}
		}

		_log_hit(p_enemy->get_name(), p_enemy->get_mat(), target->get_name(), target->get_mdf(), skill_name, formula, dmg, is_crit, is_hit, elem_info);

		ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
		if (am) {
			am->trigger_animation(nullptr, ANIM_TRIGGER_SKILL, chosen_skill_id, p_enemy->get_trigger_table());
			if (target->get_is_dead()) {
				am->trigger_animation(nullptr, ANIM_TRIGGER_DEATH, 0, target->get_trigger_table());
			} else {
				am->trigger_animation(nullptr, ANIM_TRIGGER_DAMAGE, 0, target->get_trigger_table());
			}
		}

		check_battle_end();
		return;
	}

	// Lista vacía o ninguna condición cumplida -> Ataque básico
	bool is_hit = true;
	bool is_crit = false;
	if (fm) {
		double hit_chance = fm->calculate_hit_chance(100, 1.0, 0.05, 1);
		is_hit = (fm->rand_float() <= hit_chance);
		double crit_chance = fm->calculate_critical_chance(p_enemy->get_luk() * 0.005, 0.0);
		is_crit = (fm->rand_float() <= crit_chance);
	}

	int atk_elem_id = p_enemy->get_attack_element_id();
	float elem_mult = (atk_elem_id > 0) ? target->get_element_rate(atk_elem_id) : 1.0f;
	last_element_multiplier = elem_mult;
	String elem_info;
	if (atk_elem_id > 0) {
		String ename = cat ? cat->get_element_name(atk_elem_id) : vformat("Elemento %d", atk_elem_id);
		elem_info = vformat("Elemento: %s (%s)", ename, _format_elem_mult(elem_mult));
	}

	String formula = "max(a.atk * 4 - b.def * 2, 0)";
	int64_t dmg = 0;
	if (is_hit && fm) {
		Dictionary ctx;
		ctx["is_critical"] = is_crit;
		ctx["critical_multiplier"] = 2.0;
		ctx["variance"] = 20.0;
		dmg = fm->calculate_damage(formula, p_enemy->to_formula_dict(), target->to_formula_dict(), ctx);
		if (target->get_is_defending()) {
			dmg = dmg / 2;
		}
		if (dmg <= 0) {
			dmg = 1;
		}
		if (atk_elem_id > 0) {
			dmg = int64_t(Math::round(double(dmg) * double(elem_mult)));
		}
	}

	p_enemy->play_slot_animation(p_enemy->resolve_slot_animation_name("attack"), 0.55f);
	target->set_hp(target->get_hp() - dmg);
	if (target->get_is_dead()) {
		target->play_slot_animation(target->resolve_slot_animation_name("death"), 999.0f);
	} else {
		target->play_slot_animation(target->resolve_slot_animation_name("damage"), 0.45f);
	}
	_log_hit(p_enemy->get_name(), p_enemy->get_atk(), target->get_name(), target->get_def(), String::utf8("Ataque"), formula, dmg, is_crit, is_hit, elem_info);

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		am->trigger_animation(nullptr, ANIM_TRIGGER_ATTACK, 0, p_enemy->get_trigger_table());
		if (target->get_is_dead()) {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DEATH, 0, target->get_trigger_table());
		} else {
			am->trigger_animation(nullptr, ANIM_TRIGGER_DAMAGE, 0, target->get_trigger_table());
		}
	}

	check_battle_end();
}

void ClockGearsBattleController::start_turn_cycle() {
	if (is_finished()) return;

	while (current_queue_idx < turn_queue.size()) {
		Ref<ClockGearsBattleUnit> cur = turn_queue[current_queue_idx];
		if (!cur.is_valid() || cur->get_is_dead()) {
			current_queue_idx++;
			continue;
		}

		tick_states(cur);

		if (cur->get_is_actor()) {
			phase = PHASE_TURN_INPUT;
			return;
		} else {
			execute_enemy_turn(cur);
			if (is_finished()) return;
			current_queue_idx++;
		}
	}

	// Si se consumió toda la cola de la ronda, calcular siguiente ronda
	round_counter++;
	calculate_initiative();
	start_turn_cycle();
}

bool ClockGearsBattleController::advance_turn() {
	if (is_finished()) return false;
	current_queue_idx++;
	start_turn_cycle();
	return !is_finished();
}

bool ClockGearsBattleController::is_all_enemies_dead() const {
	for (int i = 0; i < enemies.size(); i++) {
		if (!enemies[i]->get_is_dead()) return false;
	}
	return true;
}

bool ClockGearsBattleController::is_all_actors_dead() const {
	for (int i = 0; i < party.size(); i++) {
		if (!party[i]->get_is_dead()) return false;
	}
	return true;
}

void ClockGearsBattleController::process_victory_rewards() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();

	total_exp_reward = 0;
	total_gold_reward = 0;
	dropped_items.clear();
	level_up_messages.clear();

	for (int i = 0; i < enemies.size(); i++) {
		Ref<ClockGearsBattleUnit> en = enemies[i];
		total_exp_reward += en->get_exp_reward();
		total_gold_reward += en->get_gold_reward();

		// Procesar hasta 3 drops
		Array drs = en->get_drops();
		for (int d = 0; d < drs.size() && d < 3; d++) {
			Dictionary dr = drs[d];
			int prob = int(dr.get("prob", 0));
			int item_id = int(dr.get("item_id", 0));
			if (item_id > 0 && prob > 0) {
				bool won = fm ? fm->roll_drop(prob) : true;
				if (won) {
					Dictionary item_rec = dm ? dm->get_record_by_id(CAT_ITEMS, item_id) : Dictionary();
					String iname = item_rec.get("name", vformat("Item_%d", item_id));
					if (dm) dm->change_item(iname, 1);
					dropped_items.push_back(iname);
				}
			}
		}
	}

	if (dm) {
		dm->change_gold(total_gold_reward);
	}

	// Conceder EXP y verificar subidas de nivel
	for (int i = 0; i < party.size(); i++) {
		Ref<ClockGearsBattleUnit> hero = party[i];
		if (!hero->get_is_dead()) {
			hero->set_exp(hero->get_exp() + total_exp_reward);
			if (fm) {
				while (hero->get_exp() >= fm->calculate_exp_for_level(hero->get_level() + 1)) {
					hero->set_level(hero->get_level() + 1);
					hero->set_max_hp(hero->get_max_hp() + 50);
					hero->set_hp(hero->get_hp() + 50);
					hero->set_atk(hero->get_atk() + 3);
					hero->set_def(hero->get_def() + 2);
					level_up_messages.push_back(vformat(String::utf8("¡%s ha subido al Nivel %d!"), hero->get_name(), hero->get_level()));
				}
			}
		}
	}

	String vic_log = vformat(String::utf8("[Batalla] ¡Victoria! +%d EXP, +%d Oro."), total_exp_reward, total_gold_reward);
	print_line(vic_log);
	log_history.push_back(vic_log);
	ClockGearsDebugOverlay::log_battle(vic_log);

	ClockGearsAnimationManager *am = ClockGearsAnimationManager::get_singleton();
	if (am) {
		for (int i = 0; i < party.size(); i++) {
			if (!party[i]->get_is_dead()) {
				am->trigger_animation(nullptr, ANIM_TRIGGER_VICTORY, 0, party[i]->get_trigger_table());
			}
		}
	}
}

bool ClockGearsBattleController::check_battle_end() {
	if (is_all_enemies_dead()) {
		battle_result = RESULT_VICTORY;
		phase = PHASE_FINISHED;
		process_victory_rewards();
		return true;
	}
	if (is_all_actors_dead()) {
		battle_result = can_lose ? RESULT_DEFEAT : RESULT_GAME_OVER;
		phase = PHASE_FINISHED;
		String def_log = can_lose ? String::utf8("[Batalla] El grupo ha sido derrotado...") : String::utf8("[Batalla] GAME OVER: El grupo pereció en combate.");
		print_line(def_log);
		log_history.push_back(def_log);
		ClockGearsDebugOverlay::log_battle(def_log);
		return true;
	}
	return false;
}

ClockGearsBattleController::ClockGearsBattleController() {
}

ClockGearsBattleController::~ClockGearsBattleController() {
}
