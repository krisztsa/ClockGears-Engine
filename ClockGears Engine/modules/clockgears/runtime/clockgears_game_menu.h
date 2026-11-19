#ifndef CLOCKGEARS_GAME_MENU_H
#define CLOCKGEARS_GAME_MENU_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/center_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/canvas_layer.h"

class ClockGearsEventInterpreter;
class ClockGearsMap;
class ClockGearsOptionsBox;
class ClockGearsPlayer;
class ClockGearsUiDesignLayer;

// ============================================================================
// ClockGearsGameMenu — Menú del Juego en Runtime (9B Tanda 3)
// Se abre con Esc / botón menú únicamente sobre el mapa cuando no hay combate,
// mensajes, elecciones ni eventos bloqueantes activos.
// ============================================================================
class ClockGearsGameMenu : public CanvasLayer {
	GDCLASS(ClockGearsGameMenu, CanvasLayer);

private:
	ClockGearsEventInterpreter *interpreter = nullptr;
	ClockGearsMap *current_map = nullptr;
	ClockGearsPlayer *player = nullptr;

	Control *root_control = nullptr;
	ColorRect *dim_rect = nullptr;
	ColorRect *bg_color_rect = nullptr;
	TextureRect *bg_texture_rect = nullptr;
	Control *content_root = nullptr;

	// Vista principal (Comandos + Tarjeta Moneda/Tiempo a la izquierda + Grupo a la derecha)
	HBoxContainer *main_view = nullptr;
	VBoxContainer *main_left_col = nullptr;
	PanelContainer *main_cmd_panel = nullptr;
	VBoxContainer *entries_box = nullptr;
	PanelContainer *currency_time_card = nullptr;
	Label *gold_label = nullptr;
	Label *playtime_label = nullptr;
	Button *btn_close_menu = nullptr;
	PanelContainer *party_panel = nullptr;
	VBoxContainer *party_list_box = nullptr;

	// Subpantalla Objetos
	PanelContainer *items_panel = nullptr;
	HBoxContainer *items_member_selector = nullptr;
	Button *btn_items_prev_actor = nullptr;
	OptionButton *items_target_option = nullptr;
	Button *btn_items_next_actor = nullptr;
	HBoxContainer *items_split = nullptr;
	ScrollContainer *items_scroll = nullptr;
	VBoxContainer *items_list_box = nullptr;
	PanelContainer *item_preview_box = nullptr;
	Control *item_preview_icon_holder = nullptr;
	TextureRect *item_preview_icon_rect = nullptr;
	PanelContainer *item_desc_box = nullptr;
	VBoxContainer *item_desc_content = nullptr;
	ScrollContainer *item_desc_scroll = nullptr;
	Label *item_detail_name = nullptr;
	Label *item_detail_desc = nullptr;
	Label *item_detail_meta = nullptr;
	Label *item_feedback_label = nullptr;
	HBoxContainer *item_action_btns = nullptr;
	Button *btn_item_use = nullptr;
	Button *btn_item_discard = nullptr;
	Button *btn_items_back = nullptr;
	VBoxContainer *item_side_panel = nullptr;

	// Subpantalla Equipo
	PanelContainer *equip_panel = nullptr;
	HBoxContainer *equip_member_selector = nullptr;
	Button *btn_equip_prev_actor = nullptr;
	OptionButton *equip_actor_option = nullptr;
	Button *btn_equip_next_actor = nullptr;
	Label *equip_mode_label = nullptr;
	VBoxContainer *equip_slots_box = nullptr;
	VBoxContainer *equip_candidates_box = nullptr;
	VBoxContainer *equip_preview_box = nullptr;
	Label *equip_feedback_label = nullptr;
	Button *btn_equip_apply = nullptr;
	Button *btn_equip_clear = nullptr;
	Button *btn_equip_back = nullptr;
	PanelContainer *equip_item_preview_box = nullptr;
	Control *equip_preview_icon_holder = nullptr;
	TextureRect *equip_preview_icon_rect = nullptr;
	PanelContainer *equip_desc_box = nullptr;
	VBoxContainer *equip_desc_content = nullptr;
	ScrollContainer *equip_desc_scroll = nullptr;
	Label *equip_detail_name = nullptr;
	Label *equip_detail_meta = nullptr;
	Label *equip_detail_desc = nullptr;
	VBoxContainer *equip_side_panel = nullptr;

