#include "clockgears_game_menu.h"

#include "../clockgears_map.h"
#include "../core/clockgears_data_manager.h"
#include "clockgears_cutscene.h"
#include "clockgears_message_window.h"
#include "clockgears_runtime.h"
#include "clockgears_title_screen.h"
#include "clockgears_window_style.h"

#include "core/config/engine.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/io/resource_loader.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"

static void _cg_clear_box_children(Node *p_parent) {
	if (!p_parent) {
		return;
	}
	for (int i = p_parent->get_child_count() - 1; i >= 0; i--) {
		Node *ch = p_parent->get_child(i);
		p_parent->remove_child(ch);
		memdelete(ch);
	}
}

static Label *_cg_make_title_label(const String &p_text, int p_font_size = 18) {
	Label *l = memnew(Label);
	l->set_text(p_text);
	l->add_theme_font_size_override("font_size", p_font_size);
	l->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.55f, 1.0f));
	return l;
}

static Button *_cg_make_menu_button(const String &p_text, const String &p_style_type, const Callable &p_pressed_cb, bool p_disabled = false) {
	Button *b = memnew(Button);
	b->set_text(p_text);
	b->set_disabled(p_disabled);
	b->set_custom_minimum_size(Size2(140, 32));
	b->set_focus_mode(Control::FOCUS_ALL);
	b->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	if (p_pressed_cb.is_valid()) {
		b->connect("pressed", p_pressed_cb);
	}
	ClockGearsWindowStyleManager::apply_button_style(b, p_style_type);
	return b;
}

void ClockGearsGameMenu::_bind_methods() {
	ClassDB::bind_method(D_METHOD("can_open_menu"), &ClockGearsGameMenu::can_open_menu);
	ClassDB::bind_method(D_METHOD("open_menu"), &ClockGearsGameMenu::open_menu);
	ClassDB::bind_method(D_METHOD("close_menu"), &ClockGearsGameMenu::close_menu);
	ClassDB::bind_method(D_METHOD("is_menu_open"), &ClockGearsGameMenu::is_menu_open);
	ClassDB::bind_method(D_METHOD("handle_input_key", "keycode"), &ClockGearsGameMenu::handle_input_key);
	ClassDB::bind_method(D_METHOD("open_subscreen", "id"), &ClockGearsGameMenu::open_subscreen);
	ClassDB::bind_method(D_METHOD("get_current_subscreen"), &ClockGearsGameMenu::get_current_subscreen);
	ClassDB::bind_method(D_METHOD("get_visible_entry_ids"), &ClockGearsGameMenu::get_visible_entry_ids);
	ClassDB::bind_method(D_METHOD("get_visible_entry_texts"), &ClockGearsGameMenu::get_visible_entry_texts);
	ClassDB::bind_method(D_METHOD("get_displayed_gold_text"), &ClockGearsGameMenu::get_displayed_gold_text);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsGameMenu::get_layout_bounds_info, DEFVAL(Vector2()));

	ClassDB::bind_method(D_METHOD("select_item", "item_key"), &ClockGearsGameMenu::select_item);
	ClassDB::bind_method(D_METHOD("use_selected_item", "target_actor_id"), &ClockGearsGameMenu::use_selected_item, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("discard_selected_item", "amount"), &ClockGearsGameMenu::discard_selected_item, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("select_equip_actor", "actor_id"), &ClockGearsGameMenu::select_equip_actor);
	ClassDB::bind_method(D_METHOD("select_equip_slot", "slot_idx"), &ClockGearsGameMenu::select_equip_slot);
	ClassDB::bind_method(D_METHOD("select_equip_candidate", "inventory_key"), &ClockGearsGameMenu::select_equip_candidate);
	ClassDB::bind_method(D_METHOD("equip_selected_piece"), &ClockGearsGameMenu::equip_selected_piece);
	ClassDB::bind_method(D_METHOD("unequip_selected_slot"), &ClockGearsGameMenu::unequip_selected_slot);
	ClassDB::bind_method(D_METHOD("get_current_equip_preview"), &ClockGearsGameMenu::get_current_equip_preview);
	ClassDB::bind_method(D_METHOD("select_status_actor", "actor_id"), &ClockGearsGameMenu::select_status_actor);
	ClassDB::bind_method(D_METHOD("get_status_summary_text"), &ClockGearsGameMenu::get_status_summary_text);
	ClassDB::bind_method(D_METHOD("save_to_slot", "slot"), &ClockGearsGameMenu::save_to_slot);
	ClassDB::bind_method(D_METHOD("confirm_return_to_title"), &ClockGearsGameMenu::confirm_return_to_title);
	ClassDB::bind_method(D_METHOD("was_return_to_title_triggered"), &ClockGearsGameMenu::was_return_to_title_triggered);

	ADD_SIGNAL(MethodInfo("menu_opened"));
	ADD_SIGNAL(MethodInfo("menu_closed"));
	ADD_SIGNAL(MethodInfo("subscreen_changed", PropertyInfo(Variant::STRING, "subscreen")));
	ADD_SIGNAL(MethodInfo("return_to_title_requested"));
}

ClockGearsGameMenu::ClockGearsGameMenu() {
	set_name("ClockGearsGameMenu");
	set_layer(105);
	set_visible(false);
	set_process_unhandled_input(true);
	_build_ui();
}

