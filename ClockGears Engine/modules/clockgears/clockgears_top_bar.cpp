#include "clockgears_top_bar.h"

#include "clockgears_database.h"
#include "clockgears_editor_plugin.h"
#include "clockgears_event_editor_dialog.h"
#include "clockgears_simplified_export_dialog.h"
#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "editor/clockgears_project_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "editor/docks/scene_tree_dock.h"
#include "icons/clockgears_icons.h"
#include "scene/gui/base_button.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

ClockGearsTopBar *ClockGearsTopBar::singleton = nullptr;

namespace {

// Iconos conservados temporalmente (sin PNG nuevo todavía): estrella (eventos), luna (sombra), selección punteada.
const char *CLOCKGEARS_EVENT_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M12 2.2l2.8 6.1 6.7.7-5 4.5 1.4 6.6-5.9-3.4-5.9 3.4 1.4-6.6-5-4.5 6.7-.7z\"/>"
		"</svg>";

const char *CLOCKGEARS_SHADOW_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#A7AFC0\" d=\"M16.2 2.6a10 10 0 1 0 5.2 17.2A8.5 8.5 0 0 1 16.2 2.6z\"/>"
		"</svg>";

const char *CLOCKGEARS_SELECT_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M3 3h5v2H5v3H3V3zm13 0h5v5h-2V5h-3V3zM3 16h2v3h3v2H3v-5zm16 3v-3h2v5h-5v-2h3zM9 3h6v2H9V3zm0 16h6v2H9v-2zM3 9h2v6H3V9zm16 0h2v6h-2V9z\"/>"
		"</svg>";

int _get_int_scale() {
	return MAX(1, (int)Math::floor(EDSCALE + 0.001f));
}

} // namespace

void ClockGearsTopBar::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_undo_pressed"), &ClockGearsTopBar::_on_undo_pressed);
	ClassDB::bind_method(D_METHOD("_on_redo_pressed"), &ClockGearsTopBar::_on_redo_pressed);
	ClassDB::bind_method(D_METHOD("_on_database_pressed"), &ClockGearsTopBar::_on_database_pressed);
	ClassDB::bind_method(D_METHOD("_on_map_mode_pressed"), &ClockGearsTopBar::_on_map_mode_pressed);
	ClassDB::bind_method(D_METHOD("_on_event_mode_pressed"), &ClockGearsTopBar::_on_event_mode_pressed);
	ClassDB::bind_method(D_METHOD("_on_zoom_in_pressed"), &ClockGearsTopBar::_on_zoom_in_pressed);
	ClassDB::bind_method(D_METHOD("_on_zoom_out_pressed"), &ClockGearsTopBar::_on_zoom_out_pressed);
	ClassDB::bind_method(D_METHOD("_on_playtest_pressed"), &ClockGearsTopBar::_on_playtest_pressed);
	ClassDB::bind_method(D_METHOD("_relayout_rows"), &ClockGearsTopBar::_relayout_rows);
}

Ref<Texture2D> ClockGearsTopBar::_make_svg_icon(const String &p_svg, int p_int_scale) const {
	Ref<Image> image;
	image.instantiate();
	if (image->load_svg_from_buffer(p_svg.to_utf8_buffer(), float(MAX(1, p_int_scale))) != OK || image->is_empty()) {
		return Ref<Texture2D>();
	}
	return ImageTexture::create_from_image(image);
}

Ref<Texture2D> ClockGearsTopBar::_get_png_icon(const String &p_name, int p_int_scale) const {
	if (ClockGearsIcons::get_singleton()) {
		return ClockGearsIcons::get_singleton()->get_icon(p_name, float(MAX(1, p_int_scale)));
	}
	return Ref<Texture2D>();
}

