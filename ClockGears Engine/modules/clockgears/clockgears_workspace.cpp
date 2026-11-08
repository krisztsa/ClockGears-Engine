#include "clockgears_workspace.h"

#include "clockgears_map.h"
#include "clockgears_map_properties_dialog.h"
#include "clockgears_top_bar.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_pixel_art.h"
#include "modules/tilemap/tile_set.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/clockgears_project_manager.h"
#include "editor/clockgears_tile_picker.h"
#include "editor/editor_node.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/resources/packed_scene.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

static Ref<StyleBoxFlat> _clockgears_card_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.098, 0.106, 0.125));
	style->set_corner_radius_all(8);
	style->set_content_margin_all(8);
	style->set_border_width_all(1);
	style->set_border_color(Color(1, 1, 1, 0.08));
	return style;
}

static void _clockgears_icon_button(Button *p_button, const StringName &p_icon, const String &p_tooltip) {
	p_button->set_flat(true);
	p_button->set_tooltip_text(p_tooltip);
	p_button->set_focus_mode(Control::FOCUS_NONE);
	p_button->set_custom_minimum_size(Size2(26, 26));
	p_button->add_theme_constant_override("icon_max_width", 18);
	if (!p_icon.is_empty() && EditorNode::get_singleton() && EditorNode::get_singleton()->get_editor_theme().is_valid()) {
		p_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(p_icon, SNAME("EditorIcons")));
	}
}

////////////////////////////////////////////////////////////
// ClockGearsTilePalette
////////////////////////////////////////////////////////////

void ClockGearsTilePalette::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tileset", "texture"), &ClockGearsTilePalette::set_tileset);
	ClassDB::bind_method(D_METHOD("get_tileset"), &ClockGearsTilePalette::get_tileset);
	ClassDB::bind_method(D_METHOD("set_tile_size", "size"), &ClockGearsTilePalette::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &ClockGearsTilePalette::get_tile_size);
	ClassDB::bind_method(D_METHOD("set_selected_tile", "tile"), &ClockGearsTilePalette::set_selected_tile);
	ClassDB::bind_method(D_METHOD("get_selected_tile"), &ClockGearsTilePalette::get_selected_tile);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tileset", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_tileset", "get_tileset");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile_size", PROPERTY_HINT_RANGE, "8,128,1"), "set_tile_size", "get_tile_size");
	ADD_SIGNAL(MethodInfo("tile_selected", PropertyInfo(Variant::INT, "tile")));
}

void ClockGearsTilePalette::set_region_palette(bool p_is_region) {
	is_region_palette = p_is_region;
	if (is_region_palette) {
		selected_tile = 1;
	}
	_update_metrics();
	queue_redraw();
}

void ClockGearsTilePalette::_update_metrics() {
	columns = 0;
	rows = 0;
	draw_scale = user_zoom;
	texture_origin = pan_offset.round();

	if (is_region_palette) {
		columns = 8;
		rows = 32; // 8 * 32 = 256 regiones (0..255)
		draw_scale = MAX(0.75f, user_zoom);
		return;
	}

	if (tileset_texture.is_null() || tile_size <= 0) {
		return;
	}

	const Size2 tex_size = tileset_texture->get_size();
	columns = MAX(1, int(tex_size.x / tile_size));
	rows = MAX(1, int(tex_size.y / tile_size));
}

int ClockGearsTilePalette::_tile_at(const Point2 &p_position) const {
	if (columns <= 0 || rows <= 0 || draw_scale <= 0) {
		return -1;
	}
	const Point2 local = (p_position - texture_origin) / draw_scale;
	if (local.x < 0 || local.y < 0) {
		return -1;
	}
	const int col = int(local.x / tile_size);
	const int row = int(local.y / tile_size);
	if (col >= columns || row >= rows) {
		return -1;
	}
	return row * columns + col;
}

Rect2 ClockGearsTilePalette::_tile_rect(int p_index) const {
	if (p_index < 0 || columns <= 0) {
		return Rect2();
	}
	const int col = p_index % columns;
	const int row = p_index / columns;
	const Vector2 pos = (texture_origin + Vector2(col * tile_size, row * tile_size) * draw_scale).round();
	const Vector2 sz = (Vector2(tile_size, tile_size) * draw_scale).round();
	return Rect2(pos, sz);
}

void ClockGearsTilePalette::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Zoom con rueda del ratón (o scroll vertical en paleta de regiones)
		if (mb->is_pressed() && (mb->get_button_index() == MouseButton::WHEEL_UP || mb->get_button_index() == MouseButton::WHEEL_DOWN)) {
			if (is_region_palette && !mb->is_ctrl_pressed()) {
				float step = tile_size * draw_scale;
				pan_offset.y += (mb->get_button_index() == MouseButton::WHEEL_UP) ? step : -step;
				float min_y = MIN(0.0f, get_size().y - rows * step);
				pan_offset.y = CLAMP(pan_offset.y, min_y, 0.0f);
			} else {
				const float steps[] = { 0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f };
				if (mb->get_button_index() == MouseButton::WHEEL_UP) {
					for (int i = 0; i < 6; i++) {
						if (steps[i] > user_zoom + 0.01f) {
							user_zoom = steps[i];
							break;
						}
					}
				} else {
					for (int i = 5; i >= 0; i--) {
						if (steps[i] < user_zoom - 0.01f) {
							user_zoom = steps[i];
							break;
						}
					}
				}
			}
			_update_metrics();
			queue_redraw();
			accept_event();
			return;
		}

		// Paneo con botón central del ratón
		if (mb->get_button_index() == MouseButton::MIDDLE) {
			is_panning = mb->is_pressed();
			pan_last_mouse = mb->get_position();
			accept_event();
			return;
		}

		if (mb->get_button_index() == MouseButton::LEFT) {
			const int tile = _tile_at(mb->get_position());
			if (mb->is_pressed()) {
				if (tile >= 0) {
					selected_tile = tile;
					Vector2i tc(tile % columns, tile / columns);
					if (is_region_palette) {
						selected_coords_list.clear();
						selected_coords_list.push_back(tc);
						selection_block_size = Vector2i(1, 1);
						emit_signal(SNAME("tile_selected"), tile);
					} else if (mb->is_ctrl_pressed()) {
						int idx = selected_coords_list.find(tc);
						if (idx >= 0 && selected_coords_list.size() > 1) {
							selected_coords_list.remove_at(idx);
						} else if (idx < 0) {
							selected_coords_list.push_back(tc);
						}
						selection_block_size = Vector2i(1, 1);
						emit_signal(SNAME("tile_selected"), tile);
					} else {
						is_dragging_selection = true;
						drag_start_coords = tc;
						selected_coords_list.clear();
						selected_coords_list.push_back(tc);
						selection_block_size = Vector2i(1, 1);
						emit_signal(SNAME("tile_selected"), tile);
					}
					queue_redraw();
					accept_event();
				}
			} else {
				if (is_dragging_selection) {
					is_dragging_selection = false;
					emit_signal(SNAME("tile_selected"), selected_tile);
					queue_redraw();
					accept_event();
				}
			}
			return;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (is_panning) {
			Vector2 delta = mm->get_position() - pan_last_mouse;
			pan_last_mouse = mm->get_position();
			pan_offset = (pan_offset + delta).round();
			_update_metrics();
			queue_redraw();
			accept_event();
			return;
		}

		const int tile = _tile_at(mm->get_position());
		if (tile != hovered_tile) {
			hovered_tile = tile;
			queue_redraw();
		}

		if (is_dragging_selection && !is_region_palette && tile >= 0 && columns > 0) {
			Vector2i cur(tile % columns, tile / columns);
			int x0 = MIN(drag_start_coords.x, cur.x);
			int y0 = MIN(drag_start_coords.y, cur.y);
			int x1 = MAX(drag_start_coords.x, cur.x);
			int y1 = MAX(drag_start_coords.y, cur.y);
			selected_coords_list.clear();
			for (int ry = y0; ry <= y1; ry++) {
				for (int rx = x0; rx <= x1; rx++) {
					selected_coords_list.push_back(Vector2i(rx, ry));
				}
			}
			selection_block_size = Vector2i(x1 - x0 + 1, y1 - y0 + 1);
			selected_tile = y0 * columns + x0;
			emit_signal(SNAME("tile_selected"), selected_tile);
			queue_redraw();
		}
	}
}

