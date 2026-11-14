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
	TextureRect *effect_rect = nullptr;
	Ref<Texture2D> base_texture;
	Ref<AtlasTexture> atlas_texture;
	Ref<Texture2D> effect_base_texture;
	Ref<AtlasTexture> effect_atlas_texture;
	ClockGearsSpriteSheetData sprite_data;
	ClockGearsSpriteSheetData effect_sprite_data;
	String current_anim = "idle";
	String current_sheet_path;
	int current_frame_idx = 0;
	float frame_timer = 0.0f;
	float current_frame_duration = 0.1f;
	float anim_timer = 0.0f;
	float state_duration = 0.0f;
	int loaded_effect_id = 0;
	int effect_frame_idx = 0;
	float effect_frame_timer = 0.0f;
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
	Control *battleback_bg_layer = nullptr;
	Control *battleback_floor_layer = nullptr;
	Ref<Texture2D> battleback_bg_tex;
	Ref<Texture2D> battleback_floor_tex;
	PanelContainer *main_container = nullptr;

	// Barra de Iniciativa superior
	PanelContainer *initiative_panel = nullptr;
	HBoxContainer *initiative_bar = nullptr;

	// Área de enemigos (Campo posicionado libremente)
	Control *enemies_field = nullptr;
	Vector<Control *> enemy_nodes;
	Vector<Label *> enemy_name_labels;
	Vector<ProgressBar *> enemy_hp_bars;

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
	void _trigger_unit_trigger(const Ref<ClockGearsBattleUnit> &p_unit, int p_trigger, const String &p_fallback, float p_min_duration = 0.45f);

	enum BattleActionSeqState {
		SEQ_IDLE = 0,
		SEQ_ATTACK_ANIM,
		SEQ_EFFECT_ANIM,
		SEQ_DAMAGE_ANIM
	};
	BattleActionSeqState action_seq_state = SEQ_IDLE;
	float action_seq_timer = 0.0f;
	Ref<ClockGearsBattleUnit> seq_attacker;
	Ref<ClockGearsBattleUnit> seq_target;
	int seq_action_type = 0;
	int seq_sub_id = 0;
	String seq_sub_str;
	int seq_target_idx = 0;

	void _start_action_sequence(int p_action_type, int p_target_idx, int p_sub_id = 0, const String &p_sub_str = "");
	void _step_action_sequence(float p_delta);

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
	void _on_battleback_bg_layer_draw();
	void _on_battleback_floor_layer_draw();
	void _on_enemies_field_draw();
	void _sync_enemy_hit_nodes_layout();
	void _check_battle_state();
	void _on_message_completed();
	void _on_hit_executed(const String &p_log);
	bool waiting_finish_confirmation = false;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static constexpr float BATTLE_REF_WIDTH = 960.0f;
	static constexpr float BATTLE_REF_HEIGHT = 540.0f;
	static constexpr float BATTLE_HUD_BOTTOM_PX = 199.0f;
	// Punto de alineación de tropas derivado de Tropa 001 (x=395, y=178) en área 960x540:
	// bloque lógico (140x141) -> centro (465, 248.5) => (0.484375, 0.460185)
	static constexpr float BATTLE_ALIGN_POINT_FX = 465.0f / 960.0f;
	static constexpr float BATTLE_ALIGN_POINT_FY = 248.5f / 540.0f;

	struct EnemyVisualLayout {
		Point2 block_origin;
		Rect2 sprite_box;
		Rect2 draw_spr_rect;
		float label_baseline_y = 0.0f;
		float label_max_w = 140.0f;
		int font_sz = 12;
		Rect2 hp_rect;
		Rect2 full_box;
	};

	struct TroopEnemyDrawEntry {
		String name;
		int level = 1;
		int hp = 1;
		int max_hp = 1;
		bool is_dead = false;
		Vector2 troop_pos;
		Ref<Texture2D> texture;
		Rect2 src_region;
		Color modulate = Color(1, 1, 1, 1);
	};

	enum BattlebackMode {
		BB_MODE_COVER = 0,
		BB_MODE_FIT = 1,
		BB_MODE_STRETCH = 2,
		BB_MODE_TILE = 3,
	};
	enum BattlebackAnchor {
		BB_ANCHOR_CENTER = 0,
		BB_ANCHOR_BOTTOM = 1,
	};

	static bool is_texture_ready(const Ref<Texture2D> &p_tex);
	static bool can_safely_resource_load_image(const String &p_path);
	static Ref<Texture2D> load_texture_with_fallbacks(const String &p_path);
	static Ref<Texture2D> create_placeholder_marker_texture();
	static void resolve_enemy_preview_sprite(const Dictionary &p_enemy_dict, Ref<Texture2D> &r_tex, Rect2 &r_src_region, String &r_source);

	static Point2 troop_pos_to_screen(const Vector2 &p_troop_pos, const Rect2 &p_vp_rect);
	static Vector2 screen_to_troop_pos(const Point2 &p_screen_pos, const Rect2 &p_vp_rect);
	static Rect2 get_battle_hud_strip_rect(const Rect2 &p_vp_rect);
	static EnemyVisualLayout compute_enemy_visual_layout(const Vector2 &p_troop_pos, const Rect2 &p_vp_rect, const Size2 &p_src_size, const Ref<Font> &p_font = Ref<Font>());
	static void draw_troop_enemies(Control *p_canvas, const Rect2 &p_vp_rect, const Vector<TroopEnemyDrawEntry> &p_entries, int p_selected_idx, bool p_show_selection, bool p_show_grid, const Ref<Font> &p_font);

	static Dictionary default_battleback_cfg(bool p_is_floor, const Size2 &p_tex_size = Size2());
	static Dictionary normalize_battleback_cfg(const Dictionary &p_cfg, bool p_is_floor, const Size2 &p_tex_size = Size2());
	static Rect2 compute_battleback_screen_rect(const Rect2 &p_vp_rect, const Size2 &p_tex_size, const Dictionary &p_cfg, bool p_is_floor);
	static void draw_battleback_layer(Control *p_layer_canvas, const Rect2 &p_vp_rect, const Ref<Texture2D> &p_tex, const Dictionary &p_cfg, bool p_is_floor);
	static String format_enemy_name_level(const Ref<Font> &p_font, int p_font_sz, const String &p_name, int p_level, float p_max_w);

	void setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm = "", ClockGearsMessageWindow *p_msg_window = nullptr, bool p_test_mode = false);
	void finish_battle_immediate();

	Ref<ClockGearsBattleController> get_controller() const { return controller; }

	// Métodos para pruebas automatizadas y control programático
	void advance_time(float p_delta);
	bool select_action_attack(int p_target_idx);
	bool select_action_skill(int p_skill_id, int p_target_idx);
	bool select_action_item(const String &p_item_name, int p_target_idx);
	bool select_action_defend();
	bool select_action_flee();

	int get_battle_result() const;
	bool is_battle_finished() const;

	bool is_initiative_bar_visible() const;
	bool is_formula_log_visible() const;
	Vector2 get_enemy_node_pos(int p_idx) const;
	String get_enemy_label_text(int p_idx) const;
	bool is_enemy_hp_bar_visible(int p_idx) const;
	String get_enemy_current_visual_anim(int p_idx) const;
	Rect2 get_drawn_battleback_rect(int p_slot) const;

	ClockGearsBattleScene();
	~ClockGearsBattleScene();
};
