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
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
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

	// Left sidebar category buttons
	Vector<Button *> category_buttons;
	int current_category = CAT_ITEMS;

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

	// Sección 5.3: Enemigos (EXP, Oro, Battler animado, 3 Drops, Patrones de Acción)
	Label *enemy_exp_label = nullptr;
	SpinBox *enemy_exp_spin = nullptr;
	Label *enemy_gold_label = nullptr;
	SpinBox *enemy_gold_spin = nullptr;
	Label *battler_frames_label = nullptr;
	SpinBox *battler_frames_spin = nullptr;
	Label *battler_fps_label = nullptr;
	SpinBox *battler_fps_spin = nullptr;

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

	// System category panel
	VBoxContainer *system_panel = nullptr;
	LineEdit *sys_game_title_edit = nullptr;
	LineEdit *sys_currency_edit = nullptr;
	OptionButton *combat_mode_selector = nullptr;
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

	// Sección 5.4: Tropas (Lista de Miembros con posición)
	VBoxContainer *troop_members_box = nullptr;
	VBoxContainer *troop_members_list_vbox = nullptr;
	Button *btn_add_troop_member = nullptr;

	// Column 3: Graphic Viewer + Traits / Effects sub-panels
	VBoxContainer *col3_box = nullptr;
	Label *col3_title = nullptr;
	VBoxContainer *standard_graphic_box = nullptr;
	PanelContainer *graphic_frame = nullptr;
	TextureRect *graphic_preview = nullptr;
	Label *graphic_path_label = nullptr;
	Button *btn_select_graphic = nullptr;
	Button *btn_pick_tile_graphic = nullptr;
	ClockGearsTilePickerDialog *tile_picker_dialog = nullptr;

	// Sección B: Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
	VBoxContainer *sprite_anim_editor_box = nullptr;
	ScrollContainer *anim_editor_scroll = nullptr;

	// 1. Gestión de animaciones (Lista estilo Godot + desplegable sincronizado)
	ItemList *anim_item_list = nullptr;
	OptionButton *anim_track_opt = nullptr;
	Button *btn_add_anim = nullptr;
	Button *btn_rename_anim = nullptr;
	Button *btn_remove_anim = nullptr;
	ConfirmationDialog *new_anim_dialog = nullptr;
	LineEdit *new_anim_name_edit = nullptr;
	ConfirmationDialog *rename_anim_dialog = nullptr;
	LineEdit *rename_anim_name_edit = nullptr;

	// 2. Línea de tiempo de Frames (miniaturas con reordenar y quitar)
	ScrollContainer *frames_strip_scroll = nullptr;
	HBoxContainer *frames_strip_hbox = nullptr;
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
	CheckBox *anim_loop_chk = nullptr;
	Button *btn_anim_play = nullptr;
	TextureRect *anim_preview_rect = nullptr;
	Ref<AtlasTexture> anim_preview_atlas;

	bool anim_preview_playing = false;
	float anim_preview_timer = 0.0f;
	ClockGearsSpriteSheetData current_sprite_data;

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
	VBoxContainer *tileset_slots_box = nullptr;
	LineEdit *tileset_slot_edits[TILESET_SLOT_COUNT] = { nullptr };
	Button *btn_load_slot[TILESET_SLOT_COUNT] = { nullptr };
	Button *btn_clear_slot[TILESET_SLOT_COUNT] = { nullptr };

	// Tileset Collision Box (when CAT_TILESETS is selected)
	VBoxContainer *tileset_collision_box = nullptr;
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
	void _on_load_tileset_sheet_pressed();
	void _on_file_selected(const String &p_path);
	void _on_tileset_collision_changed();

	void _on_system_field_changed();
	void _on_system_text_changed(const String &p_new_text);
	void _on_system_spin_changed(double p_val);
	void _on_combat_mode_selected(int p_idx);

	Ref<Texture2D> _load_texture_from_path(const String &p_path) const;

	// Callbacks de Tropas (Fase 8A)
	void _refresh_troop_members_ui();
	void _on_add_troop_member_pressed();
	void _on_remove_troop_member_pressed(int p_idx);
	void _on_troop_member_enemy_changed(int p_enemy_id, int p_idx);
	void _on_troop_member_x_changed(double p_val, int p_idx);
	void _on_troop_member_y_changed(double p_val, int p_idx);

	// Callbacks del Editor de Sprites y Animaciones (SpriteFrames Style - Fase 8A)
	String _get_active_anim_name() const;
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

	void _refresh_frames_timeline();
	void _refresh_anim_preview();
	void _on_frame_move_left(int p_step_idx);
	void _on_frame_move_right(int p_step_idx);
	void _on_frame_remove(int p_step_idx);
	void _on_anim_playback_settings_changed();
	void _on_anim_play_pressed();
	void _update_anim_playback(float p_delta);

	void _save_database_to_disk();
	void _load_database_from_disk();

protected:
	static void _bind_methods();
	void _notification(int p_what);

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

	static ClockGearsDatabaseDialog *get_singleton() { return singleton; }
	void sync_system_player_start(const String &p_map_path, int p_x, int p_y);

	ClockGearsDatabaseDialog();
	virtual ~ClockGearsDatabaseDialog() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}
};