void ClockGearsTilePalette::_draw_palette() {
	const Size2 size = get_size();
	if (size.x <= 0 || size.y <= 0) {
		return;
	}

	// Checker background
	const Color c1 = Color(0.12, 0.13, 0.15);
	const Color c2 = Color(0.08, 0.09, 0.11);
	draw_rect(Rect2(Point2(), size), c1);

	const int checker_size = 16;
	for (int y = 0; y < size.y; y += checker_size) {
		for (int x = 0; x < size.x; x += checker_size) {
			if (((x / checker_size) + (y / checker_size)) % 2 == 1) {
				draw_rect(Rect2(Point2(x, y), Size2(checker_size, checker_size)), c2);
			}
		}
	}

	_update_metrics();
	Ref<Font> font = get_theme_font(SNAME("font"), SNAME("Label"));
	const int font_size = get_theme_font_size(SNAME("font_size"), SNAME("Label"));

	// Modo Paleta de Regiones (Sección 6.3: Pestaña R con Regiones 0..255)
	if (is_region_palette) {
		for (int idx = 0; idx < 256; idx++) {
			Rect2 r = _tile_rect(idx);
			if (r.position.y + r.size.y < 0 || r.position.y > size.y) {
				continue;
			}
			if (idx > 0) {
				float hue = Math::fmod(float(idx * 47 % 360) / 360.0f, 1.0f);
				Color rcol = Color::from_hsv(hue, 0.75f, 0.88f, 0.72f);
				draw_rect(r, rcol, true);
			} else {
				draw_rect(r, Color(0.06f, 0.07f, 0.09f, 0.85f), true);
			}
			draw_rect(r, Color(1, 1, 1, 0.22f), false, 1.0f);
			if (font.is_valid()) {
				String lbl = (idx == 0) ? "X" : String::num_int64(idx);
				draw_string(font, r.position + Point2(4, r.size.y * 0.65f), lbl, HORIZONTAL_ALIGNMENT_LEFT, -1, CLAMP(font_size - 1, 9, 12), Color(1, 1, 1, 0.95f));
			}
		}
		if (selected_tile >= 0) {
			Rect2 rect = _tile_rect(selected_tile);
			draw_rect(rect, Color(1.0, 0.85, 0.3, 0.98), false, 2.0);
		}
		if (hovered_tile >= 0 && hovered_tile != selected_tile) {
			Rect2 rect = _tile_rect(hovered_tile);
			draw_rect(rect, Color(1, 1, 1, 0.65), false, 1.0);
		}
		return;
	}

	if (tileset_texture.is_null()) {
		const String msg = empty_message.is_empty() ? CG_TR("Arrastra un tileset 32x32 o pulsa Cargar") : empty_message;
		PackedStringArray lines = msg.split("\n");
		float total_h = lines.size() * (font_size + 4);
		float cur_y = (size.y - total_h) * 0.5f + font->get_ascent(font_size);
		for (int li = 0; li < lines.size(); li++) {
			Size2 text_size = font->get_string_size(lines[li], HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
			Point2 text_pos = Point2((size.x - text_size.x) * 0.5f, cur_y);
			draw_string(font, text_pos, lines[li], HORIZONTAL_ALIGNMENT_CENTER, -1, font_size, Color(1, 1, 1, 0.40f));
			cur_y += font_size + 4;
		}
		return;
	}

	// Draw scaled tileset
	const Rect2 dst(texture_origin, (tileset_texture->get_size() * draw_scale).round());
	draw_texture_rect(tileset_texture, dst, false);

	// A4.5: Cuadrícula de píxeles en zoom >= 4x
	if (draw_scale >= 4.0f) {
		Color px_col(1, 1, 1, 0.08f);
		int px_w = columns * tile_size;
		int px_h = rows * tile_size;
		for (int px = 1; px < px_w; px++) {
			float x = Math::round(texture_origin.x + px * draw_scale);
			draw_line(Point2(x, texture_origin.y), Point2(x, texture_origin.y + px_h * draw_scale), px_col);
		}
		for (int py = 1; py < px_h; py++) {
			float y = Math::round(texture_origin.y + py * draw_scale);
			draw_line(Point2(texture_origin.x, y), Point2(texture_origin.x + px_w * draw_scale, y), px_col);
		}
	}

	// Grid lines
	const Color grid_color(1, 1, 1, 0.15);
	for (int c = 0; c <= columns; c++) {
		const real_t x = Math::round(texture_origin.x + c * tile_size * draw_scale);
		draw_line(Point2(x, texture_origin.y), Point2(x, texture_origin.y + rows * tile_size * draw_scale), grid_color);
	}
	for (int r = 0; r <= rows; r++) {
		const real_t y = Math::round(texture_origin.y + r * tile_size * draw_scale);
		draw_line(Point2(texture_origin.x, y), Point2(texture_origin.x + columns * tile_size * draw_scale, y), grid_color);
	}

	// Multi-tile or single-tile selection highlight
	if (!selected_coords_list.is_empty()) {
		for (int i = 0; i < selected_coords_list.size(); i++) {
			Vector2i c = selected_coords_list[i];
			int tidx = c.y * columns + c.x;
			const Rect2 rect = _tile_rect(tidx);
			draw_rect(rect, Color(1.0, 0.75, 0.2, 0.28), true);
			draw_rect(rect, Color(1.0, 0.85, 0.3, 0.95), false, 2.0);
		}
	} else if (selected_tile >= 0) {
		const Rect2 rect = _tile_rect(selected_tile);
		draw_rect(rect, Color(1.0, 0.75, 0.2, 0.3), true);
		draw_rect(rect, Color(1.0, 0.85, 0.3, 0.95), false, 2.0);
	}

	// Hover highlight
	if (hovered_tile >= 0 && hovered_tile != selected_tile) {
		const Rect2 rect = _tile_rect(hovered_tile);
		draw_rect(rect, Color(1, 1, 1, 0.15), true);
		draw_rect(rect, Color(1, 1, 1, 0.6), false, 1.0);
	}
}

void ClockGearsTilePalette::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			_draw_palette();
		} break;
		case NOTIFICATION_RESIZED: {
			_update_metrics();
			queue_redraw();
		} break;
		case NOTIFICATION_MOUSE_EXIT: {
			if (hovered_tile >= 0) {
				hovered_tile = -1;
				queue_redraw();
			}
		} break;
	}
}

