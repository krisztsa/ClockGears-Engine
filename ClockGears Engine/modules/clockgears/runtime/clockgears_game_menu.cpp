#include "clockgears_game_menu.h"

#include "../clockgears_animation.h"
#include "../clockgears_map.h"
#include "../core/clockgears_data_manager.h"
#include "clockgears_cutscene.h"
#include "clockgears_message_window.h"
#include "clockgears_runtime.h"
#include "clockgears_title_screen.h"
#include "clockgears_ui_design_layer.h"
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
#include "scene/resources/atlas_texture.h"

static void _cg_clear_box_children(Node *p_parent) {
	if (!p_parent) {
		return;
	}
	for (int i = p_parent->get_child_count() - 1; i >= 0; i--) {
		Node *ch = p_parent->get_child(i);
		p_parent->remove_child(ch);
		ch->queue_free();
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
	ClassDB::bind_method(D_METHOD("set_preview_mode", "enabled"), &ClockGearsGameMenu::set_preview_mode);
	ClassDB::bind_method(D_METHOD("is_preview_mode"), &ClockGearsGameMenu::is_preview_mode);
	ClassDB::bind_method(D_METHOD("set_editor_test_popup_mode", "enabled"), &ClockGearsGameMenu::set_editor_test_popup_mode);
	ClassDB::bind_method(D_METHOD("is_editor_test_popup_mode"), &ClockGearsGameMenu::is_editor_test_popup_mode);
	ClassDB::bind_method(D_METHOD("apply_game_menu_config_preview", "config", "screen_id"), &ClockGearsGameMenu::apply_game_menu_config_preview, DEFVAL(String()));

	ClassDB::bind_method(D_METHOD("can_open_menu"), &ClockGearsGameMenu::can_open_menu);
	ClassDB::bind_method(D_METHOD("open_menu"), &ClockGearsGameMenu::open_menu);
	ClassDB::bind_method(D_METHOD("open_menu_from_event", "initial_subscreen", "close_on_back"), &ClockGearsGameMenu::open_menu_from_event, DEFVAL("main"), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("refresh_window_styles"), &ClockGearsGameMenu::refresh_window_styles);
	ClassDB::bind_method(D_METHOD("close_menu"), &ClockGearsGameMenu::close_menu);
	ClassDB::bind_method(D_METHOD("is_menu_open"), &ClockGearsGameMenu::is_menu_open);
	ClassDB::bind_method(D_METHOD("handle_input_key", "keycode"), &ClockGearsGameMenu::handle_input_key);
	ClassDB::bind_method(D_METHOD("open_subscreen", "id"), &ClockGearsGameMenu::open_subscreen);
	ClassDB::bind_method(D_METHOD("get_current_subscreen"), &ClockGearsGameMenu::get_current_subscreen);
	ClassDB::bind_method(D_METHOD("get_visible_entry_ids"), &ClockGearsGameMenu::get_visible_entry_ids);
	ClassDB::bind_method(D_METHOD("get_visible_entry_texts"), &ClockGearsGameMenu::get_visible_entry_texts);
	ClassDB::bind_method(D_METHOD("get_displayed_gold_text"), &ClockGearsGameMenu::get_displayed_gold_text);
	ClassDB::bind_method(D_METHOD("get_displayed_playtime_text"), &ClockGearsGameMenu::get_displayed_playtime_text);
	ClassDB::bind_method(D_METHOD("get_party_cards_info"), &ClockGearsGameMenu::get_party_cards_info);
	ClassDB::bind_method(D_METHOD("get_member_selectors_info"), &ClockGearsGameMenu::get_member_selectors_info);
	ClassDB::bind_method(D_METHOD("get_layout_bounds_info", "viewport_size"), &ClockGearsGameMenu::get_layout_bounds_info, DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("get_focused_nav_button"), &ClockGearsGameMenu::get_focused_nav_button);
	ClassDB::bind_method(D_METHOD("get_preview_rotation_deg"), &ClockGearsGameMenu::get_preview_rotation_deg);
	ClassDB::bind_method(D_METHOD("simulate_preview_drag_rotation", "drag_dx"), &ClockGearsGameMenu::simulate_preview_drag_rotation);
	ClassDB::bind_method(D_METHOD("get_item_preview_panel_info"), &ClockGearsGameMenu::get_item_preview_panel_info);

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
	ClassDB::bind_method(D_METHOD("is_custom_ui_active"), &ClockGearsGameMenu::is_custom_ui_active);
	ClassDB::bind_method(D_METHOD("get_custom_ui_layer"), &ClockGearsGameMenu::get_custom_ui_layer);

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

	custom_ui_layer = memnew(ClockGearsUiDesignLayer);
	custom_ui_layer->set_name("CustomGameMenuUiLayer");
	custom_ui_layer->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	custom_ui_layer->set_window_type_fallback("game_menu");
	custom_ui_layer->set_visible(false);
	custom_ui_layer->connect("action_triggered", callable_mp(this, &ClockGearsGameMenu::_on_custom_ui_action));
	root_control->add_child(custom_ui_layer);

	// --- 1. Vista Principal ---
	main_view = memnew(HBoxContainer);
	main_view->set_name("MainView");
	main_view->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_view->add_theme_constant_override("separation", 16);
	content_root->add_child(main_view);

	main_left_col = memnew(VBoxContainer);
	main_left_col->set_name("MainLeftCol");
	main_left_col->set_custom_minimum_size(Size2(220, 0));
	main_left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_left_col->add_theme_constant_override("separation", 10);
	main_view->add_child(main_left_col);

	main_cmd_panel = memnew(PanelContainer);
	main_cmd_panel->set_name("MainCommandPanel");
	main_cmd_panel->set_custom_minimum_size(Size2(220, 0));
	main_cmd_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_left_col->add_child(main_cmd_panel);

	VBoxContainer *cmd_vb = memnew(VBoxContainer);
	cmd_vb->add_theme_constant_override("separation", 8);
	main_cmd_panel->add_child(cmd_vb);
	cmd_vb->add_child(_cg_make_title_label(String::utf8("Menú")));

	ScrollContainer *entries_scroll = memnew(ScrollContainer);
	entries_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	entries_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	cmd_vb->add_child(entries_scroll);

	entries_box = memnew(VBoxContainer);
	entries_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	entries_box->add_theme_constant_override("separation", 6);
	entries_scroll->add_child(entries_box);

	cmd_vb->add_child(memnew(HSeparator));

	btn_close_menu = _cg_make_menu_button("Cerrar (Esc)", "game_menu", callable_mp(this, &ClockGearsGameMenu::close_menu));
	cmd_vb->add_child(btn_close_menu);

	// Tarjeta dedicada de Moneda + Tiempo jugado
	currency_time_card = memnew(PanelContainer);
	currency_time_card->set_name("CurrencyTimeCard");
	currency_time_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_left_col->add_child(currency_time_card);

	VBoxContainer *card_vb = memnew(VBoxContainer);
	card_vb->add_theme_constant_override("separation", 4);
	currency_time_card->add_child(card_vb);

	gold_label = memnew(Label);
	gold_label->set_name("GoldLabel");
	gold_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	gold_label->add_theme_font_size_override("font_size", 15);
	gold_label->add_theme_color_override("font_color", Color(0.95f, 0.85f, 0.45f, 1.0f));
	card_vb->add_child(gold_label);

	playtime_label = memnew(Label);
	playtime_label->set_name("PlaytimeLabel");
	playtime_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	playtime_label->add_theme_font_size_override("font_size", 13);
	playtime_label->add_theme_color_override("font_color", Color(0.80f, 0.86f, 0.94f, 1.0f));
	playtime_label->set_text("Tiempo: 00:00:00");
	card_vb->add_child(playtime_label);

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
	items_member_selector = memnew(HBoxContainer);
	items_member_selector->set_name("ItemsMemberSelector");
	items_member_selector->add_theme_constant_override("separation", 6);
	items_hdr->add_child(items_member_selector);

	Label *tgt_lbl = memnew(Label);
	tgt_lbl->set_text("Objetivo:");
	items_member_selector->add_child(tgt_lbl);
	btn_items_prev_actor = _cg_make_menu_button("<", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_items_actor_cycle).bind(-1));
	btn_items_prev_actor->set_custom_minimum_size(Size2(34, 30));
	items_member_selector->add_child(btn_items_prev_actor);
	items_target_option = memnew(OptionButton);
	items_target_option->set_custom_minimum_size(Size2(150, 30));
	items_target_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_items_target_selected));
	items_member_selector->add_child(items_target_option);
	btn_items_next_actor = _cg_make_menu_button(">", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_items_actor_cycle).bind(1));
	btn_items_next_actor->set_custom_minimum_size(Size2(34, 30));
	items_member_selector->add_child(btn_items_next_actor);
	Control *items_sp = memnew(Control);
	items_sp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_hdr->add_child(items_sp);
	btn_items_back = _cg_make_menu_button("Volver", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	btn_items_back->set_custom_minimum_size(Size2(110, 30));
	items_hdr->add_child(btn_items_back);

	items_split = memnew(HBoxContainer);
	items_split->set_name("ItemsSplit");
	items_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	items_split->add_theme_constant_override("separation", 12);
	items_vb->add_child(items_split);

	items_scroll = memnew(ScrollContainer);
	items_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	items_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	items_split->add_child(items_scroll);

	items_list_box = memnew(VBoxContainer);
	items_list_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	items_list_box->add_theme_constant_override("separation", 5);
	items_scroll->add_child(items_list_box);

	item_side_panel = memnew(VBoxContainer);
	item_side_panel->set_name("ItemSidePanel");
	item_side_panel->set_custom_minimum_size(Size2(300, 0));
	item_side_panel->add_theme_constant_override("separation", 8);
	items_split->add_child(item_side_panel);

	item_preview_box = memnew(PanelContainer);
	item_preview_box->set_name("ItemPreviewBox");
	item_preview_box->set_custom_minimum_size(Size2(300, 140));
	item_side_panel->add_child(item_preview_box);

	CenterContainer *prev_cnt = memnew(CenterContainer);
	prev_cnt->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	item_preview_box->add_child(prev_cnt);

	item_preview_icon_holder = memnew(Control);
	item_preview_icon_holder->set_name("ItemPreviewIconHolder");
	item_preview_icon_holder->set_custom_minimum_size(Size2(64, 64));
	item_preview_icon_holder->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	item_preview_icon_holder->connect("gui_input", callable_mp(this, &ClockGearsGameMenu::_on_preview_icon_gui_input));
	prev_cnt->add_child(item_preview_icon_holder);

	item_preview_icon_rect = memnew(TextureRect);
	item_preview_icon_rect->set_name("ItemPreviewIconRect");
	item_preview_icon_rect->set_custom_minimum_size(Size2(64, 64));
	item_preview_icon_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	item_preview_icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	item_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	item_preview_icon_rect->set_pivot_offset(Vector2(32, 32));
	item_preview_icon_holder->add_child(item_preview_icon_rect);

	item_desc_box = memnew(PanelContainer);
	item_desc_box->set_name("ItemDescBox");
	item_desc_box->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_desc_box->set_custom_minimum_size(Size2(300, 180));
	item_side_panel->add_child(item_desc_box);

	item_desc_content = memnew(VBoxContainer);
	item_desc_content->add_theme_constant_override("separation", 6);
	item_desc_box->add_child(item_desc_content);

	item_detail_name = _cg_make_title_label("-", 16);
	item_desc_content->add_child(item_detail_name);
	item_detail_meta = memnew(Label);
	item_detail_meta->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_desc_content->add_child(item_detail_meta);

	item_desc_scroll = memnew(ScrollContainer);
	item_desc_scroll->set_name("ItemDescScroll");
	item_desc_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_desc_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	item_desc_content->add_child(item_desc_scroll);

	item_detail_desc = memnew(Label);
	item_detail_desc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_detail_desc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_desc_scroll->add_child(item_detail_desc);

	item_feedback_label = memnew(Label);
	item_feedback_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	item_feedback_label->add_theme_color_override("font_color", Color(0.65f, 0.92f, 0.65f, 1.0f));
	item_desc_content->add_child(item_feedback_label);

	item_action_btns = memnew(HBoxContainer);
	item_action_btns->add_theme_constant_override("separation", 8);
	item_desc_content->add_child(item_action_btns);

	btn_item_use = _cg_make_menu_button(String::utf8("Usar"), "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_use_item_pressed));
	btn_item_use->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_action_btns->add_child(btn_item_use);

	btn_item_discard = _cg_make_menu_button(String::utf8("Tirar"), "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_discard_item_pressed));
	btn_item_discard->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_action_btns->add_child(btn_item_discard);

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
	equip_member_selector = memnew(HBoxContainer);
	equip_member_selector->set_name("EquipMemberSelector");
	equip_member_selector->add_theme_constant_override("separation", 6);
	equip_hdr->add_child(equip_member_selector);

	btn_equip_prev_actor = _cg_make_menu_button("<", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_equip_actor_cycle).bind(-1));
	btn_equip_prev_actor->set_custom_minimum_size(Size2(34, 30));
	equip_member_selector->add_child(btn_equip_prev_actor);
	equip_actor_option = memnew(OptionButton);
	equip_actor_option->set_custom_minimum_size(Size2(150, 30));
	equip_actor_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_equip_actor_selected));
	equip_member_selector->add_child(equip_actor_option);
	btn_equip_next_actor = _cg_make_menu_button(">", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_equip_actor_cycle).bind(1));
	btn_equip_next_actor->set_custom_minimum_size(Size2(34, 30));
	equip_member_selector->add_child(btn_equip_next_actor);
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

	// Columna Vista previa objeto + parámetros antes/después
	VBoxContainer *col_prev = memnew(VBoxContainer);
	col_prev->set_custom_minimum_size(Size2(260, 0));
	col_prev->add_theme_constant_override("separation", 6);
	equip_cols->add_child(col_prev);

	equip_item_preview_box = memnew(PanelContainer);
	equip_item_preview_box->set_name("EquipItemPreviewBox");
	equip_item_preview_box->set_custom_minimum_size(Size2(260, 100));
	CenterContainer *eq_prev_cnt = memnew(CenterContainer);
	eq_prev_cnt->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	equip_item_preview_box->add_child(eq_prev_cnt);

	equip_preview_icon_holder = memnew(Control);
	equip_preview_icon_holder->set_name("EquipPreviewIconHolder");
	equip_preview_icon_holder->set_custom_minimum_size(Size2(64, 64));
	equip_preview_icon_holder->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	equip_preview_icon_holder->connect("gui_input", callable_mp(this, &ClockGearsGameMenu::_on_preview_icon_gui_input));
	eq_prev_cnt->add_child(equip_preview_icon_holder);

	equip_preview_icon_rect = memnew(TextureRect);
	equip_preview_icon_rect->set_name("EquipPreviewIconRect");
	equip_preview_icon_rect->set_custom_minimum_size(Size2(64, 64));
	equip_preview_icon_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	equip_preview_icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	equip_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	equip_preview_icon_rect->set_pivot_offset(Vector2(32, 32));
	equip_preview_icon_holder->add_child(equip_preview_icon_rect);
	col_prev->add_child(equip_item_preview_box);

	equip_desc_box = memnew(PanelContainer);
	equip_desc_box->set_name("EquipDescBox");
	equip_desc_box->set_custom_minimum_size(Size2(260, 70));
	equip_desc_content = memnew(VBoxContainer);
	equip_desc_content->add_theme_constant_override("separation", 2);
	equip_desc_box->add_child(equip_desc_content);

	equip_detail_name = _cg_make_title_label("-", 14);
	equip_desc_content->add_child(equip_detail_name);
	equip_detail_meta = memnew(Label);
	equip_detail_meta->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	equip_desc_content->add_child(equip_detail_meta);
	equip_desc_scroll = memnew(ScrollContainer);
	equip_desc_scroll->set_name("EquipDescScroll");
	equip_desc_scroll->set_custom_minimum_size(Size2(0, 40));
	equip_desc_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	equip_desc_content->add_child(equip_desc_scroll);
	equip_detail_desc = memnew(Label);
	equip_detail_desc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	equip_desc_scroll->add_child(equip_detail_desc);
	col_prev->add_child(equip_desc_box);

	col_prev->add_child(_cg_make_title_label(String::utf8("Parámetros"), 14));
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
	status_member_selector = memnew(HBoxContainer);
	status_member_selector->set_name("StatusMemberSelector");
	status_member_selector->add_theme_constant_override("separation", 6);
	st_hdr->add_child(status_member_selector);

	btn_status_prev_actor = _cg_make_menu_button("<", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_status_actor_cycle).bind(-1));
	btn_status_prev_actor->set_custom_minimum_size(Size2(34, 30));
	status_member_selector->add_child(btn_status_prev_actor);
	status_actor_option = memnew(OptionButton);
	status_actor_option->set_custom_minimum_size(Size2(160, 30));
	status_actor_option->connect("item_selected", callable_mp(this, &ClockGearsGameMenu::_on_status_actor_selected));
	status_member_selector->add_child(status_actor_option);
	btn_status_next_actor = _cg_make_menu_button(">", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_status_actor_cycle).bind(1));
	btn_status_next_actor->set_custom_minimum_size(Size2(34, 30));
	status_member_selector->add_child(btn_status_next_actor);
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

	HBoxContainer *st_top_row = memnew(HBoxContainer);
	st_top_row->add_theme_constant_override("separation", 12);
	st_left->add_child(st_top_row);

	status_portrait_rect = memnew(TextureRect);
	status_portrait_rect->set_name("StatusPortrait");
	status_portrait_rect->set_custom_minimum_size(Size2(72, 72));
	status_portrait_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	status_portrait_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	status_portrait_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	status_portrait_rect->set_visible(false);
	st_top_row->add_child(status_portrait_rect);

	status_basic_label = memnew(Label);
	status_basic_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	status_basic_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_top_row->add_child(status_basic_label);
	status_states_label = memnew(Label);
	status_states_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	st_left->add_child(status_states_label);

	Label *eq_title = _cg_make_title_label(String::utf8("Equipo equipado:"), 14);
	st_left->add_child(eq_title);
	ScrollContainer *st_eq_scroll = memnew(ScrollContainer);
	st_eq_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	st_eq_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	st_left->add_child(st_eq_scroll);
	status_equips_box = memnew(VBoxContainer);
	status_equips_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	status_equips_box->add_theme_constant_override("separation", 4);
	st_eq_scroll->add_child(status_equips_box);

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
	confirm_title_overlay = memnew(CenterContainer);
	confirm_title_overlay->set_name("ConfirmTitleOverlay");
	confirm_title_overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	confirm_title_overlay->set_visible(false);
	content_root->add_child(confirm_title_overlay);

	confirm_title_panel = memnew(PanelContainer);
	confirm_title_panel->set_name("ConfirmTitlePanel");
	confirm_title_panel->set_custom_minimum_size(Size2(380, 150));
	confirm_title_panel->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	confirm_title_panel->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	confirm_title_panel->set_visible(false);
	confirm_title_overlay->add_child(confirm_title_panel);

	VBoxContainer *ct_vb = memnew(VBoxContainer);
	ct_vb->add_theme_constant_override("separation", 12);
	confirm_title_panel->add_child(ct_vb);
	confirm_title_label = memnew(Label);
	confirm_title_label->set_text(String::utf8("¿Salir al título?\nEl progreso no guardado se perderá."));
	confirm_title_label->set_custom_minimum_size(Size2(340, 0));
	confirm_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	confirm_title_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	ct_vb->add_child(confirm_title_label);
	HBoxContainer *ct_btns = memnew(HBoxContainer);
	ct_btns->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	ct_btns->add_theme_constant_override("separation", 12);
	ct_vb->add_child(ct_btns);
	btn_confirm_title_yes = _cg_make_menu_button(String::utf8("Sí, salir"), "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_confirm_title_yes));
	ct_btns->add_child(btn_confirm_title_yes);
	btn_confirm_title_no = _cg_make_menu_button("Cancelar", "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_subscreen_back));
	ct_btns->add_child(btn_confirm_title_no);

	_apply_unified_styles();
}

void ClockGearsGameMenu::_apply_unified_styles() {
	ClockGearsWindowStyleManager::apply_window_style(main_cmd_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(currency_time_card, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(party_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(items_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(equip_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(status_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(confirm_title_panel, "game_menu");
	ClockGearsWindowStyleManager::apply_window_style(save_panel, "save_list");
	ClockGearsWindowStyleManager::apply_window_style(options_panel, "options");

	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	String prev_st = String(prev_cfg.get("preview_window_style", "")).strip_edges();
	if (prev_st.is_empty()) {
		prev_st = "object_preview";
	}
	String desc_st = String(prev_cfg.get("desc_window_style", "")).strip_edges();
	if (desc_st.is_empty()) {
		desc_st = "object_desc";
	}
	if (item_preview_box) {
		ClockGearsWindowStyleManager::apply_window_style(item_preview_box, prev_st);
	}
	if (item_desc_box) {
		ClockGearsWindowStyleManager::apply_window_style(item_desc_box, desc_st);
	}
	if (equip_item_preview_box) {
		ClockGearsWindowStyleManager::apply_window_style(equip_item_preview_box, prev_st);
	}
	if (equip_desc_box) {
		ClockGearsWindowStyleManager::apply_window_style(equip_desc_box, desc_st);
	}
	ClockGearsWindowStyleManager::apply_button_style(btn_close_menu, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_items_prev_actor, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_items_next_actor, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_items_back, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_item_use, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_item_discard, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_prev_actor, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_next_actor, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_back, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_apply, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_equip_clear, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_status_prev_actor, "game_menu");
	ClockGearsWindowStyleManager::apply_button_style(btn_status_next_actor, "game_menu");
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
	if (confirm_title_overlay && confirm_title_panel && content_root) {
		confirm_title_panel->reset_size();
		Size2 csz = confirm_title_panel->get_combined_minimum_size();
		confirm_title_panel->set_size(csz);
	}
	float left_w = CLAMP(vsz.x * 0.27f, 220.0f, 320.0f);
	if (main_left_col) {
		main_left_col->set_custom_minimum_size(Size2(left_w, 0));
	}
	if (main_cmd_panel) {
		main_cmd_panel->set_custom_minimum_size(Size2(left_w, 0));
	}
}

void ClockGearsGameMenu::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		set_process(true);
		_apply_unified_styles();
		_update_responsive_layout();
	} else if (p_what == NOTIFICATION_WM_SIZE_CHANGED) {
		_update_responsive_layout();
	} else if (p_what == NOTIFICATION_PROCESS) {
		if (menu_open && !preview_mode && current_subscreen == "main") {
			playtime_refresh_accum += get_process_delta_time();
			if (playtime_refresh_accum >= 0.5) {
				playtime_refresh_accum = 0.0;
				_refresh_playtime_label();
			}
		}
	}
}

void ClockGearsGameMenu::setup_game_menu(ClockGearsEventInterpreter *p_interpreter, ClockGearsMap *p_map, ClockGearsPlayer *p_player) {
	interpreter = p_interpreter;
	current_map = p_map;
	player = p_player;
}

void ClockGearsGameMenu::set_preview_mode(bool p_enabled) {
	preview_mode = p_enabled;
	if (preview_mode) {
		set_process_unhandled_input(false);
		menu_open = true;
		set_visible(true);
		_apply_unified_styles();
		open_subscreen(current_subscreen.is_empty() ? String("main") : current_subscreen);
		_update_responsive_layout();
	} else {
		set_process_unhandled_input(true);
	}
}

void ClockGearsGameMenu::set_editor_test_popup_mode(bool p_enabled) {
	editor_test_popup_mode = p_enabled;
	if (editor_test_popup_mode) {
		set_process_unhandled_input(true);
	}
}

void ClockGearsGameMenu::apply_game_menu_config_preview(const Dictionary &p_cfg, const String &p_screen_id) {
	preview_config_override = ClockGearsDataManager::normalize_game_menu_config(p_cfg);
	preview_config_override_active = true;
	String target = p_screen_id.strip_edges().to_lower();
	if (target == "menu") {
		target = "main";
	}
	if (!target.is_empty()) {
		open_subscreen(target);
	} else {
		open_subscreen(current_subscreen.is_empty() ? String("main") : current_subscreen);
	}
}

bool ClockGearsGameMenu::can_open_menu() const {
	if (editor_test_popup_mode) {
		return true;
	}
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
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		if (dm->is_game_menu_disabled()) {
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
	direct_subscreen_close_on_back = false;
	menu_open = true;
	last_returned_to_title = false;
	set_visible(true);
	_apply_unified_styles();
	open_subscreen("main");
	_update_responsive_layout();
	emit_signal("menu_opened");
	return true;
}

bool ClockGearsGameMenu::open_menu_from_event(const String &p_initial_subscreen, bool p_close_on_back) {
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		return false;
	}
	if (interpreter && interpreter->has_active_battle()) {
		return false;
	}
	direct_subscreen_close_on_back = p_close_on_back;
	menu_open = true;
	last_returned_to_title = false;
	set_visible(true);
	_apply_unified_styles();
	String target = p_initial_subscreen.strip_edges().to_lower();
	open_subscreen(target.is_empty() ? String("main") : target);
	_update_responsive_layout();
	emit_signal("menu_opened");
	return true;
}

void ClockGearsGameMenu::refresh_window_styles() {
	_apply_unified_styles();
	if (menu_open) {
		_refresh_main_view();
		if (current_subscreen == "save") {
			_refresh_save_view();
		} else if (current_subscreen == "items") {
			_refresh_items_view();
		} else if (current_subscreen == "equip") {
			_refresh_equip_view();
		} else if (current_subscreen == "status") {
			_refresh_status_view();
		}
	}
}

void ClockGearsGameMenu::close_menu() {
	if (!menu_open) {
		return;
	}
	preview_rotation_deg = 0.0f;
	_apply_preview_rotation(0.0f);
	preview_is_dragging = false;
	menu_open = false;
	direct_subscreen_close_on_back = false;
	current_subscreen = "main";
	set_visible(false);
	emit_signal("menu_closed");
}

Control *ClockGearsGameMenu::_get_active_subscreen_control() const {
	if (current_subscreen == "items") return items_panel;
	if (current_subscreen == "equip") return equip_panel;
	if (current_subscreen == "status") return status_panel;
	if (current_subscreen == "save") return save_panel;
	if (current_subscreen == "options") return options_panel;
	if (current_subscreen == "to_title") return confirm_title_panel;
	return main_view;
}

void ClockGearsGameMenu::_collect_nav_buttons(Control *p_root, Vector<Button *> &r_buttons) const {
	if (custom_ui_layer && custom_ui_layer->is_visible() && p_root != confirm_title_panel && p_root != save_panel && p_root != options_panel) {
		r_buttons = custom_ui_layer->get_focus_ordered_buttons();
		return;
	}
	if (!p_root || !p_root->is_visible()) {
		return;
	}
	Button *b = Object::cast_to<Button>(p_root);
	if (b && !Object::cast_to<OptionButton>(b)) {
		if (!b->is_disabled() && b->get_focus_mode() != Control::FOCUS_NONE) {
			r_buttons.push_back(b);
		}
		return;
	}
	for (int i = 0; i < p_root->get_child_count(); i++) {
		Control *ch = Object::cast_to<Control>(p_root->get_child(i));
		if (ch) {
			_collect_nav_buttons(ch, r_buttons);
		}
	}
}

Button *ClockGearsGameMenu::get_focused_nav_button() const {
	Vector<Button *> btns;
	_collect_nav_buttons(_get_active_subscreen_control(), btns);
	if (btns.is_empty()) {
		return nullptr;
	}
	if (is_inside_tree() && root_control && root_control->get_viewport()) {
		Control *fo = root_control->get_viewport()->gui_get_focus_owner();
		Button *fbtn = Object::cast_to<Button>(fo);
		if (fbtn && btns.has(fbtn)) {
			return fbtn;
		}
	}
	int idx = CLAMP(nav_focus_index, 0, int(btns.size()) - 1);
	return btns[idx];
}

bool ClockGearsGameMenu::handle_input_key(Key p_keycode) {
	if (preview_mode) {
		return false;
	}
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
		if (current_subscreen != "main" && !direct_subscreen_close_on_back) {
			open_subscreen("main");
		} else {
			close_menu();
		}
		return true;
	}
	Vector<Button *> btns;
	_collect_nav_buttons(_get_active_subscreen_control(), btns);
	if (btns.is_empty()) {
		return false;
	}
	int cur_idx = nav_focus_index;
	if (is_inside_tree() && root_control && root_control->get_viewport()) {
		Control *fo = root_control->get_viewport()->gui_get_focus_owner();
		Button *fbtn = Object::cast_to<Button>(fo);
		if (fbtn) {
			int found = btns.find(fbtn);
			if (found >= 0) {
				cur_idx = found;
			}
		}
	}
	cur_idx = CLAMP(cur_idx, 0, int(btns.size()) - 1);
	if (p_keycode == Key::DOWN || p_keycode == Key::RIGHT || p_keycode == Key::S || p_keycode == Key::D) {
		nav_focus_index = (cur_idx + 1) % int(btns.size());
		if (is_inside_tree()) {
			btns[nav_focus_index]->grab_focus();
		}
		return true;
	}
	if (p_keycode == Key::UP || p_keycode == Key::LEFT || p_keycode == Key::W || p_keycode == Key::A) {
		nav_focus_index = (cur_idx - 1 + int(btns.size())) % int(btns.size());
		if (is_inside_tree()) {
			btns[nav_focus_index]->grab_focus();
		}
		return true;
	}
	if (p_keycode == Key::ENTER || p_keycode == Key::KP_ENTER || p_keycode == Key::SPACE) {
		Button *target = btns[cur_idx];
		if (target && !target->is_disabled()) {
			target->emit_signal("pressed");
			return true;
		}
	}
	return false;
}

void ClockGearsGameMenu::unhandled_input(const Ref<InputEvent> &p_event) {
	if (preview_mode) {
		return;
	}
	Viewport *active_vp = root_control ? root_control->get_viewport() : nullptr;
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		Key kc = k->get_keycode();
		if (kc == Key::ESCAPE || kc == Key::X || kc == Key::M ||
				(menu_open && (kc == Key::UP || kc == Key::DOWN || kc == Key::LEFT || kc == Key::RIGHT ||
						kc == Key::ENTER || kc == Key::KP_ENTER || kc == Key::SPACE))) {
			if (handle_input_key(kc)) {
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
	if (menu_open && p_event.is_valid() && im) {
		if (im->has_action("ui_down") && p_event->is_action_pressed("ui_down")) {
			if (handle_input_key(Key::DOWN) && active_vp) active_vp->set_input_as_handled();
			return;
		}
		if (im->has_action("ui_up") && p_event->is_action_pressed("ui_up")) {
			if (handle_input_key(Key::UP) && active_vp) active_vp->set_input_as_handled();
			return;
		}
		if (im->has_action("ui_accept") && p_event->is_action_pressed("ui_accept")) {
			if (handle_input_key(Key::ENTER) && active_vp) active_vp->set_input_as_handled();
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

Dictionary ClockGearsGameMenu::_get_effective_game_menu_config() const {
	if (preview_config_override_active) {
		return ClockGearsDataManager::normalize_game_menu_config(preview_config_override);
	}
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		return ClockGearsDataManager::normalize_game_menu_config(dm->get_game_menu_config());
	}
	return ClockGearsDataManager::get_default_game_menu_config();
}

void ClockGearsGameMenu::_refresh_background() {
	String screen_key = "menu";
	if (current_subscreen == "items" || current_subscreen == "equip" || current_subscreen == "status") {
		screen_key = current_subscreen;
	}
	Dictionary cfg = _get_effective_game_menu_config();
	Dictionary bgs = cfg.get("backgrounds", Dictionary());
	Dictionary bg = bgs.has(screen_key) ? Dictionary(bgs[screen_key]) : Dictionary();
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
	preview_rotation_deg = 0.0f;
	_apply_preview_rotation(0.0f);
	preview_is_dragging = false;

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
	if (confirm_title_overlay) {
		confirm_title_overlay->set_visible(current_subscreen == "to_title");
	}
	if (confirm_title_panel) {
		confirm_title_panel->set_visible(current_subscreen == "to_title");
	}

	_refresh_background();
	nav_focus_index = 0;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String ui_sid = _map_subscreen_to_ui_design_id(current_subscreen);
	bool use_custom = (dm && !ui_sid.is_empty() && dm->has_custom_ui_design(ui_sid));
	if (use_custom && custom_ui_layer) {
		if (main_view) main_view->set_visible(false);
		if (items_panel) items_panel->set_visible(false);
		if (equip_panel) equip_panel->set_visible(false);
		if (status_panel) status_panel->set_visible(false);
		custom_ui_layer->set_window_type_fallback("game_menu");
		custom_ui_layer->set_preview_mode(preview_mode);
		custom_ui_layer->apply_screen_design(ui_sid);
		custom_ui_layer->set_visible(true);
	} else {
		if (custom_ui_layer) {
			custom_ui_layer->set_visible(false);
			custom_ui_layer->clear_elements();
		}
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
		}
	}

	if (current_subscreen == "save") {
		if (save_feedback_label) {
			save_feedback_label->set_text("");
		}
		_refresh_save_view();
	} else if (current_subscreen == "options" && options_box) {
		options_box->sync_from_dm();
		options_box->apply_button_theme();
	}
	_update_responsive_layout();

	Vector<Button *> nav_btns;
	_collect_nav_buttons(_get_active_subscreen_control(), nav_btns);
	if (!nav_btns.is_empty() && is_inside_tree()) {
		nav_btns[0]->grab_focus();
	}

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

String ClockGearsGameMenu::get_displayed_playtime_text() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	double secs = dm ? dm->get_runtime_clock_sec() : 0.0;
	return vformat("Tiempo: %s", ClockGearsDataManager::format_playtime_hms(secs));
}

void ClockGearsGameMenu::_refresh_playtime_label() {
	if (playtime_label) {
		playtime_label->set_text(get_displayed_playtime_text());
	}
}

Ref<Texture2D> ClockGearsGameMenu::_resolve_actor_portrait_or_sprite(int p_actor_id) const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return Ref<Texture2D>();
	}
	Dictionary actor_rec = dm->get_record_by_id(CAT_ACTORS, p_actor_id);
	if (actor_rec.is_empty()) {
		return Ref<Texture2D>();
	}
	// 1) Retrato explícito si existe
	for (const char *pkey : { "portrait_path", "face_path" }) {
		String p = String(actor_rec.get(pkey, "")).strip_edges();
		if (!p.is_empty()) {
			Ref<Texture2D> tex = ClockGearsTitleScreen::load_runtime_texture(p);
			if (tex.is_valid()) {
				return tex;
			}
		}
	}
	// 2) Primer frame del sprite del actor (sprite_data o graphic_path)
	ClockGearsSpriteSheetData sheet;
	if (actor_rec.has("sprite_data") && actor_rec["sprite_data"].get_type() == Variant::DICTIONARY) {
		sheet.from_dict(Dictionary(actor_rec["sprite_data"]));
	}
	ClockGearsSpriteAnimation anim;
	bool has_anim = false;
	for (const char *slot : { "idle_down", "idle", "walk_down", "walk" }) {
		if (sheet.animations.has(slot) && sheet.animations[slot].get_frame_count() > 0) {
			anim = sheet.animations[slot];
			has_anim = true;
			break;
		}
	}
	if (!has_anim) {
		for (const KeyValue<String, ClockGearsSpriteAnimation> &E : sheet.animations) {
			if (E.value.get_frame_count() > 0) {
				anim = E.value;
				has_anim = true;
				break;
			}
		}
	}
	if (has_anim) {
		ClockGearsAnimationFrame fr = anim.get_frame(0, 0, 0);
		String spath = !fr.sheet_path.is_empty() ? fr.sheet_path : anim.spritesheet_path;
		if (spath.is_empty()) {
			spath = String(actor_rec.get("graphic_path", ""));
		}
		Ref<Texture2D> base_tex = ClockGearsTitleScreen::load_runtime_texture(spath);
		if (base_tex.is_valid()) {
			ClockGearsAnimationFrame fr_tex = anim.get_frame(0, base_tex->get_width(), base_tex->get_height());
			Rect2 reg = fr_tex.region;
			if (reg.size.x > 0 && reg.size.y > 0 &&
					reg.position.x + reg.size.x <= base_tex->get_width() &&
					reg.position.y + reg.size.y <= base_tex->get_height()) {
				Ref<AtlasTexture> atlas;
				atlas.instantiate();
				atlas->set_atlas(base_tex);
				atlas->set_region(reg);
				return atlas;
			}
			return base_tex;
		}
	}
	String gp = String(actor_rec.get("graphic_path", "")).strip_edges();
	if (!gp.is_empty()) {
		Ref<Texture2D> base_tex = ClockGearsTitleScreen::load_runtime_texture(gp);
		if (base_tex.is_valid()) {
			int cols = MAX(1, sheet.cols > 0 ? sheet.cols : 4);
			int rows = (base_tex->get_height() >= base_tex->get_width() * 2) ? 8 : 4;
			int fw = base_tex->get_width() / cols;
			int fh = base_tex->get_height() / rows;
			if (fw > 0 && fh > 0 && base_tex->get_width() >= cols && base_tex->get_height() >= rows) {
				Ref<AtlasTexture> atlas;
				atlas.instantiate();
				atlas->set_atlas(base_tex);
				atlas->set_region(Rect2(0, 0, fw, fh));
				return atlas;
			}
			return base_tex;
		}
	}
	return Ref<Texture2D>();
}

void ClockGearsGameMenu::_refresh_main_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	visible_entry_ids.clear();
	visible_entry_texts.clear();
	_cg_clear_box_children(entries_box);

	Dictionary cfg = _get_effective_game_menu_config();
	Array raw_entries = cfg.get("entries", Array());
	Vector<Dictionary> sorted_entries;
	for (int i = 0; i < raw_entries.size(); i++) {
		if (raw_entries[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary e = raw_entries[i];
		String eid = String(e.get("id", ""));
		if (eid.is_empty() || eid == "magic" || bool(e.get("reserved", false)) || !bool(e.get("enabled", true))) {
			continue;
		}
		sorted_entries.push_back(e);
	}
	struct EntryOrderCmp {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("order", 0)) < int(b.get("order", 0));
		}
	};
	sorted_entries.sort_custom<EntryOrderCmp>();

	Button *first_btn = nullptr;
	for (int i = 0; i < sorted_entries.size(); i++) {
		const Dictionary &e = sorted_entries[i];
		String eid = String(e.get("id", ""));
		String txt = String(e.get("text", eid));
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
	_refresh_playtime_label();

	_cg_clear_box_children(party_list_box);
	if (!dm) {
		return;
	}
	Array party = dm->get_party_actor_ids();
	if (party.is_empty() && preview_mode) {
		Array actors = dm->get_category_array(CAT_ACTORS);
		if (!actors.is_empty() && actors[0].get_type() == Variant::DICTIONARY) {
			party.push_back(int(Dictionary(actors[0]).get("id", 1)));
		}
	}
	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		Dictionary sheet = dm->get_actor_status_sheet(aid);
		PanelContainer *row_panel = memnew(PanelContainer);
		row_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row_panel->set_meta("cg_actor_id", aid);
		ClockGearsWindowStyleManager::apply_window_style(row_panel, "game_menu");
		party_list_box->add_child(row_panel);

		HBoxContainer *card_hb = memnew(HBoxContainer);
		card_hb->add_theme_constant_override("separation", 10);
		row_panel->add_child(card_hb);

		Ref<Texture2D> portrait = _resolve_actor_portrait_or_sprite(aid);
		TextureRect *portrait_rect = memnew(TextureRect);
		portrait_rect->set_name("ActorPortrait");
		portrait_rect->set_custom_minimum_size(Size2(48, 48));
		portrait_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		portrait_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		portrait_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		if (portrait.is_valid()) {
			portrait_rect->set_texture(portrait);
			portrait_rect->set_visible(true);
		} else {
			portrait_rect->set_visible(false);
		}
		card_hb->add_child(portrait_rect);

		VBoxContainer *rvb = memnew(VBoxContainer);
		rvb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		card_hb->add_child(rvb);

		HBoxContainer *top = memnew(HBoxContainer);
		rvb->add_child(top);
		Label *nl = memnew(Label);
		nl->set_name("ActorNameLabel");
		String cname = String(sheet.get("class_name", ""));
		nl->set_text(cname.is_empty() ? String(sheet.get("name", "")) : vformat("%s  (%s)", sheet.get("name", ""), cname));
		nl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		nl->add_theme_font_size_override("font_size", 15);
		top->add_child(nl);

		Label *lvl = memnew(Label);
		lvl->set_name("ActorLevelLabel");
		lvl->set_text(vformat("Nv. %d", int(sheet.get("level", 1))));
		top->add_child(lvl);

		Label *vitals = memnew(Label);
		vitals->set_name("ActorVitalsLabel");
		vitals->set_text(vformat("HP: %d / %d    MP: %d / %d    EXP: %d",
				int(sheet.get("hp", 0)), int(sheet.get("max_hp", 1)),
				int(sheet.get("mp", 0)), int(sheet.get("max_mp", 0)),
				int64_t(sheet.get("exp", 0))));
		rvb->add_child(vitals);
	}
}

void ClockGearsGameMenu::_populate_actor_selector(OptionButton *p_opt, HBoxContainer *p_selector_box, int &r_selected_actor_id) {
	if (!p_opt) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	p_opt->clear();
	Array party = dm->get_party_actor_ids();
	if (p_selector_box) {
		p_selector_box->set_visible(party.size() > 1);
	}
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

void ClockGearsGameMenu::_cycle_actor_selector(OptionButton *p_opt, int p_delta, int &r_actor_id, const String &p_subscreen) {
	if (!p_opt || p_opt->get_item_count() <= 1) {
		return;
	}
	int cnt = p_opt->get_item_count();
	int cur = MAX(0, p_opt->get_selected());
	int nxt = (cur + p_delta + cnt) % cnt;
	p_opt->select(nxt);
	r_actor_id = int(p_opt->get_item_id(nxt));
	if (p_subscreen == "items") {
		_refresh_items_view();
	} else if (p_subscreen == "equip") {
		selected_equip_candidate_key = "";
		_refresh_equip_view();
	} else if (p_subscreen == "status") {
		_refresh_status_view();
	}
}

void ClockGearsGameMenu::_on_items_actor_cycle(int p_delta) {
	_cycle_actor_selector(items_target_option, p_delta, selected_status_actor_id, "items");
}

void ClockGearsGameMenu::_on_equip_actor_cycle(int p_delta) {
	_cycle_actor_selector(equip_actor_option, p_delta, selected_equip_actor_id, "equip");
}

void ClockGearsGameMenu::_on_status_actor_cycle(int p_delta) {
	_cycle_actor_selector(status_actor_option, p_delta, selected_status_actor_id, "status");
}

void ClockGearsGameMenu::_refresh_items_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	int dummy_actor = selected_status_actor_id;
	_populate_actor_selector(items_target_option, items_member_selector, dummy_actor);
	selected_status_actor_id = dummy_actor;

	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	bool prev_enabled = bool(prev_cfg.get("enabled", true));
	String prev_pos = String(prev_cfg.get("position", "right"));
	bool show_icon = bool(prev_cfg.get("show_icon", true));
	bool show_name = bool(prev_cfg.get("show_name", true));
	bool show_desc = bool(prev_cfg.get("show_desc", true));
	int icon_scale = CLAMP(int(prev_cfg.get("icon_scale", 2)), 1, 4);

	if (item_side_panel) {
		item_side_panel->set_visible(prev_enabled);
		if (items_split && items_split->get_child_count() >= 2) {
			if (prev_pos == "left") {
				items_split->move_child(item_side_panel, 0);
			} else {
				items_split->move_child(item_side_panel, 1);
			}
		}
	}
	if (item_preview_box) {
		item_preview_box->set_visible(show_icon);
	}
	if (item_detail_name) {
		item_detail_name->set_visible(show_name);
	}
	if (item_desc_scroll) {
		item_desc_scroll->set_visible(show_desc);
	}
	int preview_px = icon_scale * 32;
	if (item_preview_icon_holder) {
		item_preview_icon_holder->set_custom_minimum_size(Size2(preview_px, preview_px));
	}
	if (item_preview_icon_rect) {
		item_preview_icon_rect->set_custom_minimum_size(Size2(preview_px, preview_px));
		item_preview_icon_rect->set_pivot_offset(Vector2(preview_px * 0.5f, preview_px * 0.5f));
		item_preview_icon_rect->set_rotation_degrees(preview_rotation_deg);
		item_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	}

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
		hb->add_theme_constant_override("separation", 8);
		items_list_box->add_child(hb);

		TextureRect *ic = memnew(TextureRect);
		ic->set_custom_minimum_size(Size2(32, 32));
		ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		ic->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		Ref<Texture2D> tex = dm->get_entry_icon_texture(row, 32);
		ic->set_texture(tex);
		hb->add_child(ic);

		String item_name = String(row.get("name", key));
		if (bool(row.get("is_key_item", false))) {
			item_name += " [Clave]";
		}
		Button *b = _cg_make_menu_button(item_name, "game_menu", callable_mp(this, &ClockGearsGameMenu::_on_item_row_pressed).bind(key));
		b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		b->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		b->connect("mouse_entered", callable_mp(this, &ClockGearsGameMenu::_on_item_row_hovered).bind(key));
		b->connect("focus_entered", callable_mp(this, &ClockGearsGameMenu::_on_item_row_hovered).bind(key));
		hb->add_child(b);

		Label *qty_lbl = memnew(Label);
		qty_lbl->set_custom_minimum_size(Size2(50, 0));
		qty_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		qty_lbl->set_text(vformat("x%d", int(row.get("qty", 0))));
		hb->add_child(qty_lbl);
	}
	if (!found_sel) {
		selected_item_key = (entries.size() > 0) ? String(Dictionary(entries[0]).get("key", "")) : String();
	}

	_update_item_preview_panel(selected_item_key, false);
}

void ClockGearsGameMenu::_update_item_preview_panel(const String &p_key, bool p_is_equip_subscreen) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}

	Dictionary cur_row;
	if (!p_key.is_empty()) {
		Array entries = dm->get_inventory_entries();
		for (int i = 0; i < entries.size(); i++) {
			if (String(Dictionary(entries[i]).get("key", "")) == p_key) {
				cur_row = entries[i];
				break;
			}
		}
		if (cur_row.is_empty()) {
			static const int cats[8] = { CAT_ITEMS, CAT_WEAPONS, CAT_ARMORS, CAT_ACCESSORIES, CAT_ARPG_WEAPONS, CAT_ARPG_ARMORS, CAT_ARPG_SHIELDS, CAT_ARPG_ACCESSORIES };
			for (int c = 0; c < 8; c++) {
				Array arr = dm->get_category_array(cats[c]);
				for (int i = 0; i < arr.size(); i++) {
					if (arr[i].get_type() != Variant::DICTIONARY) continue;
					Dictionary d = arr[i];
					String dname = String(d.get("name", ""));
					String dkey = vformat("%d:%d", cats[c], int(d.get("id", 0)));
					if (dkey == p_key || dname == p_key) {
						cur_row = d;
						cur_row["category"] = cats[c];
						break;
					}
				}
				if (!cur_row.is_empty()) break;
			}
		}
	}

	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	int icon_scale = CLAMP(int(prev_cfg.get("icon_scale", 2)), 1, 4);
	int preview_px = icon_scale * 32;

	Ref<Texture2D> tex;
	if (!cur_row.is_empty()) {
		tex = dm->get_entry_icon_texture(cur_row, 32);
	}

	if (!p_is_equip_subscreen) {
		if (cur_row.is_empty()) {
			if (item_detail_name) item_detail_name->set_text(String::utf8("Sin objetos"));
			if (item_detail_meta) item_detail_meta->set_text("");
			if (item_detail_desc) item_detail_desc->set_text(String::utf8("No hay ningún objeto seleccionado."));
			if (btn_item_use) btn_item_use->set_disabled(true);
			if (btn_item_discard) btn_item_discard->set_disabled(true);
			if (item_preview_icon_rect) item_preview_icon_rect->set_texture(Ref<Texture2D>());
		} else {
			bool is_key = bool(cur_row.get("is_key_item", false));
			bool usable = bool(cur_row.get("usable_in_menu", false));
			int qty = int(cur_row.get("qty", 0));
			int max_st = int(cur_row.get("max_stack", 99));
			if (item_detail_name) item_detail_name->set_text(String(cur_row.get("name", p_key)));
			if (item_detail_meta) item_detail_meta->set_text(vformat(String::utf8("Cantidad: %d / %d%s"), qty, max_st, is_key ? String::utf8("  |  Objeto clave") : ""));
			if (item_detail_desc) item_detail_desc->set_text(String(cur_row.get("desc", "")));
			if (btn_item_use) btn_item_use->set_disabled(!usable || qty <= 0);
			if (btn_item_discard) btn_item_discard->set_disabled(is_key || qty <= 0);
			if (item_preview_icon_rect) {
				item_preview_icon_rect->set_custom_minimum_size(Size2(preview_px, preview_px));
				item_preview_icon_rect->set_pivot_offset(Vector2(preview_px * 0.5f, preview_px * 0.5f));
				item_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
				item_preview_icon_rect->set_texture(tex);
			}
		}
	} else {
		if (cur_row.is_empty()) {
			if (equip_detail_name) equip_detail_name->set_text(String::utf8("--- Vacío ---"));
			if (equip_detail_meta) equip_detail_meta->set_text("");
			if (equip_detail_desc) equip_detail_desc->set_text(String::utf8("Sin pieza seleccionada."));
			if (equip_preview_icon_rect) equip_preview_icon_rect->set_texture(Ref<Texture2D>());
		} else {
			if (equip_detail_name) equip_detail_name->set_text(String(cur_row.get("name", p_key)));
			String meta_str;
			if (cur_row.has("qty")) {
				meta_str = vformat("Cantidad: %d", int(cur_row.get("qty", 1)));
			}
			if (equip_detail_meta) equip_detail_meta->set_text(meta_str);
			if (equip_detail_desc) equip_detail_desc->set_text(String(cur_row.get("desc", "")));
			if (equip_preview_icon_rect) {
				equip_preview_icon_rect->set_custom_minimum_size(Size2(preview_px, preview_px));
				equip_preview_icon_rect->set_pivot_offset(Vector2(preview_px * 0.5f, preview_px * 0.5f));
				equip_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
				equip_preview_icon_rect->set_texture(tex);
			}
		}
	}
}

void ClockGearsGameMenu::_on_item_row_hovered(const String &p_key) {
	if (selected_item_key != p_key) {
		preview_rotation_deg = 0.0f;
		_apply_preview_rotation(0.0f);
	}
	selected_item_key = p_key;
	_update_item_preview_panel(p_key, false);
}

void ClockGearsGameMenu::_on_equip_slot_hovered(int p_slot_idx) {
	selected_equip_slot_idx = p_slot_idx;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Array slots = dm->get_actor_equip_slots(selected_equip_actor_id);
	if (p_slot_idx >= 0 && p_slot_idx < slots.size()) {
		Dictionary sd = slots[p_slot_idx];
		int pid = int(sd.get("piece_id", 0));
		int cat = dm->get_category_from_key(String(sd.get("category", "")));
		if (pid > 0 && cat >= 0) {
			Dictionary piece = dm->get_record_by_id(cat, pid);
			preview_rotation_deg = 0.0f;
			_apply_preview_rotation(0.0f);
			_update_item_preview_panel(String(piece.get("name", "")), true);
		} else {
			_update_item_preview_panel("", true);
		}
	}
}

void ClockGearsGameMenu::_on_equip_candidate_hovered(const String &p_key) {
	if (selected_equip_candidate_key != p_key) {
		preview_rotation_deg = 0.0f;
		_apply_preview_rotation(0.0f);
	}
	selected_equip_candidate_key = p_key;
	_update_item_preview_panel(p_key, true);
	_refresh_equip_preview_labels();
}

void ClockGearsGameMenu::_refresh_equip_view() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	_populate_actor_selector(equip_actor_option, equip_member_selector, selected_equip_actor_id);
	bool arpg = dm->is_arpg_mode_active();
	if (equip_mode_label) {
		equip_mode_label->set_text(arpg ? "Modo ARPG (5 huecos)" : "Modo Por Turnos");
	}

	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	bool prev_enabled = bool(prev_cfg.get("enabled", true));
	bool show_icon = bool(prev_cfg.get("show_icon", true));
	bool show_name = bool(prev_cfg.get("show_name", true));
	bool show_desc = bool(prev_cfg.get("show_desc", true));
	int icon_scale = CLAMP(int(prev_cfg.get("icon_scale", 2)), 1, 4);

	if (equip_item_preview_box) {
		equip_item_preview_box->set_visible(prev_enabled && show_icon);
	}
	if (equip_desc_box) {
		equip_desc_box->set_visible(prev_enabled && (show_name || show_desc));
	}
	if (equip_detail_name) {
		equip_detail_name->set_visible(show_name);
	}
	if (equip_desc_scroll) {
		equip_desc_scroll->set_visible(show_desc);
	}
	int preview_px = icon_scale * 32;
	if (equip_preview_icon_holder) {
		equip_preview_icon_holder->set_custom_minimum_size(Size2(preview_px, preview_px));
	}
	if (equip_preview_icon_rect) {
		equip_preview_icon_rect->set_custom_minimum_size(Size2(preview_px, preview_px));
		equip_preview_icon_rect->set_pivot_offset(Vector2(preview_px * 0.5f, preview_px * 0.5f));
		equip_preview_icon_rect->set_rotation_degrees(preview_rotation_deg);
		equip_preview_icon_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
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
		String pname = String::utf8("--- Vacío ---");
		Ref<Texture2D> slot_ic;
		if (pid > 0 && cat >= 0) {
			Dictionary piece = dm->get_record_by_id(cat, pid);
			pname = piece.is_empty() ? vformat("#%d", pid) : String(piece.get("name", vformat("#%d", pid)));
			slot_ic = dm->get_entry_icon_texture(piece, 32);
		}
		bool locked = dm->is_actor_equip_slot_locked(selected_equip_actor_id, i);
		bool sealed = dm->is_actor_equip_slot_sealed(selected_equip_actor_id, i);
		String prefix = (i == selected_equip_slot_idx) ? "> " : "";
		String suffix = sealed ? " [Sellado]" : (locked ? " [Bloqueado]" : "");
		Button *sb = _cg_make_menu_button(vformat("%s%s: %s%s", prefix, sname, pname, suffix), "game_menu",
				callable_mp(this, &ClockGearsGameMenu::_on_equip_slot_pressed).bind(i));
		sb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sb->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		if (slot_ic.is_valid()) {
			sb->set_button_icon(slot_ic);
			sb->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		}
		sb->connect("mouse_entered", callable_mp(this, &ClockGearsGameMenu::_on_equip_slot_hovered).bind(i));
		sb->connect("focus_entered", callable_mp(this, &ClockGearsGameMenu::_on_equip_slot_hovered).bind(i));
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
		cb->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		Ref<Texture2D> cand_ic = dm->get_entry_icon_texture(crow, 32);
		if (cand_ic.is_valid()) {
			cb->set_button_icon(cand_ic);
			cb->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		}
		cb->connect("mouse_entered", callable_mp(this, &ClockGearsGameMenu::_on_equip_candidate_hovered).bind(ckey));
		cb->connect("focus_entered", callable_mp(this, &ClockGearsGameMenu::_on_equip_candidate_hovered).bind(ckey));
		equip_candidates_box->add_child(cb);
	}
	if (!found_cand) {
		selected_equip_candidate_key = (cands.size() > 0) ? String(Dictionary(cands[0]).get("key", "")) : String();
	}
	if (btn_equip_apply) {
		btn_equip_apply->set_disabled(selected_equip_candidate_key.is_empty() || cur_locked || cur_sealed);
	}
	_refresh_equip_preview_labels();

	String target_preview_key = selected_equip_candidate_key;
	if (target_preview_key.is_empty() && selected_equip_slot_idx >= 0 && selected_equip_slot_idx < slots.size()) {
		Dictionary sd = slots[selected_equip_slot_idx];
		int pid = int(sd.get("piece_id", 0));
		int cat = dm->get_category_from_key(String(sd.get("category", "")));
		if (pid > 0 && cat >= 0) {
			Dictionary piece = dm->get_record_by_id(cat, pid);
			target_preview_key = String(piece.get("name", ""));
		}
	}
	_update_item_preview_panel(target_preview_key, true);
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
	_populate_actor_selector(status_actor_option, status_member_selector, selected_status_actor_id);
	Dictionary sheet = dm->get_actor_status_sheet(selected_status_actor_id);
	if (status_portrait_rect) {
		Ref<Texture2D> ptex = _resolve_actor_portrait_or_sprite(selected_status_actor_id);
		if (ptex.is_valid()) {
			status_portrait_rect->set_texture(ptex);
			status_portrait_rect->set_visible(true);
		} else {
			status_portrait_rect->set_texture(Ref<Texture2D>());
			status_portrait_rect->set_visible(false);
		}
	}

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
	String params = vformat(String::utf8("Parámetros:\nHP Máx: %d    MP Máx: %d\nATQ: %d    DEF: %d\nMAT: %d    MDF: %d\nAGI: %d    SUE: %d"),
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
		String tag = (pct > 100) ? String::utf8(" [Débil]") : (pct < 100 ? String::utf8(" [Resiste]") : String(""));
		el_str += vformat("- %s: %d%%%s\n", er.get("name", ""), pct, tag);
	}
	if (status_elements_label) {
		status_elements_label->set_text(el_str.strip_edges());
	}

	if (status_equips_box) {
		_cg_clear_box_children(status_equips_box);
		Array slots = dm->get_actor_equip_slots(selected_status_actor_id);
		for (int i = 0; i < slots.size(); i++) {
			Dictionary sd = slots[i];
			String sname = String(sd.get("name", vformat("Hueco %d", i + 1)));
			int pid = int(sd.get("piece_id", 0));
			int cat = dm->get_category_from_key(String(sd.get("category", "")));
			String pname = String::utf8("--- Vacío ---");
			Dictionary piece;
			if (pid > 0 && cat >= 0) {
				piece = dm->get_record_by_id(cat, pid);
				pname = piece.is_empty() ? vformat("#%d", pid) : String(piece.get("name", vformat("#%d", pid)));
			}
			HBoxContainer *row_hb = memnew(HBoxContainer);
			row_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			row_hb->add_theme_constant_override("separation", 6);
			status_equips_box->add_child(row_hb);

			TextureRect *ic = memnew(TextureRect);
			ic->set_custom_minimum_size(Size2(32, 32));
			ic->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			ic->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			ic->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			Ref<Texture2D> tex = pid > 0 ? dm->get_entry_icon_texture(piece, 32) : Ref<Texture2D>();
			ic->set_texture(tex);
			row_hb->add_child(ic);

			Label *lbl = memnew(Label);
			lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			lbl->set_text(vformat("%s: %s", sname, pname));
			row_hb->add_child(lbl);
		}
	}
}

