#include "clockgears_top_bar.h"

#include "clockgears_database.h"
#include "clockgears_editor_plugin.h"
#include "clockgears_event_editor_dialog.h"
#include "clockgears_simplified_export_dialog.h"
#include "core/clockgears_data_manager.h"
#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "editor/clockgears_project_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/docks/scene_tree_dock.h"
#include "icons/clockgears_icons.h"
#include "scene/gui/base_button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/margin_container.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

ClockGearsTopBar *ClockGearsTopBar::singleton = nullptr;

namespace {

// Iconos conservados temporalmente (sin PNG nuevo todavía): luna (sombra), selección punteada.
const char *CLOCKGEARS_SHADOW_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#A7AFC0\" d=\"M16.2 2.6a10 10 0 1 0 5.2 17.2A8.5 8.5 0 0 1 16.2 2.6z\"/>"
		"</svg>";

const char *CLOCKGEARS_SELECT_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M3 3h5v2H5v3H3V3zm13 0h5v5h-2V5h-3V3zM3 16h2v3h3v2H3v-5zm16 3v-3h2v5h-5v-2h3zM9 3h6v2H9V3zm0 16h6v2H9v-2zM3 9h2v6H3V9zm16 0h2v6h-2V9z\"/>"
		"</svg>";

int _get_ui_scale_int() {
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
	ClassDB::bind_method(D_METHOD("sync_active_layer_display"), &ClockGearsTopBar::sync_active_layer_display);
	ClassDB::bind_method(D_METHOD("get_active_layer_badge_button"), &ClockGearsTopBar::get_active_layer_badge_button);
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
	const int ui_scale = _get_ui_scale_int();
	const int radius = 6 * ui_scale;
	const int margin_h = 4 * ui_scale;
	const int margin_v = 4 * ui_scale;
	const int btn_side = 32 * p_int_scale + margin_h * 2;

	Color accent(0.88f, 0.63f, 0.25f);
	if (p_editor_theme.is_valid() && p_editor_theme->has_color(SNAME("accent_color"), EditorStringName(Editor))) {
		accent = p_editor_theme->get_color(SNAME("accent_color"), EditorStringName(Editor));
	}

	Ref<StyleBoxFlat> normal;
	normal.instantiate();
	normal->set_anti_aliased(true);
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

	// 32 px (o multiplo entero exacto) + filtro nearest sin suavizado, solo icono centrado.
	p_button->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	p_button->add_theme_constant_override("icon_max_width", 32 * p_int_scale);
	p_button->add_theme_constant_override("h_separation", 0);
	p_button->set_custom_minimum_size(Size2(btn_side, 32 * p_int_scale + margin_v * 2));
}

Button *ClockGearsTopBar::_make_icon_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_action, const Ref<Theme> &p_editor_theme, int p_int_scale) {
	Button *button = memnew(Button);
	button->set_button_icon(p_icon);
	button->set_text(String());
	button->set_tooltip_text(p_tooltip);
	button->set_focus_mode(Control::FOCUS_NONE);
	button->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_apply_toolbar_button_style(button, p_editor_theme, p_int_scale);
	if (p_action.is_valid()) {
		button->connect(SceneStringName(pressed), p_action);
	}
	p_parent->add_child(button);
	return button;
}

Button *ClockGearsTopBar::_make_icon_toggle_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Ref<ButtonGroup> &p_group, const Ref<Theme> &p_editor_theme, int p_int_scale) {
	Button *button = _make_icon_button(p_parent, p_icon, p_tooltip, Callable(), p_editor_theme, p_int_scale);
	button->set_toggle_mode(true);
	button->set_button_group(p_group);
	return button;
}

Control *ClockGearsTopBar::_make_group_separator(int p_int_scale) const {
	MarginContainer *sep_wrap = memnew(MarginContainer);
	sep_wrap->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	sep_wrap->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	sep_wrap->add_theme_constant_override("margin_left", 0);
	sep_wrap->add_theme_constant_override("margin_right", 0);
	sep_wrap->add_theme_constant_override("margin_top", 6 * p_int_scale);
	sep_wrap->add_theme_constant_override("margin_bottom", 6 * p_int_scale);

	ColorRect *line = memnew(ColorRect);
	line->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	line->set_color(Color(1, 1, 1, 0.14f));
	line->set_custom_minimum_size(Size2(MAX(1, p_int_scale), 0));
	line->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	sep_wrap->add_child(line);
	return sep_wrap;
}

