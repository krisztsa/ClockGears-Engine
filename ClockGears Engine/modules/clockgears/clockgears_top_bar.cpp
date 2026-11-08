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
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/base_button.h"
#include "scene/gui/separator.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box.h"
#include "scene/resources/theme.h"

ClockGearsTopBar *ClockGearsTopBar::singleton = nullptr;

namespace {

const char *CLOCKGEARS_GEAR_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\" viewBox=\"0 0 32 32\">"
		"<path fill=\"#E8B84B\" d=\"M13.5 1.5h5l1 4.2 2.6 1.1 3.4-2.7 3.5 3.5-2.7 3.4 1.1 2.6 4.2 1v5l-4.2 1-1.1 2.6 2.7 3.4-3.5 3.5-3.4-2.7-2.6 1.1-1 4.2h-5l-1-4.2-2.6-1.1-3.4 2.7-3.5-3.5 2.7-3.4-1.1-2.6-4.2-1v-5l4.2-1 1.1-2.6-2.7-3.4 3.5-3.5 3.4 2.7 2.6-1.1z\"/>"
		"<circle cx=\"16\" cy=\"16\" r=\"7.4\" fill=\"#1A1E26\"/>"
		"<circle cx=\"16\" cy=\"16\" r=\"3.2\" fill=\"#E8B84B\"/>"
		"</svg>";

const char *CLOCKGEARS_EVENT_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M12 2.2l2.8 6.1 6.7.7-5 4.5 1.4 6.6-5.9-3.4-5.9 3.4 1.4-6.6-5-4.5 6.7-.7z\"/>"
		"</svg>";

const char *CLOCKGEARS_PENCIL_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M3.2 20.8l1-4.4L16.5 4.1l3.4 3.4L7.6 19.8z\"/>"
		"<path fill=\"#1A1E26\" d=\"M15.4 5.2l3.4 3.4 1.6-1.6a1.4 1.4 0 0 0 0-2l-1.4-1.4a1.4 1.4 0 0 0-2 0z\"/>"
		"</svg>";

const char *CLOCKGEARS_ELLIPSE_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<ellipse cx=\"12\" cy=\"12\" rx=\"8.2\" ry=\"6.4\" fill=\"none\" stroke=\"#E8B84B\" stroke-width=\"2\"/>"
		"</svg>";

const char *CLOCKGEARS_SHADOW_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#A7AFC0\" d=\"M16.2 2.6a10 10 0 1 0 5.2 17.2A8.5 8.5 0 0 1 16.2 2.6z\"/>"
		"</svg>";

const char *CLOCKGEARS_SELECT_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#E8B84B\" d=\"M3 3h5v2H5v3H3V3zm13 0h5v5h-2V5h-3V3zM3 16h2v3h3v2H3v-5zm16 3v-3h2v5h-5v-2h3zM9 3h6v2H9V3zm0 16h6v2H9v-2zM3 9h2v6H3V9zm16 0h2v6h-2V9z\"/>"
		"</svg>";

const char *CLOCKGEARS_START_POS_SVG =
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
		"<path fill=\"#5CE67E\" d=\"M5 2h2v20H5V2zm3 1h12l-3.2 4.5L20 12H8V3z\"/>"
		"<circle cx=\"15\" cy=\"17\" r=\"2.5\" fill=\"#E8B84B\"/>"
		"<path fill=\"#E8B84B\" d=\"M11.5 22c0-2 1.6-3.2 3.5-3.2s3.5 1.2 3.5 3.2h-7z\"/>"
		"</svg>";

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
}

Ref<Texture2D> ClockGearsTopBar::_make_svg_icon(const String &p_svg) const {
	Ref<Image> image;
	image.instantiate();
	if (image->load_svg_from_buffer(p_svg.to_utf8_buffer(), 1.0) != OK || image->is_empty()) {
		return Ref<Texture2D>();
	}
	return ImageTexture::create_from_image(image);
}

