/**************************************************************************/
/*  project_manager.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "project_manager.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/asset_library/asset_library_editor_plugin.h"
#include "editor/doc/editor_help.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_about.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_icon_manager.h"
#include "editor/gui/editor_title_bar.h"
#include "editor/gui/editor_version_button.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/project_manager/engine_update_label.h"
#include "editor/project_manager/project_dialog.h"
#include "editor/project_manager/project_list.h"
#include "editor/project_manager/project_tag.h"
#include "editor/project_manager/quick_settings_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "main/main.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/navigation_3d/navigation_server_3d.h"

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

#include "modules/modules_enabled.gen.h" // For gdscript, mono, clockgears.

#ifdef MODULE_CLOCKGEARS_ENABLED
#include "modules/clockgears/editor/clockgears_theme.h"
#endif

#include "scene/gui/button.h"
#include "scene/gui/texture_rect.h"

constexpr int GODOT4_CONFIG_VERSION = 5;

ProjectManager *ProjectManager::singleton = nullptr;

// Notifications.

void ProjectManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			Engine::get_singleton()->set_editor_hint(false);

			Window *main_window = get_window();
			if (main_window) {
				// Handle macOS fullscreen and extend-to-title changes.
				Callable tb_cb = callable_mp(this, &ProjectManager::_titlebar_resized);
				if (!main_window->is_connected("titlebar_changed", tb_cb)) {
					main_window->connect("titlebar_changed", tb_cb);
				}
			}

			// Theme has already been created in the constructor, so we can skip that step.
			_update_theme(true);
		} break;

		case NOTIFICATION_READY: {
			DisplayServer::get_singleton()->screen_set_keep_on(EDITOR_GET("interface/editor/display/keep_screen_on"));
			const int default_sorting = (int)EDITOR_GET("project_manager/sorting_order");
			if (default_sorting >= 0 && default_sorting < filter_option->get_item_count()) {
				filter_option->select(default_sorting);
			}
			project_list->set_order_option(default_sorting, false);

			const int default_view_mode = CLAMP((int)EDITOR_GET("project_manager/view_mode"), 0, 1);
			if (view_list_btn && view_grid_btn) {
				view_list_btn->set_pressed_no_signal(default_view_mode == 0);
				view_grid_btn->set_pressed_no_signal(default_view_mode == 1);
			}
			project_list->set_view_mode(default_view_mode, false);

			_select_main_view(MAIN_VIEW_PROJECTS);
			_update_list_placeholder();
			_update_project_buttons();
			_update_selected_preview();
			_titlebar_resized();
			_update_compact_mode(true);
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (SceneTree::get_singleton() && SceneTree::get_singleton()->get_root()) {
				SceneTree::get_singleton()->get_root()->set_title(String::utf8("ClockGears Engine - Proyectos"));
			}

			_update_list_placeholder();
			_update_selected_preview();
			_titlebar_resized();

			EditorHelpBit::clear_cache();
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			set_process_shortcut_input(is_visible_in_tree());
		} break;

		case NOTIFICATION_WM_CLOSE_REQUEST: {
			_dim_window();
		} break;

		case NOTIFICATION_WM_ABOUT: {
			_show_about();
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorThemeManager::is_generated_theme_outdated()) {
				_update_theme();
			}
			_update_list_placeholder();
			_update_selected_preview();
		} break;
		case NOTIFICATION_RESIZED: {
			project_list->resize_project_titles();
			_update_selected_preview();
			_update_compact_mode();
		} break;
	}
}

// Utility data.

Ref<Texture2D> ProjectManager::_file_dialog_get_icon(const String &p_path) {
	if (p_path.has_extension("godot")) {
		return singleton->icon_type_cache["GodotMonochrome"];
	}

	return singleton->icon_type_cache["Object"];
}

Ref<Texture2D> ProjectManager::_file_dialog_get_thumbnail(const String &p_path) {
	if (p_path.has_extension("godot")) {
		return singleton->icon_type_cache["GodotFile"];
	}

	return Ref<Texture2D>();
}

void ProjectManager::_build_icon_type_cache(Ref<Theme> p_theme) {
	if (p_theme.is_null()) {
		return;
	}
	List<StringName> tl;
	p_theme->get_icon_list(EditorStringName(EditorIcons), &tl);
	for (const StringName &name : tl) {
		icon_type_cache[name] = p_theme->get_icon(name, EditorStringName(EditorIcons));
	}
}

// Main layout.

void ProjectManager::_update_compact_mode(bool p_reset_threshold) {
	if (p_reset_threshold && project_list_sidebar) {
		project_list_sidebar->show();
		compact_mode_threshold = root_container ? root_container->get_combined_minimum_size().width : 0;
	}

	const float width = get_size().width;
	// Mantener visible el panel derecho "Acciones de proyecto" desde 960 px en adelante;
	// ajustar suavemente su ancho entre 960x540 y resoluciones mayores sin recortes ni scroll horizontal.
	if (sidebar_panel) {
		const int target_sidebar_w = (width > 0.0f && width < 1120.0f * EDSCALE) ? int(290 * EDSCALE) : int(340 * EDSCALE);
		sidebar_panel->set_custom_minimum_size(Size2(target_sidebar_w, 0));
		const bool ultra_narrow = (width > 0.0f && width < 680.0f * EDSCALE);
		sidebar_panel->set_visible(!ultra_narrow);
		if (project_list_sidebar) {
			project_list_sidebar->set_visible(!ultra_narrow);
		}
	}
}

Rect2i ProjectManager::compute_16_9_usable_window_rect(const Rect2i &p_usable_rect) {
	Rect2i usable = p_usable_rect;
	if (usable.size.x <= 0 || usable.size.y <= 0) {
		return Rect2i(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	}
	const int max_fit_k = MAX(1, MIN(usable.size.x / 16, usable.size.y / 9));
	const int budget_w = int(Math::floor(double(usable.size.x) * 0.80));
	const int budget_h = int(Math::floor(double(usable.size.y) * 0.80));
	const int k_80 = MIN(budget_w / 16, budget_h / 9);
	int k = MIN(k_80, 100); // Máximo 1600x900 (k=100)
	if (usable.size.x >= 960 && usable.size.y >= 540) {
		k = CLAMP(k, 60, max_fit_k); // Mínimo 960x540 (k=60) si cabe en el área útil
	} else {
		k = max_fit_k;
	}
	const Vector2i win_sz(k * 16, k * 9);
	const Vector2i win_pos = usable.position + (usable.size - win_sz) / 2;
	return Rect2i(win_pos, win_sz);
}

void ProjectManager::_update_size_limits() {
	const Size2 default_minimum_size = Size2(640, 360) * EDSCALE;
	const Size2 display_size = DisplayServer::get_singleton()->screen_get_usable_rect(DisplayServerEnums::SCREEN_OF_MAIN_WINDOW).size;
	const real_t smallest_display_dimension = display_size.width < display_size.height ? display_size.width : display_size.height;
	const Size2 minimum_size = (smallest_display_dimension > 0.0f) ? default_minimum_size.minf(smallest_display_dimension) : default_minimum_size;

	// Define a minimum window size to prevent UI elements from overlapping or being cut off.
	Window *w = Object::cast_to<Window>(SceneTree::get_singleton()->get_root());
	if (w) {
		// Calling Window methods this early doesn't sync properties with DS.
		w->set_min_size(minimum_size);
		DisplayServer::get_singleton()->window_set_min_size(minimum_size);
	}

	Rect2i screen_rect = DisplayServer::get_singleton()->screen_get_usable_rect(DisplayServer::get_singleton()->window_get_current_screen());
	if (screen_rect.size != Vector2i()) {
		// Al abrir en el Administrador real, forzar proporción 16:9 centrada (sin recordar tamaños no 16:9).
		if (w && w->get_mode() == Window::MODE_WINDOWED && Engine::get_singleton()->is_project_manager_hint()) {
			const Rect2i target_rect = compute_16_9_usable_window_rect(screen_rect);
			DisplayServer::get_singleton()->window_set_size(target_rect.size, DisplayServerEnums::MAIN_WINDOW_ID);
			DisplayServer::get_singleton()->window_set_position(target_rect.position, DisplayServerEnums::MAIN_WINDOW_ID);
		}

		// Limit popup menus to prevent unusably long lists.
		// We try to set it to half the screen resolution, but no smaller than the minimum window size.
		Size2 half_screen_rect = (screen_rect.size * EDSCALE) / 2;
		Size2 maximum_popup_size = half_screen_rect.max(minimum_size);
		quick_settings_dialog->update_size_limits(maximum_popup_size);
	}
}

void ProjectManager::_update_theme(bool p_skip_creation) {
	if (!p_skip_creation) {
		theme = EditorThemeManager::generate_theme(theme);
	}

	Vector<Ref<Theme>> editor_themes;
#ifdef MODULE_CLOCKGEARS_ENABLED
	if (ClockGearsTheme *cg_inst = ClockGearsTheme::get_singleton()) {
		cg_inst->sync_from_project_settings();
		Ref<Theme> cg_theme = cg_inst->get_active_theme();
		if (cg_theme.is_valid()) {
			editor_themes.push_back(cg_theme);
		}
		Callable cb = callable_mp(this, &ProjectManager::_on_clockgears_theme_changed);
		if (!cg_inst->is_connected("theme_changed", cb)) {
			cg_inst->connect("theme_changed", cb);
		}
	}
#endif
	editor_themes.push_back(theme);
	editor_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext *node_tc = ThemeDB::get_singleton()->get_theme_context(this);
	if (node_tc) {
		node_tc->set_themes(editor_themes);
	} else {
		ThemeDB::get_singleton()->create_theme_context(this, editor_themes);
	}

	Window *owner_window = get_window();
	if (owner_window) {
		ThemeContext *window_tc = ThemeDB::get_singleton()->get_theme_context(owner_window);
		if (window_tc) {
			window_tc->set_themes(editor_themes);
		} else {
			ThemeDB::get_singleton()->create_theme_context(owner_window, editor_themes);
		}
	}

	// Update styles.
	{
		const Color bg_clear = has_theme_color("background_color", "ProjectManager") ? get_theme_color("background_color", "ProjectManager") : get_theme_color("background", EditorStringName(Editor));
		DisplayServer::set_early_window_clear_color_override(true, bg_clear);

		const int top_bar_separation = int(10 * EDSCALE);
		root_container->add_theme_constant_override("margin_left", int(12 * EDSCALE));
		root_container->add_theme_constant_override("margin_top", int(10 * EDSCALE));
		root_container->add_theme_constant_override("margin_bottom", int(10 * EDSCALE));
		root_container->add_theme_constant_override("margin_right", int(12 * EDSCALE));
		main_vbox->add_theme_constant_override("separation", top_bar_separation);

		if (has_theme_stylebox("background_panel", "ProjectManager")) {
			background_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("background_panel", "ProjectManager"));
		} else {
			background_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("Background", EditorStringName(EditorStyles)));
		}
		if (header_bar) {
			header_bar->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), "ProjectHeaderBar"));
		}
		main_view_container->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("panel_container", "ProjectManager"));

		Ref<Texture2D> engine_icon = get_editor_theme_icon("DefaultProjectIcon");
		title_bar_logo->set_button_icon(engine_icon);

		if (header_title_label) {
			header_title_label->add_theme_font_override(SceneStringName(font), get_theme_font("bold", EditorStringName(EditorFonts)));
			header_title_label->add_theme_font_size_override(SceneStringName(font_size), int(16 * EDSCALE));
			header_title_label->add_theme_color_override(SceneStringName(font_color), get_theme_color("font_color", "Label"));
		}
		if (header_subtitle_label) {
			header_subtitle_label->add_theme_font_size_override(SceneStringName(font_size), int(11 * EDSCALE));
			header_subtitle_label->add_theme_color_override(SceneStringName(font_color), get_theme_color("subtitle_color", "ProjectManager"));
		}
		if (section_title_label) {
			section_title_label->add_theme_font_override(SceneStringName(font), get_theme_font("bold", EditorStringName(EditorFonts)));
			section_title_label->add_theme_font_size_override(SceneStringName(font_size), int(15 * EDSCALE));
			section_title_label->add_theme_color_override(SceneStringName(font_color), get_theme_color("font_color", "Label"));
		}

		_set_main_view_icon(MAIN_VIEW_PROJECTS, get_editor_theme_icon("ProjectList"));

		// Project list & actions panel.
		{
			loading_label->add_theme_font_override(SceneStringName(font), get_theme_font("bold", EditorStringName(EditorFonts)));
			project_list_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("project_list", "ProjectManager"));
			if (sidebar_panel) {
				sidebar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("action_panel", "ProjectManager"));
			}
			if (sidebar_header_label) {
				sidebar_header_label->add_theme_font_override(SceneStringName(font), get_theme_font("bold", EditorStringName(EditorFonts)));
				sidebar_header_label->add_theme_font_size_override(SceneStringName(font_size), int(14 * EDSCALE));
				sidebar_header_label->add_theme_color_override(SceneStringName(font_color), get_theme_color("font_color", "Label"));
			}
			if (sidebar_preview_panel) {
				sidebar_preview_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox("preview_card", "ProjectManager"));
			}
			if (sidebar_preview_name) {
				sidebar_preview_name->add_theme_font_override(SceneStringName(font), get_theme_font("bold", EditorStringName(EditorFonts)));
				sidebar_preview_name->add_theme_font_size_override(SceneStringName(font_size), int(14 * EDSCALE));
				sidebar_preview_name->add_theme_color_override(SceneStringName(font_color), get_theme_color("font_color", "Label"));
			}
			if (sidebar_preview_path) {
				sidebar_preview_path->add_theme_font_size_override(SceneStringName(font_size), int(11 * EDSCALE));
				sidebar_preview_path->add_theme_color_override(SceneStringName(font_color), get_theme_color("subtitle_color", "ProjectManager"));
			}

			empty_list_create_project->set_button_icon(get_editor_theme_icon("Add"));
			empty_list_import_project->set_button_icon(get_editor_theme_icon("ClockGearsFolderLine"));
			if (empty_list_clear_filter_btn) {
				empty_list_clear_filter_btn->set_button_icon(get_editor_theme_icon("Clear"));
			}
			if (empty_list_open_assetlib) {
				empty_list_open_assetlib->set_button_icon(get_editor_theme_icon("AssetStore"));
			}

			if (empty_list_online_warning) {
				empty_list_online_warning->add_theme_font_override(SceneStringName(font), get_theme_font("italic", EditorStringName(EditorFonts)));
				empty_list_online_warning->add_theme_color_override(SceneStringName(font_color), get_theme_color("font_placeholder_color", EditorStringName(Editor)));
			}

			// Top bar & toolbar.
			search_box->set_right_icon(get_editor_theme_icon("Search"));
			quick_settings_button->set_button_icon(get_editor_theme_icon("ClockGearsGear"));
			if (view_list_btn) {
				view_list_btn->set_button_icon(get_editor_theme_icon("ClockGearsViewList"));
			}
			if (view_grid_btn) {
				view_grid_btn->set_button_icon(get_editor_theme_icon("ClockGearsViewGrid"));
			}

			create_btn->set_button_icon(Ref<Texture2D>());
			import_btn->set_button_icon(get_editor_theme_icon("ClockGearsFolderLine"));
			scan_btn->set_button_icon(get_editor_theme_icon("Search"));
			open_btn->set_button_icon(get_editor_theme_icon("Edit"));
			open_options_btn->set_button_icon(get_editor_theme_icon("Collapse"));
			run_btn->set_button_icon(get_editor_theme_icon("Play"));
			rename_btn->set_button_icon(get_editor_theme_icon("Rename"));
			duplicate_btn->set_button_icon(get_editor_theme_icon("Duplicate"));
			manage_tags_btn->set_button_icon(get_editor_theme_icon("ClockGearsTag"));
			if (show_in_folder_btn) {
				show_in_folder_btn->set_button_icon(get_editor_theme_icon("ClockGearsFolderLine"));
			}
			erase_btn->set_button_icon(get_editor_theme_icon("Remove"));
			erase_missing_btn->set_button_icon(get_editor_theme_icon("Clear"));
			create_tag_btn->set_button_icon(get_editor_theme_icon("Add"));
			if (donate_btn) {
				donate_btn->set_button_icon(get_editor_theme_icon("Heart"));
			}

			tag_error->add_theme_color_override(SceneStringName(font_color), get_theme_color("error_color", EditorStringName(Editor)));
			tag_edit_error->add_theme_color_override(SceneStringName(font_color), get_theme_color("error_color", EditorStringName(Editor)));

			const int h_separation = get_theme_constant("sidebar_button_icon_separation", "ProjectManager");
			create_btn->add_theme_constant_override("h_separation", h_separation);
			import_btn->add_theme_constant_override("h_separation", h_separation);
			scan_btn->add_theme_constant_override("h_separation", h_separation);
			open_btn->add_theme_constant_override("h_separation", h_separation);
			run_btn->add_theme_constant_override("h_separation", h_separation);
			rename_btn->add_theme_constant_override("h_separation", h_separation);
			duplicate_btn->add_theme_constant_override("h_separation", h_separation);
			manage_tags_btn->add_theme_constant_override("h_separation", h_separation);
			if (show_in_folder_btn) {
				show_in_folder_btn->add_theme_constant_override("h_separation", h_separation);
			}
			erase_btn->add_theme_constant_override("h_separation", h_separation);
			erase_missing_btn->add_theme_constant_override("h_separation", h_separation);

			open_btn_container->add_theme_constant_override("separation", int(2 * EDSCALE));
			open_options_popup->set_item_icon(0, get_editor_theme_icon("Notification"));
			open_options_popup->set_item_icon(1, get_editor_theme_icon("NodeWarning"));
		}

		// Dialogs.
		migration_guide_button->set_button_icon(get_editor_theme_icon("ExternalLink"));

		// Asset store popup.
		if (asset_library && EDITOR_GET("interface/theme/style") == "Classic") {
			// Removes extra border margins.
			asset_library->add_theme_style_override(SceneStringName(panel), memnew(StyleBoxEmpty));
		}
	}

	_update_selected_preview();
	_update_list_placeholder();

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	EditorHelpHighlighter::get_singleton()->clear_cache();
#endif

#ifdef ANDROID_ENABLED
	DisplayServer::get_singleton()->window_set_color(theme->get_color("background", EditorStringName(Editor)));
#endif
}

Button *ProjectManager::_add_main_view(MainViewTab p_id, const String &p_name, const Ref<Texture2D> &p_icon, Control *p_view_control) {
	ERR_FAIL_INDEX_V(p_id, MAIN_VIEW_MAX, nullptr);
	ERR_FAIL_COND_V(main_view_map.has(p_id), nullptr);
	ERR_FAIL_COND_V(main_view_toggle_map.has(p_id), nullptr);

	Button *toggle_button = memnew(Button);
	toggle_button->set_flat(true);
	toggle_button->set_theme_type_variation("MainScreenButton");
	toggle_button->set_toggle_mode(true);
	toggle_button->set_button_group(main_view_toggles_group);
	toggle_button->set_text(p_name);
	toggle_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_select_main_view).bind((int)p_id));

	main_view_toggles->add_child(toggle_button);
	main_view_toggle_map[p_id] = toggle_button;

	_set_main_view_icon(p_id, p_icon);

	p_view_control->set_visible(false);
	main_view_container->add_child(p_view_control);
	main_view_map[p_id] = p_view_control;

	return toggle_button;
}

void ProjectManager::_set_main_view_icon(MainViewTab p_id, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_INDEX(p_id, MAIN_VIEW_MAX);
	ERR_FAIL_COND(!main_view_toggle_map.has(p_id));

	Button *toggle_button = main_view_toggle_map[p_id];

	Ref<Texture2D> old_icon = toggle_button->get_button_icon();
	if (old_icon.is_valid()) {
		old_icon->disconnect_changed(callable_mp((Control *)toggle_button, &Control::update_minimum_size));
	}

	if (p_icon.is_valid()) {
		toggle_button->set_button_icon(p_icon);
		// Make sure the control is updated if the icon is reimported.
		p_icon->connect_changed(callable_mp((Control *)toggle_button, &Control::update_minimum_size));
	} else {
		toggle_button->set_button_icon(Ref<Texture2D>());
	}
}

void ProjectManager::_select_main_view(int p_id) {
	MainViewTab view_id = (MainViewTab)p_id;

	ERR_FAIL_INDEX(view_id, MAIN_VIEW_MAX);
	ERR_FAIL_COND(!main_view_map.has(view_id));
	ERR_FAIL_COND(!main_view_toggle_map.has(view_id));

	if (current_main_view != view_id) {
		main_view_toggle_map[current_main_view]->set_pressed_no_signal(false);
		main_view_map[current_main_view]->set_visible(false);
		current_main_view = view_id;
	}
	main_view_toggle_map[current_main_view]->set_pressed_no_signal(true);
	main_view_map[current_main_view]->set_visible(true);

#ifndef ANDROID_ENABLED
	if (current_main_view == MAIN_VIEW_PROJECTS && search_box->is_inside_tree()) {
		// Automatically grab focus when the user moves from the Templates tab
		// back to the Projects tab.
		// Needs to be deferred, otherwise the focus outline is always drawn.
		callable_mp((Control *)search_box, &Control::grab_focus).call_deferred(true);
	}

	// The Templates tab's search field is focused on display in the asset
	// library editor plugin code.
#endif
}

void ProjectManager::_show_about() {
	if (!about_dialog) {
		about_dialog = memnew(EditorAbout);
		add_child(about_dialog);
	}
	about_dialog->popup_centered(Size2(780, 500) * EDSCALE);
}

void ProjectManager::_open_asset_library_confirmed() {
	const int network_mode = EDITOR_GET("network/connection/network_mode");
	if (network_mode == EditorSettings::NETWORK_OFFLINE) {
		EditorSettings::get_singleton()->set_setting("network/connection/network_mode", EditorSettings::NETWORK_ONLINE);
		EditorSettings::get_singleton()->notify_changes();
		EditorSettings::get_singleton()->save();
	}

	_select_main_view(MAIN_VIEW_ASSETLIB);
}

void ProjectManager::_project_list_menu_option(int p_option) {
	switch (p_option) {
		case ProjectList::MENU_EDIT:
			_open_selected_projects();
			break;

		case ProjectList::MENU_EDIT_VERBOSE:
			open_in_verbose_mode = true;
			_open_selected_projects_check_warnings();
			break;

		case ProjectList::MENU_EDIT_RECOVERY:
			_open_recovery_mode_ask(true);
			break;

		case ProjectList::MENU_RUN:
			_run_project_confirm();
			break;

		case ProjectList::MENU_SHOW_IN_FILE_MANAGER:
			_show_project_in_file_manager();
			break;

		case ProjectList::MENU_COPY_PATH: {
			const Vector<ProjectList::Item> &selected_list = project_list->get_selected_projects();
			if (selected_list.is_empty()) {
				return;
			}
			DisplayServer::get_singleton()->clipboard_set(selected_list[0].path);
		} break;

		case ProjectList::MENU_RENAME:
			_rename_project();
			break;

		case ProjectList::MENU_MANAGE_TAGS:
			_manage_project_tags();
			break;

		case ProjectList::MENU_DUPLICATE:
			_duplicate_project();
			break;

		case ProjectList::MENU_REMOVE:
			_erase_project();
			break;
	}
}

void ProjectManager::_show_error(const String &p_message, const Size2 &p_min_size) {
	error_dialog->set_text(p_message);
	error_dialog->popup_centered(p_min_size);
}

void ProjectManager::_dim_window() {
	// This method must be called before calling `get_tree()->quit()`.
	// Otherwise, its effect won't be visible

	// Dim the project manager window while it's quitting to make it clearer that it's busy.
	// No transition is applied, as the effect needs to be visible immediately
	float c = 0.5f;
	Color dim_color = Color(c, c, c);
	set_modulate(dim_color);
}

// Quick settings.

void ProjectManager::_show_quick_settings() {
	if (!EditorPropertyNameProcessor::get_singleton()) {
		EditorPropertyNameProcessor *epnp = memnew(EditorPropertyNameProcessor);
		add_child(epnp);

		EditorIconManager *icon_manager = memnew(EditorIconManager);
		add_child(icon_manager);

		EditorHelp::generate_doc();
	}
	quick_settings_dialog->popup_centered(Size2(640, 200) * EDSCALE);
}

void ProjectManager::_restart_confirmed() {
	List<String> args = OS::get_singleton()->get_cmdline_args();
	Error err = OS::get_singleton()->create_instance(args);
	ERR_FAIL_COND(err);

	_dim_window();
	get_tree()->quit();
}

// Project list.

void ProjectManager::_update_list_placeholder() {
	if (!is_inside_tree() || !project_list || !empty_list_placeholder || !empty_list_message) {
		return;
	}

	const int total_count = project_list->get_project_count();
	const int filtered_count = project_list->get_filtered_project_count();

	if (empty_list_open_assetlib) {
		empty_list_open_assetlib->hide();
	}
	if (empty_list_online_warning) {
		empty_list_online_warning->hide();
	}

	if (total_count == 0) {
		const String title_col = get_theme_color("empty_title_color", "ProjectManager").to_html(false);
		const String sub_col = get_theme_color("empty_sub_color", "ProjectManager").to_html(false);
		empty_list_message->set_text(vformat("[center][color=#%s][b]%s[/b][/color]\n[color=#%s]%s[/color][/center]",
				title_col, String::utf8("Aún no tienes proyectos"),
				sub_col, String::utf8("Crea un proyecto nuevo o importa uno existente para empezar.")));
		empty_list_create_project->set_visible(true);
		empty_list_import_project->set_visible(true);
		if (empty_list_clear_filter_btn) {
			empty_list_clear_filter_btn->set_visible(false);
		}
		empty_list_placeholder->show();
		return;
	}

	if (filtered_count == 0) {
		const String title_col = get_theme_color("empty_title_color", "ProjectManager").to_html(false);
		const String sub_col = get_theme_color("empty_sub_color", "ProjectManager").to_html(false);
		empty_list_message->set_text(vformat("[center][color=#%s][b]%s[/b][/color]\n[color=#%s]%s[/color][/center]",
				title_col, String::utf8("No hay proyectos que coincidan"),
				sub_col, String::utf8("Prueba otra búsqueda o limpia el filtro actual.")));
		empty_list_create_project->set_visible(false);
		empty_list_import_project->set_visible(false);
		if (empty_list_clear_filter_btn) {
			empty_list_clear_filter_btn->set_visible(true);
		}
		empty_list_placeholder->show();
		return;
	}

	empty_list_placeholder->hide();
}

void ProjectManager::_scan_projects() {
	scan_dir->popup_file_dialog();
}

void ProjectManager::_run_project() {
	const HashSet<String> &selected_list = project_list->get_selected_project_keys();

	if (selected_list.size() < 1) {
		return;
	}

	if (selected_list.size() > 1) {
		multi_run_ask->set_text(vformat(TTR("Are you sure to run %d projects at once?"), selected_list.size()));
		multi_run_ask->popup_centered();
	} else {
		_run_project_confirm();
	}
}

void ProjectManager::_run_project_confirm() {
	Vector<ProjectList::Item> selected_list = project_list->get_selected_projects();

	for (int i = 0; i < selected_list.size(); ++i) {
		const String &selected_main = selected_list[i].main_scene;
		if (selected_main.is_empty()) {
			_show_error(TTRC("Can't run project: Project has no main scene defined.\nPlease edit the project and set the main scene in the Project Settings under the \"Application\" category."));
			continue;
		}

		const String &path = selected_list[i].path;

		// `.substr(6)` on `ProjectSettings::get_singleton()->get_imported_files_path()` strips away the leading "res://".
		if (!DirAccess::exists(path.path_join(ProjectSettings::get_singleton()->get_imported_files_path().substr(6)))) {
			_show_error(TTRC("Can't run project: Assets need to be imported first.\nPlease edit the project to trigger the initial import."));
			continue;
		}

		print_line("Running project: " + path);

		List<String> args;

		for (const String &a : Main::get_forwardable_cli_arguments(Main::CLI_SCOPE_PROJECT)) {
			args.push_back(a);
		}

		args.push_back("--path");
		args.push_back(path);

		Error err = OS::get_singleton()->create_instance(args);
		ERR_FAIL_COND(err);
	}
}

void ProjectManager::_open_selected_projects() {
	// Show loading text to tell the user that the project manager is busy loading.
	// This is especially important for the Web project manager.
	loading_label->show();

	const HashSet<String> &selected_list = project_list->get_selected_project_keys();
	for (const String &path : selected_list) {
		String conf = path.path_join("project.godot");

		if (!FileAccess::exists(conf)) {
			loading_label->hide();
			_show_error(vformat(TTR("Can't open project at '%s'.\nProject file doesn't exist or is inaccessible."), path));
			return;
		}

		print_line("Editing project: " + path);

		List<String> args;

		for (const String &a : Main::get_forwardable_cli_arguments(Main::CLI_SCOPE_TOOL)) {
			args.push_back(a);
		}

		args.push_back("--path");
		args.push_back(path);

		args.push_back("--editor");

		if (open_in_recovery_mode) {
			args.push_back("--recovery-mode");
		}

		if (open_in_verbose_mode) {
			args.push_back("--verbose");
		}

		if (ask_upgrade_tool->is_pressed()) {
			args.push_back("--run-upgrade-tool");
		}

		Error err = OS::get_singleton()->create_instance(args);
		if (err != OK) {
			loading_label->hide();
			_show_error(vformat(TTR("Can't open project at '%s'.\nFailed to start the editor."), path));
			ERR_PRINT(vformat("Failed to start an editor instance for the project at '%s', error code %d.", path, err));
			return;
		}
	}

	project_list->project_opening_initiated = true;

	_dim_window();
	get_tree()->quit();
}

void ProjectManager::_open_selected_projects_check_warnings() {
	const HashSet<String> &selected_list = project_list->get_selected_project_keys();
	if (selected_list.size() < 1) {
		return;
	}

	const Size2i popup_min_size = Size2i(400.0 * EDSCALE, 0);

	if (selected_list.size() > 1) {
		multi_open_ask->set_text(vformat(TTR("You requested to open %d projects in parallel. Do you confirm?\nNote that usual checks for engine version compatibility will be bypassed."), selected_list.size()));
		multi_open_ask->popup_centered(popup_min_size);
		return;
	}

	ProjectList::Item project = project_list->get_selected_projects()[0];
	if (project.missing) {
		return;
	}

	// Update the project settings or don't open.
	const int config_version = project.version;
	PackedStringArray unsupported_features = project.unsupported_features;

	ask_update_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT); // Reset in case of previous center align.
	ask_update_backup->set_pressed(false);
	ask_upgrade_tool->set_pressed(false);
	full_convert_button->hide();
	migration_guide_button->hide();
	ask_update_backup->hide();
	ask_upgrade_tool->hide();

	ask_update_settings->get_ok_button()->set_text("OK");

	// Check if the config_version property was empty or 0.
	if (config_version == 0) {
		ask_update_label->set_text(vformat(TTR("The selected project \"%s\" does not specify its supported ClockGears Engine version in its configuration file (\"project.godot\").\n\nProject path: %s\n\nIf you proceed with opening it, it will be converted to ClockGears Engine's current configuration file format.\n\nWarning: You won't be able to open the project with previous versions of the engine anymore."), project.project_name, project.path));
		ask_update_settings->popup_centered(popup_min_size);
		return;
	}
	// Check if we need to convert project settings from an earlier engine version.
	if (config_version < ProjectSettings::CONFIG_VERSION) {
		if (config_version == GODOT4_CONFIG_VERSION - 1 && ProjectSettings::CONFIG_VERSION == GODOT4_CONFIG_VERSION) { // Conversion from Godot 3 to 4.
			full_convert_button->show();
			ask_update_label->set_text(vformat(TTR("The selected project \"%s\" was generated by Godot 3.x, and needs to be converted for Godot 4.x.\n\nProject path: %s\n\nYou have three options:\n- Convert only the configuration file (\"project.godot\"). Use this to open the project without attempting to convert its scenes, resources and scripts.\n- Convert the entire project including its scenes, resources and scripts (recommended if you are upgrading).\n- Do nothing and go back.\n\nWarning: If you select a conversion option, you won't be able to open the project with previous versions of the engine anymore."), project.project_name, project.path));
			ask_update_settings->get_ok_button()->set_text(TTRC("Convert project.godot Only"));
		} else {
			ask_update_label->set_text(vformat(TTR("The selected project \"%s\" was generated by an older engine version, and needs to be converted for this version.\n\nProject path: %s\n\nDo you want to convert it?\n\nWarning: You won't be able to open the project with previous versions of the engine anymore."), project.project_name, project.path));
			ask_update_settings->get_ok_button()->set_text(TTRC("Convert project.godot"));
		}
		ask_update_backup->show();
		migration_guide_button->show();
		ask_update_settings->popup_centered(popup_min_size);
		ask_update_settings->get_cancel_button()->grab_focus(); // To prevent accidents.
		return;
	}
	// Check if the file was generated by a newer, incompatible engine version.
	if (config_version > ProjectSettings::CONFIG_VERSION) {
		_show_error(vformat(TTR("Can't open project \"%s\" at the following path:\n\n%s\n\nThe project settings were created by a newer engine version, whose settings are not compatible with this version."), project.project_name, project.path), popup_min_size);
		return;
	}
	// Check if the project is using features not supported by this build of Godot.
	if (!unsupported_features.is_empty()) {
		String warning_message = "";
		for (int i = 0; i < unsupported_features.size(); i++) {
			const String &feature = unsupported_features[i];
			if (feature == "Double Precision") {
				ask_update_backup->show();
				warning_message += TTR("Warning: This project uses double precision floats, but this version of\nGodot uses single precision floats. Opening this project may cause data loss.\n\n");
				unsupported_features.remove_at(i);
				i--;
			} else if (feature == "C#") {
				warning_message += TTR("Warning: This project uses C#, but this build of ClockGears Engine does not have\nthe Mono module. If you proceed you will not be able to use any C# scripts.\n\n");
				unsupported_features.remove_at(i);
				i--;
			} else if (ProjectList::project_feature_looks_like_version(feature)) {
				ask_update_backup->show();
				if (project.control->is_older_version()) {
					ask_upgrade_tool->show();
					migration_guide_button->show();
				}
				version_convert_feature = feature;
				warning_message += vformat(TTR("Warning: This project was last edited in ClockGears Engine %s. Opening will change it to ClockGears Engine %s.\n\n"), Variant(feature), Variant(GODOT_VERSION_BRANCH));
				unsupported_features.remove_at(i);
				i--;
			}
		}
		if (!unsupported_features.is_empty()) {
			String unsupported_features_str = String(", ").join(unsupported_features);
			warning_message += vformat(TTR("Warning: This project uses the following features not supported by this build of ClockGears Engine:\n\n%s\n\n"), unsupported_features_str);
		}
		warning_message += TTR("Open anyway? Project will be modified.");
		ask_update_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		ask_update_label->set_text(warning_message);
		ask_update_settings->popup_centered(popup_min_size);
		return;
	}

	// Open if the project is up-to-date.
	_open_selected_projects();
}

void ProjectManager::_open_selected_projects_check_recovery_mode() {
	Vector<ProjectList::Item> selected_projects = project_list->get_selected_projects();

	if (selected_projects.is_empty()) {
		return;
	}

	const ProjectList::Item &project = selected_projects[0];
	if (project.missing) {
		return;
	}

	open_in_verbose_mode = false;
	open_in_recovery_mode = false;
	// Check if the project failed to load during last startup.
	if (project.recovery_mode) {
		_open_recovery_mode_ask(false);
		return;
	}

	_open_selected_projects_check_warnings();
}

void ProjectManager::_open_selected_projects_with_migration() {
	if (ask_update_backup->is_pressed() && project_list->get_selected_projects().size() == 1) {
		ask_update_settings->hide();
		ask_update_backup->set_pressed(false);

		_duplicate_project_with_action(POST_DUPLICATE_ACTION_OPEN);
		return;
	}

#ifndef DISABLE_DEPRECATED
	if (project_list->get_selected_projects().size() == 1) {
		// Only migrate if a single project is opened.
		_minor_project_migrate();
	}
#endif
	_open_selected_projects();
	ask_upgrade_tool->set_pressed(false);
}

void ProjectManager::_install_project(const String &p_zip_path, const String &p_title) {
	project_dialog->set_mode(ProjectDialog::MODE_INSTALL);
	project_dialog->set_zip_path(p_zip_path);
	project_dialog->set_zip_title(p_title);
	project_dialog->show_dialog();
}

void ProjectManager::_import_project() {
	project_dialog->set_mode(ProjectDialog::MODE_IMPORT);
	project_dialog->ask_for_path_and_show();
}

void ProjectManager::_new_project() {
	project_dialog->set_mode(ProjectDialog::MODE_NEW);
	project_dialog->show_dialog();
}

void ProjectManager::_rename_project() {
	const Vector<ProjectList::Item> &selected_list = project_list->get_selected_projects();

	if (selected_list.is_empty()) {
		return;
	}

	for (const ProjectList::Item &E : selected_list) {
		project_dialog->set_project_name(E.project_name);
		project_dialog->set_project_path(E.path);
		project_dialog->set_mode(ProjectDialog::MODE_RENAME);
		project_dialog->show_dialog();
	}
}

void ProjectManager::_duplicate_project() {
	_duplicate_project_with_action(POST_DUPLICATE_ACTION_NONE);
}

void ProjectManager::_duplicate_project_with_action(PostDuplicateAction p_post_action) {
	Vector<ProjectList::Item> selected_projects = project_list->get_selected_projects();
	if (selected_projects.is_empty()) {
		return;
	}

	post_duplicate_action = p_post_action;

	const ProjectList::Item &project = selected_projects[0];

	project_dialog->set_mode(ProjectDialog::MODE_DUPLICATE);
	project_dialog->set_project_name(vformat("%s (%s)", project.project_name, p_post_action == POST_DUPLICATE_ACTION_NONE ? "Copy" : project.project_version));
	project_dialog->set_original_project_path(project.path);
	project_dialog->set_duplicate_can_edit(p_post_action == POST_DUPLICATE_ACTION_NONE);
	project_dialog->show_dialog(false);
}

void ProjectManager::_show_project_in_file_manager() {
	const Vector<ProjectList::Item> &selected_list = project_list->get_selected_projects();
	if (selected_list.is_empty()) {
		return;
	}

	for (const ProjectList::Item &E : selected_list) {
		OS::get_singleton()->shell_show_in_file_manager(E.path, true);
	}
}

void ProjectManager::_erase_project() {
	const HashSet<String> &selected_list = project_list->get_selected_project_keys();

	if (selected_list.is_empty()) {
		return;
	}

	String confirm_message;
	if (selected_list.size() >= 2) {
		confirm_message = vformat(String::utf8("¿Eliminar %d proyectos de la lista?\nNo se borrarán los archivos de sus carpetas."), selected_list.size());
	} else {
		confirm_message = String::utf8("¿Eliminar este proyecto de la lista?\nNo se borrarán los archivos de su carpeta.");
	}

	erase_ask_label->set_text(confirm_message);
	erase_ask->popup_centered();
}

void ProjectManager::_erase_missing_projects() {
	erase_missing_ask->set_text(String::utf8("¿Eliminar todos los proyectos faltantes de la lista?\nNo se modificará el contenido de las carpetas en disco."));
	erase_missing_ask->popup_centered();
}

void ProjectManager::_erase_project_confirm() {
	project_list->erase_selected_projects(false);
	_update_project_buttons();
	_update_list_placeholder();
}

void ProjectManager::_erase_missing_projects_confirm() {
	project_list->erase_missing_projects();
	_update_project_buttons();
	_update_list_placeholder();
}

void ProjectManager::_update_project_buttons() {
	Vector<ProjectList::Item> selected_projects = project_list->get_selected_projects();
	bool empty_selection = selected_projects.is_empty();

	bool is_missing_project_selected = false;
	for (int i = 0; i < selected_projects.size(); ++i) {
		if (selected_projects[i].missing) {
			is_missing_project_selected = true;
			break;
		}
	}

	erase_btn->set_disabled(empty_selection);
	open_btn->set_disabled(empty_selection || is_missing_project_selected);
	open_options_btn->set_disabled(empty_selection || is_missing_project_selected);
	rename_btn->set_disabled(empty_selection || is_missing_project_selected);
	duplicate_btn->set_disabled(empty_selection || is_missing_project_selected);
	manage_tags_btn->set_disabled(empty_selection || is_missing_project_selected || selected_projects.size() > 1);
	if (show_in_folder_btn) {
		show_in_folder_btn->set_disabled(empty_selection || is_missing_project_selected);
	}
	run_btn->set_disabled(empty_selection || is_missing_project_selected);

	erase_missing_btn->set_disabled(!project_list->is_any_project_missing());
	_update_selected_preview();
}

void ProjectManager::_open_options_popup() {
	Rect2 rect = open_btn_container->get_screen_rect();
	rect.position.y += rect.size.height;
	open_options_popup->set_size(Size2(rect.size.width, 0));
	open_options_popup->set_position(rect.position);

	open_options_popup->popup();
}

void ProjectManager::_open_recovery_mode_ask(bool manual) {
	String recovery_mode_details;

	// Only show the initial crash preamble if this popup wasn't manually triggered.
	if (!manual) {
		recovery_mode_details +=
				TTR("It looks like ClockGears Engine crashed when opening this project the last time. If you're having problems editing this project, you can try to open it in Recovery Mode.") +
				String::utf8("\n\n");
	}

	recovery_mode_details +=
			TTR("Recovery Mode is a special mode that may help to recover projects that crash the engine during initialization. This mode temporarily disables the following features:") +
			String::utf8("\n\n•  ") + TTR("Tool scripts") +
			String::utf8("\n•  ") + TTR("Editor plugins") +
			String::utf8("\n•  ") + TTR("GDExtension addons") +
			String::utf8("\n•  ") + TTR("Automatic scene restoring") +
			String::utf8("\n\n") + TTR("This mode is intended only for basic editing to troubleshoot such issues, and therefore it will not be possible to run the project during this mode. It is also a good idea to make a backup of your project before proceeding.") +
			String::utf8("\n\n") + TTR("Edit the project in Recovery Mode?");

	open_recovery_mode_ask->set_text(recovery_mode_details);
	open_recovery_mode_ask->popup_centered(Size2(550, 70) * EDSCALE);
}

void ProjectManager::_on_projects_updated() {
	Vector<ProjectList::Item> selected_projects = project_list->get_selected_projects();
	int index = 0;
	for (int i = 0; i < selected_projects.size(); ++i) {
		index = project_list->refresh_project(selected_projects[i].path);
	}
	if (index != -1) {
		project_list->ensure_project_visible(index);
	}

	project_list->update_dock_menu();
}

void ProjectManager::_on_open_options_selected(int p_option) {
	switch (p_option) {
		case 0: // Edit in verbose mode.
			open_in_verbose_mode = true;
			_open_selected_projects_check_warnings();
			break;
		case 1: // Edit in recovery mode.
			_open_recovery_mode_ask(true);
			break;
	}
}

void ProjectManager::_on_recovery_mode_popup_open_normal() {
	open_recovery_mode_ask->hide();
	open_in_recovery_mode = false;
	_open_selected_projects_check_warnings();
}

void ProjectManager::_on_recovery_mode_popup_open_recovery() {
	open_in_recovery_mode = true;
	_open_selected_projects_check_warnings();
}

void ProjectManager::_on_project_created(const String &dir, bool edit) {
	project_list->add_project(dir, false);
	project_list->save_config();
	search_box->clear();

	int i = project_list->refresh_project(dir);
	project_list->ensure_project_visible(i);
	_update_list_placeholder();

	if (edit) {
		_open_selected_projects_check_warnings();
	}

	project_list->update_dock_menu();
}

void ProjectManager::_on_project_duplicated(const String &p_original_path, const String &p_duplicate_path, bool p_edit) {
	if (post_duplicate_action == POST_DUPLICATE_ACTION_NONE) {
		_on_project_created(p_duplicate_path, p_edit);
	} else {
		project_list->add_project(p_duplicate_path, false);
		project_list->save_config();

		if (post_duplicate_action == POST_DUPLICATE_ACTION_OPEN) {
			_open_selected_projects_with_migration();
		} else if (post_duplicate_action == POST_DUPLICATE_ACTION_FULL_CONVERSION) {
			_full_convert_button_pressed();
		}

		project_list->update_dock_menu();
	}

	post_duplicate_action = POST_DUPLICATE_ACTION_NONE;
}

void ProjectManager::_on_order_option_changed(int p_idx) {
	if (is_inside_tree()) {
		project_list->set_order_option(p_idx, true);
	}
}

void ProjectManager::_on_view_mode_pressed(int p_mode) {
	const int clamped = CLAMP(p_mode, 0, 1);
	if (view_list_btn && view_grid_btn) {
		view_list_btn->set_pressed_no_signal(clamped == 0);
		view_grid_btn->set_pressed_no_signal(clamped == 1);
	}
	if (project_list) {
		project_list->set_view_mode(clamped, true);
	}
}

void ProjectManager::_clear_search_filter() {
	if (search_box) {
		search_box->clear();
	}
	_on_search_term_changed(String());
}

void ProjectManager::_on_clockgears_theme_changed(const String &p_theme_id) {
	if (is_inside_tree()) {
		_update_theme(true);
	}
}

void ProjectManager::_update_selected_preview() {
	if (!is_inside_tree() || !sidebar_preview_panel || !sidebar_preview_icon || !sidebar_preview_name || !sidebar_preview_path || !project_list) {
		return;
	}

	Vector<ProjectList::Item> selected_projects = project_list->get_selected_projects();
	if (selected_projects.is_empty()) {
		Ref<Texture2D> default_icon = get_editor_theme_icon("DefaultProjectIcon");
		sidebar_preview_icon->set_texture(default_icon);
		sidebar_preview_icon->set_self_modulate(Color(1, 1, 1, 0.45f));
		sidebar_preview_name->set_text(String::utf8("Sin proyecto seleccionado"));
		sidebar_preview_path->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		sidebar_preview_path->set_clip_text(false);
		sidebar_preview_path->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		sidebar_preview_path->set_text(String::utf8("Elige un proyecto de la lista para ver sus acciones."));
		sidebar_preview_path->set_tooltip_text(String::utf8("Elige un proyecto de la lista para ver sus acciones."));
		return;
	}

	if (selected_projects.size() > 1) {
		Ref<Texture2D> icon = get_editor_theme_icon("DefaultProjectIcon");
		int single_idx = project_list->get_single_selected_index();
		if (ProjectListItemControl *ctrl = project_list->get_bound_control_for_project(single_idx)) {
			if (!ctrl->should_load_project_icon() && ctrl->get_icon_texture().is_valid()) {
				icon = ctrl->get_icon_texture();
			}
		}
		sidebar_preview_icon->set_texture(icon);
		sidebar_preview_icon->set_self_modulate(Color(1, 1, 1, 1));
		sidebar_preview_name->set_text(vformat(String::utf8("%d proyectos seleccionados"), selected_projects.size()));
		sidebar_preview_path->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		sidebar_preview_path->set_clip_text(false);
		sidebar_preview_path->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
		sidebar_preview_path->set_text(String::utf8("Usa Editar, Ejecutar o Eliminar de la lista."));
		sidebar_preview_path->set_tooltip_text(String::utf8("Usa Editar, Ejecutar o Eliminar de la lista."));
		return;
	}

	const ProjectList::Item &item = selected_projects[0];
	Ref<Texture2D> icon = get_editor_theme_icon("DefaultProjectIcon");
	int single_idx = project_list->get_single_selected_index();
	if (ProjectListItemControl *ctrl = project_list->get_bound_control_for_project(single_idx)) {
		if (!ctrl->should_load_project_icon() && ctrl->get_icon_texture().is_valid()) {
			icon = ctrl->get_icon_texture();
		}
	}
	sidebar_preview_icon->set_texture(icon);
	sidebar_preview_icon->set_self_modulate(item.missing ? Color(1, 1, 1, 0.55f) : Color(1, 1, 1, 1));
	sidebar_preview_name->set_text(item.project_name.is_empty() ? String::utf8("Proyecto sin título") : item.project_name);

	sidebar_preview_path->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	sidebar_preview_path->set_clip_text(true);
	sidebar_preview_path->set_text_overrun_behavior(TextServer::OVERRUN_NO_TRIMMING);
	Ref<Font> path_font = sidebar_preview_path->get_theme_font(SceneStringName(font));
	int path_size = sidebar_preview_path->get_theme_font_size(SceneStringName(font_size));
	float max_w = sidebar_preview_path->get_size().x;
	if (max_w < 80.0f && sidebar_panel) {
		max_w = MAX(140.0f * EDSCALE, sidebar_panel->get_custom_minimum_size().x - 92.0f * EDSCALE);
	}
	max_w = MAX(120.0f * EDSCALE, max_w);
	String elided = ProjectList::elide_path_middle(item.path, path_font, path_size, max_w);
	if (item.missing) {
		sidebar_preview_path->set_text(String::utf8("Faltante • ") + elided);
	} else {
		sidebar_preview_path->set_text(elided);
	}
	sidebar_preview_path->set_tooltip_text(item.path);
}

void ProjectManager::_on_search_term_changed(const String &p_term) {
	project_list->set_search_term(p_term);
	project_list->sort_projects();

	// Select the first visible project in the list.
	// This makes it possible to open a project without ever touching the mouse,
	// as the search field is automatically focused on startup.
	project_list->select_first_visible_project();
	_update_project_buttons();
	_update_list_placeholder();
}

void ProjectManager::_on_search_term_submitted(const String &p_text) {
	if (current_main_view != MAIN_VIEW_PROJECTS) {
		return;
	}

	_open_selected_projects_check_recovery_mode();
}

LineEdit *ProjectManager::get_search_box() {
	return search_box;
}

// Project tag management.

void ProjectManager::_manage_project_tags() {
	for (int i = 0; i < project_tags->get_child_count(); i++) {
		project_tags->get_child(i)->queue_free();
	}

	const ProjectList::Item item = project_list->get_selected_projects()[0];
	current_project_tags = item.tags;
	for (const String &tag : current_project_tags) {
		ProjectTag *tag_control = memnew(ProjectTag(tag, true));
		project_tags->add_child(tag_control);
		tag_control->connect_button_to(callable_mp(this, &ProjectManager::_delete_project_tag).bind(tag));
	}

	tag_edit_error->hide();
	tag_manage_dialog->popup_centered(Vector2i(500, 0) * EDSCALE);
}

void ProjectManager::_add_project_tag(const String &p_tag) {
	if (current_project_tags.has(p_tag)) {
		return;
	}
	current_project_tags.append(p_tag);

	ProjectTag *tag_control = memnew(ProjectTag(p_tag, true));
	project_tags->add_child(tag_control);
	tag_control->connect_button_to(callable_mp(this, &ProjectManager::_delete_project_tag).bind(p_tag));
}

void ProjectManager::_delete_project_tag(const String &p_tag) {
	current_project_tags.erase(p_tag);
	for (int i = 0; i < project_tags->get_child_count(); i++) {
		ProjectTag *tag_control = Object::cast_to<ProjectTag>(project_tags->get_child(i));
		if (tag_control && tag_control->get_tag() == p_tag) {
			memdelete(tag_control);
			break;
		}
	}
}

void ProjectManager::_apply_project_tags() {
	PackedStringArray tags;
	for (int i = 0; i < project_tags->get_child_count(); i++) {
		ProjectTag *tag_control = Object::cast_to<ProjectTag>(project_tags->get_child(i));
		if (tag_control) {
			tags.append(tag_control->get_tag());
		}
	}

	const String project_godot = project_list->get_selected_projects()[0].path.path_join("project.godot");
	ProjectSettings *cfg = memnew(ProjectSettings(project_godot));
	if (!cfg->is_project_loaded()) {
		memdelete(cfg);
		tag_edit_error->set_text(vformat(TTR("Couldn't load project at '%s'. It may be missing or corrupted."), project_godot));
		tag_edit_error->show();
		callable_mp((Window *)tag_manage_dialog, &Window::show).call_deferred(); // Make sure the dialog does not disappear.
		return;
	} else {
		tags.sort();
		cfg->set("application/config/tags", tags);
		Error err = cfg->save_custom(project_godot);
		memdelete(cfg);

		if (err != OK) {
			tag_edit_error->set_text(vformat(TTR("Couldn't save project at '%s' (error %d)."), project_godot, err));
			tag_edit_error->show();
			callable_mp((Window *)tag_manage_dialog, &Window::show).call_deferred();
			return;
		}
	}

	_on_projects_updated();
}

void ProjectManager::_set_new_tag_name(const String p_name) {
	create_tag_dialog->get_ok_button()->set_disabled(true);
	if (p_name.strip_edges().is_empty()) {
		tag_error->set_text(TTRC("Tag name can't be empty."));
		return;
	}

	if (p_name[0] == '_' || p_name[p_name.length() - 1] == '_') {
		tag_error->set_text(TTRC("Tag name can't begin or end with underscore."));
		return;
	}

	bool was_underscore = false;
	for (const char32_t &c : p_name.span()) {
		// Treat spaces as underscores, as we convert spaces to underscores automatically in the tag input field.
		if (c == '_' || c == ' ') {
			if (was_underscore) {
				tag_error->set_text(TTRC("Tag name can't contain consecutive underscores or spaces."));
				return;
			}
			was_underscore = true;
		} else {
			was_underscore = false;
		}
	}

	for (const String &c : forbidden_tag_characters) {
		if (p_name.contains(c)) {
			tag_error->set_text(vformat(TTR("These characters are not allowed in tags: %s."), String(" ").join(forbidden_tag_characters)));
			return;
		}
	}

	tag_error->set_text("");
	create_tag_dialog->get_ok_button()->set_disabled(false);
}

void ProjectManager::_create_new_tag() {
	if (!tag_error->get_text().is_empty()) {
		return;
	}
	create_tag_dialog->hide(); // When using text_submitted, need to hide manually.

	// Enforce a valid tag name (no spaces, lowercase only) automatically.
	// The project manager displays underscores as spaces, and capitalization is performed automatically.
	const String new_tag = new_tag_name->get_text().strip_edges().to_lower().replace_char(' ', '_');
	add_new_tag(new_tag);
	_add_project_tag(new_tag);
}

void ProjectManager::add_new_tag(const String &p_tag) {
	if (!tag_set.has(p_tag)) {
		tag_set.insert(p_tag);
		ProjectTag *tag_control = memnew(ProjectTag(p_tag));
		all_tags->add_child(tag_control);
		all_tags->move_child(tag_control, -2);
		tag_control->connect_button_to(callable_mp(this, &ProjectManager::_add_project_tag).bind(p_tag));
	}
}

// Project converter/migration tool.

#ifndef DISABLE_DEPRECATED
void ProjectManager::_minor_project_migrate() {
	const ProjectList::Item migrated_project = project_list->get_selected_projects()[0];

	if (version_convert_feature.begins_with("4.3")) {
		// Migrate layout after scale changes.
		const float edscale = EDSCALE;
		if (edscale != 1.0) {
			Ref<ConfigFile> layout_file;
			layout_file.instantiate();

			const String layout_path = migrated_project.path.path_join(".godot/editor/editor_layout.cfg");
			Error err = layout_file->load(layout_path);
			if (err == OK) {
				for (int i = 0; i < 4; i++) {
					const String key = "dock_hsplit_" + itos(i + 1);
					int old_value = layout_file->get_value("docks", key, 0);
					if (old_value != 0) {
						layout_file->set_value("docks", key, old_value / edscale);
					}
				}
				layout_file->save(layout_path);
			}
		}
	}
}
#endif

void ProjectManager::_full_convert_button_pressed() {
	ask_update_settings->hide();

	if (ask_update_backup->is_pressed()) {
		ask_update_backup->set_pressed(false);

		_duplicate_project_with_action(POST_DUPLICATE_ACTION_FULL_CONVERSION);
		return;
	}

	ask_full_convert_dialog->popup_centered(Size2i(600.0 * EDSCALE, 0));
	ask_full_convert_dialog->get_cancel_button()->grab_focus();
}

void ProjectManager::_migration_guide_button_pressed() {
	const String url = vformat("%s/tutorials/migrating/index.html", GODOT_VERSION_DOCS_URL);
	OS::get_singleton()->shell_open(url);
}

void ProjectManager::_perform_full_project_conversion() {
	Vector<ProjectList::Item> selected_list = project_list->get_selected_projects();
	if (selected_list.is_empty()) {
		return;
	}

	const String &path = selected_list[0].path;

	print_line("Converting project: " + path);
	List<String> args;
	args.push_back("--path");
	args.push_back(path);
	args.push_back("--convert-3to4");
	args.push_back("--rendering-driver");
	args.push_back(OS::get_singleton()->get_current_rendering_driver_name());

	Error err = OS::get_singleton()->create_instance(args);
	ERR_FAIL_COND(err);

	project_list->set_project_version(path, GODOT4_CONFIG_VERSION);
}

// Input and I/O.

void ProjectManager::shortcut_input(const Ref<InputEvent> &p_ev) {
	ERR_FAIL_COND(p_ev.is_null());

	Ref<InputEventKey> k = p_ev;

	if (k.is_valid()) {
		if (!k->is_pressed()) {
			return;
		}

		// Pressing Command + Q quits the Project Manager
		// This is handled by the platform implementation on macOS,
		// so only define the shortcut on other platforms
#ifndef MACOS_ENABLED
		if (k->get_keycode_with_modifiers() == (KeyModifierMask::META | Key::Q)) {
			_dim_window();
			get_tree()->quit();
		}
#endif

		if (current_main_view != MAIN_VIEW_PROJECTS) {
			return;
		}

		bool keycode_handled = true;

		switch (k->get_keycode()) {
			case Key::HOME: {
				if (project_list->get_project_count() > 0) {
					project_list->ensure_project_visible(0);
				}

			} break;
			case Key::END: {
				if (project_list->get_project_count() > 0) {
					project_list->ensure_project_visible(project_list->get_project_count() - 1);
				}

			} break;
			case Key::F: {
				if (k->is_command_or_control_pressed()) {
					search_box->grab_focus();
				} else {
					keycode_handled = false;
				}
			} break;
			case Key::A: {
				if (k->is_command_or_control_pressed()) {
					if (k->is_shift_pressed()) {
						project_list->deselect_all_visible_projects();
					} else {
						project_list->select_all_visible_projects();
					}
					_update_project_buttons();
				}
			} break;
			default: {
				keycode_handled = false;
			} break;
		}

		if (keycode_handled) {
			accept_event();
		}
	}
}

void ProjectManager::_files_dropped(PackedStringArray p_files) {
#ifdef WEB_ENABLED
	if (p_files.size() == 1 && p_files[0].ends_with(".zip")) {
		const String &file = p_files[0];
		Error err = DirAccess::rename_absolute(file, "/tmp/install.zip"); // Cleaned up at shutdown.
		if (err) {
			_show_error(vformat("Error importing the ZIP file: %d", err));
			return;
		}
		_install_project("/tmp/install.zip", file.get_file().get_basename().capitalize());
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	PackedStringArray errors;
	PackedStringArray folders;
	const String home = "/home/web_user";
	for (const String &folder : p_files) {
		if (!da->dir_exists(folder)) {
			if (folder.ends_with(".zip")) {
				errors.push_back("Importing multiple ZIP projects is not supported.");
			} else {
				errors.push_back(vformat("Unrecognized file: %s", folder.get_file()));
			}
			continue;
		}
		const String &name = folder.get_file();
		const String base = folder.get_base_dir();
		const String dest = home + "/" + name;
		if (da->dir_exists(dest)) {
			errors.push_back(vformat("Cannot import folder '%s', path already exists.", name));
			continue;
		}
		// Move dropped folder to the home folder destination
		List<String> dirs;
		dirs.push_back(name);
		Error err = OK;
		while (dirs.size()) {
			const String cur = dirs.front()->get();
			dirs.pop_front();
			err = DirAccess::make_dir_absolute(home + "/" + cur);
			if (err != OK) {
				errors.push_back(vformat("Failed to create folder: '%s/%s'", home, cur));
				break;
			}
			for (const String &F : DirAccess::get_files_at(base + "/" + cur)) {
				const String src = base + "/" + cur + "/" + F;
				const String dst = home + "/" + cur + "/" + F;
				err = DirAccess::rename_absolute(src, dst);
				if (err != OK) {
					errors.push_back(vformat("Failed to move file: '%s' to '%s'", src, dst));
					break;
				}
			}
			if (err != OK) {
				break;
			}
			for (const String &D : DirAccess::get_directories_at(base + "/" + cur)) {
				dirs.push_back(cur + "/" + D);
			}
		}
		if (err != OK) {
			// Try removing the import destination.
			Ref<DirAccess> dab = DirAccess::open(dest);
			if (dab.is_valid()) {
				dab->erase_contents_recursive();
				dab.unref();
				DirAccess::remove_absolute(dest);
			}
		} else {
			folders.push_back(dest);
		}
	}

	if (errors.size()) {
		_show_error(String("\n").join(errors));
	}
	project_list->find_projects_multiple(folders);
#else
	// TODO: Support installing multiple ZIPs at the same time?
	if (p_files.size() == 1 && p_files[0].ends_with(".zip")) {
		const String &file = p_files[0];
		_install_project(file, file.get_file().get_basename().capitalize());
		return;
	}

	HashSet<String> folders_set;
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	for (int i = 0; i < p_files.size(); i++) {
		const String &file = p_files[i];
		folders_set.insert(da->dir_exists(file) ? file : file.get_base_dir());
	}
	ERR_FAIL_COND(folders_set.is_empty()); // This can't really happen, we consume every dropped file path above.

	PackedStringArray folders;
	for (const String &E : folders_set) {
		folders.push_back(E);
	}
	project_list->find_projects_multiple(folders);
#endif // WEB_ENABLED
}

void ProjectManager::_titlebar_resized() {
	DisplayServer::get_singleton()->window_set_window_buttons_offset(Vector2i(title_bar->get_global_position().y + title_bar->get_size().y / 2, title_bar->get_global_position().y + title_bar->get_size().y / 2), DisplayServerEnums::MAIN_WINDOW_ID);
	const Vector3i &margin = DisplayServer::get_singleton()->window_get_safe_title_margins(DisplayServerEnums::MAIN_WINDOW_ID);
	if (left_menu_spacer) {
		int w = (root_container->is_layout_rtl()) ? margin.y : margin.x;
		left_menu_spacer->set_custom_minimum_size(Size2(w, 0));
	}
	if (right_menu_spacer) {
		int w = (root_container->is_layout_rtl()) ? margin.x : margin.y;
		right_menu_spacer->set_custom_minimum_size(Size2(w, 0));
	}
	if (title_bar) {
		title_bar->set_custom_minimum_size(Size2(0, margin.z - title_bar->get_global_position().y));
	}
}

void ProjectManager::_open_donate_page() {
	OS::get_singleton()->shell_open("https://fund.godotengine.org/?ref=project_manager");
}

// Object methods.

ProjectManager::ProjectManager() {
	if (EditorNode::get_singleton()) {
		return;
	}
	singleton = this;

	// Turn off some servers we aren't going to be using in the Project Manager.
	NavigationServer3D::get_singleton()->set_active(false);
	PhysicsServer3D::get_singleton()->set_active(false);
	PhysicsServer2D::get_singleton()->set_active(false);

	// Initialize settings.
	{
		if (!EditorPaths::get_singleton()) {
			EditorPaths::create();
		}
		if (!EditorSettings::get_singleton()) {
			const bool was_editor_hint = Engine::get_singleton()->is_editor_hint();
			Engine::get_singleton()->set_editor_hint(true);
			EditorSettings::create();
			Engine::get_singleton()->set_editor_hint(was_editor_hint);
		}
		EditorSettings::get_singleton()->set_optimize_save(false); // Just write settings as they come.

		{
			bool agile_input_event_flushing = EDITOR_GET("input/buffering/agile_event_flushing");
			bool use_accumulated_input = EDITOR_GET("input/buffering/use_accumulated_input");

			Input::get_singleton()->set_agile_input_event_flushing(agile_input_event_flushing);
			Input::get_singleton()->set_use_accumulated_input(use_accumulated_input);
		}

		int display_scale = EDITOR_GET("interface/editor/appearance/display_scale");

		switch (display_scale) {
			case 0:
				// Try applying a suitable display scale automatically.
				EditorScale::set_scale(EditorSettings::get_auto_display_scale());
				break;
			case 1:
				EditorScale::set_scale(0.75);
				break;
			case 2:
				EditorScale::set_scale(1.0);
				break;
			case 3:
				EditorScale::set_scale(1.25);
				break;
			case 4:
				EditorScale::set_scale(1.5);
				break;
			case 5:
				EditorScale::set_scale(1.75);
				break;
			case 6:
				EditorScale::set_scale(2.0);
				break;
			default:
				EditorScale::set_scale(EDITOR_GET("interface/editor/appearance/custom_display_scale"));
				break;
		}
		FileDialog::set_get_icon_callback(callable_mp_static(ProjectManager::_file_dialog_get_icon));
		FileDialog::set_get_thumbnail_callback(callable_mp_static(ProjectManager::_file_dialog_get_thumbnail));

		FileDialog::set_default_show_hidden_files(EDITOR_GET("filesystem/file_dialog/show_hidden_files"));
		FileDialog::set_default_display_mode((FileDialog::DisplayMode)EDITOR_GET("filesystem/file_dialog/display_mode").operator int());

		int swap_cancel_ok = EDITOR_GET("interface/editor/appearance/accept_dialog_cancel_ok_buttons");
		if (swap_cancel_ok != 0) { // 0 is auto, set in register_scene based on DisplayServer.
			// Swap on means OK first.
			AcceptDialog::set_swap_cancel_ok(swap_cancel_ok == 2);
		}

		OS::get_singleton()->set_low_processor_usage_mode(true);
	}

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	if (!EditorNode::get_singleton()) {
		EditorHelpHighlighter::create_singleton();
	}
#endif

	SceneTree::get_singleton()->get_root()->connect("files_dropped", callable_mp(this, &ProjectManager::_files_dropped));

	// Initialize UI.
	{
		int pm_root_dir = EDITOR_GET("interface/editor/localization/ui_layout_direction");
		Control::set_root_layout_direction(pm_root_dir);
		Window::set_root_layout_direction(pm_root_dir);

		EditorThemeManager::initialize();
		theme = EditorThemeManager::generate_theme();
		DisplayServer::set_early_window_clear_color_override(true, theme->get_color(SNAME("background"), EditorStringName(Editor)));

		set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

		_build_icon_type_cache(theme);
	}

	// Project manager layout.

	background_panel = memnew(Panel);
	add_child(background_panel);
	background_panel->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	root_container = memnew(MarginContainer);
	add_child(root_container);
	root_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	main_vbox = memnew(VBoxContainer);
	root_container->add_child(main_vbox);

	// Header bar (Cabecera).
	bool can_expand = bool(EDITOR_GET("interface/editor/appearance/expand_to_title")) && DisplayServer::get_singleton()->has_feature(DisplayServerEnums::FEATURE_EXTEND_TO_TITLE);

	{
		header_bar = memnew(PanelContainer);
		main_vbox->add_child(header_bar);

		title_bar = memnew(EditorTitleBar);
		header_bar->add_child(title_bar);

		if (can_expand) {
			left_menu_spacer = memnew(Control);
			left_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			title_bar->add_child(left_menu_spacer);
		}

		HBoxContainer *left_hbox = memnew(HBoxContainer);
		left_hbox->set_alignment(BoxContainer::ALIGNMENT_BEGIN);
		left_hbox->set_h_size_flags(Control::SIZE_FILL);
		left_hbox->add_theme_constant_override("separation", int(10 * EDSCALE));
		title_bar->add_child(left_hbox);

		title_bar_logo = memnew(Button);
		title_bar_logo->set_flat(true);
		title_bar_logo->set_expand_icon(true);
		title_bar_logo->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		title_bar_logo->add_theme_constant_override("icon_max_width", int(32 * EDSCALE));
		title_bar_logo->set_custom_minimum_size(Size2(38, 38) * EDSCALE);
		title_bar_logo->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		title_bar_logo->set_tooltip_text(String::utf8("Acerca de ClockGears Engine"));
		left_hbox->add_child(title_bar_logo);
		title_bar_logo->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_show_about));

		VBoxContainer *brand_vb = memnew(VBoxContainer);
		brand_vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		brand_vb->add_theme_constant_override("separation", int(1 * EDSCALE));
		left_hbox->add_child(brand_vb);

		header_title_label = memnew(Label);
		header_title_label->set_text("ClockGears Engine");
		brand_vb->add_child(header_title_label);

		header_subtitle_label = memnew(Label);
		header_subtitle_label->set_text(String::utf8("Administrador de proyectos"));
		brand_vb->add_child(header_subtitle_label);

		bool global_menu = !bool(EDITOR_GET("interface/editor/appearance/use_embedded_menu")) && NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU);
		if (global_menu) {
			MenuBar *main_menu_bar = memnew(MenuBar);
			main_menu_bar->set_start_index(0);
			main_menu_bar->set_prefer_global_menu(true);
			left_hbox->add_child(main_menu_bar);

			if (NativeMenu::get_singleton()->has_system_menu(NativeMenu::WINDOW_MENU_ID)) {
				PopupMenu *window_menu = memnew(PopupMenu);
				window_menu->set_system_menu(NativeMenu::WINDOW_MENU_ID);
				window_menu->set_name(TTRC("Window"));
				main_menu_bar->add_child(window_menu);
			}
			if (NativeMenu::get_singleton()->has_system_menu(NativeMenu::HELP_MENU_ID)) {
				PopupMenu *help_menu = memnew(PopupMenu);
				help_menu->set_system_menu(NativeMenu::HELP_MENU_ID);
				help_menu->set_name(TTRC("Help"));
				main_menu_bar->add_child(help_menu);
			}
		}

		main_view_toggles = memnew(HBoxContainer);
		main_view_toggles->hide(); // Una única vista ("Proyectos locales"); estructura conservada para futuras pestañas.
		left_hbox->add_child(main_view_toggles);
		main_view_toggles_group.instantiate();

		left_spacer = memnew(Control);
		left_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		left_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		left_spacer->set_stretch_ratio(0.5f);
		left_spacer->set_custom_minimum_size(Size2(8 * EDSCALE, 0));
		title_bar->add_child(left_spacer);

		search_box = memnew(LineEdit);
		search_box->set_placeholder(String::utf8("Buscar proyectos…"));
		search_box->set_accessibility_name(String::utf8("Buscar proyectos"));
		search_box->set_tooltip_text(String::utf8("Filtra proyectos por nombre, etiqueta o componente de ruta.\nIncluye `/` para filtrar también por ruta completa."));
		search_box->set_clear_button_enabled(true);
		search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		search_box->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		search_box->set_stretch_ratio(1.6f);
		search_box->set_custom_minimum_size(Size2(200 * EDSCALE, 34 * EDSCALE));
		search_box->connect(SceneStringName(text_changed), callable_mp(this, &ProjectManager::_on_search_term_changed));
		search_box->connect(SceneStringName(text_submitted), callable_mp(this, &ProjectManager::_on_search_term_submitted));
		title_bar->add_child(search_box);
		if (can_expand) {
			title_bar->set_center_control(search_box);
		}

		right_spacer = memnew(Control);
		right_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		right_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		right_spacer->set_stretch_ratio(0.5f);
		right_spacer->set_custom_minimum_size(Size2(8 * EDSCALE, 0));
		title_bar->add_child(right_spacer);

		HBoxContainer *right_hbox = memnew(HBoxContainer);
		right_hbox->set_alignment(BoxContainer::ALIGNMENT_END);
		right_hbox->set_h_size_flags(Control::SIZE_FILL);
		title_bar->add_child(right_hbox);

		quick_settings_button = memnew(Button);
		quick_settings_button->set_text(String::utf8("Configuración"));
		quick_settings_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		quick_settings_button->set_custom_minimum_size(Size2(0, 34) * EDSCALE);
		quick_settings_button->add_theme_constant_override("h_separation", int(6 * EDSCALE));
		right_hbox->add_child(quick_settings_button);
		quick_settings_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_show_quick_settings));

		if (can_expand) {
			right_menu_spacer = memnew(Control);
			right_menu_spacer->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			title_bar->add_child(right_menu_spacer);
		}
	}

	main_view_container = memnew(PanelContainer);
	main_view_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->add_child(main_view_container);

	// Project list view.
	{
		local_projects_vb = memnew(VBoxContainer);
		local_projects_vb->set_name("LocalProjectsTab");
		local_projects_vb->add_theme_constant_override("separation", int(10 * EDSCALE));
		_add_main_view(MAIN_VIEW_PROJECTS, String::utf8("Proyectos locales"), Ref<Texture2D>(), local_projects_vb);

		// Barra de herramientas (Toolbar).
		{
			HBoxContainer *hb = memnew(HBoxContainer);
			hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hb->add_theme_constant_override("separation", int(8 * EDSCALE));
			local_projects_vb->add_child(hb);

			section_title_label = memnew(Label);
			section_title_label->set_text(String::utf8("Proyectos locales"));
			section_title_label->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
			hb->add_child(section_title_label);

			Control *toolbar_gap = memnew(Control);
			toolbar_gap->set_custom_minimum_size(Size2(4 * EDSCALE, 0));
			hb->add_child(toolbar_gap);

			create_btn = memnew(Button);
			create_btn->set_text(String::utf8("+ Crear Proyecto"));
			create_btn->set_theme_type_variation("ClockGearsPrimaryButton");
			create_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			create_btn->set_shortcut(ED_SHORTCUT("project_manager/new_project", TTRC("New Project"), KeyModifierMask::CMD_OR_CTRL | Key::N));
			create_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_new_project));
			hb->add_child(create_btn);

			import_btn = memnew(Button);
			import_btn->set_text(String::utf8("Importar"));
			import_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			import_btn->set_shortcut(ED_SHORTCUT("project_manager/import_project", TTRC("Import Project"), KeyModifierMask::CMD_OR_CTRL | Key::I));
			import_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_import_project));
			hb->add_child(import_btn);

			scan_btn = memnew(Button);
			scan_btn->set_text(TTRC("Scan"));
			scan_btn->set_shortcut(ED_SHORTCUT("project_manager/scan_projects", TTRC("Scan Projects"), KeyModifierMask::CMD_OR_CTRL | Key::S));
			scan_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_scan_projects));
			scan_btn->hide(); // Limpieza: solo Crear e Importar visibles en la barra.
			hb->add_child(scan_btn);

			loading_label = memnew(Label(TTRC("Loading, please wait...")));
			loading_label->set_accessibility_live(AccessibilityServerEnums::AccessibilityLiveMode::LIVE_ASSERTIVE);
			loading_label->hide();
			hb->add_child(loading_label);

			Control *toolbar_spacer = memnew(Control);
			toolbar_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			toolbar_spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
			hb->add_child(toolbar_spacer);

			view_mode_group.instantiate();
			HBoxContainer *view_switch_hb = memnew(HBoxContainer);
			view_switch_hb->add_theme_constant_override("separation", int(2 * EDSCALE));
			hb->add_child(view_switch_hb);

			view_list_btn = memnew(Button);
			view_list_btn->set_toggle_mode(true);
			view_list_btn->set_button_group(view_mode_group);
			view_list_btn->set_pressed_no_signal(true);
			view_list_btn->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			view_list_btn->set_custom_minimum_size(Size2(32, 32) * EDSCALE);
			view_list_btn->set_tooltip_text(String::utf8("Vista de lista"));
			view_list_btn->set_accessibility_name(String::utf8("Vista de lista"));
			view_list_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_on_view_mode_pressed).bind(0));
			view_switch_hb->add_child(view_list_btn);

			view_grid_btn = memnew(Button);
			view_grid_btn->set_toggle_mode(true);
			view_grid_btn->set_button_group(view_mode_group);
			view_grid_btn->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			view_grid_btn->set_custom_minimum_size(Size2(32, 32) * EDSCALE);
			view_grid_btn->set_tooltip_text(String::utf8("Vista de cuadrícula"));
			view_grid_btn->set_accessibility_name(String::utf8("Vista de cuadrícula"));
			view_grid_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_on_view_mode_pressed).bind(1));
			view_switch_hb->add_child(view_grid_btn);

			sort_label = memnew(Label);
			sort_label->set_text(String::utf8("Ordenar por"));
			sort_label->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
			hb->add_child(sort_label);

			filter_option = memnew(OptionButton);
			filter_option->set_clip_text(true);
			filter_option->set_fit_to_longest_item(false);
			filter_option->set_custom_minimum_size(Size2(138 * EDSCALE, 32 * EDSCALE));
			filter_option->set_accessibility_name(String::utf8("Ordenar por"));
			filter_option->connect(SceneStringName(item_selected), callable_mp(this, &ProjectManager::_on_order_option_changed));
			hb->add_child(filter_option);

			filter_option->add_item(String::utf8("Último editado"));
			filter_option->add_item(String::utf8("Nombre"));
			filter_option->add_item(String::utf8("Ruta"));
			filter_option->add_item(String::utf8("Etiquetas"));
		}

		// Project list and right panel ("Acciones de proyecto").
		{
			HBoxContainer *project_list_hbox = memnew(HBoxContainer);
			project_list_hbox->add_theme_constant_override("separation", int(12 * EDSCALE));
			project_list_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			local_projects_vb->add_child(project_list_hbox);

			project_list_panel = memnew(PanelContainer);
			project_list_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_panel->set_clip_contents(true);
			project_list_hbox->add_child(project_list_panel);

			project_list = memnew(ProjectList);
			project_list->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			project_list_panel->add_child(project_list);
			project_list->connect(ProjectList::SIGNAL_LIST_CHANGED, callable_mp(this, &ProjectManager::_update_project_buttons));
			project_list->connect(ProjectList::SIGNAL_LIST_CHANGED, callable_mp(this, &ProjectManager::_update_list_placeholder));
			project_list->connect(ProjectList::SIGNAL_SELECTION_CHANGED, callable_mp(this, &ProjectManager::_update_project_buttons));
			project_list->connect(ProjectList::SIGNAL_PROJECT_ASK_OPEN, callable_mp(this, &ProjectManager::_open_selected_projects_check_recovery_mode));
			project_list->connect(ProjectList::SIGNAL_MENU_OPTION_SELECTED, callable_mp(this, &ProjectManager::_project_list_menu_option));
			project_list->connect(SceneStringName(minimum_size_changed), callable_mp(this, &ProjectManager::_update_compact_mode).bind(true));

			// Empty / no-match placeholder centered over list panel.
			{
				empty_list_placeholder = memnew(VBoxContainer);
				empty_list_placeholder->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
				empty_list_placeholder->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				empty_list_placeholder->set_custom_minimum_size(Size2(320, 0) * EDSCALE);
				empty_list_placeholder->add_theme_constant_override("separation", int(14 * EDSCALE));
				empty_list_placeholder->hide();
				project_list_panel->add_child(empty_list_placeholder);

				empty_list_message = memnew(RichTextLabel);
				empty_list_message->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
				empty_list_message->set_use_bbcode(true);
				empty_list_message->set_fit_content(true);
				empty_list_message->set_h_size_flags(SIZE_EXPAND_FILL);
				empty_list_message->add_theme_style_override(CoreStringName(normal), memnew(StyleBoxEmpty));
				empty_list_placeholder->add_child(empty_list_message);

				FlowContainer *empty_list_actions = memnew(FlowContainer);
				empty_list_actions->set_alignment(FlowContainer::ALIGNMENT_CENTER);
				empty_list_actions->add_theme_constant_override("h_separation", int(8 * EDSCALE));
				empty_list_actions->add_theme_constant_override("v_separation", int(8 * EDSCALE));
				empty_list_placeholder->add_child(empty_list_actions);

				empty_list_create_project = memnew(Button);
				empty_list_create_project->set_text(String::utf8("Crear Proyecto"));
				empty_list_create_project->set_theme_type_variation("ClockGearsPrimaryButton");
				empty_list_create_project->set_custom_minimum_size(Size2(0, 34) * EDSCALE);
				empty_list_actions->add_child(empty_list_create_project);
				empty_list_create_project->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_new_project));

				empty_list_import_project = memnew(Button);
				empty_list_import_project->set_text(String::utf8("Importar"));
				empty_list_import_project->set_custom_minimum_size(Size2(0, 34) * EDSCALE);
				empty_list_actions->add_child(empty_list_import_project);
				empty_list_import_project->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_import_project));

				empty_list_clear_filter_btn = memnew(Button);
				empty_list_clear_filter_btn->set_text(String::utf8("Limpiar búsqueda"));
				empty_list_clear_filter_btn->set_custom_minimum_size(Size2(0, 34) * EDSCALE);
				empty_list_clear_filter_btn->hide();
				empty_list_actions->add_child(empty_list_clear_filter_btn);
				empty_list_clear_filter_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_clear_search_filter));

				empty_list_open_assetlib = nullptr;
				empty_list_online_warning = nullptr;
			}

			// Right panel: "Acciones de proyecto".
			sidebar_panel = memnew(PanelContainer);
			sidebar_panel->set_custom_minimum_size(Size2(340, 0) * EDSCALE);
			sidebar_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_hbox->add_child(sidebar_panel);

			project_list_sidebar = memnew(VBoxContainer);
			project_list_sidebar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_sidebar->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_sidebar->add_theme_constant_override("separation", int(10 * EDSCALE));
			sidebar_panel->add_child(project_list_sidebar);

			sidebar_header_label = memnew(Label);
			sidebar_header_label->set_text(String::utf8("Acciones de proyecto"));
			project_list_sidebar->add_child(sidebar_header_label);

			sidebar_preview_panel = memnew(PanelContainer);
			sidebar_preview_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_sidebar->add_child(sidebar_preview_panel);

			HBoxContainer *preview_hb = memnew(HBoxContainer);
			preview_hb->add_theme_constant_override("separation", int(10 * EDSCALE));
			sidebar_preview_panel->add_child(preview_hb);

			sidebar_preview_icon = memnew(TextureRect);
			sidebar_preview_icon->set_custom_minimum_size(Size2(48, 48) * EDSCALE);
			sidebar_preview_icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
			sidebar_preview_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
			sidebar_preview_icon->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			sidebar_preview_icon->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
			preview_hb->add_child(sidebar_preview_icon);

			VBoxContainer *preview_text_vb = memnew(VBoxContainer);
			preview_text_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			preview_text_vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
			preview_text_vb->add_theme_constant_override("separation", int(2 * EDSCALE));
			preview_hb->add_child(preview_text_vb);

			sidebar_preview_name = memnew(Label);
			sidebar_preview_name->set_clip_text(true);
			sidebar_preview_name->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			sidebar_preview_name->set_custom_minimum_size(Size2(1, 0));
			preview_text_vb->add_child(sidebar_preview_name);

			sidebar_preview_path = memnew(Label);
			sidebar_preview_path->set_clip_text(true);
			sidebar_preview_path->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			sidebar_preview_path->set_custom_minimum_size(Size2(1, 0));
			preview_text_vb->add_child(sidebar_preview_path);

			ScrollContainer *sidebar_scroll_container = memnew(ScrollContainer);
			sidebar_scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			sidebar_scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			project_list_sidebar->add_child(sidebar_scroll_container);

			VBoxContainer *sidebar_buttons_container = memnew(VBoxContainer);
			sidebar_buttons_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			sidebar_buttons_container->add_theme_constant_override("separation", int(6 * EDSCALE));
			sidebar_scroll_container->add_child(sidebar_buttons_container);

			open_btn_container = memnew(HBoxContainer);
			open_btn_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			sidebar_buttons_container->add_child(open_btn_container);

			open_btn = memnew(Button);
			open_btn->set_text(String::utf8("Editar Proyecto"));
			open_btn->set_theme_type_variation("ClockGearsPrimaryButton");
			open_btn->set_custom_minimum_size(Size2(0, 36) * EDSCALE);
			open_btn->set_shortcut(ED_SHORTCUT("project_manager/edit_project", TTRC("Edit Project"), KeyModifierMask::CMD_OR_CTRL | Key::E));
			open_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_open_selected_projects_check_recovery_mode));
			open_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			open_btn_container->add_child(open_btn);

			open_options_btn = memnew(Button);
			open_options_btn->set_custom_minimum_size(Size2(34, 36) * EDSCALE);
			open_options_btn->set_accessibility_name(String::utf8("Opciones de edición"));
			open_options_btn->set_tooltip_text(String::utf8("Más opciones para abrir el proyecto"));
			open_options_btn->set_icon_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
			open_options_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_open_options_popup));
			open_btn_container->add_child(open_options_btn);

			open_options_popup = memnew(PopupMenu);
			open_options_popup->add_item(TTRC("Edit in verbose mode"));
			open_options_popup->add_item(TTRC("Edit in recovery mode"));
			open_options_popup->connect(SceneStringName(id_pressed), callable_mp(this, &ProjectManager::_on_open_options_selected));
			open_options_btn->add_child(open_options_popup);

			run_btn = memnew(Button);
			run_btn->set_text(String::utf8("Ejecutar / Probar"));
			run_btn->set_custom_minimum_size(Size2(0, 34) * EDSCALE);
			run_btn->set_shortcut(ED_SHORTCUT("project_manager/run_project", TTRC("Run Project"), KeyModifierMask::CMD_OR_CTRL | Key::R));
			run_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_run_project));
			sidebar_buttons_container->add_child(run_btn);

			HBoxContainer *row_rename_dup = memnew(HBoxContainer);
			row_rename_dup->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			row_rename_dup->add_theme_constant_override("separation", int(6 * EDSCALE));
			sidebar_buttons_container->add_child(row_rename_dup);

			rename_btn = memnew(Button);
			rename_btn->set_text(String::utf8("Renombrar"));
			rename_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			rename_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			rename_btn->set_shortcut(ED_SHORTCUT("project_manager/rename_project", TTRC("Rename Project"), Key::F2));
			rename_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_rename_project));
			row_rename_dup->add_child(rename_btn);

			duplicate_btn = memnew(Button);
			duplicate_btn->set_text(String::utf8("Duplicar"));
			duplicate_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			duplicate_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			duplicate_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_duplicate_project));
			row_rename_dup->add_child(duplicate_btn);

			manage_tags_btn = memnew(Button);
			manage_tags_btn->set_text(String::utf8("Administrar Etiquetas"));
			manage_tags_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			manage_tags_btn->set_shortcut(ED_SHORTCUT("project_manager/project_tags", TTRC("Manage Tags"), KeyModifierMask::CMD_OR_CTRL | Key::T));
			sidebar_buttons_container->add_child(manage_tags_btn);

			show_in_folder_btn = memnew(Button);
			show_in_folder_btn->set_text(String::utf8("Ver en Carpeta"));
			show_in_folder_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			show_in_folder_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_show_project_in_file_manager));
			sidebar_buttons_container->add_child(show_in_folder_btn);

			erase_btn = memnew(Button);
			erase_btn->set_text(String::utf8("Eliminar de la lista"));
			erase_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			erase_btn->set_shortcut(ED_SHORTCUT("project_manager/remove_project", TTRC("Remove Project"), Key::KEY_DELETE));
			erase_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_erase_project));
			sidebar_buttons_container->add_child(erase_btn);

			sidebar_buttons_container->add_child(memnew(HSeparator));

			erase_missing_btn = memnew(Button);
			erase_missing_btn->set_text(String::utf8("Eliminar Faltantes"));
			erase_missing_btn->set_custom_minimum_size(Size2(0, 32) * EDSCALE);
			erase_missing_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_erase_missing_projects));
			sidebar_buttons_container->add_child(erase_missing_btn);

			/* donate_btn purged for ClockGears Engine */
		}
	}

	// Asset store view purged for ClockGears Engine.
	asset_library = nullptr;

	// Footer bar (Pie de página: izquierda sin conexión, derecha versión).
	{
		footer_bar = memnew(HBoxContainer);
		footer_bar->add_theme_constant_override("separation", int(12 * EDSCALE));
		main_vbox->add_child(footer_bar);

#ifdef ENGINE_UPDATE_CHECK_ENABLED
		EngineUpdateLabel *update_label = memnew(EngineUpdateLabel);
		update_label->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		footer_bar->add_child(update_label);
		footer_status_ctrl = update_label;
		update_label->connect("offline_clicked", callable_mp(this, &ProjectManager::_show_quick_settings));
#else
		Label *offline_footer_lbl = memnew(Label);
		offline_footer_lbl->set_text(String::utf8("Modo sin conexión • Comprobación de actualizaciones deshabilitada"));
		offline_footer_lbl->set_self_modulate(Color(1, 1, 1, 0.72f));
		footer_bar->add_child(offline_footer_lbl);
		footer_status_ctrl = offline_footer_lbl;
#endif

		Control *footer_spacer = memnew(Control);
		footer_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		footer_spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		footer_bar->add_child(footer_spacer);

		EditorVersionButton *version_btn = memnew(EditorVersionButton(EditorVersionButton::FORMAT_WITH_BUILD));
		version_btn->set_self_modulate(Color(1, 1, 1, 0.72f));
		footer_bar->add_child(version_btn);
	}

	// Dialogs.
	{
		quick_settings_dialog = memnew(QuickSettingsDialog);
		add_child(quick_settings_dialog);
		quick_settings_dialog->connect("restart_required", callable_mp(this, &ProjectManager::_restart_confirmed));

		scan_dir = memnew(EditorFileDialog);
		scan_dir->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
		scan_dir->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
		scan_dir->set_title(TTRC("Select a Folder to Scan")); // Must be after mode or it's overridden.
		scan_dir->set_current_dir(EDITOR_GET("filesystem/directories/default_project_path"));
		add_child(scan_dir);
		scan_dir->connect("dir_selected", callable_mp(project_list, &ProjectList::find_projects));

		erase_missing_ask = memnew(ConfirmationDialog);
		erase_missing_ask->set_flag(Window::FLAG_RESIZE_DISABLED, true);
		erase_missing_ask->set_ok_button_text(TTRC("Remove All"));
		erase_missing_ask->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_erase_missing_projects_confirm));
		add_child(erase_missing_ask);

		erase_ask = memnew(ConfirmationDialog);
		erase_ask->set_ok_button_text(TTRC("Remove"));
		erase_ask->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_erase_project_confirm));
		add_child(erase_ask);

		VBoxContainer *erase_ask_vb = memnew(VBoxContainer);
		erase_ask->add_child(erase_ask_vb);

		erase_ask_label = memnew(Label);
		erase_ask_label->set_focus_mode(FOCUS_ACCESSIBILITY);
		erase_ask_vb->add_child(erase_ask_label);

		// Comment out for now until we have a better warning system to
		// ensure users delete their project only.
		//delete_project_contents = memnew(CheckBox);
		//delete_project_contents->set_text(TTRC("Also delete project contents (no undo!)"));
		//erase_ask_vb->add_child(delete_project_contents);

		multi_open_ask = memnew(ConfirmationDialog);
		multi_open_ask->set_flag(Window::FLAG_RESIZE_DISABLED, true);
		multi_open_ask->set_ok_button_text(TTRC("Edit"));
		multi_open_ask->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_open_selected_projects));
		add_child(multi_open_ask);

		multi_run_ask = memnew(ConfirmationDialog);
		multi_run_ask->set_flag(Window::FLAG_RESIZE_DISABLED, true);
		multi_run_ask->set_ok_button_text(TTRC("Run"));
		multi_run_ask->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_run_project_confirm));
		add_child(multi_run_ask);

		open_recovery_mode_ask = memnew(ConfirmationDialog);
		open_recovery_mode_ask->set_min_size(Size2(550, 70) * EDSCALE);
		open_recovery_mode_ask->set_autowrap(true);
		open_recovery_mode_ask->add_button(TTRC("Edit normally"))->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_on_recovery_mode_popup_open_normal));
		open_recovery_mode_ask->set_ok_button_text(TTRC("Edit in Recovery Mode"));
		open_recovery_mode_ask->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_on_recovery_mode_popup_open_recovery));
		add_child(open_recovery_mode_ask);

		ask_update_settings = memnew(ConfirmationDialog);
		add_child(ask_update_settings);
		ask_update_vb = memnew(VBoxContainer);
		ask_update_settings->add_child(ask_update_vb);
		ask_update_label = memnew(Label);
		ask_update_label->set_focus_mode(FOCUS_ACCESSIBILITY);
		ask_update_label->set_custom_minimum_size(Size2(300 * EDSCALE, 1));
		ask_update_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
		ask_update_label->set_v_size_flags(SIZE_EXPAND_FILL);
		ask_update_vb->add_child(ask_update_label);
		ask_update_backup = memnew(CheckBox);
		ask_update_backup->set_text(TTRC("Back Up Project First"));
		ask_update_backup->set_h_size_flags(SIZE_SHRINK_CENTER);
		ask_update_vb->add_child(ask_update_backup);
		ask_upgrade_tool = memnew(CheckBox);
		ask_upgrade_tool->set_text(TTRC("Upgrade All Project Files"));
		ask_upgrade_tool->set_tooltip_text(TTRC("Automatically runs the upgrade tool. This may take a while to finish. The editor may restart during the process."));
		ask_upgrade_tool->set_h_size_flags(SIZE_SHRINK_CENTER);
		ask_update_vb->add_child(ask_upgrade_tool);
		ask_update_settings->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_open_selected_projects_with_migration));
		int ed_swap_cancel_ok = EDITOR_GET("interface/editor/appearance/accept_dialog_cancel_ok_buttons");
		if (ed_swap_cancel_ok == 0) {
			ed_swap_cancel_ok = DisplayServer::get_singleton()->get_swap_cancel_ok() ? 2 : 1;
		}
		full_convert_button = ask_update_settings->add_button(TTRC("Convert Full Project"), ed_swap_cancel_ok != 2);
		full_convert_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_full_convert_button_pressed));
		migration_guide_button = ask_update_settings->add_button(TTRC("See Migration Guide"), ed_swap_cancel_ok != 2);
		migration_guide_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_migration_guide_button_pressed));

		ask_full_convert_dialog = memnew(ConfirmationDialog);
		ask_full_convert_dialog->set_autowrap(true);
		ask_full_convert_dialog->set_text(TTRC("This option will perform full project conversion, updating scenes, resources and scripts from Godot 3 to work in Godot 4.\n\nNote that this is a best-effort conversion, i.e. it makes upgrading the project easier, but it will not open out-of-the-box and will still require manual adjustments.\n\nIMPORTANT: Make sure to backup your project before converting, as this operation makes it impossible to open it in older versions of ClockGears Engine."));
		ask_full_convert_dialog->connect(SceneStringName(confirmed), callable_mp(this, &ProjectManager::_perform_full_project_conversion));
		add_child(ask_full_convert_dialog);

		project_dialog = memnew(ProjectDialog);
		project_dialog->connect("projects_updated", callable_mp(this, &ProjectManager::_on_projects_updated));
		project_dialog->connect("project_created", callable_mp(this, &ProjectManager::_on_project_created));
		project_dialog->connect("project_duplicated", callable_mp(this, &ProjectManager::_on_project_duplicated));
		add_child(project_dialog);

		error_dialog = memnew(AcceptDialog);
		error_dialog->set_title(TTRC("Error"));
		add_child(error_dialog);
	}

	// Tag management.
	{
		tag_manage_dialog = memnew(ConfirmationDialog);
		add_child(tag_manage_dialog);
		tag_manage_dialog->set_title(TTRC("Manage Project Tags"));
		tag_manage_dialog->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_apply_project_tags));
		manage_tags_btn->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_manage_project_tags));

		VBoxContainer *tag_vb = memnew(VBoxContainer);
		tag_manage_dialog->add_child(tag_vb);

		Label *label = memnew(Label(TTRC("Project Tags")));
		tag_vb->add_child(label);
		label->set_theme_type_variation("HeaderMedium");
		label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		label = memnew(Label(TTRC("Click tag to remove it from the project.")));
		tag_vb->add_child(label);
		label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		project_tags = memnew(HFlowContainer);
		tag_vb->add_child(project_tags);
		project_tags->set_custom_minimum_size(Vector2(0, 100) * EDSCALE);

		tag_vb->add_child(memnew(HSeparator));

		label = memnew(Label(TTRC("All Tags")));
		tag_vb->add_child(label);
		label->set_theme_type_variation("HeaderMedium");
		label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		label = memnew(Label(TTRC("Click tag to add it to the project.")));
		tag_vb->add_child(label);
		label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		all_tags = memnew(HFlowContainer);
		tag_vb->add_child(all_tags);
		all_tags->set_custom_minimum_size(Vector2(0, 100) * EDSCALE);

		tag_edit_error = memnew(Label);
		tag_vb->add_child(tag_edit_error);
		tag_edit_error->set_autowrap_mode(TextServer::AUTOWRAP_WORD);

		create_tag_dialog = memnew(ConfirmationDialog);
		tag_manage_dialog->add_child(create_tag_dialog);
		create_tag_dialog->set_title(TTRC("Create New Tag"));
		create_tag_dialog->get_ok_button()->connect(SceneStringName(pressed), callable_mp(this, &ProjectManager::_create_new_tag));

		tag_vb = memnew(VBoxContainer);
		create_tag_dialog->add_child(tag_vb);

		Label *info = memnew(Label(TTRC("Tags are capitalized automatically when displayed.")));
		tag_vb->add_child(info);

		new_tag_name = memnew(LineEdit);
		tag_vb->add_child(new_tag_name);
		new_tag_name->set_accessibility_name(TTRC("New Tag Name"));
		new_tag_name->set_placeholder(TTRC("example_tag (will display as Example Tag)"));
		new_tag_name->connect(SceneStringName(text_changed), callable_mp(this, &ProjectManager::_set_new_tag_name));
		new_tag_name->connect(SceneStringName(text_submitted), callable_mp(this, &ProjectManager::_create_new_tag).unbind(1));
		create_tag_dialog->connect("about_to_popup", callable_mp(new_tag_name, &LineEdit::clear));
		create_tag_dialog->connect("about_to_popup", callable_mp((Control *)new_tag_name, &Control::grab_focus).bind(false), CONNECT_DEFERRED);

		tag_error = memnew(Label);
		tag_error->set_focus_mode(FOCUS_ACCESSIBILITY);
		tag_vb->add_child(tag_error);

		create_tag_btn = memnew(Button);
		create_tag_btn->set_accessibility_name(TTRC("Create Tag"));
		all_tags->add_child(create_tag_btn);
		create_tag_btn->connect(SceneStringName(pressed), callable_mp((Window *)create_tag_dialog, &Window::popup_centered).bind(Vector2i(500, 0) * EDSCALE));

		_set_new_tag_name("");
	}

	// Initialize project list.
	{
		project_list->load_project_list();

		Ref<DirAccess> dir_access = DirAccess::create(DirAccess::AccessType::ACCESS_FILESYSTEM);

		String default_project_path = EDITOR_GET("filesystem/directories/default_project_path");
		if (!default_project_path.is_empty() && !dir_access->dir_exists(default_project_path)) {
			Error error = dir_access->make_dir_recursive(default_project_path);
			if (error != OK) {
				ERR_PRINT("Could not create default project directory at: " + default_project_path);
			}
		}

		String autoscan_path = EDITOR_GET("filesystem/directories/autoscan_project_path");
		if (!autoscan_path.is_empty()) {
			if (dir_access->dir_exists(autoscan_path)) {
				project_list->find_projects(autoscan_path);
			} else {
				Error error = dir_access->make_dir_recursive(autoscan_path);
				if (error != OK) {
					ERR_PRINT("Could not create project autoscan directory at: " + autoscan_path);
				}
			}
		}
		project_list->update_project_list();
		initialized = true;
	}

	// Extend menu bar to window title.
	if (can_expand) {
		DisplayServer::get_singleton()->process_events();
		DisplayServer::get_singleton()->window_set_flag(DisplayServerEnums::WINDOW_FLAG_EXTEND_TO_TITLE, true, DisplayServerEnums::MAIN_WINDOW_ID);
		title_bar->set_can_move_window(true);
		title_bar->connect(SceneStringName(item_rect_changed), callable_mp(this, &ProjectManager::_titlebar_resized));
	}

	_update_size_limits();
}

