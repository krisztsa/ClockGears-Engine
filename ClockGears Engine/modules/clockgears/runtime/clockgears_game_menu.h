#ifndef CLOCKGEARS_GAME_MENU_H
#define CLOCKGEARS_GAME_MENU_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
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

	// Vista principal (Comandos a la izquierda + Grupo/Oro a la derecha)
	HBoxContainer *main_view = nullptr;
	PanelContainer *main_cmd_panel = nullptr;
	VBoxContainer *entries_box = nullptr;
	Label *gold_label = nullptr;
	Button *btn_close_menu = nullptr;
	PanelContainer *party_panel = nullptr;
	VBoxContainer *party_list_box = nullptr;

	// Subpantalla Objetos
	PanelContainer *items_panel = nullptr;
	OptionButton *items_target_option = nullptr;
	VBoxContainer *items_list_box = nullptr;
	Label *item_detail_name = nullptr;
	Label *item_detail_desc = nullptr;
	Label *item_detail_meta = nullptr;
	Label *item_feedback_label = nullptr;
	Button *btn_item_use = nullptr;
	Button *btn_item_discard = nullptr;
	Button *btn_items_back = nullptr;

	// Subpantalla Equipo
	PanelContainer *equip_panel = nullptr;
	OptionButton *equip_actor_option = nullptr;
	Label *equip_mode_label = nullptr;
	VBoxContainer *equip_slots_box = nullptr;
	VBoxContainer *equip_candidates_box = nullptr;
	VBoxContainer *equip_preview_box = nullptr;
	Label *equip_feedback_label = nullptr;
	Button *btn_equip_apply = nullptr;
	Button *btn_equip_clear = nullptr;
	Button *btn_equip_back = nullptr;

	// Subpantalla Estado
	PanelContainer *status_panel = nullptr;
	OptionButton *status_actor_option = nullptr;
	Label *status_basic_label = nullptr;
	Label *status_params_label = nullptr;
	Label *status_states_label = nullptr;
	Label *status_elements_label = nullptr;
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
	PanelContainer *confirm_title_panel = nullptr;
	Label *confirm_title_label = nullptr;
	Button *btn_confirm_title_yes = nullptr;
	Button *btn_confirm_title_no = nullptr;

	bool menu_open = false;
	String current_subscreen = "main";
	PackedStringArray visible_entry_ids;
	PackedStringArray visible_entry_texts;

	String selected_item_key;
	int selected_equip_actor_id = 1;
	int selected_equip_slot_idx = 0;
	String selected_equip_candidate_key;
	int selected_status_actor_id = 1;
	bool last_returned_to_title = false;
	String last_applied_style_type;

	void _build_ui();
	void _apply_unified_styles();
	void _update_responsive_layout();
	void _refresh_background();
	void _refresh_main_view();
	void _refresh_items_view();
	void _refresh_equip_view();
	void _refresh_equip_preview_labels();
	void _refresh_status_view();
	void _refresh_save_view();

	void _populate_actor_selector(OptionButton *p_opt, int &r_selected_actor_id);
	Dictionary _build_runtime_save_context() const;

	void _on_entry_pressed(const String &p_id);
	void _on_subscreen_back();
	void _on_item_row_pressed(const String &p_key);
	void _on_items_target_selected(int p_idx);
	void _on_use_item_pressed();
	void _on_discard_item_pressed();
	void _on_equip_actor_selected(int p_idx);
	void _on_equip_slot_pressed(int p_slot_idx);
	void _on_equip_candidate_pressed(const String &p_key);
	void _on_equip_apply_pressed();
	void _on_equip_clear_pressed();
	void _on_status_actor_selected(int p_idx);
	void _on_save_slot_pressed(int p_slot);
	void _on_confirm_title_yes();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	void setup_game_menu(ClockGearsEventInterpreter *p_interpreter, ClockGearsMap *p_map, ClockGearsPlayer *p_player);

	bool can_open_menu() const;
	bool open_menu();
	void close_menu();
	bool is_menu_open() const { return menu_open; }
	bool handle_input_key(Key p_keycode);

	bool open_subscreen(const String &p_id);
	String get_current_subscreen() const { return current_subscreen; }
	PackedStringArray get_visible_entry_ids() const { return visible_entry_ids; }
	PackedStringArray get_visible_entry_texts() const { return visible_entry_texts; }
	String get_displayed_gold_text() const;
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;

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

	ClockGearsGameMenu();
};

#endif // CLOCKGEARS_GAME_MENU_H
