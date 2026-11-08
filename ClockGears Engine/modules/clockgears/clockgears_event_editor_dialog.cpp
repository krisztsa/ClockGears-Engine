#include "clockgears_event_editor_dialog.h"
#include "clockgears_editor_plugin.h"
#include "core/clockgears_data_manager.h"
#include "core/io/dir_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/clockgears_project_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

static Ref<StyleBoxFlat> _event_card_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.12, 0.13, 0.15));
	style->set_corner_radius_all(6);
	style->set_content_margin_all(8);
	style->set_border_width_all(1);
	style->set_border_color(Color(1, 1, 1, 0.08));
	return style;
}

void ClockGearsEventEditorDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("edit_event", "event"), &ClockGearsEventEditorDialog::edit_event);
}

Dictionary ClockGearsEventEditorDialog::_create_empty_page() const {
	Dictionary p;
	p["sw1_en"] = false;
	p["sw1_name"] = "";
	p["sw2_en"] = false;
	p["sw2_name"] = "";
	p["self_sw_en"] = false;
	p["self_sw_letter"] = "A";
	p["var_en"] = false;
	p["var_name"] = "";
	p["var_val"] = 0;
	p["item_en"] = false;
	p["item_name"] = "";
	p["actor_en"] = false;
	p["actor_name"] = "";
	p["trigger"] = TRIGGER_ACTION_BUTTON;
	p["priority"] = 1;
	p["through"] = false;
	p["dir_fix"] = false;
	p["graphic_path"] = "";
	p["hframes"] = 1;
	p["vframes"] = 1;
	p["frame"] = 0;
	p["move_type"] = MOVE_FIXED;
	p["speed"] = 3; // 1..6
	p["freq"] = 3; // 1..5
	p["commands"] = Array();
	return p;
}

void ClockGearsEventEditorDialog::_save_current_page_from_ui() {
	if (loading_page_ui || current_page_index < 0 || current_page_index >= working_pages.size()) {
		return;
	}
	Dictionary p = working_pages[current_page_index];
	p["sw1_en"] = cond_sw1_check->is_pressed();
	p["sw1_name"] = cond_sw1_name_edit->get_text();
	p["sw2_en"] = cond_sw2_check->is_pressed();
	p["sw2_name"] = cond_sw2_name_edit->get_text();
	p["self_sw_en"] = cond_self_sw_check->is_pressed();
	p["self_sw_letter"] = cond_self_sw_opt->get_item_text(cond_self_sw_opt->get_selected());
	p["var_en"] = cond_var_check->is_pressed();
	p["var_name"] = cond_var_name_edit->get_text();
	p["var_val"] = int(cond_var_val_spin->get_value());
	p["item_en"] = cond_item_check->is_pressed();
	p["item_name"] = cond_item_name_edit->get_text();
	p["actor_en"] = cond_actor_check->is_pressed();
	p["actor_name"] = cond_actor_name_edit->get_text();

	p["trigger"] = trigger_opt->get_selected_id();
	p["priority"] = priority_opt->get_selected_id();
	p["through"] = through_check->is_pressed();
	p["dir_fix"] = dir_fix_check->is_pressed();

	p["graphic_path"] = current_graphic_path;
	p["hframes"] = int(sprite_hframes_spin->get_value());
	p["vframes"] = int(sprite_vframes_spin->get_value());
	p["frame"] = int(sprite_frame_spin->get_value());

	p["move_type"] = move_type_opt->get_selected_id();
	p["speed"] = speed_opt->get_selected() + 1;
	p["freq"] = freq_opt->get_selected() + 1;

	working_pages[current_page_index] = p;
}

void ClockGearsEventEditorDialog::_load_page_to_ui(int p_page_idx) {
	if (p_page_idx < 0 || p_page_idx >= working_pages.size()) {
		return;
	}
	loading_page_ui = true;
	current_page_index = p_page_idx;
	Dictionary p = working_pages[current_page_index];

	cond_sw1_check->set_pressed(p.get("sw1_en", false));
	cond_sw1_name_edit->set_text(p.get("sw1_name", ""));
	cond_sw2_check->set_pressed(p.get("sw2_en", false));
	cond_sw2_name_edit->set_text(p.get("sw2_name", ""));
	cond_self_sw_check->set_pressed(p.get("self_sw_en", false));
	String s_letter = p.get("self_sw_letter", "A");
	int sel_idx = 0;
	if (s_letter == "B") sel_idx = 1;
	else if (s_letter == "C") sel_idx = 2;
	else if (s_letter == "D") sel_idx = 3;
	cond_self_sw_opt->select(sel_idx);
	cond_var_check->set_pressed(p.get("var_en", false));
	cond_var_name_edit->set_text(p.get("var_name", ""));
	cond_var_val_spin->set_value(int(p.get("var_val", 0)));
	cond_item_check->set_pressed(p.get("item_en", false));
	cond_item_name_edit->set_text(p.get("item_name", ""));
	cond_actor_check->set_pressed(p.get("actor_en", false));
	cond_actor_name_edit->set_text(p.get("actor_name", ""));

	trigger_opt->select(CLAMP(int(p.get("trigger", 0)), 0, trigger_opt->get_item_count() - 1));
	priority_opt->select(CLAMP(int(p.get("priority", 1)), 0, priority_opt->get_item_count() - 1));
	through_check->set_pressed(p.get("through", false));
	dir_fix_check->set_pressed(p.get("dir_fix", false));

	sprite_hframes_spin->set_value(MAX(1, int(p.get("hframes", 1))));
	sprite_vframes_spin->set_value(MAX(1, int(p.get("vframes", 1))));
	sprite_frame_spin->set_value(MAX(0, int(p.get("frame", 0))));
	_update_graphic_preview(p.get("graphic_path", ""));

	move_type_opt->select(CLAMP(int(p.get("move_type", 0)), 0, move_type_opt->get_item_count() - 1));
	speed_opt->select(CLAMP(int(p.get("speed", 3)) - 1, 0, speed_opt->get_item_count() - 1));
	freq_opt->select(CLAMP(int(p.get("freq", 3)) - 1, 0, freq_opt->get_item_count() - 1));

	loading_page_ui = false;
	_refresh_command_list();
}

void ClockGearsEventEditorDialog::_rebuild_page_tabs() {
	if (!page_tabs) {
		return;
	}
	loading_page_ui = true;
	page_tabs->clear_tabs();
	for (int i = 0; i < working_pages.size(); i++) {
		page_tabs->add_tab(vformat(CG_TR("Página %d"), i + 1));
	}
	if (current_page_index >= 0 && current_page_index < page_tabs->get_tab_count()) {
		page_tabs->set_current_tab(current_page_index);
	}
	if (btn_delete_page) {
		btn_delete_page->set_disabled(working_pages.size() <= 1);
	}
	if (btn_paste_page) {
		btn_paste_page->set_disabled(clipboard_page.is_empty());
	}
	loading_page_ui = false;
}

void ClockGearsEventEditorDialog::_on_page_tab_changed(int p_tab) {
	if (loading_page_ui) {
		return;
	}
	_save_current_page_from_ui();
	_load_page_to_ui(p_tab);
}

void ClockGearsEventEditorDialog::_on_new_page_pressed() {
	_save_current_page_from_ui();
	working_pages.push_back(_create_empty_page());
	current_page_index = working_pages.size() - 1;
	_rebuild_page_tabs();
	_load_page_to_ui(current_page_index);
}

void ClockGearsEventEditorDialog::_on_copy_page_pressed() {
	_save_current_page_from_ui();
	if (current_page_index >= 0 && current_page_index < working_pages.size()) {
		clipboard_page = Dictionary(working_pages[current_page_index]).duplicate(true);
		if (btn_paste_page) {
			btn_paste_page->set_disabled(false);
		}
	}
}

void ClockGearsEventEditorDialog::_on_paste_page_pressed() {
	if (clipboard_page.is_empty()) {
		return;
	}
	_save_current_page_from_ui();
	working_pages.push_back(clipboard_page.duplicate(true));
	current_page_index = working_pages.size() - 1;
	_rebuild_page_tabs();
	_load_page_to_ui(current_page_index);
}

void ClockGearsEventEditorDialog::_on_delete_page_pressed() {
	if (working_pages.size() <= 1) {
		return;
	}
	working_pages.remove_at(current_page_index);
	current_page_index = CLAMP(current_page_index, 0, working_pages.size() - 1);
	_rebuild_page_tabs();
	_load_page_to_ui(current_page_index);
}

Array ClockGearsEventEditorDialog::_get_active_page_commands() const {
	if (current_page_index >= 0 && current_page_index < working_pages.size()) {
		Dictionary p = working_pages[current_page_index];
		return p.get("commands", Array());
	}
	return Array();
}

void ClockGearsEventEditorDialog::_set_active_page_commands(const Array &p_cmds) {
	if (current_page_index >= 0 && current_page_index < working_pages.size()) {
		Dictionary p = working_pages[current_page_index];
		p["commands"] = p_cmds;
		working_pages[current_page_index] = p;
	}
}

void ClockGearsEventEditorDialog::_append_command_to_active_page(const Dictionary &p_cmd) {
	Array cmds = _get_active_page_commands();
	if (editing_command_index >= 0 && editing_command_index < cmds.size()) {
		cmds[editing_command_index] = p_cmd;
		editing_command_index = -1;
	} else {
		TreeItem *sel = command_tree ? command_tree->get_selected() : nullptr;
		if (sel) {
			int sel_idx = int(sel->get_metadata(0));
			if (sel_idx >= 0 && sel_idx < cmds.size()) {
				cmds.insert(sel_idx + 1, p_cmd);
			} else {
				cmds.push_back(p_cmd);
			}
		} else {
			cmds.push_back(p_cmd);
		}
	}
	_set_active_page_commands(cmds);
	_refresh_command_list();
}

void ClockGearsEventEditorDialog::_update_graphic_preview(const String &p_path) {
	current_graphic_path = p_path;
	if (!graphic_preview) {
		return;
	}
	if (p_path.is_empty()) {
		graphic_preview->set_texture(Ref<Texture2D>());
		return;
	}

	Ref<Texture2D> base_tex;
	if (ResourceLoader::exists(p_path)) {
		base_tex = ResourceLoader::load(p_path);
	}
	if (base_tex.is_null()) {
		Ref<Image> img = Image::load_from_file(p_path);
		if (img.is_valid() && !img->is_empty()) {
			base_tex = ImageTexture::create_from_image(img);
		}
	}
	if (base_tex.is_null()) {
		graphic_preview->set_texture(Ref<Texture2D>());
		return;
	}

	int hf = sprite_hframes_spin ? MAX(1, int(sprite_hframes_spin->get_value())) : 1;
	int vf = sprite_vframes_spin ? MAX(1, int(sprite_vframes_spin->get_value())) : 1;
	int total_frames = hf * vf;
	if (sprite_frame_spin) {
		sprite_frame_spin->set_max(MAX(0, total_frames - 1));
	}
	int fr = sprite_frame_spin ? CLAMP(int(sprite_frame_spin->get_value()), 0, total_frames - 1) : 0;

	if (hf == 1 && vf == 1) {
		graphic_preview->set_texture(base_tex);
	} else {
		Size2 sz = base_tex->get_size();
		float fw = sz.x / float(hf);
		float fh = sz.y / float(vf);
		int fx = fr % hf;
		int fy = fr / hf;
		Ref<AtlasTexture> atlas_tex;
		atlas_tex.instantiate();
		atlas_tex->set_atlas(base_tex);
		atlas_tex->set_region(Rect2(fx * fw, fy * fh, fw, fh));
		graphic_preview->set_texture(atlas_tex);
	}
}

void ClockGearsEventEditorDialog::_on_graphic_preview_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
		_on_select_graphic_pressed();
	}
}

void ClockGearsEventEditorDialog::_on_sprite_frame_changed(double p_val) {
	if (loading_page_ui) {
		return;
	}
	_update_graphic_preview(current_graphic_path);
}

