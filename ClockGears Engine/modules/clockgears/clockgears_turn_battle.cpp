#include "clockgears_turn_battle.h"

void ClockGearsTurnBattle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start_battle", "party_battlers", "enemy_battlers"), &ClockGearsTurnBattle::start_battle);
	ClassDB::bind_method(D_METHOD("add_party_member", "battler"), &ClockGearsTurnBattle::add_party_member);
	ClassDB::bind_method(D_METHOD("add_enemy", "enemy"), &ClockGearsTurnBattle::add_enemy);
	ClassDB::bind_method(D_METHOD("get_state"), &ClockGearsTurnBattle::get_state);

	ADD_SIGNAL(MethodInfo("battle_finished", PropertyInfo(Variant::BOOL, "victory"), PropertyInfo(Variant::INT, "exp_earned"), PropertyInfo(Variant::INT, "gold_earned")));
	ADD_SIGNAL(MethodInfo("battle_escaped"));
}

void ClockGearsTurnBattle::_setup_ui() {
	battle_panel = memnew(Panel);
	battle_panel->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(battle_panel);

	VBoxContainer *main_layout = memnew(VBoxContainer);
	main_layout->set_anchors_preset(Control::PRESET_FULL_RECT);
	main_layout->set_offset(SIDE_LEFT, 20);
	main_layout->set_offset(SIDE_RIGHT, -20);
	main_layout->set_offset(SIDE_TOP, 20);
	main_layout->set_offset(SIDE_BOTTOM, -20);
	main_layout->add_theme_constant_override("separation", 16);
	battle_panel->add_child(main_layout);

	// Battle message banner
	log_label = memnew(Label(String::utf8("¡Ha comenzado la batalla!")));
	log_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	log_label->set_custom_minimum_size(Size2(0, 40));
	log_label->add_theme_font_size_override("font_size", 18);
	main_layout->add_child(log_label);

	// Enemies area (Top / Middle)
	enemies_container = memnew(HBoxContainer);
	enemies_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	enemies_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	enemies_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	main_layout->add_child(enemies_container);

	// Party area (Bottom)
	party_container = memnew(HBoxContainer);
	party_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	party_container->set_custom_minimum_size(Size2(0, 100));
	party_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	main_layout->add_child(party_container);

	// Actions bar
	actions_bar = memnew(HBoxContainer);
	actions_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	actions_bar->set_custom_minimum_size(Size2(0, 48));
	actions_bar->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	actions_bar->add_theme_constant_override("separation", 12);
	main_layout->add_child(actions_bar);

	btn_attack = memnew(Button(String::utf8("⚔️ Atacar")));
	btn_attack->set_custom_minimum_size(Size2(120, 40));
	btn_attack->connect("pressed", callable_mp(this, &ClockGearsTurnBattle::_on_attack_pressed));
	actions_bar->add_child(btn_attack);

	btn_skill = memnew(Button(String::utf8("✨ Habilidad")));
	btn_skill->set_custom_minimum_size(Size2(120, 40));
	btn_skill->connect("pressed", callable_mp(this, &ClockGearsTurnBattle::_on_skill_pressed));
	actions_bar->add_child(btn_skill);

	btn_defend = memnew(Button(String::utf8("🛡️ Defender")));
	btn_defend->set_custom_minimum_size(Size2(120, 40));
	btn_defend->connect("pressed", callable_mp(this, &ClockGearsTurnBattle::_on_defend_pressed));
	actions_bar->add_child(btn_defend);

	btn_item = memnew(Button(String::utf8("🧪 Objeto")));
	btn_item->set_custom_minimum_size(Size2(120, 40));
	btn_item->connect("pressed", callable_mp(this, &ClockGearsTurnBattle::_on_item_pressed));
	actions_bar->add_child(btn_item);

	btn_flee = memnew(Button(String::utf8("🏃 Huir")));
	btn_flee->set_custom_minimum_size(Size2(120, 40));
	btn_flee->connect("pressed", callable_mp(this, &ClockGearsTurnBattle::_on_flee_pressed));
	actions_bar->add_child(btn_flee);
}