void ClockGearsGameMenu::_build_ui() {
	root_control = memnew(Control);
	root_control->set_name("GameMenuRoot");
	root_control->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root_control->set_clip_contents(true);
	root_control->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	add_child(root_control);

	dim_rect = memnew(ColorRect);
	dim_rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	dim_rect->set_color(Color(0.03f, 0.04f, 0.07f, 0.65f));
	dim_rect->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	root_control->add_child(dim_rect);

	bg_color_rect = memnew(ColorRect);
	bg_color_rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	bg_color_rect->set_color(Color(0, 0, 0, 0));
	bg_color_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	bg_color_rect->set_visible(false);
	root_control->add_child(bg_color_rect);

	bg_texture_rect = memnew(TextureRect);
	bg_texture_rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	bg_texture_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	bg_texture_rect->set_visible(false);
	root_control->add_child(bg_texture_rect);

	MarginContainer *margin = memnew(MarginContainer);
	margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	margin->add_theme_constant_override("margin_left", 24);
	margin->add_theme_constant_override("margin_right", 24);
	margin->add_theme_constant_override("margin_top", 20);
	margin->add_theme_constant_override("margin_bottom", 20);
	root_control->add_child(margin);

	content_root = memnew(Control);
	content_root->set_name("ContentRoot");
	content_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	content_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	content_root->set_clip_contents(true);
	margin->add_child(content_root);

	// --- 1. Vista Principal ---
	main_view = memnew(HBoxContainer);
	main_view->set_name("MainView");
	main_view->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_view->add_theme_constant_override("separation", 16);
	content_root->add_child(main_view);

	main_cmd_panel = memnew(PanelContainer);
	main_cmd_panel->set_name("MainCommandPanel");
	main_cmd_panel->set_custom_minimum_size(Size2(220, 0));
	main_cmd_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_view->add_child(main_cmd_panel);

	VBoxContainer *cmd_vb = memnew(VBoxContainer);
	cmd_vb->add_theme_constant_override("separation", 8);
	main_cmd_panel->add_child(cmd_vb);
	cmd_vb->add_child(_cg_make_title_label("Menú"));

	ScrollContainer *entries_scroll = memnew(ScrollContainer);
	entries_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	entries_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	cmd_vb->add_child(entries_scroll);

	entries_box = memnew(VBoxContainer);
	entries_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	entries_box->add_theme_constant_override("separation", 6);
	entries_scroll->add_child(entries_box);

	cmd_vb->add_child(memnew(HSeparator));
	gold_label = memnew(Label);
	gold_label->set_name("GoldLabel");
	gold_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	gold_label->add_theme_font_size_override("font_size", 15);
	gold_label->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.45f, 1.0f));
	cmd_vb->add_child(gold_label);

	btn_close_menu = _cg_make_menu_button("Cerrar (Esc)", "game_menu", callable_mp(this, &ClockGearsGameMenu::close_menu));
	cmd_vb->add_child(btn_close_menu);

	party_panel = memnew(PanelContainer);
	party_panel->set_name("PartyPanel");
	party_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	party_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_view->add_child(party_panel);

	VBoxContainer *party_outer = memnew(VBoxContainer);
	party_outer->add_theme_constant_override("separation", 8);
	party_panel->add_child(party_outer);
	party_outer->add_child(_cg_make_title_label("Grupo"));

	ScrollContainer *party_scroll = memnew(ScrollContainer);
	party_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	party_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	party_outer->add_child(party_scroll);

	party_list_box = memnew(VBoxContainer);
	party_list_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	party_list_box->add_theme_constant_override("separation", 8);
	party_scroll->add_child(party_list_box);

	// --- 2. Subpantalla Objetos ---
	items_panel = memnew(PanelContainer);
	items_panel->set_name("ItemsPanel");
	items_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	items_panel->set_visible(false);
	content_root->add_child(items_panel);

	VBoxContainer *items_vb = memnew(VBoxContainer);
	items_vb->add_theme_constant_override("separation", 10);
	items_panel->add_child(items_vb);

	HBoxContainer *items_hdr = memnew(HBoxContainer);
	items_hdr->add_theme_constant_override("separation", 10);
	items_vb->add_child(items_hdr);
	items_hdr->add_child(_cg_make_title_label("Objetos"));
	Label *tgt_lbl = memnew(Label);
	tgt_lbl->set_text("Objetivo:");
	items_hdr->add_child(tgt_lbl);
	items_target_option = memnew(OptionButton);
	items_target_option->set_custom_minimum_size(Size2(170, 30));
	items_target_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_items_target_selected));
	items_hdr->add_child(items_target_option);
	Control *items_sp = memnew(Control);
	items_sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_hdr->add_child(items_sp);
	btn_items_back = _cg_make_menu_button("Volver", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	btn_items_back->set_custom_minimum_size(Size2(110, 30));
	items_hdr->add_child(btn_items_back);

	HBoxContainer *items_split = memnew(HBoxContainer);
	items_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	items_split->add_theme_constant_override("separation", 12);
	items_vb->add_child(items_split);

	ScrollContainer *items_scroll = memnew(ScrollContainer);
	items_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	items_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	items_split->add_child(items_scroll);

	items_list_box = memnew(VBoxContainer);
	items_list_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_list_box->add_theme_constant_override("separation", 5);
	items_scroll->add_child(items_list_box);

	VBoxContainer *item_det = memnew(VBoxContainer);
	item_det->set_custom_minimum_size(Size2(290, 0));
	item_det->add_theme_constant_override("separation", 8);
	items_split->add_child(item_det);

	item_detail_name = _cg_make_title_label("-", 16);
	item_det->add_child(item_detail_name);
	item_detail_meta = memnew(Label);
	item_detail_meta->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_det->add_child(item_detail_meta);
	item_detail_desc = memnew(Label);
	item_detail_desc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_detail_desc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_det->add_child(item_detail_desc);
	item_feedback_label = memnew(Label);
	item_feedback_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_feedback_label->add_theme_color_override("font_color", Color(0.65f, 0.92f, 0.65f, 1.0f));
	item_det->add_child(item_feedback_label);

	HBoxContainer *item_btns = memnew(HBoxContainer);
	item_btns->add_theme_constant_override("separation", 8);
	item_det->add_child(item_btns);
	btn_item_use = _cg_make_menu_button("Usar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_use_item_pressed));
	btn_item_use->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_btns->add_child(btn_item_use);
	btn_item_discard = _cg_make_menu_button("Tirar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_discard_item_pressed));
	btn_item_discard->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_btns->add_child(btn_item_discard);

	// --- 3. Subpantalla Equipo ---
	equip_panel = memnew(PanelContainer);
	equip_panel->set_name("EquipPanel");
	equip_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	equip_panel->set_visible(false);
	content_root->add_child(equip_panel);

	VBoxContainer *equip_vb = memnew(VBoxContainer);
	equip_vb->add_theme_constant_override("separation", 8);
	equip_panel->add_child(equip_vb);

	HBoxContainer *equip_hdr = memnew(HBoxContainer);
	equip_hdr->add_theme_constant_override("separation", 10);
	equip_vb->add_child(equip_hdr);
	equip_hdr->add_child(_cg_make_title_label("Equipo"));
	equip_actor_option = memnew(OptionButton);
	equip_actor_option->set_custom_minimum_size(Size2(180, 30));
	equip_actor_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_equip_actor_selected));
	equip_hdr->add_child(equip_actor_option);
	equip_mode_label = memnew(Label);
	equip_mode_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	equip_hdr->add_child(equip_mode_label);
	btn_equip_back = _cg_make_menu_button("Volver", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	btn_equip_back->set_custom_minimum_size(Size2(110, 30));
	equip_hdr->add_child(btn_equip_back);

	HBoxContainer *equip_cols = memnew(HBoxContainer);
	equip_cols->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	equip_cols->add_theme_constant_override("separation", 10);
	equip_vb->add_child(equip_cols);

	// Columna Huecos
	VBoxContainer *col_slots = memnew(VBoxContainer);
	col_slots->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col_slots->add_theme_constant_override("separation", 6);
	equip_cols->add_child(col_slots);
	col_slots->add_child(_cg_make_title_label("Huecos", 14));
	ScrollContainer *slots_scroll = memnew(ScrollContainer);
	slots_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	slots_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	col_slots->add_child(slots_scroll);
	equip_slots_box = memnew(VBoxContainer);
	equip_slots_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	equip_slots_box->add_theme_constant_override("separation", 5);
	slots_scroll->add_child(equip_slots_box);
	btn_equip_clear = _cg_make_menu_button("Desequipar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_equip_clear_pressed));
	col_slots->add_child(btn_equip_clear);

	// Columna Piezas disponibles
	VBoxContainer *col_cand = memnew(VBoxContainer);
	col_cand->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col_cand->add_theme_constant_override("separation", 6);
	equip_cols->add_child(col_cand);
	col_cand->add_child(_cg_make_title_label("Inventario compatible", 14));
	ScrollContainer *cand_scroll = memnew(ScrollContainer);
	cand_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cand_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	col_cand->add_child(cand_scroll);
	equip_candidates_box = memnew(VBoxContainer);
	equip_candidates_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	equip_candidates_box->add_theme_constant_override("separation", 5);
	cand_scroll->add_child(equip_candidates_box);
	btn_equip_apply = _cg_make_menu_button("Equipar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_equip_apply_pressed));
	col_cand->add_child(btn_equip_apply);

	// Columna Vista previa parámetros antes/después
	VBoxContainer *col_prev = memnew(VBoxContainer);
	col_prev->set_custom_minimum_size(Size2(240, 0));
	col_prev->add_theme_constant_override("separation", 6);
	equip_cols->add_child(col_prev);
	col_prev->add_child(_cg_make_title_label("Parámetros", 14));
	ScrollContainer *prev_scroll = memnew(ScrollContainer);
	prev_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	prev_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	col_prev->add_child(prev_scroll);
	equip_preview_box = memnew(VBoxContainer);
	equip_preview_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	equip_preview_box->add_theme_constant_override("separation", 4);
	prev_scroll->add_child(equip_preview_box);
	equip_feedback_label = memnew(Label);
	equip_feedback_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	col_prev->add_child(equip_feedback_label);

	// --- 4. Subpantalla Estado ---
	status_panel = memnew(PanelContainer);
	status_panel->set_name("StatusPanel");
	status_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	status_panel->set_visible(false);
	content_root->add_child(status_panel);

	VBoxContainer *st_vb = memnew(VBoxContainer);
	st_vb->add_theme_constant_override("separation", 10);
	status_panel->add_child(st_vb);

	HBoxContainer *st_hdr = memnew(HBoxContainer);
	st_hdr->add_theme_constant_override("separation", 10);
	st_vb->add_child(st_hdr);
	st_hdr->add_child(_cg_make_title_label("Estado"));
	status_actor_option = memnew(OptionButton);
	status_actor_option->set_custom_minimum_size(Size2(190, 30));
	status_actor_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_status_actor_selected));
	st_hdr->add_child(status_actor_option);
	Control *st_sp = memnew(Control);
	st_sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	st_hdr->add_child(st_sp);
	btn_status_back = _cg_make_menu_button("Volver", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	btn_status_back->set_custom_minimum_size(Size2(110, 30));
	st_hdr->add_child(btn_status_back);

	HBoxContainer *st_cols = memnew(HBoxContainer);
	st_cols->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	st_cols->add_theme_constant_override("separation", 16);
	st_vb->add_child(st_cols);

	VBoxContainer *st_left = memnew(VBoxContainer);
	st_left->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	st_left->add_theme_constant_override("separation", 8);
	st_cols->add_child(st_left);
	status_basic_label = memnew(Label);
	status_basic_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_left->add_child(status_basic_label);
	status_states_label = memnew(Label);
	status_states_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_left->add_child(status_states_label);

	VBoxContainer *st_right = memnew(VBoxContainer);
	st_right->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	st_right->add_theme_constant_override("separation", 8);
	st_cols->add_child(st_right);
	status_params_label = memnew(Label);
	status_params_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_right->add_child(status_params_label);
	status_elements_label = memnew(Label);
	status_elements_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_right->add_child(status_elements_label);

	// --- 5. Subpantalla Guardar ---
	save_panel = memnew(PanelContainer);
	save_panel->set_name("SavePanel");
	save_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	save_panel->set_visible(false);
	content_root->add_child(save_panel);

	VBoxContainer *save_vb = memnew(VBoxContainer);
	save_vb->add_theme_constant_override("separation", 10);
	save_panel->add_child(save_vb);
	save_vb->add_child(_cg_make_title_label("Guardar partida"));
	ScrollContainer *save_scroll = memnew(ScrollContainer);
	save_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	save_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	save_vb->add_child(save_scroll);
	save_slots_box = memnew(VBoxContainer);
	save_slots_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	save_slots_box->add_theme_constant_override("separation", 6);
	save_scroll->add_child(save_slots_box);
	save_feedback_label = memnew(Label);
	save_vb->add_child(save_feedback_label);
	btn_save_back = _cg_make_menu_button("Volver", "save_list", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	save_vb->add_child(btn_save_back);

	// --- 6. Subpantalla Opciones (reutiliza ClockGearsOptionsBox) ---
	options_panel = memnew(PanelContainer);
	options_panel->set_name("OptionsPanel");
	options_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	options_panel->set_visible(false);
	content_root->add_child(options_panel);

	options_box = memnew(ClockGearsOptionsBox);
	options_box->set_name("GameMenuOptionsBox");
	options_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	options_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	options_box->connect("back_requested", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	options_panel->add_child(options_box);

	// --- 7. Confirmación Salir al Título ---
	confirm_title_panel = memnew(PanelContainer);
	confirm_title_panel->set_name("ConfirmTitlePanel");
	confirm_title_panel->set_anchors_preset(Control::PRESET_CENTER);
	confirm_title_panel->set_custom_minimum_size(Size2(380, 150));
	confirm_title_panel->set_visible(false);
	content_root->add_child(confirm_title_panel);

	VBoxContainer *ct_vb = memnew(VBoxContainer);
	ct_vb->add_theme_constant_override("separation", 12);
	confirm_title_panel->add_child(ct_vb);
	confirm_title_label = memnew(Label);
	confirm_title_label->set_text("¿Salir al título?\nEl progreso no guardado se perderá.");
	confirm_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	confirm_title_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	ct_vb->add_child(confirm_title_label);
	HBoxContainer *ct_btns = memnew(HBoxContainer);
	ct_btns->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	ct_btns->add_theme_constant_override("separation", 12);
	ct_vb->add_child(ct_btns);
	btn_confirm_title_yes = _cg_make_menu_button("Sí, salir", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_confirm_title_yes));
	ct_btns->add_child(btn_confirm_title_yes);
	btn_confirm_title_no = _cg_make_menu_button("Cancelar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	ct_btns->add_child(btn_confirm_title_no);

	_apply_unified_styles();
}

void ClockGearsGameMenu::_apply_unified_styles() {
	ClockGearsWindowStyleManager::apply_window_style(main_cmd_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(party_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(items_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(equip_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(status_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(confirm_title_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(save_panel, "save_list");
	ClockGearsWindowStyleManager::apply_window_style(options_panel, "options");
	ClockGearsWindowStyleManager::apply_button_style(btn_close_menu, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_items_back, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_item_use, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_item_discard, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_back, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_apply, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_clear, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_status_back, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_save_back, "save_list");
	ClockGearsWindowStyleManager::apply_button_style(btn_confirm_title_yes, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_confirm_title_no, "game_menu");
	if (options_box) {
		options_box->apply_button_theme();
	}
	last_applied_style_type = "game_menu";
}

void ClockGearsGameMenu::_update_responsive_layout() {
	if (!root_control) {
		return;
	}
	Size2 vsz = Size2(960, 540);
	if (is_inside_tree() && root_control && root_control->get_viewport()) {
		Size2 actual = root_control->get_viewport()->get_visible_rect().size;
		if (actual.x >= 320.0f && actual.y >= 180.0f) {
			vsz = actual;
		}
	}
	if (confirm_title_panel && content_root) {
		Size2 csz = confirm_title_panel->get_size();
		if (csz.x < 10.0f) {
			csz = confirm_title_panel->get_custom_minimum_size();
		}
		Vector2 cr_sz = content_root->get_size();
		if (cr_sz.x < 10.0f) {
			cr_sz = vsz - Vector2(48, 40);
		}
		confirm_title_panel->set_position(((cr_sz - csz) * 0.5f).max(Vector2()));
	}
	if (main_cmd_panel) {
		float left_w = CLAMP(vsz.x * 0.25f, 200.0f, 300.0f);
		main_cmd_panel->set_custom_minimum_size(Size2(left_w, 0));
	}
}

void ClockGearsGameMenu::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_apply_unified_styles();
		_update_responsive_layout();
	} else if (p_what == NOTIFICATION_WM_SIZE_CHANGED) {
		_update_responsive_layout();
	}
}

void ClockGearsGameMenu::setup_game_menu(ClockGearsEventInterpreter *p_interpreter, ClockGearsMap *p_map, ClockGearsPlayer *p_player) {
	interpreter = p_interpreter;
	current_map = p_map;
	player = p_player;
}

bool ClockGearsGameMenu::can_open_menu() const {
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		return false;
	}
	if (!interpreter) {
		return false;
	}
	if (interpreter->is_running()) {
		return false;
	}
	if (interpreter->has_active_battle()) {
		return false;
	}
	if (ClockGearsMessageWindow *mw = interpreter->get_message_window()) {
		if (mw->is_busy() || mw->is_history_open() || mw->is_visible()) {
			return false;
		}
	}
	if (ClockGearsCutscenePlayer *cp = interpreter->get_cutscene_player()) {
		if (cp->is_playing()) {
			return false;
		}
	}
	return true;
}

bool ClockGearsGameMenu::open_menu() {
	if (menu_open) {
		return true;
	}
	if (!can_open_menu()) {
		return false;
	}
	menu_open = true;
	last_returned_to_title = false;
	set_visible(true);
	_apply_unified_styles();
	open_subscreen("main");
	_update_responsive_layout();
	emit_signal("menu_opened");
	return true;
}

void ClockGearsGameMenu::close_menu() {
	if (!menu_open) {
		return;
	}
	menu_open = false;
	current_subscreen = "main";
	set_visible(false);
	emit_signal("menu_closed");
}

bool ClockGearsGameMenu::handle_input_key(Key p_keycode) {
	bool is_cancel = (p_keycode == Key::ESCAPE || p_keycode == Key::X);
	if (p_keycode == Key::M) {
		if (!menu_open) {
			return open_menu();
		}
		close_menu();
		return true;
	}
	if (!menu_open) {
		if (p_keycode == Key::ESCAPE) {
			return open_menu();
		}
		return false;
	}
	if (is_cancel) {
		if (current_subscreen != "main") {
			open_subscreen("main");
		} else {
			close_menu();
		}
		return true;
	}
	return false;
}

void ClockGearsGameMenu::unhandled_input(const Ref<InputEvent> &p_event) {
	Viewport *active_vp = root_control ? root_control->get_viewport() : nullptr;
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::ESCAPE) {
			if (handle_input_key(Key::ESCAPE)) {
				if (active_vp) {
					active_vp->set_input_as_handled();
				}
				return;
			}
		}
	}
	InputMap *im = InputMap::get_singleton();
	if (p_event.is_valid() && im && im->has_action("ui_cancel") && p_event->is_action_pressed("ui_cancel")) {
		if (handle_input_key(Key::ESCAPE)) {
			if (active_vp) {
				active_vp->set_input_as_handled();
			}
			return;
		}
	}
	bool menu_action = p_event.is_valid() && im &&
			((im->has_action("game_menu") && p_event->is_action_pressed("game_menu")) ||
					(im->has_action("ui_menu") && p_event->is_action_pressed("ui_menu")));
	if (menu_action) {
		if (!menu_open ? open_menu() : (close_menu(), true)) {
			if (active_vp) {
				active_vp->set_input_as_handled();
			}
		}
	}
}

void ClockGearsGameMenu::_refresh_background() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	String screen_key = "menu";
	if (current_subscreen == "items" || current_subscreen == "equip" || current_subscreen == "status") {
		screen_key = current_subscreen;
	}
	Dictionary bg = dm->get_game_menu_screen_background(screen_key);
	ClockGearsTitleScreen::apply_screen_background(bg_color_rect, bg_texture_rect, bg, "#0e111a");
}

bool ClockGearsGameMenu::open_subscreen(const String &p_id) {
	String target = p_id.strip_edges().to_lower();
	if (target == "magic") {
		return false; // Entrada reservada sin lógica
	}
	if (target == "save_exit") {
		target = "main";
	}
	current_subscreen = target.is_empty() ? String("main") : target;

	if (main_view) {
		main_view->set_visible(current_subscreen == "main");
	}
	if (items_panel) {
		items_panel->set_visible(current_subscreen == "items");
	}
	if (equip_panel) {
		equip_panel->set_visible(current_subscreen == "equip");
	}
	if (status_panel) {
		status_panel->set_visible(current_subscreen == "status");
	}
	if (save_panel) {
		save_panel->set_visible(current_subscreen == "save");
	}
	if (options_panel) {
		options_panel->set_visible(current_subscreen == "options");
	}
	if (confirm_title_panel) {
		confirm_title_panel->set_visible(current_subscreen == "to_title");
	}

	_refresh_background();

	if (current_subscreen == "main") {
		_refresh_main_view();
	} else if (current_subscreen == "items") {
		if (item_feedback_label) {
			item_feedback_label->set_text("");
		}
		_refresh_items_view();
	} else if (current_subscreen == "equip") {
		if (equip_feedback_label) {
			equip_feedback_label->set_text("");
		}
		_refresh_equip_view();
	} else if (current_subscreen == "status") {
		_refresh_status_view();
	} else if (current_subscreen == "save") {
		if (save_feedback_label) {
			save_feedback_label->set_text("");
		}
		_refresh_save_view();
	} else if (current_subscreen == "options" && options_box) {
		options_box->sync_from_dm();
		options_box->apply_button_theme();
	}
	_update_responsive_layout();
	emit_signal("subscreen_changed", current_subscreen);
	return true;
}

String ClockGearsGameMenu::get_displayed_gold_text() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return "0 Oro";
	}
	return vformat("%d %s", dm->get_gold(), dm->get_currency_unit());
}

void ClockGearsGameMenu::_refresh_main_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	visible_entry_ids.clear();
	visible_entry_texts.clear();
	_cg_clear_box_children(entries_box);

	Array entries = dm->get_visible_game_menu_entries();
	Button *first_btn = nullptr;
	for (int i = 0; i < entries.size(); i++) {
		if (entries[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary e = entries[i];
		String eid = String(e.get("id", ""));
		String txt = String(e.get("text", eid));
		if (eid.is_empty()) {
			continue;
		}
		visible_entry_ids.push_back(eid);
		visible_entry_texts.push_back(txt);
		Button *b = _cg_make_menu_button(txt, "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_entry_pressed).bind(eid));
		entries_box->add_child(b);
		if (!first_btn) {
			first_btn = b;
		}
	}
	if (first_btn && is_inside_tree()) {
		first_btn->grab_focus();
	}

	if (gold_label) {
		gold_label->set_text(get_displayed_gold_text());
	}

	_cg_clear_box_children(party_list_box);
	Array party = dm->get_party_actor_ids();
	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		Dictionary sheet = dm->get_actor_status_sheet(aid);
		PanelContainer *row_panel = memnew(PanelContainer);
		row_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		ClockGearsWindowStyleManager::apply_window_style(row_panel, "game_menu");
		party_list_box->add_child(row_panel);

		VBoxContainer *rvb = memnew(VBoxContainer);
		row_panel->add_child(rvb);

		HBoxContainer *top = memnew(HBoxContainer);
		rvb->add_child(top);
		Label *nl = memnew(Label);
		String cname = String(sheet.get("class_name", ""));
		nl->set_text(cname.is_empty() ? String(sheet.get("name", "")) : vformat("%s  (%s)", sheet.get("name", ""), cname));
		nl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		nl->add_theme_font_size_override("font_size", 15);
		top->add_child(nl);

		Label *lvl = memnew(Label);
		lvl->set_text(vformat("Nv. %d", int(sheet.get("level", 1))));
		top->add_child(lvl);

		Label *vitals = memnew(Label);
		vitals->set_text(vformat("HP: %d / %d    MP: %d / %d    EXP: %d",
				int(sheet.get("hp", 0)), int(sheet.get("max_hp", 1)),
				int(sheet.get("mp", 0)), int(sheet.get("max_mp", 0)),
				int64_t(sheet.get("exp", 0))));
		rvb->add_child(vitals);
	}
}

void ClockGearsGameMenu::_populate_actor_selector(OptionButton *p_opt, int &r_selected_actor_id) {
	if (!p_opt) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	p_opt->clear();
	Array party = dm->get_party_actor_ids();
	int match_idx = -1;
	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		Dictionary act = dm->get_record_by_id(CAT_ACTORS, aid);
		String aname = act.is_empty() ? vformat("Actor %d", aid) : String(act.get("name", vformat("Actor %d", aid)));
		p_opt->add_item(aname, aid);
		if (aid == r_selected_actor_id) {
			match_idx = i;
		}
	}
	if (party.size() > 0) {
		int idx = (match_idx >= 0) ? match_idx : 0;
		p_opt->select(idx);
		r_selected_actor_id = int(party[idx]);
	}
}

void ClockGearsGameMenu::_refresh_items_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	int dummy_actor = selected_status_actor_id;
	_populate_actor_selector(items_target_option, dummy_actor);
	selected_status_actor_id = dummy_actor;

	_cg_clear_box_children(items_list_box);
	Array entries = dm->get_inventory_entries();
	bool found_sel = false;
	for (int i = 0; i < entries.size(); i++) {
		Dictionary row = entries[i];
		String key = String(row.get("key", ""));
		if (key == selected_item_key) {
			found_sel = true;
		}
		if (selected_item_key.is_empty() && i == 0) {
			selected_item_key = key;
			found_sel = true;
		}

		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", 6);
		items_list_box->add_child(hb);

		TextureRect *ic = memnew(TextureRect);
		ic->set_custom_minimum_size(Size2(24, 24));
		ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		String ipath = String(row.get("icon_path", ""));
		if (!ipath.is_empty()) {
			ic->set_texture(ClockGearsTitleScreen::load_runtime_texture(ipath));
		}
		hb->add_child(ic);

		String label = vformat("%s  x%d", row.get("name", key), int(row.get("qty", 0)));
		if (bool(row.get("is_key_item", false))) {
			label += " [Clave]";
		}
		Button *b = _cg_make_menu_button(label, "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_item_row_pressed).bind(key));
		b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_child(b);
	}
	if (!found_sel) {
		selected_item_key = (entries.size() > 0) ? String(Dictionary(entries[0]).get("key", "")) : String();
	}

	Dictionary cur_row;
	for (int i = 0; i < entries.size(); i++) {
		if (String(Dictionary(entries[i]).get("key", "")) == selected_item_key) {
			cur_row = entries[i];
			break;
		}
	}
	if (cur_row.is_empty()) {
		item_detail_name->set_text("Sin objetos");
		item_detail_meta->set_text("");
		item_detail_desc->set_text("No hay objetos en el inventario.");
		btn_item_use->set_disabled(true);
		btn_item_discard->set_disabled(true);
	} else {
		bool is_key = bool(cur_row.get("is_key_item", false));
		bool usable = bool(cur_row.get("usable_in_menu", false));
		int qty = int(cur_row.get("qty", 0));
		int max_st = int(cur_row.get("max_stack", 99));
		item_detail_name->set_text(String(cur_row.get("name", selected_item_key)));
		item_detail_meta->set_text(vformat("Cantidad: %d / %d%s", qty, max_st, is_key ? "  |  Objeto clave (no se puede tirar)" : ""));
		item_detail_desc->set_text(String(cur_row.get("desc", "")));
		btn_item_use->set_disabled(!usable || qty <= 0);
		btn_item_discard->set_disabled(is_key || qty <= 0);
	}
}

void ClockGearsGameMenu::_refresh_equip_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	_populate_actor_selector(equip_actor_option, selected_equip_actor_id);
	bool arpg = dm->is_arpg_mode_active();
	if (equip_mode_label) {
		equip_mode_label->set_text(arpg ? "Modo ARPG (5 huecos)" : "Modo Por Turnos");
	}

	_cg_clear_box_children(equip_slots_box);
	Array slots = dm->get_actor_equip_slots(selected_equip_actor_id);
	if (slots.is_empty()) {
		selected_equip_slot_idx = 0;
	} else {
		selected_equip_slot_idx = CLAMP(selected_equip_slot_idx, 0, int(slots.size()) - 1);
	}

	for (int i = 0; i < slots.size(); i++) {
		Dictionary sd = slots[i];
		String sname = String(sd.get("name", vformat("Hueco %d", i + 1)));
		int pid = int(sd.get("piece_id", 0));
		int cat = dm->get_category_from_key(String(sd.get("category", "")));
		String pname = "--- Vacío ---";
		if (pid > 0 && cat >= 0) {
			Dictionary piece = dm->get_record_by_id(cat, pid);
			pname = piece.is_empty() ? vformat("#%d", pid) : String(piece.get("name", vformat("#%d", pid)));
		}
		bool locked = dm->is_actor_equip_slot_locked(selected_equip_actor_id, i);
		bool sealed = dm->is_actor_equip_slot_sealed(selected_equip_actor_id, i);
		String prefix = (i == selected_equip_slot_idx) ? "> " : "";
		String suffix = sealed ? " [Sellado]" : (locked ? " [Bloqueado]" : "");
		Button *sb = _cg_make_menu_button(vformat("%s%s: %s%s", prefix, sname, pname, suffix), "game_menu",
				callable_mp(this, &ClockGearsGameMenu::_on_equip_slot_pressed).bind(i));
		sb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		equip_slots_box->add_child(sb);
	}

	bool cur_locked = dm->is_actor_equip_slot_locked(selected_equip_actor_id, selected_equip_slot_idx);
	bool cur_sealed = dm->is_actor_equip_slot_sealed(selected_equip_actor_id, selected_equip_slot_idx);
	int cur_pid = (selected_equip_slot_idx >= 0 && selected_equip_slot_idx < slots.size())
			? int(Dictionary(slots[selected_equip_slot_idx]).get("piece_id", 0))
			: 0;
	if (btn_equip_clear) {
		btn_equip_clear->set_disabled(cur_pid <= 0 || cur_locked || cur_sealed);
	}

	_cg_clear_box_children(equip_candidates_box);
	Array cands = dm->get_equippable_pieces_for_slot(selected_equip_actor_id, selected_equip_slot_idx);
	bool found_cand = false;
	for (int i = 0; i < cands.size(); i++) {
		Dictionary crow = cands[i];
		String ckey = String(crow.get("key", ""));
		if (ckey == selected_equip_candidate_key) {
			found_cand = true;
		}
		if (selected_equip_candidate_key.is_empty() && i == 0) {
			selected_equip_candidate_key = ckey;
			found_cand = true;
		}
		String prefix = (ckey == selected_equip_candidate_key) ? "> " : "";
		Button *cb = _cg_make_menu_button(vformat("%s%s (x%d)", prefix, crow.get("name", ckey), int(crow.get("qty", 0))),
				"game_menu", callable_mp(this, &ClockGearsGameMenu::_on_equip_candidate_pressed).bind(ckey));
		cb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		equip_candidates_box->add_child(cb);
	}
	if (!found_cand) {
		selected_equip_candidate_key = (cands.size() > 0) ? String(Dictionary(cands[0]).get("key", "")) : String();
	}
	if (btn_equip_apply) {
		btn_equip_apply->set_disabled(selected_equip_candidate_key.is_empty() || cur_locked || cur_sealed);
	}
	_refresh_equip_preview_labels();
}

Dictionary ClockGearsGameMenu::get_current_equip_preview() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return Dictionary();
	}
	int cand_cat = -1;
	int cand_id = 0;
	if (!selected_equip_candidate_key.is_empty()) {
		Array cands = dm->get_equippable_pieces_for_slot(selected_equip_actor_id, selected_equip_slot_idx);
		for (int i = 0; i < cands.size(); i++) {
			Dictionary cr = cands[i];
			if (String(cr.get("key", "")) == selected_equip_candidate_key) {
				cand_cat = int(cr.get("category", -1));
				cand_id = int(cr.get("id", 0));
				break;
			}
		}
	}
	return dm->preview_equip_actor_stats(selected_equip_actor_id, selected_equip_slot_idx, cand_cat, cand_id);
}