void ClockGearsEventEditorDialog::_on_select_graphic_pressed() {
	if (graphic_file_dialog) {
		String default_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/characters");
		if (DirAccess::dir_exists_absolute(default_dir)) {
			graphic_file_dialog->set_current_dir(default_dir);
		}
		graphic_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_select_tile_graphic_pressed() {
	if (!event_tile_picker_dialog) {
		return;
	}
	Ref<Texture2D> tex;
	if (!current_graphic_path.is_empty()) {
		if (ResourceLoader::exists(current_graphic_path)) {
			tex = ResourceLoader::load(current_graphic_path);
		}
		if (tex.is_null()) {
			Ref<Image> img = Image::load_from_file(current_graphic_path);
			if (img.is_valid() && !img->is_empty()) {
				tex = ImageTexture::create_from_image(img);
			}
		}
	}
	event_tile_picker_dialog->open_picker(tex, callable_mp(this, &ClockGearsEventEditorDialog::_on_event_tile_picked));
}

void ClockGearsEventEditorDialog::_on_event_tile_picked(const Array &p_sel, const Vector2i &p_block) {
	if (!event_tile_picker_dialog || !event_tile_picker_dialog->get_picker()) {
		return;
	}
	ClockGearsTilePicker *pk = event_tile_picker_dialog->get_picker();
	Ref<Texture2D> atlas_tex = pk->get_atlas_texture();
	if (atlas_tex.is_null()) {
		return;
	}
	int tsz = MAX(1, pk->get_tile_size());
	Vector2i primary = pk->get_primary_selected_tile();
	int cols = MAX(1, int(atlas_tex->get_size().x) / tsz);
	int rows = MAX(1, int(atlas_tex->get_size().y) / tsz);
	int frame_idx = primary.y * cols + primary.x;

	loading_page_ui = true;
	if (sprite_hframes_spin) {
		sprite_hframes_spin->set_max(MAX(64, cols));
		sprite_hframes_spin->set_value(cols);
	}
	if (sprite_vframes_spin) {
		sprite_vframes_spin->set_max(MAX(64, rows));
		sprite_vframes_spin->set_value(rows);
	}
	if (sprite_frame_spin) {
		sprite_frame_spin->set_max(MAX(0, cols * rows - 1));
		sprite_frame_spin->set_value(frame_idx);
	}
	loading_page_ui = false;
	_update_graphic_preview(pk->get_atlas_path());
}

void ClockGearsEventEditorDialog::_on_clear_graphic_pressed() {
	_update_graphic_preview("");
}

void ClockGearsEventEditorDialog::_on_graphic_file_selected(const String &p_path) {
	ClockGearsProjectStructure::write_pixel_art_import_preset(p_path);
	String warn_32_msg;
	if (!ClockGearsProjectStructure::validate_folder_asset(p_path, "img/characters", warn_32_msg)) {
		WARN_PRINT(vformat("ClockGearsEventEditor: %s", warn_32_msg));
	}
	_update_graphic_preview(p_path);
}

void ClockGearsEventEditorDialog::_on_browse_face_pressed() {
	if (msg_face_file_dialog) {
		String default_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/faces");
		if (DirAccess::dir_exists_absolute(default_dir)) {
			msg_face_file_dialog->set_current_dir(default_dir);
		}
		msg_face_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_face_file_selected(const String &p_path) {
	ClockGearsProjectStructure::write_pixel_art_import_preset(p_path);
	String warn_32_msg;
	if (!ClockGearsProjectStructure::validate_folder_asset(p_path, "img/faces", warn_32_msg)) {
		WARN_PRINT(vformat("ClockGearsEventEditor (Rostro): %s", warn_32_msg));
	}
	if (msg_face_edit) {
		msg_face_edit->set_text(p_path);
	}
	_update_msg_live_preview();
}

void ClockGearsEventEditorDialog::_on_browse_bgm_pressed() {
	if (bgm_file_dialog) {
		String default_dir = ClockGearsProjectStructure::resolve_project_folder_global("audio/bgm");
		if (DirAccess::dir_exists_absolute(default_dir)) {
			bgm_file_dialog->set_current_dir(default_dir);
		}
		bgm_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_bgm_file_selected(const String &p_path) {
	if (bgm_file_edit) {
		bgm_file_edit->set_text(p_path);
	}
}

void ClockGearsEventEditorDialog::_on_add_command_pressed() {
	editing_command_index = -1;
	if (cmd_catalog_dialog) {
		cmd_catalog_dialog->popup_catalog();
	}
}

void ClockGearsEventEditorDialog::_on_edit_command_pressed() {
	if (!command_tree) {
		return;
	}
	TreeItem *selected = command_tree->get_selected();
	if (!selected) {
		_on_add_command_pressed();
		return;
	}
	int idx = int(selected->get_metadata(0));
	Array cmds = _get_active_page_commands();
	if (idx < 0 || idx >= cmds.size()) {
		_on_add_command_pressed();
		return;
	}
	Dictionary d = cmds[idx];
	editing_command_index = idx;
	String type = d.get("type", "");

	if (type == "message" && dlg_msg) {
		if (msg_face_edit) msg_face_edit->set_text(d.get("face", ""));
		if (msg_text_edit) msg_text_edit->set_text(d.get("text", ""));
		if (msg_frame_opt) msg_frame_opt->select(CLAMP(int(d.get("frame", 0)), 0, msg_frame_opt->get_item_count() - 1));
		if (msg_pos_opt) msg_pos_opt->select(CLAMP(int(d.get("position", d.get("pos", 0))), 0, msg_pos_opt->get_item_count() - 1));
		_update_msg_live_preview();
		dlg_msg->popup_centered(Size2(640, 520) * EDSCALE);
	} else if (type == "choices" && dlg_choices) {
		if (choice_1_edit) choice_1_edit->set_text(d.get("c1", ""));
		if (choice_2_edit) choice_2_edit->set_text(d.get("c2", ""));
		if (choice_3_edit) choice_3_edit->set_text(d.get("c3", ""));
		if (choice_4_edit) choice_4_edit->set_text(d.get("c4", ""));
		if (choice_5_edit) choice_5_edit->set_text(d.get("c5", ""));
		if (choice_6_edit) choice_6_edit->set_text(d.get("c6", ""));
		if (choice_cancel_opt) choice_cancel_opt->select(CLAMP(int(d.get("cancel_opt", d.get("cancel", 0))), 0, choice_cancel_opt->get_item_count() - 1));
		dlg_choices->popup_centered(Size2(420, 360) * EDSCALE);
	} else if (type == "number_input" && dlg_number_input) {
		if (num_input_var_spin) num_input_var_spin->set_value(int(d.get("variable", 1)));
		if (num_input_digits_spin) num_input_digits_spin->set_value(int(d.get("digits", 3)));
		dlg_number_input->popup_centered(Size2(360, 180) * EDSCALE);
	} else if (type == "switch" && dlg_switch) {
		if (sw_name_edit) sw_name_edit->set_text(d.get("name", ""));
		if (sw_val_opt) sw_val_opt->select(bool(d.get("val", true)) ? 0 : 1);
		dlg_switch->popup_centered(Size2(380, 160) * EDSCALE);
	} else if (type == "self_switch" && dlg_self_switch) {
		String l = d.get("letter", "A");
		int l_idx = (l == "B") ? 1 : ((l == "C") ? 2 : ((l == "D") ? 3 : 0));
		if (self_sw_letter_opt) self_sw_letter_opt->select(l_idx);
		if (self_sw_val_opt) self_sw_val_opt->select(bool(d.get("val", true)) ? 0 : 1);
		dlg_self_switch->popup_centered(Size2(360, 150) * EDSCALE);
	} else if (type == "variable" && dlg_var) {
		if (var_name_edit) var_name_edit->set_text(d.get("name", ""));
		String op = d.get("op", "=");
		int op_idx = (op == "+=") ? 1 : ((op == "-=") ? 2 : ((op == "*=") ? 3 : ((op == "/=") ? 4 : 0)));
		if (var_op_opt) var_op_opt->select(op_idx);
		if (var_val_spin) var_val_spin->set_value(int(d.get("val", 0)));
		dlg_var->popup_centered(Size2(420, 180) * EDSCALE);
	} else if (type == "wait" && dlg_wait) {
		if (wait_frames_spin) wait_frames_spin->set_value(int(d.get("frames", 60)));
		dlg_wait->popup_centered(Size2(320, 140) * EDSCALE);
	} else if (type == "teleport" && dlg_teleport) {
		_populate_teleport_maps();
		String tele_mpath = d.get("map", "");
		if (tele_map_edit) tele_map_edit->set_text(tele_mpath);
		if (tele_map_opt) {
			for (int i = 0; i < tele_map_opt->get_item_count(); i++) {
				if (tele_map_opt->get_item_metadata(i) == tele_mpath) {
					tele_map_opt->select(i);
					break;
				}
			}
		}
		if (tele_x_spin) tele_x_spin->set_value(int(d.get("x", 0)));
		if (tele_y_spin) tele_y_spin->set_value(int(d.get("y", 0)));
		String dir_str = d.get("dir", "Abajo");
		int dir_idx = (dir_str == "Izquierda") ? 1 : ((dir_str == "Derecha") ? 2 : ((dir_str == "Arriba") ? 3 : ((dir_str == "Mantener") ? 4 : 0)));
		if (tele_dir_opt) tele_dir_opt->select(dir_idx);
		dlg_teleport->popup_centered(Size2(420, 260) * EDSCALE);
	} else if (type == "move_route" && dlg_move_route) {
		if (mr_target_opt) mr_target_opt->select(d.get("target", "Este Evento") == "Este Evento" ? 0 : 1);
		String act = d.get("action", "");
		if (mr_action_opt) {
			for (int i = 0; i < mr_action_opt->get_item_count(); i++) {
				if (mr_action_opt->get_item_text(i) == act) {
					mr_action_opt->select(i);
					break;
				}
			}
		}
		if (mr_repeat_check) mr_repeat_check->set_pressed(bool(d.get("repeat", false)));
		dlg_move_route->popup_centered(Size2(380, 200) * EDSCALE);
	} else if (type == "item" && dlg_item) {
		if (item_name_edit) item_name_edit->set_text(d.get("name", ""));
		if (item_op_opt) item_op_opt->select(d.get("op", "+") == "+" ? 0 : 1);
		if (item_qty_spin) item_qty_spin->set_value(int(d.get("qty", 1)));
		dlg_item->popup_centered(Size2(380, 180) * EDSCALE);
	} else if (type == "gold" && dlg_gold) {
		if (gold_op_opt) gold_op_opt->select(d.get("op", "+") == "+" ? 0 : 1);
		if (gold_val_spin) gold_val_spin->set_value(int(d.get("amount", 100)));
		dlg_gold->popup_centered(Size2(360, 150) * EDSCALE);
	} else if (type == "hp_mp" && dlg_hp_mp) {
		if (hp_mp_stat_opt) hp_mp_stat_opt->select(d.get("stat", "HP") == "HP" ? 0 : 1);
		if (hp_mp_op_opt) hp_mp_op_opt->select(d.get("op", "+") == "+" ? 0 : 1);
		if (hp_mp_amount_spin) hp_mp_amount_spin->set_value(int(d.get("amount", 50)));
		dlg_hp_mp->popup_centered(Size2(380, 180) * EDSCALE);
	} else if (type == "audio" && dlg_bgm) {
		if (bgm_file_edit) bgm_file_edit->set_text(d.get("file", ""));
		if (bgm_volume_spin) bgm_volume_spin->set_value(int(d.get("volume", 90)));
		if (bgm_pitch_spin) bgm_pitch_spin->set_value(int(d.get("pitch", 100)));
		dlg_bgm->popup_centered(Size2(400, 200) * EDSCALE);
	} else if (type == "se" && dlg_se) {
		if (se_file_edit) se_file_edit->set_text(d.get("file", ""));
		if (se_volume_spin) se_volume_spin->set_value(int(d.get("volume", 90)));
		if (se_pitch_spin) se_pitch_spin->set_value(int(d.get("pitch", 100)));
		dlg_se->popup_centered(Size2(400, 200) * EDSCALE);
	} else if (type == "flash" && dlg_flash) {
		String col = d.get("color", "");
		if (flash_color_opt) {
			for (int i = 0; i < flash_color_opt->get_item_count(); i++) {
				if (flash_color_opt->get_item_text(i).contains(col)) {
					flash_color_opt->select(i);
					break;
				}
			}
		}
		if (flash_duration_spin) flash_duration_spin->set_value(int(d.get("duration", 30)));
		dlg_flash->popup_centered(Size2(360, 160) * EDSCALE);
	} else if (type == "tint" && dlg_tint) {
		String pres = d.get("preset", "");
		if (tint_preset_opt) {
			for (int i = 0; i < tint_preset_opt->get_item_count(); i++) {
				if (tint_preset_opt->get_item_text(i).contains(pres)) {
					tint_preset_opt->select(i);
					break;
				}
			}
		}
		if (tint_duration_spin) tint_duration_spin->set_value(int(d.get("duration", 60)));
		dlg_tint->popup_centered(Size2(360, 160) * EDSCALE);
	} else if (type == "fade" && dlg_fade) {
		if (fade_type_opt) fade_type_opt->select(d.get("mode", "Oscurecer") == "Oscurecer" ? 0 : 1);
		if (fade_duration_spin) fade_duration_spin->set_value(int(d.get("duration", 30)));
		dlg_fade->popup_centered(Size2(360, 160) * EDSCALE);
	} else if (type == "battle" && dlg_battle) {
		if (battle_troop_opt) {
			battle_troop_opt->clear();
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			Array troops = dm ? dm->get_category_array(CAT_TROOPS) : Array();
			if (troops.is_empty()) {
				battle_troop_opt->add_item(CG_TR("Tropa 1 (Por defecto)"), 1);
			} else {
				for (int i = 0; i < troops.size(); i++) {
					Dictionary tr = troops[i];
					int tid = int(tr.get("id", i + 1));
					String nm = tr.get("name", vformat("Tropa %03d", tid));
					battle_troop_opt->add_item(vformat("%03d: %s", tid, nm), tid);
				}
			}
			int sel_id = int(d.get("troop_id", 1));
			int opt_idx = battle_troop_opt->get_item_index(sel_id);
			if (opt_idx >= 0) battle_troop_opt->select(opt_idx);
		}
		if (battle_troop_edit) battle_troop_edit->set_text(d.get("troop", ""));
		if (battle_bgm_edit) battle_bgm_edit->set_text(d.get("bgm", ""));
		if (battle_can_escape_check) battle_can_escape_check->set_pressed(bool(d.get("can_escape", true)));
		if (battle_can_lose_check) battle_can_lose_check->set_pressed(bool(d.get("can_lose", false)));
		dlg_battle->popup_centered(Size2(480, 240) * EDSCALE);
	} else if ((type == "show_picture" || type == "picture") && dlg_show_picture) {
		if (pic_show_id_spin) pic_show_id_spin->set_value(int(d.get("id", 1)));
		if (pic_show_path_edit) pic_show_path_edit->set_text(d.get("path", ""));
		if (pic_show_origin_opt) pic_show_origin_opt->select(int(d.get("origin", 0)));
		if (pic_show_x_spin) pic_show_x_spin->set_value(int(d.get("x", 0)));
		if (pic_show_y_spin) pic_show_y_spin->set_value(int(d.get("y", 0)));
		if (pic_show_scale_x_spin) pic_show_scale_x_spin->set_value(int(d.get("scale_x", 100)));
		if (pic_show_scale_y_spin) pic_show_scale_y_spin->set_value(int(d.get("scale_y", 100)));
		if (pic_show_opacity_spin) pic_show_opacity_spin->set_value(int(float(d.get("opacity", 1.0f)) * 100.0f));
		if (pic_show_blend_opt) pic_show_blend_opt->select(int(d.get("blend", 0)));
		int dur_m = int(d.get("duration_mode", 0));
		if (pic_show_dur_mode_opt) {
			pic_show_dur_mode_opt->select(dur_m);
			_on_pic_show_dur_mode_selected(dur_m);
		}
		if (pic_show_dur_sec_spin) pic_show_dur_sec_spin->set_value(float(d.get("duration_sec", d.get("duration", 3.0f))));
		if (pic_show_fadeout_check) pic_show_fadeout_check->set_pressed(bool(d.get("auto_fadeout", false)));
		if (pic_show_wait_check) pic_show_wait_check->set_pressed(bool(d.get("wait", false)));
		if (pic_show_picker) {
			pic_show_picker->set_image_path(d.get("path", ""));
			pic_show_picker->set_origin_mode(int(d.get("origin", 0)));
			pic_show_picker->set_values(int(d.get("x", 0)), int(d.get("y", 0)), int(d.get("scale_x", 100)), int(d.get("scale_y", 100)));
		}
		dlg_show_picture->popup_centered(Size2(540, 580) * EDSCALE);
	} else if (type == "move_picture" && dlg_move_picture) {
		if (pic_move_id_spin) pic_move_id_spin->set_value(int(d.get("id", 1)));
		if (pic_move_origin_opt) pic_move_origin_opt->select(int(d.get("origin", 0)));
		if (pic_move_x_spin) pic_move_x_spin->set_value(int(d.get("x", 0)));
		if (pic_move_y_spin) pic_move_y_spin->set_value(int(d.get("y", 0)));
		if (pic_move_scale_x_spin) pic_move_scale_x_spin->set_value(int(d.get("scale_x", 100)));
		if (pic_move_scale_y_spin) pic_move_scale_y_spin->set_value(int(d.get("scale_y", 100)));
		if (pic_move_opacity_spin) pic_move_opacity_spin->set_value(int(float(d.get("opacity", 1.0f)) * 100.0f));
		if (pic_move_duration_spin) pic_move_duration_spin->set_value(int(d.get("duration", 60)));
		if (pic_move_easing_opt) pic_move_easing_opt->select(int(d.get("easing", 0)));
		if (pic_move_wait_check) pic_move_wait_check->set_pressed(bool(d.get("wait", false)));
		if (pic_move_picker) {
			pic_move_picker->set_origin_mode(int(d.get("origin", 0)));
			pic_move_picker->set_values(int(d.get("x", 0)), int(d.get("y", 0)), int(d.get("scale_x", 100)), int(d.get("scale_y", 100)));
		}
		dlg_move_picture->popup_centered(Size2(540, 580) * EDSCALE);
	} else if (type == "rotate_picture" && dlg_rotate_picture) {
		if (pic_rot_id_spin) pic_rot_id_spin->set_value(int(d.get("id", 1)));
		if (pic_rot_speed_spin) pic_rot_speed_spin->set_value(int(d.get("speed", 90)));
		dlg_rotate_picture->popup_centered(Size2(360, 160) * EDSCALE);
	} else if (type == "tint_picture" && dlg_tint_picture) {
		if (pic_tint_id_spin) pic_tint_id_spin->set_value(int(d.get("id", 1)));
		String pres = d.get("preset", "Normal");
		int pidx = (pres.find("Noche") >= 0 || pres.find("Oscuro") >= 0) ? 1 : ((pres.find("Atardecer") >= 0) ? 2 : ((pres.find("Sepia") >= 0) ? 3 : 0));
		if (pic_tint_preset_opt) pic_tint_preset_opt->select(pidx);
		if (pic_tint_duration_spin) pic_tint_duration_spin->set_value(int(d.get("duration", 60)));
		if (pic_tint_wait_check) pic_tint_wait_check->set_pressed(bool(d.get("wait", false)));
		dlg_tint_picture->popup_centered(Size2(380, 200) * EDSCALE);
	} else if (type == "erase_picture" && dlg_erase_picture) {
		if (pic_erase_id_spin) pic_erase_id_spin->set_value(int(d.get("id", 1)));
		dlg_erase_picture->popup_centered(Size2(320, 140) * EDSCALE);
	} else if (type == "change_camera_zoom" && dlg_change_camera_zoom) {
		float z = float(d.get("zoom", 2.0f));
		if (cam_zoom_spin) {
			cam_zoom_spin->set_value(z);
		}
		if (cam_zoom_duration_spin) cam_zoom_duration_spin->set_value(int(d.get("duration", 0)));
		if (cam_zoom_wait_check) cam_zoom_wait_check->set_pressed(bool(d.get("wait", false)));
		dlg_change_camera_zoom->popup_centered(Size2(400, 160) * EDSCALE);
	} else if ((type == "shake_screen" || type == "shake") && dlg_shake) {
		if (shake_power_spin) shake_power_spin->set_value(int(d.get("power", 5)));
		if (shake_speed_spin) shake_speed_spin->set_value(int(d.get("speed", 5)));
		if (shake_duration_spin) shake_duration_spin->set_value(int(d.get("duration", 60)));
		if (shake_wait_check) shake_wait_check->set_pressed(bool(d.get("wait", false)));
		dlg_shake->popup_centered(Size2(380, 200) * EDSCALE);
	} else if (type == "weather" && dlg_weather) {
		String w = d.get("weather", "none");
		int widx = (w == "rain" || w == "Lluvia") ? 1 : ((w == "storm" || w == "Tormenta") ? 2 : ((w == "snow" || w == "Nieve") ? 3 : 0));
		if (weather_type_opt) weather_type_opt->select(widx);
		if (weather_power_spin) weather_power_spin->set_value(int(d.get("power", 5)));
		if (weather_duration_spin) weather_duration_spin->set_value(int(d.get("duration", 60)));
		dlg_weather->popup_centered(Size2(380, 180) * EDSCALE);
	} else if ((type == "play_bgs" || type == "bgs") && dlg_bgs) {
		if (bgs_file_edit) bgs_file_edit->set_text(d.get("file", ""));
		if (bgs_volume_spin) bgs_volume_spin->set_value(int(d.get("volume", 90)));
		if (bgs_pitch_spin) bgs_pitch_spin->set_value(int(d.get("pitch", 100)));
		dlg_bgs->popup_centered(Size2(400, 200) * EDSCALE);
	} else if ((type == "play_me" || type == "me") && dlg_me) {
		if (me_file_edit) me_file_edit->set_text(d.get("file", ""));
		if (me_volume_spin) me_volume_spin->set_value(int(d.get("volume", 90)));
		if (me_pitch_spin) me_pitch_spin->set_value(int(d.get("pitch", 100)));
		dlg_me->popup_centered(Size2(400, 200) * EDSCALE);
	} else if ((type == "stop_fade_bgm" || type == "fadeout_bgm") && dlg_stop_fade_bgm) {
		if (bgm_stop_action_opt) bgm_stop_action_opt->select(d.get("action", "fade") == "stop" ? 1 : 0);
		if (bgm_stop_duration_spin) bgm_stop_duration_spin->set_value(int(d.get("duration", 60)));
		dlg_stop_fade_bgm->popup_centered(Size2(380, 160) * EDSCALE);
	} else if ((type == "restore_bgm" || type == "resume_bgm") && dlg_restore_bgm) {
		if (bgm_restore_duration_spin) bgm_restore_duration_spin->set_value(int(d.get("duration", 30)));
		dlg_restore_bgm->popup_centered(Size2(360, 150) * EDSCALE);
	} else if ((type == "audio_stems" || type == "stems") && dlg_stems) {
		if (stem_id_spin) stem_id_spin->set_value(int(d.get("stem", 0)));
		if (stem_file_edit) stem_file_edit->set_text(d.get("file", ""));
		if (stem_volume_spin) stem_volume_spin->set_value(int(float(d.get("volume", 1.0f)) * 100.0f));
		if (stem_fade_spin) stem_fade_spin->set_value(int(float(d.get("fade", 0.3f)) * 60.0f));
		dlg_stems->popup_centered(Size2(420, 240) * EDSCALE);
	} else if ((type == "play_cutscene" || type == "cutscene") && dlg_play_cutscene) {
		if (d.has("cutscene_dict") && dlg_create_cutscene) {
			Dictionary csd = d.get("cutscene_dict", Dictionary());
			if (cutscene_create_name_edit) cutscene_create_name_edit->set_text(csd.get("name", "Cinematica"));
			if (cutscene_create_dur_spin) cutscene_create_dur_spin->set_value(float(csd.get("duration", 5.0f)));
			if (cutscene_create_letterbox_check) cutscene_create_letterbox_check->set_pressed(bool(csd.get("letterbox", true)));
			if (cutscene_create_block_check) cutscene_create_block_check->set_pressed(bool(csd.get("block_player", true)));
			cutscene_temp_keyframes = csd.get("keyframes", Array());
			_refresh_cutscene_kf_tree();
			dlg_create_cutscene->popup_centered(Size2(560, 440) * EDSCALE);
		} else {
			if (cutscene_file_edit) cutscene_file_edit->set_text(d.get("path", ""));
			if (cutscene_wait_check) cutscene_wait_check->set_pressed(bool(d.get("wait", true)));
			dlg_play_cutscene->popup_centered(Size2(420, 180) * EDSCALE);
		}
	} else {
		_on_add_command_pressed();
	}
}

void ClockGearsEventEditorDialog::_on_move_command_up() {
	if (!command_tree) return;
	TreeItem *selected = command_tree->get_selected();
	if (!selected) return;
	int idx = int(selected->get_metadata(0));
	Array cmds = _get_active_page_commands();
	if (idx > 0 && idx < cmds.size()) {
		Variant tmp = cmds[idx];
		cmds[idx] = cmds[idx - 1];
		cmds[idx - 1] = tmp;
		_set_active_page_commands(cmds);
		_refresh_command_list();
		TreeItem *root = command_tree->get_root();
		if (root) {
			TreeItem *it = root->get_first_child();
			while (it) {
				if (int(it->get_metadata(0)) == idx - 1) {
					command_tree->set_selected(it);
					break;
				}
				it = it->get_next();
			}
		}
	}
}

void ClockGearsEventEditorDialog::_on_move_command_down() {
	if (!command_tree) return;
	TreeItem *selected = command_tree->get_selected();
	if (!selected) return;
	int idx = int(selected->get_metadata(0));
	Array cmds = _get_active_page_commands();
	if (idx >= 0 && idx < cmds.size() - 1) {
		Variant tmp = cmds[idx];
		cmds[idx] = cmds[idx + 1];
		cmds[idx + 1] = tmp;
		_set_active_page_commands(cmds);
		_refresh_command_list();
		TreeItem *root = command_tree->get_root();
		if (root) {
			TreeItem *it = root->get_first_child();
			while (it) {
				if (int(it->get_metadata(0)) == idx + 1) {
					command_tree->set_selected(it);
					break;
				}
				it = it->get_next();
			}
		}
	}
}

void ClockGearsEventEditorDialog::_on_command_id_selected(int p_cmd_id) {
	switch (p_cmd_id) {
		case CMD_SHOW_MESSAGE:
			if (dlg_msg) {
				if (msg_text_edit) msg_text_edit->set_text("");
				_update_msg_live_preview();
				dlg_msg->popup_centered(Size2(640, 520) * EDSCALE);
			}
			break;
		case CMD_SHOW_CHOICES:
			if (dlg_choices) {
				if (choice_1_edit) choice_1_edit->set_text("");
				if (choice_2_edit) choice_2_edit->set_text("");
				if (choice_3_edit) choice_3_edit->set_text("");
				if (choice_4_edit) choice_4_edit->set_text("");
				if (choice_5_edit) choice_5_edit->set_text("");
				if (choice_6_edit) choice_6_edit->set_text("");
				dlg_choices->popup_centered(Size2(420, 360) * EDSCALE);
			}
			break;
		case CMD_INPUT_NUMBER:
			if (dlg_number_input) {
				if (num_input_var_spin) num_input_var_spin->set_value(1);
				if (num_input_digits_spin) num_input_digits_spin->set_value(3);
				dlg_number_input->popup_centered(Size2(360, 180) * EDSCALE);
			}
			break;
		case CMD_CONTROL_SWITCH:
			if (dlg_switch) dlg_switch->popup_centered(Size2(380, 160) * EDSCALE);
			break;
		case CMD_CONTROL_SELF_SWITCH:
			if (dlg_self_switch) dlg_self_switch->popup_centered(Size2(360, 150) * EDSCALE);
			break;
		case CMD_CONTROL_VARIABLE:
			if (dlg_var) dlg_var->popup_centered(Size2(420, 180) * EDSCALE);
			break;
		case CMD_WAIT_FRAMES:
			if (dlg_wait) dlg_wait->popup_centered(Size2(320, 140) * EDSCALE);
			break;
		case CMD_TRANSFER_PLAYER:
			_populate_teleport_maps();
			if (dlg_teleport) dlg_teleport->popup_centered(Size2(420, 260) * EDSCALE);
			break;
		case CMD_SET_MOVE_ROUTE:
			if (dlg_move_route) dlg_move_route->popup_centered(Size2(380, 200) * EDSCALE);
			break;
		case CMD_ADD_ITEM:
			if (dlg_item) dlg_item->popup_centered(Size2(380, 180) * EDSCALE);
			break;
		case CMD_CHANGE_GOLD:
			if (dlg_gold) dlg_gold->popup_centered(Size2(360, 150) * EDSCALE);
			break;
		case CMD_CHANGE_HP_MP:
			if (dlg_hp_mp) dlg_hp_mp->popup_centered(Size2(380, 180) * EDSCALE);
			break;
		case CMD_PLAY_BGM:
			if (dlg_bgm) dlg_bgm->popup_centered(Size2(400, 200) * EDSCALE);
			break;
		case CMD_PLAY_SE:
			if (dlg_se) dlg_se->popup_centered(Size2(400, 200) * EDSCALE);
			break;
		case CMD_FLASH_EFFECT:
			if (dlg_flash) dlg_flash->popup_centered(Size2(360, 160) * EDSCALE);
			break;
		case CMD_TINT_SCREEN:
			if (dlg_tint) dlg_tint->popup_centered(Size2(360, 160) * EDSCALE);
			break;
		case CMD_FADE_SCREEN:
			if (dlg_fade) dlg_fade->popup_centered(Size2(360, 150) * EDSCALE);
			break;
		case CMD_BOSS_BATTLE:
			if (battle_troop_opt) {
				battle_troop_opt->clear();
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				Array troops = dm ? dm->get_category_array(CAT_TROOPS) : Array();
				if (troops.is_empty()) {
					battle_troop_opt->add_item(CG_TR("Tropa 1 (Por defecto)"), 1);
				} else {
					for (int i = 0; i < troops.size(); i++) {
						Dictionary tr = troops[i];
						int tid = int(tr.get("id", i + 1));
						String nm = tr.get("name", vformat("Tropa %03d", tid));
						battle_troop_opt->add_item(vformat("%03d: %s", tid, nm), tid);
					}
				}
			}
			if (dlg_battle) dlg_battle->popup_centered(Size2(480, 240) * EDSCALE);
			break;
		case CMD_SHOW_PICTURE:
			if (pic_show_picker && pic_show_x_spin && pic_show_y_spin && pic_show_scale_x_spin && pic_show_scale_y_spin) {
				pic_show_picker->set_image_path(pic_show_path_edit ? pic_show_path_edit->get_text() : "");
				pic_show_picker->set_origin_mode(pic_show_origin_opt ? pic_show_origin_opt->get_selected() : 0);
				pic_show_picker->set_values(int(pic_show_x_spin->get_value()), int(pic_show_y_spin->get_value()), int(pic_show_scale_x_spin->get_value()), int(pic_show_scale_y_spin->get_value()));
			}
			if (dlg_show_picture) dlg_show_picture->popup_centered(Size2(540, 580) * EDSCALE);
			break;
		case CMD_MOVE_PICTURE:
			if (pic_move_picker && pic_move_x_spin && pic_move_y_spin && pic_move_scale_x_spin && pic_move_scale_y_spin) {
				pic_move_picker->set_origin_mode(pic_move_origin_opt ? pic_move_origin_opt->get_selected() : 0);
				pic_move_picker->set_values(int(pic_move_x_spin->get_value()), int(pic_move_y_spin->get_value()), int(pic_move_scale_x_spin->get_value()), int(pic_move_scale_y_spin->get_value()));
			}
			if (dlg_move_picture) dlg_move_picture->popup_centered(Size2(540, 580) * EDSCALE);
			break;
		case CMD_ROTATE_PICTURE:
			if (dlg_rotate_picture) dlg_rotate_picture->popup_centered(Size2(360, 160) * EDSCALE);
			break;
		case CMD_TINT_PICTURE:
			if (dlg_tint_picture) dlg_tint_picture->popup_centered(Size2(380, 200) * EDSCALE);
			break;
		case CMD_ERASE_PICTURE:
			if (dlg_erase_picture) dlg_erase_picture->popup_centered(Size2(320, 140) * EDSCALE);
			break;
		case CMD_CHANGE_CAMERA_ZOOM:
			if (cam_zoom_spin) cam_zoom_spin->set_value(2.0);
			if (cam_zoom_opt) cam_zoom_opt->select(3); // 2.0x
			if (cam_zoom_duration_spin) cam_zoom_duration_spin->set_value(0);
			if (cam_zoom_wait_check) cam_zoom_wait_check->set_pressed(false);
			if (dlg_change_camera_zoom) dlg_change_camera_zoom->popup_centered(Size2(400, 160) * EDSCALE);
			break;
		case CMD_SHAKE_SCREEN:
			if (dlg_shake) dlg_shake->popup_centered(Size2(380, 200) * EDSCALE);
			break;
		case CMD_WEATHER:
			if (dlg_weather) dlg_weather->popup_centered(Size2(380, 180) * EDSCALE);
			break;
		case CMD_PLAY_BGS:
			if (dlg_bgs) dlg_bgs->popup_centered(Size2(400, 200) * EDSCALE);
			break;
		case CMD_PLAY_ME:
			if (dlg_me) dlg_me->popup_centered(Size2(400, 200) * EDSCALE);
			break;
		case CMD_STOP_FADE_BGM:
			if (dlg_stop_fade_bgm) dlg_stop_fade_bgm->popup_centered(Size2(380, 160) * EDSCALE);
			break;
		case CMD_SAVE_BGM: {
			Dictionary cmd;
			cmd["type"] = "save_bgm";
			cmd["code"] = CMD_SAVE_BGM;
			_append_command_to_active_page(cmd);
			break;
		}
		case CMD_RESTORE_BGM:
			if (dlg_restore_bgm) dlg_restore_bgm->popup_centered(Size2(360, 150) * EDSCALE);
			break;
		case CMD_AUDIO_STEMS:
			if (dlg_stems) dlg_stems->popup_centered(Size2(420, 240) * EDSCALE);
			break;
		case CMD_PLAY_CUTSCENE:
			if (dlg_play_cutscene) dlg_play_cutscene->popup_centered(Size2(420, 180) * EDSCALE);
			break;
		case CMD_CREATE_CUTSCENE:
			if (dlg_create_cutscene) {
				cutscene_temp_keyframes.clear();
				_refresh_cutscene_kf_tree();
				dlg_create_cutscene->popup_centered(Size2(560, 440) * EDSCALE);
			}
			break;
	}
}

// -------------------------------------------------------------
// Sub-dialog confirmation handlers
// -------------------------------------------------------------
void ClockGearsEventEditorDialog::_on_confirm_msg() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "message";
	cmd["text"] = msg_text_edit->get_text();
	cmd["frame"] = msg_frame_opt->get_selected();
	cmd["position"] = msg_pos_opt->get_selected();
	cmd["face"] = msg_face_edit->get_text();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_choices() {
	if (!target_event) return;
	bool is_new = (editing_command_index < 0);
	Dictionary cmd;
	cmd["type"] = "choices";
	cmd["c1"] = choice_1_edit->get_text();
	cmd["c2"] = choice_2_edit->get_text();
	cmd["c3"] = choice_3_edit->get_text();
	cmd["c4"] = choice_4_edit ? choice_4_edit->get_text() : "";
	cmd["c5"] = choice_5_edit ? choice_5_edit->get_text() : "";
	cmd["c6"] = choice_6_edit ? choice_6_edit->get_text() : "";
	int cancel_opt = choice_cancel_opt ? choice_cancel_opt->get_selected() : 0;
	cmd["cancel_opt"] = cancel_opt;
	_append_command_to_active_page(cmd);

	if (is_new) {
		int opt_idx = 0;
		for (int c = 1; c <= 6; c++) {
			String key = "c" + itos(c);
			String txt = cmd.get(key, "");
			if (!txt.strip_edges().is_empty()) {
				Dictionary b;
				b["type"] = "when_choice";
				b["choice"] = opt_idx++;
				b["text"] = txt;
				_append_command_to_active_page(b);
			}
		}
		if (cancel_opt == 2) {
			Dictionary bcanc;
			bcanc["type"] = "when_cancel";
			bcanc["choice"] = -1;
			_append_command_to_active_page(bcanc);
		}
		Dictionary bend;
		bend["type"] = "branch_end_choice";
		_append_command_to_active_page(bend);
	}
}

void ClockGearsEventEditorDialog::_on_confirm_number_input() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "number_input";
	cmd["variable"] = num_input_var_spin ? int(num_input_var_spin->get_value()) : 1;
	cmd["digits"] = num_input_digits_spin ? int(num_input_digits_spin->get_value()) : 3;
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_insert_msg_escape_code(const String &p_code) {
	if (msg_text_edit) {
		msg_text_edit->insert_text_at_caret(p_code);
		msg_text_edit->grab_focus();
		_update_msg_live_preview();
	}
}

void ClockGearsEventEditorDialog::_update_msg_live_preview() {
	if (!msg_text_edit || !msg_preview_label) {
		return;
	}

	String raw_text = msg_text_edit->get_text();

	// 1. Contador de líneas (estándar 4 líneas por mensaje en RPG Maker)
	int line_count = 1;
	for (int i = 0; i < raw_text.length(); i++) {
		if (raw_text[i] == '\n') {
			line_count++;
		}
	}
	if (msg_lines_label) {
		msg_lines_label->set_text(vformat(CG_TR("Líneas: %d / 4"), line_count));
		if (line_count > 4) {
			msg_lines_label->add_theme_color_override("font_color", Color(1.0f, 0.45f, 0.35f, 1.0f));
		} else {
			msg_lines_label->remove_theme_color_override("font_color");
		}
	}

	// 2. Parseo completo mediante ClockGearsTextParser (resuelve \\V, \\N, \\G, \\C, \\B, \\IT, etc. y extrae \\NM y \\E)
	ClockGearsTextParser::ParsedMessage parsed = ClockGearsTextParser::parse_full(raw_text);

	// Actualizar texto formateado con soporte BBCode
	msg_preview_label->set_text(parsed.bbcode_text);

	// 3. Caja de Nombre Flotante (\\NM[Nombre])
	if (msg_preview_name_box && msg_preview_name_label) {
		if (!parsed.speaker_name.is_empty()) {
			msg_preview_name_label->set_text(parsed.speaker_name);
			msg_preview_name_box->show();
		} else {
			msg_preview_name_box->hide();
		}
	}

	// 4. Retrato / Rostro
	if (msg_preview_face_rect) {
		String face_path = msg_face_edit ? msg_face_edit->get_text().strip_edges() : "";
		if (!face_path.is_empty()) {
			int frame_idx = (parsed.face_index >= 0) ? parsed.face_index : 0;
			if (!parsed.face_expression.is_empty()) {
				String exp = parsed.face_expression.to_lower().strip_edges();
				if (exp.is_valid_int()) {
					frame_idx = exp.to_int();
				} else if (exp == "neutral" || exp == "normal") {
					frame_idx = 0;
				} else if (exp == "feliz" || exp == "happy" || exp == "alegre" || exp == "sonrisa") {
					frame_idx = 1;
				} else if (exp == "triste" || exp == "sad") {
					frame_idx = 2;
				} else if (exp == "enojado" || exp == "angry" || exp == "ira" || exp == "molesto") {
					frame_idx = 3;
				} else if (exp == "sorprendido" || exp == "surprised" || exp == "asombro") {
					frame_idx = 4;
				} else if (exp == "asustado" || exp == "scared" || exp == "miedo") {
					frame_idx = 5;
				} else if (exp == "pensativo" || exp == "thinking") {
					frame_idx = 6;
				} else if (exp.begins_with("gui") || exp == "wink") {
					frame_idx = 7;
				}
			}
			Ref<Texture2D> tex = ResourceLoader::load(face_path);
			if (tex.is_valid()) {
				if (tex->get_width() >= 144 && tex->get_height() >= 144 && (tex->get_width() > 144 || tex->get_height() > 144)) {
					int cell_w = tex->get_width() / 4;
					int cell_h = tex->get_height() / 2;
					int col = frame_idx % 4;
					int row = (frame_idx / 4) % 2;
					Ref<AtlasTexture> atlas;
					atlas.instantiate();
					atlas->set_atlas(tex);
					atlas->set_region(Rect2(col * cell_w, row * cell_h, cell_w, cell_h));
					msg_preview_face_rect->set_texture(atlas);
				} else {
					msg_preview_face_rect->set_texture(tex);
				}
				msg_preview_face_rect->show();
			} else {
				msg_preview_face_rect->hide();
			}
		} else {
			msg_preview_face_rect->hide();
		}
	}

	// 5. Estilo de fondo según Fondo seleccionado (Ventana Normal, Oscurecer, Transparente)
	if (msg_preview_panel && msg_frame_opt) {
		int bg_mode = msg_frame_opt->get_selected();
		Ref<StyleBoxFlat> sb = memnew(StyleBoxFlat);
		sb->set_anti_aliased(false);
		sb->set_content_margin_all(8);
		if (bg_mode == 0) { // Ventana Normal
			sb->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.94f));
			sb->set_border_width_all(2);
			sb->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f));
			sb->set_corner_radius_all(4);
		} else if (bg_mode == 1) { // Oscurecer
			sb->set_bg_color(Color(0.02f, 0.03f, 0.05f, 0.75f));
			sb->set_border_width_all(0);
			sb->set_corner_radius_all(2);
		} else { // Transparente
			sb->set_bg_color(Color(0, 0, 0, 0));
			sb->set_border_width_all(0);
		}
		msg_preview_panel->add_theme_style_override("panel", sb);
	}
}

void ClockGearsEventEditorDialog::_on_confirm_switch() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "switch";
	cmd["name"] = sw_name_edit->get_text();
	cmd["val"] = (sw_val_opt->get_selected() == 0);
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_self_switch() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "self_switch";
	cmd["letter"] = self_sw_letter_opt->get_item_text(self_sw_letter_opt->get_selected());
	cmd["val"] = (self_sw_val_opt->get_selected() == 0);
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_var() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "variable";
	cmd["name"] = var_name_edit->get_text();
	static const char *ops[] = { "=", "+=", "-=", "*=", "/=" };
	cmd["op"] = ops[CLAMP(var_op_opt->get_selected(), 0, 4)];
	cmd["val"] = int(var_val_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_wait() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "wait";
	cmd["frames"] = int(wait_frames_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_populate_teleport_maps() {
	if (!tele_map_opt) {
		return;
	}
	tele_map_opt->clear();
	Vector<String> maps;
	if (DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			da->list_dir_begin();
			String f = da->get_next();
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
		}
	}
	maps.sort();
	for (int i = 0; i < maps.size(); i++) {
		tele_map_opt->add_item(maps[i].get_file(), i);
		tele_map_opt->set_item_metadata(i, maps[i]);
	}
	if (!maps.is_empty() && tele_map_edit && tele_map_edit->get_text().strip_edges().is_empty()) {
		tele_map_opt->select(0);
		tele_map_edit->set_text(maps[0]);
	}
}

void ClockGearsEventEditorDialog::_on_tele_map_opt_selected(int p_idx) {
	if (!tele_map_opt || !tele_map_edit || p_idx < 0 || p_idx >= tele_map_opt->get_item_count()) {
		return;
	}
	String path = tele_map_opt->get_item_metadata(p_idx);
	if (!path.is_empty()) {
		tele_map_edit->set_text(path);
	}
}

void ClockGearsEventEditorDialog::_on_confirm_teleport() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "teleport";
	String map_target = tele_map_edit ? tele_map_edit->get_text().strip_edges() : "";
	if (map_target.is_empty() && tele_map_opt && tele_map_opt->get_selected() >= 0) {
		map_target = tele_map_opt->get_item_metadata(tele_map_opt->get_selected());
	}
	cmd["map"] = map_target;
	cmd["x"] = int(tele_x_spin->get_value());
	cmd["y"] = int(tele_y_spin->get_value());
	static const char *dirs[] = { "Abajo", "Izquierda", "Derecha", "Arriba", "Mantener" };
	cmd["dir"] = dirs[CLAMP(tele_dir_opt->get_selected(), 0, 4)];
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_move_route() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "move_route";
	cmd["target"] = mr_target_opt->get_selected() == 0 ? "Este Evento" : "Jugador";
	cmd["action"] = mr_action_opt->get_text();
	cmd["repeat"] = mr_repeat_check->is_pressed();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_item() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "item";
	cmd["name"] = item_name_edit->get_text();
	cmd["op"] = item_op_opt->get_selected() == 0 ? "+" : "-";
	cmd["qty"] = int(item_qty_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_gold() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "gold";
	cmd["op"] = gold_op_opt->get_selected() == 0 ? "+" : "-";
	cmd["amount"] = int(gold_val_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_hp_mp() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "hp_mp";
	cmd["stat"] = hp_mp_stat_opt->get_selected() == 0 ? "HP" : "MP";
	cmd["op"] = hp_mp_op_opt->get_selected() == 0 ? "+" : "-";
	cmd["amount"] = int(hp_mp_amount_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_bgm() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "audio";
	cmd["file"] = bgm_file_edit->get_text();
	cmd["volume"] = int(bgm_volume_spin->get_value());
	cmd["pitch"] = int(bgm_pitch_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_se() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "se";
	cmd["file"] = se_file_edit->get_text();
	cmd["volume"] = int(se_volume_spin->get_value());
	cmd["pitch"] = int(se_pitch_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_browse_se_pressed() {
	if (se_file_dialog) {
		String default_dir = ClockGearsProjectStructure::resolve_project_folder_global("audio/se");
		if (DirAccess::dir_exists_absolute(default_dir)) {
			se_file_dialog->set_current_dir(default_dir);
		}
		se_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_se_file_selected(const String &p_path) {
	if (se_file_edit) {
		se_file_edit->set_text(p_path);
	}
}

void ClockGearsEventEditorDialog::_on_confirm_flash() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "flash";
	cmd["color"] = flash_color_opt->get_text();
	cmd["duration"] = int(flash_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_tint() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "tint";
	cmd["preset"] = tint_preset_opt->get_text();
	cmd["duration"] = int(tint_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_fade() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "fade";
	cmd["mode"] = fade_type_opt->get_selected() == 0 ? "Oscurecer" : "Aclarar";
	cmd["duration"] = int(fade_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_battle() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "battle";
	cmd["code"] = CMD_BOSS_BATTLE;
	int tid = 1;
	if (battle_troop_opt && battle_troop_opt->get_selected() >= 0) {
		tid = battle_troop_opt->get_selected_id();
	}
	cmd["troop_id"] = tid;
	String t_name = battle_troop_edit ? battle_troop_edit->get_text().strip_edges() : "";
	if (t_name.is_empty() && battle_troop_opt && battle_troop_opt->get_selected() >= 0) {
		t_name = battle_troop_opt->get_item_text(battle_troop_opt->get_selected());
	}
	cmd["troop"] = t_name;
	cmd["can_escape"] = battle_can_escape_check ? battle_can_escape_check->is_pressed() : true;
	cmd["can_lose"] = battle_can_lose_check ? battle_can_lose_check->is_pressed() : false;
	cmd["bgm"] = battle_bgm_edit ? battle_bgm_edit->get_text().strip_edges() : "";
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_battle_bgm_browse_pressed() {
	if (battle_bgm_file_dialog) {
		battle_bgm_file_dialog->popup_centered(Size2(600, 400) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_battle_bgm_selected(const String &p_path) {
	if (battle_bgm_edit) {
		battle_bgm_edit->set_text(p_path);
	}
}

// -------------------------------------------------------------
// Handlers de Confirmación y Timeline Fase 7B
// -------------------------------------------------------------
void ClockGearsEventEditorDialog::_on_confirm_show_picture() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "show_picture";
	cmd["code"] = CMD_SHOW_PICTURE;
	cmd["id"] = int(pic_show_id_spin->get_value());
	cmd["path"] = pic_show_path_edit->get_text();
	cmd["origin"] = pic_show_origin_opt->get_selected();
	cmd["x"] = int(pic_show_x_spin->get_value());
	cmd["y"] = int(pic_show_y_spin->get_value());
	cmd["scale_x"] = int(pic_show_scale_x_spin->get_value());
	cmd["scale_y"] = int(pic_show_scale_y_spin->get_value());
	cmd["opacity"] = float(pic_show_opacity_spin->get_value()) / 100.0f;
	cmd["blend"] = pic_show_blend_opt->get_selected();
	cmd["duration_mode"] = pic_show_dur_mode_opt ? pic_show_dur_mode_opt->get_selected() : 0;
	cmd["duration_sec"] = pic_show_dur_sec_spin ? float(pic_show_dur_sec_spin->get_value()) : 3.0f;
	cmd["auto_fadeout"] = pic_show_fadeout_check ? pic_show_fadeout_check->is_pressed() : false;
	cmd["wait"] = pic_show_wait_check ? pic_show_wait_check->is_pressed() : false;
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_move_picture() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "move_picture";
	cmd["code"] = CMD_MOVE_PICTURE;
	cmd["id"] = int(pic_move_id_spin->get_value());
	cmd["origin"] = pic_move_origin_opt->get_selected();
	cmd["x"] = int(pic_move_x_spin->get_value());
	cmd["y"] = int(pic_move_y_spin->get_value());
	cmd["scale_x"] = int(pic_move_scale_x_spin->get_value());
	cmd["scale_y"] = int(pic_move_scale_y_spin->get_value());
	cmd["opacity"] = float(pic_move_opacity_spin->get_value()) / 100.0f;
	cmd["duration"] = int(pic_move_duration_spin->get_value());
	cmd["easing"] = pic_move_easing_opt->get_selected();
	cmd["wait"] = pic_move_wait_check->is_pressed();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_rotate_picture() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "rotate_picture";
	cmd["code"] = CMD_ROTATE_PICTURE;
	cmd["id"] = int(pic_rot_id_spin->get_value());
	cmd["speed"] = float(pic_rot_speed_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_tint_picture() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "tint_picture";
	cmd["code"] = CMD_TINT_PICTURE;
	cmd["id"] = int(pic_tint_id_spin->get_value());
	int pidx = pic_tint_preset_opt->get_selected();
	String pres = "Normal";
	Color col = Color(1, 1, 1, 1);
	if (pidx == 1) { pres = "Oscuro / Noche"; col = Color(0.1f, 0.15f, 0.45f, 1.0f); }
	else if (pidx == 2) { pres = "Atardecer"; col = Color(0.6f, 0.35f, 0.2f, 1.0f); }
	else if (pidx == 3) { pres = "Sepia"; col = Color(0.5f, 0.4f, 0.3f, 1.0f); }
	cmd["preset"] = pres;
	cmd["color"] = col;
	cmd["duration"] = int(pic_tint_duration_spin->get_value());
	cmd["wait"] = pic_tint_wait_check->is_pressed();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_erase_picture() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "erase_picture";
	cmd["code"] = CMD_ERASE_PICTURE;
	cmd["id"] = int(pic_erase_id_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_pic_show_adv_toggle() {
	if (!pic_show_adv_box || !pic_show_adv_btn) return;
	bool vis = !pic_show_adv_box->is_visible();
	pic_show_adv_box->set_visible(vis);
	pic_show_adv_btn->set_text(vis ? CG_TR("▼ Avanzado (Origen, Mezcla, Coordenadas y Escala)") : CG_TR("▶ Avanzado (Origen, Mezcla, Coordenadas y Escala)"));
	if (dlg_show_picture) dlg_show_picture->reset_size();
}

void ClockGearsEventEditorDialog::_on_pic_move_adv_toggle() {
	if (!pic_move_adv_box || !pic_move_adv_btn) return;
	bool vis = !pic_move_adv_box->is_visible();
	pic_move_adv_box->set_visible(vis);
	pic_move_adv_btn->set_text(vis ? CG_TR("▼ Avanzado (Origen, Coordenadas y Escala de Destino)") : CG_TR("▶ Avanzado (Origen, Coordenadas y Escala de Destino)"));
	if (dlg_move_picture) dlg_move_picture->reset_size();
}

void ClockGearsEventEditorDialog::_on_cam_zoom_preset_selected(int p_idx) {
	if (!cam_zoom_opt || !cam_zoom_spin) return;
	int id = cam_zoom_opt->get_selected_id();
	if (id > 0) {
		cam_zoom_spin->set_value(float(id) / 10.0f);
	}
}

void ClockGearsEventEditorDialog::_on_confirm_change_camera_zoom() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "change_camera_zoom";
	cmd["code"] = CMD_CHANGE_CAMERA_ZOOM;
	float z = cam_zoom_spin ? float(cam_zoom_spin->get_value()) : 2.0f;
	cmd["zoom"] = z;
	cmd["duration"] = cam_zoom_duration_spin ? int(cam_zoom_duration_spin->get_value()) : 0;
	cmd["wait"] = cam_zoom_wait_check ? cam_zoom_wait_check->is_pressed() : false;
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_shake() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "shake_screen";
	cmd["code"] = CMD_SHAKE_SCREEN;
	cmd["power"] = float(shake_power_spin->get_value());
	cmd["speed"] = float(shake_speed_spin->get_value());
	cmd["duration"] = int(shake_duration_spin->get_value());
	cmd["wait"] = shake_wait_check->is_pressed();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_weather() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "weather";
	cmd["code"] = CMD_WEATHER;
	static const char *wtypes[] = { "none", "rain", "storm", "snow" };
	int sel = weather_type_opt->get_selected();
	cmd["weather"] = (sel >= 0 && sel < 4) ? wtypes[sel] : "none";
	cmd["power"] = float(weather_power_spin->get_value());
	cmd["duration"] = int(weather_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_bgs() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "play_bgs";
	cmd["code"] = CMD_PLAY_BGS;
	cmd["file"] = bgs_file_edit->get_text();
	cmd["volume"] = float(bgs_volume_spin->get_value());
	cmd["pitch"] = float(bgs_pitch_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_me() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "play_me";
	cmd["code"] = CMD_PLAY_ME;
	cmd["file"] = me_file_edit->get_text();
	cmd["volume"] = float(me_volume_spin->get_value());
	cmd["pitch"] = float(me_pitch_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_stop_fade_bgm() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "stop_fade_bgm";
	cmd["code"] = CMD_STOP_FADE_BGM;
	cmd["action"] = (bgm_stop_action_opt->get_selected() == 1) ? "stop" : "fade";
	cmd["duration"] = int(bgm_stop_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_restore_bgm() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "restore_bgm";
	cmd["code"] = CMD_RESTORE_BGM;
	cmd["duration"] = int(bgm_restore_duration_spin->get_value());
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_stems() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "audio_stems";
	cmd["code"] = CMD_AUDIO_STEMS;
	cmd["stem"] = int(stem_id_spin->get_value());
	cmd["file"] = stem_file_edit->get_text();
	cmd["volume"] = float(stem_volume_spin->get_value()) / 100.0f;
	cmd["fade"] = float(stem_fade_spin->get_value()) / 60.0f;
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_play_cutscene() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "play_cutscene";
	cmd["code"] = CMD_PLAY_CUTSCENE;
	cmd["path"] = cutscene_file_edit->get_text();
	cmd["wait"] = cutscene_wait_check->is_pressed();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_confirm_create_cutscene() {
	if (!target_event) return;
	Dictionary cmd;
	cmd["type"] = "play_cutscene";
	cmd["code"] = CMD_PLAY_CUTSCENE;
	Dictionary cs_dict;
	cs_dict["name"] = cutscene_create_name_edit->get_text();
	cs_dict["duration"] = float(cutscene_create_dur_spin->get_value());
	cs_dict["letterbox"] = cutscene_create_letterbox_check->is_pressed();
	cs_dict["block_player"] = cutscene_create_block_check->is_pressed();
	cs_dict["keyframes"] = cutscene_temp_keyframes;
	cmd["cutscene_dict"] = cs_dict;
	cmd["name"] = cutscene_create_name_edit->get_text();
	_append_command_to_active_page(cmd);
}

void ClockGearsEventEditorDialog::_on_cutscene_add_kf() {
	if (!kf_time_spin || !kf_track_opt || !kf_value_edit) return;
	Dictionary kf;
	kf["time"] = float(kf_time_spin->get_value());
	kf["track"] = kf_track_opt->get_selected();
	kf["value"] = kf_value_edit->get_text();
	cutscene_temp_keyframes.push_back(kf);
	_refresh_cutscene_kf_tree();
}

void ClockGearsEventEditorDialog::_on_cutscene_remove_kf() {
	if (!cutscene_kf_tree) return;
	TreeItem *sel = cutscene_kf_tree->get_selected();
	if (!sel) return;
	int idx = int(sel->get_metadata(0));
	if (idx >= 0 && idx < cutscene_temp_keyframes.size()) {
		cutscene_temp_keyframes.remove_at(idx);
		_refresh_cutscene_kf_tree();
	}
}

void ClockGearsEventEditorDialog::_refresh_cutscene_kf_tree() {
	if (!cutscene_kf_tree) return;
	cutscene_kf_tree->clear();
	TreeItem *root = cutscene_kf_tree->create_item();
	static const char *track_names[] = {
		"Cámara X", "Cámara Y", "Zoom", "Fundido Pantalla", "Mover Actor", "Diálogo", "Audio", "Espera"
	};
	for (int i = 0; i < cutscene_temp_keyframes.size(); i++) {
		Dictionary kf = cutscene_temp_keyframes[i];
		TreeItem *item = cutscene_kf_tree->create_item(root);
		item->set_text(0, String::num_real(float(kf.get("time", 0.0f)), false));
		int tr = int(kf.get("track", 0));
		String tr_str = (tr >= 0 && tr < 8) ? track_names[tr] : itos(tr);
		item->set_text(1, tr_str);
		item->set_text(2, kf.get("value", ""));
		item->set_metadata(0, i);
	}
}

void ClockGearsEventEditorDialog::_on_browse_pic_pressed() {
	if (pic_show_file_dialog) {
		String d = ClockGearsProjectStructure::resolve_project_folder_global("pictures");
		if (DirAccess::dir_exists_absolute(d)) pic_show_file_dialog->set_current_dir(d);
		pic_show_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_pic_file_selected(const String &p_path) {
	if (pic_show_path_edit) pic_show_path_edit->set_text(p_path);
	if (pic_show_picker) pic_show_picker->set_image_path(p_path);
}

void ClockGearsEventEditorDialog::_on_pic_show_dur_mode_selected(int p_idx) {
	bool is_sec = (p_idx == 1);
	if (pic_show_dur_sec_spin) pic_show_dur_sec_spin->set_visible(is_sec);
	if (pic_show_fadeout_check) pic_show_fadeout_check->set_visible(is_sec);
	if (pic_show_wait_check) pic_show_wait_check->set_visible(is_sec);
}

void ClockGearsEventEditorDialog::_on_pic_show_origin_selected(int p_idx) {
	if (pic_show_picker) pic_show_picker->set_origin_mode(p_idx);
}

void ClockGearsEventEditorDialog::_on_pic_move_origin_selected(int p_idx) {
	if (pic_move_picker) pic_move_picker->set_origin_mode(p_idx);
}

void ClockGearsEventEditorDialog::_on_pic_show_path_changed(const String &p_text) {
	if (pic_show_picker) pic_show_picker->set_image_path(p_text);
}

void ClockGearsEventEditorDialog::_on_browse_bgs_pressed() {
	if (bgs_file_dialog) {
		String d = ClockGearsProjectStructure::resolve_project_folder_global("audio/bgs");
		if (DirAccess::dir_exists_absolute(d)) bgs_file_dialog->set_current_dir(d);
		bgs_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_bgs_file_selected(const String &p_path) {
	if (bgs_file_edit) bgs_file_edit->set_text(p_path);
}

void ClockGearsEventEditorDialog::_on_browse_me_pressed() {
	if (me_file_dialog) {
		String d = ClockGearsProjectStructure::resolve_project_folder_global("audio/me");
		if (DirAccess::dir_exists_absolute(d)) me_file_dialog->set_current_dir(d);
		me_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_me_file_selected(const String &p_path) {
	if (me_file_edit) me_file_edit->set_text(p_path);
}

void ClockGearsEventEditorDialog::_on_browse_stem_pressed() {
	if (stem_file_dialog) {
		String d = ClockGearsProjectStructure::resolve_project_folder_global("audio/bgm");
		if (DirAccess::dir_exists_absolute(d)) stem_file_dialog->set_current_dir(d);
		stem_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_stem_file_selected(const String &p_path) {
	if (stem_file_edit) stem_file_edit->set_text(p_path);
}

void ClockGearsEventEditorDialog::_on_browse_cutscene_pressed() {
	if (cutscene_file_dialog) {
		String d = ClockGearsProjectStructure::resolve_project_folder_global("cutscenes");
		if (DirAccess::dir_exists_absolute(d)) cutscene_file_dialog->set_current_dir(d);
		cutscene_file_dialog->popup_centered(Size2(680, 460) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_on_cutscene_file_selected(const String &p_path) {
	if (cutscene_file_edit) cutscene_file_edit->set_text(p_path);
}

void ClockGearsEventEditorDialog::_setup_command_subdialogs() {
	// 1. Mostrar Mensaje / Texto (Fase 7A: con Vista Previa en Vivo y Barra de Códigos)
	dlg_msg = memnew(ConfirmationDialog);
	dlg_msg->set_title(CG_TR("Mostrar Mensaje / Diálogo"));
	dlg_msg->set_ok_button_text(CG_TR("Aceptar"));
	dlg_msg->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *mv = memnew(VBoxContainer);
	mv->add_theme_constant_override("separation", 6);
	dlg_msg->add_child(mv);

	HBoxContainer *mface = memnew(HBoxContainer);
	mface->add_child(memnew(Label(CG_TR("Rostro (Opcional):"))));
	msg_face_edit = memnew(LineEdit);
	msg_face_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	msg_face_edit->set_placeholder("res://img/faces/actor.png");
	msg_face_edit->connect("text_changed", callable_mp(this, &ClockGearsEventEditorDialog::_update_msg_live_preview).unbind(1));
	mface->add_child(msg_face_edit);
	msg_face_browse_btn = memnew(Button(CG_TR("...")));
	msg_face_browse_btn->set_tooltip_text(CG_TR("Seleccionar Gráfico de Rostro (img/faces)"));
	msg_face_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_face_pressed));
	mface->add_child(msg_face_browse_btn);
	mv->add_child(mface);

	// Barra de Códigos de Escape rápidos
	HBoxContainer *mquick = memnew(HBoxContainer);
	mquick->add_theme_constant_override("separation", 4);
	mquick->add_child(memnew(Label(CG_TR("Insertar:"))));

	auto add_esc_btn = [this, mquick](const String &label, const String &code, const String &tooltip) {
		Button *b = memnew(Button(label));
		b->set_tooltip_text(tooltip);
		b->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_insert_msg_escape_code).bind(code));
		mquick->add_child(b);
	};

	add_esc_btn("\\V[n]", "\\V[1]", CG_TR("Valor de la Variable (\\V[1])"));
	add_esc_btn("\\N[n]", "\\N[1]", CG_TR("Nombre del Héroe (\\N[1])"));
	add_esc_btn("\\C[n]", "\\C[1]", CG_TR("Color de Texto de la Paleta 0-31 (\\C[1])"));
	add_esc_btn("\\I[n]", "\\I[1]", CG_TR("Icono de la Base de Datos (\\I[1])"));
	add_esc_btn("\\NM[txt]", "\\NM[Nombre]", CG_TR("Caja flotante de Nombre (\\NM[Nombre])"));
	add_esc_btn("\\E[exp]", "\\E[feliz]", CG_TR("Expresión de Retrato (\\E[feliz])"));
	add_esc_btn("\\S[n]", "\\S[35]", CG_TR("Velocidad de tipeo en caracteres/s (\\S[35])"));
	add_esc_btn("\\!", "\\!", CG_TR("Pausa esperando botón del jugador (\\!)"));
	add_esc_btn("\\.", "\\.", CG_TR("Pausa breve de 15 frames (\\.)"));
	mv->add_child(mquick);

	// Encabezado de texto + Contador de líneas
	HBoxContainer *mtext_hdr = memnew(HBoxContainer);
	mtext_hdr->add_child(memnew(Label(CG_TR("Texto del Diálogo:"))));
	Control *msp = memnew(Control);
	msp->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	mtext_hdr->add_child(msp);
	msg_lines_label = memnew(Label(CG_TR("Líneas: 1 / 4")));
	mtext_hdr->add_child(msg_lines_label);
	mv->add_child(mtext_hdr);

	msg_text_edit = memnew(TextEdit);
	msg_text_edit->set_custom_minimum_size(Size2(0, 95) * EDSCALE);
	msg_text_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	msg_text_edit->connect("text_changed", callable_mp(this, &ClockGearsEventEditorDialog::_update_msg_live_preview));
	mv->add_child(msg_text_edit);

	HBoxContainer *mopts = memnew(HBoxContainer);
	mopts->add_theme_constant_override("separation", 8);
	mopts->add_child(memnew(Label(CG_TR("Fondo:"))));
	msg_frame_opt = memnew(OptionButton);
	msg_frame_opt->add_item(CG_TR("Ventana Normal"), 0);
	msg_frame_opt->add_item(CG_TR("Oscurecer"), 1);
	msg_frame_opt->add_item(CG_TR("Transparente"), 2);
	msg_frame_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_update_msg_live_preview).unbind(1));
	mopts->add_child(msg_frame_opt);

	mopts->add_child(memnew(Label(CG_TR("Posición:"))));
	msg_pos_opt = memnew(OptionButton);
	msg_pos_opt->add_item(CG_TR("Inferior"), 0);
	msg_pos_opt->add_item(CG_TR("Centro"), 1);
	msg_pos_opt->add_item(CG_TR("Superior"), 2);
	mopts->add_child(msg_pos_opt);
	mv->add_child(mopts);

	// Separador visual
	mv->add_child(memnew(HSeparator));

	// Vista Previa en Vivo (Pixel Art)
	mv->add_child(memnew(Label(CG_TR("Vista Previa en Vivo (Pixel Art):"))));
	VBoxContainer *prev_col = memnew(VBoxContainer);
	prev_col->add_theme_constant_override("separation", 2);

	msg_preview_name_box = memnew(PanelContainer);
	Ref<StyleBoxFlat> nb_style = memnew(StyleBoxFlat);
	nb_style->set_anti_aliased(false);
	nb_style->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.94f));
	nb_style->set_border_width_all(2);
	nb_style->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f));
	nb_style->set_corner_radius_all(3);
	nb_style->set_content_margin(SIDE_LEFT, 8);
	nb_style->set_content_margin(SIDE_RIGHT, 8);
	nb_style->set_content_margin(SIDE_TOP, 2);
	nb_style->set_content_margin(SIDE_BOTTOM, 2);
	msg_preview_name_box->add_theme_style_override("panel", nb_style);
	msg_preview_name_box->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
	msg_preview_name_label = memnew(Label);
	msg_preview_name_label->add_theme_color_override("font_color", Color(1.0f, 0.92f, 0.55f, 1.0f));
	msg_preview_name_box->add_child(msg_preview_name_label);
	msg_preview_name_box->hide();
	prev_col->add_child(msg_preview_name_box);

	msg_preview_panel = memnew(PanelContainer);
	msg_preview_panel->set_custom_minimum_size(Size2(0, 110) * EDSCALE);
	Ref<StyleBoxFlat> pw_style = memnew(StyleBoxFlat);
	pw_style->set_anti_aliased(false);
	pw_style->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.94f));
	pw_style->set_border_width_all(2);
	pw_style->set_border_color(Color(0.88f, 0.63f, 0.25f, 1.0f));
	pw_style->set_corner_radius_all(4);
	pw_style->set_content_margin_all(8);
	msg_preview_panel->add_theme_style_override("panel", pw_style);

	HBoxContainer *prev_hbox = memnew(HBoxContainer);
	prev_hbox->add_theme_constant_override("separation", 10);
	msg_preview_panel->add_child(prev_hbox);

	msg_preview_face_rect = memnew(TextureRect);
	msg_preview_face_rect->set_custom_minimum_size(Size2(80, 80) * EDSCALE);
	msg_preview_face_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	msg_preview_face_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	msg_preview_face_rect->hide();
	prev_hbox->add_child(msg_preview_face_rect);

	msg_preview_label = memnew(RichTextLabel);
	msg_preview_label->set_use_bbcode(true);
	msg_preview_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	msg_preview_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	msg_preview_label->set_fit_content(false);
	msg_preview_label->set_scroll_active(true);
	prev_hbox->add_child(msg_preview_label);

	prev_col->add_child(msg_preview_panel);
	mv->add_child(prev_col);

	dlg_msg->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_msg));
	add_child(dlg_msg);

	msg_face_file_dialog = memnew(FileDialog);
	msg_face_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	msg_face_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	msg_face_file_dialog->add_filter(String::utf8("*.png, *.webp ; Imágenes de Rostro (*.png, *.webp)"));
	msg_face_file_dialog->set_title(CG_TR("Seleccionar Gráfico de Rostro"));
	msg_face_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_face_file_selected));
	add_child(msg_face_file_dialog);

	// 2. Mostrar Elecciones (Hasta 6 opciones según RPG Maker MZ / Anexo A1.1)
	dlg_choices = memnew(ConfirmationDialog);
	dlg_choices->set_title(CG_TR("Mostrar Elecciones"));
	dlg_choices->set_ok_button_text(CG_TR("Aceptar"));
	dlg_choices->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *cv = memnew(VBoxContainer);
	cv->add_theme_constant_override("separation", 6);
	dlg_choices->add_child(cv);
	cv->add_child(memnew(Label(CG_TR("Opción 1:"))));
	choice_1_edit = memnew(LineEdit);
	cv->add_child(choice_1_edit);
	cv->add_child(memnew(Label(CG_TR("Opción 2:"))));
	choice_2_edit = memnew(LineEdit);
	cv->add_child(choice_2_edit);
	cv->add_child(memnew(Label(CG_TR("Opción 3 (Opcional):"))));
	choice_3_edit = memnew(LineEdit);
	cv->add_child(choice_3_edit);
	cv->add_child(memnew(Label(CG_TR("Opción 4 (Opcional):"))));
	choice_4_edit = memnew(LineEdit);
	cv->add_child(choice_4_edit);
	cv->add_child(memnew(Label(CG_TR("Opción 5 (Opcional):"))));
	choice_5_edit = memnew(LineEdit);
	cv->add_child(choice_5_edit);
	cv->add_child(memnew(Label(CG_TR("Opción 6 (Opcional):"))));
	choice_6_edit = memnew(LineEdit);
	cv->add_child(choice_6_edit);
	HBoxContainer *ccanc = memnew(HBoxContainer);
	ccanc->add_child(memnew(Label(CG_TR("Al Cancelar:"))));
	choice_cancel_opt = memnew(OptionButton);
	choice_cancel_opt->add_item(CG_TR("Opción 2"), 0);
	choice_cancel_opt->add_item(CG_TR("Prohibir"), 1);
	choice_cancel_opt->add_item(CG_TR("Rama Propia"), 2);
	ccanc->add_child(choice_cancel_opt);
	cv->add_child(ccanc);
	dlg_choices->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_choices));
	add_child(dlg_choices);

	// Entrada de Número (CMD_INPUT_NUMBER)
	dlg_number_input = memnew(ConfirmationDialog);
	dlg_number_input->set_title(CG_TR("Entrada de Número"));
	dlg_number_input->set_ok_button_text(CG_TR("Aceptar"));
	dlg_number_input->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *num_vbox = memnew(VBoxContainer);
	num_vbox->add_theme_constant_override("separation", 6);
	dlg_number_input->add_child(num_vbox);

	HBoxContainer *num_h1 = memnew(HBoxContainer);
	num_h1->add_child(memnew(Label(CG_TR("Variable Destino (ID):"))));
	num_input_var_spin = memnew(SpinBox);
	num_input_var_spin->set_min(1);
	num_input_var_spin->set_max(9999);
	num_input_var_spin->set_value(1);
	num_input_var_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	num_h1->add_child(num_input_var_spin);
	num_vbox->add_child(num_h1);

	HBoxContainer *num_h2 = memnew(HBoxContainer);
	num_h2->add_child(memnew(Label(CG_TR("Número de Dígitos (1-8):"))));
	num_input_digits_spin = memnew(SpinBox);
	num_input_digits_spin->set_min(1);
	num_input_digits_spin->set_max(8);
	num_input_digits_spin->set_value(3);
	num_input_digits_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	num_h2->add_child(num_input_digits_spin);
	num_vbox->add_child(num_h2);

	dlg_number_input->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_number_input));
	add_child(dlg_number_input);

	// 3. Control de Interruptores
	dlg_switch = memnew(ConfirmationDialog);
	dlg_switch->set_title(CG_TR("Control de Interruptores"));
	dlg_switch->set_ok_button_text(CG_TR("Aceptar"));
	dlg_switch->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *swv = memnew(VBoxContainer);
	swv->add_theme_constant_override("separation", 6);
	dlg_switch->add_child(swv);
	HBoxContainer *swh1 = memnew(HBoxContainer);
	swh1->add_child(memnew(Label(CG_TR("Interruptor:"))));
	sw_name_edit = memnew(LineEdit);
	sw_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	swh1->add_child(sw_name_edit);
	swv->add_child(swh1);
	HBoxContainer *swh2 = memnew(HBoxContainer);
	swh2->add_child(memnew(Label(CG_TR("Operación:"))));
	sw_val_opt = memnew(OptionButton);
	sw_val_opt->add_item(CG_TR("ON (Activado)"), 0);
	sw_val_opt->add_item(CG_TR("OFF (Desactivado)"), 1);
	swh2->add_child(sw_val_opt);
	swv->add_child(swh2);
	dlg_switch->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_switch));
	add_child(dlg_switch);

	// Control de Interruptor Local (A, B, C, D)
	dlg_self_switch = memnew(ConfirmationDialog);
	dlg_self_switch->set_title(CG_TR("Control de Interruptor Local"));
	dlg_self_switch->set_ok_button_text(CG_TR("Aceptar"));
	dlg_self_switch->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *sswv = memnew(VBoxContainer);
	sswv->add_theme_constant_override("separation", 6);
	dlg_self_switch->add_child(sswv);
	HBoxContainer *sswh1 = memnew(HBoxContainer);
	sswh1->add_child(memnew(Label(CG_TR("Interruptor Local:"))));
	self_sw_letter_opt = memnew(OptionButton);
	self_sw_letter_opt->add_item("A", 0);
	self_sw_letter_opt->add_item("B", 1);
	self_sw_letter_opt->add_item("C", 2);
	self_sw_letter_opt->add_item("D", 3);
	self_sw_letter_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sswh1->add_child(self_sw_letter_opt);
	sswv->add_child(sswh1);
	HBoxContainer *sswh2 = memnew(HBoxContainer);
	sswh2->add_child(memnew(Label(CG_TR("Operación:"))));
	self_sw_val_opt = memnew(OptionButton);
	self_sw_val_opt->add_item(CG_TR("ON (Activado)"), 0);
	self_sw_val_opt->add_item(CG_TR("OFF (Desactivado)"), 1);
	sswh2->add_child(self_sw_val_opt);
	sswv->add_child(sswh2);
	dlg_self_switch->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_self_switch));
	add_child(dlg_self_switch);

	// 4. Control de Variables
	dlg_var = memnew(ConfirmationDialog);
	dlg_var->set_title(CG_TR("Control de Variables"));
	dlg_var->set_ok_button_text(CG_TR("Aceptar"));
	dlg_var->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *vv = memnew(VBoxContainer);
	vv->add_theme_constant_override("separation", 6);
	dlg_var->add_child(vv);
	HBoxContainer *vh1 = memnew(HBoxContainer);
	vh1->add_child(memnew(Label(CG_TR("Variable:"))));
	var_name_edit = memnew(LineEdit);
	var_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vh1->add_child(var_name_edit);
	vv->add_child(vh1);
	HBoxContainer *vh2 = memnew(HBoxContainer);
	vh2->add_child(memnew(Label(CG_TR("Operación:"))));
	var_op_opt = memnew(OptionButton);
	var_op_opt->add_item(CG_TR("= Asignar"), 0);
	var_op_opt->add_item(CG_TR("+= Sumar"), 1);
	var_op_opt->add_item(CG_TR("-= Restar"), 2);
	var_op_opt->add_item(CG_TR("*= Multiplicar"), 3);
	var_op_opt->add_item(CG_TR("/= Dividir"), 4);
	vh2->add_child(var_op_opt);
	vh2->add_child(memnew(Label(CG_TR("Valor:"))));
	var_val_spin = memnew(SpinBox);
	var_val_spin->set_min(-999999);
	var_val_spin->set_max(999999);
	var_val_spin->set_value(0);
	vh2->add_child(var_val_spin);
	vv->add_child(vh2);
	dlg_var->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_var));
	add_child(dlg_var);

	// 5. Esperar
	dlg_wait = memnew(ConfirmationDialog);
	dlg_wait->set_title(CG_TR("Esperar"));
	dlg_wait->set_ok_button_text(CG_TR("Aceptar"));
	dlg_wait->set_cancel_button_text(CG_TR("Cancelar"));
	HBoxContainer *wh = memnew(HBoxContainer);
	wh->add_child(memnew(Label(CG_TR("Duración:"))));
	wait_frames_spin = memnew(SpinBox);
	wait_frames_spin->set_min(1);
	wait_frames_spin->set_max(999);
	wait_frames_spin->set_value(60);
	wh->add_child(wait_frames_spin);
	wh->add_child(memnew(Label(CG_TR("fotogramas (1/60 seg)"))));
	dlg_wait->add_child(wh);
	dlg_wait->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_wait));
	add_child(dlg_wait);

	// 6. Transferir / Teletransportar Jugador
	dlg_teleport = memnew(ConfirmationDialog);
	dlg_teleport->set_title(CG_TR("Transferir Jugador"));
	dlg_teleport->set_ok_button_text(CG_TR("Aceptar"));
	dlg_teleport->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *tv = memnew(VBoxContainer);
	tv->add_theme_constant_override("separation", 6);
	dlg_teleport->add_child(tv);
	HBoxContainer *th0 = memnew(HBoxContainer);
	th0->add_child(memnew(Label(CG_TR("Seleccionar Mapa:"))));
	tele_map_opt = memnew(OptionButton);
	tele_map_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tele_map_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_tele_map_opt_selected));
	th0->add_child(tele_map_opt);
	tv->add_child(th0);
	HBoxContainer *th1 = memnew(HBoxContainer);
	th1->add_child(memnew(Label(CG_TR("Ruta Mapa Destino:"))));
	tele_map_edit = memnew(LineEdit);
	tele_map_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tele_map_edit->set_placeholder("res://maps/map001.tscn");
	th1->add_child(tele_map_edit);
	tv->add_child(th1);
	HBoxContainer *th2 = memnew(HBoxContainer);
	th2->add_child(memnew(Label("X:")));
	tele_x_spin = memnew(SpinBox);
	tele_x_spin->set_max(999);
	th2->add_child(tele_x_spin);
	th2->add_child(memnew(Label("Y:")));
	tele_y_spin = memnew(SpinBox);
	tele_y_spin->set_max(999);
	th2->add_child(tele_y_spin);
	tv->add_child(th2);
	HBoxContainer *th3 = memnew(HBoxContainer);
	th3->add_child(memnew(Label(CG_TR("Dirección:"))));
	tele_dir_opt = memnew(OptionButton);
	tele_dir_opt->add_item(CG_TR("Abajo"), 0);
	tele_dir_opt->add_item(CG_TR("Izquierda"), 1);
	tele_dir_opt->add_item(CG_TR("Derecha"), 2);
	tele_dir_opt->add_item(CG_TR("Arriba"), 3);
	tele_dir_opt->add_item(CG_TR("Mantener"), 4);
	th3->add_child(tele_dir_opt);
	tv->add_child(th3);
	dlg_teleport->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_teleport));
	add_child(dlg_teleport);

	// 7. Fijar Ruta de Movimiento
	dlg_move_route = memnew(ConfirmationDialog);
	dlg_move_route->set_title(CG_TR("Fijar Ruta de Movimiento"));
	dlg_move_route->set_ok_button_text(CG_TR("Aceptar"));
	dlg_move_route->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *mrv = memnew(VBoxContainer);
	mrv->add_theme_constant_override("separation", 6);
	dlg_move_route->add_child(mrv);
	HBoxContainer *mrh1 = memnew(HBoxContainer);
	mrh1->add_child(memnew(Label(CG_TR("Objetivo:"))));
	mr_target_opt = memnew(OptionButton);
	mr_target_opt->add_item(CG_TR("Este Evento"), 0);
	mr_target_opt->add_item(CG_TR("Jugador"), 1);
	mrh1->add_child(mr_target_opt);
	mrv->add_child(mrh1);
	HBoxContainer *mrh2 = memnew(HBoxContainer);
	mrh2->add_child(memnew(Label(CG_TR("Acción:"))));
	mr_action_opt = memnew(OptionButton);
	mr_action_opt->add_item(CG_TR("Mover Abajo"), 0);
	mr_action_opt->add_item(CG_TR("Mover Izquierda"), 1);
	mr_action_opt->add_item(CG_TR("Mover Derecha"), 2);
	mr_action_opt->add_item(CG_TR("Mover Arriba"), 3);
	mr_action_opt->add_item(CG_TR("Acercarse al Jugador"), 4);
	mr_action_opt->add_item(CG_TR("Alejarse del Jugador"), 5);
	mr_action_opt->add_item(CG_TR("Saltar en el Sitio"), 6);
	mrh2->add_child(mr_action_opt);
	mrv->add_child(mrh2);
	mr_repeat_check = memnew(CheckBox(CG_TR("Repetir Movimiento")));
	mrv->add_child(mr_repeat_check);
	dlg_move_route->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_move_route));
	add_child(dlg_move_route);

	// 8. Añadir/Quitar Objeto
	dlg_item = memnew(ConfirmationDialog);
	dlg_item->set_title(CG_TR("Añadir / Quitar Objeto"));
	dlg_item->set_ok_button_text(CG_TR("Aceptar"));
	dlg_item->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *itv = memnew(VBoxContainer);
	itv->add_theme_constant_override("separation", 6);
	dlg_item->add_child(itv);
	HBoxContainer *ith1 = memnew(HBoxContainer);
	ith1->add_child(memnew(Label(CG_TR("Objeto:"))));
	item_name_edit = memnew(LineEdit);
	item_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	ith1->add_child(item_name_edit);
	Button *pick_item_btn = memnew(Button(CG_TR("Seleccionar...")));
	pick_item_btn->set_tooltip_text(CG_TR("Abrir selector de objetos, armas, armaduras y accesorios"));
	pick_item_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_open_item_picker).bind(item_name_edit));
	ith1->add_child(pick_item_btn);
	itv->add_child(ith1);
	HBoxContainer *ith2 = memnew(HBoxContainer);
	item_op_opt = memnew(OptionButton);
	item_op_opt->add_item(CG_TR("+ Incrementar"), 0);
	item_op_opt->add_item(CG_TR("- Disminuir"), 1);
	ith2->add_child(item_op_opt);
	ith2->add_child(memnew(Label(CG_TR("Cantidad:"))));
	item_qty_spin = memnew(SpinBox);
	item_qty_spin->set_min(1);
	item_qty_spin->set_max(99);
	item_qty_spin->set_value(1);
	ith2->add_child(item_qty_spin);
	itv->add_child(ith2);
	dlg_item->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_item));
	add_child(dlg_item);

	// Cambiar Dinero (Oro)
	dlg_gold = memnew(ConfirmationDialog);
	dlg_gold->set_title(CG_TR("Cambiar Dinero (Oro)"));
	dlg_gold->set_ok_button_text(CG_TR("Aceptar"));
	dlg_gold->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *gv = memnew(VBoxContainer);
	gv->add_theme_constant_override("separation", 6);
	dlg_gold->add_child(gv);
	HBoxContainer *gh = memnew(HBoxContainer);
	gh->add_child(memnew(Label(CG_TR("Operación:"))));
	gold_op_opt = memnew(OptionButton);
	gold_op_opt->add_item(CG_TR("+ Incrementar"), 0);
	gold_op_opt->add_item(CG_TR("- Disminuir"), 1);
	gh->add_child(gold_op_opt);
	gh->add_child(memnew(Label(CG_TR("Cantidad:"))));
	gold_val_spin = memnew(SpinBox);
	gold_val_spin->set_min(1);
	gold_val_spin->set_max(9999999);
	gold_val_spin->set_value(100);
	gh->add_child(gold_val_spin);
	gh->add_child(memnew(Label(CG_TR("Oro"))));
	gv->add_child(gh);
	dlg_gold->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_gold));
	add_child(dlg_gold);

	// 9. Curar/Dañar (Cambiar HP/MP)
	dlg_hp_mp = memnew(ConfirmationDialog);
	dlg_hp_mp->set_title(CG_TR("Curar / Dañar (Cambiar HP/MP)"));
	dlg_hp_mp->set_ok_button_text(CG_TR("Aceptar"));
	dlg_hp_mp->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *hpv = memnew(VBoxContainer);
	hpv->add_theme_constant_override("separation", 6);
	dlg_hp_mp->add_child(hpv);
	HBoxContainer *hph1 = memnew(HBoxContainer);
	hph1->add_child(memnew(Label(CG_TR("Atributo:"))));
	hp_mp_stat_opt = memnew(OptionButton);
	hp_mp_stat_opt->add_item(CG_TR("PV (HP)"), 0);
	hp_mp_stat_opt->add_item(CG_TR("PM (MP)"), 1);
	hph1->add_child(hp_mp_stat_opt);
	hpv->add_child(hph1);
	HBoxContainer *hph2 = memnew(HBoxContainer);
	hp_mp_op_opt = memnew(OptionButton);
	hp_mp_op_opt->add_item(CG_TR("+ Curar / Recuperar"), 0);
	hp_mp_op_opt->add_item(CG_TR("- Dañar / Reducir"), 1);
	hph2->add_child(hp_mp_op_opt);
	hp_mp_amount_spin = memnew(SpinBox);
	hp_mp_amount_spin->set_min(1);
	hp_mp_amount_spin->set_max(9999);
	hp_mp_amount_spin->set_value(50);
	hph2->add_child(hp_mp_amount_spin);
	hpv->add_child(hph2);
	dlg_hp_mp->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_hp_mp));
	add_child(dlg_hp_mp);

	// 10. Reproducir BGM/SE
	dlg_bgm = memnew(ConfirmationDialog);
	dlg_bgm->set_title(CG_TR("Reproducir BGM / SE"));
	dlg_bgm->set_ok_button_text(CG_TR("Aceptar"));
	dlg_bgm->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *bgv = memnew(VBoxContainer);
	bgv->add_theme_constant_override("separation", 6);
	dlg_bgm->add_child(bgv);
	HBoxContainer *bgh1 = memnew(HBoxContainer);
	bgh1->add_child(memnew(Label(CG_TR("Archivo de Audio:"))));
	bgm_file_edit = memnew(LineEdit);
	bgm_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgm_file_edit->set_placeholder("res://audio/bgm/tema.ogg");
	bgh1->add_child(bgm_file_edit);
	bgm_file_browse_btn = memnew(Button(CG_TR("...")));
	bgm_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo de Audio (audio/bgm, audio/se)"));
	bgm_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_bgm_pressed));
	bgh1->add_child(bgm_file_browse_btn);
	bgv->add_child(bgh1);
	HBoxContainer *bgh2 = memnew(HBoxContainer);
	bgh2->add_child(memnew(Label(CG_TR("Volumen (%):"))));
	bgm_volume_spin = memnew(SpinBox);
	bgm_volume_spin->set_min(0);
	bgm_volume_spin->set_max(100);
	bgm_volume_spin->set_value(90);
	bgh2->add_child(bgm_volume_spin);
	bgh2->add_child(memnew(Label(CG_TR("Tono (%):"))));
	bgm_pitch_spin = memnew(SpinBox);
	bgm_pitch_spin->set_min(50);
	bgm_pitch_spin->set_max(150);
	bgm_pitch_spin->set_value(100);
	bgh2->add_child(bgm_pitch_spin);
	bgv->add_child(bgh2);
	dlg_bgm->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_bgm));
	add_child(dlg_bgm);

	bgm_file_dialog = memnew(FileDialog);
	bgm_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	bgm_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	bgm_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Archivos de Audio (*.ogg, *.wav, *.mp3)");
	bgm_file_dialog->set_title(CG_TR("Seleccionar Archivo de Audio"));
	bgm_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_bgm_file_selected));
	add_child(bgm_file_dialog);

	// Reproducir SE (Efecto de Sonido)
	dlg_se = memnew(ConfirmationDialog);
	dlg_se->set_title(CG_TR("Reproducir Efecto de Sonido (SE)"));
	dlg_se->set_ok_button_text(CG_TR("Aceptar"));
	dlg_se->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *sev = memnew(VBoxContainer);
	sev->add_theme_constant_override("separation", 6);
	dlg_se->add_child(sev);
	HBoxContainer *seh1 = memnew(HBoxContainer);
	seh1->add_child(memnew(Label(CG_TR("Archivo SE:"))));
	se_file_edit = memnew(LineEdit);
	se_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	se_file_edit->set_placeholder("res://audio/se/sonido.ogg");
	seh1->add_child(se_file_edit);
	se_file_browse_btn = memnew(Button(CG_TR("...")));
	se_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Efecto de Sonido (audio/se)"));
	se_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_se_pressed));
	seh1->add_child(se_file_browse_btn);
	sev->add_child(seh1);
	HBoxContainer *seh2 = memnew(HBoxContainer);
	seh2->add_child(memnew(Label(CG_TR("Volumen (%):"))));
	se_volume_spin = memnew(SpinBox);
	se_volume_spin->set_min(0);
	se_volume_spin->set_max(100);
	se_volume_spin->set_value(90);
	seh2->add_child(se_volume_spin);
	seh2->add_child(memnew(Label(CG_TR("Tono (%):"))));
	se_pitch_spin = memnew(SpinBox);
	se_pitch_spin->set_min(50);
	se_pitch_spin->set_max(150);
	se_pitch_spin->set_value(100);
	seh2->add_child(se_pitch_spin);
	sev->add_child(seh2);
	dlg_se->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_se));
	add_child(dlg_se);

	se_file_dialog = memnew(FileDialog);
	se_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	se_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	se_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Archivos de Audio (*.ogg, *.wav, *.mp3)");
	se_file_dialog->set_title(CG_TR("Seleccionar Efecto de Sonido (audio/se)"));
	se_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_se_file_selected));
	add_child(se_file_dialog);

	// 11. Efecto de Flash
	dlg_flash = memnew(ConfirmationDialog);
	dlg_flash->set_title(CG_TR("Efecto de Flash"));
	dlg_flash->set_ok_button_text(CG_TR("Aceptar"));
	dlg_flash->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *flv = memnew(VBoxContainer);
	flv->add_theme_constant_override("separation", 6);
	dlg_flash->add_child(flv);
	HBoxContainer *flh1 = memnew(HBoxContainer);
	flh1->add_child(memnew(Label(CG_TR("Color del Destello:"))));
	flash_color_opt = memnew(OptionButton);
	flash_color_opt->add_item(CG_TR("Blanco (Relámpago)"), 0);
	flash_color_opt->add_item(CG_TR("Rojo (Daño)"), 1);
	flash_color_opt->add_item(CG_TR("Dorado (Magia)"), 2);
	flh1->add_child(flash_color_opt);
	flv->add_child(flh1);
	HBoxContainer *flh2 = memnew(HBoxContainer);
	flh2->add_child(memnew(Label(CG_TR("Duración (frames):"))));
	flash_duration_spin = memnew(SpinBox);
	flash_duration_spin->set_min(1);
	flash_duration_spin->set_max(600);
	flash_duration_spin->set_value(30);
	flh2->add_child(flash_duration_spin);
	flv->add_child(flh2);
	dlg_flash->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_flash));
	add_child(dlg_flash);

	// 12. Teñir Pantalla
	dlg_tint = memnew(ConfirmationDialog);
	dlg_tint->set_title(CG_TR("Teñir Pantalla"));
	dlg_tint->set_ok_button_text(CG_TR("Aceptar"));
	dlg_tint->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *tiv = memnew(VBoxContainer);
	tiv->add_theme_constant_override("separation", 6);
	dlg_tint->add_child(tiv);
	HBoxContainer *tih1 = memnew(HBoxContainer);
	tih1->add_child(memnew(Label(CG_TR("Tono Predefinido:"))));
	tint_preset_opt = memnew(OptionButton);
	tint_preset_opt->add_item(CG_TR("Normal"), 0);
	tint_preset_opt->add_item(CG_TR("Oscuro / Noche"), 1);
	tint_preset_opt->add_item(CG_TR("Sepia / Recuerdo"), 2);
	tint_preset_opt->add_item(CG_TR("Atardecer"), 3);
	tih1->add_child(tint_preset_opt);
	tiv->add_child(tih1);
	HBoxContainer *tih2 = memnew(HBoxContainer);
	tih2->add_child(memnew(Label(CG_TR("Duración (frames):"))));
	tint_duration_spin = memnew(SpinBox);
	tint_duration_spin->set_min(1);
	tint_duration_spin->set_max(600);
	tint_duration_spin->set_value(60);
	tih2->add_child(tint_duration_spin);
	tiv->add_child(tih2);
	dlg_tint->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_tint));
	add_child(dlg_tint);

	// Fundido de Pantalla
	dlg_fade = memnew(ConfirmationDialog);
	dlg_fade->set_title(CG_TR("Fundido de Pantalla"));
	dlg_fade->set_ok_button_text(CG_TR("Aceptar"));
	dlg_fade->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *fdv = memnew(VBoxContainer);
	fdv->add_theme_constant_override("separation", 6);
	dlg_fade->add_child(fdv);
	HBoxContainer *fdh1 = memnew(HBoxContainer);
	fdh1->add_child(memnew(Label(CG_TR("Tipo de Fundido:"))));
	fade_type_opt = memnew(OptionButton);
	fade_type_opt->add_item(CG_TR("Fundido a Negro (Oscurecer)"), 0);
	fade_type_opt->add_item(CG_TR("Aparecer desde Negro (Aclarar)"), 1);
	fdh1->add_child(fade_type_opt);
	fdv->add_child(fdh1);
	HBoxContainer *fdh2 = memnew(HBoxContainer);
	fdh2->add_child(memnew(Label(CG_TR("Duración (fotogramas):"))));
	fade_duration_spin = memnew(SpinBox);
	fade_duration_spin->set_min(1);
	fade_duration_spin->set_max(300);
	fade_duration_spin->set_value(30);
	fdh2->add_child(fade_duration_spin);
	fdv->add_child(fdh2);
	dlg_fade->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_fade));
	add_child(dlg_fade);

	// 13. Iniciar Batalla (CMD_BATTLE_PROCESSING / CMD_BOSS_BATTLE)
	dlg_battle = memnew(ConfirmationDialog);
	dlg_battle->set_title(CG_TR("Iniciar Batalla"));
	dlg_battle->set_ok_button_text(CG_TR("Aceptar"));
	dlg_battle->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *batv = memnew(VBoxContainer);
	batv->add_theme_constant_override("separation", 6);
	dlg_battle->add_child(batv);

	HBoxContainer *bath_opt = memnew(HBoxContainer);
	bath_opt->add_child(memnew(Label(CG_TR("Tropa de la BD:"))));
	battle_troop_opt = memnew(OptionButton);
	battle_troop_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bath_opt->add_child(battle_troop_opt);
	batv->add_child(bath_opt);

	HBoxContainer *bath = memnew(HBoxContainer);
	bath->add_child(memnew(Label(CG_TR("O Nombre / ID:"))));
	battle_troop_edit = memnew(LineEdit);
	battle_troop_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bath->add_child(battle_troop_edit);
	batv->add_child(bath);

	HBoxContainer *bath_bgm = memnew(HBoxContainer);
	bath_bgm->add_child(memnew(Label(CG_TR("Música de Batalla:"))));
	battle_bgm_edit = memnew(LineEdit);
	battle_bgm_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bath_bgm->add_child(battle_bgm_edit);
	battle_bgm_browse_btn = memnew(Button(CG_TR("Buscar...")));
	battle_bgm_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_battle_bgm_browse_pressed));
	bath_bgm->add_child(battle_bgm_browse_btn);
	batv->add_child(bath_bgm);

	battle_can_escape_check = memnew(CheckBox(CG_TR("Permitir Huida")));
	battle_can_escape_check->set_pressed(true);
	batv->add_child(battle_can_escape_check);
	battle_can_lose_check = memnew(CheckBox(CG_TR("Continuar si Pierde (La derrota no provoca Game Over)")));
	batv->add_child(battle_can_lose_check);

	battle_bgm_file_dialog = memnew(FileDialog);
	battle_bgm_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	battle_bgm_file_dialog->set_access(FileDialog::ACCESS_RESOURCES);
	battle_bgm_file_dialog->add_filter("*.ogg, *.mp3, *.wav ; " + CG_TR("Archivos de Audio"));
	battle_bgm_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_battle_bgm_selected));
	add_child(battle_bgm_file_dialog);

	dlg_battle->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_battle));
	add_child(dlg_battle);

	// 14. Mostrar Imagen (CMD_SHOW_PICTURE)
	dlg_show_picture = memnew(ConfirmationDialog);
	dlg_show_picture->set_title(CG_TR("Mostrar Imagen"));
	dlg_show_picture->set_ok_button_text(CG_TR("Aceptar"));
	dlg_show_picture->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *spv = memnew(VBoxContainer);
	spv->add_theme_constant_override("separation", 6);
	dlg_show_picture->add_child(spv);

	// Fila superior visible: Número de Imagen y Archivo
	HBoxContainer *sph1 = memnew(HBoxContainer);
	sph1->add_child(memnew(Label(CG_TR("Número de Imagen (1-100):"))));
	pic_show_id_spin = memnew(SpinBox);
	pic_show_id_spin->set_min(1);
	pic_show_id_spin->set_max(100);
	pic_show_id_spin->set_value(1);
	sph1->add_child(pic_show_id_spin);

	sph1->add_child(memnew(Label(CG_TR("Archivo:"))));
	pic_show_path_edit = memnew(LineEdit);
	pic_show_path_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pic_show_path_edit->set_placeholder("res://pictures/imagen.png");
	pic_show_path_edit->connect("text_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_show_path_changed));
	sph1->add_child(pic_show_path_edit);

	pic_show_browse_btn = memnew(Button(CG_TR("...")));
	pic_show_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo de Imagen"));
	pic_show_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_pic_pressed));
	sph1->add_child(pic_show_browse_btn);
	spv->add_child(sph1);

	pic_show_file_dialog = memnew(FileDialog);
	pic_show_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	pic_show_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	pic_show_file_dialog->add_filter("*.png, *.jpg, *.webp ; Archivos de Imagen (*.png, *.jpg, *.webp)");
	pic_show_file_dialog->set_title(CG_TR("Seleccionar Archivo de Imagen"));
	pic_show_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_file_selected));
	add_child(pic_show_file_dialog);

	// Vista previa interactiva con arrastre visible
	pic_show_picker = memnew(ClockGearsPictureVisualPicker);
	spv->add_child(pic_show_picker);

	// Duración, Fundido y Esperar visibles
	HBoxContainer *sph_dur = memnew(HBoxContainer);
	sph_dur->add_child(memnew(Label(CG_TR("Duración:"))));
	pic_show_dur_mode_opt = memnew(OptionButton);
	pic_show_dur_mode_opt->add_item(CG_TR("Hasta que se borre"), 0);
	pic_show_dur_mode_opt->add_item(CG_TR("X segundos"), 1);
	pic_show_dur_mode_opt->add_item(CG_TR("Hasta que termine el siguiente mensaje"), 2);
	pic_show_dur_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_show_dur_mode_selected));
	sph_dur->add_child(pic_show_dur_mode_opt);

	pic_show_dur_sec_spin = memnew(SpinBox);
	pic_show_dur_sec_spin->set_min(0.1);
	pic_show_dur_sec_spin->set_max(999.0);
	pic_show_dur_sec_spin->set_step(0.1);
	pic_show_dur_sec_spin->set_value(3.0);
	pic_show_dur_sec_spin->set_prefix(" ");
	pic_show_dur_sec_spin->set_suffix(" s");
	sph_dur->add_child(pic_show_dur_sec_spin);

	pic_show_fadeout_check = memnew(CheckBox);
	pic_show_fadeout_check->set_text(CG_TR("Fundido de salida"));
	sph_dur->add_child(pic_show_fadeout_check);

	pic_show_wait_check = memnew(CheckBox);
	pic_show_wait_check->set_text(CG_TR("Esperar"));
	sph_dur->add_child(pic_show_wait_check);
	spv->add_child(sph_dur);
	_on_pic_show_dur_mode_selected(0);

	// Opacidad visible
	HBoxContainer *sph_op = memnew(HBoxContainer);
	sph_op->add_child(memnew(Label(CG_TR("Opacidad (%):"))));
	pic_show_opacity_spin = memnew(SpinBox);
	pic_show_opacity_spin->set_min(0);
	pic_show_opacity_spin->set_max(100);
	pic_show_opacity_spin->set_value(100);
	sph_op->add_child(pic_show_opacity_spin);
	spv->add_child(sph_op);

	// Sección "Avanzado" plegada
	pic_show_adv_btn = memnew(Button(CG_TR("▶ Avanzado (Origen, Mezcla, Coordenadas y Escala)")));
	pic_show_adv_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pic_show_adv_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_show_adv_toggle));
	spv->add_child(pic_show_adv_btn);

	pic_show_adv_box = memnew(VBoxContainer);
	pic_show_adv_box->add_theme_constant_override("separation", 6);
	pic_show_adv_box->set_visible(false);
	spv->add_child(pic_show_adv_box);

	HBoxContainer *sph_adv1 = memnew(HBoxContainer);
	sph_adv1->add_child(memnew(Label(CG_TR("Origen:"))));
	pic_show_origin_opt = memnew(OptionButton);
	pic_show_origin_opt->add_item(CG_TR("Superior Izquierda"), 0);
	pic_show_origin_opt->add_item(CG_TR("Centro"), 1);
	pic_show_origin_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_show_origin_selected));
	sph_adv1->add_child(pic_show_origin_opt);

	sph_adv1->add_child(memnew(Label(CG_TR("Mezcla:"))));
	pic_show_blend_opt = memnew(OptionButton);
	pic_show_blend_opt->add_item(CG_TR("Normal"), 0);
	pic_show_blend_opt->add_item(CG_TR("Añadir"), 1);
	pic_show_blend_opt->add_item(CG_TR("Multiplicar"), 2);
	pic_show_blend_opt->add_item(CG_TR("Pantalla"), 3);
	sph_adv1->add_child(pic_show_blend_opt);
	pic_show_adv_box->add_child(sph_adv1);

	HBoxContainer *sph_adv2 = memnew(HBoxContainer);
	sph_adv2->add_child(memnew(Label(CG_TR("Posición X:"))));
	pic_show_x_spin = memnew(SpinBox);
	pic_show_x_spin->set_min(-9999);
	pic_show_x_spin->set_max(9999);
	pic_show_x_spin->set_value(0);
	sph_adv2->add_child(pic_show_x_spin);
	sph_adv2->add_child(memnew(Label(CG_TR("Y:"))));
	pic_show_y_spin = memnew(SpinBox);
	pic_show_y_spin->set_min(-9999);
	pic_show_y_spin->set_max(9999);
	pic_show_y_spin->set_value(0);
	sph_adv2->add_child(pic_show_y_spin);

	sph_adv2->add_child(memnew(Label(CG_TR("Escala X (%):"))));
	pic_show_scale_x_spin = memnew(SpinBox);
	pic_show_scale_x_spin->set_min(0);
	pic_show_scale_x_spin->set_max(2000);
	pic_show_scale_x_spin->set_value(100);
	sph_adv2->add_child(pic_show_scale_x_spin);
	sph_adv2->add_child(memnew(Label(CG_TR("Y (%):"))));
	pic_show_scale_y_spin = memnew(SpinBox);
	pic_show_scale_y_spin->set_min(0);
	pic_show_scale_y_spin->set_max(2000);
	pic_show_scale_y_spin->set_value(100);
	sph_adv2->add_child(pic_show_scale_y_spin);
	pic_show_adv_box->add_child(sph_adv2);

	pic_show_picker->bind_spinboxes(pic_show_x_spin, pic_show_y_spin, pic_show_scale_x_spin, pic_show_scale_y_spin);

	dlg_show_picture->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_show_picture));
	add_child(dlg_show_picture);

	// 15. Mover Imagen (CMD_MOVE_PICTURE)
	dlg_move_picture = memnew(ConfirmationDialog);
	dlg_move_picture->set_title(CG_TR("Mover Imagen"));
	dlg_move_picture->set_ok_button_text(CG_TR("Aceptar"));
	dlg_move_picture->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *mpv = memnew(VBoxContainer);
	mpv->add_theme_constant_override("separation", 6);
	dlg_move_picture->add_child(mpv);

	HBoxContainer *mph1 = memnew(HBoxContainer);
	mph1->add_child(memnew(Label(CG_TR("Número de Imagen (1-100):"))));
	pic_move_id_spin = memnew(SpinBox);
	pic_move_id_spin->set_min(1);
	pic_move_id_spin->set_max(100);
	pic_move_id_spin->set_value(1);
	mph1->add_child(pic_move_id_spin);
	mpv->add_child(mph1);

	// Selector Visual Interactivo (Item 3 - Destino) visible
	pic_move_picker = memnew(ClockGearsPictureVisualPicker);
	mpv->add_child(pic_move_picker);

	HBoxContainer *mph_op = memnew(HBoxContainer);
	mph_op->add_child(memnew(Label(CG_TR("Opacidad (%):"))));
	pic_move_opacity_spin = memnew(SpinBox);
	pic_move_opacity_spin->set_min(0);
	pic_move_opacity_spin->set_max(100);
	pic_move_opacity_spin->set_value(100);
	mph_op->add_child(pic_move_opacity_spin);

	mph_op->add_child(memnew(Label(CG_TR("Duración (fotogramas):"))));
	pic_move_duration_spin = memnew(SpinBox);
	pic_move_duration_spin->set_min(1);
	pic_move_duration_spin->set_max(999);
	pic_move_duration_spin->set_value(60);
	mph_op->add_child(pic_move_duration_spin);

	mph_op->add_child(memnew(Label(CG_TR("Easing:"))));
	pic_move_easing_opt = memnew(OptionButton);
	pic_move_easing_opt->add_item(CG_TR("Lineal"), 0);
	pic_move_easing_opt->add_item(CG_TR("Entrada (Ease In)"), 1);
	pic_move_easing_opt->add_item(CG_TR("Salida (Ease Out)"), 2);
	pic_move_easing_opt->add_item(CG_TR("Entrada/Salida (Ease In Out)"), 3);
	mph_op->add_child(pic_move_easing_opt);
	mpv->add_child(mph_op);

	pic_move_wait_check = memnew(CheckBox(CG_TR("Esperar que termine el movimiento")));
	pic_move_wait_check->set_pressed(false);
	mpv->add_child(pic_move_wait_check);

	// Sección "Avanzado" plegada
	pic_move_adv_btn = memnew(Button(CG_TR("▶ Avanzado (Origen, Coordenadas y Escala de Destino)")));
	pic_move_adv_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pic_move_adv_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_move_adv_toggle));
	mpv->add_child(pic_move_adv_btn);

	pic_move_adv_box = memnew(VBoxContainer);
	pic_move_adv_box->add_theme_constant_override("separation", 6);
	pic_move_adv_box->set_visible(false);
	mpv->add_child(pic_move_adv_box);

	HBoxContainer *mph_adv1 = memnew(HBoxContainer);
	mph_adv1->add_child(memnew(Label(CG_TR("Origen:"))));
	pic_move_origin_opt = memnew(OptionButton);
	pic_move_origin_opt->add_item(CG_TR("Superior Izquierda"), 0);
	pic_move_origin_opt->add_item(CG_TR("Centro"), 1);
	pic_move_origin_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_pic_move_origin_selected));
	mph_adv1->add_child(pic_move_origin_opt);
	pic_move_adv_box->add_child(mph_adv1);

	HBoxContainer *mph_adv2 = memnew(HBoxContainer);
	mph_adv2->add_child(memnew(Label(CG_TR("Destino X:"))));
	pic_move_x_spin = memnew(SpinBox);
	pic_move_x_spin->set_min(-9999);
	pic_move_x_spin->set_max(9999);
	pic_move_x_spin->set_value(0);
	mph_adv2->add_child(pic_move_x_spin);
	mph_adv2->add_child(memnew(Label(CG_TR("Y:"))));
	pic_move_y_spin = memnew(SpinBox);
	pic_move_y_spin->set_min(-9999);
	pic_move_y_spin->set_max(9999);
	pic_move_y_spin->set_value(0);
	mph_adv2->add_child(pic_move_y_spin);

	mph_adv2->add_child(memnew(Label(CG_TR("Escala X (%):"))));
	pic_move_scale_x_spin = memnew(SpinBox);
	pic_move_scale_x_spin->set_min(0);
	pic_move_scale_x_spin->set_max(2000);
	pic_move_scale_x_spin->set_value(100);
	mph_adv2->add_child(pic_move_scale_x_spin);
	mph_adv2->add_child(memnew(Label(CG_TR("Y (%):"))));
	pic_move_scale_y_spin = memnew(SpinBox);
	pic_move_scale_y_spin->set_min(0);
	pic_move_scale_y_spin->set_max(2000);
	pic_move_scale_y_spin->set_value(100);
	mph_adv2->add_child(pic_move_scale_y_spin);
	pic_move_adv_box->add_child(mph_adv2);

	pic_move_picker->bind_spinboxes(pic_move_x_spin, pic_move_y_spin, pic_move_scale_x_spin, pic_move_scale_y_spin);

	dlg_move_picture->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_move_picture));
	add_child(dlg_move_picture);

	// 16. Rotar Imagen (CMD_ROTATE_PICTURE)
	dlg_rotate_picture = memnew(ConfirmationDialog);
	dlg_rotate_picture->set_title(CG_TR("Rotar Imagen"));
	dlg_rotate_picture->set_ok_button_text(CG_TR("Aceptar"));
	dlg_rotate_picture->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *rpv = memnew(VBoxContainer);
	rpv->add_theme_constant_override("separation", 6);
	dlg_rotate_picture->add_child(rpv);

	HBoxContainer *rph1 = memnew(HBoxContainer);
	rph1->add_child(memnew(Label(CG_TR("Número de Imagen (1-100):"))));
	pic_rot_id_spin = memnew(SpinBox);
	pic_rot_id_spin->set_min(1);
	pic_rot_id_spin->set_max(100);
	pic_rot_id_spin->set_value(1);
	rph1->add_child(pic_rot_id_spin);
	rpv->add_child(rph1);

	HBoxContainer *rph2 = memnew(HBoxContainer);
	rph2->add_child(memnew(Label(CG_TR("Velocidad (°/s, -360 a 360, 0=parar):"))));
	pic_rot_speed_spin = memnew(SpinBox);
	pic_rot_speed_spin->set_min(-360);
	pic_rot_speed_spin->set_max(360);
	pic_rot_speed_spin->set_value(90);
	rph2->add_child(pic_rot_speed_spin);
	rpv->add_child(rph2);

	dlg_rotate_picture->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_rotate_picture));
	add_child(dlg_rotate_picture);

	// 17. Teñir Imagen (CMD_TINT_PICTURE)
	dlg_tint_picture = memnew(ConfirmationDialog);
	dlg_tint_picture->set_title(CG_TR("Teñir Imagen"));
	dlg_tint_picture->set_ok_button_text(CG_TR("Aceptar"));
	dlg_tint_picture->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *tpv = memnew(VBoxContainer);
	tpv->add_theme_constant_override("separation", 6);
	dlg_tint_picture->add_child(tpv);

	HBoxContainer *tph1 = memnew(HBoxContainer);
	tph1->add_child(memnew(Label(CG_TR("Número de Imagen (1-100):"))));
	pic_tint_id_spin = memnew(SpinBox);
	pic_tint_id_spin->set_min(1);
	pic_tint_id_spin->set_max(100);
	pic_tint_id_spin->set_value(1);
	tph1->add_child(pic_tint_id_spin);
	tpv->add_child(tph1);

	HBoxContainer *tph2 = memnew(HBoxContainer);
	tph2->add_child(memnew(Label(CG_TR("Tono / Preset:"))));
	pic_tint_preset_opt = memnew(OptionButton);
	pic_tint_preset_opt->add_item(CG_TR("Normal (Sin tinte)"), 0);
	pic_tint_preset_opt->add_item(CG_TR("Oscuro / Noche"), 1);
	pic_tint_preset_opt->add_item(CG_TR("Atardecer"), 2);
	pic_tint_preset_opt->add_item(CG_TR("Sepia"), 3);
	tph2->add_child(pic_tint_preset_opt);
	tph2->add_child(memnew(Label(CG_TR("Duración (frames):"))));
	pic_tint_duration_spin = memnew(SpinBox);
	pic_tint_duration_spin->set_min(1);
	pic_tint_duration_spin->set_max(999);
	pic_tint_duration_spin->set_value(60);
	tph2->add_child(pic_tint_duration_spin);
	tpv->add_child(tph2);

	pic_tint_wait_check = memnew(CheckBox(CG_TR("Esperar que termine el tinte")));
	pic_tint_wait_check->set_pressed(false);
	tpv->add_child(pic_tint_wait_check);

	dlg_tint_picture->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_tint_picture));
	add_child(dlg_tint_picture);

	// 18. Borrar Imagen (CMD_ERASE_PICTURE)
	dlg_erase_picture = memnew(ConfirmationDialog);
	dlg_erase_picture->set_title(CG_TR("Borrar Imagen"));
	dlg_erase_picture->set_ok_button_text(CG_TR("Aceptar"));
	dlg_erase_picture->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *epv = memnew(VBoxContainer);
	epv->add_theme_constant_override("separation", 6);
	dlg_erase_picture->add_child(epv);

	HBoxContainer *eph1 = memnew(HBoxContainer);
	eph1->add_child(memnew(Label(CG_TR("Número de Imagen a Borrar (1-100):"))));
	pic_erase_id_spin = memnew(SpinBox);
	pic_erase_id_spin->set_min(1);
	pic_erase_id_spin->set_max(100);
	pic_erase_id_spin->set_value(1);
	eph1->add_child(pic_erase_id_spin);
	epv->add_child(eph1);

	dlg_erase_picture->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_erase_picture));
	add_child(dlg_erase_picture);

	// 19. Temblor de Pantalla (CMD_SHAKE_SCREEN)
	dlg_shake = memnew(ConfirmationDialog);
	dlg_shake->set_title(CG_TR("Temblor de Pantalla"));
	dlg_shake->set_ok_button_text(CG_TR("Aceptar"));
	dlg_shake->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *shkv = memnew(VBoxContainer);
	shkv->add_theme_constant_override("separation", 6);
	dlg_shake->add_child(shkv);

	HBoxContainer *shkh1 = memnew(HBoxContainer);
	shkh1->add_child(memnew(Label(CG_TR("Potencia (1-10):"))));
	shake_power_spin = memnew(SpinBox);
	shake_power_spin->set_min(1);
	shake_power_spin->set_max(10);
	shake_power_spin->set_value(5);
	shkh1->add_child(shake_power_spin);
	shkh1->add_child(memnew(Label(CG_TR("Velocidad (1-10):"))));
	shake_speed_spin = memnew(SpinBox);
	shake_speed_spin->set_min(1);
	shake_speed_spin->set_max(10);
	shake_speed_spin->set_value(5);
	shkh1->add_child(shake_speed_spin);
	shkv->add_child(shkh1);

	HBoxContainer *shkh2 = memnew(HBoxContainer);
	shkh2->add_child(memnew(Label(CG_TR("Duración (fotogramas):"))));
	shake_duration_spin = memnew(SpinBox);
	shake_duration_spin->set_min(1);
	shake_duration_spin->set_max(600);
	shake_duration_spin->set_value(60);
	shkh2->add_child(shake_duration_spin);
	shkv->add_child(shkh2);

	shake_wait_check = memnew(CheckBox(CG_TR("Esperar que termine el temblor")));
	shake_wait_check->set_pressed(false);
	shkv->add_child(shake_wait_check);

	dlg_shake->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_shake));
	add_child(dlg_shake);

	// Cambiar Zoom de Cámara (CMD_CHANGE_CAMERA_ZOOM)
	dlg_change_camera_zoom = memnew(ConfirmationDialog);
	dlg_change_camera_zoom->set_title(CG_TR("Cambiar Zoom de Cámara"));
	dlg_change_camera_zoom->set_ok_button_text(CG_TR("Aceptar"));
	dlg_change_camera_zoom->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *czv = memnew(VBoxContainer);
	czv->add_theme_constant_override("separation", 8);
	dlg_change_camera_zoom->add_child(czv);

	HBoxContainer *czh1 = memnew(HBoxContainer);
	czh1->add_theme_constant_override("separation", 6);
	czh1->add_child(memnew(Label(CG_TR("Zoom Objetivo:"))));
	cam_zoom_spin = memnew(SpinBox);
	cam_zoom_spin->set_min(0.25);
	cam_zoom_spin->set_max(4.0);
	cam_zoom_spin->set_step(0.1);
	cam_zoom_spin->set_value(2.0);
	czh1->add_child(cam_zoom_spin);

	cam_zoom_opt = memnew(OptionButton);
	cam_zoom_opt->add_item(CG_TR("Presets..."), 0);
	cam_zoom_opt->add_item(CG_TR("1.0x (Normal)"), 10);
	cam_zoom_opt->add_item(CG_TR("1.5x"), 15);
	cam_zoom_opt->add_item(CG_TR("2.0x (Pixel Perfect)"), 20);
	cam_zoom_opt->add_item(CG_TR("2.5x"), 25);
	cam_zoom_opt->add_item(CG_TR("3.0x (Primer Plano)"), 30);
	cam_zoom_opt->add_item(CG_TR("4.0x (Macro)"), 40);
	cam_zoom_opt->select(3); // 2.0x por defecto
	cam_zoom_opt->connect("item_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_cam_zoom_preset_selected));
	czh1->add_child(cam_zoom_opt);
	czv->add_child(czh1);

	HBoxContainer *czh2 = memnew(HBoxContainer);
	czh2->add_child(memnew(Label(CG_TR("Duración de Transición (fotogramas, 0 = Inmediato):"))));
	cam_zoom_duration_spin = memnew(SpinBox);
	cam_zoom_duration_spin->set_min(0);
	cam_zoom_duration_spin->set_max(999);
	cam_zoom_duration_spin->set_value(0);
	czh2->add_child(cam_zoom_duration_spin);
	czv->add_child(czh2);

	cam_zoom_wait_check = memnew(CheckBox(CG_TR("Esperar que termine la transición de zoom")));
	cam_zoom_wait_check->set_pressed(false);
	czv->add_child(cam_zoom_wait_check);

	dlg_change_camera_zoom->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_change_camera_zoom));
	add_child(dlg_change_camera_zoom);

	// 20. Clima (CMD_WEATHER)
	dlg_weather = memnew(ConfirmationDialog);
	dlg_weather->set_title(CG_TR("Fijar Efecto de Clima"));
	dlg_weather->set_ok_button_text(CG_TR("Aceptar"));
	dlg_weather->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *wev = memnew(VBoxContainer);
	wev->add_theme_constant_override("separation", 6);
	dlg_weather->add_child(wev);

	HBoxContainer *weh1 = memnew(HBoxContainer);
	weh1->add_child(memnew(Label(CG_TR("Tipo de Clima:"))));
	weather_type_opt = memnew(OptionButton);
	weather_type_opt->add_item(CG_TR("Ninguno"), 0);
	weather_type_opt->add_item(CG_TR("Lluvia"), 1);
	weather_type_opt->add_item(CG_TR("Tormenta"), 2);
	weather_type_opt->add_item(CG_TR("Nieve"), 3);
	weh1->add_child(weather_type_opt);
	wev->add_child(weh1);

	HBoxContainer *weh2 = memnew(HBoxContainer);
	weh2->add_child(memnew(Label(CG_TR("Potencia (1-10):"))));
	weather_power_spin = memnew(SpinBox);
	weather_power_spin->set_min(1);
	weather_power_spin->set_max(10);
	weather_power_spin->set_value(5);
	weh2->add_child(weather_power_spin);
	weh2->add_child(memnew(Label(CG_TR("Duración (fotogramas):"))));
	weather_duration_spin = memnew(SpinBox);
	weather_duration_spin->set_min(1);
	weather_duration_spin->set_max(600);
	weather_duration_spin->set_value(60);
	weh2->add_child(weather_duration_spin);
	wev->add_child(weh2);

	dlg_weather->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_weather));
	add_child(dlg_weather);

	// 21. BGS (CMD_PLAY_BGS)
	dlg_bgs = memnew(ConfirmationDialog);
	dlg_bgs->set_title(CG_TR("Reproducir BGS (Sonido Ambiental)"));
	dlg_bgs->set_ok_button_text(CG_TR("Aceptar"));
	dlg_bgs->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *bgsv = memnew(VBoxContainer);
	bgsv->add_theme_constant_override("separation", 6);
	dlg_bgs->add_child(bgsv);

	HBoxContainer *bgsh1 = memnew(HBoxContainer);
	bgsh1->add_child(memnew(Label(CG_TR("Archivo BGS:"))));
	bgs_file_edit = memnew(LineEdit);
	bgs_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bgs_file_edit->set_placeholder("res://audio/bgs/viento.ogg");
	bgsh1->add_child(bgs_file_edit);
	bgs_file_browse_btn = memnew(Button(CG_TR("...")));
	bgs_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo BGS"));
	bgs_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_bgs_pressed));
	bgsh1->add_child(bgs_file_browse_btn);
	bgsv->add_child(bgsh1);

	bgs_file_dialog = memnew(FileDialog);
	bgs_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	bgs_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	bgs_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Archivos de Audio (*.ogg, *.wav, *.mp3)");
	bgs_file_dialog->set_title(CG_TR("Seleccionar Archivo BGS"));
	bgs_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_bgs_file_selected));
	add_child(bgs_file_dialog);

	HBoxContainer *bgsh2 = memnew(HBoxContainer);
	bgsh2->add_child(memnew(Label(CG_TR("Volumen (%):"))));
	bgs_volume_spin = memnew(SpinBox);
	bgs_volume_spin->set_min(0);
	bgs_volume_spin->set_max(100);
	bgs_volume_spin->set_value(90);
	bgsh2->add_child(bgs_volume_spin);
	bgsh2->add_child(memnew(Label(CG_TR("Tono (%):"))));
	bgs_pitch_spin = memnew(SpinBox);
	bgs_pitch_spin->set_min(50);
	bgs_pitch_spin->set_max(150);
	bgs_pitch_spin->set_value(100);
	bgsh2->add_child(bgs_pitch_spin);
	bgsv->add_child(bgsh2);

	dlg_bgs->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_bgs));
	add_child(dlg_bgs);

	// 22. ME (CMD_PLAY_ME)
	dlg_me = memnew(ConfirmationDialog);
	dlg_me->set_title(CG_TR("Reproducir ME (Efecto Musical)"));
	dlg_me->set_ok_button_text(CG_TR("Aceptar"));
	dlg_me->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *mev = memnew(VBoxContainer);
	mev->add_theme_constant_override("separation", 6);
	dlg_me->add_child(mev);

	HBoxContainer *meh1 = memnew(HBoxContainer);
	meh1->add_child(memnew(Label(CG_TR("Archivo ME:"))));
	me_file_edit = memnew(LineEdit);
	me_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	me_file_edit->set_placeholder("res://audio/me/victoria.ogg");
	meh1->add_child(me_file_edit);
	me_file_browse_btn = memnew(Button(CG_TR("...")));
	me_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo ME"));
	me_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_me_pressed));
	meh1->add_child(me_file_browse_btn);
	mev->add_child(meh1);

	me_file_dialog = memnew(FileDialog);
	me_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	me_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	me_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Archivos de Audio (*.ogg, *.wav, *.mp3)");
	me_file_dialog->set_title(CG_TR("Seleccionar Archivo ME"));
	me_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_me_file_selected));
	add_child(me_file_dialog);

	HBoxContainer *meh2 = memnew(HBoxContainer);
	meh2->add_child(memnew(Label(CG_TR("Volumen (%):"))));
	me_volume_spin = memnew(SpinBox);
	me_volume_spin->set_min(0);
	me_volume_spin->set_max(100);
	me_volume_spin->set_value(90);
	meh2->add_child(me_volume_spin);
	meh2->add_child(memnew(Label(CG_TR("Tono (%):"))));
	me_pitch_spin = memnew(SpinBox);
	me_pitch_spin->set_min(50);
	me_pitch_spin->set_max(150);
	me_pitch_spin->set_value(100);
	meh2->add_child(me_pitch_spin);
	mev->add_child(meh2);

	dlg_me->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_me));
	add_child(dlg_me);

	// 23. Parar/Fundir BGM (CMD_STOP_FADE_BGM)
	dlg_stop_fade_bgm = memnew(ConfirmationDialog);
	dlg_stop_fade_bgm->set_title(CG_TR("Parar / Fundir BGM"));
	dlg_stop_fade_bgm->set_ok_button_text(CG_TR("Aceptar"));
	dlg_stop_fade_bgm->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *sfv = memnew(VBoxContainer);
	sfv->add_theme_constant_override("separation", 6);
	dlg_stop_fade_bgm->add_child(sfv);

	HBoxContainer *sfh1 = memnew(HBoxContainer);
	sfh1->add_child(memnew(Label(CG_TR("Acción:"))));
	bgm_stop_action_opt = memnew(OptionButton);
	bgm_stop_action_opt->add_item(CG_TR("Fundir salida (Fadeout)"), 0);
	bgm_stop_action_opt->add_item(CG_TR("Parar inmediatamente"), 1);
	sfh1->add_child(bgm_stop_action_opt);
	sfv->add_child(sfh1);

	HBoxContainer *sfh2 = memnew(HBoxContainer);
	sfh2->add_child(memnew(Label(CG_TR("Duración de Fundido (fotogramas):"))));
	bgm_stop_duration_spin = memnew(SpinBox);
	bgm_stop_duration_spin->set_min(1);
	bgm_stop_duration_spin->set_max(600);
	bgm_stop_duration_spin->set_value(60);
	sfh2->add_child(bgm_stop_duration_spin);
	sfv->add_child(sfh2);

	dlg_stop_fade_bgm->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_stop_fade_bgm));
	add_child(dlg_stop_fade_bgm);

	// 24. Restaurar BGM (CMD_RESTORE_BGM)
	dlg_restore_bgm = memnew(ConfirmationDialog);
	dlg_restore_bgm->set_title(CG_TR("Restaurar BGM Guardada"));
	dlg_restore_bgm->set_ok_button_text(CG_TR("Aceptar"));
	dlg_restore_bgm->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *rbv = memnew(VBoxContainer);
	rbv->add_theme_constant_override("separation", 6);
	dlg_restore_bgm->add_child(rbv);

	HBoxContainer *rbh1 = memnew(HBoxContainer);
	rbh1->add_child(memnew(Label(CG_TR("Transición / Crossfade (fotogramas):"))));
	bgm_restore_duration_spin = memnew(SpinBox);
	bgm_restore_duration_spin->set_min(0);
	bgm_restore_duration_spin->set_max(300);
	bgm_restore_duration_spin->set_value(30);
	rbh1->add_child(bgm_restore_duration_spin);
	rbv->add_child(rbh1);

	dlg_restore_bgm->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_restore_bgm));
	add_child(dlg_restore_bgm);

	// 25. Stems de Audio (CMD_AUDIO_STEMS)
	dlg_stems = memnew(ConfirmationDialog);
	dlg_stems->set_title(CG_TR("Stems / Capas de Audio"));
	dlg_stems->set_ok_button_text(CG_TR("Aceptar"));
	dlg_stems->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *stv = memnew(VBoxContainer);
	stv->add_theme_constant_override("separation", 6);
	dlg_stems->add_child(stv);

	HBoxContainer *sth1 = memnew(HBoxContainer);
	sth1->add_child(memnew(Label(CG_TR("Pista / Stem (0 a 7):"))));
	stem_id_spin = memnew(SpinBox);
	stem_id_spin->set_min(0);
	stem_id_spin->set_max(7);
	stem_id_spin->set_value(0);
	sth1->add_child(stem_id_spin);
	stv->add_child(sth1);

	HBoxContainer *sth2 = memnew(HBoxContainer);
	sth2->add_child(memnew(Label(CG_TR("Archivo (opcional):"))));
	stem_file_edit = memnew(LineEdit);
	stem_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	stem_file_edit->set_placeholder("res://audio/bgm/stem_percussion.ogg");
	sth2->add_child(stem_file_edit);
	stem_file_browse_btn = memnew(Button(CG_TR("...")));
	stem_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo de Stem"));
	stem_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_stem_pressed));
	sth2->add_child(stem_file_browse_btn);
	stv->add_child(sth2);

	stem_file_dialog = memnew(FileDialog);
	stem_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	stem_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	stem_file_dialog->add_filter("*.ogg, *.wav, *.mp3 ; Archivos de Audio (*.ogg, *.wav, *.mp3)");
	stem_file_dialog->set_title(CG_TR("Seleccionar Stem de Audio"));
	stem_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_stem_file_selected));
	add_child(stem_file_dialog);

	HBoxContainer *sth3 = memnew(HBoxContainer);
	sth3->add_child(memnew(Label(CG_TR("Volumen (%):"))));
	stem_volume_spin = memnew(SpinBox);
	stem_volume_spin->set_min(0);
	stem_volume_spin->set_max(100);
	stem_volume_spin->set_value(100);
	sth3->add_child(stem_volume_spin);
	sth3->add_child(memnew(Label(CG_TR("Fundido (frames):"))));
	stem_fade_spin = memnew(SpinBox);
	stem_fade_spin->set_min(0);
	stem_fade_spin->set_max(300);
	stem_fade_spin->set_value(18);
	sth3->add_child(stem_fade_spin);
	stv->add_child(sth3);

	dlg_stems->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_stems));
	add_child(dlg_stems);

	// 26. Reproducir Cinemática (CMD_PLAY_CUTSCENE)
	dlg_play_cutscene = memnew(ConfirmationDialog);
	dlg_play_cutscene->set_title(CG_TR("Reproducir Cinemática"));
	dlg_play_cutscene->set_ok_button_text(CG_TR("Aceptar"));
	dlg_play_cutscene->set_cancel_button_text(CG_TR("Cancelar"));
	VBoxContainer *pcv = memnew(VBoxContainer);
	pcv->add_theme_constant_override("separation", 6);
	dlg_play_cutscene->add_child(pcv);

	HBoxContainer *pch1 = memnew(HBoxContainer);
	pch1->add_child(memnew(Label(CG_TR("Archivo / Nombre:"))));
	cutscene_file_edit = memnew(LineEdit);
	cutscene_file_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cutscene_file_edit->set_placeholder("res://cutscenes/intro.tres");
	pch1->add_child(cutscene_file_edit);
	cutscene_file_browse_btn = memnew(Button(CG_TR("...")));
	cutscene_file_browse_btn->set_tooltip_text(CG_TR("Seleccionar Archivo de Cinemática"));
	cutscene_file_browse_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_browse_cutscene_pressed));
	pch1->add_child(cutscene_file_browse_btn);
	pcv->add_child(pch1);

	cutscene_file_dialog = memnew(FileDialog);
	cutscene_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	cutscene_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	cutscene_file_dialog->add_filter("*.tres, *.res, *.json ; Cinemáticas (*.tres, *.res, *.json)");
	cutscene_file_dialog->set_title(CG_TR("Seleccionar Cinemática"));
	cutscene_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_cutscene_file_selected));
	add_child(cutscene_file_dialog);

	cutscene_wait_check = memnew(CheckBox(CG_TR("Esperar que termine la cinemática")));
	cutscene_wait_check->set_pressed(true);
	pcv->add_child(cutscene_wait_check);

	dlg_play_cutscene->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_play_cutscene));
	add_child(dlg_play_cutscene);

	// 27. Crear Cinemática (Pantalla mínima de Timeline - Anexo A1.2)
	dlg_create_cutscene = memnew(ConfirmationDialog);
	dlg_create_cutscene->set_title(CG_TR("Creador de Cinemáticas - Timeline"));
	dlg_create_cutscene->set_ok_button_text(CG_TR("Guardar Cinemática"));
	dlg_create_cutscene->set_cancel_button_text(CG_TR("Cancelar"));
	dlg_create_cutscene->set_min_size(Size2(560, 440) * EDSCALE);

	VBoxContainer *ccv = memnew(VBoxContainer);
	ccv->add_theme_constant_override("separation", 6);
	dlg_create_cutscene->add_child(ccv);

	HBoxContainer *cch1 = memnew(HBoxContainer);
	cch1->add_child(memnew(Label(CG_TR("Nombre:"))));
	cutscene_create_name_edit = memnew(LineEdit);
	cutscene_create_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cutscene_create_name_edit->set_text("Cinematica_01");
	cch1->add_child(cutscene_create_name_edit);
	cch1->add_child(memnew(Label(CG_TR("Duración Total (s):"))));
	cutscene_create_dur_spin = memnew(SpinBox);
	cutscene_create_dur_spin->set_min(0.5);
	cutscene_create_dur_spin->set_max(300.0);
	cutscene_create_dur_spin->set_step(0.1);
	cutscene_create_dur_spin->set_value(5.0);
	cch1->add_child(cutscene_create_dur_spin);
	ccv->add_child(cch1);

	HBoxContainer *cch2 = memnew(HBoxContainer);
	cutscene_create_letterbox_check = memnew(CheckBox(CG_TR("Barras de Cine (Letterbox)")));
	cutscene_create_letterbox_check->set_pressed(true);
	cch2->add_child(cutscene_create_letterbox_check);
	cutscene_create_block_check = memnew(CheckBox(CG_TR("Bloquear Controles Jugador")));
	cutscene_create_block_check->set_pressed(true);
	cch2->add_child(cutscene_create_block_check);
	ccv->add_child(cch2);

	ccv->add_child(memnew(Label(CG_TR("Lista de Keyframes:"))));
	cutscene_kf_tree = memnew(Tree);
	cutscene_kf_tree->set_columns(3);
	cutscene_kf_tree->set_column_title(0, CG_TR("Tiempo (s)"));
	cutscene_kf_tree->set_column_title(1, CG_TR("Pista"));
	cutscene_kf_tree->set_column_title(2, CG_TR("Valor / Parámetros"));
	cutscene_kf_tree->set_column_titles_visible(true);
	cutscene_kf_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cutscene_kf_tree->set_custom_minimum_size(Size2(500, 180) * EDSCALE);
	ccv->add_child(cutscene_kf_tree);

	HBoxContainer *cch3 = memnew(HBoxContainer);
	cch3->add_child(memnew(Label(CG_TR("T:"))));
	kf_time_spin = memnew(SpinBox);
	kf_time_spin->set_min(0.0);
	kf_time_spin->set_max(300.0);
	kf_time_spin->set_step(0.1);
	kf_time_spin->set_value(0.0);
	cch3->add_child(kf_time_spin);

	cch3->add_child(memnew(Label(CG_TR("Pista:"))));
	kf_track_opt = memnew(OptionButton);
	kf_track_opt->add_item(CG_TR("0: Cámara X"), 0);
	kf_track_opt->add_item(CG_TR("1: Cámara Y"), 1);
	kf_track_opt->add_item(CG_TR("2: Zoom"), 2);
	kf_track_opt->add_item(CG_TR("3: Fundido Pantalla"), 3);
	kf_track_opt->add_item(CG_TR("4: Mover Actor"), 4);
	kf_track_opt->add_item(CG_TR("5: Diálogo / Texto"), 5);
	kf_track_opt->add_item(CG_TR("6: Audio / BGM"), 6);
	kf_track_opt->add_item(CG_TR("7: Espera"), 7);
	cch3->add_child(kf_track_opt);

	cch3->add_child(memnew(Label(CG_TR("Valor:"))));
	kf_value_edit = memnew(LineEdit);
	kf_value_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	kf_value_edit->set_placeholder("100, target, texto, etc.");
	cch3->add_child(kf_value_edit);

	kf_add_btn = memnew(Button(CG_TR("+ Añadir")));
	kf_add_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_cutscene_add_kf));
	cch3->add_child(kf_add_btn);

	kf_remove_btn = memnew(Button(CG_TR("- Borrar")));
	kf_remove_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_cutscene_remove_kf));
	cch3->add_child(kf_remove_btn);
	ccv->add_child(cch3);

	dlg_create_cutscene->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_confirm_create_cutscene));
	add_child(dlg_create_cutscene);
}

