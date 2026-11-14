#pragma once

#include "clockgears_types.h"
#include "clockgears_animation.h"
#include "core/clockgears_data_manager.h"
#include "core/variant/dictionary.h"
#include "editor/clockgears_tile_picker.h"
#include "editor/clockgears_ui_widgets.h"
#include "scene/resources/atlas_texture.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"

class ClockGearsBattleScene;
class ClockGearsEventEditorDialog;
class ClockGearsShapeEditor;
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

// ============================================================================
// Tileset Collision Editor (O = Pasa, X = Bloqueado, ★ = Pasa por detrás)
// ============================================================================
class ClockGearsTilesetCollisionEditor : public Control {
	GDCLASS(ClockGearsTilesetCollisionEditor, Control);

private:
	Ref<Texture2D> tileset_texture;
	Dictionary collision_map;
	int tile_size = 32;
	Callable on_collision_changed;

protected:
	static void _bind_methods() {}
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_tileset_texture(const Ref<Texture2D> &p_tex);
	Ref<Texture2D> get_tileset_texture() const { return tileset_texture; }

	void set_collision_map(const Dictionary &p_map);
	Dictionary get_collision_map() const { return collision_map; }

	void set_on_collision_changed(const Callable &p_cb) { on_collision_changed = p_cb; }

	ClockGearsTilesetCollisionEditor();
};

// ============================================================================
// ClockGearsDatabaseDialog
// ============================================================================
class ClockGearsDatabaseDialog : public AcceptDialog {
	GDCLASS(ClockGearsDatabaseDialog, AcceptDialog);

private:
	static inline ClockGearsDatabaseDialog *singleton = nullptr;

	struct ChapterDef {
		String name;
		Vector<int> categories;
	};
	Vector<ChapterDef> chapters;
	Vector<Button *> chapter_buttons;
	Vector<Button *> category_buttons;
	int current_chapter = 0;
	int current_category = CAT_ACTORS;
	int last_icon_scale = 0;

	void _refresh_icon_scale();
	void _on_editor_settings_changed();

	// Editor Tabs (Enemigos: General | Combate | Animaciones; Actores: General | Animaciones; Resto: General sin pestañas)
	TabContainer *main_editor_tabs = nullptr;
	VBoxContainer *combat_tab_vbox = nullptr;
	ScrollContainer *event_tab_scroll = nullptr;
	VBoxContainer *event_tab_vbox = nullptr;
	Label *event_tab_summary_label = nullptr;
	Tree *event_tab_pages_tree = nullptr;
	Button *btn_open_record_event_editor = nullptr;
	ClockGearsEventEditorDialog *record_event_editor = nullptr;

	// Pestaña 4: "Cuerpo y zonas" (Actores solo cuerpo; NPC, NPC ARPG, Enemigos ARPG cuerpo + zonas)
	MarginContainer *body_zones_tab_margin = nullptr;
	Label *bz_empty_label = nullptr;
	ClockGearsShapeEditor *body_zones_editor = nullptr;
	VBoxContainer *zone_props_box = nullptr;
	OptionButton *zone_filter_opt = nullptr;
	OptionButton *zone_trigger_opt = nullptr;
	HBoxContainer *zone_interval_row = nullptr;
	SpinBox *zone_interval_spin = nullptr;
	ItemList *zone_actions_list = nullptr;
	Button *btn_zone_act_add = nullptr;
	Button *btn_zone_act_remove = nullptr;
	Button *btn_zone_act_up = nullptr;
	Button *btn_zone_act_down = nullptr;
	VBoxContainer *zone_act_edit_box = nullptr;
	OptionButton *zone_act_type_opt = nullptr;
	HBoxContainer *zone_act_event_row = nullptr;
	OptionButton *zone_act_event_page_opt = nullptr;
	VBoxContainer *zone_act_switch_box = nullptr;
	ClockGearsSwitchVarPickerButton *zone_act_switch_picker = nullptr;
	LineEdit *zone_act_switch_name_edit = nullptr;
	OptionButton *zone_act_switch_val_opt = nullptr;
	VBoxContainer *zone_act_var_box = nullptr;
	ClockGearsSwitchVarPickerButton *zone_act_var_picker = nullptr;
	LineEdit *zone_act_var_name_edit = nullptr;
	OptionButton *zone_act_var_op_opt = nullptr;
	SpinBox *zone_act_var_val_spin = nullptr;
	bool updating_bz_ui = false;

	// Pestaña 5: "Movimiento" (NPC, NPC ARPG, Enemigos ARPG)
	MarginContainer *movement_tab_margin = nullptr;
	Label *mov_empty_label = nullptr;
	HBoxContainer *movement_tab_hbox = nullptr;
	ScrollContainer *mov_left_scroll = nullptr;
	VBoxContainer *mov_left_vbox = nullptr;
	OptionButton *mov_type_opt = nullptr;
	VBoxContainer *mov_wander_box = nullptr;
	SpinBox *mov_wander_spin = nullptr;
	VBoxContainer *mov_patrol_box = nullptr;
	OptionButton *mov_patrol_mode_opt = nullptr;
	ItemList *mov_patrol_list = nullptr;
	Button *btn_mov_patrol_add = nullptr;
	Button *btn_mov_patrol_remove = nullptr;
	Button *btn_mov_patrol_up = nullptr;
	Button *btn_mov_patrol_down = nullptr;
	VBoxContainer *mov_follow_box = nullptr;
	SpinBox *mov_follow_dist_spin = nullptr;
	HBoxContainer *mov_face_on_event_row = nullptr;
	CheckBox *mov_face_on_event_chk = nullptr;
	VBoxContainer *mov_chase_box = nullptr;
	SpinBox *mov_lose_sight_spin = nullptr;
	VBoxContainer *mov_ranged_box = nullptr;
	SpinBox *mov_min_dist_spin = nullptr;
	SpinBox *mov_max_dist_spin = nullptr;
	CheckBox *mov_avoid_obstacles_chk = nullptr;
	SpinBox *mov_recalc_ms_spin = nullptr;
	ClockGearsShapeEditor *movement_shape_editor = nullptr;
	bool updating_mov_ui = false;