void ClockGearsTilePalette::set_tileset(const Ref<Texture2D> &p_texture) {
	tileset_texture = p_texture;
	selected_tile = 0;
	hovered_tile = -1;
	pan_offset = Vector2();
	user_zoom = 1.0f;
	selected_coords_list.clear();
	selected_coords_list.push_back(Vector2i(0, 0));
	selection_block_size = Vector2i(1, 1);
	_update_metrics();
	queue_redraw();
}

void ClockGearsTilePalette::set_tile_size(int p_size) {
	if (p_size <= 0 || p_size == tile_size) {
		return;
	}
	tile_size = p_size;
	_update_metrics();
	queue_redraw();
}

void ClockGearsTilePalette::set_selected_tile(int p_tile) {
	selected_tile = p_tile;
	queue_redraw();
}

Size2 ClockGearsTilePalette::get_minimum_size() const {
	return Size2(120, 150);
}

ClockGearsTilePalette::ClockGearsTilePalette() {
	set_mouse_filter(Control::MOUSE_FILTER_STOP);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST); // A4.5: Siempre NEAREST
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	set_clip_contents(true);
}

////////////////////////////////////////////////////////////
// ClockGearsMapPanel
////////////////////////////////////////////////////////////

ClockGearsMapPanel *ClockGearsMapPanel::singleton = nullptr;

void ClockGearsMapPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_context_menu_id_pressed", "id"), &ClockGearsMapPanel::_on_context_menu_id_pressed);
	ClassDB::bind_method(D_METHOD("_on_map_rmb_clicked", "pos", "button"), &ClockGearsMapPanel::_on_map_rmb_clicked);
	ClassDB::bind_method(D_METHOD("import_tileset_file", "path", "tab"), &ClockGearsMapPanel::import_tileset_file, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("load_persisted_tilesets"), &ClockGearsMapPanel::load_persisted_tilesets);
	ClassDB::bind_method(D_METHOD("get_palette_tileset_path", "tab"), &ClockGearsMapPanel::get_palette_tileset_path);
}

String ClockGearsMapPanel::get_palette_tileset_path(int p_tab) const {
	if (p_tab < 0 || p_tab >= PALETTE_TABS || !palettes[p_tab] || palettes[p_tab]->get_tileset().is_null()) {
		return "";
	}
	return palettes[p_tab]->get_tileset()->get_path();
}

Control *ClockGearsMapPanel::_make_section_header(const String &p_title, bool p_with_tileset_action) {
	HBoxContainer *header = memnew(HBoxContainer);
	header->add_theme_constant_override("separation", 6);

	Label *title = memnew(Label(p_title));
	title->add_theme_color_override("font_color", Color(0.95, 0.72, 0.26));
	title->add_theme_font_size_override("font_size", 11);
	title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	header->add_child(title);

	if (p_with_tileset_action) {
		Button *picker_button = memnew(Button);
		_clockgears_icon_button(picker_button, SNAME("Grid"), CG_TR("Abrir Selector de Tiles Avanzado (Zoom / Favoritos / Pasabilidad)"));
		picker_button->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsMapPanel::_on_open_advanced_picker_pressed));
		header->add_child(picker_button);

		Button *load_button = memnew(Button);
		_clockgears_icon_button(load_button, SNAME("Folder"), CG_TR("Cargar tileset 32x32 en esta pestaña"));
		load_button->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsMapPanel::_on_load_tileset_pressed));
		header->add_child(load_button);
	} else {
		Button *new_button = memnew(Button);
		_clockgears_icon_button(new_button, SNAME("New"), CG_TR("Nuevo mapa (Abre Propiedades)"));
		new_button->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsMapPanel::_on_add_map));
		header->add_child(new_button);

		Button *edit_button = memnew(Button);
		_clockgears_icon_button(edit_button, SNAME("Edit"), CG_TR("Editar Propiedades del mapa"));
		edit_button->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsMapPanel::_on_edit_map_properties));
		header->add_child(edit_button);

		Button *delete_button = memnew(Button);
		_clockgears_icon_button(delete_button, SNAME("Remove"), CG_TR("Eliminar mapa seleccionado"));
		delete_button->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsMapPanel::_on_delete_map));
		header->add_child(delete_button);
	}

	return header;
}

void ClockGearsMapPanel::_on_load_tileset_pressed() {
	Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
	int ts_id = map ? map->get_tileset_id() : 0;
	if (ts_id <= 0) {
		if (tileset_error_dialog) {
			tileset_error_dialog->set_text(CG_TR("Este mapa no tiene TileSet asignado.\nAsígnalo primero en Propiedades del Mapa antes de importar una hoja."));
			tileset_error_dialog->popup_centered(Size2(480, 160) * EDSCALE);
		}
		return;
	}
	String ts_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/tilesets");
	if (!ts_dir.is_empty()) {
		tileset_file_dialog->set_current_dir(ts_dir);
	}
	tileset_file_dialog->popup_centered(Size2(720, 460));
}