void ClockGearsTopBar::_apply_toolbar_button_style(Button *p_button, const Ref<Theme> &p_editor_theme, int p_int_scale) const {
	const int radius = 6 * p_int_scale;
	const int margin_h = 7 * p_int_scale;
	const int margin_v = 4 * p_int_scale;

	Color accent(0.88f, 0.63f, 0.25f);
	if (p_editor_theme.is_valid() && p_editor_theme->has_color(SNAME("accent_color"), EditorStringName(Editor))) {
		accent = p_editor_theme->get_color(SNAME("accent_color"), EditorStringName(Editor));
	}

	Ref<StyleBoxFlat> normal;
	normal.instantiate();
	normal->set_bg_color(Color(1, 1, 1, 0.0));
	normal->set_border_width_all(1);
	normal->set_border_color(Color(1, 1, 1, 0.0));
	normal->set_corner_radius_all(radius);
	normal->set_content_margin_individual(margin_h, margin_v, margin_h, margin_v);

	Ref<StyleBoxFlat> hover;
	hover.instantiate();
	hover->set_bg_color(Color(1, 1, 1, 0.11f));
	hover->set_border_width_all(1);
	hover->set_border_color(Color(1, 1, 1, 0.16f));
	hover->set_corner_radius_all(radius);
	hover->set_content_margin_individual(margin_h, margin_v, margin_h, margin_v);

	Ref<StyleBoxFlat> pressed;
	pressed.instantiate();
	pressed->set_bg_color(Color(accent.r, accent.g, accent.b, 0.30f));
	pressed->set_border_width_all(1);
	pressed->set_border_color(Color(accent.r, accent.g, accent.b, 0.85f));
	pressed->set_corner_radius_all(radius);
	pressed->set_content_margin_individual(margin_h, margin_v, margin_h, margin_v);

	Ref<StyleBoxFlat> hover_pressed;
	hover_pressed.instantiate();
	hover_pressed->set_bg_color(Color(accent.r, accent.g, accent.b, 0.40f));
	hover_pressed->set_border_width_all(1);
	hover_pressed->set_border_color(Color(accent.r, accent.g, accent.b, 0.95f));
	hover_pressed->set_corner_radius_all(radius);
	hover_pressed->set_content_margin_individual(margin_h, margin_v, margin_h, margin_v);

	Ref<StyleBoxFlat> disabled;
	disabled.instantiate();
	disabled->set_bg_color(Color(1, 1, 1, 0.0));
	disabled->set_border_width_all(1);
	disabled->set_border_color(Color(1, 1, 1, 0.0));
	disabled->set_corner_radius_all(radius);
	disabled->set_content_margin_individual(margin_h, margin_v, margin_h, margin_v);

	Ref<StyleBoxEmpty> focus_empty;
	focus_empty.instantiate();

	p_button->add_theme_style_override("normal", normal);
	p_button->add_theme_style_override("hover", hover);
	p_button->add_theme_style_override("pressed", pressed);
	p_button->add_theme_style_override("hover_pressed", hover_pressed);
	p_button->add_theme_style_override("disabled", disabled);
	p_button->add_theme_style_override("focus", focus_empty);

	// NO teñir los iconos: conservan su color propio; al deshabilitarse van al 40%.
	p_button->add_theme_color_override("icon_normal_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_hover_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_hover_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_focus_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_disabled_color", Color(1, 1, 1, 0.40f));

	const Color text_normal(0.93f, 0.94f, 0.96f, 1.0f);
	const Color text_disabled(0.93f, 0.94f, 0.96f, 0.40f);
	p_button->add_theme_color_override("font_color", text_normal);
	p_button->add_theme_color_override("font_hover_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("font_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("font_hover_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("font_focus_color", text_normal);
	p_button->add_theme_color_override("font_disabled_color", text_disabled);

	// 32 px (o multiplo entero exacto) + filtro nearest sin suavizado.
	p_button->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	p_button->add_theme_constant_override("icon_max_width", 32 * p_int_scale);
	p_button->add_theme_constant_override("h_separation", 5 * p_int_scale);
	p_button->add_theme_font_size_override(SceneStringName(font_size), 11 * p_int_scale);
	p_button->set_custom_minimum_size(Size2(0, 32 * p_int_scale + margin_v * 2));
}

Button *ClockGearsTopBar::_make_labeled_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_label, const String &p_tooltip, const Callable &p_action, const Ref<Theme> &p_editor_theme, int p_int_scale) {
	Button *button = memnew(Button);
	button->set_button_icon(p_icon);
	button->set_text(p_label);
	button->set_tooltip_text(p_tooltip);
	button->set_focus_mode(Control::FOCUS_NONE);
	button->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	_apply_toolbar_button_style(button, p_editor_theme, p_int_scale);
	if (p_action.is_valid()) {
		button->connect(SceneStringName(pressed), p_action);
	}
	p_parent->add_child(button);
	return button;
}

Button *ClockGearsTopBar::_make_labeled_toggle_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_label, const String &p_tooltip, const Ref<ButtonGroup> &p_group, const Ref<Theme> &p_editor_theme, int p_int_scale) {
	Button *button = _make_labeled_button(p_parent, p_icon, p_label, p_tooltip, Callable(), p_editor_theme, p_int_scale);
	button->set_toggle_mode(true);
	button->set_button_group(p_group);
	return button;
}