	VBoxContainer *general_section_box = nullptr;
	VBoxContainer *stats_section_box = nullptr;
	VBoxContainer *exp_section_box = nullptr;
	VBoxContainer *invocation_section_box = nullptr;
	HBoxContainer *tileset_main_hbox = nullptr;

	// Column 1: Record List ("+", "-", "Max", "Copiar", "Pegar")
	VBoxContainer *col1_box = nullptr;
	Label *col1_title = nullptr;
	ItemList *record_list = nullptr;
	Button *btn_add_record = nullptr;
	Button *btn_remove_record = nullptr;
	Button *btn_change_max = nullptr;
	Button *btn_copy_record = nullptr;
	Button *btn_paste_record = nullptr;
	Dictionary clipboard_record;

	// Change Maximum Modal
	AcceptDialog *change_max_dialog = nullptr;
	SpinBox *change_max_spin = nullptr;

	// Column 2: Parameters
	VBoxContainer *col2_box = nullptr;
	MarginContainer *tab0_margin_container = nullptr;
	Label *col2_title = nullptr;
	GridContainer *params_grid = nullptr;
	LineEdit *name_edit = nullptr;
	LineEdit *desc_edit = nullptr;
	Label *type_label = nullptr;
	OptionButton *type_option = nullptr;
	Label *secondary_label = nullptr;
	OptionButton *secondary_option = nullptr;
	Label *tertiary_label = nullptr;
	OptionButton *tertiary_option = nullptr;
	Label *formula_label = nullptr;
	ClockGearsFormulaLineEdit *formula_edit = nullptr;

	// Campos numéricos de la fórmula de EXP para Clases (Sección 9.3: base=30, extra=20, acc_a=30, acc_b=30)
	Label *exp_labels[4] = { nullptr };
	SpinBox *exp_spins[4] = { nullptr };
	Label *exp_preview_title = nullptr;
	Label *exp_preview_label = nullptr;

	// Sección 5.1 y 5.2: Precio, Venta, Animación de Ataque, Consumible, Invocación y Daño
	Label *price_label = nullptr;
	SpinBox *price_spin = nullptr;
	Label *sell_price_title = nullptr;
	Label *sell_price_label = nullptr;
	Label *attack_anim_label = nullptr;
	OptionButton *attack_anim_option = nullptr;
	Label *consumable_label = nullptr;
	CheckBox *consumable_check = nullptr;
	Label *item_speed_label = nullptr;
	SpinBox *item_speed_spin = nullptr;
	Label *item_success_label = nullptr;
	SpinBox *item_success_spin = nullptr;
	Label *item_repeats_label = nullptr;
	SpinBox *item_repeats_spin = nullptr;
	Label *item_tp_gain_label = nullptr;
	SpinBox *item_tp_gain_spin = nullptr;
	Label *hit_type_label = nullptr;
	OptionButton *hit_type_option = nullptr;
	Label *item_element_label = nullptr;
	OptionButton *item_element_option = nullptr;
	Label *variance_label = nullptr;
	SpinBox *variance_spin = nullptr;
	Label *critical_label = nullptr;
	CheckBox *critical_check = nullptr;

	// Sección 5.3: Enemigos (Nivel, EXP, Oro, Debilidades y Resistencias, 3 Drops, Patrones de Acción)
	Label *enemy_level_label = nullptr;
	SpinBox *enemy_level_spin = nullptr;
	Label *enemy_exp_label = nullptr;
	SpinBox *enemy_exp_spin = nullptr;
	Label *enemy_gold_label = nullptr;
	SpinBox *enemy_gold_spin = nullptr;
	Label *battler_frames_label = nullptr;
	SpinBox *battler_frames_spin = nullptr;
	Label *battler_fps_label = nullptr;
	SpinBox *battler_fps_spin = nullptr;

	// Debilidades y Resistencias (como Pokémon) -> persiste en traits CG_TRAIT_ELEMENT_RATE
	VBoxContainer *enemy_element_rates_box = nullptr;
	GridContainer *enemy_element_rates_rows_vbox = nullptr;
	Vector<OptionButton *> enemy_elem_preset_opts;
	Vector<SpinBox *> enemy_elem_rate_spins;
	Vector<int> enemy_elem_ids;

	VBoxContainer *enemy_drops_box = nullptr;
	OptionButton *drop_kind_opt[3] = { nullptr };
	OptionButton *drop_item_opt[3] = { nullptr };
	SpinBox *drop_prob_spin[3] = { nullptr };

	VBoxContainer *enemy_actions_box = nullptr;
	Tree *enemy_actions_tree = nullptr;
	Button *btn_add_action_pattern = nullptr;
	Button *btn_remove_action_pattern = nullptr;
	AcceptDialog *action_pattern_dialog = nullptr;
	OptionButton *ap_skill_opt = nullptr;
	OptionButton *ap_cond_opt = nullptr;
	SpinBox *ap_cond_a_spin = nullptr;
	SpinBox *ap_cond_b_spin = nullptr;
	SpinBox *ap_rating_spin = nullptr;

	Label *stat_labels[8] = { nullptr };
	SpinBox *stat_spins[8] = { nullptr };

	// Sección Velocidades (Actores, NPC, NPC ARPG, Enemigos ARPG)
	VBoxContainer *speeds_section_box = nullptr;
	SpinBox *speed_walk_spin = nullptr;
	SpinBox *speed_run_spin = nullptr;
	Label *speed_attack_label = nullptr;
	SpinBox *speed_attack_spin = nullptr;
	Label *speeds_help_label = nullptr;

	// Sección IA de Enemigos ARPG (Pestaña Combate)
	VBoxContainer *arpg_ai_box = nullptr;
	OptionButton *arpg_ai_type_opt = nullptr;
	SpinBox *arpg_ai_vision_spin = nullptr;
	SpinBox *arpg_ai_range_spin = nullptr;
	SpinBox *arpg_ai_interval_spin = nullptr;
	SpinBox *arpg_ai_kb_resist_spin = nullptr;