void ClockGearsGameMenu::_apply_preview_rotation(float p_deg) {
	preview_rotation_deg = p_deg;
	if (item_preview_icon_rect) {
		item_preview_icon_rect->set_rotation_degrees(preview_rotation_deg);
	}
	if (equip_preview_icon_rect) {
		equip_preview_icon_rect->set_rotation_degrees(preview_rotation_deg);
	}
}

void ClockGearsGameMenu::_on_preview_icon_gui_input(const Ref<InputEvent> &p_event) {
	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	if (!bool(prev_cfg.get("rotation_enabled", true))) {
		return;
	}
	int rot_step = CLAMP(int(prev_cfg.get("rotation_step", 15)), 5, 45);

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		if (mb->is_pressed()) {
			preview_is_dragging = true;
			preview_drag_start_pos = mb->get_position();
			preview_drag_start_deg = preview_rotation_deg;
		} else {
			preview_is_dragging = false;
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && preview_is_dragging) {
		float dx = mm->get_position().x - preview_drag_start_pos.x;
		int steps = Math::round(dx / 8.0f);
		float target_deg = Math::wrapf(preview_drag_start_deg + float(steps * rot_step), 0.0f, 360.0f);
		_apply_preview_rotation(target_deg);
	}
}

void ClockGearsGameMenu::simulate_preview_drag_rotation(float p_drag_dx) {
	Dictionary gm_cfg = _get_effective_game_menu_config();
	Dictionary prev_cfg = gm_cfg.get("item_preview", Dictionary());
	if (!bool(prev_cfg.get("rotation_enabled", true))) {
		return;
	}
	int rot_step = CLAMP(int(prev_cfg.get("rotation_step", 15)), 5, 45);
	int steps = Math::round(p_drag_dx / 8.0f);
	if (steps == 0 && Math::abs(p_drag_dx) > 0.001f) {
		steps = (p_drag_dx > 0.0f) ? 1 : -1;
	}
	float target_deg = Math::wrapf(preview_rotation_deg + float(steps * rot_step), 0.0f, 360.0f);
	_apply_preview_rotation(target_deg);
}