void ClockGearsTurnBattle::_refresh_displays() {
	// Rebuild enemy cards
	while (enemies_container->get_child_count() > 0) {
		Node *c = enemies_container->get_child(0);
		enemies_container->remove_child(c);
		memdelete(c);
	}

	for (int i = 0; i < enemies.size(); i++) {
		ClockGearsBattler *en = enemies[i];
		if (!en) continue;

		VBoxContainer *card = memnew(VBoxContainer);
		card->set_custom_minimum_size(Size2(160, 120));
		card->add_theme_constant_override("separation", 6);

		Label *nm = memnew(Label(en->get_battler_name()));
		nm->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		card->add_child(nm);

		ProgressBar *hp_bar = memnew(ProgressBar);
		hp_bar->set_max(en->get_max_hp());
		hp_bar->set_value(en->get_current_hp());
		hp_bar->set_custom_minimum_size(Size2(140, 18));
		card->add_child(hp_bar);

		Label *hp_txt = memnew(Label(vformat("%d / %d PV", en->get_current_hp(), en->get_max_hp())));
		hp_txt->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		card->add_child(hp_txt);

		if (!en->is_alive()) {
			card->set_modulate(Color(0.4f, 0.4f, 0.4f, 0.5f));
		}
		enemies_container->add_child(card);
	}

	// Rebuild party cards
	while (party_container->get_child_count() > 0) {
		Node *c = party_container->get_child(0);
		party_container->remove_child(c);
		memdelete(c);
	}

	for (int i = 0; i < party_members.size(); i++) {
		ClockGearsBattler *hero = party_members[i];
		if (!hero) continue;

		VBoxContainer *card = memnew(VBoxContainer);
		card->set_custom_minimum_size(Size2(180, 90));
		card->add_theme_constant_override("separation", 4);

		Label *nm = memnew(Label(vformat("%s (Nv. %d)", hero->get_battler_name(), hero->get_level())));
		card->add_child(nm);

		ProgressBar *hp_bar = memnew(ProgressBar);
		hp_bar->set_max(hero->get_max_hp());
		hp_bar->set_value(hero->get_current_hp());
		hp_bar->set_custom_minimum_size(Size2(160, 16));
		card->add_child(hp_bar);

		ProgressBar *mp_bar = memnew(ProgressBar);
		mp_bar->set_max(hero->get_max_mp());
		mp_bar->set_value(hero->get_current_mp());
		mp_bar->set_custom_minimum_size(Size2(160, 12));
		card->add_child(mp_bar);

		if (i == current_actor_idx) {
			card->set_modulate(Color(1.0f, 1.0f, 0.5f, 1.0f)); // Active actor highlight
		}

		party_container->add_child(card);
	}
}

void ClockGearsTurnBattle::start_battle(const Array &p_party, const Array &p_enemies) {
	party_members.clear();
	enemies.clear();

	for (int i = 0; i < p_party.size(); i++) {
		ClockGearsBattler *b = Object::cast_to<ClockGearsBattler>(p_party[i]);
		if (b) {
			party_members.push_back(b);
		}
	}

	for (int i = 0; i < p_enemies.size(); i++) {
		ClockGearsBattler *b = Object::cast_to<ClockGearsBattler>(p_enemies[i]);
		if (b) {
			enemies.push_back(b);
		}
	}

	// Create fallback demo battlers if empty
	if (party_members.is_empty()) {
		ClockGearsBattler *hero = memnew(ClockGearsBattler);
		hero->set_battler_name("Guerrero");
		hero->set_level(5);
		hero->set_is_enemy(false);
		add_child(hero);
		party_members.push_back(hero);
	}

	if (enemies.is_empty()) {
		ClockGearsBattler *slime = memnew(ClockGearsBattler);
		slime->set_battler_name("Slime Azul");
		slime->set_max_hp(180);
		slime->set_current_hp(180);
		slime->set_atk(18);
		slime->set_def(12);
		slime->set_is_enemy(true);
		// Slime is weak to Fire (150%) and resistant to Water (50%)
		slime->set_element_affinity(ELEMENT_FIRE, AFFINITY_WEAK);
		slime->set_element_affinity(ELEMENT_WATER, AFFINITY_RESISTANT);
		add_child(slime);
		enemies.push_back(slime);
	}

	current_state = STATE_COMMAND_INPUT;
	current_actor_idx = 0;
	target_enemy_idx = 0;

	_refresh_displays();
	_start_actor_turn();
}