	// System category panel
	VBoxContainer *system_panel = nullptr;
	LineEdit *sys_game_title_edit = nullptr;
	LineEdit *sys_currency_edit = nullptr;
	OptionButton *combat_mode_selector = nullptr;
	Label *lbl_sys_arpg_atk_move = nullptr;
	CheckBox *sys_arpg_move_atk_chk = nullptr;
	Label *lbl_sys_arpg_atk_spd = nullptr;
	SpinBox *sys_arpg_atk_spd_spin = nullptr;
	Label *lbl_sys_arpg_blk_move = nullptr;
	CheckBox *sys_arpg_move_blk_chk = nullptr;
	Label *lbl_sys_arpg_blk_spd = nullptr;
	SpinBox *sys_arpg_blk_spd_spin = nullptr;
	OptionButton *sys_movement_dirs_opt = nullptr;
	OptionButton *sys_mannequin_size_opt = nullptr;
	OptionButton *sys_start_map_option = nullptr;
	SpinBox *sys_start_x_spin = nullptr;
	SpinBox *sys_start_y_spin = nullptr;
	ItemList *sys_party_list = nullptr;
	OptionButton *sys_party_actor_opt = nullptr;
	Button *btn_add_party_member = nullptr;
	Button *btn_remove_party_member = nullptr;
	OptionButton *sys_camera_zoom_opt = nullptr;
	CheckBox *sys_camera_smooth_check = nullptr;
	CheckBox *sys_camera_clamp_check = nullptr;

	// Sección 5.4: Tropas (Colocación interactiva como en RPG Maker MZ)
	VBoxContainer *troop_members_box = nullptr;
	VBoxContainer *troop_members_list_vbox = nullptr;
	Button *btn_add_troop_member = nullptr;
	Button *btn_remove_troop_member_sel = nullptr;
	Button *btn_clear_troop_members = nullptr;
	Button *btn_align_troop_members = nullptr;
	Button *btn_autoname_troop = nullptr;
	Button *btn_battle_test_troop = nullptr;
	Button *btn_change_troop_bg = nullptr;
	Button *btn_adjust_troop_bg = nullptr;
	PopupMenu *troop_bg_adjust_menu = nullptr;
	PopupMenu *troop_ctx_menu = nullptr;
	PopupMenu *troop_scale_submenu = nullptr;
	PopupMenu *troop_hp_bar_submenu = nullptr;
	int troop_ctx_member_idx = -1;
	CheckBox *chk_troop_snap_grid = nullptr;
	VBoxContainer *troop_placement_col3_box = nullptr;
	Control *troop_placement_canvas = nullptr;
	Control *troop_bg_layer_ctrl = nullptr;
	Control *troop_floor_layer_ctrl = nullptr;
	Control *troop_enemies_overlay_ctrl = nullptr;
	Label *troop_hud_warning_label = nullptr;
	mutable HashMap<String, Ref<Texture2D>> troop_preview_tex_cache;
	mutable Ref<Texture2D> troop_placeholder_tex_cache;
	mutable bool troop_waiting_tex_ready = false;
	float troop_tex_poll_timer = 0.0f;
	int selected_troop_member_idx = 0;
	bool troop_member_dragging = false;
	Vector2 troop_drag_offset;
	bool troop_hp_bar_dragging = false;
	Vector2 troop_hp_drag_offset;

	// Ventana "Cambiar fondo" (Fondo + Suelo por tropa)
	AcceptDialog *troop_bg_dialog = nullptr;
	LineEdit *troop_bg1_edit = nullptr;
	LineEdit *troop_bg2_edit = nullptr;
	FileDialog *troop_bg_file_dialog = nullptr;
	int troop_bg_selecting_slot = 0; // 1 = Fondo (battleback1), 2 = Suelo (battleback2)

	// Ventanas "Ajustar Fondo" / "Ajustar Suelo" (una emergente propia por capa)
	struct AdjustDialogUI {
		AcceptDialog *dialog = nullptr;
		Control *canvas = nullptr;
		Control *bg_layer = nullptr;
		Control *floor_layer = nullptr;
		Control *overlay = nullptr;
		Label *no_img_label = nullptr;
		Label *info_label = nullptr;
		Button *mode_btns[4] = { nullptr };
		CheckBox *chk_integer_scale = nullptr;
		Button *btn_center = nullptr;
		Button *btn_reset = nullptr;
		int slot = 1; // 1=Fondo, 2=Suelo
		int troop_idx = -1;
		Dictionary edit_cfg;
		int drag_mode = 0; // 0=none, 1=move, 2..5=corners TL,TR,BL,BR
		Vector2 drag_start_mouse;
		Dictionary drag_start_cfg;
	};
	AdjustDialogUI troop_adjust_ui[2];

	// Ranuras con nombre para Enemigos ("Lanzamientos" con nombre y elemento opcional + "Otras" migradas)
	VBoxContainer *enemy_casts_box = nullptr;
	VBoxContainer *enemy_casts_rows_vbox = nullptr;
	Button *btn_add_enemy_cast = nullptr;
	VBoxContainer *enemy_others_box = nullptr;
	VBoxContainer *enemy_others_rows_vbox = nullptr;

	// Prueba de batalla + resumen + Reintentar / Continuar / Salir
	AcceptDialog *battle_test_dialog = nullptr;
	Control *battle_test_root = nullptr;
	ClockGearsBattleScene *battle_test_scene = nullptr;
	PanelContainer *battle_test_summary_panel = nullptr;
	Label *battle_test_summary_title = nullptr;
	Label *battle_test_summary_body = nullptr;
	Button *btn_battle_test_retry = nullptr;
	Button *btn_battle_test_continue = nullptr;
	int battle_test_troop_id = 0;
	int battle_test_last_result = 0;