void ClockGearsEventEditorDialog::_setup_database_picker_dialogs() {
	// Item Picker Dialog
	dlg_item_picker = memnew(ConfirmationDialog);
	dlg_item_picker->set_title(CG_TR("Seleccionar Objeto / Equipamiento"));
	dlg_item_picker->set_ok_button_text(CG_TR("Aceptar"));
	dlg_item_picker->set_cancel_button_text(CG_TR("Cancelar"));

	VBoxContainer *ipv = memnew(VBoxContainer);
	ipv->add_theme_constant_override("separation", 6);
	dlg_item_picker->add_child(ipv);

	HBoxContainer *shb = memnew(HBoxContainer);
	shb->add_child(memnew(Label(CG_TR("Buscar:"))));
	item_picker_search = memnew(LineEdit);
	item_picker_search->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_picker_search->set_placeholder(CG_TR("Filtrar por nombre..."));
	item_picker_search->connect("text_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_item_picker_search_changed));
	shb->add_child(item_picker_search);
	ipv->add_child(shb);

	item_picker_tabs = memnew(TabBar);
	item_picker_tabs->add_tab(CG_TR("Objetos"));
	item_picker_tabs->add_tab(CG_TR("Armas"));
	item_picker_tabs->add_tab(CG_TR("Armaduras"));
	item_picker_tabs->add_tab(CG_TR("Accesorios"));
	item_picker_tabs->connect("tab_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_item_picker_tab_changed));
	ipv->add_child(item_picker_tabs);

	item_picker_list = memnew(ItemList);
	item_picker_list->set_custom_minimum_size(Size2(360, 220) * EDSCALE);
	item_picker_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	item_picker_list->connect("item_activated", callable_mp(this, &ClockGearsEventEditorDialog::_on_item_picker_item_activated));
	ipv->add_child(item_picker_list);

	dlg_item_picker->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_item_picker_confirmed));
	add_child(dlg_item_picker);

	// Actor Picker Dialog
	dlg_actor_picker = memnew(ConfirmationDialog);
	dlg_actor_picker->set_title(CG_TR("Seleccionar Actor"));
	dlg_actor_picker->set_ok_button_text(CG_TR("Aceptar"));
	dlg_actor_picker->set_cancel_button_text(CG_TR("Cancelar"));

	VBoxContainer *apv = memnew(VBoxContainer);
	apv->add_theme_constant_override("separation", 6);
	dlg_actor_picker->add_child(apv);

	HBoxContainer *ashb = memnew(HBoxContainer);
	ashb->add_child(memnew(Label(CG_TR("Buscar:"))));
	actor_picker_search = memnew(LineEdit);
	actor_picker_search->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	actor_picker_search->set_placeholder(CG_TR("Filtrar actor por nombre..."));
	actor_picker_search->connect("text_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_actor_picker_search_changed));
	ashb->add_child(actor_picker_search);
	apv->add_child(ashb);

	actor_picker_list = memnew(ItemList);
	actor_picker_list->set_custom_minimum_size(Size2(320, 200) * EDSCALE);
	actor_picker_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	actor_picker_list->connect("item_activated", callable_mp(this, &ClockGearsEventEditorDialog::_on_actor_picker_item_activated));
	apv->add_child(actor_picker_list);

	dlg_actor_picker->connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_actor_picker_confirmed));
	add_child(dlg_actor_picker);
}