void ClockGearsGameMenu::_refresh_equip_preview_labels() {
	if (!equip_preview_box) {
		return;
	}
	_cg_clear_box_children(equip_preview_box);
	Dictionary prev = get_current_equip_preview();
	Dictionary before = prev.get("before", Dictionary());
	Dictionary after = prev.get("after", Dictionary());
	Dictionary diff = prev.get("diff", Dictionary());

	static const char *keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	static const char *names[8] = { "HP", "MP", "ATQ", "DEF", "MAT", "MDF", "AGI", "SUE" };
	for (int i = 0; i < 8; i++) {
		int b = int(before.get(keys[i], 0));
		int a = int(after.get(keys[i], 0));
		int d = int(diff.get(keys[i], 0));
		Label *l = memnew(Label);
		String dstr = (d > 0) ? vformat("(+%d)", d) : (d < 0 ? vformat("(%d)", d) : String(""));
		l->set_text(vformat("%s: %d -> %d %s", names[i], b, a, dstr));
		if (d > 0) {
			l->add_theme_color_override("font_color", Color(0.55f, 0.95f, 0.55f, 1.0f));
		} else if (d < 0) {
			l->add_theme_color_override("font_color", Color(0.95f, 0.52f, 0.52f, 1.0f));
		}
		equip_preview_box->add_child(l);
	}
}