	// Column 3: Graphic Viewer + Traits / Effects sub-panels
	VBoxContainer *col3_box = nullptr;
	Label *col3_title = nullptr;
	VBoxContainer *standard_graphic_box = nullptr;
	PanelContainer *graphic_frame = nullptr;
	TextureRect *graphic_preview = nullptr;
	Label *graphic_path_label = nullptr;
	String graphic_path_full_text;
	Button *btn_select_graphic = nullptr;
	Button *btn_pick_tile_graphic = nullptr;
	ClockGearsTilePickerDialog *tile_picker_dialog = nullptr;
	Button *btn_apply_changes = nullptr;
	bool has_pending_changes = false;
	mutable HashMap<String, Ref<Texture2D>> record_list_icon_cache;

	// Sección B: Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
	Label *anim_empty_state_label = nullptr;
	VBoxContainer *sprite_anim_editor_box = nullptr;
	ScrollContainer *anim_editor_scroll = nullptr;

	// 1. Gestión de animaciones (Lista estilo Godot + desplegable sincronizado)
	ItemList *anim_item_list = nullptr;
	OptionButton *anim_track_opt = nullptr;
	CheckBox *actor_dir_idle_chk = nullptr;
	Button *btn_add_anim = nullptr;
	Button *btn_rename_anim = nullptr;
	Button *btn_remove_anim = nullptr;
	ConfirmationDialog *new_anim_dialog = nullptr;
	LineEdit *new_anim_name_edit = nullptr;
	ConfirmationDialog *rename_anim_dialog = nullptr;
	LineEdit *rename_anim_name_edit = nullptr;

	// 2. Línea de tiempo de Frames (miniaturas con reordenar y quitar)
	ScrollContainer *frames_strip_scroll = nullptr;
	HFlowContainer *frames_strip_hbox = nullptr;
	Button *btn_open_sheet_importer = nullptr;

	// 3. Ventana "Añadir frames desde spritesheet"
	ConfirmationDialog *sheet_importer_dialog = nullptr;
	LineEdit *sheet_importer_path_edit = nullptr;
	Button *sheet_importer_browse_btn = nullptr;
	SpinBox *sheet_importer_cols_spin = nullptr;
	SpinBox *sheet_importer_rows_spin = nullptr;
	Label *sheet_importer_info_lbl = nullptr;
	Button *sheet_importer_sel_all_btn = nullptr;
	Button *sheet_importer_clear_sel_btn = nullptr;
	ScrollContainer *sheet_importer_scroll = nullptr;
	Control *sheet_importer_canvas = nullptr;
	Label *sheet_importer_status_lbl = nullptr;
	Button *sheet_importer_add_btn = nullptr;
	FileDialog *sheet_importer_file_dialog = nullptr;
	Ref<Texture2D> sheet_importer_texture;
	String sheet_importer_cur_path;
	Vector<int> sheet_importer_selected_frames;
	bool sheet_importer_drag_active = false;
	int sheet_importer_drag_start_c = 0;
	int sheet_importer_drag_start_r = 0;

	// 4. Reproducción y Ajustes
	SpinBox *anim_fps_spin = nullptr;
	bool anim_fps_line_edited = false;
	CheckBox *anim_loop_chk = nullptr;
	CheckBox *anim_random_order_chk = nullptr;
	Button *btn_anim_play = nullptr;
	TextureRect *anim_preview_rect = nullptr;
	Ref<AtlasTexture> anim_preview_atlas;

	bool anim_preview_playing = false;
	float anim_preview_timer = 0.0f;
	int anim_preview_frame_idx = 0;
	float anim_preview_frame_target_dur = 0.1f;
	ClockGearsSpriteSheetData current_sprite_data;

	// Sección B2: Equipo ARPG (Modo Completo / Simple, Orden de Capa, Detrás del Cuerpo y Vista Previa en Capas - Tanda 1C)
	VBoxContainer *anim_sel_sec = nullptr;
	VBoxContainer *anim_frames_sec = nullptr;
	HFlowContainer *anim_submode_bar = nullptr;
	Button *btn_anim_mode_frames = nullptr;
	Button *btn_anim_mode_shapes = nullptr;
	int anim_submode = 0; // 0 = Frames, 1 = Formas

	// Editor de Formas, Hitboxes, Hurtbox, Ancla y Maniquí (9A Tanda 2)
	VBoxContainer *anim_formas_sec = nullptr;
	HFlowContainer *formas_frame_bar = nullptr;
	Button *btn_shape_prev_frame = nullptr;
	OptionButton *shape_frame_opt = nullptr;
	Button *btn_shape_next_frame = nullptr;
	Button *btn_shape_copy_next = nullptr;
	Button *btn_shape_copy_all = nullptr;
	OptionButton *shape_copy_dir_opt = nullptr;
	OptionButton *shape_copy_mode_opt = nullptr;
	Button *btn_shape_copy_dir = nullptr;
	Label *shape_no_frames_lbl = nullptr;
	ClockGearsShapeEditor *shape_editor = nullptr;
	int shape_frame_idx = 0;
	bool updating_shape_ui = false;

	HFlowContainer *arpg_piece_mode_bar = nullptr;
	OptionButton *arpg_draw_mode_opt = nullptr;
	SpinBox *arpg_layer_order_spin = nullptr;
	CheckBox *anim_behind_body_chk = nullptr;

	VBoxContainer *arpg_simple_editor_box = nullptr;
	OptionButton *arpg_simple_group_opt = nullptr;
	OptionButton *arpg_simple_dir_opt = nullptr;
	LineEdit *arpg_simple_path_edit = nullptr;
	Button *arpg_simple_browse_btn = nullptr;
	Button *arpg_simple_clear_btn = nullptr;
	FileDialog *arpg_simple_file_dialog = nullptr;
	SpinBox *arpg_simple_offset_x_spin = nullptr;
	SpinBox *arpg_simple_offset_y_spin = nullptr;
	Button *arpg_simple_center_btn = nullptr;
	CheckBox *arpg_simple_behind_chk = nullptr;