void ClockGearsEventEditorDialog::_open_item_picker(LineEdit *p_target) {
	target_item_line_edit = p_target;
	if (item_picker_search) {
		item_picker_search->clear();
	}
	_refresh_item_picker_list();
	if (dlg_item_picker) {
		dlg_item_picker->popup_centered(Size2(420, 350) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_refresh_item_picker_list() {
	if (!item_picker_list || !item_picker_tabs) return;
	item_picker_list->clear();
	int tab = item_picker_tabs->get_current_tab();
	int cat = CAT_ITEMS;
	if (tab == 1) cat = CAT_WEAPONS;
	else if (tab == 2) cat = CAT_ARMORS;
	else if (tab == 3) cat = CAT_ACCESSORIES;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	if (dm->get_category_array(cat).is_empty() && FileAccess::exists("res://clockgears_data/items.json")) {
		dm->load_all("res://clockgears_data");
	}

	String filter = item_picker_search ? item_picker_search->get_text().strip_edges().to_lower() : "";
	String target_val = target_item_line_edit ? target_item_line_edit->get_text().strip_edges() : "";
	int select_idx = -1;

	Array arr = dm->get_category_array(cat);
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		int id = d.get("id", i + 1);
		String name = d.get("name", "");
		if (!filter.is_empty()) {
			String id_str = String::num_int64(id);
			if (!name.to_lower().contains(filter) && !id_str.contains(filter)) {
				continue;
			}
		}
		String disp = vformat("%03d: %s", id, name.is_empty() ? CG_TR("(Sin Nombre)") : name);
		int nidx = item_picker_list->add_item(disp);
		item_picker_list->set_item_metadata(nidx, name.is_empty() ? vformat("Item%03d", id) : name);
		if (name == target_val) {
			select_idx = nidx;
		}
	}
	if (select_idx >= 0) {
		item_picker_list->select(select_idx);
		item_picker_list->ensure_current_is_visible();
	}
}

void ClockGearsEventEditorDialog::_on_item_picker_tab_changed(int p_tab) {
	_refresh_item_picker_list();
}

void ClockGearsEventEditorDialog::_on_item_picker_search_changed(const String &p_text) {
	_refresh_item_picker_list();
}

void ClockGearsEventEditorDialog::_on_item_picker_item_activated(int p_idx) {
	_on_item_picker_confirmed();
}

void ClockGearsEventEditorDialog::_on_item_picker_confirmed() {
	if (item_picker_list && target_item_line_edit) {
		Vector<int> sel = item_picker_list->get_selected_items();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < item_picker_list->get_item_count()) {
			String name = item_picker_list->get_item_metadata(sel[0]);
			target_item_line_edit->set_text(name);
		}
	}
	if (dlg_item_picker) {
		dlg_item_picker->hide();
	}
}

void ClockGearsEventEditorDialog::_open_actor_picker(LineEdit *p_target) {
	target_actor_line_edit = p_target;
	if (actor_picker_search) {
		actor_picker_search->clear();
	}
	_refresh_actor_picker_list();
	if (dlg_actor_picker) {
		dlg_actor_picker->popup_centered(Size2(380, 320) * EDSCALE);
	}
}

void ClockGearsEventEditorDialog::_refresh_actor_picker_list() {
	if (!actor_picker_list) return;
	actor_picker_list->clear();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	if (dm->get_category_array(CAT_ACTORS).is_empty() && FileAccess::exists("res://clockgears_data/actors.json")) {
		dm->load_all("res://clockgears_data");
	}

	String filter = actor_picker_search ? actor_picker_search->get_text().strip_edges().to_lower() : "";
	String target_val = target_actor_line_edit ? target_actor_line_edit->get_text().strip_edges() : "";
	int select_idx = -1;

	Array arr = dm->get_category_array(CAT_ACTORS);
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		int id = d.get("id", i + 1);
		String name = d.get("name", "");
		if (!filter.is_empty()) {
			String id_str = String::num_int64(id);
			if (!name.to_lower().contains(filter) && !id_str.contains(filter)) {
				continue;
			}
		}
		String disp = vformat("%03d: %s", id, name.is_empty() ? CG_TR("(Sin Nombre)") : name);
		int nidx = actor_picker_list->add_item(disp);
		actor_picker_list->set_item_metadata(nidx, name.is_empty() ? vformat("Actor%03d", id) : name);
		if (name == target_val) {
			select_idx = nidx;
		}
	}
	if (select_idx >= 0) {
		actor_picker_list->select(select_idx);
		actor_picker_list->ensure_current_is_visible();
	}
}

