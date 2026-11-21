#include "clockgears_editor_plugin.h"
#include "clockgears_battle_window.h"
#include "clockgears_map.h"
#include "clockgears_animation.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_system_catalog.h"
#include "runtime/clockgears_title_screen.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/clockgears_theme.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/clockgears_map_cell_selector.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/main/viewport.h"
#include "servers/display/display_server.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_flat.h"

// ============================================================================
// ClockGearsGenerateEntityDialog
// ============================================================================

void ClockGearsGenerateEntityDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_for_cell", "x", "y"), &ClockGearsGenerateEntityDialog::open_for_cell);
	ClassDB::bind_method(D_METHOD("select_tab", "tab"), &ClockGearsGenerateEntityDialog::select_tab);
	ClassDB::bind_method(D_METHOD("get_selected_tab"), &ClockGearsGenerateEntityDialog::get_selected_tab);
	ClassDB::bind_method(D_METHOD("get_listed_count"), &ClockGearsGenerateEntityDialog::get_listed_count);
	ClassDB::bind_method(D_METHOD("select_list_index", "index"), &ClockGearsGenerateEntityDialog::select_list_index);
	ClassDB::bind_method(D_METHOD("check_layout_fits_window", "window_size"), &ClockGearsGenerateEntityDialog::check_layout_fits_window, DEFVAL(Vector2i(1366, 768)));
	ClassDB::bind_method(D_METHOD("get_target_cell"), &ClockGearsGenerateEntityDialog::get_target_cell);

	ClassDB::bind_static_method("ClockGearsGenerateEntityDialog", D_METHOD("populate_event_context_menu", "menu", "quick_menu", "has_event", "clipboard_empty"), &ClockGearsGenerateEntityDialog::populate_event_context_menu);
	ClassDB::bind_static_method("ClockGearsGenerateEntityDialog", D_METHOD("create_entity_event_on_map", "scene", "tab", "record_id", "x", "y", "grid_size", "use_undo_redo", "undo_redo"), &ClockGearsGenerateEntityDialog::create_entity_event_on_map, DEFVAL(32), DEFVAL(true), DEFVAL(Variant()));

	BIND_CONSTANT(TAB_NPC);
	BIND_CONSTANT(TAB_ARPG_NPC);
	BIND_CONSTANT(TAB_ENEMIGO);
	BIND_CONSTANT(TAB_ARPG_ENEMIGO);
	BIND_CONSTANT(TAB_FLORA);
	BIND_CONSTANT(TAB_FAUNA);

	ADD_SIGNAL(MethodInfo("entity_chosen",
			PropertyInfo(Variant::INT, "tab"),
			PropertyInfo(Variant::INT, "record_id"),
			PropertyInfo(Variant::STRING, "record_name"),
			PropertyInfo(Variant::INT, "cell_x"),
			PropertyInfo(Variant::INT, "cell_y")));
}

void ClockGearsGenerateEntityDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(this);
		}
	}
}

int ClockGearsGenerateEntityDialog::_tab_to_category(int p_tab) {
	switch (p_tab) {
		case TAB_NPC:
			return CAT_NPCS;
		case TAB_ARPG_NPC:
			return CAT_ARPG_NPCS;
		case TAB_ENEMIGO:
			return CAT_TROOPS;
		case TAB_ARPG_ENEMIGO:
			return CAT_ARPG_ENEMIES;
		default:
			return -1; // FLORA y FAUNA no tienen categoría ni datos en BD
	}
}

static Ref<Texture2D> s_extract_sheet_first_frame(const Dictionary &p_rec, String &r_desc) {
	Dictionary sp_dict = p_rec.get("sprite_data", Dictionary());
	if (sp_dict.is_empty()) {
		return Ref<Texture2D>();
	}
	ClockGearsSpriteSheetData sheet;
	sheet.from_dict(sp_dict);
	bool dir_idle = bool(p_rec.get("directional_idle", false));
	ClockGearsSpriteAnimation resolved;
	bool freeze_f0 = false;
	String res_slot;
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	bool ok = reg && reg->resolve_actor_animation(sheet, dir_idle, "quieto", 2, resolved, freeze_f0, res_slot) && resolved.get_frame_count() > 0;
	if (!ok) {
		const char *fallbacks[4] = { "mover_abajo", "idle", "atacar_abajo", "attack" };
		for (int i = 0; i < 4; i++) {
			if (sheet.animations.has(fallbacks[i]) && sheet.animations[fallbacks[i]].get_frame_count() > 0) {
				resolved = sheet.animations[fallbacks[i]];
				res_slot = fallbacks[i];
				ok = true;
				break;
			}
		}
	}
	if (!ok || resolved.get_frame_count() <= 0) {
		return Ref<Texture2D>();
	}

	ClockGearsAnimationFrame raw_f = resolved.get_frame(0, 0, 0);
	String sp_path = raw_f.sheet_path.is_empty() ? resolved.spritesheet_path : raw_f.sheet_path;
	if (sp_path.is_empty()) {
		sp_path = String(p_rec.get("graphic_path", ""));
	}
	if (sp_path.is_empty() || !FileAccess::exists(sp_path)) {
		return Ref<Texture2D>();
	}
	Ref<Texture2D> tex = ResourceLoader::load(sp_path, "Texture2D");
	if (tex.is_null()) {
		Ref<Image> img = Image::load_from_file(sp_path);
		if (img.is_valid() && !img->is_empty()) {
			tex = ImageTexture::create_from_image(img);
		}
	}
	if (tex.is_null() || tex->get_width() <= 0 || tex->get_height() <= 0) {
		return Ref<Texture2D>();
	}

	ClockGearsAnimationFrame f0 = resolved.get_frame(0, tex->get_width(), tex->get_height());
	Rect2i r = f0.region;
	if (r.size.x <= 0 || r.size.y <= 0) {
		int cols = MAX(1, resolved.cols > 0 ? resolved.cols : sheet.cols);
		int rows = MAX(1, resolved.rows > 0 ? resolved.rows : sheet.rows);
		r = Rect2i(0, 0, MAX(1, tex->get_width() / cols), MAX(1, tex->get_height() / rows));
	}
	r = r.intersection(Rect2i(0, 0, tex->get_width(), tex->get_height()));
	if (r.size.x <= 0 || r.size.y <= 0) {
		return tex;
	}
	Ref<AtlasTexture> at;
	at.instantiate();
	at->set_atlas(tex);
	at->set_region(Rect2(r));
	r_desc = vformat("%dx%d px (%s)", r.size.x, r.size.y, res_slot);
	return at;
}

void ClockGearsGenerateEntityDialog::_resolve_preview_sprite(int p_tab, const Dictionary &p_rec, Ref<Texture2D> &r_tex, String &r_detail) const {
	r_tex = Ref<Texture2D>();
	r_detail = CG_TR("Sin sprite asignado");

	if (p_tab == TAB_NPC || p_tab == TAB_ARPG_NPC) {
		String desc;
		r_tex = s_extract_sheet_first_frame(p_rec, desc);
		if (r_tex.is_valid()) {
			r_detail = desc;
		}
	} else if (p_tab == TAB_ARPG_ENEMIGO) {
		String desc;
		r_tex = s_extract_sheet_first_frame(p_rec, desc);
		if (r_tex.is_valid()) {
			r_detail = desc;
		} else {
			Ref<Texture2D> sp_tex;
			Rect2 sp_reg;
			String sp_src;
			ClockGearsBattleScene::resolve_enemy_preview_sprite(p_rec, sp_tex, sp_reg, sp_src);
			if (sp_src != "placeholder" && sp_tex.is_valid()) {
				if (sp_reg.size.x > 0 && sp_reg.size.y > 0) {
					Ref<AtlasTexture> at;
					at.instantiate();
					at->set_atlas(sp_tex);
					at->set_region(sp_reg);
					r_tex = at;
					r_detail = vformat("%dx%d px", int(sp_reg.size.x), int(sp_reg.size.y));
				} else {
					r_tex = sp_tex;
					r_detail = vformat("%dx%d px", sp_tex->get_width(), sp_tex->get_height());
				}
			}
		}
	} else if (p_tab == TAB_ENEMIGO) {
		Array mems = p_rec.get("members", Array());
		int mem_count = mems.size();
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (mem_count > 0 && dm && mems[0].get_type() == Variant::DICTIONARY) {
			int eid = int(Dictionary(mems[0]).get("enemy_id", 1));
			Dictionary ed = dm->get_record_by_id(CAT_ENEMIES, eid);
			if (!ed.is_empty()) {
				String ename = String(ed.get("name", ""));
				Ref<Texture2D> sp_tex;
				Rect2 sp_reg;
				String sp_src;
				ClockGearsBattleScene::resolve_enemy_preview_sprite(ed, sp_tex, sp_reg, sp_src);
				if (sp_src != "placeholder" && sp_tex.is_valid()) {
					if (sp_reg.size.x > 0 && sp_reg.size.y > 0) {
						Ref<AtlasTexture> at;
						at.instantiate();
						at->set_atlas(sp_tex);
						at->set_region(sp_reg);
						r_tex = at;
					} else {
						r_tex = sp_tex;
					}
				}
				r_detail = mem_count == 1
						? vformat(String::utf8("%s (1 enemigo · Contacto)"), ename)
						: vformat(String::utf8("%s (+%d más · Contacto)"), ename, mem_count - 1);
			} else {
				r_detail = vformat(CG_TR("Tropa (%d miembros)"), mem_count);
			}
		} else {
			r_detail = CG_TR("Tropa sin miembros (Contacto)");
		}
	}

	if (r_tex.is_null()) {
		r_tex = ClockGearsCharacterStateRegistry::get_missing_question_marker_texture();
	}
}

void ClockGearsGenerateEntityDialog::_populate_list_for_tab(int p_tab) {
	current_tab_idx = CLAMP(p_tab, 0, TAB_MAX - 1);
	current_records.clear();
	if (entity_list) {
		entity_list->clear();
	}

	static const char *tab_titles[TAB_MAX] = {
		"NPC", "ARPG NPC", "Tropas (Enemigo)", "Enemigo ARPG", "Flora", "Fauna"
	};

	int cat = _tab_to_category(current_tab_idx);
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (cat >= 0 && dm) {
		Array arr = dm->get_category_array(cat);
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary d = arr[i];
			int rid = int(d.get("id", i + 1));
			String rname = String(d.get("name", ""));
			current_records.push_back(d);
			if (entity_list) {
				String label = rname.is_empty() ? vformat("%03d: (Sin nombre)", rid) : vformat("%03d: %s", rid, rname);
				Ref<Texture2D> item_tex;
				String dummy_detail;
				_resolve_preview_sprite(current_tab_idx, d, item_tex, dummy_detail);
				entity_list->add_item(label, item_tex);
			}
		}
	}

	if (list_header_label) {
		list_header_label->set_text(vformat("%s (%d)", CG_TR(tab_titles[current_tab_idx]), current_records.size()));
	}

	bool has_items = !current_records.is_empty();
	if (empty_list_label) {
		empty_list_label->set_visible(!has_items);
	}
	if (has_items && entity_list) {
		entity_list->select(0);
		_update_preview_for_index(0);
	} else {
		_update_preview_for_index(-1);
	}
}

void ClockGearsGenerateEntityDialog::_update_preview_for_index(int p_list_idx) {
	bool valid = (p_list_idx >= 0 && p_list_idx < current_records.size() && current_tab_idx != TAB_FLORA && current_tab_idx != TAB_FAUNA);
	if (get_ok_button()) {
		get_ok_button()->set_disabled(!valid);
	}
	if (!valid) {
		if (preview_texture_rect) {
			preview_texture_rect->set_texture(Ref<Texture2D>());
		}
		if (preview_name_label) {
			preview_name_label->set_text(CG_TR("Sin selección"));
		}
		if (preview_detail_label) {
			preview_detail_label->set_text((current_tab_idx == TAB_FLORA || current_tab_idx == TAB_FAUNA)
							? CG_TR("Categoría sin entradas disponibles.")
							: CG_TR("No hay entradas en esta categoría."));
		}
		return;
	}

	Dictionary d = current_records[p_list_idx];
	int rid = int(d.get("id", p_list_idx + 1));
	String rname = String(d.get("name", ""));
	if (preview_name_label) {
		preview_name_label->set_text(rname.is_empty() ? vformat("%03d: (Sin nombre)", rid) : vformat("%03d: %s", rid, rname));
	}
	Ref<Texture2D> ptex;
	String pdetail;
	_resolve_preview_sprite(current_tab_idx, d, ptex, pdetail);
	if (preview_texture_rect) {
		preview_texture_rect->set_texture(ptex);
	}
	if (preview_detail_label) {
		preview_detail_label->set_text(pdetail);
	}
}

void ClockGearsGenerateEntityDialog::_on_tab_changed(int p_tab) {
	_populate_list_for_tab(p_tab);
}

void ClockGearsGenerateEntityDialog::_on_entity_selected(int p_idx) {
	_update_preview_for_index(p_idx);
}

void ClockGearsGenerateEntityDialog::_on_entity_activated(int p_idx) {
	if (p_idx < 0 || p_idx >= current_records.size() || current_tab_idx == TAB_FLORA || current_tab_idx == TAB_FAUNA) {
		return;
	}
	_on_confirmed_generate();
	hide();
}

void ClockGearsGenerateEntityDialog::_on_confirmed_generate() {
	if (!entity_list || current_tab_idx == TAB_FLORA || current_tab_idx == TAB_FAUNA) {
		return;
	}
	Vector<int> sel = entity_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	int idx = sel[0];
	if (idx < 0 || idx >= current_records.size()) {
		return;
	}
	Dictionary d = current_records[idx];
	int rid = int(d.get("id", idx + 1));
	String rname = String(d.get("name", ""));
	emit_signal("entity_chosen", current_tab_idx, rid, rname, target_cell.x, target_cell.y);
}

