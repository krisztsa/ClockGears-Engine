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

	ADD_SIGNAL(MethodInfo("hit_executed", PropertyInfo(Variant::STRING, "log")));

	BIND_CONSTANT(RESULT_NONE);
	BIND_CONSTANT(RESULT_VICTORY);
	BIND_CONSTANT(RESULT_DEFEAT);
	BIND_CONSTANT(RESULT_ESCAPED);
	BIND_CONSTANT(RESULT_GAME_OVER);
}

void ClockGearsBattleController::_log_hit(const String &p_attacker, int p_a_atk, const String &p_target, int p_b_def, const String &p_action, const String &p_formula, int64_t p_damage, bool p_crit, bool p_hit) {
	String entry;
	if (!p_hit) {
		entry = vformat(String::utf8("[Batalla] %s (ATK: %d) usa %s en %s (DEF: %d) -> ¡FALLÓ!"), p_attacker, p_a_atk, p_action, p_target, p_b_def);
	} else {
		entry = vformat(String::utf8("[Batalla] %s (ATK: %d) -> %s (DEF: %d) | Fórmula: '%s' | Daño final: %d (Crítico: %s)"),
				p_attacker, p_a_atk, p_target, p_b_def, p_formula, p_damage, p_crit ? "Sí" : "No");
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
			u->set_graphic_path(d.get("graphic_path", ""));
			if (d.has("sprite_data")) {
				u->set_sprite_data_dict(d.get("sprite_data", Dictionary()));
			} else if (d.has("animations")) {
				Dictionary sp;
				sp["animations"] = d.get("animations", Dictionary());
				sp["cols"] = int(d.get("battler_frames", 4));
				sp["rows"] = 1;
				u->set_sprite_data_dict(sp);
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

	Array troop_members = troop_rec.get("members", Array());
	if (troop_members.is_empty() && troop_rec.has("type") && int(troop_rec["type"]) > 0) {
		Dictionary m;
		m["enemy_id"] = troop_rec["type"];
		m["x"] = 300;
		m["y"] = 150;
		troop_members.push_back(m);
	}

	if (!troop_members.is_empty() && dm) {
		for (int i = 0; i < troop_members.size(); i++) {
			Variant item = troop_members[i];
			int e_id = 1;
			int pos_x = 200 + i * 160;
			int pos_y = 150;
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
			en->set_graphic_path(en_data.get("graphic_path", ""));
			if (en_data.has("sprite_data")) {
				en->set_sprite_data_dict(en_data.get("sprite_data", Dictionary()));
			} else if (en_data.has("animations")) {
				Dictionary sp;
				sp["animations"] = en_data.get("animations", Dictionary());
				sp["cols"] = int(en_data.get("battler_frames", 4));
				sp["rows"] = 1;
				en->set_sprite_data_dict(sp);
			}
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
	}

	target->set_hp(target->get_hp() - dmg);
	_log_hit(attacker->get_name(), attacker->get_atk(), target->get_name(), target->get_def(), String::utf8("Ataque"), formula, dmg, is_crit, is_hit);

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

	int mp_cost = int(sk.get("mp_cost", 10));
	if (attacker->get_mp() < mp_cost) {
		return false; // MP insuficiente
	}
	attacker->set_mp(attacker->get_mp() - mp_cost);

	String skill_name = sk.get("name", String::utf8("Habilidad"));
	String formula = sk.get("formula", "max(a.mat * 3 - b.mdf * 1.5, 0)");
	float variance = float(sk.get("variance", 20.0f));

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
	}

	target->set_hp(target->get_hp() - dmg);
	_log_hit(attacker->get_name(), attacker->get_atk(), target->get_name(), target->get_def(), skill_name, formula, dmg, is_crit, is_hit);

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
	int target_idx = fm ? fm->rand_range_int(0, living_party.size() - 1) : 0;
	Ref<ClockGearsBattleUnit> target = living_party[target_idx];

	bool is_hit = true;
	bool is_crit = false;
	if (fm) {
		double hit_chance = fm->calculate_hit_chance(100, 1.0, 0.05, 1);
		is_hit = (fm->rand_float() <= hit_chance);
		double crit_chance = fm->calculate_critical_chance(p_enemy->get_luk() * 0.005, 0.0);
		is_crit = (fm->rand_float() <= crit_chance);
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
	}

	target->set_hp(target->get_hp() - dmg);
	_log_hit(p_enemy->get_name(), p_enemy->get_atk(), target->get_name(), target->get_def(), String::utf8("Golpe"), formula, dmg, is_crit, is_hit);

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