void ClockGearsMapPanel::_on_open_advanced_picker_pressed() {
	if (!advanced_tile_picker_dialog) {
		return;
	}
	int tab = CLAMP(tileset_tabs->get_current_tab(), 0, PALETTE_TABS - 2);
	Ref<Texture2D> tex = palettes[tab] ? palettes[tab]->get_tileset() : Ref<Texture2D>();
	advanced_tile_picker_dialog->open_picker(tex, callable_mp(this, &ClockGearsMapPanel::_on_advanced_picker_confirmed));
}

void ClockGearsMapPanel::_on_advanced_picker_confirmed(const Array &p_sel, const Vector2i &p_block) {
	if (!advanced_tile_picker_dialog || !advanced_tile_picker_dialog->get_picker()) {
		return;
	}
	ClockGearsTilePicker *pk = advanced_tile_picker_dialog->get_picker();
	Ref<Texture2D> tex = pk->get_atlas_texture();
	if (tex.is_valid()) {
		int tab = CLAMP(tileset_tabs->get_current_tab(), 0, PALETTE_TABS - 2);
		if (palettes[tab] && palettes[tab]->get_tileset() != tex) {
			palettes[tab]->set_tileset(tex);
		}
		if (CanvasItemEditor::get_singleton()) {
			CanvasItemEditor::get_singleton()->set_clockgears_region_mode(false);
			CanvasItemEditor::get_singleton()->set_clockgears_selected_tile_block(
					pk->get_primary_selected_tile(),
					p_block,
					pk->get_selected_coords(),
					tex);
		}
	}
}

void ClockGearsMapPanel::_on_palette_tab_changed(int p_tab) {
	if (!CanvasItemEditor::get_singleton()) {
		return;
	}
	if (p_tab == 5 && palettes[5]) {
		int rid = MAX(0, palettes[5]->get_selected_tile());
		CanvasItemEditor::get_singleton()->set_clockgears_region_mode(true, rid);
	} else {
		CanvasItemEditor::get_singleton()->set_clockgears_region_mode(false);
	}
}

void ClockGearsMapPanel::_on_tileset_file_selected(const String &p_path) {
	import_tileset_file(p_path, -1);
}

bool ClockGearsMapPanel::import_tileset_file(const String &p_path, int p_tab) {
	auto show_tileset_err = [this](const String &p_msg) {
		print_line(vformat(String::utf8("[ClockGears Tileset] ERROR: %s"), p_msg));
		if (tileset_error_dialog) {
			tileset_error_dialog->set_text(p_msg);
			tileset_error_dialog->popup_centered(Size2(480, 180) * EDSCALE);
		}
	};

	Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
	int ts_id = map ? map->get_tileset_id() : 0;
	if (ts_id <= 0) {
		show_tileset_err(CG_TR("Este mapa no tiene TileSet asignado. Asígnalo primero en Propiedades del Mapa antes de importar una hoja."));
		return false;
	}

	Ref<Texture2D> texture;
	Ref<TileSet> ts_res;
	String res_path;
	String err_msg;
	if (!ClockGearsTilePicker::import_tileset_resource(p_path, texture, ts_res, res_path, err_msg, 32)) {
		show_tileset_err(err_msg);
		return false;
	}

	int tab = (p_tab >= 0 && p_tab < PALETTE_TABS - 1) ? p_tab : CLAMP(tileset_tabs ? tileset_tabs->get_current_tab() : 0, 0, PALETTE_TABS - 2);
	if (palettes[tab]) {
		palettes[tab]->set_tile_size(32);
		palettes[tab]->set_tileset(texture);
	}
	if (tileset_tabs) {
		tileset_tabs->set_tab_tooltip(tab, res_path);
	}

	static const char *tab_slot_keys[5] = { "A5", "B", "C", "D", "E" };
	String slot_key = tab_slot_keys[CLAMP(tab, 0, 4)];

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Array ts_arr = dm->get_category_array(CAT_TILESETS);
		bool found_ts = false;
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				if (id == ts_id) {
					Dictionary slots = d.get("slots", Dictionary());
					slots[slot_key] = res_path;
					d["slots"] = slots;
					if (tab == 0 || String(d.get("graphic_path", "")).is_empty()) {
						d["graphic_path"] = res_path;
					}
					ts_arr[i] = d;
					found_ts = true;
					break;
				}
			}
		}
		if (found_ts) {
			dm->set_category_array(CAT_TILESETS, ts_arr);
			dm->save_all("res://clockgears_data");
		}
	}

	if (map) {
		map->setup_tileset_from_database(ts_id);
		if (!map->get_scene_file_path().is_empty()) {
			EditorNode::get_singleton()->save_scene_if_open(map->get_scene_file_path());
		}
	}

	load_persisted_tilesets(ts_id);
	_on_tile_selected(0, tab);
	print_line(vformat(String::utf8("[ClockGears Tileset] Tileset cargado en Pestaña %d (%s) para TileSet #%03d: '%s' (%dx%d px, %d tiles de 32x32)."),
			tab, slot_key, ts_id, res_path, texture->get_width(), texture->get_height(), (texture->get_width() / 32) * (texture->get_height() / 32)));
	return true;
}