void ClockGearsEventEditorDialog::_on_actor_picker_search_changed(const String &p_text) {
	_refresh_actor_picker_list();
}

void ClockGearsEventEditorDialog::_on_actor_picker_item_activated(int p_idx) {
	_on_actor_picker_confirmed();
}

void ClockGearsEventEditorDialog::_on_actor_picker_confirmed() {
	if (actor_picker_list && target_actor_line_edit) {
		Vector<int> sel = actor_picker_list->get_selected_items();
		if (!sel.is_empty() && sel[0] >= 0 && sel[0] < actor_picker_list->get_item_count()) {
			String name = actor_picker_list->get_item_metadata(sel[0]);
			target_actor_line_edit->set_text(name);
		}
	}
	if (dlg_actor_picker) {
		dlg_actor_picker->hide();
	}
}

ClockGearsEventEditorDialog::ClockGearsEventEditorDialog() {
	set_title(CG_TR("Editor de Eventos - ClockGears Engine"));
	set_min_size(Size2(860, 560) * EDSCALE);
	set_ok_button_text(CG_TR("Aceptar"));
	add_cancel_button(CG_TR("Cancelar"));

	if (get_ok_button()) {
		get_ok_button()->set_custom_minimum_size(Size2(90, 28) * EDSCALE);
	}

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 6);
	add_child(main_vb);

	// -------------------------------------------------------------
	// Header: ID, Event Name, and Page Management Buttons
	// -------------------------------------------------------------
	HBoxContainer *header_hb = memnew(HBoxContainer);
	header_hb->add_theme_constant_override("separation", 8);
	main_vb->add_child(header_hb);

	id_label = memnew(Label("ID: 001"));
	id_label->add_theme_color_override("font_color", Color(0.9, 0.75, 0.3));
	header_hb->add_child(id_label);

	header_hb->add_child(memnew(VSeparator));

	header_hb->add_child(memnew(Label(CG_TR("Nombre:"))));
	name_edit = memnew(LineEdit);
	name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	name_edit->set_placeholder("EV001");
	header_hb->add_child(name_edit);

	btn_new_page = memnew(Button(CG_TR("Nueva Página")));
	btn_new_page->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_new_page_pressed));
	header_hb->add_child(btn_new_page);

	btn_copy_page = memnew(Button(CG_TR("Copiar Página")));
	btn_copy_page->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_copy_page_pressed));
	header_hb->add_child(btn_copy_page);

	btn_paste_page = memnew(Button(CG_TR("Pegar Página")));
	btn_paste_page->set_disabled(true);
	btn_paste_page->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_paste_page_pressed));
	header_hb->add_child(btn_paste_page);

	btn_delete_page = memnew(Button(CG_TR("Borrar Página")));
	btn_delete_page->set_disabled(true);
	btn_delete_page->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_delete_page_pressed));
	header_hb->add_child(btn_delete_page);

	// -------------------------------------------------------------
	// 1. Page Tabs (TabBar: "Página 1", "Página 2", etc.)
	// -------------------------------------------------------------
	page_tabs = memnew(TabBar);
	page_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	page_tabs->add_tab(CG_TR("Página 1"));
	page_tabs->connect("tab_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_page_tab_changed));
	main_vb->add_child(page_tabs);

	main_vb->add_child(memnew(HSeparator));

	// -------------------------------------------------------------
	// Three-Column Body (HBoxContainer de 3 columnas)
	// -------------------------------------------------------------
	HBoxContainer *columns_hb = memnew(HBoxContainer);
	columns_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	columns_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	columns_hb->add_theme_constant_override("separation", 6);
	main_vb->add_child(columns_hb);

	// =============================================================
	// COLUMN 1: Condiciones de Aparición, Disparador y Opciones
	// =============================================================
	VBoxContainer *col1 = memnew(VBoxContainer);
	col1->set_custom_minimum_size(Size2(240, 0) * EDSCALE);
	col1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col1->add_theme_constant_override("separation", 6);
	columns_hb->add_child(col1);

	// Group: Condiciones de Aparición
	PanelContainer *cond_card = memnew(PanelContainer);
	cond_card->add_theme_style_override("panel", _event_card_style());
	col1->add_child(cond_card);

	VBoxContainer *cond_box = memnew(VBoxContainer);
	cond_box->add_theme_constant_override("separation", 6);
	cond_card->add_child(cond_box);

	Label *cond_title = memnew(Label(CG_TR("Condiciones de Aparición")));
	cond_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	cond_box->add_child(cond_title);

	// Interruptor 1
	HBoxContainer *sw1_hb = memnew(HBoxContainer);
	cond_sw1_check = memnew(CheckBox);
	cond_sw1_check->set_text(CG_TR("Interruptor 1:"));
	sw1_hb->add_child(cond_sw1_check);
	cond_sw1_name_edit = memnew(LineEdit);
	cond_sw1_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sw1_hb->add_child(cond_sw1_name_edit);
	cond_box->add_child(sw1_hb);

	// Interruptor 2
	HBoxContainer *sw2_hb = memnew(HBoxContainer);
	cond_sw2_check = memnew(CheckBox);
	cond_sw2_check->set_text(CG_TR("Interruptor 2:"));
	sw2_hb->add_child(cond_sw2_check);
	cond_sw2_name_edit = memnew(LineEdit);
	cond_sw2_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sw2_hb->add_child(cond_sw2_name_edit);
	cond_box->add_child(sw2_hb);

	// Interruptor Local (A, B, C, D)
	HBoxContainer *self_sw_hb = memnew(HBoxContainer);
	cond_self_sw_check = memnew(CheckBox);
	cond_self_sw_check->set_text(CG_TR("Interruptor Local:"));
	self_sw_hb->add_child(cond_self_sw_check);
	cond_self_sw_opt = memnew(OptionButton);
	cond_self_sw_opt->add_item("A", 0);
	cond_self_sw_opt->add_item("B", 1);
	cond_self_sw_opt->add_item("C", 2);
	cond_self_sw_opt->add_item("D", 3);
	cond_self_sw_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	self_sw_hb->add_child(cond_self_sw_opt);
	cond_box->add_child(self_sw_hb);

	// Variable (>= X)
	HBoxContainer *var_hb = memnew(HBoxContainer);
	cond_var_check = memnew(CheckBox);
	cond_var_check->set_text(CG_TR("Variable:"));
	var_hb->add_child(cond_var_check);
	cond_var_name_edit = memnew(LineEdit);
	cond_var_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	var_hb->add_child(cond_var_name_edit);
	var_hb->add_child(memnew(Label(">=")));
	cond_var_val_spin = memnew(SpinBox);
	cond_var_val_spin->set_min(-999999);
	cond_var_val_spin->set_max(999999);
	cond_var_val_spin->set_value(0);
	var_hb->add_child(cond_var_val_spin);
	cond_box->add_child(var_hb);

	// Objeto en Inventario
	HBoxContainer *item_hb = memnew(HBoxContainer);
	cond_item_check = memnew(CheckBox);
	cond_item_check->set_text(CG_TR("Objeto:"));
	item_hb->add_child(cond_item_check);
	cond_item_name_edit = memnew(LineEdit);
	cond_item_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	item_hb->add_child(cond_item_name_edit);
	Button *pick_cond_item_btn = memnew(Button("..."));
	pick_cond_item_btn->set_tooltip_text(CG_TR("Seleccionar objeto desde la Base de Datos"));
	pick_cond_item_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_open_item_picker).bind(cond_item_name_edit));
	item_hb->add_child(pick_cond_item_btn);
	cond_box->add_child(item_hb);

	// Actor en Grupo
	HBoxContainer *actor_hb = memnew(HBoxContainer);
	cond_actor_check = memnew(CheckBox);
	cond_actor_check->set_text(CG_TR("Actor:"));
	actor_hb->add_child(cond_actor_check);
	cond_actor_name_edit = memnew(LineEdit);
	cond_actor_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	actor_hb->add_child(cond_actor_name_edit);
	Button *pick_cond_actor_btn = memnew(Button("..."));
	pick_cond_actor_btn->set_tooltip_text(CG_TR("Seleccionar actor desde la Base de Datos"));
	pick_cond_actor_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_open_actor_picker).bind(cond_actor_name_edit));
	actor_hb->add_child(pick_cond_actor_btn);
	cond_box->add_child(actor_hb);

	// Group: Disparador
	PanelContainer *trig_card = memnew(PanelContainer);
	trig_card->add_theme_style_override("panel", _event_card_style());
	col1->add_child(trig_card);

	VBoxContainer *trig_box = memnew(VBoxContainer);
	trig_box->add_theme_constant_override("separation", 6);
	trig_card->add_child(trig_box);

	Label *trig_title = memnew(Label(CG_TR("Disparador (Trigger)")));
	trig_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	trig_box->add_child(trig_title);

	trigger_opt = memnew(OptionButton);
	trigger_opt->add_item(CG_TR("Botón de Acción"), TRIGGER_ACTION_BUTTON);
	trigger_opt->add_item(CG_TR("Contacto con Jugador"), TRIGGER_PLAYER_TOUCH);
	trigger_opt->add_item(CG_TR("Contacto con Evento"), TRIGGER_EVENT_TOUCH);
	trigger_opt->add_item(CG_TR("Ejecución Automática"), TRIGGER_AUTORUN);
	trigger_opt->add_item(CG_TR("Proceso Paralelo"), TRIGGER_PARALLEL);
	trig_box->add_child(trigger_opt);

	// Group: Opciones
	PanelContainer *opt_card = memnew(PanelContainer);
	opt_card->add_theme_style_override("panel", _event_card_style());
	col1->add_child(opt_card);

	VBoxContainer *opt_box = memnew(VBoxContainer);
	opt_box->add_theme_constant_override("separation", 6);
	opt_card->add_child(opt_box);

	Label *opt_title = memnew(Label(CG_TR("Opciones de Prioridad y Físicas")));
	opt_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	opt_box->add_child(opt_title);

	HBoxContainer *prio_hb = memnew(HBoxContainer);
	prio_hb->add_child(memnew(Label(CG_TR("Prioridad:"))));
	priority_opt = memnew(OptionButton);
	priority_opt->add_item(CG_TR("Por debajo de los personajes"), 0);
	priority_opt->add_item(CG_TR("Mismo nivel que personajes"), 1);
	priority_opt->add_item(CG_TR("Por encima de los personajes"), 2);
	priority_opt->select(1);
	priority_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prio_hb->add_child(priority_opt);
	opt_box->add_child(prio_hb);

	through_check = memnew(CheckBox(CG_TR("Atravesable (Sin colisión)")));
	opt_box->add_child(through_check);

	dir_fix_check = memnew(CheckBox(CG_TR("Bloquear Dirección (Fija)")));
	opt_box->add_child(dir_fix_check);

	// =============================================================
	// COLUMN 2: Gráfico / SpriteSheet Frame y Movimiento Autónomo
	// =============================================================
	VBoxContainer *col2 = memnew(VBoxContainer);
	col2->set_custom_minimum_size(Size2(240, 0) * EDSCALE);
	col2->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2->add_theme_constant_override("separation", 6);
	columns_hb->add_child(col2);

	// Group: Gráfico del Evento
	PanelContainer *gfx_card = memnew(PanelContainer);
	gfx_card->add_theme_style_override("panel", _event_card_style());
	col2->add_child(gfx_card);

	VBoxContainer *gfx_box = memnew(VBoxContainer);
	gfx_box->add_theme_constant_override("separation", 6);
	gfx_card->add_child(gfx_box);

	Label *gfx_title = memnew(Label(CG_TR("Imagen / SpriteSheet (Doble Clic)")));
	gfx_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	gfx_box->add_child(gfx_title);

	CenterContainer *preview_center = memnew(CenterContainer);
	preview_center->set_custom_minimum_size(Size2(0, 108) * EDSCALE);
	gfx_box->add_child(preview_center);

	PanelContainer *preview_frame = memnew(PanelContainer);
	Ref<StyleBoxFlat> preview_sb;
	preview_sb.instantiate();
	preview_sb->set_bg_color(Color(0.08, 0.08, 0.1));
	preview_sb->set_border_width_all(1);
	preview_sb->set_border_color(Color(0.9, 0.75, 0.3, 0.5));
	preview_sb->set_corner_radius_all(4);
	preview_frame->add_theme_style_override("panel", preview_sb);
	preview_center->add_child(preview_frame);

	graphic_preview = memnew(TextureRect);
	graphic_preview->set_custom_minimum_size(Size2(96, 96) * EDSCALE);
	graphic_preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	graphic_preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	graphic_preview->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST); // A4.5: Vista previa en NEAREST
	graphic_preview->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	graphic_preview->set_tooltip_text(CG_TR("Doble clic para cargar SpriteSheet/Atlas"));
	graphic_preview->connect("gui_input", callable_mp(this, &ClockGearsEventEditorDialog::_on_graphic_preview_gui_input));
	preview_frame->add_child(graphic_preview);

	// SpriteSheet / Atlas Frame Selectors
	HBoxContainer *atlas_hb = memnew(HBoxContainer);
	atlas_hb->add_theme_constant_override("separation", 4);
	gfx_box->add_child(atlas_hb);

	atlas_hb->add_child(memnew(Label("Cols:")));
	sprite_hframes_spin = memnew(SpinBox);
	sprite_hframes_spin->set_min(1);
	sprite_hframes_spin->set_max(64);
	sprite_hframes_spin->set_value(1);
	sprite_hframes_spin->connect("value_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_sprite_frame_changed));
	atlas_hb->add_child(sprite_hframes_spin);

	atlas_hb->add_child(memnew(Label("Filas:")));
	sprite_vframes_spin = memnew(SpinBox);
	sprite_vframes_spin->set_min(1);
	sprite_vframes_spin->set_max(64);
	sprite_vframes_spin->set_value(1);
	sprite_vframes_spin->connect("value_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_sprite_frame_changed));
	atlas_hb->add_child(sprite_vframes_spin);

	atlas_hb->add_child(memnew(Label("Frame:")));
	sprite_frame_spin = memnew(SpinBox);
	sprite_frame_spin->set_min(0);
	sprite_frame_spin->set_max(0);
	sprite_frame_spin->set_value(0);
	sprite_frame_spin->connect("value_changed", callable_mp(this, &ClockGearsEventEditorDialog::_on_sprite_frame_changed));
	atlas_hb->add_child(sprite_frame_spin);

	HBoxContainer *gfx_btn_hb = memnew(HBoxContainer);
	gfx_btn_hb->add_theme_constant_override("separation", 4);
	select_graphic_btn = memnew(Button(CG_TR("Sprite...")));
	select_graphic_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	select_graphic_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_select_graphic_pressed));
	gfx_btn_hb->add_child(select_graphic_btn);

	select_tile_graphic_btn = memnew(Button(CG_TR("Tile 32x32...")));
	select_tile_graphic_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	select_tile_graphic_btn->set_tooltip_text(CG_TR("Elegir un tile de 32x32 desde un TileSet para este evento"));
	select_tile_graphic_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_select_tile_graphic_pressed));
	gfx_btn_hb->add_child(select_tile_graphic_btn);

	clear_graphic_btn = memnew(Button(CG_TR("Vaciar")));
	clear_graphic_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_clear_graphic_pressed));
	gfx_btn_hb->add_child(clear_graphic_btn);
	gfx_box->add_child(gfx_btn_hb);

	event_tile_picker_dialog = memnew(ClockGearsTilePickerDialog);
	add_child(event_tile_picker_dialog);

	// Group: Movimiento Autónomo
	PanelContainer *move_card = memnew(PanelContainer);
	move_card->add_theme_style_override("panel", _event_card_style());
	col2->add_child(move_card);

	VBoxContainer *move_box = memnew(VBoxContainer);
	move_box->add_theme_constant_override("separation", 6);
	move_card->add_child(move_box);

	Label *move_title = memnew(Label(CG_TR("Movimiento Autónomo")));
	move_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	move_box->add_child(move_title);

	move_box->add_child(memnew(Label(CG_TR("Tipo:"))));
	move_type_opt = memnew(OptionButton);
	move_type_opt->add_item(CG_TR("Fijo"), MOVE_FIXED);
	move_type_opt->add_item(CG_TR("Aleatorio"), MOVE_RANDOM);
	move_type_opt->add_item(CG_TR("Acercarse"), MOVE_APPROACH_PLAYER);
	move_type_opt->add_item(CG_TR("Ruta Personalizada"), MOVE_CUSTOM_ROUTE);
	move_box->add_child(move_type_opt);

	HBoxContainer *spd_hb = memnew(HBoxContainer);
	spd_hb->add_child(memnew(Label(CG_TR("Velocidad (1-6):"))));
	speed_opt = memnew(OptionButton);
	speed_opt->add_item(CG_TR("1: ×8 Más Lenta"), 1);
	speed_opt->add_item(CG_TR("2: ×4 Más Lenta"), 2);
	speed_opt->add_item(CG_TR("3: ×2 Más Lenta"), 3);
	speed_opt->add_item(CG_TR("4: Normal"), 4);
	speed_opt->add_item(CG_TR("5: ×2 Más Rápida"), 5);
	speed_opt->add_item(CG_TR("6: ×4 Más Rápida"), 6);
	speed_opt->select(2);
	speed_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	spd_hb->add_child(speed_opt);
	move_box->add_child(spd_hb);

	HBoxContainer *frq_hb = memnew(HBoxContainer);
	frq_hb->add_child(memnew(Label(CG_TR("Frecuencia (1-5):"))));
	freq_opt = memnew(OptionButton);
	freq_opt->add_item(CG_TR("1: Más Baja"), 1);
	freq_opt->add_item(CG_TR("2: Baja"), 2);
	freq_opt->add_item(CG_TR("3: Normal"), 3);
	freq_opt->add_item(CG_TR("4: Alta"), 4);
	freq_opt->add_item(CG_TR("5: Más Alta"), 5);
	freq_opt->select(2);
	freq_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	frq_hb->add_child(freq_opt);
	move_box->add_child(frq_hb);

	// =============================================================
	// COLUMN 3: Contenido / Tree de Comandos del Evento
	// =============================================================
	VBoxContainer *col3 = memnew(VBoxContainer);
	col3->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col3->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col3->add_theme_constant_override("separation", 6);
	columns_hb->add_child(col3);

	HBoxContainer *cmd_header_hb = memnew(HBoxContainer);
	Label *cmd_title = memnew(Label(CG_TR("Contenido / Lista de Comandos:")));
	cmd_title->add_theme_color_override("font_color", Color(0.95, 0.8, 0.35));
	cmd_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cmd_header_hb->add_child(cmd_title);
	col3->add_child(cmd_header_hb);

	command_tree = memnew(Tree);
	command_tree->set_hide_root(true);
	command_tree->set_columns(1);
	command_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	command_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	command_tree->connect("item_activated", callable_mp(this, &ClockGearsEventEditorDialog::_on_edit_command_pressed));
	col3->add_child(command_tree);

	// Botones situados DIRECTAMENTE DEBAJO del Tree de comandos
	HBoxContainer *btn_hb = memnew(HBoxContainer);
	btn_hb->add_theme_constant_override("separation", 6);
	col3->add_child(btn_hb);

	add_cmd_btn = memnew(Button(CG_TR("+ Insertar...")));
	add_cmd_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_add_command_pressed));
	btn_hb->add_child(add_cmd_btn);

	edit_cmd_btn = memnew(Button(CG_TR("Editar")));
	edit_cmd_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	edit_cmd_btn->set_tooltip_text(CG_TR("Editar parámetros del comando seleccionado"));
	edit_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_edit_command_pressed));
	btn_hb->add_child(edit_cmd_btn);

	remove_cmd_btn = memnew(Button(CG_TR("Eliminar")));
	remove_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_remove_command_pressed));
	btn_hb->add_child(remove_cmd_btn);

	clear_cmd_btn = memnew(Button(CG_TR("Vaciar")));
	clear_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_clear_commands_pressed));
	btn_hb->add_child(clear_cmd_btn);

	move_up_cmd_btn = memnew(Button(String::utf8("▲ Subir")));
	move_up_cmd_btn->set_tooltip_text(CG_TR("Subir comando seleccionado"));
	move_up_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_move_command_up));
	btn_hb->add_child(move_up_cmd_btn);

	move_down_cmd_btn = memnew(Button(String::utf8("▼ Bajar")));
	move_down_cmd_btn->set_tooltip_text(CG_TR("Bajar comando seleccionado"));
	move_down_cmd_btn->connect("pressed", callable_mp(this, &ClockGearsEventEditorDialog::_on_move_command_down));
	btn_hb->add_child(move_down_cmd_btn);

	// Setup FileDialog for Sprite selection
	graphic_file_dialog = memnew(FileDialog);
	graphic_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	graphic_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	graphic_file_dialog->add_filter("*.png, *.webp ; Sprites / Atlas (*.png, *.webp)");
	graphic_file_dialog->set_title(CG_TR("Seleccionar SpriteSheet / Atlas de Evento (img/characters)"));
	graphic_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_graphic_file_selected));
	add_child(graphic_file_dialog);

	// Setup Visual Command Catalog
	cmd_catalog_dialog = memnew(ClockGearsCommandSelector);
	cmd_catalog_dialog->connect("command_selected", callable_mp(this, &ClockGearsEventEditorDialog::_on_command_id_selected));
	add_child(cmd_catalog_dialog);

	// Setup parameter sub-dialogs
	_setup_command_subdialogs();
	_setup_database_picker_dialogs();

	connect("confirmed", callable_mp(this, &ClockGearsEventEditorDialog::_on_dialog_confirmed));
	connect("canceled", callable_mp(this, &ClockGearsEventEditorDialog::_on_dialog_canceled));
}