void ClockGearsGameMenu::_refresh_status_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	_populate_actor_selector(status_actor_option, selected_status_actor_id);
	Dictionary sheet = dm->get_actor_status_sheet(selected_status_actor_id);

	String cls_name = String(sheet.get("class_name", ""));
	if (cls_name.is_empty()) {
		cls_name = "-";
	}
	String basic = vformat("Nombre: %s\nClase: %s\nNivel: %d\nEXP: %d  (Siguiente: %d)\nHP: %d / %d\nMP: %d / %d",
			sheet.get("name", ""), cls_name, int(sheet.get("level", 1)),
			int64_t(sheet.get("exp", 0)), int64_t(sheet.get("next_level_exp", 0)),
			int(sheet.get("hp", 0)), int(sheet.get("max_hp", 1)),
			int(sheet.get("mp", 0)), int(sheet.get("max_mp", 0)));
	if (status_basic_label) {
		status_basic_label->set_text(basic);
	}

	Dictionary st = sheet.get("stats", Dictionary());
	String params = vformat("Parámetros:\nHP Máx: %d    MP Máx: %d\nATQ: %d    DEF: %d\nMAT: %d    MDF: %d\nAGI: %d    SUE: %d",
			int(st.get("hp", 0)), int(st.get("mp", 0)),
			int(st.get("atk", 0)), int(st.get("def", 0)),
			int(st.get("mat", 0)), int(st.get("mdf", 0)),
			int(st.get("agi", 0)), int(st.get("luk", 0)));
	if (status_params_label) {
		status_params_label->set_text(params);
	}

	Array states = sheet.get("states", Array());
	String st_str = "Estados activos: ";
	if (states.is_empty()) {
		st_str += "Normal";
	} else {
		for (int i = 0; i < states.size(); i++) {
			if (i > 0) {
				st_str += ", ";
			}
			st_str += String(Dictionary(states[i]).get("name", ""));
		}
	}
	if (status_states_label) {
		status_states_label->set_text(st_str);
	}

	Array elems = sheet.get("element_rates", Array());
	String el_str = "Afinidades elementales:\n";
	for (int i = 0; i < elems.size(); i++) {
		Dictionary er = elems[i];
		int pct = int(er.get("pct", 100));
		String tag = (pct > 100) ? " [Débil]" : (pct < 100 ? " [Resiste]" : "");
		el_str += vformat("- %s: %d%%%s\n", er.get("name", ""), pct, tag);
	}
	if (status_elements_label) {
		status_elements_label->set_text(el_str.strip_edges());
	}
}