Dictionary ClockGearsGameMenu::get_item_preview_panel_info() const {
	Dictionary res;
	res["visible"] = (item_side_panel && item_side_panel->is_visible());
	res["icon_visible"] = (item_preview_box && item_preview_box->is_visible());
	res["name_visible"] = (item_detail_name && item_detail_name->is_visible());
	res["desc_visible"] = (item_desc_scroll && item_desc_scroll->is_visible());
	res["name"] = item_detail_name ? item_detail_name->get_text() : String();
	res["desc"] = item_detail_desc ? item_detail_desc->get_text() : String();
	res["meta"] = item_detail_meta ? item_detail_meta->get_text() : String();
	res["rotation_deg"] = preview_rotation_deg;
	Ref<Texture2D> tex = item_preview_icon_rect ? item_preview_icon_rect->get_texture() : Ref<Texture2D>();
	res["has_texture"] = tex.is_valid();
	if (tex.is_valid()) {
		res["texture_size"] = tex->get_size();
	}
	if (items_split && item_side_panel) {
		res["panel_index"] = items_split->get_children().find(item_side_panel);
	}
	return res;
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

String ClockGearsGameMenu::_map_subscreen_to_ui_design_id(const String &p_subscreen) const {
	String s = p_subscreen.strip_edges().to_lower();
	if (s == "main" || s == "menu") {
		return "game_menu_main";
	}
	if (s == "items") {
		return "game_menu_items";
	}
	if (s == "equip") {
		return "game_menu_equip";
	}
	if (s == "status") {
		return "game_menu_status";
	}
	return String();
}

void ClockGearsGameMenu::_on_custom_ui_action(const String &p_action, const String &p_elem_id) {
	String act = p_action.strip_edges().to_lower();
	if (act == "close") {
		close_menu();
		return;
	}
	if (act == "back") {
		if (current_subscreen == "main") {
			close_menu();
		} else {
			_on_subscreen_back();
		}
		return;
	}
	if (!act.is_empty()) {
		open_subscreen(act);
	}
}

bool ClockGearsGameMenu::is_custom_ui_active() const {
	return custom_ui_layer && custom_ui_layer->is_visible();
}

void ClockGearsGameMenu::_on_subscreen_back() {
	if (direct_subscreen_close_on_back) {
		close_menu();
		return;
	}
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

static void _cg_check_controls_within_rect(Control *p_node, const Rect2 &p_vp, Array &r_oob) {
	if (!p_node || !p_node->is_visible()) {
		return;
	}
	Rect2 gr = p_node->get_global_rect();
	if (gr.size.x > 0.5f && gr.size.y > 0.5f) {
		if (!p_vp.grow(1.5f).encloses(gr)) {
			Dictionary bad;
			bad["name"] = p_node->get_name();
			bad["class"] = p_node->get_class();
			bad["rect"] = gr;
			r_oob.push_back(bad);
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Control *ch = Object::cast_to<Control>(p_node->get_child(i));
		if (ch) {
			_cg_check_controls_within_rect(ch, p_vp, r_oob);
		}
	}
}

Dictionary ClockGearsGameMenu::get_party_cards_info() const {
	Dictionary out;
	Array cards;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (party_list_box) {
		for (int i = 0; i < party_list_box->get_child_count(); i++) {
			Control *card = Object::cast_to<Control>(party_list_box->get_child(i));
			if (!card) {
				continue;
			}
			Dictionary cd;
			cd["actor_id"] = card->has_meta("cg_actor_id") ? int(card->get_meta("cg_actor_id")) : 0;
			TextureRect *pr = Object::cast_to<TextureRect>(card->find_child("ActorPortrait", true, false));
			Label *nl = Object::cast_to<Label>(card->find_child("ActorNameLabel", true, false));
			Label *ll = Object::cast_to<Label>(card->find_child("ActorLevelLabel", true, false));
			Label *vl = Object::cast_to<Label>(card->find_child("ActorVitalsLabel", true, false));
			cd["has_portrait"] = (pr && pr->is_visible() && pr->get_texture().is_valid());
			cd["name_text"] = nl ? nl->get_text() : String();
			cd["level_text"] = ll ? ll->get_text() : String();
			cd["vitals_text"] = vl ? vl->get_text() : String();
			cards.push_back(cd);
		}
	}
	out["party_size"] = dm ? int(dm->get_party_actor_ids().size()) : int(cards.size());
	out["cards"] = cards;
	out["gold_text"] = get_displayed_gold_text();
	out["playtime_text"] = get_displayed_playtime_text();
	return out;
}

Dictionary ClockGearsGameMenu::get_member_selectors_info() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary out;
	out["party_size"] = dm ? int(dm->get_party_actor_ids().size()) : 0;
	out["items_visible"] = items_member_selector ? items_member_selector->is_visible() : false;
	out["equip_visible"] = equip_member_selector ? equip_member_selector->is_visible() : false;
	out["status_visible"] = status_member_selector ? status_member_selector->is_visible() : false;
	out["status_has_portrait"] = (status_portrait_rect && status_portrait_rect->is_visible() && status_portrait_rect->get_texture().is_valid());
	out["selected_equip_actor_id"] = selected_equip_actor_id;
	out["selected_status_actor_id"] = selected_status_actor_id;
	return out;
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
	info["subscreen"] = current_subscreen;
	info["custom_design_active"] = is_custom_ui_active();
	if (is_custom_ui_active() && custom_ui_layer) {
		Dictionary cinfo = custom_ui_layer->get_layout_bounds_info(vsz);
		info["stage_rect"] = cinfo.get("stage_rect", Rect2());
		info["element_rects"] = cinfo.get("element_rects", Dictionary());
		info["button_rects"] = cinfo.get("button_rects", Array());
		info["clips"] = cinfo.get("clips", false);
		info["overlaps"] = cinfo.get("any_overlap", false);
		info["all_within_viewport"] = cinfo.get("all_within_viewport", true);
		return info;
	}
	bool clips = false;
	bool overlaps = false;
	Rect2 vp_rect(Vector2(0, 0), vsz);
	if (main_cmd_panel && party_panel && main_cmd_panel->is_visible_in_tree() && party_panel->is_visible_in_tree()) {
		Rect2 r_cmd = main_cmd_panel->get_global_rect();
		Rect2 r_party = party_panel->get_global_rect();
		bool cmd_ok = vp_rect.grow(1.5f).encloses(r_cmd);
		bool party_ok = vp_rect.grow(1.5f).encloses(r_party);
		bool ov = r_cmd.intersects(r_party);
		if (currency_time_card && currency_time_card->is_visible_in_tree()) {
			Rect2 r_card = currency_time_card->get_global_rect();
			info["currency_card_rect"] = r_card;
			if (!vp_rect.grow(1.5f).encloses(r_card)) {
				clips = true;
			}
			if (r_card.intersects(r_cmd) || r_card.intersects(r_party)) {
				ov = true;
			}
		}
		if (!cmd_ok || !party_ok) {
			clips = true;
		}
		if (ov) {
			overlaps = true;
		}
		info["cmd_rect"] = r_cmd;
		info["party_rect"] = r_party;
	}
	Array oob;
	Control *active = _get_active_subscreen_control();
	if (active && active->is_visible_in_tree()) {
		info["active_panel_rect"] = active->get_global_rect();
		_cg_check_controls_within_rect(active, vp_rect, oob);
		if (!oob.is_empty()) {
			clips = true;
		}
	}
	info["out_of_bounds_controls"] = oob;
	info["clips"] = clips;
	info["overlaps"] = overlaps;
	info["all_within_viewport"] = (!clips && !overlaps && oob.is_empty());
	return info;
}