void ClockGearsTurnBattle::add_party_member(ClockGearsBattler *p_battler) {
	if (p_battler) {
		party_members.push_back(p_battler);
	}
}

void ClockGearsTurnBattle::add_enemy(ClockGearsBattler *p_enemy) {
	if (p_enemy) {
		enemies.push_back(p_enemy);
	}
}

void ClockGearsTurnBattle::_start_actor_turn() {
	if (party_members.is_empty()) {
		return;
	}

	// Find next alive actor
	while (current_actor_idx < party_members.size() && !party_members[current_actor_idx]->is_alive()) {
		current_actor_idx++;
	}

	if (current_actor_idx >= party_members.size()) {
		// All party members entered commands, execute enemy turns
		_execute_enemy_turns();
		return;
	}

	ClockGearsBattler *actor = party_members[current_actor_idx];
	log_label->set_text(vformat(String::utf8("Turno de %s. ¿Qué deseas hacer?"), actor->get_battler_name()));
	_refresh_displays();
}

void ClockGearsTurnBattle::_on_attack_pressed() {
	if (party_members.is_empty() || enemies.is_empty()) return;

	ClockGearsBattler *attacker = party_members[current_actor_idx];
	// Find alive enemy
	ClockGearsBattler *target = nullptr;
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i]->is_alive()) {
			target = enemies[i];
			break;
		}
	}

	if (!target) {
		_check_battle_end();
		return;
	}

	// Calculate damage with Physical element
	Dictionary res = ClockGearsDamageEngine::evaluate_damage(
			attacker, target, "a.atk * 4 - b.def * 2", ELEMENT_PHYSICAL, 20.0f, 0.08f, 2.0f);

	int dmg = (int)res["damage"];
	bool crit = (bool)res["is_critical"];
	target->take_damage(dmg, crit, ELEMENT_PHYSICAL);

	// Spawn particles and popup in UI
	Vector2 impact_pos = enemies_container->get_global_position() + Vector2(200, 60);
	ClockGearsFXManager::spawn_impact_fx(this, impact_pos, dmg, crit, ELEMENT_PHYSICAL);

	log_label->set_text(vformat(String::utf8("%s ataca a %s causando %d de daño%s!"),
			attacker->get_battler_name(), target->get_battler_name(), dmg, crit ? String::utf8(" (¡GOLPE CRÍTICO!)") : ""));

	_refresh_displays();

	if (_check_battle_end()) return;

	current_actor_idx++;
	_start_actor_turn();
}

void ClockGearsTurnBattle::_on_skill_pressed() {
	if (party_members.is_empty() || enemies.is_empty()) return;

	ClockGearsBattler *attacker = party_members[current_actor_idx];
	if (attacker->get_current_mp() < 10) {
		log_label->set_text(String::utf8("¡No tienes suficiente PM!"));
		return;
	}
	attacker->consume_mp(10);

	ClockGearsBattler *target = nullptr;
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i]->is_alive()) {
			target = enemies[i];
			break;
		}
	}
	if (!target) return;

	// Magic Skill: Bola de Fuego (Element FIRE)
	Dictionary res = ClockGearsDamageEngine::evaluate_damage(
			attacker, target, "a.mat * 3 + 35", ELEMENT_FIRE, 15.0f, 0.12f, 2.5f);

	int dmg = (int)res["damage"];
	bool crit = (bool)res["is_critical"];
	bool weak = (bool)res["is_weak"];
	target->take_damage(dmg, crit, ELEMENT_FIRE);

	Vector2 impact_pos = enemies_container->get_global_position() + Vector2(200, 60);
	ClockGearsFXManager::spawn_impact_fx(this, impact_pos, dmg, crit, ELEMENT_FIRE);

	log_label->set_text(vformat(String::utf8("¡%s lanza Bola de Fuego a %s causando %d de daño! %s"),
			attacker->get_battler_name(), target->get_battler_name(), dmg, weak ? String::utf8("(¡SUPER EFECTIVO!)") : ""));

	_refresh_displays();

	if (_check_battle_end()) return;

	current_actor_idx++;
	_start_actor_turn();
}