ProjectManager::~ProjectManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
	if (!EditorNode::get_singleton()) {
		EditorInspector::cleanup_plugins();

		EditorHelp::cleanup_doc();

#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
		EditorHelpHighlighter::free_singleton();
#endif

		if (EditorSettings::get_singleton()) {
			EditorSettings::destroy();
		}

		EditorThemeManager::finalize();
	}
}

void ProjectManager::reload_projects_from_config_for_test(const String &p_cfg_path) {
	if (!project_list) {
		return;
	}
	project_list->set_config_path_for_test(p_cfg_path);
	project_list->load_project_list();
	project_list->update_project_list();
	_update_project_buttons();
	_update_list_placeholder();
}

void ProjectManager::simulate_select_view_mode(int p_mode) {
	_on_view_mode_pressed(p_mode);
}

void ProjectManager::simulate_select_order_option(int p_idx) {
	if (filter_option && p_idx >= 0 && p_idx < filter_option->get_item_count()) {
		filter_option->select(p_idx);
	}
	_on_order_option_changed(p_idx);
}

Dictionary ProjectManager::inspect_layout_metrics() const {
	Dictionary out;
	const Rect2 root_rect = get_rect();
	out["root_size"] = root_rect.size;
	out["window_title"] = (SceneTree::get_singleton() && SceneTree::get_singleton()->get_root()) ? SceneTree::get_singleton()->get_root()->get_title() : String();
	out["header_pos"] = header_bar ? header_bar->get_global_rect().position : Vector2();
	out["header_size"] = header_bar ? header_bar->get_global_rect().size : Vector2();
	out["search_rect"] = search_box ? search_box->get_global_rect() : Rect2();
	out["settings_rect"] = quick_settings_button ? quick_settings_button->get_global_rect() : Rect2();
	out["list_panel_rect"] = project_list_panel ? project_list_panel->get_global_rect() : Rect2();
	out["sidebar_visible"] = sidebar_panel ? sidebar_panel->is_visible_in_tree() : false;
	out["sidebar_rect"] = sidebar_panel ? sidebar_panel->get_global_rect() : Rect2();
	out["preview_name"] = sidebar_preview_name ? sidebar_preview_name->get_text() : String();
	out["preview_path"] = sidebar_preview_path ? sidebar_preview_path->get_text() : String();
	out["preview_tooltip"] = sidebar_preview_path ? sidebar_preview_path->get_tooltip_text() : String();
	out["placeholder_visible"] = empty_list_placeholder ? empty_list_placeholder->is_visible_in_tree() : false;
	out["placeholder_text"] = empty_list_message ? empty_list_message->get_text() : String();
	out["clear_filter_visible"] = empty_list_clear_filter_btn ? empty_list_clear_filter_btn->is_visible_in_tree() : false;
	out["view_mode"] = project_list ? project_list->get_view_mode() : 0;
	out["order_option"] = filter_option ? filter_option->get_selected() : 0;
	out["project_count"] = project_list ? project_list->get_project_count() : 0;
	out["filtered_count"] = project_list ? project_list->get_filtered_project_count() : 0;
	out["pool_count"] = project_list ? project_list->get_pool_control_count() : 0;
	out["visible_bound_count"] = project_list ? project_list->get_visible_bound_count() : 0;
	out["hscroll_max"] = (project_list && project_list->get_h_scroll_bar()) ? project_list->get_h_scroll_bar()->get_max() : 0.0;
	out["hscroll_visible"] = (project_list && project_list->get_h_scroll_bar()) ? project_list->get_h_scroll_bar()->is_visible_in_tree() : false;

	out["open_disabled"] = open_btn ? open_btn->is_disabled() : true;
	out["open_options_disabled"] = open_options_btn ? open_options_btn->is_disabled() : true;
	out["run_disabled"] = run_btn ? run_btn->is_disabled() : true;
	out["rename_disabled"] = rename_btn ? rename_btn->is_disabled() : true;
	out["duplicate_disabled"] = duplicate_btn ? duplicate_btn->is_disabled() : true;
	out["tags_disabled"] = manage_tags_btn ? manage_tags_btn->is_disabled() : true;
	out["folder_disabled"] = show_in_folder_btn ? show_in_folder_btn->is_disabled() : true;
	out["erase_disabled"] = erase_btn ? erase_btn->is_disabled() : true;
	out["erase_missing_disabled"] = erase_missing_btn ? erase_missing_btn->is_disabled() : true;

	// Detectar cualquier hijo visible cuyo rect se salga horizontalmente del root o tenga solape en barras horizontales.
	PackedStringArray overflows;
	Vector<Node *> stack;
	stack.push_back(const_cast<ProjectManager *>(this));
	while (!stack.is_empty()) {
		Node *n = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		if (Control *c = Object::cast_to<Control>(n)) {
			if (!c->is_visible_in_tree()) {
				continue;
			}
			// No inspeccionar el interior del ScrollContainer vertical salvo su propio viewport.
			if (c != project_list && Object::cast_to<ScrollContainer>(c->get_parent()) == project_list) {
				continue;
			}
			Rect2 r = c->get_global_rect();
			if (r.position.x < -2.0f || r.position.x + r.size.x > root_rect.size.x + 2.0f) {
				overflows.push_back(c->get_class() + ":" + String(c->get_name()) + " (" + String::num(r.position.x) + ".." + String::num(r.position.x + r.size.x) + " vs " + String::num(root_rect.size.x) + ")");
			}
		}
		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}
	}
	out["horizontal_overflows"] = overflows;
	PackedStringArray v_overflows;
	stack.clear();
	stack.push_back(const_cast<ProjectManager *>(this));
	while (!stack.is_empty()) {
		Node *n = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		if (Control *c = Object::cast_to<Control>(n)) {
			if (!c->is_visible_in_tree()) {
				continue;
			}
			Control *par = Object::cast_to<Control>(c->get_parent());
			if (par && Object::cast_to<ScrollContainer>(par)) {
				continue;
			}
			Rect2 r = c->get_global_rect();
			if (r.position.y < -2.0f || r.position.y + r.size.y > root_rect.size.y + 2.0f) {
				v_overflows.push_back(c->get_class() + ":" + String(c->get_name()) + " (" + String::num(r.position.y) + ".." + String::num(r.position.y + r.size.y) + " vs " + String::num(root_rect.size.y) + ")");
			}
		}
		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}
	}
	out["vertical_overflows"] = v_overflows;
	out["footer_rect"] = footer_bar ? footer_bar->get_global_rect() : Rect2();
	out["footer_visible"] = footer_bar ? footer_bar->is_visible_in_tree() : false;
	out["footer_status_rect"] = footer_status_ctrl ? footer_status_ctrl->get_global_rect() : Rect2();
	out["sidebar_path_rect"] = sidebar_preview_path ? sidebar_preview_path->get_global_rect() : Rect2();
	out["sidebar_path_lines"] = sidebar_preview_path ? sidebar_preview_path->get_line_count() : 0;
	return out;
}

void ProjectManager::_bind_methods() {
	ClassDB::bind_static_method("ProjectManager", D_METHOD("compute_16_9_usable_window_rect", "usable_rect"), &ProjectManager::compute_16_9_usable_window_rect);
	ClassDB::bind_method(D_METHOD("get_search_box"), &ProjectManager::get_search_box);
	ClassDB::bind_method(D_METHOD("get_project_list"), &ProjectManager::get_project_list);
	ClassDB::bind_method(D_METHOD("reload_projects_from_config_for_test", "cfg_path"), &ProjectManager::reload_projects_from_config_for_test);
	ClassDB::bind_method(D_METHOD("simulate_select_view_mode", "mode"), &ProjectManager::simulate_select_view_mode);
	ClassDB::bind_method(D_METHOD("simulate_select_order_option", "idx"), &ProjectManager::simulate_select_order_option);
	ClassDB::bind_method(D_METHOD("inspect_layout_metrics"), &ProjectManager::inspect_layout_metrics);
}