void ClockGearsTopBar::_relayout_rows() {
	if (!row1_box || !row2_box || all_groups.is_empty()) {
		return;
	}

	const int ui_scale = _get_ui_scale_int();
	const float row_sep = float(4 * ui_scale);
	const float avail_width = get_size().x - bar_h_padding;

	float single_row_width = 0.0f;
	for (int i = 0; i < all_groups.size(); i++) {
		if (i > 0) {
			single_row_width += row_sep;
			if (i - 1 < all_separators.size()) {
				single_row_width += all_separators[i - 1]->get_combined_minimum_size().x + row_sep;
			}
		}
		single_row_width += all_groups[i]->get_combined_minimum_size().x;
	}

	const int want_state = (avail_width <= 0.0f || avail_width + 0.5f < single_row_width) ? 1 : 0;
	if (want_state == last_two_row_state) {
		return;
	}
	last_two_row_state = want_state;
	const bool need_two_rows = (want_state == 1);

	for (int i = 0; i < all_groups.size(); i++) {
		Node *p = all_groups[i]->get_parent();
		if (p) {
			p->remove_child(all_groups[i]);
		}
	}
	for (int i = 0; i < all_separators.size(); i++) {
		Node *p = all_separators[i]->get_parent();
		if (p) {
			p->remove_child(all_separators[i]);
		}
	}

	for (int i = 0; i < all_groups.size(); i++) {
		const bool on_second_row = need_two_rows && i >= split_group_index;
		HBoxContainer *target = on_second_row ? row2_box : row1_box;

		const bool is_first_in_row = on_second_row ? (i == split_group_index) : (i == 0);
		if (!is_first_in_row && i - 1 >= 0 && i - 1 < all_separators.size()) {
			target->add_child(all_separators[i - 1]);
		}
		target->add_child(all_groups[i]);
	}

	row2_box->set_visible(need_two_rows);
}

void ClockGearsTopBar::_refresh_icon_scale() {
	const int icon_scale = ClockGearsIcons::get_editor_icon_scale(this);
	last_icon_scale = icon_scale;
	Ref<Theme> editor_theme = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_theme() : Ref<Theme>();
	for (int i = 0; i < icon_entries.size(); i++) {
		const ToolbarIconEntry &entry = icon_entries[i];
		if (!entry.button) {
			continue;
		}
		if (!entry.png_name.is_empty()) {
			entry.button->set_button_icon(_get_png_icon(entry.png_name, icon_scale));
		} else if (!entry.svg_src.is_empty()) {
			entry.button->set_button_icon(_make_svg_icon(entry.svg_src, icon_scale));
		} else if (entry.editor_icon_name != StringName() && editor_theme.is_valid()) {
			entry.button->set_button_icon(editor_theme->get_icon(entry.editor_icon_name, SNAME("EditorIcons")));
		}
		_apply_toolbar_button_style(entry.button, editor_theme, icon_scale);
	}
	last_two_row_state = -1;
	last_layout_width = -1.0f;
	call_deferred("_relayout_rows");
}

void ClockGearsTopBar::_on_editor_settings_changed() {
	const int cur_scale = ClockGearsIcons::get_editor_icon_scale(this);
	if (cur_scale != last_icon_scale) {
		_refresh_icon_scale();
	}
}