void ClockGearsTopBar::_relayout_rows() {
	if (!row1_flow || !row2_flow || all_groups.is_empty()) {
		return;
	}

	const int int_scale = _get_int_scale();
	const float h_sep = 16.0f * int_scale;
	const float avail_width = get_size().x - 20.0f * int_scale;

	float single_row_width = 0.0f;
	for (int i = 0; i < all_groups.size(); i++) {
		single_row_width += all_groups[i]->get_combined_minimum_size().x;
		if (i > 0) {
			single_row_width += h_sep;
		}
	}

	const bool need_two_rows = (avail_width <= 0.0f) || (avail_width < single_row_width);

	for (int i = 0; i < all_groups.size(); i++) {
		HBoxContainer *grp = all_groups[i];
		HFlowContainer *target = (!need_two_rows || i < split_group_index) ? row1_flow : row2_flow;
		if (grp->get_parent() != target) {
			if (grp->get_parent()) {
				grp->get_parent()->remove_child(grp);
			}
			target->add_child(grp);
		}
		target->move_child(grp, target == row1_flow ? i : (i - split_group_index));
	}

	row2_flow->set_visible(need_two_rows);
	two_row_mode = need_two_rows;
	queue_redraw();
}

void ClockGearsTopBar::_draw_flow_separators(HFlowContainer *p_flow, int p_int_scale) {
	if (!p_flow || !p_flow->is_visible_in_tree()) {
		return;
	}
	const Color sep_color(1, 1, 1, 0.14f);
	const float sep_w = MAX(1.0f, float(p_int_scale));
	const Vector2 self_pos = get_global_position();

	Control *prev = nullptr;
	Rect2 prev_rect;
	for (int i = 0; i < p_flow->get_child_count(); i++) {
		Control *curr = Object::cast_to<Control>(p_flow->get_child(i));
		if (!curr || !curr->is_visible()) {
			continue;
		}
		Rect2 curr_rect = curr->get_global_rect();
		curr_rect.position -= self_pos;
		if (prev && Math::abs(prev_rect.position.y - curr_rect.position.y) < 6.0f) {
			const float gap_x = Math::round((prev_rect.position.x + prev_rect.size.x + curr_rect.position.x) * 0.5f);
			const float y_top = curr_rect.position.y + 5.0f * p_int_scale;
			const float y_bot = curr_rect.position.y + curr_rect.size.y - 5.0f * p_int_scale;
			if (y_bot > y_top) {
				draw_line(Point2(gap_x, y_top), Point2(gap_x, y_bot), sep_color, sep_w);
			}
		}
		prev = curr;
		prev_rect = curr_rect;
	}
}

void ClockGearsTopBar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_RESIZED: {
			const float w = get_size().x;
			if (Math::abs(w - last_layout_width) >= 1.0f) {
				last_layout_width = w;
				call_deferred("_relayout_rows");
			}
		} break;
		case NOTIFICATION_DRAW: {
			const int int_scale = _get_int_scale();
			_draw_flow_separators(row1_flow, int_scale);
			_draw_flow_separators(row2_flow, int_scale);
		} break;
	}
}

void ClockGearsTopBar::_on_undo_pressed() {
	EditorUndoRedoManager::get_singleton()->undo();
}

void ClockGearsTopBar::_on_redo_pressed() {
	EditorUndoRedoManager::get_singleton()->redo();
}