void ClockGearsMapPanel::load_persisted_tilesets(int p_tileset_id) {
	int loaded_any = -1;
	Dictionary db_slots;
	String db_legacy_path;
	String tileset_name;
	int target_ts_id = p_tileset_id;

	if (target_ts_id == -1) {
		Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
		if (map) {
			target_ts_id = map->get_tileset_id();
		}
	}

	if (target_ts_id <= 0 && ClockGearsDataManager::get_singleton()) {
		Array ts_arr = ClockGearsDataManager::get_singleton()->get_category_array(CAT_TILESETS);
		if (!ts_arr.is_empty()) {
			target_ts_id = 1;
			Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
			ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
			if (map) {
				map->set_tileset_id(1);
			}
		}
	}

	if (ClockGearsDataManager::get_singleton() && target_ts_id > 0) {
		Array ts_arr = ClockGearsDataManager::get_singleton()->get_category_array(CAT_TILESETS);
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				if (id == target_ts_id) {
					tileset_name = d.get("name", vformat("Tileset %03d", id));
					db_slots = d.get("slots", Dictionary());
					db_legacy_path = d.get("graphic_path", "");
					break;
				}
			}
		}
	}

	if (target_ts_id <= 0 || (db_slots.is_empty() && db_legacy_path.is_empty())) {
		print_line(String::utf8("[ClockGears Paleta] AVISO: El mapa actual no tiene TileSet asignado o sus ranuras están vacías."));
		for (int tab = 0; tab < PALETTE_TABS - 1; tab++) {
			if (palettes[tab]) {
				palettes[tab]->set_tileset(Ref<Texture2D>());
				palettes[tab]->set_empty_message(CG_TR("Este mapa no tiene TileSet.\nAsígnalo en Propiedades del Mapa"));
				if (tileset_tabs) {
					tileset_tabs->set_tab_tooltip(tab, CG_TR("(Sin TileSet asignado)"));
				}
			}
		}
		if (CanvasItemEditor::get_singleton()) {
			CanvasItemEditor::get_singleton()->set_clockgears_selected_tile(-1, Vector2i(-1, -1), Ref<Texture2D>(), 0);
		}
		return;
	}

	print_line(vformat(String::utf8("[ClockGears Paleta] Cargando ranuras del Tileset #%03d ('%s') para el mapa actual..."), target_ts_id, tileset_name));

	for (int tab = 0; tab < PALETTE_TABS - 1; tab++) {
		if (!palettes[tab]) {
			continue;
		}
		String ts_path;
		// 1. Leer las ranuras A (A1..A5) y B, C, D, E de Base de Datos > Tilesets
		if (tab == 0) {
			static const char *a_keys[5] = { "A5", "A2", "A1", "A3", "A4" };
			for (int k = 0; k < 5; k++) {
				String cand = db_slots.get(a_keys[k], "");
				if (!cand.is_empty() && FileAccess::exists(cand)) {
					ts_path = cand;
					break;
				}
			}
			if (ts_path.is_empty() && !db_legacy_path.is_empty() && FileAccess::exists(db_legacy_path)) {
				ts_path = db_legacy_path;
			}
		} else {
			static const char *bcde_keys[4] = { "B", "C", "D", "E" };
			String cand = db_slots.get(bcde_keys[tab - 1], "");
			if (!cand.is_empty() && FileAccess::exists(cand)) {
				ts_path = cand;
			}
		}

		if (!ts_path.is_empty() && FileAccess::exists(ts_path)) {
			Ref<Texture2D> tex = ResourceLoader::load(ts_path);
			if (tex.is_null()) {
				Ref<Image> img;
				img.instantiate();
				if (ImageLoader::load_image(ts_path, img) == OK) {
					Ref<ImageTexture> itex = ImageTexture::create_from_image(img);
					itex->set_path(ts_path, true);
					tex = itex;
				}
			}
			if (tex.is_valid()) {
				palettes[tab]->set_tile_size(32);
				palettes[tab]->set_tileset(tex);
				if (tileset_tabs) {
					tileset_tabs->set_tab_tooltip(tab, ts_path);
				}
				if (loaded_any < 0) {
					loaded_any = tab;
				}
				continue;
			}
		}

		palettes[tab]->set_tileset(Ref<Texture2D>());
		palettes[tab]->set_empty_message(CG_TR("Ranura vacía en este TileSet"));
		if (tileset_tabs) {
			tileset_tabs->set_tab_tooltip(tab, CG_TR("(Ranura vacía)"));
		}
	}

	if (loaded_any >= 0) {
		int cur_tab = tileset_tabs ? tileset_tabs->get_current_tab() : 0;
		if (cur_tab < PALETTE_TABS - 1 && palettes[cur_tab] && palettes[cur_tab]->get_tileset().is_valid()) {
			_on_tile_selected(0, cur_tab);
		} else {
			_on_tile_selected(0, loaded_any);
		}
	}
}

void ClockGearsMapPanel::_on_tile_selected(int p_tile, int p_tab) {
	if (p_tab < 0 || p_tab >= PALETTE_TABS || !palettes[p_tab] || !CanvasItemEditor::get_singleton()) {
		return;
	}
	if (ClockGearsTopBar::get_singleton()) {
		ClockGearsTopBar::get_singleton()->set_map_mode();
	} else if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_event_mode(false);
	}
	if (palettes[p_tab]->is_region_palette_mode()) {
		CanvasItemEditor::get_singleton()->set_clockgears_region_mode(true, p_tile);
		return;
	}
	CanvasItemEditor::get_singleton()->set_clockgears_region_mode(false);
	int cols = palettes[p_tab]->get_columns();
	if (cols <= 0) {
		cols = 8;
	}
	Vector2i atlas_coords(p_tile % cols, p_tile / cols);
	Ref<Texture2D> tex = palettes[p_tab]->get_tileset();
	Vector<Vector2i> coords_list = palettes[p_tab]->get_selected_coords_list();
	Vector2i block_sz = palettes[p_tab]->get_selection_block_size();
	if (block_sz.x > 1 || block_sz.y > 1 || coords_list.size() > 1) {
		CanvasItemEditor::get_singleton()->set_clockgears_selected_tile_block(atlas_coords, block_sz, coords_list, tex, p_tab);
	} else {
		CanvasItemEditor::get_singleton()->set_clockgears_selected_tile(p_tile, atlas_coords, tex, p_tab);
	}
}

void ClockGearsMapPanel::_select_map_item(TreeItem *p_item) {
	if (!p_item) {
		return;
	}
	map_tree->set_selected(p_item);
	map_tree->scroll_to_item(p_item);
	_on_map_selected();
}

void ClockGearsMapPanel::_on_add_map() {
	if (map_properties_dialog) {
		map_properties_dialog->popup_for_new_map(map_counter + 1, 0);
	}
}

void ClockGearsMapPanel::_on_add_child_map() {
	_on_add_map();
}

void ClockGearsMapPanel::_on_edit_map_properties() {
	TreeItem *selected = map_tree->get_selected();
	if (!selected || selected == map_tree->get_root()) {
		return;
	}

	Dictionary meta = selected->get_metadata(0);
	int id = meta.get("id", 1);
	String name = meta.get("name", "MAP001");
	String display_name = meta.get("display_name", selected->get_text(0));
	String bgm = meta.get("bgm", "(Ninguna)");
	int width = meta.get("width", 25);
	int height = meta.get("height", 20);
	int tileset_id = meta.get("tileset_id", 0);

	if (map_properties_dialog) {
		map_properties_dialog->popup_for_edit_map(id, name, display_name, bgm, width, height, tileset_id);
	}
}