void ClockGearsGenerateEntityDialog::open_for_cell(int p_x, int p_y) {
	target_cell = Point2i(p_x, p_y);
	set_title(vformat(CG_TR("Generar entidad (%d, %d) - ClockGears Engine"), p_x, p_y));
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	if (entity_tabs) {
		entity_tabs->set_current_tab(current_tab_idx);
	}
	_populate_list_for_tab(current_tab_idx);
	Size2i desired(int(640 * EDSCALE), int(380 * EDSCALE));
	if (DisplayServer::get_singleton()) {
		int scr = DisplayServer::get_singleton()->window_get_current_screen();
		Rect2i usable = DisplayServer::get_singleton()->screen_get_usable_rect(scr);
		if (usable.size.x <= 0 || usable.size.y <= 0) {
			usable = Rect2i(Point2i(), DisplayServer::get_singleton()->screen_get_size(scr));
		}
		if (usable.size.x > 0 && usable.size.y > 0) {
			desired.x = MIN(desired.x, MAX(420, usable.size.x - 24));
			desired.y = MIN(desired.y, MAX(260, usable.size.y - 48));
		}
	}
	popup_centered(desired);
}

void ClockGearsGenerateEntityDialog::select_tab(int p_tab) {
	int clamped = CLAMP(p_tab, 0, TAB_MAX - 1);
	if (entity_tabs) {
		entity_tabs->set_current_tab(clamped);
	}
	_populate_list_for_tab(clamped);
}

int ClockGearsGenerateEntityDialog::get_listed_count() const {
	return entity_list ? entity_list->get_item_count() : 0;
}

void ClockGearsGenerateEntityDialog::select_list_index(int p_idx) {
	if (!entity_list || p_idx < 0 || p_idx >= entity_list->get_item_count()) {
		return;
	}
	entity_list->select(p_idx);
	_update_preview_for_index(p_idx);
}

Dictionary ClockGearsGenerateEntityDialog::check_layout_fits_window(const Vector2i &p_window_size) const {
	Dictionary res;
	Size2 min_sz = get_min_size();
	if (get_child_count() > 0) {
		Control *root_ctrl = Object::cast_to<Control>(get_child(0));
		if (root_ctrl) {
			min_sz = min_sz.max(root_ctrl->get_combined_minimum_size());
		}
	}
	res["window_size"] = p_window_size;
	res["min_w"] = int(min_sz.x);
	res["min_h"] = int(min_sz.y);
	res["fits_horizontally"] = (min_sz.x <= float(p_window_size.x));
	res["fits_vertically"] = (min_sz.y <= float(p_window_size.y));
	return res;
}

ClockGearsGenerateEntityDialog::ClockGearsGenerateEntityDialog() {
	set_title(CG_TR("Generar entidad - ClockGears Engine"));
	set_ok_button_text(CG_TR("Generar"));
	set_cancel_button_text(CG_TR("Cancelar"));
	set_min_size(Size2i(int(480 * EDSCALE), int(280 * EDSCALE)));

	MarginContainer *margin = memnew(MarginContainer);
	margin->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	margin->add_theme_constant_override("margin_left", int(10 * EDSCALE));
	margin->add_theme_constant_override("margin_right", int(10 * EDSCALE));
	margin->add_theme_constant_override("margin_top", int(8 * EDSCALE));
	margin->add_theme_constant_override("margin_bottom", int(8 * EDSCALE));
	add_child(margin);

	VBoxContainer *root_vbox = memnew(VBoxContainer);
	root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_vbox->add_theme_constant_override("separation", int(8 * EDSCALE));
	margin->add_child(root_vbox);

	entity_tabs = memnew(TabBar);
	entity_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	entity_tabs->set_tab_alignment(TabBar::ALIGNMENT_LEFT);
	entity_tabs->add_tab(CG_TR("NPC"));
	entity_tabs->add_tab(CG_TR("ARPG NPC"));
	entity_tabs->add_tab(CG_TR("ENEMIGO"));
	entity_tabs->add_tab(CG_TR("ARPG ENEMIGO"));
	entity_tabs->add_tab(CG_TR("FLORA"));
	entity_tabs->add_tab(CG_TR("FAUNA"));
	entity_tabs->connect("tab_changed", callable_mp(this, &ClockGearsGenerateEntityDialog::_on_tab_changed));
	root_vbox->add_child(entity_tabs);

	ScrollContainer *body_scroll = memnew(ScrollContainer);
	body_scroll->set_name("BodyScroll");
	body_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	body_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	body_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	body_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	root_vbox->add_child(body_scroll);

	HBoxContainer *split_hb = memnew(HBoxContainer);
	split_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	split_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	split_hb->add_theme_constant_override("separation", int(12 * EDSCALE));
	body_scroll->add_child(split_hb);

	VBoxContainer *left_col = memnew(VBoxContainer);
	left_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	left_col->set_stretch_ratio(1.05f);
	left_col->set_custom_minimum_size(Size2(int(200 * EDSCALE), int(150 * EDSCALE)));
	left_col->add_theme_constant_override("separation", int(4 * EDSCALE));
	split_hb->add_child(left_col);

	list_header_label = memnew(Label(CG_TR("NPC (0)")));
	list_header_label->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	left_col->add_child(list_header_label);
	HSeparator *sep_list = memnew(HSeparator);
	sep_list->set_theme_type_variation("ClockGearsSectionHeaderLine");
	left_col->add_child(sep_list);

	entity_list = memnew(ItemList);
	entity_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	entity_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	entity_list->set_fixed_icon_size(Size2i(int(20 * EDSCALE), int(20 * EDSCALE)));
	entity_list->add_theme_constant_override("v_separation", int(2 * EDSCALE));
	entity_list->set_allow_rmb_select(false);
	entity_list->connect("item_selected", callable_mp(this, &ClockGearsGenerateEntityDialog::_on_entity_selected));
	entity_list->connect("item_activated", callable_mp(this, &ClockGearsGenerateEntityDialog::_on_entity_activated));
	left_col->add_child(entity_list);

	empty_list_label = memnew(Label(CG_TR("No hay entradas en esta categoría.")));
	empty_list_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty_list_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	empty_list_label->hide();
	left_col->add_child(empty_list_label);

	VBoxContainer *right_col = memnew(VBoxContainer);
	right_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	right_col->set_stretch_ratio(0.95f);
	right_col->set_custom_minimum_size(Size2(int(200 * EDSCALE), int(150 * EDSCALE)));
	right_col->add_theme_constant_override("separation", int(6 * EDSCALE));
	split_hb->add_child(right_col);

	Label *lbl_prev = memnew(Label(CG_TR("Vista previa del sprite")));
	lbl_prev->add_theme_color_override("font_color", Color(0.88f, 0.65f, 0.25f));
	right_col->add_child(lbl_prev);
	HSeparator *sep_prev = memnew(HSeparator);
	sep_prev->set_theme_type_variation("ClockGearsSectionHeaderLine");
	right_col->add_child(sep_prev);

	preview_frame_panel = memnew(PanelContainer);
	preview_frame_panel->set_theme_type_variation("ClockGearsPreviewFrame");
	preview_frame_panel->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	preview_frame_panel->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	preview_frame_panel->set_custom_minimum_size(Size2(int(140 * EDSCALE), int(116 * EDSCALE)));
	right_col->add_child(preview_frame_panel);

	MarginContainer *prev_margin = memnew(MarginContainer);
	prev_margin->add_theme_constant_override("margin_left", int(6 * EDSCALE));
	prev_margin->add_theme_constant_override("margin_right", int(6 * EDSCALE));
	prev_margin->add_theme_constant_override("margin_top", int(6 * EDSCALE));
	prev_margin->add_theme_constant_override("margin_bottom", int(6 * EDSCALE));
	preview_frame_panel->add_child(prev_margin);

	preview_texture_rect = memnew(TextureRect);
	preview_texture_rect->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	preview_texture_rect->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	preview_texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	preview_texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	preview_texture_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	prev_margin->add_child(preview_texture_rect);

	preview_name_label = memnew(Label(CG_TR("Sin selección")));
	preview_name_label->set_clip_text(true);
	preview_name_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	right_col->add_child(preview_name_label);

	preview_detail_label = memnew(Label(""));
	preview_detail_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	preview_detail_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	preview_detail_label->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
	right_col->add_child(preview_detail_label);

	if (get_ok_button()) {
		get_ok_button()->set_theme_type_variation("ClockGearsPrimaryButton");
	}
	connect("confirmed", callable_mp(this, &ClockGearsGenerateEntityDialog::_on_confirmed_generate));
}

// ============================================================================
// ClockGearsLayersDialog (Etapa 1 - 9B-Ext: Capas con Nombre y Orden Entero)
// ============================================================================

// Tarjetas y botones de Capas heredan estilos ClockGearsContainer / ClockGearsPrimaryButton del tema.

String ClockGearsLayersDialog::_describe_order_band(int p_order) {
	if (p_order < 0) {
		return CG_TR("Debajo del Suelo (Z < 0)");
	}
	if (p_order == 0) {
		return CG_TR("Mismo plano que Capa 1: Suelo");
	}
	if (p_order == 1) {
		return CG_TR("Mismo plano que Capa 2: Decoración");
	}
	if (p_order == 2) {
		return CG_TR("Mismo plano que Capa 3: Objetos");
	}
	if (p_order == 3) {
		return CG_TR("Nivel Jugador y Eventos (Z = 3)");
	}
	if (p_order == 4) {
		return CG_TR("Mismo plano que Capa 4: Techo");
	}
	return CG_TR("Por encima de Capa 4: Techo (Z > 4)");
}

void ClockGearsLayersDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_layers_dialog"), &ClockGearsLayersDialog::open_layers_dialog);
	ClassDB::bind_method(D_METHOD("refresh_layers_list"), &ClockGearsLayersDialog::refresh_layers_list);
	ClassDB::bind_method(D_METHOD("create_layer_from_ui", "name", "order"), &ClockGearsLayersDialog::create_layer_from_ui, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("select_layer_from_ui", "id"), &ClockGearsLayersDialog::select_layer_from_ui);
	ClassDB::bind_method(D_METHOD("request_delete_layer", "id"), &ClockGearsLayersDialog::request_delete_layer);
	ClassDB::bind_method(D_METHOD("confirm_pending_delete"), &ClockGearsLayersDialog::confirm_pending_delete);
	ClassDB::bind_method(D_METHOD("get_row_count"), &ClockGearsLayersDialog::get_row_count);
	ClassDB::bind_method(D_METHOD("get_ui_handles"), &ClockGearsLayersDialog::get_ui_handles);
	ClassDB::bind_method(D_METHOD("check_layout_fits", "viewport_size"), &ClockGearsLayersDialog::check_layout_fits, DEFVAL(Vector2i(1366, 768)));
}

void ClockGearsLayersDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(this);
		}
		if (ClockGearsDataManager::get_singleton()) {
			Callable cb = callable_mp(this, &ClockGearsLayersDialog::_on_dm_data_changed);
			if (!ClockGearsDataManager::get_singleton()->is_connected("data_changed", cb)) {
				ClockGearsDataManager::get_singleton()->connect("data_changed", cb);
			}
		}
		refresh_layers_list();
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		if (ClockGearsDataManager::get_singleton()) {
			Callable cb = callable_mp(this, &ClockGearsLayersDialog::_on_dm_data_changed);
			if (ClockGearsDataManager::get_singleton()->is_connected("data_changed", cb)) {
				ClockGearsDataManager::get_singleton()->disconnect("data_changed", cb);
			}
		}
	}
}

void ClockGearsLayersDialog::_on_dm_data_changed(const String &p_category, int p_id) {
	if (rebuilding_rows) {
		return;
	}
	if ((p_category == "custom_layers" || p_category == "system" || p_category.is_empty()) && is_visible()) {
		refresh_layers_list();
	}
}

void ClockGearsLayersDialog::_on_fixed_layer_pressed(int p_base_layer) {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_editor_layer(p_base_layer);
	}
	refresh_layers_list();
}

void ClockGearsLayersDialog::_on_create_text_submitted(const String &p_text) {
	_on_create_pressed();
}

void ClockGearsLayersDialog::_on_create_pressed() {
	String nm = new_name_edit ? new_name_edit->get_text().strip_edges() : String();
	if (nm.is_empty()) {
		if (new_name_edit) {
			new_name_edit->grab_focus();
		}
		return;
	}
	int ord = new_order_spin ? int(new_order_spin->get_value()) : 2;
	create_layer_from_ui(nm, ord);
}

void ClockGearsLayersDialog::_on_row_select_pressed(const String &p_id) {
	select_layer_from_ui(p_id);
}

void ClockGearsLayersDialog::_on_row_name_committed(const String &p_text, const String &p_id, LineEdit *p_edit) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary cur = dm->get_custom_map_layer(p_id);
	String effective_text = (p_text.is_empty() && p_edit) ? p_edit->get_text() : p_text;
	String trimmed = effective_text.strip_edges();
	if (trimmed.is_empty()) {
		if (p_edit && !cur.is_empty()) {
			p_edit->set_text(String(cur.get("name", "")));
		}
		return;
	}
	if (!cur.is_empty() && String(cur.get("name", "")) != trimmed) {
		dm->rename_custom_map_layer(p_id, trimmed);
		callable_mp(this, &ClockGearsLayersDialog::refresh_layers_list).call_deferred();
	}
}

void ClockGearsLayersDialog::_on_row_order_changed(double p_val, const String &p_id) {
	if (rebuilding_rows) {
		return;
	}
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_custom_map_layer_order(p_id, int(Math::round(p_val)));
		callable_mp(this, &ClockGearsLayersDialog::refresh_layers_list).call_deferred();
	}
}

void ClockGearsLayersDialog::_on_row_visible_pressed(const String &p_id, bool p_next_vis) {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_custom_map_layer_visible(p_id, p_next_vis);
	}
	refresh_layers_list();
}

void ClockGearsLayersDialog::_on_row_locked_pressed(const String &p_id, bool p_next_locked) {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_custom_map_layer_locked(p_id, p_next_locked);
	}
	refresh_layers_list();
}

void ClockGearsLayersDialog::_on_row_duplicate_pressed(const String &p_id) {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		Dictionary created = dm->duplicate_custom_map_layer(p_id);
		if (!created.is_empty()) {
			dm->set_editor_custom_layer(String(created.get("id", "")));
		}
	}
	refresh_layers_list();
}

void ClockGearsLayersDialog::_on_row_delete_pressed(const String &p_id) {
	request_delete_layer(p_id);
}

void ClockGearsLayersDialog::_on_delete_confirmed() {
	confirm_pending_delete();
}