void ClockGearsTopBar::_on_cut_pressed() {
	if (CanvasItemEditor::get_singleton() && !CanvasItemEditor::get_singleton()->is_clockgears_event_mode()) {
		CanvasItemEditor::get_singleton()->clockgears_copy_map_selection();
		CanvasItemEditor::get_singleton()->clockgears_delete_map_selection();
		return;
	}
	if (SceneTreeDock::get_singleton()) {
		SceneTreeDock::get_singleton()->cut_selected_nodes();
	}
}

void ClockGearsTopBar::_on_copy_pressed() {
	if (CanvasItemEditor::get_singleton() && !CanvasItemEditor::get_singleton()->is_clockgears_event_mode()) {
		CanvasItemEditor::get_singleton()->clockgears_copy_map_selection();
		return;
	}
	if (SceneTreeDock::get_singleton()) {
		SceneTreeDock::get_singleton()->copy_selected_nodes();
	}
}

void ClockGearsTopBar::_on_paste_pressed() {
	if (CanvasItemEditor::get_singleton() && !CanvasItemEditor::get_singleton()->is_clockgears_event_mode()) {
		CanvasItemEditor::get_singleton()->clockgears_paste_map_selection();
		return;
	}
	if (SceneTreeDock::get_singleton()) {
		SceneTreeDock::get_singleton()->paste_nodes_from_clipboard();
	}
}

void ClockGearsTopBar::_on_folder_menu_id_pressed(int p_id) {
	if (p_id == 100) {
		String root_path = ProjectSettings::get_singleton()->globalize_path("res://");
		OS::get_singleton()->shell_open(root_path);
		return;
	}
	if (p_id == 101) {
		if (ClockGearsEditorPlugin::get_singleton()) {
			ClockGearsEditorPlugin::get_singleton()->open_resource_manager();
		}
		return;
	}

	const char *folder_rel = nullptr;
	switch (p_id) {
		case 1: folder_rel = "img/characters"; break;
		case 2: folder_rel = "img/tilesets"; break;
		case 3: folder_rel = "img/faces"; break;
		case 4: folder_rel = "img/portraits"; break;
		case 5: folder_rel = "img/battlers"; break;
		case 6: folder_rel = "img/pictures"; break;
		case 7: folder_rel = "img/cutscenes"; break;
		case 8: folder_rel = "img/animations"; break;
		case 9: folder_rel = "img/parallaxes"; break;
		case 10: folder_rel = "img/battlebacks"; break;
		case 11: folder_rel = "img/titles"; break;
		case 12: folder_rel = "img/system"; break;
		case 13: folder_rel = "img/ui"; break;
		case 20: folder_rel = "audio/bgm"; break;
		case 21: folder_rel = "audio/bgs"; break;
		case 22: folder_rel = "audio/me"; break;
		case 23: folder_rel = "audio/se"; break;
		case 24: folder_rel = "audio/voice"; break;
		case 30: folder_rel = "maps"; break;
		case 31: folder_rel = "clockgears_data"; break;
		case 32: folder_rel = "fonts"; break;
		case 33: folder_rel = "movies"; break;
		default: break;
	}
	if (folder_rel) {
		String full_path = ClockGearsProjectStructure::resolve_project_folder_global(folder_rel);
		OS::get_singleton()->shell_open(full_path);
	}
}

void ClockGearsTopBar::shortcut_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo()) {
		return;
	}
	switch (k->get_keycode()) {
		case Key::F5:
		case Key::F12: {
			if (k->get_keycode() == Key::F5 && k->is_shift_pressed()) {
				_on_map_mode_pressed();
				if (layer_buttons[0]) {
					layer_buttons[0]->set_pressed(true);
				}
				_on_layer_pressed(1);
			} else {
				_on_playtest_pressed();
			}
			accept_event();
		} break;
		case Key::F6: {
			if (k->is_shift_pressed()) {
				_on_map_mode_pressed();
				if (layer_buttons[1]) {
					layer_buttons[1]->set_pressed(true);
				}
				_on_layer_pressed(2);
			} else {
				_on_event_mode_pressed();
			}
			accept_event();
		} break;
		case Key::F7: {
			_on_map_mode_pressed();
			if (layer_buttons[2]) {
				layer_buttons[2]->set_pressed(true);
			}
			_on_layer_pressed(3);
			accept_event();
		} break;
		case Key::F8: {
			_on_map_mode_pressed();
			if (layer_buttons[5]) {
				layer_buttons[5]->set_pressed(true);
			}
			_on_layer_pressed(6);
			accept_event();
		} break;
		case Key::F9: {
			_on_database_pressed();
			accept_event();
		} break;
		default:
			break;
	}
}