void ClockGearsMapPanel::_on_map_properties_saved(int p_id, const String &p_name, const String &p_display_name, const String &p_bgm, int p_width, int p_height, int p_tileset_id, bool p_is_new) {
	if (p_is_new) {
		pending_save_id = p_id;
		pending_save_name = p_name;
		pending_save_display_name = p_display_name;
		pending_save_bgm = p_bgm;
		pending_save_width = p_width;
		pending_save_height = p_height;
		pending_save_tileset_id = p_tileset_id;
		pending_save_is_new = true;
		_apply_map_properties(false);
		return;
	}

	TreeItem *item = map_tree->get_selected();
	int old_ts_id = 0;
	if (item) {
		Dictionary meta = item->get_metadata(0);
		old_ts_id = int(meta.get("tileset_id", 0));
	}

	pending_save_id = p_id;
	pending_save_name = p_name;
	pending_save_display_name = p_display_name;
	pending_save_bgm = p_bgm;
	pending_save_width = p_width;
	pending_save_height = p_height;
	pending_save_tileset_id = p_tileset_id;
	pending_save_is_new = false;

	if (p_tileset_id != old_ts_id) {
		Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
		bool has_painted_tiles = false;
		if (map) {
			for (int l = 0; l < 4; l++) {
				TileMapLayer *layer = map->get_layer(l);
				if (layer && !layer->get_used_cells().is_empty()) {
					has_painted_tiles = true;
					break;
				}
			}
		}

		if (has_painted_tiles && tileset_change_dialog) {
			tileset_change_dialog->set_text(CG_TR("El mapa ya contiene tiles pintados con el tileset anterior.\n¿Deseas conservar los tiles existentes o limpiarlos del mapa?"));
			tileset_change_dialog->popup_centered(Size2(520, 160) * EDSCALE);
			return;
		}
	}

	_apply_map_properties(false);
}

void ClockGearsMapPanel::_on_tileset_change_confirmed() {
	// "Conservar Tiles" seleccionado
	_apply_map_properties(false);
}

void ClockGearsMapPanel::_on_tileset_change_custom_action(const StringName &p_action) {
	if (p_action == "clear") {
		if (tileset_change_dialog) {
			tileset_change_dialog->hide();
		}
		_apply_map_properties(true);
	}
}

void ClockGearsMapPanel::_on_tileset_change_canceled() {
	// Cancelar: no aplicar cambios al tileset
}

void ClockGearsMapPanel::_apply_map_properties(bool p_clear_tiles) {
	TreeItem *root = map_tree->get_root();
	TreeItem *item = nullptr;
	String map_path = vformat("res://maps/map%03d.tscn", pending_save_id);

	if (pending_save_is_new) {
		if (pending_save_id > map_counter) {
			map_counter = pending_save_id;
		} else {
			map_counter++;
		}
		item = map_tree->create_item(root);

		// Asegurar existencia del directorio res://maps
		DirAccess::make_dir_recursive_absolute("res://maps");

		ClockGearsMap *map = memnew(ClockGearsMap);
		String internal_name = pending_save_name.strip_edges();
		if (internal_name.is_empty()) {
			internal_name = vformat("Map%03d", pending_save_id);
		}
		map->set_name(internal_name);
		map->set_map_name(pending_save_display_name);
		map->set_map_size(Vector2i(pending_save_width, pending_save_height));
		map->set_bgm_path(pending_save_bgm);
		map->set_tileset_id(pending_save_tileset_id);
		map->ensure_default_layers();
		if (pending_save_tileset_id > 0) {
			map->setup_tileset_from_database(pending_save_tileset_id);
		}

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

		if (ProjectSettings::get_singleton()) {
			String current_main = ProjectSettings::get_singleton()->get_setting("application/run/main_scene");
			if (current_main.is_empty() || pending_save_id == 1) {
				ProjectSettings::get_singleton()->set_setting("application/run/main_scene", map_path);
				ProjectSettings::get_singleton()->save();
			}
		}
	} else {
		item = map_tree->get_selected();
	}

	if (!item) {
		return;
	}

	item->set_text(0, vformat("%03d: %s", pending_save_id, pending_save_display_name));
	item->set_editable(0, false);

	String internal_name = pending_save_name.strip_edges();
	if (internal_name.is_empty()) {
		internal_name = vformat("Map%03d", pending_save_id);
	}

	Dictionary meta;
	meta["id"] = pending_save_id;
	meta["name"] = internal_name;
	meta["display_name"] = pending_save_display_name;
	meta["bgm"] = pending_save_bgm;
	meta["width"] = pending_save_width;
	meta["height"] = pending_save_height;
	meta["tileset_id"] = pending_save_tileset_id;
	meta["scene_path"] = map_path;
	item->set_metadata(0, meta);

	if (pending_save_is_new) {
		Node *current = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		if (current && !current->get_scene_file_path().is_empty()) {
			EditorNode::get_singleton()->save_scene_if_open(current->get_scene_file_path());
		}
		EditorNode::get_singleton()->open_scene(map_path);
	} else {
		Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
		if (map && map->get_scene_file_path() == map_path) {
			map->set_name(internal_name);
			map->set_map_name(pending_save_display_name);
			map->set_map_size(Vector2i(pending_save_width, pending_save_height));
			map->set_bgm_path(pending_save_bgm);
			map->set_tileset_id(pending_save_tileset_id);
			if (p_clear_tiles) {
				for (int l = 0; l < 4; l++) {
					TileMapLayer *layer = map->get_layer(l);
					if (layer) {
						layer->clear();
					}
				}
			}
			map->setup_tileset_from_database(pending_save_tileset_id);
			EditorNode::get_singleton()->save_scene_if_open(map_path);
		} else if (FileAccess::exists(map_path)) {
			Ref<PackedScene> sdata = ResourceLoader::load(map_path);
			if (sdata.is_valid()) {
				Node *inst = sdata->instantiate();
				ClockGearsMap *disk_map = Object::cast_to<ClockGearsMap>(inst);
				if (disk_map) {
					disk_map->set_name(internal_name);
					disk_map->set_map_name(pending_save_display_name);
					disk_map->set_map_size(Vector2i(pending_save_width, pending_save_height));
					disk_map->set_bgm_path(pending_save_bgm);
					disk_map->set_tileset_id(pending_save_tileset_id);
					if (p_clear_tiles) {
						for (int l = 0; l < 4; l++) {
							TileMapLayer *layer = disk_map->get_layer(l);
							if (layer) {
								layer->clear();
							}
						}
					}
					disk_map->setup_tileset_from_database(pending_save_tileset_id);
					for (int i = 0; i < disk_map->get_child_count(); i++) {
						Node *c = disk_map->get_child(i);
						if (c) {
							c->set_owner(disk_map);
						}
					}
					Ref<PackedScene> new_packed = memnew(PackedScene);
					new_packed->pack(disk_map);
					ResourceSaver::save(new_packed, map_path);
				}
				if (inst) {
					memdelete(inst);
				}
			}
		}
	}

	load_persisted_tilesets(pending_save_tileset_id);
	_select_map_item(item);
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}

int ClockGearsMapPanel::_count_maps(TreeItem *p_item) const {
	if (!p_item) {
		return 0;
	}
	int count = p_item->get_parent() ? 1 : 0;
	for (int i = 0; i < p_item->get_child_count(); i++) {
		count += _count_maps(p_item->get_child(i));
	}
	return count;
}