void ClockGearsTopBar::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		if (EditorSettings::get_singleton() && !EditorSettings::get_singleton()->is_connected("settings_changed", callable_mp(this, &ClockGearsTopBar::_on_editor_settings_changed))) {
			EditorSettings::get_singleton()->connect("settings_changed", callable_mp(this, &ClockGearsTopBar::_on_editor_settings_changed));
		}
		if (ClockGearsDataManager::get_singleton()) {
			Callable cb = callable_mp(this, &ClockGearsTopBar::_on_dm_data_changed_topbar);
			if (!ClockGearsDataManager::get_singleton()->is_connected("data_changed", cb)) {
				ClockGearsDataManager::get_singleton()->connect("data_changed", cb);
			}
		}
		sync_active_layer_display();
		if (ClockGearsIcons::get_editor_icon_scale(this) != last_icon_scale) {
			_refresh_icon_scale();
		}
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		if (EditorSettings::get_singleton() && EditorSettings::get_singleton()->is_connected("settings_changed", callable_mp(this, &ClockGearsTopBar::_on_editor_settings_changed))) {
			EditorSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &ClockGearsTopBar::_on_editor_settings_changed));
		}
		if (ClockGearsDataManager::get_singleton()) {
			Callable cb = callable_mp(this, &ClockGearsTopBar::_on_dm_data_changed_topbar);
			if (ClockGearsDataManager::get_singleton()->is_connected("data_changed", cb)) {
				ClockGearsDataManager::get_singleton()->disconnect("data_changed", cb);
			}
		}
	} else if (p_what == NOTIFICATION_RESIZED) {
		const float w = get_size().x;
		if (Math::abs(w - last_layout_width) >= 1.0f) {
			last_layout_width = w;
			call_deferred("_relayout_rows");
		}
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
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		if (!dm->get_editor_custom_layer().is_empty() || dm->get_editor_layer() != p_layer) {
			dm->set_editor_layer(p_layer);
		}
	}
	sync_active_layer_display();
}

void ClockGearsTopBar::_on_active_layer_badge_pressed() {
	if (ClockGearsEditorPlugin::get_singleton()) {
		ClockGearsEditorPlugin::get_singleton()->open_layers_dialog();
	}
}

void ClockGearsTopBar::_on_dm_data_changed_topbar(const String &p_category, int p_id) {
	if (p_category == "custom_layers" || p_category == "system" || p_category.is_empty()) {
		sync_active_layer_display();
	}
}