void ClockGearsTopBar::_on_database_pressed() {
	if (database_dialog) {
		database_dialog->open_database();
	}
}

void ClockGearsTopBar::set_map_mode() {
	_on_map_mode_pressed();
}

void ClockGearsTopBar::_on_map_mode_pressed() {
	if (map_mode_btn) {
		map_mode_btn->set_pressed(true);
	}
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_event_mode(false);
	}
}

void ClockGearsTopBar::_on_event_mode_pressed() {
	if (event_mode_btn) {
		event_mode_btn->set_pressed(true);
	}
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_event_mode(true);
	}
}

void ClockGearsTopBar::_on_layer_pressed(int p_layer) {
	if (map_mode_btn) {
		map_mode_btn->set_pressed(true);
		if (CanvasItemEditor::get_singleton()) {
			CanvasItemEditor::get_singleton()->set_clockgears_event_mode(false);
		}
	}
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_current_layer(p_layer);
	}
}

void ClockGearsTopBar::_on_tool_pressed(int p_tool) {
	if (map_mode_btn) {
		map_mode_btn->set_pressed(true);
		if (CanvasItemEditor::get_singleton()) {
			CanvasItemEditor::get_singleton()->set_clockgears_event_mode(false);
		}
	}
	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_draw_tool(p_tool);
	}
}

void ClockGearsTopBar::_on_zoom_in_pressed() {
	CanvasItemEditor *editor = CanvasItemEditor::get_singleton();
	if (editor) {
		editor->set_view_zoom(editor->get_zoom() * 1.25);
	}
}

void ClockGearsTopBar::_on_zoom_out_pressed() {
	CanvasItemEditor *editor = CanvasItemEditor::get_singleton();
	if (editor) {
		editor->set_view_zoom(editor->get_zoom() / 1.25);
	}
}

void ClockGearsTopBar::_on_playtest_pressed() {
	if (ClockGearsEditorPlugin::get_singleton()) {
		ClockGearsEditorPlugin::get_singleton()->start_playtest();
	} else {
		EditorNode::get_singleton()->clockgears_play_main_scene();
	}
}

void ClockGearsTopBar::set_dialogs(ClockGearsDatabaseDialog *p_db, ClockGearsEventEditorDialog *p_ev, ClockGearsSimplifiedExportDialog *p_exp) {
	database_dialog = p_db;
	event_editor_dialog = p_ev;
	export_dialog = p_exp;
}