Dictionary ClockGearsLayersDialog::create_layer_from_ui(const String &p_name, int p_order) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String trimmed = p_name.strip_edges();
	if (!dm || trimmed.is_empty()) {
		return Dictionary();
	}
	if (new_name_edit) {
		new_name_edit->set_text(trimmed);
	}
	if (new_order_spin) {
		new_order_spin->set_value(CLAMP(p_order, -50, 50));
	}
	Dictionary created = dm->create_custom_map_layer(trimmed, p_order);
	if (!created.is_empty()) {
		dm->set_editor_custom_layer(String(created.get("id", "")));
		if (new_name_edit) {
			new_name_edit->set_text("");
		}
	}
	refresh_layers_list();
	return created;
}

bool ClockGearsLayersDialog::select_layer_from_ui(const String &p_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	bool ok = dm->set_editor_custom_layer(p_id);
	refresh_layers_list();
	return ok;
}

bool ClockGearsLayersDialog::request_delete_layer(const String &p_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !delete_confirm_dialog) {
		return false;
	}
	Dictionary layer = dm->get_custom_map_layer(p_id);
	if (layer.is_empty()) {
		return false;
	}
	pending_delete_id = p_id;
	String lname = String(layer.get("name", p_id));
	delete_confirm_dialog->set_text(vformat(
			CG_TR("¿Eliminar la capa personalizada «%s»?\n\nSus tiles en los mapas se quitarán y esta acción no se puede deshacer."),
			lname));
	delete_confirm_dialog->popup_centered(Size2(420, 170) * EDSCALE);
	return true;
}

bool ClockGearsLayersDialog::confirm_pending_delete() {
	if (pending_delete_id.is_empty()) {
		return false;
	}
	String del_id = pending_delete_id;
	pending_delete_id = String();
	if (delete_confirm_dialog && delete_confirm_dialog->is_visible()) {
		delete_confirm_dialog->hide();
	}
	bool ok = false;
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		ok = dm->delete_custom_map_layer(del_id);
	}
	refresh_layers_list();
	return ok;
}

int ClockGearsLayersDialog::get_row_count() const {
	if (!rows_vbox) {
		return 0;
	}
	int cnt = 0;
	for (int i = 0; i < rows_vbox->get_child_count(); i++) {
		if (rows_vbox->get_child(i)->has_meta("custom_layer_id")) {
			cnt++;
		}
	}
	return cnt;
}

Dictionary ClockGearsLayersDialog::get_ui_handles() const {
	Dictionary d;
	d["dialog"] = const_cast<ClockGearsLayersDialog *>(this);
	MenuBar *mbar = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_clockgears_main_menu_bar() : nullptr;
	if (!mbar) {
		HBoxContainer *row = EditorNode::create_clockgears_top_bar_row();
		d["title_bar_row"] = row;
		mbar = Object::cast_to<MenuBar>(row->get_node_or_null(NodePath("MainMenuBar")));
	} else if (mbar->get_parent()) {
		d["title_bar_row"] = mbar->get_parent();
	}
	d["main_menu_bar"] = mbar;
	d["active_indicator"] = Variant();
	d["count_label"] = count_label;
	d["scale_box"] = scale_buttons_box;
	d["create_name_edit"] = new_name_edit;
	d["create_order_spin"] = new_order_spin;
	d["create_btn"] = create_btn;
	d["list_scroll"] = list_scroll;
	d["rows_vbox"] = rows_vbox;
	d["delete_confirm_dialog"] = delete_confirm_dialog;
	Array sbtns;
	for (int i = 0; i < fixed_layer_buttons.size(); i++) {
		sbtns.push_back(fixed_layer_buttons[i]);
	}
	d["scale_buttons"] = sbtns;
	return d;
}

void ClockGearsLayersDialog::_update_responsive_size(const Vector2i &p_viewport_override) {
	Vector2i vp = p_viewport_override;
	if (vp.x <= 0 || vp.y <= 0) {
		if (Window *pw = (get_parent() ? get_parent()->get_window() : nullptr)) {
			vp = pw->get_size();
		} else {
			vp = Vector2i(1366, 768);
		}
	}
	int max_w = MAX(420, vp.x - 40);
	int max_h = MAX(320, vp.y - 40);
	int target_w = MIN(int(Math::round(820.0 * EDSCALE)), max_w);
	int target_h = MIN(int(Math::round(620.0 * EDSCALE)), max_h);
	set_min_size(Size2i(MIN(480, max_w), MIN(340, max_h)));
	set_size(Size2i(target_w, target_h));
}

Dictionary ClockGearsLayersDialog::check_layout_fits(const Vector2i &p_viewport_size) {
	refresh_layers_list();
	_update_responsive_size(p_viewport_size);
	Size2i dlg_sz = get_size();
	Size2 min_sz = get_contents_minimum_size();
	bool h_scroll_disabled = list_scroll && list_scroll->get_horizontal_scroll_mode() == ScrollContainer::SCROLL_MODE_DISABLED;
	float rows_min_w = rows_vbox ? rows_vbox->get_combined_minimum_size().x : 0.0f;
	float max_row_allowance = float(dlg_sz.x - 44);
	bool rows_fit_width = rows_min_w <= max_row_allowance;
	bool fits_viewport = (dlg_sz.x <= p_viewport_size.x) && (dlg_sz.y <= p_viewport_size.y) && (min_sz.x <= dlg_sz.x) && (min_sz.y <= dlg_sz.y) && h_scroll_disabled && rows_fit_width;

	Dictionary res;
	res["fits"] = fits_viewport;
	res["viewport_size"] = p_viewport_size;
	res["dialog_size"] = dlg_sz;
	res["contents_min_size"] = min_sz;
	res["rows_min_width"] = rows_min_w;
	res["max_row_allowance"] = max_row_allowance;
	res["horizontal_scroll_disabled"] = h_scroll_disabled;
	res["rows_fit_width"] = rows_fit_width;
	return res;
}

void ClockGearsLayersDialog::refresh_layers_list() {
	if (!rows_vbox) {
		return;
	}
	rebuilding_rows = true;

	while (rows_vbox->get_child_count() > 0) {
		Node *ch = rows_vbox->get_child(0);
		rows_vbox->remove_child(ch);
		memdelete(ch);
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array raw_layers = dm ? dm->get_custom_map_layers() : Array();
	String active_custom_id = dm ? dm->get_editor_custom_layer() : String();
	int active_base_idx = dm ? dm->get_editor_layer() : 1;

	EditorNode::sync_clockgears_active_layer_indicator_all();
	if (count_label) {
		count_label->set_text(vformat(CG_TR("Capas personalizadas: %d / %d"), raw_layers.size(), ClockGearsDataManager::MAX_CUSTOM_MAP_LAYERS));
	}
	if (create_btn) {
		create_btn->set_disabled(raw_layers.size() >= ClockGearsDataManager::MAX_CUSTOM_MAP_LAYERS);
	}

	// Resaltar botón de capa base en la barra de escala si ninguna capa personalizada está activa
	for (int i = 0; i < fixed_layer_buttons.size(); i++) {
		Button *b = fixed_layer_buttons[i];
		if (!b) {
			continue;
		}
		int base_id = int(b->get_meta("fixed_base_layer", 0));
		bool is_cur = active_custom_id.is_empty() && (base_id == active_base_idx);
		b->set_pressed_no_signal(is_cur);
	}

	if (raw_layers.is_empty()) {
		Label *empty_lbl = memnew(Label);
		empty_lbl->set_text(CG_TR("Aún no has creado capas personalizadas.\nEscribe un nombre libre y un orden entero Z arriba para crear hasta 16 capas."));
		empty_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		empty_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		empty_lbl->set_custom_minimum_size(Size2(360, 0) * EDSCALE);
		empty_lbl->add_theme_color_override("font_color", Color(0.68f, 0.72f, 0.78f));
		rows_vbox->add_child(empty_lbl);
		rebuilding_rows = false;
		return;
	}

	// Mostrar ordenadas de mayor Z (arriba) a menor Z (abajo), con orden de proyecto en empate
	struct SortedCustom {
		Dictionary data;
		int order = 0;
		int idx = 0;
	};
	Vector<SortedCustom> sorted;
	for (int i = 0; i < raw_layers.size(); i++) {
		if (raw_layers[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary d = raw_layers[i];
		SortedCustom sc;
		sc.data = d;
		sc.order = int(d.get("order", 2));
		sc.idx = i;
		sorted.push_back(sc);
	}
	for (int i = 1; i < sorted.size(); i++) {
		SortedCustom key = sorted[i];
		int j = i - 1;
		while (j >= 0 && (sorted[j].order < key.order || (sorted[j].order == key.order && sorted[j].idx < key.idx))) {
			sorted.write[j + 1] = sorted[j];
			j--;
		}
		sorted.write[j + 1] = key;
	}

	const int max_custom = ClockGearsDataManager::MAX_CUSTOM_MAP_LAYERS;
	const bool can_duplicate = raw_layers.size() < max_custom;

	for (int i = 0; i < sorted.size(); i++) {
		Dictionary layer = sorted[i].data;
		String lid = String(layer.get("id", ""));
		String lname = String(layer.get("name", lid));
		int lorder = int(layer.get("order", 2));
		bool lvis = bool(layer.get("visible", true));
		bool llck = bool(layer.get("locked", false));
		bool is_active = (lid == active_custom_id);

		PanelContainer *row_card = memnew(PanelContainer);
		row_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row_card->set_meta("custom_layer_id", lid);
		if (is_active) {
			const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
			const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
			const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, EDSCALE);
			Ref<ClockGearsBrassStyleBox> act_sb;
			act_sb.instantiate();
			act_sb->set_anti_aliased(true);
			act_sb->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, 0.16f));
			act_sb->set_border_width_all(2);
			act_sb->set_border_color(tk.accent_neon);
			act_sb->set_corner_radius_all(int(6 * EDSCALE));
			act_sb->set_content_margin_individual(int(10 * EDSCALE), int(8 * EDSCALE), int(10 * EDSCALE), int(8 * EDSCALE));
			act_sb->set_led_dot(true, MAX(3, int(4 * EDSCALE)), tk.accent_neon);
			act_sb->set_shadow_color(tk.halo_color);
			act_sb->set_shadow_size(tk.halo_radius_px);
			row_card->add_theme_style_override("panel", act_sb);
		} else {
			row_card->set_theme_type_variation("ClockGearsContainer");
		}
		rows_vbox->add_child(row_card);

		VBoxContainer *row_vb = memnew(VBoxContainer);
		row_vb->add_theme_constant_override("separation", 6);
		row_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row_card->add_child(row_vb);

		// Línea 1: botón Activa + Nombre UTF-8 editable + Posición en escala
		HBoxContainer *line1 = memnew(HBoxContainer);
		line1->add_theme_constant_override("separation", 8);
		line1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row_vb->add_child(line1);

		Button *sel_btn = memnew(Button);
		sel_btn->set_name("SelectLayerBtn");
		sel_btn->set_text(is_active ? String::utf8("Activa") : CG_TR("Usar"));
		sel_btn->set_tooltip_text(CG_TR("Usar esta capa como capa actual para dibujar en el mapa"));
		sel_btn->set_custom_minimum_size(Size2(86, 28) * EDSCALE);
		sel_btn->set_focus_mode(Control::FOCUS_NONE);
		sel_btn->set_theme_type_variation(is_active ? StringName("ClockGearsPrimaryButton") : StringName("Button"));
		sel_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_row_select_pressed).bind(lid));
		line1->add_child(sel_btn);

		LineEdit *name_edit = memnew(LineEdit);
		name_edit->set_name("LayerNameEdit");
		name_edit->set_text(lname);
		name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		name_edit->set_custom_minimum_size(Size2(140, 28) * EDSCALE);
		name_edit->set_tooltip_text(CG_TR("Nombre libre en UTF-8 (pulsa Enter o sal del campo para renombrar)"));
		name_edit->connect("text_submitted", callable_mp(this, &ClockGearsLayersDialog::_on_row_name_committed).bind(lid, name_edit));
		name_edit->connect("focus_exited", callable_mp(this, &ClockGearsLayersDialog::_on_row_name_committed).bind(String(), lid, name_edit));
		// Nota: focus_exited pasa 0 args; conectamos a lambda/wrapper via helper? ¡Mejor lambda o un pequeño ajuste en _on_row_name_committed leyendo p_edit->get_text()!
		line1->add_child(name_edit);

		Label *band_lbl = memnew(Label);
		band_lbl->set_text(_describe_order_band(lorder));
		band_lbl->add_theme_color_override("font_color", Color(0.75f, 0.79f, 0.86f));
		line1->add_child(band_lbl);

		// Línea 2: Orden Z + acciones (Visible / Bloqueada / Duplicar / Eliminar)
		HBoxContainer *line2 = memnew(HBoxContainer);
		line2->add_theme_constant_override("separation", 6);
		line2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		row_vb->add_child(line2);

		Label *ord_lbl = memnew(Label(CG_TR("Orden Z:")));
		line2->add_child(ord_lbl);

		SpinBox *ord_spin = memnew(SpinBox);
		ord_spin->set_name("LayerOrderSpin");
		ord_spin->set_min(-50);
		ord_spin->set_max(50);
		ord_spin->set_step(1);
		ord_spin->set_value(lorder);
		ord_spin->set_custom_minimum_size(Size2(78, 26) * EDSCALE);
		ord_spin->set_tooltip_text(CG_TR("Menor = se dibuja más abajo; mayor = se dibuja más arriba"));
		ord_spin->connect("value_changed", callable_mp(this, &ClockGearsLayersDialog::_on_row_order_changed).bind(lid));
		line2->add_child(ord_spin);

		Control *spacer = memnew(Control);
		spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		line2->add_child(spacer);

		Button *vis_btn = memnew(Button);
		vis_btn->set_name("LayerVisibleBtn");
		vis_btn->set_text(lvis ? CG_TR("Visible") : CG_TR("Oculta"));
		vis_btn->set_tooltip_text(lvis ? CG_TR("Ocultar esta capa en el editor y juego") : CG_TR("Mostrar esta capa en el editor y juego"));
		vis_btn->set_focus_mode(Control::FOCUS_NONE);
		vis_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_row_visible_pressed).bind(lid, !lvis));
		line2->add_child(vis_btn);

		Button *lck_btn = memnew(Button);
		lck_btn->set_name("LayerLockBtn");
		lck_btn->set_text(llck ? CG_TR("Bloqueada") : CG_TR("Libre"));
		lck_btn->set_tooltip_text(llck ? CG_TR("Desbloquear capa para permitir edición") : CG_TR("Bloquear capa para evitar cambios accidentales"));
		lck_btn->set_focus_mode(Control::FOCUS_NONE);
		lck_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_row_locked_pressed).bind(lid, !llck));
		line2->add_child(lck_btn);

		Button *dup_btn = memnew(Button);
		dup_btn->set_name("LayerDuplicateBtn");
		dup_btn->set_text(CG_TR("Duplicar"));
		dup_btn->set_tooltip_text(CG_TR("Duplicar esta capa con sus tiles"));
		dup_btn->set_disabled(!can_duplicate);
		dup_btn->set_focus_mode(Control::FOCUS_NONE);
		dup_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_row_duplicate_pressed).bind(lid));
		line2->add_child(dup_btn);

		Button *del_btn = memnew(Button);
		del_btn->set_name("LayerDeleteBtn");
		del_btn->set_text(CG_TR("Eliminar"));
		del_btn->set_tooltip_text(CG_TR("Eliminar esta capa"));
		del_btn->set_focus_mode(Control::FOCUS_NONE);
		del_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_row_delete_pressed).bind(lid));
		line2->add_child(del_btn);
	}

	rebuilding_rows = false;
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
}

