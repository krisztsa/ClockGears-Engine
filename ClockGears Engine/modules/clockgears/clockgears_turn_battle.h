#pragma once

#include "clockgears_types.h"
#include "clockgears_battler.h"
#include "clockgears_damage_engine.h"
#include "clockgears_fx_manager.h"
#include "scene/main/canvas_layer.h"
#include "scene/gui/panel.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/progress_bar.h"

class ClockGearsTurnBattle : public CanvasLayer {
	GDCLASS(ClockGearsTurnBattle, CanvasLayer);

public:
	enum BattleState {
		STATE_IDLE = 0,
		STATE_COMMAND_INPUT = 1,
		STATE_EXECUTE_TURN = 2,
		STATE_VICTORY = 3,
		STATE_DEFEAT = 4,
	};

private:
	BattleState current_state = STATE_IDLE;
	Vector<ClockGearsBattler *> party_members;
	Vector<ClockGearsBattler *> enemies;

	int current_actor_idx = 0;
	int target_enemy_idx = 0;

	// UI controls
	Panel *battle_panel = nullptr;
	Label *log_label = nullptr;
	HBoxContainer *enemies_container = nullptr;
	HBoxContainer *party_container = nullptr;
	HBoxContainer *actions_bar = nullptr;

	Button *btn_attack = nullptr;
	Button *btn_skill = nullptr;
	Button *btn_defend = nullptr;
	Button *btn_item = nullptr;
	Button *btn_flee = nullptr;

	void _setup_ui();
	void _refresh_displays();
	void _start_actor_turn();
	void _execute_enemy_turns();
	bool _check_battle_end();

	void _on_attack_pressed();
	void _on_skill_pressed();
	void _on_defend_pressed();
	void _on_item_pressed();
	void _on_flee_pressed();

protected:
	static void _bind_methods();

public:
	void start_battle(const Array &p_party_battlers, const Array &p_enemy_battlers);
	void add_party_member(ClockGearsBattler *p_battler);
	void add_enemy(ClockGearsBattler *p_enemy);

	int get_state() const { return (int)current_state; }

	ClockGearsTurnBattle();
};