	VBoxContainer *arpg_preview_box = nullptr;
	OptionButton *arpg_prev_actor_opt = nullptr;
	OptionButton *arpg_prev_state_opt = nullptr;
	OptionButton *arpg_prev_dir_opt = nullptr;
	CheckBox *arpg_prev_show_rest_chk = nullptr;
	Button *btn_arpg_prev_play = nullptr;
	Control *arpg_prev_canvas = nullptr;
	bool arpg_prev_dragging = false;
	Vector2 arpg_prev_drag_start_mouse;
	int arpg_prev_drag_start_ox = 0;
	int arpg_prev_drag_start_oy = 0;
	int arpg_prev_draw_scale = 3;

	// Sección: Conexión - Animaciones de combate (Actores y Enemigos)
	VBoxContainer *combat_anims_box = nullptr;
	OptionButton *opt_anim_idle = nullptr;
	OptionButton *opt_anim_attack = nullptr;
	OptionButton *opt_anim_damage = nullptr;
	OptionButton *opt_anim_death = nullptr;
	OptionButton *opt_anim_victory = nullptr;
	OptionButton *opt_anim_defend = nullptr;
	Label *lbl_anim_victory = nullptr;

	// Sección 4.2 / 5.1 / 5.3: Editor de Rasgos (Traits) en Columna 3
	VBoxContainer *traits_box = nullptr;
	Tree *traits_tree = nullptr;
	Button *btn_add_trait = nullptr;
	Button *btn_remove_trait = nullptr;
	AcceptDialog *trait_dialog = nullptr;
	OptionButton *trait_type_opt = nullptr;
	OptionButton *trait_data_opt = nullptr;
	SpinBox *trait_val_spin = nullptr;

	// Sección 4.3 / 5.2: Editor de Efectos (Effects) en Columna 3
	VBoxContainer *effects_box = nullptr;
	Tree *effects_tree = nullptr;
	Button *btn_add_effect = nullptr;
	Button *btn_remove_effect = nullptr;
	AcceptDialog *effect_dialog = nullptr;
	OptionButton *effect_type_opt = nullptr;
	OptionButton *effect_data_opt = nullptr;
	SpinBox *effect_val1_spin = nullptr;
	SpinBox *effect_val2_spin = nullptr;

	// Tileset Slots (RPG Maker MZ A1..A5, B, C, D, E) in Column 2 + Tabs in Column 3
	static const int TILESET_SLOT_COUNT = 9;
	int active_tileset_slot_index = 4; // Por defecto A5 (tiles normales 32x32)
	int pending_load_slot_index = -1;
	int active_a_subtab_index = 4; // 0=A1, 1=A2, 2=A3, 3=A4, 4=A5
	VBoxContainer *tileset_left_col = nullptr;
	ScrollContainer *tileset_slots_scroll = nullptr;
	VBoxContainer *tileset_slots_box = nullptr;
	LineEdit *tileset_slot_edits[TILESET_SLOT_COUNT] = { nullptr };
	Button *btn_load_slot[TILESET_SLOT_COUNT] = { nullptr };
	Button *btn_clear_slot[TILESET_SLOT_COUNT] = { nullptr };

	// Tileset Collision Box (when CAT_TILESETS is selected)
	PanelContainer *tileset_collision_panel = nullptr;
	VBoxContainer *tileset_collision_box = nullptr;
	HBoxContainer *tileset_tabs_row = nullptr;
	HBoxContainer *tileset_group_tabs_box = nullptr;
	Button *btn_ts_group_tab[5] = { nullptr }; // A, B, C, D, E
	HBoxContainer *tileset_a_subtabs_box = nullptr;
	Button *btn_ts_a_subtab[5] = { nullptr }; // A1, A2, A3, A4, A5
	Label *active_slot_info_label = nullptr;
	Button *btn_load_tileset_sheet = nullptr;
	ScrollContainer *tileset_scroll = nullptr;
	ClockGearsTilesetCollisionEditor *tileset_collision_editor = nullptr;
	ClockGearsTilePicker *db_tile_picker = nullptr;
	PanelContainer *left_sidebar_panel = nullptr;
	ScrollContainer *col2_scroll_container = nullptr;
	ScrollContainer *col3_scroll_container = nullptr;

	Array category_data[CAT_MAX];
	Dictionary system_config;

	int current_combat_mode = COMBAT_MODE_TURN_BASED;
	bool updating_ui = false;

	// File dialog for Sprite / Icon / Battler / Tileset selection
	FileDialog *file_dialog = nullptr;
	AcceptDialog *asset_warning_dialog = nullptr;
	bool selecting_tileset_sheet = false;

	static String _get_slot_key(int p_slot_idx);
	static String _get_slot_label(int p_slot_idx);
	static int _find_slot_index(const String &p_slot_key);
	int _ensure_active_tileset_record();
	void _refresh_tileset_slots_ui();
	void _update_tileset_split_layout();
	void _on_tileset_group_tab_pressed(int p_group_idx);
	void _on_tileset_a_subtab_pressed(int p_sub_idx);
	void _on_load_slot_button_pressed(int p_slot_idx);
	void _on_clear_slot_button_pressed(int p_slot_idx);
	void _on_window_size_changed();

	void _init_empty_database();
	Dictionary _create_empty_record(int p_category, int p_id) const;
	String _get_category_name(int p_category) const;
	void _populate_db_records_option(OptionButton *p_opt, int p_db_category, const String &p_empty_text, bool p_only_user_records_when_non_empty = false);
	void _populate_catalog_option(OptionButton *p_opt, const PackedStringArray &p_items);
	void _select_option_by_id_or_index(OptionButton *p_opt, int p_id_or_idx);
	void _update_exp_preview();

	void _init_chapters();
	int _find_chapter_for_category(int p_category) const;
	void _on_chapter_selected(int p_chapter_idx);
	void _update_chapter_and_category_visibility();
	void _save_editor_ui_state();
	Dictionary _load_editor_ui_state() const;
	void _on_category_selected(int p_category);
	void _refresh_category_view();
	void _refresh_record_list();
	void _on_record_selected(int p_index);

	void _on_add_record_pressed();
	void _on_remove_record_pressed();
	void _on_change_max_pressed();
	void _on_change_max_confirmed();
	void _on_copy_record_pressed();
	void _on_paste_record_pressed();