void ClockGearsTurnBattle::_on_defend_pressed() {
	ClockGearsBattler *actor = party_members[current_actor_idx];
	log_label->set_text(vformat(String::utf8("%s asume una posición defensiva."), actor->get_battler_name()));

	current_actor_idx++;
	_start_actor_turn();
}

void ClockGearsTurnBattle::_on_item_pressed() {
	ClockGearsBattler *actor = party_members[current_actor_idx];
	actor->heal_hp(150);
	log_label->set_text(vformat(String::utf8("%s usa una Poción y recupera 150 PV."), actor->get_battler_name()));

	Vector2 impact_pos = party_container->get_global_position() + Vector2(100, 40);
	ClockGearsFXManager::spawn_impact_fx(this, impact_pos, -150, false, ELEMENT_PHYSICAL, false, true);

	_refresh_displays();

	current_actor_idx++;
	_start_actor_turn();
}

void ClockGearsTurnBattle::_on_flee_pressed() {
	log_label->set_text(String::utf8("¡Escapaste con éxito!"));
	emit_signal("battle_escaped");
	queue_free();
}

void ClockGearsTurnBattle::_execute_enemy_turns() {
	for (int i = 0; i < enemies.size(); i++) {
		ClockGearsBattler *en = enemies[i];
		if (!en || !en->is_alive()) continue;

		// Select random alive party member
		ClockGearsBattler *target = nullptr;
		for (int p = 0; p < party_members.size(); p++) {
			if (party_members[p]->is_alive()) {
				target = party_members[p];
				break;
			}
		}
		if (!target) break;

		Dictionary res = ClockGearsDamageEngine::evaluate_damage(
				en, target, "a.atk * 3 - b.def * 1.5", ELEMENT_PHYSICAL, 20.0f, 0.05f, 2.0f);

		int dmg = (int)res["damage"];
		bool crit = (bool)res["is_critical"];
		target->take_damage(dmg, crit, ELEMENT_PHYSICAL);

		Vector2 impact_pos = party_container->get_global_position() + Vector2(150, 40);
		ClockGearsFXManager::spawn_impact_fx(this, impact_pos, dmg, crit, ELEMENT_PHYSICAL);

		log_label->set_text(vformat(String::utf8("%s ataca a %s causando %d de daño!"),
				en->get_battler_name(), target->get_battler_name(), dmg));
	}

	_refresh_displays();

	if (_check_battle_end()) return;

	// Reset to first alive actor for next round
	current_actor_idx = 0;
	_start_actor_turn();
}

bool ClockGearsTurnBattle::_check_battle_end() {
	bool any_enemy_alive = false;
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i]->is_alive()) {
			any_enemy_alive = true;
			break;
		}
	}

	if (!any_enemy_alive) {
		current_state = STATE_VICTORY;
		int total_exp = 50 * enemies.size();
		int total_gold = 75 * enemies.size();
		for (int i = 0; i < party_members.size(); i++) {
			party_members[i]->gain_exp(total_exp);
		}
		log_label->set_text(vformat(String::utf8("¡Victoria! Conseguiste %d EXP y %d de Oro."), total_exp, total_gold));
		emit_signal("battle_finished", true, total_exp, total_gold);
		return true;
	}

	bool any_party_alive = false;
	for (int i = 0; i < party_members.size(); i++) {
		if (party_members[i]->is_alive()) {
			any_party_alive = true;
			break;
		}
	}

	if (!any_party_alive) {
		current_state = STATE_DEFEAT;
		log_label->set_text(String::utf8("¡El grupo ha sido derrotado!"));
		emit_signal("battle_finished", false, 0, 0);
		return true;
	}

	return false;
}

ClockGearsTurnBattle::ClockGearsTurnBattle() {
	_setup_ui();
}