void ClockGearsMapPanel::_on_delete_map() {
	TreeItem *selected = map_tree->get_selected();
	if (!selected || selected == map_tree->get_root()) {
		return;
	}

	Dictionary meta = selected->get_metadata(0);
	String scene_path = meta.get("scene_path", "");

	TreeItem *parent = selected->get_parent();
	TreeItem *next = selected->get_next();
	if (!next) {
		next = selected->get_prev();
	}
	memdelete(selected);

	if (!scene_path.is_empty() && FileAccess::exists(scene_path)) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			da->remove(scene_path.get_file());
		}
	}

	if (next) {
		_select_map_item(next);
	} else if (parent && parent != map_tree->get_root()) {
		_select_map_item(parent);
	}
}

void ClockGearsMapPanel::_on_map_selected() {
	TreeItem *selected = map_tree->get_selected();
	if (!selected) {
		return;
	}
	Dictionary meta = selected->get_metadata(0);
	String scene_path = meta.get("scene_path", "");
	if (scene_path.is_empty()) {
		int id = meta.get("id", 1);
		scene_path = vformat("res://maps/map%03d.tscn", id);
	}

	if (!scene_path.is_empty() && FileAccess::exists(scene_path)) {
		Node *current = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
		if (!current || current->get_scene_file_path() != scene_path) {
			if (current && !current->get_scene_file_path().is_empty()) {
				EditorNode::get_singleton()->save_scene_if_open(current->get_scene_file_path());
			}
			EditorNode::get_singleton()->open_scene(scene_path);
		}
	}

	Node *edited = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(edited);
	int ts_id = 0;
	if (map) {
		if (meta.has("display_name")) {
			map->set_map_name(meta["display_name"]);
		} else {
			map->set_map_name(selected->get_text(0));
		}
		if (meta.has("tileset_id")) {
			map->set_tileset_id(int(meta["tileset_id"]));
		}
		ts_id = map->get_tileset_id();
		map->setup_tileset_from_database(ts_id);
	} else if (meta.has("tileset_id")) {
		ts_id = int(meta["tileset_id"]);
	}

	load_persisted_tilesets(ts_id);

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}

void ClockGearsMapPanel::refresh_maps_from_disk() {
	load_persisted_tilesets();
	if (!map_tree) {
		return;
	}
	TreeItem *root = map_tree->get_root();
	if (!root) {
		root = map_tree->create_item();
		root->set_text(0, "Mapas");
		root->set_selectable(0, false);
	}

	if (!DirAccess::exists("res://maps")) {
		return;
	}

	Ref<DirAccess> da = DirAccess::open("res://maps");
	if (da.is_null()) {
		return;
	}

	da->list_dir_begin();
	String f = da->get_next();
	Vector<String> map_files;
	while (!f.is_empty()) {
		if (!da->current_is_dir() && f.ends_with(".tscn")) {
			map_files.push_back(String("res://maps").path_join(f));
		}
		f = da->get_next();
	}
	da->list_dir_end();
	map_files.sort();

	if (map_files.is_empty()) {
		return;
	}

	// Clear existing items under root to avoid duplicates
	TreeItem *child = root->get_first_child();
	while (child) {
		TreeItem *next = child->get_next();
		memdelete(child);
		child = next;
	}

	TreeItem *first_item = nullptr;
	for (int i = 0; i < map_files.size(); i++) {
		String mpath = map_files[i];
		int id = i + 1;
		String base = mpath.get_file().get_basename();
		if (base.begins_with("map") || base.begins_with("Map")) {
			String num_part = base.substr(3);
			if (num_part.is_valid_int()) {
				id = num_part.to_int();
			}
		}
		if (id > map_counter) {
			map_counter = id;
		}

		String display_name = vformat("Mapa%03d", id);
		int w = 25;
		int h = 20;
		int tileset_id = 0;
		String bgm = "(Ninguna)";

		Ref<FileAccess> fa = FileAccess::open(mpath, FileAccess::READ);
		if (fa.is_valid()) {
			while (!fa->eof_reached()) {
				String line = fa->get_line();
				if (line.begins_with("map_name = ")) {
					display_name = line.substr(11).strip_edges().trim_prefix("\"").trim_suffix("\"");
				} else if (line.begins_with("map_size = Vector2i(")) {
					String sz = line.substr(20).trim_suffix(")");
					Vector<String> parts = sz.split(",");
					if (parts.size() >= 2) {
						w = parts[0].to_int();
						h = parts[1].to_int();
					}
				} else if (line.begins_with("bgm_path = ")) {
					bgm = line.substr(11).strip_edges().trim_prefix("\"").trim_suffix("\"");
				} else if (line.begins_with("tileset_id = ")) {
					tileset_id = line.substr(13).strip_edges().to_int();
				}
			}
		}

		TreeItem *item = map_tree->create_item(root);
		item->set_text(0, vformat("%03d: %s", id, display_name));
		item->set_editable(0, false);

		Dictionary meta;
		meta["id"] = id;
		meta["name"] = base;
		meta["display_name"] = display_name;
		meta["bgm"] = bgm;
		meta["width"] = w;
		meta["height"] = h;
		meta["tileset_id"] = tileset_id;
		meta["scene_path"] = mpath;
		item->set_metadata(0, meta);

		if (!first_item) {
			first_item = item;
		}
	}

	Node *curr_scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (curr_scene) {
		String curr_path = curr_scene->get_scene_file_path();
		TreeItem *it = root->get_first_child();
		while (it) {
			Dictionary meta = it->get_metadata(0);
			if (meta.get("scene_path", "") == curr_path) {
				map_tree->set_selected(it);
				break;
			}
			it = it->get_next();
		}
	} else if (first_item) {
		_select_map_item(first_item);
	}
}

void ClockGearsMapPanel::_on_map_edited() {
	_on_map_selected();
}

void ClockGearsMapPanel::_on_map_activated() {
	_on_edit_map_properties();
}

void ClockGearsMapPanel::_on_map_rmb_clicked(const Vector2 &p_pos, MouseButton p_button) {
	if (p_button == MouseButton::RIGHT) {
		if (map_context_menu) {
			map_context_menu->set_position(map_tree->get_screen_position() + p_pos);
			map_context_menu->reset_size();
			map_context_menu->popup();
		}
	}
}

void ClockGearsMapPanel::_on_context_menu_id_pressed(int p_id) {
	switch (p_id) {
		case 0: _on_add_map(); break;
		case 1: _on_edit_map_properties(); break;
		case 2: _on_delete_map(); break;
	}
}

