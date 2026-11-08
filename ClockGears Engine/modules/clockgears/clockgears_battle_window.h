#pragma once

#include "clockgears_types.h"
#include "clockgears_battle.h"
#include "clockgears_animation.h"
#include "runtime/clockgears_message_window.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/canvas_layer.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"

// Representación visual animada de un battler en combate
struct ClockGearsBattlerVisual {
	Ref<ClockGearsBattleUnit> unit;
	TextureRect *texture_rect = nullptr;
	Ref<Texture2D> base_texture;
	Ref<AtlasTexture> atlas_texture;
	ClockGearsSpriteSheetData sprite_data;
	String current_anim = "idle";
	String current_sheet_path;
	float anim_timer = 0.0f;
	float state_duration = 0.0f;
	bool is_actor = false;
};

// ============================================================================
// Escena de Batalla en Runtime (ClockGearsBattleScene)
// CanvasLayer que encapsula gráficos de tropas, grupo, barra de iniciativa,
// comandos y la ventana de mensajes de la Fase 7A.
// ============================================================================
class ClockGearsBattleScene : public CanvasLayer {
	GDCLASS(ClockGearsBattleScene, CanvasLayer);

private:
	Ref<ClockGearsBattleController> controller;
	ClockGearsMessageWindow *message_window = nullptr;
	bool owns_message_window = false;

	// UI Layout
	ColorRect *bg_rect = nullptr;
	PanelContainer *main_container = nullptr;

	// Barra de Iniciativa superior
	PanelContainer *initiative_panel = nullptr;
	HBoxContainer *initiative_bar = nullptr;

	// Área de enemigos (Centro)
	HBoxContainer *enemies_container = nullptr;
	Vector<PanelContainer *> enemy_cards;

	// Área del grupo (Abajo izquierda)
	HBoxContainer *party_container = nullptr;
	Vector<PanelContainer *> party_cards;

	// Barra de comandos (Abajo derecha)
	PanelContainer *actions_panel = nullptr;
	VBoxContainer *actions_vbox = nullptr;
	Button *btn_attack = nullptr;
	Button *btn_skill = nullptr;
	Button *btn_item = nullptr;
	Button *btn_defend = nullptr;
	Button *btn_flee = nullptr;

	// Submenú selector (Objetivo / Habilidad / Objeto)
	PanelContainer *sub_menu_panel = nullptr;
	VBoxContainer *sub_menu_vbox = nullptr;

	// Log de combate en pantalla (abajo, compacto y no bloqueante)
	PanelContainer *battle_log_panel = nullptr;
	Label *battle_log_label = nullptr;

	Vector<ClockGearsBattlerVisual> battler_visuals;
	void _update_battler_visuals(float p_delta);
	void _trigger_visual_anim(const Ref<ClockGearsBattleUnit> &p_unit, const String &p_anim, float p_duration = 0.45f);

	void _setup_ui();
	void _refresh_ui();
	void _update_initiative_bar();
	void _show_target_selector(int p_action_type, int p_sub_id = 0, const String &p_sub_str = "");
	void _show_skill_selector();
	void _show_item_selector();
	void _close_sub_menu();

	void _on_btn_attack_pressed();
	void _on_btn_skill_pressed();
	void _on_btn_item_pressed();
	void _on_btn_defend_pressed();
	void _on_btn_flee_pressed();
	void _on_target_selected(int p_action_type, int p_target_idx, int p_sub_id, const String &p_sub_str);

	void _display_message(const String &p_text);
	void _check_battle_state();
	void _on_message_completed();
	void _on_hit_executed(const String &p_log);
	bool waiting_finish_confirmation = false;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static Ref<Texture2D> load_texture_with_fallbacks(const String &p_path);
	void setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm = "", ClockGearsMessageWindow *p_msg_window = nullptr);
	void finish_battle_immediate();

	Ref<ClockGearsBattleController> get_controller() const { return controller; }

	// Métodos para pruebas automatizadas y control programático
	bool select_action_attack(int p_target_idx);
	bool select_action_skill(int p_skill_id, int p_target_idx);
	bool select_action_item(const String &p_item_name, int p_target_idx);
	bool select_action_defend();
	bool select_action_flee();

	int get_battle_result() const;
	bool is_battle_finished() const;

	ClockGearsBattleScene();
	~ClockGearsBattleScene();
};