void ClockGearsEventEditorDialog::edit_event(ClockGearsEvent *p_event) {
	target_event = p_event;
	if (!target_event) {
		return;
	}

	id_label->set_text(vformat(CG_TR("ID: %03d"), target_event->get_event_id()));
	name_edit->set_text(target_event->get_event_name());

	working_pages = target_event->get_pages().duplicate(true);
	if (working_pages.is_empty()) {
		Dictionary p0 = _create_empty_page();
		p0["sw1_en"] = target_event->is_condition_switch_1_enabled();
		p0["sw1_name"] = target_event->get_condition_switch_1_name();
		p0["sw2_en"] = target_event->is_condition_switch_2_enabled();
		p0["sw2_name"] = target_event->get_condition_switch_2_name();
		p0["self_sw_en"] = target_event->is_condition_self_switch_enabled();
		p0["self_sw_letter"] = target_event->get_condition_self_switch_letter();
		p0["var_en"] = target_event->is_condition_var_enabled();
		p0["var_name"] = target_event->get_condition_var_name();
		p0["var_val"] = target_event->get_condition_var_value();
		p0["item_en"] = target_event->is_condition_item_enabled();
		p0["item_name"] = target_event->get_condition_item_name();
		p0["actor_en"] = target_event->is_condition_actor_enabled();
		p0["actor_name"] = target_event->get_condition_actor_name();
		p0["trigger"] = target_event->get_trigger_type();
		p0["priority"] = target_event->get_priority_type();
		p0["through"] = target_event->is_through();
		p0["dir_fix"] = target_event->is_direction_fix();
		p0["graphic_path"] = target_event->get_graphic_path();
		p0["hframes"] = target_event->get_sprite_hframes();
		p0["vframes"] = target_event->get_sprite_vframes();
		p0["frame"] = target_event->get_sprite_frame();
		p0["move_type"] = target_event->get_move_type();
		p0["speed"] = CLAMP(int(target_event->get_move_speed() / 40.0f), 1, 6);
		p0["freq"] = CLAMP(int(target_event->get_move_frequency()), 1, 5);
		p0["commands"] = target_event->get_commands().duplicate(true);
		working_pages.push_back(p0);
	}

	current_page_index = 0;
	_rebuild_page_tabs();
	_load_page_to_ui(0);

	if (is_inside_tree()) {
		popup_centered();
	}
}