void ClockGearsLayersDialog::open_layers_dialog() {
	refresh_layers_list();
	_update_responsive_size();
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	popup_centered(get_size());
}

ClockGearsLayersDialog::ClockGearsLayersDialog() {
	set_title(CG_TR("Capas Personalizadas del Mapa - ClockGears Engine"));
	set_ok_button_text(CG_TR("Cerrar"));
	if (get_ok_button()) {
		get_ok_button()->set_theme_type_variation("ClockGearsPrimaryButton");
	}
	set_wrap_controls(false);
	set_min_size(Size2(520, 380) * EDSCALE);

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 10);
	add_child(main_vb);

	// Cabecera con contador de capas personalizadas (el indicador de capa actual está únicamente arriba en la barra superior)
	PanelContainer *hdr_card = memnew(PanelContainer);
	hdr_card->set_theme_type_variation("ClockGearsContainer");
	main_vb->add_child(hdr_card);

	HBoxContainer *hdr_hb = memnew(HBoxContainer);
	hdr_hb->add_theme_constant_override("separation", 8);
	hdr_card->add_child(hdr_hb);

	count_label = memnew(Label(CG_TR("Capas personalizadas: 0 / 16")));
	hdr_hb->add_child(count_label);

	// Tarjeta de escala de referencia (capas 1 a 4 + jugador y eventos)
	PanelContainer *scale_card = memnew(PanelContainer);
	scale_card->set_theme_type_variation("ClockGearsContainer");
	main_vb->add_child(scale_card);

	VBoxContainer *scale_vb = memnew(VBoxContainer);
	scale_vb->add_theme_constant_override("separation", 6);
	scale_card->add_child(scale_vb);

	Label *scale_title = memnew(Label(CG_TR("Escala Z de dibujo (menor = más abajo, mayor = más arriba • clic en 1–4 para usar esa capa):")));
	scale_title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	scale_title->set_custom_minimum_size(Size2(380, 0) * EDSCALE);
	scale_vb->add_child(scale_title);

	scale_buttons_box = memnew(HFlowContainer);
	scale_buttons_box->set_custom_minimum_size(Size2(380, 0) * EDSCALE);
	scale_buttons_box->add_theme_constant_override("h_separation", 6);
	scale_buttons_box->add_theme_constant_override("v_separation", 6);
	scale_vb->add_child(scale_buttons_box);

	struct ScaleChip {
		int z_ord;
		int base_idx;
		const char *label;
	};
	static const ScaleChip chips[5] = {
		{ 0, 1, "0: Capa 1 (Suelo)" },
		{ 1, 2, "1: Capa 2 (Decoración)" },
		{ 2, 3, "2: Capa 3 (Objetos)" },
		{ 3, 0, "3: Jugador y Eventos" },
		{ 4, 4, "4: Capa 4 (Techo ★=5)" },
	};
	for (int i = 0; i < 5; i++) {
		Button *chip = memnew(Button);
		chip->set_text(String::utf8(chips[i].label));
		chip->set_focus_mode(Control::FOCUS_NONE);
		chip->set_theme_type_variation("ClockGearsTabButton");
		if (chips[i].base_idx > 0) {
			chip->set_toggle_mode(true);
			chip->set_meta("fixed_base_layer", chips[i].base_idx);
			chip->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_fixed_layer_pressed).bind(chips[i].base_idx));
			fixed_layer_buttons.push_back(chip);
		} else {
			chip->set_disabled(true);
			chip->set_tooltip_text(CG_TR("Plano donde se dibujan el Jugador y los Eventos (Z = 3)"));
		}
		scale_buttons_box->add_child(chip);
	}

	// Tarjeta para crear nueva capa personalizada
	PanelContainer *create_card = memnew(PanelContainer);
	create_card->set_theme_type_variation("ClockGearsContainer");
	main_vb->add_child(create_card);

	HBoxContainer *create_hb = memnew(HBoxContainer);
	create_hb->add_theme_constant_override("separation", 8);
	create_card->add_child(create_hb);

	new_name_edit = memnew(LineEdit);
	new_name_edit->set_placeholder(CG_TR("Nombre libre de la capa (ej.: Niebla, Puentes, Techo alto…)"));
	new_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	new_name_edit->set_custom_minimum_size(Size2(150, 30) * EDSCALE);
	new_name_edit->connect("text_submitted", callable_mp(this, &ClockGearsLayersDialog::_on_create_text_submitted));
	create_hb->add_child(new_name_edit);

	create_hb->add_child(memnew(Label(CG_TR("Orden Z:"))));

	new_order_spin = memnew(SpinBox);
	new_order_spin->set_min(-50);
	new_order_spin->set_max(50);
	new_order_spin->set_step(1);
	new_order_spin->set_value(2);
	new_order_spin->set_custom_minimum_size(Size2(76, 30) * EDSCALE);
	create_hb->add_child(new_order_spin);

	create_btn = memnew(Button(CG_TR("+ Crear Capa")));
	create_btn->set_focus_mode(Control::FOCUS_NONE);
	create_btn->set_custom_minimum_size(Size2(110, 30) * EDSCALE);
	create_btn->set_theme_type_variation("ClockGearsPrimaryButton");
	create_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsLayersDialog::_on_create_pressed));
	create_hb->add_child(create_btn);

	// Lista desplazable verticalmente (nunca scroll horizontal)
	list_scroll = memnew(ScrollContainer);
	list_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	list_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	list_scroll->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	list_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	list_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	main_vb->add_child(list_scroll);

	rows_vbox = memnew(VBoxContainer);
	rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rows_vbox->add_theme_constant_override("separation", 8);
	list_scroll->add_child(rows_vbox);

	delete_confirm_dialog = memnew(ConfirmationDialog);
	delete_confirm_dialog->set_title(CG_TR("Confirmar eliminación de capa"));
	delete_confirm_dialog->set_ok_button_text(CG_TR("Eliminar"));
	delete_confirm_dialog->set_cancel_button_text(CG_TR("Cancelar"));
	delete_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsLayersDialog::_on_delete_confirmed));
	add_child(delete_confirm_dialog);
}

// ============================================================================
// ClockGearsEditorPlugin
// ============================================================================

ClockGearsEditorPlugin *ClockGearsEditorPlugin::singleton = nullptr;

void ClockGearsEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_database"), &ClockGearsEditorPlugin::open_database);
	ClassDB::bind_method(D_METHOD("open_resource_manager"), &ClockGearsEditorPlugin::open_resource_manager);
	ClassDB::bind_method(D_METHOD("open_new_project"), &ClockGearsEditorPlugin::open_new_project);
	ClassDB::bind_method(D_METHOD("open_export"), &ClockGearsEditorPlugin::open_export);
	ClassDB::bind_method(D_METHOD("open_preferences"), &ClockGearsEditorPlugin::open_preferences);
	ClassDB::bind_method(D_METHOD("open_camera_dialog"), &ClockGearsEditorPlugin::open_camera_dialog);
	ClassDB::bind_method(D_METHOD("open_layers_dialog"), &ClockGearsEditorPlugin::open_layers_dialog);
	ClassDB::bind_method(D_METHOD("get_layers_dialog"), &ClockGearsEditorPlugin::get_layers_dialog);
	ClassDB::bind_method(D_METHOD("validate_playtest_conditions"), &ClockGearsEditorPlugin::validate_playtest_conditions);
	ClassDB::bind_method(D_METHOD("start_playtest"), &ClockGearsEditorPlugin::start_playtest);
	ClassDB::bind_method(D_METHOD("open_event_editor_at_cell", "x", "y"), &ClockGearsEditorPlugin::open_event_editor_at_cell);
	ClassDB::bind_method(D_METHOD("open_generate_entity_dialog_at_cell", "x", "y"), &ClockGearsEditorPlugin::open_generate_entity_dialog_at_cell);
	ClassDB::bind_method(D_METHOD("generate_entity_at_cell", "tab", "record_id", "x", "y", "use_undo_redo"), &ClockGearsEditorPlugin::generate_entity_at_cell, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("show_event_context_menu", "x", "y", "screen_pos"), &ClockGearsEditorPlugin::show_event_context_menu);
	ClassDB::bind_method(D_METHOD("get_event_context_menu"), &ClockGearsEditorPlugin::get_event_context_menu);
	ClassDB::bind_method(D_METHOD("get_generate_entity_dialog"), &ClockGearsEditorPlugin::get_generate_entity_dialog);
	ClassDB::bind_method(D_METHOD("open_quick_door_dialog", "cell"), &ClockGearsEditorPlugin::open_quick_door_dialog);
	ClassDB::bind_method(D_METHOD("create_quick_door_event", "cell", "map", "x", "y", "dir", "sprite_path", "hframes"), &ClockGearsEditorPlugin::create_quick_door_event, DEFVAL("Abajo"), DEFVAL(""), DEFVAL(1));
	ClassDB::bind_method(D_METHOD("get_quick_door_dialog"), &ClockGearsEditorPlugin::get_quick_door_dialog);
}

void ClockGearsExportPlugin::_export_file(const String &p_path, const String &p_type, const HashSet<String> &p_features) {
	if (p_path != "res://clockgears_data/system.json" && !p_path.ends_with("/clockgears_data/system.json")) {
		return;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	if (fa.is_null()) {
		return;
	}
	String txt = fa->get_as_utf8_string();
	Variant parsed = JSON::parse_string(txt);
	if (parsed.get_type() != Variant::DICTIONARY) {
		return;
	}
	Dictionary d = parsed;
	if (!d.has("mannequin_size")) {
		return;
	}
	d.erase("mannequin_size");
	CharString utf8 = JSON::stringify(d, "\t").utf8();
	Vector<uint8_t> bytes;
	bytes.resize(utf8.length());
	if (utf8.length() > 0) {
		memcpy(bytes.ptrw(), utf8.get_data(), utf8.length());
	}
	add_file(p_path, bytes, false);
	skip();
}

void ClockGearsEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			singleton = this;
			set_process_shortcut_input(true);
			if (cg_export_plugin.is_null()) {
				cg_export_plugin.instantiate();
			}
			add_export_plugin(cg_export_plugin);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (cg_export_plugin.is_valid()) {
				remove_export_plugin(cg_export_plugin);
			}
		} break;
		case NOTIFICATION_READY: {
			if (ClockGearsDataManager::get_singleton() && FileAccess::exists("res://clockgears_data/actors.json")) {
				ClockGearsDataManager::get_singleton()->load_all("res://clockgears_data");
			}
		} break;
	}
}

void ClockGearsEditorPlugin::shortcut_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo()) {
		return;
	}
	// Atajo global F9: abrir Base de Datos (Sección 11.1)
	if (k->get_keycode() == Key::F9 && !k->is_ctrl_pressed() && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		open_database();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	} else if (k->get_keycode() == Key::F10 && !k->is_ctrl_pressed() && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		open_resource_manager();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	} else if ((k->get_keycode() == Key::F5 || k->get_keycode() == Key::F12) && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		start_playtest();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	}
}

void ClockGearsEditorPlugin::save_external_data() {
	// Al guardar proyecto/escena (Ctrl+S), persistir también la Base de Datos de ClockGears (Sección 12)
	if (ClockGearsDataManager::get_singleton() && ClockGearsDataManager::get_singleton()->is_dirty()) {
		ClockGearsDataManager::get_singleton()->save_all("res://clockgears_data");
	}
}

void ClockGearsEditorPlugin::make_visible(bool p_visible) {
	if (automation_panel) {
		automation_panel->set_visible(p_visible);
	}
}

Node *ClockGearsEditorPlugin::ensure_active_map() {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (scene) {
		return scene;
	}

	// 1. If maps exist in res://maps/, load the first one
	if (DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			da->list_dir_begin();
			String f = da->get_next();
			Vector<String> maps;
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
			maps.sort();
			if (!maps.is_empty()) {
				EditorNode::get_singleton()->open_scene(maps[0]);
				if (map_panel) {
					map_panel->refresh_maps_from_disk();
				}
				return EditorNode::get_singleton()->get_edited_scene();
			}
		}
	}

	// 2. No map exists: automatically create default Map 001
	DirAccess::make_dir_recursive_absolute("res://maps");
	String map_path = "res://maps/map001.tscn";

	ClockGearsMap *map = memnew(ClockGearsMap);
	map->set_name("Map001");
	map->set_map_name("Mapa001");
	map->set_map_size(Vector2i(25, 20));
	map->ensure_default_layers();

	for (int i = 0; i < map->get_child_count(); i++) {
		Node *c = map->get_child(i);
		if (c) {
			c->set_owner(map);
		}
	}

	Ref<PackedScene> packed = memnew(PackedScene);
	packed->pack(map);
	ResourceSaver::save(packed, map_path);
	memdelete(map);

	ClockGearsTitleScreen::ensure_title_screen_scene("res://title_screen.tscn");
	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", "res://title_screen.tscn");
		ProjectSettings::get_singleton()->save();
	}

	EditorNode::get_singleton()->open_scene(map_path);

	if (map_panel) {
		map_panel->refresh_maps_from_disk();
	}

	return EditorNode::get_singleton()->get_edited_scene();
}