void ClockGearsTopBar::_apply_toolbar_button_style(Button *p_button) const {
	const int radius = 8 * EDSCALE;
	const int margin = 3 * EDSCALE;

	Ref<StyleBoxFlat> normal;
	normal.instantiate();
	normal->set_bg_color(Color(1, 1, 1, 0.0));
	normal->set_corner_radius_all(radius);
	normal->set_content_margin_all(margin);

	Ref<StyleBoxFlat> hover;
	hover.instantiate();
	hover->set_bg_color(Color(1, 1, 1, 0.075));
	hover->set_corner_radius_all(radius);
	hover->set_content_margin_all(margin);

	Ref<StyleBoxFlat> pressed;
	pressed.instantiate();
	pressed->set_bg_color(Color(0.95, 0.72, 0.26, 0.24));
	pressed->set_corner_radius_all(radius);
	pressed->set_content_margin_all(margin);

	Ref<StyleBoxFlat> disabled;
	disabled.instantiate();
	disabled->set_bg_color(Color(1, 1, 1, 0.0));
	disabled->set_corner_radius_all(radius);
	disabled->set_content_margin_all(margin);

	Ref<StyleBoxEmpty> focus_empty;
	focus_empty.instantiate();

	p_button->add_theme_style_override("normal", normal);
	p_button->add_theme_style_override("hover", hover);
	p_button->add_theme_style_override("pressed", pressed);
	p_button->add_theme_style_override("hover_pressed", pressed);
	p_button->add_theme_style_override("disabled", disabled);
	p_button->add_theme_style_override("focus", focus_empty);
}

Button *ClockGearsTopBar::_make_icon_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_action) {
	Button *button = memnew(Button);
	button->set_button_icon(p_icon);
	button->set_tooltip_text(p_tooltip);
	button->set_focus_mode(Control::FOCUS_NONE);
	button->set_custom_minimum_size(Size2(26, 26) * EDSCALE);
	button->add_theme_constant_override("icon_max_width", 17 * EDSCALE);
	_apply_toolbar_button_style(button);
	if (p_action.is_valid()) {
		button->connect(SceneStringName(pressed), p_action);
	}
	p_parent->add_child(button);
	return button;
}

Button *ClockGearsTopBar::_make_toggle_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Ref<ButtonGroup> &p_group) {
	Button *button = _make_icon_button(p_parent, p_icon, p_tooltip, Callable());
	button->set_toggle_mode(true);
	button->set_button_group(p_group);
	return button;
}