void ClockGearsTopBar::sync_active_layer_display() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String full_name = dm ? dm->get_active_editor_layer_display_name() : String("Capa 1: Suelo");
	String custom_id = dm ? dm->get_editor_custom_layer() : String();
	int base_idx = dm ? dm->get_editor_layer() : 1;

	if (custom_id.is_empty()) {
		for (int i = 0; i < 6; i++) {
			if (layer_buttons[i]) {
				layer_buttons[i]->set_pressed_no_signal((i + 1) == base_idx);
			}
		}
	} else {
		for (int i = 0; i < 6; i++) {
			if (layer_buttons[i]) {
				layer_buttons[i]->set_pressed_no_signal(false);
			}
		}
	}

	if (active_layer_badge_btn) {
		String trimmed = ClockGearsDataManager::truncate_with_ellipsis(full_name, 22);
		active_layer_badge_btn->set_text(vformat(CG_TR("Capa: %s"), trimmed));
		active_layer_badge_btn->set_tooltip_text(vformat(CG_TR("Capa activa actual: %s\nHaz clic para gestionar capas personalizadas (hasta 16)."), full_name));
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

	const int ui_scale = _get_ui_scale_int();
	const int int_scale = ClockGearsIcons::get_editor_icon_scale(this);
	last_icon_scale = int_scale;

	Ref<StyleBoxFlat> bar_style;
	bar_style.instantiate();
	bar_style->set_anti_aliased(true);
	bar_style->set_bg_color(Color(0.088f, 0.096f, 0.115f));
	bar_style->set_corner_radius_all(6 * ui_scale);
	bar_style->set_content_margin_individual(6 * ui_scale, 3 * ui_scale, 6 * ui_scale, 3 * ui_scale);
	bar_style->set_border_width_all(1);
	bar_style->set_border_color(Color(1, 1, 1, 0.08f));
	add_theme_style_override(SceneStringName(panel), bar_style);
	bar_h_padding = float(12 * ui_scale + 2);

	Ref<StyleBoxEmpty> scroll_empty;
	scroll_empty.instantiate();

	scroll_container = memnew(ScrollContainer);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	scroll_container->set_follow_focus(false);
	scroll_container->add_theme_style_override(SceneStringName(panel), scroll_empty);
	add_child(scroll_container);

	rows_vbox = memnew(VBoxContainer);
	rows_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rows_vbox->add_theme_constant_override("separation", 4 * ui_scale);
	scroll_container->add_child(rows_vbox);

	row1_box = memnew(HBoxContainer);
	row1_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	row1_box->add_theme_constant_override("separation", 4 * ui_scale);
	rows_vbox->add_child(row1_box);

	row2_box = memnew(HBoxContainer);
	row2_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	row2_box->add_theme_constant_override("separation", 4 * ui_scale);
	rows_vbox->add_child(row2_box);

	EditorNode *en = EditorNode::get_singleton();
	Ref<Theme> editor_theme = en ? en->get_editor_theme() : Ref<Theme>();
	const auto editor_icon = [&editor_theme](const StringName &p_name) -> Ref<Texture2D> {
		return editor_theme.is_valid() ? editor_theme->get_icon(p_name, SNAME("EditorIcons")) : Ref<Texture2D>();
	};
	const auto make_group = [ui_scale, this]() -> HBoxContainer * {
		HBoxContainer *g = memnew(HBoxContainer);
		g->add_theme_constant_override("separation", 4 * ui_scale);
		all_groups.push_back(g);
		return g;
	};
	const auto track_png = [this](Button *p_btn, const String &p_png) -> Button * {
		ToolbarIconEntry e;
		e.button = p_btn;
		e.png_name = p_png;
		icon_entries.push_back(e);
		return p_btn;
	};
	const auto track_svg = [this](Button *p_btn, const String &p_svg) -> Button * {
		ToolbarIconEntry e;
		e.button = p_btn;
		e.svg_src = p_svg;
		icon_entries.push_back(e);
		return p_btn;
	};
	const auto track_editor = [this](Button *p_btn, const StringName &p_ed) -> Button * {
		ToolbarIconEntry e;
		e.button = p_btn;
		e.editor_icon_name = p_ed;
		icon_entries.push_back(e);
		return p_btn;
	};

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 0: Archivo
	// ------------------------------------------------------------------
	HBoxContainer *grp_file = make_group();
	track_png(_make_icon_button(grp_file, _get_png_icon("nuevo_mapa_icono", int_scale), CG_TR("Nuevo Mapa (Ctrl+N)"), en ? callable_mp(en, &EditorNode::clockgears_new_scene) : Callable(), editor_theme, int_scale), "nuevo_mapa_icono");
	track_png(_make_icon_button(grp_file, _get_png_icon("abrir_mapa_icono", int_scale), CG_TR("Abrir Mapa (Ctrl+O)"), en ? callable_mp(en, &EditorNode::clockgears_open_scene) : Callable(), editor_theme, int_scale), "abrir_mapa_icono");
	track_png(_make_icon_button(grp_file, _get_png_icon("guardar_proyecto_icono", int_scale), CG_TR("Guardar (Ctrl+S)"), en ? callable_mp(en, &EditorNode::clockgears_save_scene) : Callable(), editor_theme, int_scale), "guardar_proyecto_icono");

	folders_btn = memnew(MenuButton);
	folders_btn->set_flat(false);
	folders_btn->set_button_icon(_get_png_icon("cargar_proyecto_icono", int_scale));
	folders_btn->set_text(String());
	folders_btn->set_tooltip_text(CG_TR("Carpetas del Proyecto y Gestor de Recursos (F10)"));
	folders_btn->set_focus_mode(Control::FOCUS_NONE);
	folders_btn->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_apply_toolbar_button_style(folders_btn, editor_theme, int_scale);
	grp_file->add_child(folders_btn);
	track_png(folders_btn, "cargar_proyecto_icono");

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
	track_editor(_make_icon_button(grp_edit, editor_icon(SNAME("ActionCut")), CG_TR("Cortar"), callable_mp(this, &ClockGearsTopBar::_on_cut_pressed), editor_theme, int_scale), SNAME("ActionCut"));
	track_editor(_make_icon_button(grp_edit, editor_icon(SNAME("ActionCopy")), CG_TR("Copiar (Ctrl+C)"), callable_mp(this, &ClockGearsTopBar::_on_copy_pressed), editor_theme, int_scale), SNAME("ActionCopy"));
	track_editor(_make_icon_button(grp_edit, editor_icon(SNAME("ActionPaste")), CG_TR("Pegar (Ctrl+V)"), callable_mp(this, &ClockGearsTopBar::_on_paste_pressed), editor_theme, int_scale), SNAME("ActionPaste"));
	undo_btn = track_png(_make_icon_button(grp_edit, _get_png_icon("deshacer_icono", int_scale), CG_TR("Deshacer (Ctrl+Z)"), callable_mp(this, &ClockGearsTopBar::_on_undo_pressed), editor_theme, int_scale), "deshacer_icono");
	redo_btn = track_png(_make_icon_button(grp_edit, _get_png_icon("rehacer_icono", int_scale), CG_TR("Rehacer (Ctrl+Y)"), callable_mp(this, &ClockGearsTopBar::_on_redo_pressed), editor_theme, int_scale), "rehacer_icono");

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 2: Base de Datos y Modos
	// ------------------------------------------------------------------
	HBoxContainer *grp_db_modes = make_group();
	database_btn = track_png(_make_icon_button(grp_db_modes, _get_png_icon("database_icon_mainmenu", int_scale), CG_TR("Base de Datos - RPG Maker Paradigm (F9)"), callable_mp(this, &ClockGearsTopBar::_on_database_pressed), editor_theme, int_scale), "database_icon_mainmenu");

	Ref<ButtonGroup> mode_group;
	mode_group.instantiate();
	map_mode_btn = track_png(_make_icon_toggle_button(grp_db_modes, _get_png_icon("modo_mapa_icono", int_scale), CG_TR("Modo Mapa (F5)"), mode_group, editor_theme, int_scale), "modo_mapa_icono");
	map_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_map_mode_pressed));
	map_mode_btn->set_pressed(true);

	event_mode_btn = track_png(_make_icon_toggle_button(grp_db_modes, _get_png_icon("modo_evento", int_scale), CG_TR("Modo Eventos (F6)"), mode_group, editor_theme, int_scale), "modo_evento");
	event_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_event_mode_pressed));

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 3: Capas (1..4, A, R)
	// ------------------------------------------------------------------
	HBoxContainer *grp_layers = make_group();
	Ref<ButtonGroup> layer_group;
	layer_group.instantiate();
	const char *layer_icons[6] = { "capa1_icono", "capa2_icono", "capa3_icono", "capa4_icono", "capa_auto_icono", "capa_r_icono" };
	const char *layer_tooltips[6] = { "Capa 1: Suelo (F5)", "Capa 2: Decoración (Shift+F6)", "Capa 3: Objetos (F7)", "Capa 4: Techo (★)", "Capa Automática", "Capa R: Regiones 0-255 (F8)" };
	for (int i = 0; i < 6; i++) {
		layer_buttons[i] = track_png(_make_icon_toggle_button(grp_layers, _get_png_icon(layer_icons[i], int_scale), CG_TR(layer_tooltips[i]), layer_group, editor_theme, int_scale), layer_icons[i]);
		layer_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_layer_pressed).bind(i + 1));
	}
	layer_buttons[0]->set_pressed(true);

	active_layer_badge_btn = memnew(Button);
	active_layer_badge_btn->set_focus_mode(Control::FOCUS_NONE);
	active_layer_badge_btn->set_tooltip_text(CG_TR("Capa activa actual (clic para abrir Capas personalizadas)"));
	{
		Ref<StyleBoxFlat> badge_n;
		badge_n.instantiate();
		badge_n->set_anti_aliased(true);
		badge_n->set_bg_color(Color(0.14f, 0.16f, 0.21f, 0.92f));
		badge_n->set_border_width_all(1);
		badge_n->set_border_color(Color(0.88f, 0.63f, 0.25f, 0.55f));
		badge_n->set_corner_radius_all(6 * ui_scale);
		badge_n->set_content_margin_individual(10 * ui_scale, 4 * ui_scale, 10 * ui_scale, 4 * ui_scale);

		Ref<StyleBoxFlat> badge_h = badge_n->duplicate();
		badge_h->set_bg_color(Color(0.20f, 0.23f, 0.30f, 0.96f));
		badge_h->set_border_color(Color(0.95f, 0.72f, 0.30f, 0.92f));

		Ref<StyleBoxEmpty> badge_focus;
		badge_focus.instantiate();

		active_layer_badge_btn->add_theme_style_override("normal", badge_n);
		active_layer_badge_btn->add_theme_style_override("hover", badge_h);
		active_layer_badge_btn->add_theme_style_override("pressed", badge_h);
		active_layer_badge_btn->add_theme_style_override("focus", badge_focus);
		active_layer_badge_btn->add_theme_color_override("font_color", Color(0.95f, 0.87f, 0.68f));
		active_layer_badge_btn->add_theme_color_override("font_hover_color", Color(1.0f, 0.94f, 0.78f));
	}
	active_layer_badge_btn->set_custom_minimum_size(Size2(110 * ui_scale, 32 * int_scale + 8 * ui_scale));
	active_layer_badge_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_active_layer_badge_pressed));
	grp_layers->add_child(active_layer_badge_btn);
	sync_active_layer_display();

	// ------------------------------------------------------------------
	// Fila 2 (en 2 filas) - Grupos 4 y 5: Herramientas (dibujo y mapa)
	// ------------------------------------------------------------------
	split_group_index = all_groups.size(); // 4: a partir de aqui pasan a fila 2 cuando no cabe todo en 1 fila

	Ref<ButtonGroup> tool_group;
	tool_group.instantiate();

	HBoxContainer *grp_tools_draw = make_group();
	tool_buttons[0] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("lapiz_icono", int_scale), CG_TR("Lápiz (Individual o Bloque)"), tool_group, editor_theme, int_scale), "lapiz_icono");
	tool_buttons[1] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("rectangulo_icono", int_scale), CG_TR("Rectángulo"), tool_group, editor_theme, int_scale), "rectangulo_icono");
	tool_buttons[2] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("elipse_icono", int_scale), CG_TR("Elipse"), tool_group, editor_theme, int_scale), "elipse_icono");
	tool_buttons[3] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("rellenar", int_scale), CG_TR("Cubo de Pintura (Flood Fill)"), tool_group, editor_theme, int_scale), "rellenar");
	tool_buttons[4] = track_svg(_make_icon_toggle_button(grp_tools_draw, _make_svg_icon(CLOCKGEARS_SHADOW_SVG, int_scale), CG_TR("Pincel de Sombra"), tool_group, editor_theme, int_scale), CLOCKGEARS_SHADOW_SVG);

	HBoxContainer *grp_tools_map = make_group();
	tool_buttons[5] = track_svg(_make_icon_toggle_button(grp_tools_map, _make_svg_icon(CLOCKGEARS_SELECT_SVG, int_scale), CG_TR("Selección Rectangular (Ctrl+C / Ctrl+V / Supr)"), tool_group, editor_theme, int_scale), CLOCKGEARS_SELECT_SVG);
	tool_buttons[6] = track_png(_make_icon_toggle_button(grp_tools_map, _get_png_icon("posicion_jugador", int_scale), CG_TR("Posición Inicial del Jugador (Clic para colocar / Clic repetido para rotar dirección)"), tool_group, editor_theme, int_scale), "posicion_jugador");
	tool_buttons[7] = track_png(_make_icon_toggle_button(grp_tools_map, _get_png_icon("goma_borrar", int_scale), CG_TR("Goma de Borrar (Eliminar tiles con clic o arrastre)"), tool_group, editor_theme, int_scale), "goma_borrar");

	for (int i = 0; i < 8; i++) {
		tool_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_tool_pressed).bind(i));
	}
	tool_buttons[0]->set_pressed(true);

	// ------------------------------------------------------------------
	// Fila 2 (en 2 filas) - Grupos 6 y 7: Zoom y Ejecutar
	// ------------------------------------------------------------------
	HBoxContainer *grp_zoom = make_group();
	track_png(_make_icon_button(grp_zoom, _get_png_icon("zoom_icono", int_scale), CG_TR("Acercar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_in_pressed), editor_theme, int_scale), "zoom_icono");
	track_png(_make_icon_button(grp_zoom, _get_png_icon("alejar_icono", int_scale), CG_TR("Alejar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_out_pressed), editor_theme, int_scale), "alejar_icono");

	HBoxContainer *grp_play = make_group();
	playtest_btn = track_png(_make_icon_button(grp_play, _get_png_icon("test_game", int_scale), CG_TR("Probar Juego (Playtest F5)"), callable_mp(this, &ClockGearsTopBar::_on_playtest_pressed), editor_theme, int_scale), "test_game");

	for (int i = 0; i < all_groups.size() - 1; i++) {
		all_separators.push_back(_make_group_separator(ui_scale));
	}

	_relayout_rows();
}

ClockGearsTopBar::~ClockGearsTopBar() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