	void _on_param_changed();
	void _on_param_text_changed(const String &p_new_text);
	void _on_param_option_changed(int p_idx);
	void _on_param_spin_changed(double p_val);
	void _on_param_check_toggled(bool p_pressed);
	void _update_sell_price_label(int p_price);

	// Sección 5.3: Drops y Patrones de Acción
	void _populate_drop_item_option(int p_slot, int p_kind);
	void _on_drop_kind_changed(int p_idx, int p_slot);
	void _on_drop_field_changed(int p_slot);
	void _on_arpg_ai_field_changed();
	void _refresh_action_patterns_tree(const Array &p_actions);
	void _on_add_action_pattern_pressed();
	void _on_remove_action_pattern_pressed();
	void _on_action_pattern_confirmed();

	// Sección 4.2 / 5.1 / 5.3: Rasgos (Traits)
	void _refresh_traits_tree(const Array &p_traits);
	void _populate_trait_data_option(int p_trait_code);
	void _on_trait_type_changed(int p_idx);
	void _on_add_trait_pressed();
	void _on_remove_trait_pressed();
	void _on_trait_confirmed();

	// Sección 4.3 / 5.2: Efectos (Effects)
	void _refresh_effects_tree(const Array &p_effects);
	void _populate_effect_data_option(int p_effect_code);
	void _on_effect_type_changed(int p_idx);
	void _on_add_effect_pressed();
	void _on_remove_effect_pressed();
	void _on_effect_confirmed();

	// Categoría Sistema: Mapa Inicial y Grupo Inicial
	void _refresh_system_maps_and_party();
	void _on_add_party_member_pressed();
	void _on_remove_party_member_pressed();
	void _on_start_map_selected(int p_idx);

	void _bind_numeric_spinbox(SpinBox *p_spin);
	void _on_spinbox_line_edit_text_changed(const String &p_new_text, SpinBox *p_spin);
	void _on_spinbox_line_edit_submitted(const String &p_text, SpinBox *p_spin);
	void _on_spinbox_line_edit_focus_exited(SpinBox *p_spin);

	void _on_graphic_gui_input(const Ref<InputEvent> &p_event);
	void _on_select_graphic_pressed();
	void _on_pick_tile_graphic_pressed();
	void _on_tile_graphic_picked(const Array &p_sel, const Vector2i &p_block);
	void _on_tile_picker_closed();
	void _restore_focus_after_subdialog();
	bool _has_visible_child_dialog() const;
	void _set_pending_changes(bool p_pending);
	void _mark_database_dirty();
	void _on_db_apply_pressed();
	void _on_db_ok_pressed();
	void _clear_record_list_icon_cache() const;
	void _invalidate_record_list_icon(int p_category, int p_index) const;
	Ref<Texture2D> _build_record_list_icon_texture(int p_category, const Dictionary &p_rec, int p_icon_px, const Ref<Texture2D> &p_fallback) const;
	void _update_record_list_item_icon(int p_index);
	void _update_graphic_preview_ui(const Dictionary &p_rec);
	void _set_graphic_path_label_text(const String &p_full_text);
	void _refit_graphic_path_label();
	void _apply_record_graphic(int p_index, const String &p_path, bool p_has_tile, const Vector2i &p_tile_coords, const Vector2i &p_tile_block);
	void _on_load_tileset_sheet_pressed();
	void _on_file_selected(const String &p_path);
	void _on_tileset_collision_changed();

	void _on_system_field_changed();
	void _on_system_text_changed(const String &p_new_text);
	void _on_system_spin_changed(double p_val);
	void _update_system_arpg_controls_visibility();
	void _on_system_arpg_controls_changed();
	bool _has_valid_selected_record() const;
	void _set_anim_editor_controls_enabled(bool p_enabled);
	void _clear_and_disable_anim_editor();
	void _on_combat_mode_selected(int p_idx);

	Ref<Texture2D> _load_texture_from_path(const String &p_path) const;

	// Callbacks de Tropas (Fase 8A / Cierre)
	void _refresh_troop_members_ui();
	void _on_add_troop_member_pressed();
	void _on_remove_troop_member_pressed(int p_idx);
	void _on_remove_selected_troop_member_pressed();
	void _on_clear_troop_members_pressed();
	void _on_align_troop_members_pressed();
	void _on_autoname_troop_pressed();
	void _on_battle_test_troop_pressed();
	void _start_battle_test(int p_troop_id);
	void _on_battle_test_finished(int p_result);
	void _on_battle_test_retry_pressed();
	void _on_battle_test_close_pressed();
	void _on_battle_test_window_input(const Ref<InputEvent> &p_ev);

	void _on_change_troop_bg_pressed();
	void _on_adjust_troop_bg_pressed();
	void _on_adjust_troop_bg_menu_id(int p_id);
	void _on_troop_scale_menu_id_pressed(int p_id);
	void _on_troop_hp_bar_menu_id_pressed(int p_id);
	void _build_adjust_dialog(int p_ui_idx, int p_slot);
	void _open_adjust_dialog(int p_slot);
	void _sync_adjust_dialog_controls(int p_ui_idx);
	void _redraw_adjust_dialog(int p_ui_idx);
	void _on_adjust_mode_selected(int p_mode, int p_ui_idx);
	void _on_adjust_integer_toggled(bool p_pressed, int p_ui_idx);
	void _on_adjust_center_pressed(int p_ui_idx);
	void _on_adjust_reset_pressed(int p_ui_idx);
	void _on_adjust_confirmed(int p_ui_idx);
	void _on_adjust_layer_draw(int p_ui_idx, int p_layer_slot);
	void _on_adjust_overlay_draw(int p_ui_idx);
	void _on_adjust_canvas_gui_input(const Ref<InputEvent> &p_ev, int p_ui_idx);
	void _on_adjust_window_input(const Ref<InputEvent> &p_ev, int p_ui_idx);