Button *ClockGearsTopBar::_make_layer_button(HBoxContainer *p_parent, const String &p_label, const String &p_tooltip, const Ref<ButtonGroup> &p_group) {
	Button *button = memnew(Button);
	button->set_text(p_label);
	button->set_tooltip_text(p_tooltip);
	button->set_toggle_mode(true);
	button->set_button_group(p_group);
	button->set_focus_mode(Control::FOCUS_NONE);
	button->set_custom_minimum_size(Size2(24, 24) * EDSCALE);
	button->add_theme_font_size_override(SceneStringName(font_size), 12 * EDSCALE);
	_apply_toolbar_button_style(button);
	p_parent->add_child(button);
	return button;
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
	set_custom_minimum_size(Size2(0, 54) * EDSCALE);
	set_process_shortcut_input(true);

	Ref<StyleBoxFlat> bar_style;
	bar_style.instantiate();
	bar_style->set_bg_color(Color(0.088, 0.096, 0.115));
	bar_style->set_corner_radius_all(12);
	bar_style->set_content_margin_all(7);
	bar_style->set_border_width_all(1);
	bar_style->set_border_color(Color(1, 1, 1, 0.06));
	add_theme_style_override(SceneStringName(panel), bar_style);

	HBoxContainer *bar = memnew(HBoxContainer);
	bar->add_theme_constant_override("separation", 10 * EDSCALE);
	add_child(bar);

	Ref<Theme> editor_theme = EditorNode::get_singleton()->get_editor_theme();
	const auto editor_icon = [&editor_theme](const StringName &p_name) -> Ref<Texture2D> {
		return editor_theme->get_icon(p_name, SNAME("EditorIcons"));
	};

	// ------------------------------------------------------------------
	// Left: file + clipboard + history.
	// ------------------------------------------------------------------
	HBoxContainer *left_group = memnew(HBoxContainer);
	left_group->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	left_group->set_stretch_ratio(1.0);
	left_group->add_theme_constant_override("separation", 6 * EDSCALE);
	bar->add_child(left_group);

	_make_icon_button(left_group, editor_icon(SNAME("New")), CG_TR("Nuevo Mapa"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_new_scene));
	_make_icon_button(left_group, editor_icon(SNAME("Load")), CG_TR("Abrir Mapa"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_open_scene));
	_make_icon_button(left_group, editor_icon(SNAME("Save")), CG_TR("Guardar"), callable_mp(EditorNode::get_singleton(), &EditorNode::clockgears_save_scene));

	folders_btn = memnew(MenuButton);
	folders_btn->set_button_icon(editor_icon(SNAME("Folder")));
	folders_btn->set_tooltip_text(CG_TR("Carpetas del Proyecto y Gestor de Recursos (F10)"));
	folders_btn->set_focus_mode(Control::FOCUS_NONE);
	folders_btn->set_custom_minimum_size(Size2(26, 26) * EDSCALE);
	folders_btn->add_theme_constant_override("icon_max_width", 17 * EDSCALE);
	_apply_toolbar_button_style(folders_btn);
	left_group->add_child(folders_btn);

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

	left_group->add_child(memnew(VSeparator));

	_make_icon_button(left_group, editor_icon(SNAME("ActionCut")), CG_TR("Cortar"), callable_mp(this, &ClockGearsTopBar::_on_cut_pressed));
	_make_icon_button(left_group, editor_icon(SNAME("ActionCopy")), CG_TR("Copiar"), callable_mp(this, &ClockGearsTopBar::_on_copy_pressed));
	_make_icon_button(left_group, editor_icon(SNAME("ActionPaste")), CG_TR("Pegar"), callable_mp(this, &ClockGearsTopBar::_on_paste_pressed));

	left_group->add_child(memnew(VSeparator));

	undo_btn = _make_icon_button(left_group, editor_icon(SNAME("UndoRedo")), CG_TR("Deshacer (Ctrl+Z)"), callable_mp(this, &ClockGearsTopBar::_on_undo_pressed));
	redo_btn = _make_icon_button(left_group, editor_icon(SNAME("Redo")), CG_TR("Rehacer (Ctrl+Y)"), callable_mp(this, &ClockGearsTopBar::_on_redo_pressed));

	// ------------------------------------------------------------------
	// Center: the large DATABASE gear, geometrically centered.
	// ------------------------------------------------------------------
	database_btn = memnew(Button);
	database_btn->set_button_icon(_make_svg_icon(CLOCKGEARS_GEAR_SVG));
	database_btn->set_tooltip_text(CG_TR("Base de Datos - RPG Maker Paradigm (F9)"));
	database_btn->set_focus_mode(Control::FOCUS_NONE);
	database_btn->set_custom_minimum_size(Size2(44, 44) * EDSCALE);
	database_btn->add_theme_constant_override("icon_max_width", 34 * EDSCALE);

	Ref<StyleBoxFlat> database_normal;
	database_normal.instantiate();
	database_normal->set_bg_color(Color(0.95, 0.72, 0.26, 0.13));
	database_normal->set_corner_radius_all(11);
	database_normal->set_content_margin_all(4);

	Ref<StyleBoxFlat> database_hover;
	database_hover.instantiate();
	database_hover->set_bg_color(Color(0.95, 0.72, 0.26, 0.26));
	database_hover->set_corner_radius_all(11);
	database_hover->set_content_margin_all(4);

	Ref<StyleBoxFlat> database_pressed;
	database_pressed.instantiate();
	database_pressed->set_bg_color(Color(0.95, 0.72, 0.26, 0.38));
	database_pressed->set_corner_radius_all(11);
	database_pressed->set_content_margin_all(4);

	database_btn->add_theme_style_override("normal", database_normal);
	database_btn->add_theme_style_override("hover", database_hover);
	database_btn->add_theme_style_override("pressed", database_pressed);
	database_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_database_pressed));
	bar->add_child(database_btn);

	// ------------------------------------------------------------------
	// Right: map tools, layers, painting tools, zoom and playtest.
	// ------------------------------------------------------------------
	HBoxContainer *right_group = memnew(HBoxContainer);
	right_group->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	right_group->set_stretch_ratio(1.0);
	right_group->set_alignment(BoxContainer::ALIGNMENT_END);
	right_group->add_theme_constant_override("separation", 6 * EDSCALE);
	bar->add_child(right_group);

	Ref<ButtonGroup> mode_group;
	mode_group.instantiate();
	map_mode_btn = _make_toggle_button(right_group, editor_icon(SNAME("2D")), CG_TR("Modo Mapa (F5)"), mode_group);
	map_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_map_mode_pressed));
	map_mode_btn->set_pressed(true);

	event_mode_btn = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_EVENT_SVG), CG_TR("Modo Eventos (F6)"), mode_group);
	event_mode_btn->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_event_mode_pressed));

	right_group->add_child(memnew(VSeparator));

	Ref<ButtonGroup> layer_group;
	layer_group.instantiate();
	const char *layer_labels[6] = { "1", "2", "3", "4", "A", "R" };
	const char *layer_tooltips[6] = { "Capa 1: Suelo (F5)", "Capa 2: Decoración (Shift+F6)", "Capa 3: Objetos (F7)", "Capa 4: Techo (★)", "Capa Automática", "Capa R: Regiones 0-255 (F8)" };
	for (int i = 0; i < 6; i++) {
		layer_buttons[i] = _make_layer_button(right_group, layer_labels[i], CG_TR(layer_tooltips[i]), layer_group);
		layer_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_layer_pressed).bind(i + 1));
	}
	layer_buttons[0]->set_pressed(true);

	right_group->add_child(memnew(VSeparator));

	Ref<ButtonGroup> tool_group;
	tool_group.instantiate();
	tool_buttons[0] = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_PENCIL_SVG), CG_TR("Lápiz (Individual o Bloque)"), tool_group);
	tool_buttons[1] = _make_toggle_button(right_group, editor_icon(SNAME("Rectangle")), CG_TR("Rectángulo"), tool_group);
	tool_buttons[2] = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_ELLIPSE_SVG), CG_TR("Elipse"), tool_group);
	tool_buttons[3] = _make_toggle_button(right_group, editor_icon(SNAME("Bucket")), CG_TR("Cubo de Pintura (Flood Fill)"), tool_group);
	tool_buttons[4] = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_SHADOW_SVG), CG_TR("Pincel de Sombra"), tool_group);
	tool_buttons[5] = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_SELECT_SVG), CG_TR("Selección Rectangular (Ctrl+C / Ctrl+V / Supr)"), tool_group);
	tool_buttons[6] = _make_toggle_button(right_group, _make_svg_icon(CLOCKGEARS_START_POS_SVG), CG_TR("Posición Inicial del Jugador (Clic para colocar / Clic repetido para rotar dirección)"), tool_group);
	tool_buttons[7] = _make_toggle_button(right_group, editor_icon(SNAME("Eraser")), CG_TR("Goma de Borrar (Eliminar tiles con clic o arrastre)"), tool_group);
	for (int i = 0; i < 8; i++) {
		tool_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_tool_pressed).bind(i));
	}
	tool_buttons[0]->set_pressed(true);

	right_group->add_child(memnew(VSeparator));

	_make_icon_button(right_group, editor_icon(SNAME("ZoomMore")), CG_TR("Acercar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_in_pressed));
	_make_icon_button(right_group, editor_icon(SNAME("ZoomLess")), CG_TR("Alejar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_out_pressed));

	right_group->add_child(memnew(VSeparator));

	playtest_btn = _make_icon_button(right_group, editor_icon(SNAME("Play")), CG_TR("Probar Juego (Playtest F5)"), callable_mp(this, &ClockGearsTopBar::_on_playtest_pressed));
	playtest_btn->add_theme_color_override("icon_normal_color", Color(0.55, 0.88, 0.62));
}

ClockGearsTopBar::~ClockGearsTopBar() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