ClockGearsEvent *ClockGearsEditorPlugin::get_event_at_cell(int p_x, int p_y) const {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!scene) {
		return nullptr;
	}
	int grid_size = 32;
	if (CanvasItemEditor::get_singleton()) {
		grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
	}
	TypedArray<Node> events = scene->find_children("*", "ClockGearsEvent", true, false);
	for (int i = 0; i < events.size(); i++) {
		ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(events[i]);
		if (ev) {
			int ev_cx = Math::floor(ev->get_position().x / (real_t)grid_size);
			int ev_cy = Math::floor(ev->get_position().y / (real_t)grid_size);
			if (ev_cx == p_x && ev_cy == p_y) {
				return ev;
			}
		}
	}
	return nullptr;
}

void ClockGearsEditorPlugin::open_event_editor(ClockGearsEvent *p_ev) {
	if (event_editor_dialog) {
		event_editor_dialog->edit_event(p_ev);
	}
}

void ClockGearsEditorPlugin::open_event_editor_at_cell(int p_x, int p_y) {
	if (!event_editor_dialog) {
		return;
	}

	Node *scene = ensure_active_map();
	if (!scene) {
		ERR_PRINT("ClockGears: No se pudo obtener ni crear un mapa activo para el evento.");
		return;
	}

	ClockGearsEvent *found_ev = get_event_at_cell(p_x, p_y);
	if (!found_ev) {
		found_ev = memnew(ClockGearsEvent);
		int grid_size = 32;
		if (CanvasItemEditor::get_singleton()) {
			grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
		}
		found_ev->set_position(Vector2(p_x * grid_size, p_y * grid_size));
		found_ev->set_event_name(vformat("EV_%03d_%03d", p_x, p_y));
		found_ev->set_name(found_ev->get_event_name());
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
		if (map && map->get_events_container()) {
			map->get_events_container()->add_child(found_ev);
		} else {
			scene->add_child(found_ev);
		}
		found_ev->set_owner(scene);
	}

	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(event_editor_dialog);
	}
	event_editor_dialog->edit_event(found_ev);
}

void ClockGearsGenerateEntityDialog::populate_event_context_menu(Node *p_menu, Node *p_quick_menu, bool p_has_event, bool p_clipboard_empty) {
	PopupMenu *menu = Object::cast_to<PopupMenu>(p_menu);
	if (!menu) {
		return;
	}
	PopupMenu *quick_menu = Object::cast_to<PopupMenu>(p_quick_menu);

	// Keep IDs synchronized with ClockGearsEditorPlugin context menu enums.
	constexpr int ID_EDIT_OR_CREATE = 0;
	constexpr int ID_COPY = 1;
	constexpr int ID_PASTE = 2;
	constexpr int ID_DELETE = 3;
	constexpr int ID_SET_START_POS = 4;
	constexpr int ID_GENERATE_ENTITY = 5;
	constexpr int ID_QUICK_EVENTS_SUBMENU = 100;
	constexpr int ID_QUICK_TRANSFER = 10;
	constexpr int ID_QUICK_INN = 11;
	constexpr int ID_QUICK_CHEST = 12;
	constexpr int ID_QUICK_DOOR = 13;
	constexpr int ID_QUICK_SAVE = 14;

	menu->clear(false);
	if (p_has_event) {
		menu->add_item(CG_TR("Editar Evento..."), ID_EDIT_OR_CREATE);
	} else {
		menu->add_item(CG_TR("Crear Nuevo Evento..."), ID_EDIT_OR_CREATE);
		if (quick_menu) {
			quick_menu->clear();
			quick_menu->add_item(CG_TR("Cofre del Tesoro..."), ID_QUICK_CHEST);
			quick_menu->add_item(CG_TR("Puerta..."), ID_QUICK_DOOR);
			quick_menu->add_item(CG_TR("Transferir..."), ID_QUICK_TRANSFER);
			quick_menu->add_item(CG_TR("Posada..."), ID_QUICK_INN);
			quick_menu->add_item(CG_TR("Punto de Guardado..."), ID_QUICK_SAVE);
			menu->add_submenu_node_item(CG_TR("Creación Rápida de Eventos"), quick_menu, ID_QUICK_EVENTS_SUBMENU);
			int q_idx = menu->get_item_index(ID_QUICK_EVENTS_SUBMENU);
			if (q_idx >= 0) {
				menu->set_item_disabled(q_idx, false);
			}
		}
		menu->add_item(CG_TR("Generar entidad..."), ID_GENERATE_ENTITY);
	}
	menu->add_separator();
	menu->add_item(CG_TR("Fijar Posición Inicial del Jugador aquí"), ID_SET_START_POS);
	menu->add_separator();
	menu->add_item(CG_TR("Copiar"), ID_COPY);
	int copy_idx = menu->get_item_index(ID_COPY);
	if (copy_idx >= 0) {
		menu->set_item_disabled(copy_idx, !p_has_event);
	}

	menu->add_item(CG_TR("Pegar"), ID_PASTE);
	int paste_idx = menu->get_item_index(ID_PASTE);
	if (paste_idx >= 0) {
		menu->set_item_disabled(paste_idx, p_clipboard_empty);
	}

	menu->add_separator();
	menu->add_item(CG_TR("Eliminar"), ID_DELETE);
	int del_idx = menu->get_item_index(ID_DELETE);
	if (del_idx >= 0) {
		menu->set_item_disabled(del_idx, !p_has_event);
	}
}

void ClockGearsEditorPlugin::show_event_context_menu(int p_x, int p_y, const Vector2 &p_screen_pos) {
	if (!event_context_menu) {
		return;
	}
	context_menu_cell = Point2i(p_x, p_y);
	ClockGearsEvent *ev = get_event_at_cell(p_x, p_y);

	ClockGearsGenerateEntityDialog::populate_event_context_menu(event_context_menu, quick_events_menu, ev != nullptr, clipboard_event_data.is_empty());

	event_context_menu->set_position(p_screen_pos);
	event_context_menu->reset_size();
	event_context_menu->popup();
}

void ClockGearsEditorPlugin::_on_event_context_menu_id_pressed(int p_id) {
	switch (p_id) {
		case ACTION_EDIT_OR_CREATE: { // Crear / Editar
			open_event_editor_at_cell(context_menu_cell.x, context_menu_cell.y);
		} break;

		case ACTION_SET_START_POS: {
			Node *scene = ensure_active_map();
			ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
			if (map) {
				map->set_player_start_position(Vector2i(context_menu_cell.x, context_menu_cell.y), 2);
				if (CanvasItemEditor::get_singleton()) {
					CanvasItemEditor::get_singleton()->update_viewport();
				}
			}
		} break;

		case ACTION_QUICK_EVENTS_SUBMENU: {
			if (quick_events_menu) {
				Point2 mpos = event_context_menu->get_position();
				Size2 msize = event_context_menu->get_size();
				quick_events_menu->set_position(mpos + Point2(msize.x - 4, 28));
				quick_events_menu->reset_size();
				quick_events_menu->popup();
			}
		} break;

		case ACTION_GENERATE_ENTITY: {
			open_generate_entity_dialog_at_cell(context_menu_cell.x, context_menu_cell.y);
		} break;

		case ACTION_COPY: { // Copiar
			ClockGearsEvent *ev = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
			if (ev) {
				clipboard_event_data["name"] = ev->get_event_name();
				clipboard_event_data["trigger"] = ev->get_trigger_type();
				clipboard_event_data["move_type"] = ev->get_move_type();
				clipboard_event_data["speed"] = ev->get_move_speed();
				clipboard_event_data["freq"] = ev->get_move_frequency();
				clipboard_event_data["through"] = ev->is_through();
				clipboard_event_data["direction_fix"] = ev->is_direction_fix();
				clipboard_event_data["direction"] = ev->get_direction();
				clipboard_event_data["priority"] = ev->get_priority_type();
				clipboard_event_data["graphic_path"] = ev->get_graphic_path();
				clipboard_event_data["hframes"] = ev->get_sprite_hframes();
				clipboard_event_data["vframes"] = ev->get_sprite_vframes();
				clipboard_event_data["frame"] = ev->get_sprite_frame();
				clipboard_event_data["commands"] = ev->get_commands().duplicate(true);
				clipboard_event_data["pages"] = ev->get_pages().duplicate(true);
				clipboard_event_data["sw1_en"] = ev->is_condition_switch_1_enabled();
				clipboard_event_data["sw1_name"] = ev->get_condition_switch_1_name();
				clipboard_event_data["sw2_en"] = ev->is_condition_switch_2_enabled();
				clipboard_event_data["sw2_name"] = ev->get_condition_switch_2_name();
				clipboard_event_data["self_sw_en"] = ev->is_condition_self_switch_enabled();
				clipboard_event_data["self_sw_letter"] = ev->get_condition_self_switch_letter();
				clipboard_event_data["self_switches"] = ev->get_self_switches().duplicate(true);
				clipboard_event_data["var_en"] = ev->is_condition_var_enabled();
				clipboard_event_data["var_name"] = ev->get_condition_var_name();
				clipboard_event_data["var_val"] = ev->get_condition_var_value();
				clipboard_event_data["item_en"] = ev->is_condition_item_enabled();
				clipboard_event_data["item_name"] = ev->get_condition_item_name();
				clipboard_event_data["actor_en"] = ev->is_condition_actor_enabled();
				clipboard_event_data["actor_name"] = ev->get_condition_actor_name();
			}
		} break;

		case ACTION_PASTE: { // Pegar
			if (!clipboard_event_data.is_empty()) {
				Node *scene = ensure_active_map();
				if (scene) {
					int grid_size = 32;
					if (CanvasItemEditor::get_singleton()) {
						grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
					}
					ClockGearsEvent *target = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
					if (!target) {
						target = memnew(ClockGearsEvent);
						target->set_position(Vector2(context_menu_cell.x * grid_size, context_menu_cell.y * grid_size));
						target->set_event_name(clipboard_event_data.get("name", vformat("EV_%03d_%03d", context_menu_cell.x, context_menu_cell.y)));
						target->set_name(target->get_event_name());
						ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
						if (map && map->get_events_container()) {
							map->get_events_container()->add_child(target);
						} else {
							scene->add_child(target);
						}
						target->set_owner(scene);
					}
					target->set_event_name(clipboard_event_data.get("name", "EV_Pasted"));
					target->set_trigger_type(clipboard_event_data.get("trigger", 0));
					target->set_move_type(clipboard_event_data.get("move_type", 0));
					target->set_move_speed(clipboard_event_data.get("speed", 100.0f));
					target->set_move_frequency(clipboard_event_data.get("freq", 1.0f));
					target->set_through(clipboard_event_data.get("through", false));
					target->set_direction_fix(clipboard_event_data.get("direction_fix", false));
					target->set_direction(clipboard_event_data.get("direction", 2));
					target->set_priority_type(clipboard_event_data.get("priority", 1));
					target->set_graphic_path(clipboard_event_data.get("graphic_path", ""));
					target->set_sprite_hframes(clipboard_event_data.get("hframes", 1));
					target->set_sprite_vframes(clipboard_event_data.get("vframes", 1));
					target->set_sprite_frame(clipboard_event_data.get("frame", 0));
					target->set_commands(clipboard_event_data.get("commands", Array()));
					if (clipboard_event_data.has("pages")) {
						target->set_pages(clipboard_event_data["pages"].duplicate(true));
					}
					target->set_condition_switch_1_enabled(clipboard_event_data.get("sw1_en", false));
					target->set_condition_switch_1_name(clipboard_event_data.get("sw1_name", ""));
					target->set_condition_switch_2_enabled(clipboard_event_data.get("sw2_en", false));
					target->set_condition_switch_2_name(clipboard_event_data.get("sw2_name", ""));
					target->set_condition_self_switch_enabled(clipboard_event_data.get("self_sw_en", false));
					target->set_condition_self_switch_letter(clipboard_event_data.get("self_sw_letter", "A"));
					target->set_self_switches(clipboard_event_data.get("self_switches", Dictionary()));
					target->set_condition_var_enabled(clipboard_event_data.get("var_en", false));
					target->set_condition_var_name(clipboard_event_data.get("var_name", ""));
					target->set_condition_var_value(clipboard_event_data.get("var_val", 0));
					target->set_condition_item_enabled(clipboard_event_data.get("item_en", false));
					target->set_condition_item_name(clipboard_event_data.get("item_name", ""));
					target->set_condition_actor_enabled(clipboard_event_data.get("actor_en", false));
					target->set_condition_actor_name(clipboard_event_data.get("actor_name", ""));

					target->update_active_page();
					target->serialize_to_meta();

					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					if (ur) {
						int history_id = ur->get_history_id_for_object(scene);
						ur->set_history_as_unsaved(history_id);
					}

					String scene_path = scene->get_scene_file_path();
					if (!scene_path.is_empty()) {
						EditorNode::get_singleton()->save_scene_if_open(scene_path);
					}

					if (CanvasItemEditor::get_singleton()) {
						CanvasItemEditor::get_singleton()->update_viewport();
					}
				}
			}
		} break;

		case ACTION_DELETE: { // Eliminar
			ClockGearsEvent *ev = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
			if (ev) {
				Node *scene = ev->get_owner() ? ev->get_owner() : ev->get_parent();
				ev->queue_free();
				if (scene) {
					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					if (ur) {
						int history_id = ur->get_history_id_for_object(scene);
						ur->set_history_as_unsaved(history_id);
					}
					String scene_path = scene->get_scene_file_path();
					if (!scene_path.is_empty()) {
						EditorNode::get_singleton()->save_scene_if_open(scene_path);
					}
				}
				if (CanvasItemEditor::get_singleton()) {
					CanvasItemEditor::get_singleton()->update_viewport();
				}
			}
		} break;
	}
}