	// Subpantalla Estado
	PanelContainer *status_panel = nullptr;
	HBoxContainer *status_member_selector = nullptr;
	Button *btn_status_prev_actor = nullptr;
	OptionButton *status_actor_option = nullptr;
	Button *btn_status_next_actor = nullptr;
	TextureRect *status_portrait_rect = nullptr;
	Label *status_basic_label = nullptr;
	Label *status_params_label = nullptr;
	Label *status_states_label = nullptr;
	Label *status_elements_label = nullptr;
	VBoxContainer *status_equips_box = nullptr;
	Button *btn_status_back = nullptr;

	// Subpantalla Guardar
	PanelContainer *save_panel = nullptr;
	VBoxContainer *save_slots_box = nullptr;
	Label *save_feedback_label = nullptr;
	Button *btn_save_back = nullptr;

	// Subpantalla Opciones (reutiliza ClockGearsOptionsBox de Tanda 1)
	PanelContainer *options_panel = nullptr;
	ClockGearsOptionsBox *options_box = nullptr;

	// Confirmación Salir al Título
	CenterContainer *confirm_title_overlay = nullptr;
	PanelContainer *confirm_title_panel = nullptr;
	Label *confirm_title_label = nullptr;
	Button *btn_confirm_title_yes = nullptr;
	Button *btn_confirm_title_no = nullptr;
	ClockGearsUiDesignLayer *custom_ui_layer = nullptr;

	bool menu_open = false;
	String current_subscreen = "main";
	String active_menu_id = "game_menu_main";
	Vector<String> menu_history_stack;
	bool paused_tree_by_menu = false;
	bool save_panel_load_mode = false;
	bool auto_quit_tree = true;
	bool quit_requested_flag = false;
	String last_played_menu_sound;
	int last_common_event_id = 0;
	Array last_executed_actions;
	PackedStringArray visible_entry_ids;
	PackedStringArray visible_entry_texts;

	String selected_item_key;
	int selected_equip_actor_id = 1;
	int selected_equip_slot_idx = 0;
	String selected_equip_candidate_key;
	int selected_status_actor_id = 1;
	bool last_returned_to_title = false;
	bool direct_subscreen_close_on_back = false;
	String last_applied_style_type;
	uint64_t applied_styles_generation = 0;
	double applied_styles_scale = -1.0;
	bool preview_mode = false;
	bool editor_test_popup_mode = false;
	bool preview_config_override_active = false;
	Dictionary preview_config_override;
	int nav_focus_index = 0;
	double playtime_refresh_accum = 0.0;
	float preview_rotation_deg = 0.0f;
	bool preview_is_dragging = false;
	Vector2 preview_drag_start_pos;
	float preview_drag_start_deg = 0.0f;

	void _build_ui();
	void _apply_unified_styles(bool p_force = false);
	void _update_responsive_layout();
	void _refresh_background();
	void _refresh_playtime_label();
	void _refresh_main_view();
	void _refresh_items_view();
	void _refresh_equip_view();
	void _refresh_equip_preview_labels();
	void _refresh_status_view();
	void _refresh_save_view();

	Dictionary _get_effective_game_menu_config() const;
	Ref<Texture2D> _resolve_actor_portrait_or_sprite(int p_actor_id) const;
	void _populate_actor_selector(OptionButton *p_opt, HBoxContainer *p_selector_box, int &r_selected_actor_id);
	void _cycle_actor_selector(OptionButton *p_opt, int p_delta, int &r_actor_id, const String &p_subscreen);
	void _collect_nav_buttons(Control *p_root, Vector<Button *> &r_buttons) const;
	Control *_get_active_subscreen_control() const;
	Dictionary _build_runtime_save_context() const;