ClockGearsMapPanel::ClockGearsMapPanel() {
	// Fixed-width left column: fill vertically, never steal width from the canvas.
	set_h_size_flags(Control::SIZE_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	add_theme_constant_override("separation", 0);

	// Map Properties Dialog
	map_properties_dialog = memnew(ClockGearsMapPropertiesDialog);
	map_properties_dialog->connect("map_properties_saved", callable_mp(this, &ClockGearsMapPanel::_on_map_properties_saved));
	add_child(map_properties_dialog);

	// Advanced Tile Picker Dialog (Fase 3)
	advanced_tile_picker_dialog = memnew(ClockGearsTilePickerDialog);
	add_child(advanced_tile_picker_dialog);

	// Context Menu
	map_context_menu = memnew(PopupMenu);
	map_context_menu->add_item(CG_TR("Nuevo Mapa..."), 0);
	map_context_menu->add_item(CG_TR("Editar Propiedades..."), 1);
	map_context_menu->add_separator();
	map_context_menu->add_item(CG_TR("Eliminar Mapa"), 2);
	map_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsMapPanel::_on_context_menu_id_pressed));
	add_child(map_context_menu);

	// Left column VSplitContainer: Paleta de Tiles (arriba) y Árbol de Mapas (abajo)
	VSplitContainer *vsplit = memnew(VSplitContainer);
	vsplit->set_name("ClockGearsLeftVSplit");
	vsplit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vsplit->add_theme_constant_override("separation", 8);
	// Both halves expand, so offset 0 places the divider exactly in the middle.
	vsplit->set_split_offset(0);
	add_child(vsplit);

	// ---------------------------------------------------------
	// Top half: tileset palette with A, B, C, D, E, R tabs.
	// ---------------------------------------------------------
	PanelContainer *tileset_panel = memnew(PanelContainer);
	tileset_panel->add_theme_style_override("panel", _clockgears_card_style());
	tileset_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vsplit->add_child(tileset_panel);

	VBoxContainer *tileset_box = memnew(VBoxContainer);
	tileset_box->add_theme_constant_override("separation", 6);
	tileset_panel->add_child(tileset_box);

	tileset_box->add_child(_make_section_header(CG_TR("PALETA DE TILES"), true));

	tileset_tabs = memnew(TabContainer);
	tileset_tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tileset_tabs->connect("tab_changed", callable_mp(this, &ClockGearsMapPanel::_on_palette_tab_changed));
	tileset_box->add_child(tileset_tabs);

	const char tab_letters[] = { 'A', 'B', 'C', 'D', 'E', 'R' };
	for (int i = 0; i < PALETTE_TABS; i++) {
		palettes[i] = memnew(ClockGearsTilePalette);
		if (i == 5) {
			palettes[i]->set_region_palette(true);
		}
		palettes[i]->connect("tile_selected", callable_mp(this, &ClockGearsMapPanel::_on_tile_selected).bind(i));
		tileset_tabs->add_child(palettes[i]);
		tileset_tabs->set_tab_title(i, String::chr(tab_letters[i]));
		if (i == 5) {
			tileset_tabs->set_tab_tooltip(i, CG_TR("Pestaña R: Regiones (0-255) (F8)"));
		} else {
			tileset_tabs->set_tab_tooltip(i, vformat(CG_TR("Pestaña de Tiles %s"), String::chr(tab_letters[i])));
		}
	}

	// ---------------------------------------------------------
	// Bottom half: hierarchical map tree.
	// ---------------------------------------------------------
	PanelContainer *maps_panel = memnew(PanelContainer);
	maps_panel->add_theme_style_override("panel", _clockgears_card_style());
	maps_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	maps_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vsplit->add_child(maps_panel);

	VBoxContainer *maps_box = memnew(VBoxContainer);
	maps_box->add_theme_constant_override("separation", 6);
	maps_panel->add_child(maps_box);

	maps_box->add_child(_make_section_header(CG_TR("ÁRBOL DE MAPAS"), false));

	map_tree = memnew(Tree);
	map_tree->set_hide_root(true);
	map_tree->set_columns(1);
	map_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	map_tree->set_allow_rmb_select(true);
	map_tree->set_allow_reselect(true);
	map_tree->connect("item_selected", callable_mp(this, &ClockGearsMapPanel::_on_map_selected));
	map_tree->connect("item_edited", callable_mp(this, &ClockGearsMapPanel::_on_map_edited));
	map_tree->connect("item_activated", callable_mp(this, &ClockGearsMapPanel::_on_map_activated));
	map_tree->connect("item_mouse_selected", callable_mp(this, &ClockGearsMapPanel::_on_map_rmb_clicked));
	maps_box->add_child(map_tree);

	TreeItem *root = map_tree->create_item();
	root->set_text(0, "Mapas");
	root->set_selectable(0, false);
	// Regla Cero (Bug 4d): El árbol de mapas inicia 100% vacío, sin "001: Pueblo Inicial" ni datos de prueba.

	tileset_file_dialog = memnew(FileDialog);
	tileset_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	tileset_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	tileset_file_dialog->add_filter("*.png, *.jpg, *.jpeg, *.webp ; Tilesets 32x32 (*.png, *.webp)");
	tileset_file_dialog->set_title(CG_TR("Importar Tileset (32x32)"));
	tileset_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsMapPanel::_on_tileset_file_selected));
	add_child(tileset_file_dialog);

	tileset_error_dialog = memnew(AcceptDialog);
	tileset_error_dialog->set_title(String::utf8("Importación de Tileset (32x32)"));
	tileset_error_dialog->set_ok_button_text(String::utf8("Entendido"));
	add_child(tileset_error_dialog);

	tileset_change_dialog = memnew(ConfirmationDialog);
	tileset_change_dialog->set_title(CG_TR("Cambiar TileSet del Mapa"));
	tileset_change_dialog->set_ok_button_text(CG_TR("Conservar Tiles"));
	tileset_change_dialog->set_cancel_button_text(CG_TR("Cancelar"));
	tileset_change_dialog->add_button(CG_TR("Limpiar Tiles"), true, "clear");
	tileset_change_dialog->connect("confirmed", callable_mp(this, &ClockGearsMapPanel::_on_tileset_change_confirmed));
	tileset_change_dialog->connect("custom_action", callable_mp(this, &ClockGearsMapPanel::_on_tileset_change_custom_action));
	tileset_change_dialog->connect("canceled", callable_mp(this, &ClockGearsMapPanel::_on_tileset_change_canceled));
	add_child(tileset_change_dialog);

	singleton = this;
}

ClockGearsMapPanel::~ClockGearsMapPanel() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