void ClockGearsEditorPlugin::_on_quick_event_id_pressed(int p_id) {
	print_line(vformat("[ClockGears] Quick event selected: %d at (%d, %d)", p_id, context_menu_cell.x, context_menu_cell.y));
	Node *scene = ensure_active_map();
	if (!scene) {
		ERR_PRINT(String::utf8("ClockGears: No se pudo obtener ni crear un mapa activo para el evento rápido."));
		return;
	}
	int grid_size = 32;
	if (CanvasItemEditor::get_singleton()) {
		grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
	}

	ClockGearsEvent *ev = memnew(ClockGearsEvent);
	ev->set_position(Vector2(context_menu_cell.x * grid_size, context_menu_cell.y * grid_size));

	Array pages;

	switch (p_id) {
		case QUICK_EV_CHEST: {
			ev->set_name(vformat("EV_Cofre_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Cofre");
			// Page 1: Cerrado, con contenido
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds1;
			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/chest.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds1.push_back(c_se);

			Dictionary c_gold;
			c_gold["type"] = "gold";
			c_gold["op"] = "+";
			c_gold["amount"] = 100;
			cmds1.push_back(c_gold);

			Dictionary c_msg;
			c_msg["type"] = "message";
			c_msg["text"] = CG_TR("¡Has encontrado 100 Monedas de Oro en el cofre!");
			c_msg["frame"] = 0;
			c_msg["position"] = 0;
			c_msg["face"] = "";
			cmds1.push_back(c_msg);

			Dictionary c_ssw;
			c_ssw["type"] = "self_switch";
			c_ssw["letter"] = "A";
			c_ssw["val"] = true;
			cmds1.push_back(c_ssw);

			p1["commands"] = cmds1;
			pages.push_back(p1);

			// Page 2: Abierto (Condición Interruptor Local A = ON)
			Dictionary p2;
			p2["sw1_en"] = false;
			p2["sw2_en"] = false;
			p2["self_sw_en"] = true;
			p2["self_sw_letter"] = "A";
			p2["var_en"] = false;
			p2["item_en"] = false;
			p2["actor_en"] = false;
			p2["trigger"] = TRIGGER_ACTION_BUTTON;
			p2["priority"] = 1;
			p2["through"] = false;
			p2["dir_fix"] = true;
			p2["graphic_path"] = "";
			p2["hframes"] = 1;
			p2["vframes"] = 1;
			p2["frame"] = 0;
			p2["move_type"] = MOVE_FIXED;
			p2["speed"] = 3;
			p2["freq"] = 3;

			Array cmds2;
			Dictionary c_empty_msg;
			c_empty_msg["type"] = "message";
			c_empty_msg["text"] = CG_TR("El cofre ya está vacío.");
			c_empty_msg["frame"] = 0;
			c_empty_msg["position"] = 0;
			c_empty_msg["face"] = "";
			cmds2.push_back(c_empty_msg);
			p2["commands"] = cmds2;
			pages.push_back(p2);
		} break;

		case QUICK_EV_DOOR: {
			if (ev) {
				memdelete(ev);
			}
			open_quick_door_dialog(context_menu_cell);
			return;
		} break;

		case QUICK_EV_TRANSFER: {
			ev->set_name(vformat("EV_Transfer_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Transferir");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_PLAYER_TOUCH;
			p1["priority"] = 0;
			p1["through"] = true;
			p1["dir_fix"] = false;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_fade_out;
			c_fade_out["type"] = "fade";
			c_fade_out["mode"] = "Oscurecer";
			c_fade_out["duration"] = 20;
			cmds.push_back(c_fade_out);

			Dictionary c_tele;
			c_tele["type"] = "teleport";
			c_tele["map"] = "maps/map002.tscn";
			c_tele["x"] = 10;
			c_tele["y"] = 10;
			c_tele["dir"] = "Mantener";
			cmds.push_back(c_tele);

			Dictionary c_fade_in;
			c_fade_in["type"] = "fade";
			c_fade_in["mode"] = "Aclarar";
			c_fade_in["duration"] = 20;
			cmds.push_back(c_fade_in);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;

		case QUICK_EV_INN: {
			ev->set_name(vformat("EV_Posada_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Posada");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_msg1;
			c_msg1["type"] = "message";
			c_msg1["text"] = CG_TR("¿Deseas descansar en la posada por 10 monedas de oro?");
			cmds.push_back(c_msg1);

			Dictionary c_choices;
			c_choices["type"] = "choices";
			c_choices["c1"] = CG_TR("Sí, descansar");
			c_choices["c2"] = CG_TR("No, gracias");
			c_choices["c3"] = "";
			cmds.push_back(c_choices);

			Dictionary c_gold;
			c_gold["type"] = "gold";
			c_gold["op"] = "-";
			c_gold["amount"] = 10;
			cmds.push_back(c_gold);

			Dictionary c_fade_out;
			c_fade_out["type"] = "fade";
			c_fade_out["mode"] = "Oscurecer";
			c_fade_out["duration"] = 30;
			cmds.push_back(c_fade_out);

			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/inn.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds.push_back(c_se);

			Dictionary c_wait;
			c_wait["type"] = "wait";
			c_wait["frames"] = 60;
			cmds.push_back(c_wait);

			Dictionary c_heal;
			c_heal["type"] = "hp_mp";
			c_heal["stat"] = "HP";
			c_heal["op"] = "+";
			c_heal["amount"] = 9999;
			cmds.push_back(c_heal);

			Dictionary c_fade_in;
			c_fade_in["type"] = "fade";
			c_fade_in["mode"] = "Aclarar";
			c_fade_in["duration"] = 30;
			cmds.push_back(c_fade_in);

			Dictionary c_msg2;
			c_msg2["type"] = "message";
			c_msg2["text"] = CG_TR("¡Has descansado placenteramente y recuperado todas tus fuerzas!");
			cmds.push_back(c_msg2);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;

		case QUICK_EV_SAVE: {
			ev->set_name(vformat("EV_Guardar_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Guardado");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/save.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds.push_back(c_se);

			Dictionary c_msg;
			c_msg["type"] = "message";
			c_msg["text"] = CG_TR("¡Progreso de la partida guardado con éxito!");
			cmds.push_back(c_msg);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;
	}

	ev->set_pages(pages);
	ev->update_active_page();
	ev->serialize_to_meta();

	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
	if (map && map->get_events_container()) {
		map->get_events_container()->add_child(ev);
	} else {
		scene->add_child(ev);
	}
	ev->set_owner(scene);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	if (ur) {
		int history_id = ur->get_history_id_for_object(scene);
		ur->set_history_as_unsaved(history_id);
	}

	String scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		EditorNode::get_singleton()->save_scene_if_open(scene_path);
	} else {
		EditorNode::get_singleton()->clockgears_save_scene();
	}

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}

void ClockGearsEditorPlugin::open_quick_door_dialog(const Point2i &p_cell) {
	quick_door_cell = p_cell;
	_populate_quick_door_sprites();
	if (quick_door_map_selector) {
		quick_door_map_selector->populate_maps();
	}
	if (dlg_quick_door) {
		dlg_quick_door->popup_centered(Size2(680, 520) * EDSCALE);
	}
}

void ClockGearsEditorPlugin::_populate_quick_door_sprites() {
	if (!quick_door_sprite_opt) return;
	quick_door_sprite_opt->clear();
	quick_door_sprite_opt->add_item(CG_TR("(Sin gráfico / Invisible)"), 0);
	quick_door_sprite_opt->set_item_metadata(0, "");

	Vector<String> files;
	if (DirAccess::exists("res://img/characters")) {
		Ref<DirAccess> da = DirAccess::open("res://img/characters");
		if (da.is_valid()) {
			da->list_dir_begin();
			String f = da->get_next();
			while (!f.is_empty()) {
				if (!da->current_is_dir() && (f.ends_with(".png") || f.ends_with(".webp"))) {
					files.push_back(String("res://img/characters").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
		}
	}
	files.sort();

	int sel_idx = 0;
	int item_count = 1;
	for (int i = 0; i < files.size(); i++) {
		String fpath = files[i];
		String fname = fpath.get_file();
		quick_door_sprite_opt->add_item(fname, item_count);
		quick_door_sprite_opt->set_item_metadata(item_count, fpath);
		if (fname.to_lower().contains("door") || fname.to_lower().contains("puerta")) {
			if (sel_idx == 0) {
				sel_idx = item_count;
			}
		}
		item_count++;
	}

	if (files.is_empty()) {
		quick_door_sprite_opt->select(0);
		if (quick_door_preview) quick_door_preview->set_texture(Ref<Texture2D>());
	} else {
		quick_door_sprite_opt->select(sel_idx);
		_on_quick_door_sprite_selected(sel_idx);
	}
}

void ClockGearsEditorPlugin::_on_quick_door_sprite_selected(int p_idx) {
	if (!quick_door_sprite_opt || p_idx < 0 || p_idx >= quick_door_sprite_opt->get_item_count()) return;
	String fpath = quick_door_sprite_opt->get_item_metadata(p_idx);
	if (quick_door_preview) {
		if (!fpath.is_empty() && ResourceLoader::exists(fpath)) {
			Ref<Texture2D> tex = ResourceLoader::load(fpath);
			quick_door_preview->set_texture(tex);
		} else {
			quick_door_preview->set_texture(Ref<Texture2D>());
		}
	}
}

void ClockGearsEditorPlugin::_on_quick_door_confirmed() {
	String target_map = quick_door_map_selector ? quick_door_map_selector->get_target_map() : "";
	Vector2i target_coords = quick_door_map_selector ? quick_door_map_selector->get_target_coords() : Vector2i(0, 0);
	String target_dir = quick_door_map_selector ? quick_door_map_selector->get_target_direction() : "Abajo";
	String sprite_path;
	if (quick_door_sprite_opt && quick_door_sprite_opt->get_selected() > 0) {
		sprite_path = quick_door_sprite_opt->get_item_metadata(quick_door_sprite_opt->get_selected());
	}
	int hframes = quick_door_hframes_spin ? int(quick_door_hframes_spin->get_value()) : 1;

	create_quick_door_event(quick_door_cell, target_map, target_coords.x, target_coords.y, target_dir, sprite_path, hframes);
}

ClockGearsEvent *ClockGearsEditorPlugin::create_quick_door_event(const Point2i &p_cell, const String &p_map, int p_x, int p_y, const String &p_dir, const String &p_sprite_path, int p_hframes) {
	Node *scene = ensure_active_map();
	ERR_FAIL_COND_V(!scene, nullptr);

	ClockGearsEvent *ev = ClockGearsMapCellSelector::create_quick_door_event(scene, p_cell, p_map, p_x, p_y, p_dir, p_sprite_path, p_hframes);
	if (!ev) return nullptr;

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	if (ur) {
		int history_id = ur->get_history_id_for_object(scene);
		ur->set_history_as_unsaved(history_id);
	}

	String scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		EditorNode::get_singleton()->save_scene_if_open(scene_path);
	} else {
		EditorNode::get_singleton()->clockgears_save_scene();
	}

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}

	return ev;
}

void ClockGearsEditorPlugin::open_database() {
	if (database_dialog) {
		database_dialog->open_database();
	}
}

void ClockGearsEditorPlugin::open_generate_entity_dialog_at_cell(int p_x, int p_y) {
	if (!generate_entity_dialog) {
		return;
	}
	generate_entity_dialog->open_for_cell(p_x, p_y);
}

void ClockGearsEditorPlugin::_on_generate_entity_chosen(int p_tab, int p_record_id, const String &p_record_name, int p_cell_x, int p_cell_y) {
	generate_entity_at_cell(p_tab, p_record_id, p_cell_x, p_cell_y, true);
}

Node *ClockGearsGenerateEntityDialog::create_entity_event_on_map(Node *p_scene, int p_tab, int p_record_id, int p_x, int p_y, int p_grid_size, bool p_use_undo_redo, Object *p_undo_redo) {
	if (!p_scene || p_tab == TAB_FLORA || p_tab == TAB_FAUNA || p_record_id <= 0) {
		return nullptr;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return nullptr;
	}

	int grid_size = p_grid_size > 0 ? p_grid_size : 32;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(p_scene);
	Node *parent_container = (map && map->get_events_container()) ? (Node *)map->get_events_container() : p_scene;

	ClockGearsEvent *ev = memnew(ClockGearsEvent);
	ev->set_position(Vector2(p_x * grid_size, p_y * grid_size));

	String base_node_name = vformat("EV_%03d_%03d", p_x, p_y);
	String unique_name = base_node_name;
	int suffix = 1;
	while (parent_container->has_node(NodePath(unique_name))) {
		unique_name = vformat("%s_%d", base_node_name, suffix++);
	}
	ev->set_name(unique_name);

	if (p_tab == TAB_NPC || p_tab == TAB_ARPG_NPC || p_tab == TAB_ARPG_ENEMIGO) {
		int btype = 1;
		int cat = CAT_NPCS;
		if (p_tab == TAB_ARPG_NPC) {
			btype = 2;
			cat = CAT_ARPG_NPCS;
		} else if (p_tab == TAB_ARPG_ENEMIGO) {
			btype = 3;
			cat = CAT_ARPG_ENEMIES;
		}
		Dictionary rec = dm->get_record_by_id(cat, p_record_id);
		String rname = String(rec.get("name", ""));
		ev->set_event_name(rname.is_empty() ? base_node_name : rname);
		ev->set_behavior_type(btype);
		ev->set_behavior_ref_id(p_record_id);
		ev->serialize_to_meta();
	} else if (p_tab == TAB_ENEMIGO) {
		Dictionary troop = dm->get_record_by_id(CAT_TROOPS, p_record_id);
		String troop_name = String(troop.get("name", ""));
		ev->set_event_name(troop_name.is_empty() ? base_node_name : troop_name);
		ev->set_behavior_type(0);
		ev->set_behavior_ref_id(0);
		ev->set_trigger_type(TRIGGER_PLAYER_TOUCH);
		ev->set_priority_type(1);
		ev->set_through(false);
		ev->set_direction_fix(false);
		ev->set_move_type(MOVE_FIXED);
		ev->set_move_speed(3);
		ev->set_move_frequency(3);

		String gpath;
		int hf = 1;
		int vf = 1;
		int fr = 0;
		Array mems = troop.get("members", Array());
		if (!mems.is_empty() && mems[0].get_type() == Variant::DICTIONARY) {
			int eid = int(Dictionary(mems[0]).get("enemy_id", 1));
			Dictionary ed = dm->get_record_by_id(CAT_ENEMIES, eid);
			if (!ed.is_empty()) {
				Ref<Texture2D> sp_tex;
				Rect2 sp_reg;
				String sp_src;
				ClockGearsBattleScene::resolve_enemy_preview_sprite(ed, sp_tex, sp_reg, sp_src);
				if (sp_src != "placeholder" && sp_tex.is_valid() && !sp_tex->get_path().is_empty()) {
					gpath = sp_tex->get_path();
					if (sp_reg.size.x > 0 && sp_reg.size.y > 0) {
						hf = MAX(1, int(Math::round(float(sp_tex->get_width()) / sp_reg.size.x)));
						vf = MAX(1, int(Math::round(float(sp_tex->get_height()) / sp_reg.size.y)));
						int col = CLAMP(int(sp_reg.position.x / sp_reg.size.x), 0, hf - 1);
						int row = CLAMP(int(sp_reg.position.y / sp_reg.size.y), 0, vf - 1);
						fr = row * hf + col;
					}
				}
			}
		}
		ev->set_graphic_path(gpath);
		ev->set_sprite_hframes(hf);
		ev->set_sprite_vframes(vf);
		ev->set_sprite_frame(fr);

		Array cmds;
		Dictionary cmd;
		cmd["type"] = "battle";
		cmd["code"] = CMD_BATTLE_PROCESSING;
		cmd["troop_id"] = p_record_id;
		cmd["troop"] = troop_name.is_empty() ? vformat("%03d", p_record_id) : vformat("%03d: %s", p_record_id, troop_name);
		cmd["can_escape"] = true;
		cmd["can_lose"] = false;
		cmd["bgm"] = "";
		cmds.push_back(cmd);

		Dictionary p0;
		p0["sw1_en"] = false;
		p0["sw2_en"] = false;
		p0["self_sw_en"] = false;
		p0["self_sw_letter"] = "A";
		p0["var_en"] = false;
		p0["item_en"] = false;
		p0["actor_en"] = false;
		p0["trigger"] = TRIGGER_PLAYER_TOUCH;
		p0["priority"] = 1;
		p0["through"] = false;
		p0["dir_fix"] = false;
		p0["graphic_path"] = gpath;
		p0["hframes"] = hf;
		p0["vframes"] = vf;
		p0["frame"] = fr;
		p0["move_type"] = MOVE_FIXED;
		p0["speed"] = 3;
		p0["freq"] = 3;
		p0["commands"] = cmds;

		Array pages;
		pages.push_back(p0);
		ev->set_pages(pages);
		ev->set_commands(cmds);
	} else {
		memdelete(ev);
		return nullptr;
	}

	UndoRedo *test_ur = p_use_undo_redo ? Object::cast_to<UndoRedo>(p_undo_redo) : nullptr;
	EditorUndoRedoManager *editor_ur = (p_use_undo_redo && !test_ur) ? EditorUndoRedoManager::get_singleton() : nullptr;
	if (test_ur) {
		test_ur->create_action(CG_TR("Generar entidad en el mapa"), UndoRedo::MERGE_DISABLE);
		test_ur->add_do_method(Callable(parent_container, "add_child").bind(ev));
		test_ur->add_do_method(Callable(ev, "set_owner").bind(p_scene));
		test_ur->add_do_method(Callable(ev, "update_active_page"));
		test_ur->add_do_method(Callable(ev, "serialize_to_meta"));
		test_ur->add_do_reference(ev);
		test_ur->add_undo_method(Callable(parent_container, "remove_child").bind(ev));
		test_ur->commit_action();
	} else if (editor_ur) {
		editor_ur->create_action(CG_TR("Generar entidad en el mapa"), UndoRedo::MERGE_DISABLE, p_scene);
		editor_ur->add_do_method(parent_container, "add_child", ev);
		editor_ur->add_do_method(ev, "set_owner", p_scene);
		editor_ur->add_do_method(ev, "update_active_page");
		editor_ur->add_do_method(ev, "serialize_to_meta");
		editor_ur->add_do_reference(ev);
		editor_ur->add_undo_method(parent_container, "remove_child", ev);
		editor_ur->commit_action();
	} else {
		parent_container->add_child(ev);
		ev->set_owner(p_scene);
		ev->update_active_page();
		ev->serialize_to_meta();
	}
	return ev;
}

ClockGearsEvent *ClockGearsEditorPlugin::generate_entity_at_cell(int p_tab, int p_record_id, int p_x, int p_y, bool p_use_undo_redo) {
	Node *scene = ensure_active_map();
	if (!scene) {
		return nullptr;
	}
	int grid_size = 32;
	if (CanvasItemEditor::get_singleton()) {
		grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
	}
	ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(
			ClockGearsGenerateEntityDialog::create_entity_event_on_map(scene, p_tab, p_record_id, p_x, p_y, grid_size, p_use_undo_redo, nullptr));
	if (ev && CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
	return ev;
}

void ClockGearsEditorPlugin::open_resource_manager() {
	if (resource_manager_dialog) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(resource_manager_dialog);
		}
		resource_manager_dialog->open_manager();
	}
}

void ClockGearsEditorPlugin::open_new_project() {
	if (new_project_dialog) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(new_project_dialog);
		}
		new_project_dialog->open_dialog();
	}
}

void ClockGearsEditorPlugin::open_export() {
	if (export_dialog) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(export_dialog);
		}
		export_dialog->popup_simplified_export();
	}
}

void ClockGearsEditorPlugin::open_preferences() {
	if (preferences_dialog) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(preferences_dialog);
		}
		preferences_dialog->open_preferences();
	}
}