void ClockGearsGameMenu::_refresh_save_view() {
	ClockGearsTitleScreen::populate_save_slots_into(save_slots_box, btn_save_back, true,
			callable_mp(this, &ClockGearsGameMenu::_on_save_slot_pressed));
	ClockGearsWindowStyleManager::apply_window_style(save_panel, "save_list");
	ClockGearsWindowStyleManager::apply_buttons_in_container(save_slots_box, "save_list");
}

Dictionary ClockGearsGameMenu::_build_runtime_save_context() const {
	Dictionary ctx;
	if (current_map) {
		ctx["map_path"] = current_map->get_scene_file_path();
		ctx["map_name"] = current_map->get_map_name();
	}
	if (player) {
		Vector2i c = player->get_grid_cell();
		Vector2 p = player->get_position();
		ctx["player_x"] = c.x;
		ctx["player_y"] = c.y;
		ctx["player_px_x"] = p.x;
		ctx["player_px_y"] = p.y;
		ctx["player_dir"] = player->get_direction();
	}
	return ctx;
}

void ClockGearsGameMenu::_on_entry_pressed(const String &p_id) {
	open_subscreen(p_id);
}

void ClockGearsGameMenu::_on_subscreen_back() {
	open_subscreen("main");
}

void ClockGearsGameMenu::_on_item_row_pressed(const String &p_key) {
	select_item(p_key);
}