	void _on_entry_pressed(const String &p_id);
	void _on_custom_ui_action(const String &p_action, const String &p_elem_id);
	String _map_subscreen_to_ui_design_id(const String &p_subscreen) const;
	String _map_menu_id_to_subscreen(const String &p_menu_id) const;
	Dictionary _get_menu_design(const String &p_menu_id) const;
	void _play_menu_sound(const String &p_event_key);
	void _update_pause_state_for_menu(const String &p_menu_id);
	void _restore_pause_state();
	bool _execute_button_step(const Dictionary &p_step, const String &p_elem_id);
	void _execute_button_actions(const Array &p_actions, const String &p_fallback_action, const String &p_elem_id);
	bool _does_trigger_match_event(const Dictionary &p_trigger, const Ref<InputEvent> &p_event) const;
	bool _context_allows_trigger(const String &p_trigger_context, const String &p_runtime_context) const;
	void _open_load_panel();
	void _on_subscreen_back();
	void _on_item_row_pressed(const String &p_key);
	void _on_item_row_hovered(const String &p_key);
	void _on_items_target_selected(int p_idx);
	void _on_items_actor_cycle(int p_delta);
	void _on_use_item_pressed();
	void _on_discard_item_pressed();
	void _on_equip_actor_selected(int p_idx);
	void _on_equip_actor_cycle(int p_delta);
	void _on_equip_slot_pressed(int p_slot_idx);
	void _on_equip_slot_hovered(int p_slot_idx);
	void _on_equip_candidate_pressed(const String &p_key);
	void _on_equip_candidate_hovered(const String &p_key);
	void _on_equip_apply_pressed();
	void _on_equip_clear_pressed();
	void _on_preview_icon_gui_input(const Ref<InputEvent> &p_event);
	void _update_item_preview_panel(const String &p_key, bool p_is_equip_subscreen = false);
	void _apply_preview_rotation(float p_deg);
	void _on_status_actor_selected(int p_idx);
	void _on_status_actor_cycle(int p_delta);
	void _on_save_slot_pressed(int p_slot);
	void _on_confirm_title_yes();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	void setup_game_menu(ClockGearsEventInterpreter *p_interpreter, ClockGearsMap *p_map, ClockGearsPlayer *p_player);

	void set_preview_mode(bool p_enabled);
	bool is_preview_mode() const { return preview_mode; }
	void set_editor_test_popup_mode(bool p_enabled);
	bool is_editor_test_popup_mode() const { return editor_test_popup_mode; }
	void apply_game_menu_config_preview(const Dictionary &p_cfg, const String &p_screen_id = "");

	bool can_open_menu() const;
	bool can_open_menu_for_context(const String &p_menu_id, const String &p_context = "map") const;
	bool open_menu();
	bool open_menu_by_id(const String &p_menu_id, bool p_from_event = false, bool p_push_history = false, bool p_bypass_access = false);
	bool open_menu_from_event(const String &p_initial_subscreen = "main", bool p_close_on_back = false);
	bool handle_trigger_input_event(const Ref<InputEvent> &p_event, const String &p_context = "map");
	bool trigger_custom_element_by_id(const String &p_elem_id);
	void refresh_window_styles();
	void close_menu();
	bool is_menu_open() const { return menu_open; }
	bool handle_input_key(Key p_keycode);

	String get_active_menu_id() const { return active_menu_id; }
	PackedStringArray get_menu_history_stack() const;
	int get_menu_history_depth() const { return menu_history_stack.size(); }
	bool is_tree_paused_by_menu() const { return paused_tree_by_menu; }
	String get_last_played_menu_sound() const { return last_played_menu_sound; }
	int get_last_common_event_id() const { return last_common_event_id; }
	Array get_last_executed_actions() const { return last_executed_actions.duplicate(true); }
	void set_auto_quit_tree(bool p_enable) { auto_quit_tree = p_enable; }
	bool was_quit_requested() const { return quit_requested_flag; }
	bool is_save_panel_load_mode() const { return save_panel_load_mode; }

	bool open_subscreen(const String &p_id);
	String get_current_subscreen() const { return current_subscreen; }
	PackedStringArray get_visible_entry_ids() const { return visible_entry_ids; }
	PackedStringArray get_visible_entry_texts() const { return visible_entry_texts; }
	String get_displayed_gold_text() const;
	String get_displayed_playtime_text() const;
	Dictionary get_party_cards_info() const;
	Dictionary get_member_selectors_info() const;
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;
	Button *get_focused_nav_button() const;
	float get_preview_rotation_deg() const { return preview_rotation_deg; }
	void simulate_preview_drag_rotation(float p_drag_dx);
	Dictionary get_item_preview_panel_info() const;

	// Acciones directas invocables desde UI y tests
	bool select_item(const String &p_item_key);
	bool use_selected_item(int p_target_actor_id = -1);
	bool discard_selected_item(int p_amount = 1);
	bool select_equip_actor(int p_actor_id);
	bool select_equip_slot(int p_slot_idx);
	bool select_equip_candidate(const String &p_inventory_key);
	bool equip_selected_piece();
	bool unequip_selected_slot();
	Dictionary get_current_equip_preview() const;
	bool select_status_actor(int p_actor_id);
	String get_status_summary_text() const;
	bool save_to_slot(int p_slot);
	bool confirm_return_to_title();
	bool was_return_to_title_triggered() const { return last_returned_to_title; }
	bool is_custom_ui_active() const;
	ClockGearsUiDesignLayer *get_custom_ui_layer() const { return custom_ui_layer; }

	ClockGearsGameMenu();
};

#endif // CLOCKGEARS_GAME_MENU_H