void ClockGearsEditorPlugin::open_camera_dialog() {
	if (camera_dialog) {
		if (ClockGearsTheme::get_singleton()) {
			ClockGearsTheme::get_singleton()->apply_to_window(camera_dialog);
		}
		camera_dialog->open_camera_dialog();
	}
}

void ClockGearsEditorPlugin::open_layers_dialog() {
	if (layers_dialog) {
		layers_dialog->open_layers_dialog();
	}
}

Dictionary ClockGearsEditorPlugin::validate_playtest_conditions() {
	Dictionary result;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		result["valid"] = false;
		result["error"] = "ClockGearsDataManager no inicializado.";
		return result;
	}

	if (dm->get_category_array(CAT_ACTORS).is_empty() && FileAccess::exists("res://clockgears_data/actors.json")) {
		dm->load_all("res://clockgears_data");
	}

	// 1. Condición: Actor jugable creado en la Base de Datos
	Array actors = dm->get_category_array(CAT_ACTORS);
	bool has_actor = !actors.is_empty();

	// 2. Condición: Posición inicial del jugador y Mapa inicial
	Dictionary sys = dm->get_system_config();
	bool has_start_pos = (bool)sys.get("has_start_pos", false);
	String sys_start_map = sys.get("start_map", "");
	int start_x = (int)sys.get("start_x", -1);
	int start_y = (int)sys.get("start_y", -1);
	int start_dir = (int)sys.get("start_dir", 2);

	if (!has_start_pos && !sys_start_map.is_empty() && start_x >= 0 && start_y >= 0 && FileAccess::exists(sys_start_map)) {
		has_start_pos = true;
	}

	// Determinar el mapa objetivo a cargar (prioridad: start_map de Posición Inicial / Sistema, luego mapa abierto)
	String target_map_path;
	Node *edited_scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!sys_start_map.is_empty() && FileAccess::exists(sys_start_map)) {
		target_map_path = sys_start_map;
	} else if (edited_scene && Object::cast_to<ClockGearsMap>(edited_scene)) {
		target_map_path = edited_scene->get_scene_file_path();
	}

	if (target_map_path.is_empty() && DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			Vector<String> maps;
			da->list_dir_begin();
			String f = da->get_next();
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
			maps.sort();
			if (!maps.is_empty()) {
				target_map_path = maps[0];
			}
		}
	}

	bool has_map = !target_map_path.is_empty() && FileAccess::exists(target_map_path);

	// 4. Condición diagnóstica: Tileset del mapa
	bool has_tileset = false;
	String tileset_info = "Sin tileset asignado (opcional, se puede jugar sobre rejilla)";
	if (edited_scene) {
		ClockGearsMap *cm = Object::cast_to<ClockGearsMap>(edited_scene);
		if (cm && cm->get_tileset_id() > 0) {
			has_tileset = true;
			tileset_info = vformat("TileSet ID: %d", cm->get_tileset_id());
		} else if (cm && cm->get_layer(0) && cm->get_layer(0)->get_tile_set().is_valid() && cm->get_layer(0)->get_tile_set()->get_source_count() > 0) {
			has_tileset = true;
			tileset_info = "TileSet activo en mapa";
		}
	}

	print_line("==========================================================");
	print_line("[ClockGears Playtest] Verificando condiciones de arranque (F5 / Jugar):");
	print_line(vformat("  1. Actor en Base de Datos : %s (Total: %d)", has_actor ? "OK" : "FALTA", actors.size()));
	print_line(vformat("  2. Mapa inicial           : %s (Ruta: '%s')", has_map ? "OK" : "FALTA", target_map_path));
	print_line(vformat(CG_TR("  3. Posición inicial (PJ)  : %s (Mapa: '%s', X: %d, Y: %d, Dir: %d)"),
			has_start_pos ? "OK" : "FALTA", sys_start_map, start_x, start_y, start_dir));
	print_line(vformat("  4. Tileset                : %s (%s)", has_tileset ? "OK" : "AVISO", tileset_info));
	print_line("==========================================================");

	PackedStringArray missing_items;
	if (!has_actor) {
		missing_items.push_back(String::utf8("Actor jugable en la Base de Datos (F9 > Actores)"));
	}
	if (!has_map) {
		missing_items.push_back(String::utf8("Mapa inicial válido en el proyecto (Árbol de Mapas)"));
	}
	if (!has_start_pos) {
		missing_items.push_back(String::utf8("Posición inicial del jugador (Herramienta 'Posición Inicial del Jugador' en la barra del mapa)"));
	}

	result["has_actor"] = has_actor;
	result["has_map"] = has_map;
	result["has_start_pos"] = has_start_pos;
	result["has_tileset"] = has_tileset;
	result["target_map"] = target_map_path;
	result["start_x"] = start_x;
	result["start_y"] = start_y;
	result["start_dir"] = start_dir;
	result["missing"] = missing_items;
	result["valid"] = (has_actor && has_map && has_start_pos);
	return result;
}

bool ClockGearsEditorPlugin::start_playtest() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		ERR_PRINT(CG_TR("[ClockGears Playtest] Error crítico: ClockGearsDataManager no disponible."));
		return false;
	}

	// Guardar mapa abierto antes de validar por si es un mapa recién creado
	Node *edited_scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (edited_scene && !edited_scene->get_scene_file_path().is_empty()) {
		EditorNode::get_singleton()->save_scene_if_open(edited_scene->get_scene_file_path());
	}

	Dictionary check = validate_playtest_conditions();
	bool is_valid = check.get("valid", false);
	if (!is_valid) {
		bool has_actor = check.get("has_actor", false);
		bool has_map = check.get("has_map", false);
		bool has_start_pos = check.get("has_start_pos", false);

		String msg = String::utf8("No se puede iniciar el juego. Faltan los siguientes requisitos obligatorios:\n\n");
		if (!has_actor) {
			msg += String::utf8("• FALTA ACTOR: No hay ningún Actor creado en la Base de Datos.\n  Solución: Abre Base de Datos (F9) > Actores y crea al menos un personaje.\n\n");
		}
		if (!has_map) {
			msg += String::utf8("• FALTA MAPA INICIAL: No hay ningún mapa guardado en el proyecto.\n  Solución: Crea un mapa en el Árbol de Mapas (panel izquierdo).\n\n");
		}
		if (!has_start_pos) {
			msg += String::utf8("• FALTA POSICIÓN INICIAL DEL JUGADOR: No se ha elegido dónde empieza el jugador.\n  Solución: Selecciona la herramienta 'Posición Inicial del Jugador' (icono de banderín en la barra superior del mapa) y haz clic en una casilla del mapa.\n\n");
		}
		msg += String::utf8("Regla Cero: ClockGears Engine nunca inventa contenido no definido por el autor.");
		print_line("[ClockGears Playtest] ARRANQUE CANCELADO:\n" + msg);

		if (playtest_warning_dialog) {
			playtest_warning_dialog->set_text(msg);
			playtest_warning_dialog->popup_centered(Size2(560, 270) * EDSCALE);
		}
		return false;
	}

	String target_map_path = check.get("target_map", "");
	Array actors = dm->get_category_array(CAT_ACTORS);

	// Asegurar que el grupo inicial tenga al menos al primer actor si estaba vacío
	Dictionary sys = dm->get_system_config();
	Array party = sys.get("party", Array());
	if (party.is_empty() && !actors.is_empty()) {
		Dictionary first_actor = actors[0];
		party.push_back((int)first_actor.get("id", 1));
		sys["party"] = party;
	}
	if (String(sys.get("start_map", "")).is_empty()) {
		sys["start_map"] = target_map_path;
	}
	dm->set_system_config(sys);

	// Persistir Base de Datos JSON antes de lanzar el proceso de juego
	dm->save_all("res://clockgears_data");

	const String title_scene_path = "res://title_screen.tscn";
	ClockGearsTitleScreen::ensure_title_screen_scene(title_scene_path);

	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", title_scene_path);
		String game_title = dm->get_effective_game_title();
		if (!game_title.is_empty()) {
			ProjectSettings::get_singleton()->set_setting("application/config/name", game_title);
		}
		ProjectSettings::get_singleton()->save();
	}

	// Cerrar instancia previa de prueba si seguía abierta
	if (playtest_pid != 0 && OS::get_singleton()->is_process_running(playtest_pid)) {
		OS::get_singleton()->kill(playtest_pid);
		playtest_pid = 0;
	}

	// Lanzar ventana independiente de juego arrancando siempre por la pantalla de título
	List<String> args;
	String resource_path = ProjectSettings::get_singleton()->get_resource_path();
	if (!resource_path.is_empty()) {
		args.push_back("--path");
		args.push_back(resource_path);
	}
	args.push_back("--position");
	args.push_back("120,80");
	args.push_back("--clockgears-playtest");
	args.push_back(title_scene_path);

	print_line(vformat("[ClockGears Playtest] Lanzando juego: ejecutable='%s' --path '%s' escena='%s' (start_map='%s')",
			OS::get_singleton()->get_executable_path(), resource_path, title_scene_path, target_map_path));

	Error err = OS::get_singleton()->create_instance(args, &playtest_pid);
	if (err != OK) {
		String err_msg = vformat(String::utf8("Error al lanzar el proceso del juego (código %d)."), (int)err);
		ERR_PRINT("[ClockGears Playtest] " + err_msg);
		if (playtest_warning_dialog) {
			playtest_warning_dialog->set_text(err_msg);
			playtest_warning_dialog->popup_centered(Size2(480, 180) * EDSCALE);
		}
		return false;
	}

	print_line(vformat("[ClockGears Playtest] Juego iniciado correctamente (PID: %d).", (int)playtest_pid));
	return true;
}