void ClockGearsGameMenu::_on_items_target_selected(int p_idx) {
	if (items_target_option && p_idx >= 0) {
		selected_status_actor_id = int(items_target_option->get_item_id(p_idx));
	}
}

void ClockGearsGameMenu::_on_use_item_pressed() {
	int tgt = selected_status_actor_id;
	if (items_target_option && items_target_option->get_selected() >= 0) {
		tgt = int(items_target_option->get_item_id(items_target_option->get_selected()));
	}
	bool ok = use_selected_item(tgt);
	if (item_feedback_label) {
		item_feedback_label->set_text(ok ? "Objeto usado." : "No se puede usar sobre este objetivo.");
	}
}

void ClockGearsGameMenu::_on_discard_item_pressed() {
	bool blocked = false;
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		blocked = dm->is_key_item(selected_item_key);
	}
	bool ok = discard_selected_item(1);
	if (item_feedback_label) {
		item_feedback_label->set_text(ok ? "Objeto descartado." : (blocked ? "Los objetos clave no se pueden tirar." : "No se pudo descartar."));
	}
}

void ClockGearsGameMenu::_on_equip_actor_selected(int p_idx) {
	if (equip_actor_option && p_idx >= 0) {
		select_equip_actor(int(equip_actor_option->get_item_id(p_idx)));
	}
}