ClockGearsTopBar::ClockGearsTopBar() {
	singleton = this;
	set_process_shortcut_input(true);

	const int int_scale = _get_int_scale();

	Ref<StyleBoxFlat> bar_style;
	bar_style.instantiate();
	bar_style->set_bg_color(Color(0.088f, 0.096f, 0.115f));
	bar_style->set_corner_radius_all(10 * int_scale);
	bar_style->set_content_margin_individual(10 * int_scale, 5 * int_scale, 10 * int_scale, 5 * int_scale);
	bar_style->set_border_width_all(1);
	bar_style->set_border_color(Color(1, 1, 1, 0.07f));
	add_theme_style_override(SceneStringName(panel), bar_style);

	rows_vbox = memnew(VBoxContainer);
	rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rows_vbox->add_theme_constant_override("separation", 5 * int_scale);
	add_child(rows_vbox);

	row1_flow = memnew(HFlowContainer);
	row1_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	row1_flow->add_theme_constant_override("h_separation", 16 * int_scale);
	row1_flow->add_theme_constant_override("v_separation", 4 * int_scale);
	row1_flow->connect("sort_children", callable_mp((CanvasItem *)this, &CanvasItem::queue_redraw));
	rows_vbox->add_child(row1_flow);

	row2_flow = memnew(HFlowContainer);
	row2_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	row2_flow->add_theme_constant_override("h_separation", 16 * int_scale);
	row2_flow->add_theme_constant_override("v_separation", 4 * int_scale);
	row2_flow->connect("sort_children", callable_mp((CanvasItem *)this, &CanvasItem::queue_redraw));
	rows_vbox->add_child(row2_flow);

	Ref<Theme> editor_theme = EditorNode::get_singleton()->get_editor_theme();
	const auto editor_icon = [&editor_theme](const StringName &p_name) -> Ref<Texture2D> {
		return editor_theme->get_icon(p_name, SNAME("EditorIcons"));
	};
	const auto make_group = [int_scale, this]() -> HBoxContainer * {
		HBoxContainer *g = memnew(HBoxContainer);
		g->add_theme_constant_override("separation", 3 * int_scale);
		all_groups.push_back(g);
		return g;
	};

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 0: Archivo
	// ------------------------------------------------------------------
	HBoxContainer *grp_file = make_group();
	_make_labeled_button(grp_file, _get_png_icon("nuevo_mapa_icono", int_scale), CG_TR("Nuevo Mapa"), CG_TR("Nuevo Mapa"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_new_scene), editor_theme, int_scale);
	_make_labeled_button(grp_file, _get_png_icon("abrir_mapa_icono", int_scale), CG_TR("Abrir Mapa"), CG_TR("Abrir Mapa"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_open_scene), editor_theme, int_scale);
	_make_labeled_button(grp_file, _get_png_icon("guardar_proyecto_icono", int_scale), CG_TR("Guardar"), CG_TR("Guardar"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_save_scene), editor_theme, int_scale);

	folders_btn = memnew(MenuButton);
	folders_btn->set_flat(false);
	folders_btn->set_button_icon(_get_png_icon("cargar_proyecto_icono", int_scale));
	folders_btn->set_text(CG_TR("Carpetas"));
	folders_btn->set_tooltip_text(CG_TR("Carpetas del Proyecto y Gestor de Recursos (F10)"));
	folders_btn->set_focus_mode(Control::FOCUS_NONE);
	folders_btn->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	_apply_toolbar_button_style(folders_btn, editor_theme, int_scale);
	grp_file->add_child(folders_btn);

	folders_popup = folders_btn->get_popup();
	folders_popup->connect("id_pressed", callable_mp(this, &ClockGearsTopBar::_on_folder_menu_id_pressed));

	folders_popup->add_item(CG_TR("📁 Abrir Carpeta Raíz del Proyecto (res://)"), 100);
	folders_popup->add_item(CG_TR("📦 Gestor de Recursos... (F10)"), 101);
	folders_popup->add_separator(String::utf8("Imágenes (img/)"));
	folders_popup->add_item(String::utf8("img/characters (Personajes y NPCs)"), 1);
	folders_popup->add_item(String::utf8("img/tilesets (Tilesets 32x32)"), 2);
	folders_popup->add_item(String::utf8("img/faces (Caras / Rostros)"), 3);
	folders_popup->add_item(String::utf8("img/portraits (Retratos)"), 4);
	folders_popup->add_item(String::utf8("img/battlers (Enemigos y Batalla)"), 5);
	folders_popup->add_item(String::utf8("img/pictures (Ilustraciones)"), 6);
	folders_popup->add_item(String::utf8("img/cutscenes (Cinemáticas)"), 7);
	folders_popup->add_item(String::utf8("img/animations (Animaciones)"), 8);
	folders_popup->add_item(String::utf8("img/parallaxes (Parallax)"), 9);
	folders_popup->add_item(String::utf8("img/battlebacks (Fondos de Batalla)"), 10);
	folders_popup->add_item(String::utf8("img/titles (Títulos)"), 11);
	folders_popup->add_item(String::utf8("img/system (Iconos y Sistema)"), 12);
	folders_popup->add_item(String::utf8("img/ui (Diseñador UI/HUD)"), 13);
	folders_popup->add_separator(String::utf8("Audio (audio/)"));
	folders_popup->add_item(String::utf8("audio/bgm (Música de Fondo)"), 20);
	folders_popup->add_item(String::utf8("audio/bgs (Sonidos Ambientales)"), 21);
	folders_popup->add_item(String::utf8("audio/me (Fanfarrias / Efectos)"), 22);
	folders_popup->add_item(String::utf8("audio/se (Efectos de Sonido)"), 23);
	folders_popup->add_item(String::utf8("audio/voice (Voces / Blips)"), 24);
	folders_popup->add_separator(String::utf8("Proyecto y Datos"));
	folders_popup->add_item(String::utf8("maps (Mapas)"), 30);
	folders_popup->add_item(String::utf8("clockgears_data (Base de Datos JSON)"), 31);
	folders_popup->add_item(String::utf8("fonts (Fuentes)"), 32);
	folders_popup->add_item(String::utf8("movies (Vídeos)"), 33);

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 1: Edición e historial
	// ------------------------------------------------------------------
	HBoxContainer *grp_edit = make_group();
	_make_labeled_button(grp_edit, editor_icon(SNAME("ActionCut")), CG_TR("Cortar"), CG_TR("Cortar"), callable_mp(this, &ClockGearsTopBar::_on_cut_pressed), editor_theme, int_scale);
	_make_labeled_button(grp_edit, editor_icon(SNAME("ActionCopy")), CG_TR("Copiar"), CG_TR("Copiar"), callable_mp(this, &ClockGearsTopBar::_on_copy_pressed), editor_theme, int_scale);
	_make_labeled_button(grp_edit, editor_icon(SNAME("ActionPaste")), CG_TR("Pegar"), CG_TR("Pegar"), callable_mp(this, &ClockGearsTopBar::_on_paste_pressed), editor_theme, int_scale);
	undo_btn = _make_labeled_button(grp_edit, _get_png_icon("deshacer_icono", int_scale), CG_TR("Deshacer"), CG_TR("Deshacer (Ctrl+Z)"), callable_mp(this, &ClockGearsTopBar::_on_undo_pressed), editor_theme, int_scale);
	redo_btn = _make_labeled_button(grp_edit, _get_png_icon("rehacer_icono", int_scale), CG_TR("Rehacer"), CG_TR("Rehacer (Ctrl+Y)"), callable_mp(this, &ClockGearsTopBar::_on_redo_pressed), editor_theme, int_scale);

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 2: Base de Datos y Modos
	// ------------------------------------------------------------------
	HBoxContainer *grp_db_modes = make_group();
	database_btn = _make_labeled_button(grp_db_modes, _get_png_icon("database_icon_mainmenu", int_scale), CG_TR("Base de Datos"), CG_TR("Base de Datos - RPG Maker Paradigm (F9)"), callable_mp(this, &ClockGearsTopBar::_on_database_pressed), editor_theme, int_scale);

	Ref<ButtonGroup> mode_group;
	mode_group.instantiate();
	map_mode_btn = _make_labeled_toggle_button(grp_db_modes, _get_png_icon("modo_mapa_icono", int_scale), CG_TR("Modo Mapa"), CG_TR("Modo Mapa (F5)"), mode_group, editor_theme, int_scale);
	map_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_map_mode_pressed));
	map_mode_btn->set_pressed(true);

	event_mode_btn = _make_labeled_toggle_button(grp_db_modes, _make_svg_icon(CLOCKGEARS_EVENT_SVG, int_scale), CG_TR("Modo Eventos"), CG_TR("Modo Eventos (F6)"), mode_group, editor_theme, int_scale);
	event_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_event_mode_pressed));

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 3: Capas (1..4, A, R)
	// ------------------------------------------------------------------
	HBoxContainer *grp_layers = make_group();
	Ref<ButtonGroup> layer_group;
	layer_group.instantiate();
	const char *layer_labels[6] = { "1", "2", "3", "4", "A", "R" };
	const char *layer_icons[6] = { "capa1_icono", "capa2_icono", "capa3_icono", "capa4_icono", "capa_auto_icono", "capa_r_icono" };
	const char *layer_tooltips[6] = { "Capa 1: Suelo (F5)", "Capa 2: Decoración (Shift+F6)", "Capa 3: Objetos (F7)", "Capa 4: Techo (★)", "Capa Automática", "Capa R: Regiones 0-255 (F8)" };
	for (int i = 0; i < 6; i++) {
		layer_buttons[i] = _make_labeled_toggle_button(grp_layers, _get_png_icon(layer_icons[i], int_scale), layer_labels[i], CG_TR(layer_tooltips[i]), layer_group, editor_theme, int_scale);
		layer_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_layer_pressed).bind(i + 1));
	}
	layer_buttons[0]->set_pressed(true);

	// ------------------------------------------------------------------
	// Fila 2 (en 2 filas) - Grupos 4 y 5: Herramientas (dibujo y mapa)
	// ------------------------------------------------------------------
	split_group_index = all_groups.size(); // 4: a partir de aqui pasan a fila 2 cuando no cabe todo en 1 fila

	Ref<ButtonGroup> tool_group;
	tool_group.instantiate();

	HBoxContainer *grp_tools_draw = make_group();
	tool_buttons[0] = _make_labeled_toggle_button(grp_tools_draw, _get_png_icon("lapiz_icono", int_scale), CG_TR("Lápiz"), CG_TR("Lápiz (Individual o Bloque)"), tool_group, editor_theme, int_scale);
	tool_buttons[1] = _make_labeled_toggle_button(grp_tools_draw, _get_png_icon("rectangulo_icono", int_scale), CG_TR("Rectángulo"), CG_TR("Rectángulo"), tool_group, editor_theme, int_scale);
	tool_buttons[2] = _make_labeled_toggle_button(grp_tools_draw, _get_png_icon("elipse_icono", int_scale), CG_TR("Elipse"), CG_TR("Elipse"), tool_group, editor_theme, int_scale);
	tool_buttons[3] = _make_labeled_toggle_button(grp_tools_draw, _get_png_icon("rellenar", int_scale), CG_TR("Cubo de Pintura"), CG_TR("Cubo de Pintura (Flood Fill)"), tool_group, editor_theme, int_scale);
	tool_buttons[4] = _make_labeled_toggle_button(grp_tools_draw, _make_svg_icon(CLOCKGEARS_SHADOW_SVG, int_scale), CG_TR("Sombra"), CG_TR("Pincel de Sombra"), tool_group, editor_theme, int_scale);

	HBoxContainer *grp_tools_map = make_group();
	tool_buttons[5] = _make_labeled_toggle_button(grp_tools_map, _make_svg_icon(CLOCKGEARS_SELECT_SVG, int_scale), CG_TR("Selección"), CG_TR("Selección Rectangular (Ctrl+C / Ctrl+V / Supr)"), tool_group, editor_theme, int_scale);
	tool_buttons[6] = _make_labeled_toggle_button(grp_tools_map, _get_png_icon("posicion_jugador", int_scale), CG_TR("Pos. Jugador"), CG_TR("Posición Inicial del Jugador (Clic para colocar / Clic repetido para rotar dirección)"), tool_group, editor_theme, int_scale);
	tool_buttons[7] = _make_labeled_toggle_button(grp_tools_map, _get_png_icon("goma_borrar", int_scale), CG_TR("Goma de Borrar"), CG_TR("Goma de Borrar (Eliminar tiles con clic o arrastre)"), tool_group, editor_theme, int_scale);

	for (int i = 0; i < 8; i++) {
		tool_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_tool_pressed).bind(i));
	}
	tool_buttons[0]->set_pressed(true);

	// ------------------------------------------------------------------
	// Fila 2 (en 2 filas) - Grupos 6 y 7: Zoom y Ejecutar
	// ------------------------------------------------------------------
	HBoxContainer *grp_zoom = make_group();
	_make_labeled_button(grp_zoom, _get_png_icon("zoom_icono", int_scale), CG_TR("Acercar"), CG_TR("Acercar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_in_pressed), editor_theme, int_scale);
	_make_labeled_button(grp_zoom, _get_png_icon("alejar_icono", int_scale), CG_TR("Alejar"), CG_TR("Alejar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_out_pressed), editor_theme, int_scale);

	HBoxContainer *grp_play = make_group();
	playtest_btn = _make_labeled_button(grp_play, _get_png_icon("test_game", int_scale), CG_TR("Ejecutar"), CG_TR("Probar Juego (Playtest F5)"), callable_mp(this, &ClockGearsTopBar::_on_playtest_pressed), editor_theme, int_scale);

	_relayout_rows();
}

ClockGearsTopBar::~ClockGearsTopBar() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