void ClockGearsEditorPlugin::_on_create_missing_folders_confirmed() {
	ClockGearsProjectStructure::create_missing_folders("res://");
}

void ClockGearsEditorPlugin::_check_missing_project_folders() {
	PackedStringArray missing = ClockGearsProjectStructure::get_missing_folders("res://");
	if (missing.is_empty() || !missing_folders_dialog) {
		return;
	}
	String preview_list;
	int show_max = MIN(8, missing.size());
	for (int i = 0; i < show_max; i++) {
		preview_list += String::utf8("  • res://") + missing[i] + "\n";
	}
	if (missing.size() > show_max) {
		preview_list += vformat(CG_TR("  ... y %d carpetas más.\n"), missing.size() - show_max);
	}
	missing_folders_dialog->set_text(vformat(
			CG_TR("Al proyecto actual le faltan %d carpetas estándar de ClockGears Engine:\n\n%s\n¿Deseas crearlas ahora junto con LEEME.txt? (No se borrará ni sobrescribirá ningún archivo existente)."),
			missing.size(), preview_list));
	missing_folders_dialog->popup_centered();
}

void ClockGearsEditorPlugin::apply_clockgears_workspace() {
	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return;
	}

	// Permanently strip the stock Godot shell and lock the 2D map viewport.
	editor->set_clockgears_workspace(true);

	// Desactivar los atajos F5/F6 nativos de EditorRunBar para que F5 siempre pase por ClockGearsEditorPlugin::start_playtest()
	if (EditorSettings::get_singleton()) {
		Ref<Shortcut> sc_run = ED_GET_SHORTCUT("editor/run_project");
		if (sc_run.is_valid()) {
			sc_run->set_events(Array());
		}
		Ref<Shortcut> sc_cur = ED_GET_SHORTCUT("editor/run_current_scene");
		if (sc_cur.is_valid()) {
			sc_cur->set_events(Array());
		}
	}

	VBoxContainer *root = editor->get_clockgears_workspace_root();
	if (root && top_bar) {
		top_bar->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		if (top_bar->get_parent() != root) {
			if (top_bar->get_parent()) {
				top_bar->reparent(root);
			} else {
				root->add_child(top_bar);
			}
		}
		// Position directly below title_bar
		root->move_child(top_bar, 1);
	}

	if (root && status_bar) {
		status_bar->set_v_size_flags(Control::SIZE_SHRINK_END);
		if (status_bar->get_parent() != root) {
			if (status_bar->get_parent()) {
				status_bar->reparent(root);
			} else {
				root->add_child(status_bar);
			}
		}
		root->move_child(status_bar, root->get_child_count() - 1);
	}

	if (workspace_built) {
		return;
	}

	// ClockGearsBody is created directly by EditorNode's constructor (zero flicker):
	// the native dock shell already lives in a hidden holder, so we only need to
	// mount the left Map/Tile panel at index 0 of the body.
	HSplitContainer *body = editor->get_clockgears_body();
	if (!root || !body || !map_panel) {
		return;
	}

	// Ensure all children of root other than body do not expand vertically
	for (int i = 0; i < root->get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(root->get_child(i));
		if (c && c != body) {
			c->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		}
	}
	body->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// Left column: fixed ~300px (fills vertically, does not expand horizontally).
	map_panel->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	map_panel->set_h_size_flags(Control::SIZE_FILL);
	map_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	if (map_panel->get_parent() != body) {
		if (map_panel->get_parent()) {
			map_panel->reparent(body, false);
		} else {
			body->add_child(map_panel);
		}
	}
	body->move_child(map_panel, 0);
	body->set_split_offset(0);

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_event_double_clicked_callback(
				callable_mp(this, &ClockGearsEditorPlugin::open_event_editor_at_cell));
		CanvasItemEditor::get_singleton()->set_clockgears_event_cell_rmb_callback(
				callable_mp(this, &ClockGearsEditorPlugin::show_event_context_menu));
	}
	editor->set_clockgears_database_callback(callable_mp(this, &ClockGearsEditorPlugin::open_database));
	editor->set_clockgears_resource_manager_callback(callable_mp(this, &ClockGearsEditorPlugin::open_resource_manager));
	editor->set_clockgears_new_project_callback(callable_mp(this, &ClockGearsEditorPlugin::open_new_project));
	editor->set_clockgears_export_callback(callable_mp(this, &ClockGearsEditorPlugin::open_export));
	editor->set_clockgears_preferences_callback(callable_mp(this, &ClockGearsEditorPlugin::open_preferences));
	editor->set_clockgears_camera_callback(callable_mp(this, &ClockGearsEditorPlugin::open_camera_dialog));
	editor->set_clockgears_layers_callback(callable_mp(this, &ClockGearsEditorPlugin::open_layers_dialog));

	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme *th = ClockGearsTheme::get_singleton();
		th->sync_from_project_settings();
		th->apply_to_window(database_dialog);
		th->apply_to_window(event_editor_dialog);
		th->apply_to_window(export_dialog);
		th->apply_to_window(preferences_dialog);
		th->apply_to_window(camera_dialog);
		th->apply_to_window(layers_dialog);
		th->apply_to_window(new_project_dialog);
		th->apply_to_window(resource_manager_dialog);
		th->apply_to_window(generate_entity_dialog);
		th->apply_to_window(missing_folders_dialog);
		th->apply_to_window(playtest_warning_dialog);
		th->apply_to_window(event_context_menu);
		th->apply_to_window(quick_events_menu);
		th->apply_to_control(top_bar);
		th->apply_to_control(map_panel);
		if (status_bar) {
			th->apply_to_control(status_bar);
		}
		th->refresh_all_themed_roots();
	}

	workspace_built = true;
	if (map_panel) {
		map_panel->refresh_maps_from_disk();
	}
	// Requisito 6: Si al proyecto abierto le faltan carpetas estándar, ofrecer crearlas sin borrar nada.
	callable_mp(this, &ClockGearsEditorPlugin::_check_missing_project_folders).call_deferred();
	print_verbose("ClockGears exclusive organic workspace installed.");
}

ClockGearsEditorPlugin::ClockGearsEditorPlugin() {
	singleton = this;

	// Instantiate dialogs
	database_dialog = memnew(ClockGearsDatabaseDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(database_dialog);

	event_editor_dialog = memnew(ClockGearsEventEditorDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(event_editor_dialog);

	export_dialog = memnew(ClockGearsSimplifiedExportDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(export_dialog);

	preferences_dialog = memnew(ClockGearsPreferencesDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(preferences_dialog);

	camera_dialog = memnew(ClockGearsCameraDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(camera_dialog);

	layers_dialog = memnew(ClockGearsLayersDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(layers_dialog);
	if (EditorNode::get_singleton()) {
		EditorNode::get_singleton()->set_clockgears_layers_callback(callable_mp(this, &ClockGearsEditorPlugin::open_layers_dialog));
	}

	new_project_dialog = memnew(ClockGearsNewProjectDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(new_project_dialog);

	resource_manager_dialog = memnew(ClockGearsResourceManagerDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(resource_manager_dialog);

	generate_entity_dialog = memnew(ClockGearsGenerateEntityDialog);
	generate_entity_dialog->connect("entity_chosen", callable_mp(this, &ClockGearsEditorPlugin::_on_generate_entity_chosen));
	EditorNode::get_singleton()->get_gui_base()->add_child(generate_entity_dialog);

	missing_folders_dialog = memnew(ConfirmationDialog);
	missing_folders_dialog->set_title(CG_TR("Estructura de Carpetas de ClockGears Engine"));
	missing_folders_dialog->set_ok_button_text(CG_TR("Crear Carpetas Faltantes"));
	missing_folders_dialog->set_cancel_button_text(CG_TR("Ahora No"));
	missing_folders_dialog->connect("confirmed", callable_mp(this, &ClockGearsEditorPlugin::_on_create_missing_folders_confirmed));
	EditorNode::get_singleton()->get_gui_base()->add_child(missing_folders_dialog);

	playtest_warning_dialog = memnew(AcceptDialog);
	playtest_warning_dialog->set_title(String::utf8("Aviso de Prueba de Juego - Regla Cero"));
	playtest_warning_dialog->set_ok_button_text(String::utf8("Entendido"));
	EditorNode::get_singleton()->get_gui_base()->add_child(playtest_warning_dialog);

	// Event cell context menu
	event_context_menu = memnew(PopupMenu);
	event_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsEditorPlugin::_on_event_context_menu_id_pressed));
	EditorNode::get_singleton()->get_gui_base()->add_child(event_context_menu);

	quick_events_menu = memnew(PopupMenu);
	quick_events_menu->set_name("QuickEventsSubmenu");
	quick_events_menu->connect("id_pressed", callable_mp(this, &ClockGearsEditorPlugin::_on_quick_event_id_pressed));
	event_context_menu->add_child(quick_events_menu);

	// Diálogo Creación Rápida: Puerta
	dlg_quick_door = memnew(ConfirmationDialog);
	dlg_quick_door->set_title(CG_TR("Creación Rápida: Puerta"));
	dlg_quick_door->set_ok_button_text(CG_TR("Aceptar"));
	dlg_quick_door->set_cancel_button_text(CG_TR("Cancelar"));
	if (dlg_quick_door->get_ok_button()) {
		dlg_quick_door->get_ok_button()->set_theme_type_variation("ClockGearsPrimaryButton");
	}

	VBoxContainer *qd_main_vb = memnew(VBoxContainer);
	qd_main_vb->add_theme_constant_override("separation", 8);
	dlg_quick_door->add_child(qd_main_vb);

	PanelContainer *qd_sprite_card = memnew(PanelContainer);
	qd_sprite_card->set_theme_type_variation("ClockGearsContainer");
	qd_main_vb->add_child(qd_sprite_card);

	HBoxContainer *qd_sprite_hb = memnew(HBoxContainer);
	qd_sprite_hb->add_theme_constant_override("separation", 8);
	qd_sprite_card->add_child(qd_sprite_hb);

	quick_door_preview = memnew(TextureRect);
	quick_door_preview->set_custom_minimum_size(Size2(64, 64) * EDSCALE);
	quick_door_preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	quick_door_preview->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	quick_door_preview->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	qd_sprite_hb->add_child(quick_door_preview);

	VBoxContainer *qd_sp_info = memnew(VBoxContainer);
	qd_sp_info->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	qd_sprite_hb->add_child(qd_sp_info);

	HBoxContainer *qd_opt_hb = memnew(HBoxContainer);
	qd_opt_hb->add_child(memnew(Label(CG_TR("Sprite de Puerta (img/characters):"))));
	quick_door_sprite_opt = memnew(OptionButton);
	quick_door_sprite_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	quick_door_sprite_opt->connect("item_selected", callable_mp(this, &ClockGearsEditorPlugin::_on_quick_door_sprite_selected));
	qd_opt_hb->add_child(quick_door_sprite_opt);
	qd_sp_info->add_child(qd_opt_hb);

	HBoxContainer *qd_frames_hb = memnew(HBoxContainer);
	qd_frames_hb->add_child(memnew(Label(CG_TR("Frames horizontales (hframes):"))));
	quick_door_hframes_spin = memnew(SpinBox);
	quick_door_hframes_spin->set_min(1);
	quick_door_hframes_spin->set_max(16);
	quick_door_hframes_spin->set_value(4);
	qd_frames_hb->add_child(quick_door_hframes_spin);
	qd_sp_info->add_child(qd_frames_hb);

	quick_door_map_selector = memnew(ClockGearsMapCellSelector);
	qd_main_vb->add_child(quick_door_map_selector);

	dlg_quick_door->connect("confirmed", callable_mp(this, &ClockGearsEditorPlugin::_on_quick_door_confirmed));
	EditorNode::get_singleton()->get_gui_base()->add_child(dlg_quick_door);

	// Instantiate Top Bar
	top_bar = memnew(ClockGearsTopBar);
	top_bar->set_dialogs(database_dialog, event_editor_dialog, export_dialog);

	map_panel = memnew(ClockGearsMapPanel);
	status_bar = memnew(ClockGearsStatusBar);

	// Mount the ClockGears chrome immediately (before the first frame) to avoid any flicker.
	{
		EditorNode *en = EditorNode::get_singleton();
		VBoxContainer *root = en->get_clockgears_workspace_root();
		HSplitContainer *body = en->get_clockgears_body();
		if (root) {
			top_bar->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
			root->add_child(top_bar);
			root->move_child(top_bar, MIN(1, root->get_child_count() - 1));
			status_bar->set_v_size_flags(Control::SIZE_SHRINK_END);
			root->add_child(status_bar);
		}
		if (body) {
			map_panel->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
			map_panel->set_h_size_flags(Control::SIZE_FILL);
			map_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			body->add_child(map_panel);
			body->move_child(map_panel, 0);
		}
	}
	EditorNode::get_singleton()->add_clockgears_layout_loaded_callback(callable_mp(this, &ClockGearsEditorPlugin::apply_clockgears_workspace));

	// Main screens
	automation_panel = memnew(ClockGearsAutomationPanel);
	automation_panel->set_name(String::utf8("Herramientas de Automatización"));
	EditorInterface::get_singleton()->get_editor_main_screen()->add_child(automation_panel);
	automation_panel->hide();

	ui_editor_panel = memnew(ClockGearsUIEditorPanel);
	ui_editor_panel->set_name(String::utf8("Diseñador UI/HUD"));
	EditorInterface::get_singleton()->get_editor_main_screen()->add_child(ui_editor_panel);
	ui_editor_panel->hide();
}

ClockGearsEditorPlugin::~ClockGearsEditorPlugin() {
	singleton = nullptr;
}