void ClockGearsGameMenu::_on_equip_slot_pressed(int p_slot_idx) {
	select_equip_slot(p_slot_idx);
}

void ClockGearsGameMenu::_on_equip_candidate_pressed(const String &p_key) {
	select_equip_candidate(p_key);
}

void ClockGearsGameMenu::_on_equip_apply_pressed() {
	bool ok = equip_selected_piece();
	if (equip_feedback_label) {
		equip_feedback_label->set_text(ok ? "Pieza equipada." : "No se pudo equipar.");
	}
}

void ClockGearsGameMenu::_on_equip_clear_pressed() {
	bool ok = unequip_selected_slot();
	if (equip_feedback_label) {
		equip_feedback_label->set_text(ok ? "Pieza desequipada y devuelta al inventario." : "No se pudo desequipar.");
	}
}

void ClockGearsGameMenu::_on_status_actor_selected(int p_idx) {
	if (status_actor_option && p_idx >= 0) {
		select_status_actor(int(status_actor_option->get_item_id(p_idx)));
	}
}

void ClockGearsGameMenu::_on_save_slot_pressed(int p_slot) {
	bool ok = save_to_slot(p_slot);
	if (save_feedback_label) {
		save_feedback_label->set_text(ok ? vformat("Partida guardada en Espacio %d.", p_slot) : "Error al guardar.");
	}
}