void ClockGearsEventEditorDialog::_refresh_command_list() {
	if (!command_tree) {
		return;
	}
	command_tree->clear();
	TreeItem *root = command_tree->create_item();
	if (!target_event) {
		return;
	}

	Array cmds = _get_active_page_commands();
	for (int i = 0; i < cmds.size(); i++) {
		Dictionary d = cmds[i];
		String type = d.get("type", "Unknown");
		String text;
		if (type == "message") {
			text = vformat(CG_TR("◆ Mostrar Mensaje: \"%s\""), d.get("text", ""));
		} else if (type == "choices") {
			PackedStringArray opts;
			for (int c = 1; c <= 6; c++) {
				String key = "c" + itos(c);
				if (d.has(key) && !String(d[key]).is_empty()) {
					opts.push_back(d[key]);
				}
			}
			text = vformat(CG_TR("◆ Mostrar Elecciones: [%s]"), String(", ").join(opts));
		} else if (type == "when_choice" || int(d.get("code", 0)) == 402) {
			text = vformat(CG_TR(" : Cuando [%s]"), d.get("text", vformat(CG_TR("Opción %d"), int(d.get("choice", 0)) + 1)));
		} else if (type == "when_cancel" || int(d.get("code", 0)) == 403) {
			text = CG_TR(" : Cuando Cancelar");
		} else if (type == "branch_end_choice" || int(d.get("code", 0)) == 404) {
			text = CG_TR(" : Fin Elecciones");
		} else if (type == "number_input") {
			text = vformat(CG_TR("◆ Entrada de Número: Variable [%s], %d dígitos"), d.get("variable", 1), d.get("digits", 3));
		} else if (type == "switch") {
			text = vformat(CG_TR("◆ Control Interruptor: [%s] = %s"), d.get("name", ""), d.get("val", true) ? "ON" : "OFF");
		} else if (type == "self_switch") {
			text = vformat(CG_TR("◆ Control Interruptor Local: [%s] = %s"), d.get("letter", "A"), d.get("val", true) ? "ON" : "OFF");
		} else if (type == "variable") {
			text = vformat(CG_TR("◆ Control Variable: [%s] %s %s"), d.get("name", ""), d.get("op", "="), d.get("val", 0));
		} else if (type == "wait") {
			text = vformat(CG_TR("◆ Esperar: %d fotogramas"), int(d.get("frames", 60)));
		} else if (type == "teleport") {
			text = vformat(CG_TR("◆ Transferir Jugador: %s a (%d, %d) [%s]"), d.get("map", ""), int(d.get("x", 0)), int(d.get("y", 0)), d.get("dir", ""));
		} else if (type == "move_route") {
			text = vformat(CG_TR("◆ Fijar Ruta [%s]: %s"), d.get("target", "Este Evento"), d.get("action", ""));
		} else if (type == "item") {
			text = vformat(CG_TR("◆ Cambiar Objeto: %s%d %s"), d.get("op", "+"), int(d.get("qty", 1)), d.get("name", ""));
		} else if (type == "gold") {
			text = vformat(CG_TR("◆ Cambiar Dinero: %s%d Oro"), d.get("op", "+"), int(d.get("amount", 0)));
		} else if (type == "hp_mp") {
			text = vformat(CG_TR("◆ Cambiar %s: %s%d"), d.get("stat", "HP"), d.get("op", "+"), int(d.get("amount", 50)));
		} else if (type == "audio") {
			text = vformat(CG_TR("◆ Reproducir Audio: %s"), d.get("file", ""));
		} else if (type == "se") {
			text = vformat(CG_TR("◆ Reproducir SE: %s"), d.get("file", ""));
		} else if (type == "flash") {
			text = vformat(CG_TR("◆ Efecto de Flash: %s (%d f)"), d.get("color", ""), int(d.get("duration", 30)));
		} else if (type == "tint") {
			text = vformat(CG_TR("◆ Teñir Pantalla: %s (%d f)"), d.get("preset", ""), int(d.get("duration", 60)));
		} else if (type == "fade") {
			text = vformat(CG_TR("◆ Fundido de Pantalla: %s (%d f)"), d.get("mode", "Oscurecer"), int(d.get("duration", 30)));
		} else if (type == "battle") {
			text = vformat(CG_TR("◆ Iniciar Batalla: [%s] (Huir: %s, Derrota: %s)"), d.get("troop", "1"), bool(d.get("can_escape", true)) ? "Sí" : "No", bool(d.get("can_lose", false)) ? "Permitida" : "Game Over");
		} else if (type == "show_picture" || type == "picture") {
			int dur_m = int(d.get("duration_mode", 0));
			String dur_str;
			if (dur_m == 1) {
				dur_str = vformat(" (%.1fs%s%s)", float(d.get("duration_sec", 3.0f)), bool(d.get("auto_fadeout", false)) ? " + fundido" : "", bool(d.get("wait", false)) ? " [esperar]" : "");
			} else if (dur_m == 2) {
				dur_str = " (hasta próx. mensaje)";
			}
			text = vformat(CG_TR("◆ Mostrar Imagen: #%d \"%s\" en (%d, %d)%s"), int(d.get("id", 1)), d.get("path", ""), int(d.get("x", 0)), int(d.get("y", 0)), dur_str);
		} else if (type == "move_picture") {
			text = vformat(CG_TR("◆ Mover Imagen: #%d a (%d, %d) en %d f"), int(d.get("id", 1)), int(d.get("x", 0)), int(d.get("y", 0)), int(d.get("duration", 60)));
		} else if (type == "rotate_picture") {
			text = vformat(CG_TR("◆ Rotar Imagen: #%d a %.1f°/s"), int(d.get("id", 1)), float(d.get("speed", 0.0f)));
		} else if (type == "tint_picture") {
			text = vformat(CG_TR("◆ Teñir Imagen: #%d (%s, %d f)"), int(d.get("id", 1)), d.get("preset", "Normal"), int(d.get("duration", 60)));
		} else if (type == "erase_picture") {
			text = vformat(CG_TR("◆ Borrar Imagen: #%d"), int(d.get("id", 1)));
		} else if (type == "shake_screen" || type == "shake") {
			text = vformat(CG_TR("◆ Temblor de Pantalla: Potencia %d, Vel %d (%d f)"), int(d.get("power", 5)), int(d.get("speed", 5)), int(d.get("duration", 60)));
		} else if (type == "weather") {
			text = vformat(CG_TR("◆ Clima: %s, Potencia %d (%d f)"), d.get("weather", "none"), int(d.get("power", 5)), int(d.get("duration", 60)));
		} else if (type == "play_bgs" || type == "bgs") {
			text = vformat(CG_TR("◆ Reproducir BGS: %s (Vol %d%%)"), d.get("file", ""), int(d.get("volume", 90)));
		} else if (type == "play_me" || type == "me") {
			text = vformat(CG_TR("◆ Reproducir ME: %s (Vol %d%%)"), d.get("file", ""), int(d.get("volume", 90)));
		} else if (type == "stop_fade_bgm") {
			text = vformat(CG_TR("◆ Parar / Fundir BGM: %s (%d f)"), d.get("action", "fade") == "stop" ? CG_TR("Parar") : CG_TR("Fundir"), int(d.get("duration", 60)));
		} else if (type == "save_bgm") {
			text = CG_TR("◆ Guardar BGM");
		} else if (type == "restore_bgm") {
			text = vformat(CG_TR("◆ Restaurar BGM (%d f)"), int(d.get("duration", 30)));
		} else if (type == "audio_stems" || type == "stems") {
			text = vformat(CG_TR("◆ Stem Audio #%d: Vol %d%%"), int(d.get("stem", 0)), int(float(d.get("volume", 1.0f)) * 100.0f));
		} else if (type == "change_camera_zoom") {
			text = vformat(CG_TR("◆ Cambiar Zoom de Cámara: %.1fx (Duración: %d f%s)"), float(d.get("zoom", 2.0f)), int(d.get("duration", 0)), bool(d.get("wait", false)) ? CG_TR(", Esperar") : "");
		} else if (type == "play_cutscene" || type == "cutscene") {
			if (d.has("cutscene_dict")) {
				Dictionary csd = d.get("cutscene_dict", Dictionary());
				text = vformat(CG_TR("◆ Cinemática: \"%s\" (%.1f s, %d kf)"), csd.get("name", "Cinematica"), float(csd.get("duration", 5.0f)), Array(csd.get("keyframes", Array())).size());
			} else {
				text = vformat(CG_TR("◆ Reproducir Cinemática: %s"), d.get("path", ""));
			}
		} else {
			text = vformat(CG_TR("◆ Comando: %s"), type);
		}
		TreeItem *item = command_tree->create_item(root);
		item->set_text(0, text);
		item->set_metadata(0, i);
	}
	TreeItem *end_marker = command_tree->create_item(root);
	end_marker->set_text(0, String::utf8("◆"));
	end_marker->set_metadata(0, -1);
}