	void _on_troop_bg_browse_pressed(int p_slot);
	void _on_troop_bg_clear_pressed(int p_slot);
	void _on_troop_bg_file_selected(const String &p_path);
	Ref<Texture2D> _retain_troop_texture(const String &p_path) const;
	Ref<Texture2D> _get_troop_placeholder_texture() const;
	void _clear_troop_texture_cache() const;
	void _update_troop_hud_warning(const Dictionary &p_troop_dict);
	void _on_editor_filesystem_changed();
	void _on_editor_resources_reimported(const PackedStringArray &p_resources);
	void _queue_troop_preview_redraw();
	void _draw_troop_enemies_preview(Control *p_canvas, const Rect2 &p_vp_rect, const Dictionary &p_troop_dict, bool p_show_selection, bool p_show_grid) const;
	void _on_troop_placement_canvas_draw();
	void _on_troop_bg_layer_draw();
	void _on_troop_floor_layer_draw();
	void _on_troop_enemies_overlay_draw();
	void _on_troop_placement_canvas_gui_input(const Ref<InputEvent> &p_ev);
	void _on_troop_member_enemy_changed(int p_enemy_id, int p_idx);
	void _on_troop_member_x_changed(double p_val, int p_idx);
	void _on_troop_member_y_changed(double p_val, int p_idx);

	// Debilidades y Resistencias + Ranuras fijas y Lanzamientos de Enemigos
	void _refresh_enemy_element_rates_ui();
	void _on_enemy_elem_preset_changed(int p_sel_idx, int p_row);
	void _on_enemy_elem_rate_spin_changed(double p_val, int p_row);
	void _update_enemy_element_rate_trait(int p_elem_id, double p_rate_pct);
	void _ensure_enemy_fixed_anim_slots(Dictionary &r_enemy_dict);
	void _refresh_enemy_casts_ui();
	void _on_add_enemy_cast_pressed();
	void _on_rename_enemy_cast_pressed(int p_idx);
	void _on_remove_enemy_cast_pressed(int p_idx);
	void _on_enemy_cast_element_changed(int p_sel_idx, int p_idx);
	void _refresh_enemy_others_ui();
	void _on_reassign_enemy_other_selected(int p_opt_idx, int p_other_idx);
	void _on_remove_enemy_other_pressed(int p_other_idx);
	void _update_current_anim_slot_labels();

	// Callbacks del Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
	String _get_active_anim_name() const;
	void _ensure_actor_fixed_slot_exists(const String &p_slot_name);
	void _on_system_movement_dirs_selected(int p_idx);
	void _on_actor_directional_idle_toggled(bool p_pressed);
	void _refresh_anim_track_options();
	void _on_anim_track_selected(int p_idx);
	void _on_anim_item_selected(int p_idx);
	void _on_anim_item_activated(int p_idx);
	void _on_add_anim_pressed();
	void _on_new_anim_dialog_confirmed();
	void _on_rename_anim_pressed();
	void _on_rename_anim_dialog_confirmed();
	void _on_remove_anim_pressed();

	void _on_open_sheet_importer_pressed();
	void _on_sheet_importer_browse_pressed();
	void _on_sheet_importer_file_selected(const String &p_path);
	void _on_sheet_importer_grid_changed(double p_unused = 0.0);
	void _on_sheet_importer_canvas_draw();
	void _on_sheet_importer_canvas_gui_input(const Ref<InputEvent> &p_ev);
	void _on_sheet_importer_sel_all_pressed();
	void _on_sheet_importer_clear_sel_pressed();
	void _on_sheet_importer_add_confirmed();
	void _update_sheet_importer_ui();

	void _setup_combat_anims_ui();
	void _refresh_combat_anims_dropdowns();
	void _on_combat_anim_selected(int p_unused = 0);
	void _on_frame_duration_changed(double p_val, int p_idx);
	void _on_frame_random_toggled(bool p_pressed, int p_idx);
	void _on_frame_min_dur_changed(double p_val, int p_idx);
	void _on_frame_max_dur_changed(double p_val, int p_idx);

	void _refresh_frames_timeline();
	void _refresh_anim_preview();
	void _on_frame_move_left(int p_step_idx);
	void _on_frame_move_right(int p_step_idx);
	void _on_frame_remove(int p_step_idx);
	void _apply_anim_fps_to_current_slot();
	void _on_anim_fps_value_changed(double p_val);
	void _on_anim_fps_text_changed(const String &p_text);
	void _on_anim_fps_submitted(const String &p_text);
	void _on_anim_fps_focus_exited();
	void _on_anim_playback_settings_changed();
	void _on_anim_play_pressed();
	void _update_anim_playback(float p_delta);

	void _setup_arpg_equip_anim_ui();
	void _setup_shape_editor_ui();
	void _refresh_anim_submode_ui();
	void _refresh_shape_editor_context();
	void _refresh_shape_copy_dir_options();
	void _sync_shape_editor_for_current_frame();
	void _on_anim_submode_selected(int p_mode);
	void _on_shape_frame_opt_selected(int p_idx);
	void _on_shape_prev_frame_pressed();
	void _on_shape_next_frame_pressed();
	void _on_shape_copy_next_pressed();
	void _on_shape_copy_all_pressed();
	void _on_shape_copy_dir_pressed();
	void _on_shape_editor_data_changed();
	void _on_system_mannequin_size_selected(int p_idx);
	void _refresh_arpg_piece_anim_mode_ui();
	void _refresh_arpg_simple_fields();
	void _refresh_arpg_preview_selectors();
	void _update_arpg_preview_canvas();
	void _on_arpg_draw_mode_selected(int p_idx);
	void _on_arpg_layer_order_changed(double p_val);
	void _on_arpg_simple_group_or_dir_changed(int p_idx);
	void _on_arpg_simple_browse_pressed();
	void _on_arpg_simple_file_selected(const String &p_path);
	void _on_arpg_simple_clear_pressed();
	void _on_arpg_simple_center_pressed();
	void _on_arpg_simple_prop_changed();
	void _on_arpg_prev_selector_changed(int p_idx = 0);
	void _on_arpg_prev_show_rest_toggled(bool p_pressed);
	void _on_arpg_prev_canvas_draw();
	void _on_arpg_prev_canvas_gui_input(const Ref<InputEvent> &p_ev);