void ClockGearsGameMenu::_on_confirm_title_yes() {
	confirm_return_to_title();
}

bool ClockGearsGameMenu::select_item(const String &p_item_key) {
	selected_item_key = p_item_key;
	_refresh_items_view();
	return true;
}

bool ClockGearsGameMenu::use_selected_item(int p_target_actor_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_item_key.is_empty()) {
		return false;
	}
	int aid = p_target_actor_id > 0 ? p_target_actor_id : selected_status_actor_id;
	bool ok = dm->use_item_on_actor(selected_item_key, aid);
	_refresh_items_view();
	_refresh_main_view();
	return ok;
}

bool ClockGearsGameMenu::discard_selected_item(int p_amount) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_item_key.is_empty()) {
		return false;
	}
	bool ok = dm->discard_item(selected_item_key, MAX(1, p_amount));
	_refresh_items_view();
	return ok;
}

bool ClockGearsGameMenu::select_equip_actor(int p_actor_id) {
	selected_equip_actor_id = p_actor_id;
	selected_equip_candidate_key = "";
	_refresh_equip_view();
	return true;
}

bool ClockGearsGameMenu::select_equip_slot(int p_slot_idx) {
	selected_equip_slot_idx = MAX(0, p_slot_idx);
	selected_equip_candidate_key = "";
	_refresh_equip_view();
	return true;
}

bool ClockGearsGameMenu::select_equip_candidate(const String &p_inventory_key) {
	selected_equip_candidate_key = p_inventory_key;
	_refresh_equip_view();
	return true;
}

bool ClockGearsGameMenu::equip_selected_piece() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_equip_candidate_key.is_empty()) {
		return false;
	}
	bool ok = dm->equip_actor_piece(selected_equip_actor_id, selected_equip_slot_idx, selected_equip_candidate_key);
	if (ok && player) {
		player->refresh_arpg_equip_layers();
	}
	selected_equip_candidate_key = "";
	_refresh_equip_view();
	return ok;
}

bool ClockGearsGameMenu::unequip_selected_slot() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	bool ok = dm->unequip_actor_slot(selected_equip_actor_id, selected_equip_slot_idx);
	if (ok && player) {
		player->refresh_arpg_equip_layers();
	}
	_refresh_equip_view();
	return ok;
}

bool ClockGearsGameMenu::select_status_actor(int p_actor_id) {
	selected_status_actor_id = p_actor_id;
	_refresh_status_view();
	return true;
}

String ClockGearsGameMenu::get_status_summary_text() const {
	String out;
	if (status_basic_label) {
		out += status_basic_label->get_text() + "\n";
	}
	if (status_params_label) {
		out += status_params_label->get_text() + "\n";
	}
	if (status_states_label) {
		out += status_states_label->get_text() + "\n";
	}
	if (status_elements_label) {
		out += status_elements_label->get_text();
	}
	return out.strip_edges();
}

bool ClockGearsGameMenu::save_to_slot(int p_slot) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	bool ok = dm->save_game_slot(p_slot, _build_runtime_save_context());
	_refresh_save_view();
	return ok;
}

bool ClockGearsGameMenu::confirm_return_to_title() {
	last_returned_to_title = true;
	close_menu();
	emit_signal("return_to_title_requested");
	if (is_inside_tree() && get_tree()) {
		ClockGearsTitleScreen::ensure_title_screen_scene("res://title_screen.tscn");
		if (ResourceLoader::exists("res://title_screen.tscn")) {
			get_tree()->change_scene_to_file("res://title_screen.tscn");
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsGameMenu::get_layout_bounds_info(const Vector2 &p_viewport_size) const {
	Size2 vsz = p_viewport_size;
	if ((vsz.x <= 1.0f || vsz.y <= 1.0f) && is_inside_tree() && root_control && root_control->get_viewport()) {
		vsz = root_control->get_viewport()->get_visible_rect().size;
	}
	if (vsz.x <= 1.0f || vsz.y <= 1.0f) {
		vsz = Size2(960, 540);
	}
	if (root_control) {
		root_control->set_size(vsz);
	}
	const_cast<ClockGearsGameMenu *>(this)->_update_responsive_layout();

	Dictionary info;
	info["viewport"] = vsz;
	info["clips"] = false;
	info["overlaps"] = false;
	if (main_cmd_panel && party_panel && main_cmd_panel->is_visible_in_tree() && party_panel->is_visible_in_tree()) {
		Rect2 r_cmd = main_cmd_panel->get_global_rect();
		Rect2 r_party = party_panel->get_global_rect();
		Rect2 vp_rect(Vector2(0, 0), vsz);
		bool cmd_ok = vp_rect.encloses(r_cmd);
		bool party_ok = vp_rect.encloses(r_party);
		bool ov = r_cmd.intersects(r_party);
		info["clips"] = (!cmd_ok || !party_ok);
		info["overlaps"] = ov;
		info["cmd_rect"] = r_cmd;
		info["party_rect"] = r_party;
	}
	return info;
}