void ClockGearsEventEditorDialog::_on_remove_command_pressed() {
	if (!target_event || !command_tree) {
		return;
	}
	TreeItem *sel = command_tree->get_selected();
	if (!sel) {
		return;
	}
	int idx = int(sel->get_metadata(0));
	Array cmds = _get_active_page_commands();
	if (idx >= 0 && idx < cmds.size()) {
		cmds.remove_at(idx);
		_set_active_page_commands(cmds);
		_refresh_command_list();
	}
}

void ClockGearsEventEditorDialog::_on_clear_commands_pressed() {
	if (!target_event) {
		return;
	}
	_set_active_page_commands(Array());
	_refresh_command_list();
}

void ClockGearsEventEditorDialog::_on_dialog_confirmed() {
	if (!target_event) {
		return;
	}
	_save_current_page_from_ui();
	target_event->set_event_name(name_edit->get_text());
	target_event->set_pages(working_pages.duplicate(true));

	// Sync primary properties with active page (or Page 1) for immediate map display
	Dictionary active_p = working_pages[CLAMP(current_page_index, 0, working_pages.size() - 1)];
	target_event->set_condition_switch_1_enabled(active_p.get("sw1_en", false));
	target_event->set_condition_switch_1_name(active_p.get("sw1_name", ""));
	target_event->set_condition_switch_2_enabled(active_p.get("sw2_en", false));
	target_event->set_condition_switch_2_name(active_p.get("sw2_name", ""));
	target_event->set_condition_self_switch_enabled(active_p.get("self_sw_en", false));
	target_event->set_condition_self_switch_letter(active_p.get("self_sw_letter", "A"));
	target_event->set_condition_var_enabled(active_p.get("var_en", false));
	target_event->set_condition_var_name(active_p.get("var_name", ""));
	target_event->set_condition_var_value(int(active_p.get("var_val", 0)));
	target_event->set_condition_item_enabled(active_p.get("item_en", false));
	target_event->set_condition_item_name(active_p.get("item_name", ""));
	target_event->set_condition_actor_enabled(active_p.get("actor_en", false));
	target_event->set_condition_actor_name(active_p.get("actor_name", ""));

	target_event->set_trigger_type(int(active_p.get("trigger", 0)));
	target_event->set_priority_type(int(active_p.get("priority", 1)));
	target_event->set_through(active_p.get("through", false));
	target_event->set_direction_fix(active_p.get("dir_fix", false));

	target_event->set_sprite_hframes(int(active_p.get("hframes", 1)));
	target_event->set_sprite_vframes(int(active_p.get("vframes", 1)));
	target_event->set_sprite_frame(int(active_p.get("frame", 0)));
	target_event->set_graphic_path(active_p.get("graphic_path", ""));

	target_event->set_move_type(int(active_p.get("move_type", 0)));
	target_event->set_move_speed(int(active_p.get("speed", 3)) * 40.0f);
	target_event->set_move_frequency(int(active_p.get("freq", 3)));
	target_event->set_commands(active_p.get("commands", Array()));

	// Persist to metadata for reliable backend serialization
	target_event->serialize_to_meta();

	// CRITICAL GODOT BACKEND SERIALIZATION RULE:
	// The event node MUST be a child of the map and its owner MUST be set to map_root!
	Node *map_root = ClockGearsEditorPlugin::get_singleton() ? ClockGearsEditorPlugin::get_singleton()->ensure_active_map() : (EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr);
	if (map_root) {
		if (target_event->get_parent() == nullptr) {
			map_root->add_child(target_event);
		}
		target_event->set_owner(map_root);

		EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
		if (ur) {
			int history_id = ur->get_history_id_for_object(map_root);
			ur->set_history_as_unsaved(history_id);
		}

		String scene_path = map_root->get_scene_file_path();
		if (!scene_path.is_empty()) {
			EditorNode::get_singleton()->save_scene_if_open(scene_path);
		}
	}

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}

void ClockGearsEventEditorDialog::_on_dialog_canceled() {
	if (target_event) {
		if (target_event->get_commands().is_empty() && target_event->get_pages().is_empty() && target_event->get_graphic_path().is_empty()) {
			target_event->queue_free();
			target_event = nullptr;
		}
	}
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}