	void _refresh_event_tab_summary();
	void _on_open_record_event_editor_pressed();
	void _on_record_event_saved(const Array &p_pages);

	// Pestaña Cuerpo y zonas + Pestaña Movimiento
	void _setup_body_zones_tab_ui();
	void _setup_movement_tab_ui();
	void _resolve_entity_idle_preview(const Dictionary &p_rec, Ref<Texture2D> &r_tex, Rect2i &r_region) const;
	void _sync_entity_preview_textures();
	void _load_body_zones_tab_from_record(const Dictionary &p_rec);
	void _refresh_zone_props_ui();
	void _refresh_zone_action_detail_ui();
	void _commit_zone_metadata_from_ui();
	void _commit_body_zones_to_record();
	void _on_bz_editor_data_changed();
	void _on_bz_editor_zone_selected(int p_zone_idx);
	void _on_zone_prop_changed(int p_unused = 0);
	void _on_zone_interval_changed(double p_val);
	void _on_zone_action_list_selected(int p_idx);
	void _on_zone_act_add_pressed();
	void _on_zone_act_remove_pressed();
	void _on_zone_act_move_pressed(int p_delta);
	void _on_zone_act_detail_changed();
	void _on_zone_act_switch_picked(int p_id, const String &p_name);
	void _on_zone_act_var_picked(int p_id, const String &p_name);

	void _load_movement_tab_from_record(const Dictionary &p_rec);
	void _update_movement_contextual_visibility(const String &p_mode);
	void _refresh_patrol_points_list();
	void _sync_movement_canvas();
	void _commit_movement_to_record();
	void _on_mov_type_selected(int p_idx);
	void _on_mov_field_changed(double p_unused = 0.0);
	void _on_mov_bool_toggled(bool p_pressed);
	void _on_mov_patrol_list_selected(int p_idx);
	void _on_mov_patrol_add_pressed();
	void _on_mov_patrol_remove_pressed();
	void _on_mov_patrol_move_pressed(int p_delta);
	void _on_movement_visual_changed();

	void _save_database_to_disk();
	void _load_database_from_disk();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void _input_from_window(const Ref<InputEvent> &p_event) override;
	virtual void cancel_pressed() override;

public:
	void open_database();
	void select_category(int p_category);
	int add_tileset_record(const String &p_name = "");
	void remove_tileset_record(int p_index = -1);
	int get_tileset_count() const;
	void select_tileset_slot(const String &p_slot_key);
	String get_active_tileset_slot() const;
	bool import_tileset_slot(const String &p_slot_key, const String &p_file_path);
	void clear_tileset_slot(const String &p_slot_key);
	String get_tileset_slot_path(int p_record_idx, const String &p_slot_key) const;
	void set_tileset_slot_tile_meta(const String &p_slot_key, const Vector2i &p_coords, const Dictionary &p_meta);
	void drag_paint_tiles_meta(const String &p_slot_key, const Vector2i &p_from, const Vector2i &p_to, const Dictionary &p_meta);
	Dictionary get_tileset_slot_tile_meta(const String &p_slot_key, const Vector2i &p_coords) const;
	void save_to_disk();
	void load_from_disk();
	void set_remembered_window_size(const Vector2i &p_size);
	Vector2i get_remembered_window_size() const;
	Dictionary check_layout_fits_window(const Vector2i &p_window_size) const;
	Dictionary get_troop_member_preview_info(int p_troop_idx, int p_member_idx) const;
	Rect2 get_troop_preview_viewport_rect() const;
	Dictionary get_troop_preview_enemy_layout(int p_member_idx) const;
	Dictionary get_enemy_anim_slot_info(int p_enemy_idx, const String &p_slot_id) const;
	bool reassign_enemy_other_animation(int p_enemy_idx, const String &p_other_name, const String &p_target_slot);
	bool remove_enemy_other_animation(int p_enemy_idx, const String &p_other_name);
	void select_anim_slot(const String &p_slot_id);
	void remove_anim_frame_at(int p_frame_idx);
	void set_system_movement_directions(int p_dirs);
	int get_system_movement_directions() const;
	void set_system_mannequin_size(int p_size);
	int get_system_mannequin_size() const;
	void set_anim_submode(int p_mode);
	int get_anim_submode() const;
	ClockGearsShapeEditor *get_shape_editor() const { return shape_editor; }
	void select_shape_frame(int p_frame_idx);
	bool copy_shapes_to_next_frame();
	bool copy_shapes_to_all_frames();
	bool copy_shapes_from_direction(const String &p_from_dir, const String &p_mode = "auto");
	void set_actor_directional_idle(bool p_enabled);
	bool is_actor_directional_idle() const;
	Dictionary get_troop_battleback_cfg(int p_troop_idx, int p_slot) const;
	void set_troop_battleback_cfg(int p_troop_idx, int p_slot, const Dictionary &p_cfg);
	String get_formatted_troop_enemy_label(int p_troop_idx, int p_member_idx, float p_max_w = 0.0f) const;
	Dictionary get_chapter_structure_info() const;
	void select_category_by_key(const String &p_key);
	Ref<Texture2D> get_record_list_icon(int p_row_idx) const;
	bool open_record_event_editor();
	ClockGearsShapeEditor *get_body_zones_editor() const { return body_zones_editor; }
	ClockGearsShapeEditor *get_movement_shape_editor() const { return movement_shape_editor; }
	TabContainer *get_main_editor_tabs() const { return main_editor_tabs; }
	bool select_main_tab_by_name(const String &p_name);
	Dictionary check_all_tabs_layout(const Vector2i &p_window_size);

	static ClockGearsDatabaseDialog *get_singleton() { return singleton; }
	void sync_system_player_start(const String &p_map_path, int p_x, int p_y);

	ClockGearsDatabaseDialog();
	virtual ~ClockGearsDatabaseDialog() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}
};
