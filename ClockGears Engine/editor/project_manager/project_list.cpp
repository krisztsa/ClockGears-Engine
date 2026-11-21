/**************************************************************************/
/*  project_list.cpp                                                      */
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

#include "project_list.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/dir_access.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/project_manager/project_manager.h"
#include "editor/project_manager/project_tag.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"

void ProjectListItemControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			if (icon_needs_reload) {
				project_icon->set_texture(get_editor_theme_icon(SNAME("ProjectIconLoading")));
			}

			project_title->begin_bulk_theme_override();
			project_title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
			project_title->add_theme_font_size_override(SceneStringName(font_size), get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts)) + int(1 * EDSCALE));
			project_title->add_theme_color_override(SceneStringName(font_color), get_theme_color(SceneStringName(font_color), SNAME("ProjectList")));
			project_title->end_bulk_theme_override();

			project_path->begin_bulk_theme_override();
			project_path->add_theme_font_size_override(SceneStringName(font_size), MAX(11, get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts)) - int(1 * EDSCALE)));
			project_path->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("path_color"), SNAME("ProjectList")));
			project_path->end_bulk_theme_override();

			last_edited_info->begin_bulk_theme_override();
			last_edited_info->add_theme_font_size_override(SceneStringName(font_size), MAX(11, get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts)) - int(2 * EDSCALE)));
			last_edited_info->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("date_color"), SNAME("ProjectList")));
			last_edited_info->end_bulk_theme_override();

			const int badge_sz = MAX(10, get_theme_font_size(SNAME("main_size"), EditorStringName(EditorFonts)) - int(2 * EDSCALE));
			project_version->add_theme_font_size_override(SceneStringName(font_size), badge_sz);
			missing_badge->add_theme_font_size_override(SceneStringName(font_size), badge_sz);

			switch (version_match_type) {
				case VersionMatchType::PROJECT_USES_OLDER_MAJOR:
					project_different_version->set_texture(get_editor_theme_icon(SNAME("ProjectUpgradeMajor")));
					break;
				case VersionMatchType::PROJECT_USES_OLDER_MINOR:
					project_different_version->set_texture(get_editor_theme_icon(SNAME("ProjectUpgrade")));
					break;
				case VersionMatchType::PROJECT_USES_NEWER_MAJOR:
					project_different_version->set_texture(get_editor_theme_icon(SNAME("ProjectDowngradeMajor")));
					break;
				case VersionMatchType::PROJECT_USES_NEWER_MINOR:
					project_different_version->set_texture(get_editor_theme_icon(SNAME("ProjectDowngrade")));
					break;
				default:
					break;
			}

			project_unsupported_features->set_texture(get_editor_theme_icon(SNAME("NodeWarning")));

			favorite_focus_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			favorite_active_color = has_theme_color(SNAME("favorite_color"), SNAME("ProjectList"))
					? get_theme_color(SNAME("favorite_color"), SNAME("ProjectList"))
					: favorite_focus_color;
			_update_favorite_icon();
			_update_favorite_button_focus_color();

			if (project_is_missing) {
				explore_button->set_button_icon(get_editor_theme_icon(SNAME("FileBroken")));
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
			} else {
				explore_button->set_button_icon(get_editor_theme_icon(SNAME("ClockGearsFolderLine")));
#endif
			}
			if (touch_menu_button) {
				touch_menu_button->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
			}
			_update_path_display();
			queue_redraw();
		} break;

		case NOTIFICATION_RESIZED: {
			_update_path_display();
		} break;

		case NOTIFICATION_MOUSE_ENTER: {
			is_hovering = true;
			queue_redraw();
			queue_accessibility_update();
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			is_hovering = false;
			queue_redraw();
			queue_accessibility_update();
		} break;

		case NOTIFICATION_ACCESSIBILITY_UPDATE: {
			RID ae = get_accessibility_element();
			ERR_FAIL_COND(ae.is_null());

			AccessibilityServer::get_singleton()->update_set_role(ae, AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_BOX_OPTION);
			AccessibilityServer::get_singleton()->update_set_name(ae, TTR("Project") + " " + project_title->get_text());
			AccessibilityServer::get_singleton()->update_set_value(ae, project_title->get_text());

			AccessibilityServer::get_singleton()->update_add_action(ae, AccessibilityServerEnums::AccessibilityAction::ACTION_CLICK, callable_mp(this, &ProjectListItemControl::_accessibility_action_open));
			AccessibilityServer::get_singleton()->update_add_action(ae, AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_INTO_VIEW, callable_mp(this, &ProjectListItemControl::_accessibility_action_scroll_into_view));
			AccessibilityServer::get_singleton()->update_add_action(ae, AccessibilityServerEnums::AccessibilityAction::ACTION_FOCUS, callable_mp(this, &ProjectListItemControl::_accessibility_action_focus));
			AccessibilityServer::get_singleton()->update_add_action(ae, AccessibilityServerEnums::AccessibilityAction::ACTION_BLUR, callable_mp(this, &ProjectListItemControl::_accessibility_action_blur));

			AccessibilityServer::get_singleton()->update_set_list_item_index(ae, bound_project_index);
			AccessibilityServer::get_singleton()->update_set_list_item_level(ae, 0);
			AccessibilityServer::get_singleton()->update_set_list_item_selected(ae, is_selected);
		} break;

		case NOTIFICATION_FOCUS_ENTER: {
			ProjectList *pl = get_list();
			if (pl && bound_project_index >= 0) {
				pl->select_project(bound_project_index, !has_focus(true));
				pl->emit_signal(SNAME(ProjectList::SIGNAL_SELECTION_CHANGED));
			}
		} break;

		case NOTIFICATION_DRAW: {
			const Rect2 r(Point2(), get_size());
			if (is_selected && is_hovering) {
				draw_style_box(get_theme_stylebox(SNAME("hover_pressed"), SNAME("ProjectList")), r);
			} else if (is_selected) {
				draw_style_box(get_theme_stylebox(SNAME("selected"), SNAME("ProjectList")), r);
			} else if (is_hovering) {
				draw_style_box(get_theme_stylebox(SNAME("hovered"), SNAME("ProjectList")), r);
			} else if (project_is_missing && has_theme_stylebox(SNAME("missing"), SNAME("ProjectList"))) {
				draw_style_box(get_theme_stylebox(SNAME("missing"), SNAME("ProjectList")), r);
			} else if (has_theme_stylebox(CoreStringName(normal), SNAME("ProjectList"))) {
				draw_style_box(get_theme_stylebox(CoreStringName(normal), SNAME("ProjectList")), r);
			}
			if (has_focus() && !is_focus_hidden) {
				draw_style_box(get_theme_stylebox(SNAME("focus"), SNAME("ProjectList")), r);
			}
		} break;

		case NOTIFICATION_READY: {
			_update_path_display();
		} break;
	}
}

ProjectList *ProjectListItemControl::get_list() const {
	if (!is_inside_tree() || !get_parent()) {
		return nullptr;
	}
	return Object::cast_to<ProjectList>(get_parent()->get_parent());
}

void ProjectListItemControl::_accessibility_action_scroll_into_view(const Variant &p_data) {
	ProjectList *pl = get_list();
	if (pl && bound_project_index >= 0) {
		pl->ensure_project_visible(bound_project_index);
	}
}

void ProjectListItemControl::_accessibility_action_open(const Variant &p_data) {
	ProjectList *pl = get_list();
	if (pl && !pl->project_opening_initiated) {
		pl->emit_signal(SNAME(ProjectList::SIGNAL_PROJECT_ASK_OPEN));
	}
}

void ProjectListItemControl::_accessibility_action_focus(const Variant &p_data) {
	ProjectList *pl = get_list();
	if (pl && bound_project_index >= 0) {
		pl->ensure_project_visible(bound_project_index);
		pl->select_project(bound_project_index);
	}
}

void ProjectListItemControl::_accessibility_action_blur(const Variant &p_data) {
	ProjectList *pl = get_list();
	if (pl && bound_project_index >= 0) {
		pl->ensure_project_visible(bound_project_index);
		pl->deselect_project(bound_project_index);
	}
}

void ProjectListItemControl::_update_favorite_icon() {
	if (!favorite_button) {
		return;
	}
	if (is_favorite) {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Favorites")));
		favorite_button->set_accessibility_name(TTRC("Remove from Favorites"));
	} else {
		favorite_button->set_texture_normal(get_editor_theme_icon(SNAME("Unfavorite")));
		favorite_button->set_accessibility_name(TTRC("Add to Favorites"));
	}
	_update_favorite_button_focus_color();
}

void ProjectListItemControl::_update_favorite_button_focus_color() {
	if (favorite_button->has_focus()) {
		favorite_button->set_self_modulate(favorite_focus_color);
	} else if (is_favorite) {
		favorite_button->set_self_modulate(favorite_active_color);
	} else {
		favorite_button->set_self_modulate(Color(1.0, 1.0, 1.0, 0.55));
	}
}

void ProjectListItemControl::_update_path_display() {
	if (!project_path) {
		return;
	}
	project_path->set_tooltip_text(full_path_cache);
	const Ref<Font> font = project_path->get_theme_font(SceneStringName(font));
	const int font_size = project_path->get_theme_font_size(SceneStringName(font_size));
	float max_w = project_path->get_size().x;
	if (card_view_mode == ProjectList::VIEW_GRID && get_size().x > 80.0f) {
		const float grid_budget = MAX(90.0f, get_size().x - 56.0f * EDSCALE);
		max_w = (max_w > 8.0f) ? MIN(max_w, grid_budget) : grid_budget;
	} else if (max_w <= 8.0f && get_size().x > 80.0f) {
		max_w = MAX(110.0f, get_size().x - 320.0f * EDSCALE);
	}
	if (max_w > 8.0f && font.is_valid()) {
		project_path->set_text(ProjectList::elide_path_middle(full_path_cache, font, font_size, max_w));
	} else {
		project_path->set_text(full_path_cache);
	}
}

void ProjectListItemControl::apply_view_mode(int p_view_mode) {
	if (card_view_mode == p_view_mode && ((p_view_mode == 0 && root_hbox->is_visible()) || (p_view_mode == 1 && root_vbox->is_visible()))) {
		return;
	}
	card_view_mode = p_view_mode;

	// Mover controles compartidos entre diseño horizontal (Lista) y vertical (Cuadrícula)
	auto _reparent_to = [](Control *p_node, Node *p_new_parent) {
		if (!p_node || !p_new_parent || p_node->get_parent() == p_new_parent) {
			return;
		}
		if (p_node->get_parent()) {
			p_node->get_parent()->remove_child(p_node);
		}
		p_new_parent->add_child(p_node);
	};

	if (p_view_mode == ProjectList::VIEW_GRID) {
		root_hbox->hide();
		root_vbox->show();

		_reparent_to(favorite_button, grid_top_hb);
		if (favorite_button->get_parent() == grid_top_hb) {
			grid_top_hb->move_child(favorite_button, 0);
		}
		_reparent_to(missing_badge, grid_top_hb);
		_reparent_to(project_version, grid_top_hb);

		_reparent_to(project_icon, root_vbox);
		_reparent_to(main_vbox, root_vbox);
		_reparent_to(last_edited_info, root_vbox);
		_reparent_to(tag_container, root_vbox);

		root_vbox->move_child(grid_top_hb, 0);
		root_vbox->move_child(project_icon, 1);
		root_vbox->move_child(main_vbox, 2);
		root_vbox->move_child(last_edited_info, 3);
		root_vbox->move_child(tag_container, 4);

		project_icon->set_custom_minimum_size(Size2(48, 48) * EDSCALE);
		project_icon->set_h_size_flags(SIZE_SHRINK_CENTER);
		main_vbox->set_v_size_flags(SIZE_SHRINK_CENTER);
		project_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		project_path->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		last_edited_info->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		tag_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	} else {
		root_vbox->hide();
		root_hbox->show();

		_reparent_to(favorite_button, root_hbox);
		_reparent_to(project_icon, root_hbox);
		_reparent_to(main_vbox, root_hbox);
		_reparent_to(right_meta_vb, root_hbox);
		if (touch_menu_button) {
			_reparent_to(touch_menu_button, root_hbox);
		}

		root_hbox->move_child(favorite_button, 0);
		root_hbox->move_child(project_icon, 1);
		root_hbox->move_child(main_vbox, 2);
		root_hbox->move_child(right_meta_vb, 3);
		if (touch_menu_button && touch_menu_button->get_parent() == root_hbox) {
			root_hbox->move_child(touch_menu_button, 4);
		}

		_reparent_to(missing_badge, title_hb);
		_reparent_to(project_version, title_hb);
		_reparent_to(project_unsupported_features, title_hb);
		_reparent_to(project_different_version, title_hb);

		title_hb->move_child(project_title, 0);
		title_hb->move_child(missing_badge, 1);
		title_hb->move_child(project_version, 2);

		_reparent_to(last_edited_info, right_meta_vb);
		_reparent_to(tag_container, right_meta_vb);
		right_meta_vb->move_child(last_edited_info, 0);
		right_meta_vb->move_child(tag_container, 1);

		favorite_button->set_v_size_flags(SIZE_SHRINK_CENTER);
		project_icon->set_custom_minimum_size(Size2(64, 64) * EDSCALE);
		project_icon->set_h_size_flags(SIZE_SHRINK_BEGIN);
		project_icon->set_v_size_flags(SIZE_SHRINK_CENTER);
		main_vbox->set_v_size_flags(SIZE_SHRINK_CENTER);
		project_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		project_path->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		last_edited_info->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		tag_container->set_alignment(BoxContainer::ALIGNMENT_END);
	}
	callable_mp(this, &ProjectListItemControl::_update_path_display).call_deferred();
}

void ProjectListItemControl::_favorite_button_pressed() {
	emit_signal(SNAME("favorite_pressed"));
}

void ProjectListItemControl::_explore_button_pressed() {
	emit_signal(SNAME("explore_pressed"));
}

void ProjectListItemControl::_request_menu() {
	emit_signal(SNAME("request_menu"), Vector2(touch_menu_button->get_position()));
}

void ProjectListItemControl::set_project_title(const String &p_title) {
	project_title->set_text(p_title);
	project_title->set_tooltip_text(p_title);
	project_title->set_accessibility_name(TTRC("Project Name"));
	queue_accessibility_update();
}

void ProjectListItemControl::set_project_path(const String &p_path) {
	full_path_cache = p_path;
	_update_path_display();
	project_path->set_accessibility_name(TTRC("Project Path"));
	queue_accessibility_update();
}

void ProjectListItemControl::set_tags(const PackedStringArray &p_tags, ProjectList *p_parent_list) {
	if (cached_tags == p_tags) {
		return;
	}
	cached_tags = p_tags;
	while (tag_container->get_child_count() > 0) {
		Node *ch = tag_container->get_child(0);
		tag_container->remove_child(ch);
		memdelete(ch);
	}
	const int max_visible_tags = 3;
	const int show_n = MIN((int)p_tags.size(), max_visible_tags);
	for (int i = 0; i < show_n; i++) {
		const String &tag = p_tags[i];
		ProjectTag *tag_control = memnew(ProjectTag(tag));
		tag_container->add_child(tag_control);
		tag_control->connect_button_to(callable_mp(p_parent_list, &ProjectList::add_search_tag).bind(tag));
	}
	tag_container->set_visible(show_n > 0);
}

void ProjectListItemControl::set_project_icon(const Ref<Texture2D> &p_icon) {
	icon_needs_reload = p_icon.is_null();

	project_icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	const float icon_px = (card_view_mode == ProjectList::VIEW_GRID) ? 48.0f : 64.0f;
	project_icon->set_custom_minimum_size(Size2(icon_px, icon_px) * EDSCALE);
	project_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	project_icon->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);

	if (p_icon.is_valid()) {
		project_icon->set_texture(p_icon);
	} else {
		project_icon->set_texture(get_editor_theme_icon(SNAME("ProjectIconLoading")));
	}
}

void ProjectListItemControl::set_last_edited_info(const String &p_info) {
	last_edited_info->set_text(p_info);
}

void ProjectListItemControl::set_project_version(const String &p_info) {
	String v = p_info.strip_edges();
	if (v == "?") {
		v = String();
	}
	if (!v.is_empty() && !v.begins_with("v")) {
		v = "v" + v;
	}
	project_version->set_text(v);
	project_version->set_visible(!v.is_empty());
}

void ProjectListItemControl::set_unsupported_features(PackedStringArray p_features) {
	if (p_features.size() > 0) {
		String tooltip_text = "";
		bool unknown_version = false;
		for (int i = 0; i < p_features.size(); i++) {
			if (ProjectList::project_feature_looks_like_version(p_features[i])) {
				PackedStringArray project_version_split = p_features[i].split(".");
				int project_version_major = 0, project_version_minor = 0;
				if (project_version_split.size() >= 2) {
					project_version_major = project_version_split[0].to_int();
					project_version_minor = project_version_split[1].to_int();
				}

				version_match_type = VersionMatchType::PROJECT_USES_SAME;
				if (project_version_major > GODOT_VERSION_MAJOR) {
					version_match_type = VersionMatchType::PROJECT_USES_NEWER_MAJOR;
				} else if (project_version_major < GODOT_VERSION_MAJOR) {
					version_match_type = VersionMatchType::PROJECT_USES_OLDER_MAJOR;
				} else {
					// Project is same major version.
					// Is it the same minor version, or an upgrade or downgrade?
					if (project_version_minor > GODOT_VERSION_MINOR) {
						version_match_type = VersionMatchType::PROJECT_USES_NEWER_MINOR;
					} else if (project_version_minor < GODOT_VERSION_MINOR) {
						version_match_type = VersionMatchType::PROJECT_USES_OLDER_MINOR;
					}
				}

				if (version_match_type != VersionMatchType::PROJECT_USES_SAME) {
					String project_version_tooltip_text = TTR("This project was last edited in a different ClockGears Engine version: ") + p_features[i] + "\n";
					if (version_match_type == VersionMatchType::PROJECT_USES_OLDER_MAJOR || version_match_type == VersionMatchType::PROJECT_USES_OLDER_MINOR) {
						project_version_tooltip_text += vformat(TTR("Opening it will upgrade it to ClockGears Engine %s.%s."), GODOT_VERSION_MAJOR, GODOT_VERSION_MINOR) + "\n";
					} else if (version_match_type == VersionMatchType::PROJECT_USES_NEWER_MAJOR || version_match_type == VersionMatchType::PROJECT_USES_NEWER_MINOR) {
						project_version_tooltip_text += vformat(TTR("Opening it will downgrade it to ClockGears Engine %s.%s."), GODOT_VERSION_MAJOR, GODOT_VERSION_MINOR) + "\n";
						project_version_tooltip_text += TTR("Downgrading projects is not recommended.") + "\n";
					}
					project_version->set_tooltip_text(project_version_tooltip_text);
					project_different_version->hide(); // Una sola insignia de versión en la tarjeta (sin ícono duplicado).
				} else {
					project_different_version->hide();
				}
			} else {
				if (p_features[i] == "3.x") {
					version_match_type = VersionMatchType::PROJECT_USES_OLDER_MAJOR;
					String project_version_tooltip_text = TTR("This project was last edited in a different ClockGears Engine version: ") + p_features[i] + "\n";
					project_version_tooltip_text += vformat(TTR("Opening it will upgrade it to ClockGears Engine %s.%s."), GODOT_VERSION_MAJOR, GODOT_VERSION_MINOR) + "\n";
					project_different_version->set_focus_mode(FOCUS_ACCESSIBILITY);
					project_different_version->set_tooltip_text(project_version_tooltip_text);
					project_different_version->show();
				} else if (p_features[i] == "u-ver") {
					unknown_version = true;
					project_different_version->hide();
				}
			}

			p_features.remove_at(i);
			i--;
		}

		// This is actually triggered when the project.godot file's config_version
		// is less than 4, so perhaps it'd be more accurate to say the engine configuration
		// file's version is not supported...? If the config/features array includes
		// a proper version number, it will be displayed alongside the "unknown version"
		// warning otherwise.
		if (unknown_version) {
			tooltip_text += TTR("This project uses an unknown version of ClockGears Engine.") + "\n";
		}
		if (p_features.size() > 0) {
			String unsupported_features_str = String(", ").join(p_features);
			tooltip_text += TTR("This project uses features unsupported by the current build:") + "\n" + unsupported_features_str;
		}

		if (!tooltip_text.is_empty() && project_version) {
			project_version->set_tooltip_text(tooltip_text);
		}
		project_different_version->hide();
		project_unsupported_features->hide();
	} else {
		project_different_version->hide();
		project_unsupported_features->hide();
	}
}

bool ProjectListItemControl::should_load_project_icon() const {
	return icon_needs_reload;
}

void ProjectListItemControl::set_selected(bool p_selected, bool p_hide_focus) {
	is_selected = p_selected;
	is_focus_hidden = is_selected && p_hide_focus;
	queue_redraw();
	queue_accessibility_update();
}

void ProjectListItemControl::set_is_favorite(bool p_favorite) {
	is_favorite = p_favorite;
	_update_favorite_icon();
}

void ProjectListItemControl::set_is_missing(bool p_missing) {
	project_is_missing = p_missing;
	if (missing_badge) {
		missing_badge->set_visible(p_missing);
	}

	if (project_is_missing) {
		project_icon->set_modulate(Color(1, 1, 1, 0.45));
		explore_button->set_button_icon(get_editor_theme_icon(SNAME("FileBroken")));
		explore_button->set_tooltip_text(TTRC("Error: Project is missing on the filesystem."));
	} else {
		project_icon->set_modulate(Color(1, 1, 1, 1.0));
#if defined(ANDROID_ENABLED) || defined(WEB_ENABLED)
		explore_button->hide();
#else
		explore_button->set_button_icon(get_editor_theme_icon(SNAME("ClockGearsFolderLine")));
		explore_button->set_tooltip_text(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN));
#endif
	}
	queue_redraw();
}

void ProjectListItemControl::set_is_grayed(bool p_grayed) {
	is_grayed = p_grayed;
	if (p_grayed) {
		project_title->set_modulate(Color(1, 1, 1, 0.65));
		project_path->set_modulate(Color(1, 1, 1, 0.55));
		last_edited_info->set_modulate(Color(1, 1, 1, 0.55));
		explore_button->set_modulate(Color(1, 1, 1, 0.85));
	} else {
		project_title->set_modulate(Color(1, 1, 1, 1.0));
		project_path->set_modulate(Color(1, 1, 1, 1.0));
		last_edited_info->set_modulate(Color(1, 1, 1, 1.0));
		explore_button->set_modulate(Color(1, 1, 1, 0.85));
	}
}

void ProjectListItemControl::set_project_title_autowrap() {
	_update_path_display();
}

void ProjectListItemControl::resize_project_title(const int p_title_size) {
	_update_path_display();
}

Ref<Texture2D> ProjectListItemControl::get_icon_texture() const {
	return project_icon ? project_icon->get_texture() : Ref<Texture2D>();
}

void ProjectListItemControl::_bind_methods() {
	ADD_SIGNAL(MethodInfo("favorite_pressed"));
	ADD_SIGNAL(MethodInfo("explore_pressed"));
	ADD_SIGNAL(MethodInfo("request_menu"));
}

ProjectListItemControl::ProjectListItemControl() {
	if (EditorNode::get_singleton()) {
		return;
	}
	set_focus_mode(FocusMode::FOCUS_ALL);
	set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	set_clip_contents(true);

	Ref<StyleBoxEmpty> pad_sb;
	pad_sb.instantiate();
	pad_sb->set_content_margin_individual(int(10 * EDSCALE), int(6 * EDSCALE), int(10 * EDSCALE), int(6 * EDSCALE));
	add_theme_style_override(SceneStringName(panel), pad_sb);

	root_hbox = memnew(HBoxContainer);
	root_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_hbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	add_child(root_hbox);

	root_vbox = memnew(VBoxContainer);
	root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	root_vbox->hide();
	add_child(root_vbox);

	grid_top_hb = memnew(HBoxContainer);
	grid_top_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid_top_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	root_vbox->add_child(grid_top_hb);

	Control *grid_top_spacer = memnew(Control);
	grid_top_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid_top_spacer->set_mouse_filter(MOUSE_FILTER_IGNORE);
	grid_top_hb->add_child(grid_top_spacer);

	favorite_button = memnew(TextureButton);
	favorite_button->set_name("FavoriteButton");
	favorite_button->set_tooltip_text(TTRC("Toggle Favorite"));
	favorite_button->set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	favorite_button->set_mouse_filter(MOUSE_FILTER_PASS);
	favorite_button->set_v_size_flags(SIZE_SHRINK_CENTER);
	root_hbox->add_child(favorite_button);
	favorite_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_favorite_button_pressed));
	favorite_button->connect(SceneStringName(focus_entered), callable_mp(this, &ProjectListItemControl::_update_favorite_button_focus_color));
	favorite_button->connect(SceneStringName(focus_exited), callable_mp(this, &ProjectListItemControl::_update_favorite_button_focus_color));

	project_icon = memnew(TextureRect);
	project_icon->set_name("ProjectIcon");
	project_icon->set_custom_minimum_size(Size2(64, 64) * EDSCALE);
	project_icon->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	project_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	project_icon->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	project_icon->set_v_size_flags(SIZE_SHRINK_CENTER);
	root_hbox->add_child(project_icon);

	main_vbox = memnew(VBoxContainer);
	main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vbox->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	main_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	root_hbox->add_child(main_vbox);

	// Top row: título + insignias (faltante, versión).
	{
		title_hb = memnew(HBoxContainer);
		title_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		title_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		main_vbox->add_child(title_hb);

		project_title = memnew(Label);
		project_title->set_focus_mode(FOCUS_ACCESSIBILITY);
		project_title->set_name("ProjectName");
		project_title->set_clip_text(true);
		project_title->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		project_title->set_custom_minimum_size(Size2(1, 0));
		project_title->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		title_hb->add_child(project_title);

		missing_badge = memnew(Label);
		missing_badge->set_name("MissingBadge");
		missing_badge->set_text(String::utf8("Faltante"));
		missing_badge->set_theme_type_variation("ProjectMissingBadge");
		missing_badge->set_v_size_flags(SIZE_SHRINK_CENTER);
		missing_badge->hide();
		title_hb->add_child(missing_badge);

		project_version = memnew(Label);
		project_version->set_focus_mode(FOCUS_ACCESSIBILITY);
		project_version->set_name("ProjectVersion");
		project_version->set_theme_type_variation("ProjectVersionBadge");
		project_version->set_mouse_filter(Control::MOUSE_FILTER_PASS);
		project_version->set_v_size_flags(SIZE_SHRINK_CENTER);
		title_hb->add_child(project_version);

		project_unsupported_features = memnew(TextureRect);
		project_unsupported_features->set_name("ProjectUnsupportedFeatures");
		project_unsupported_features->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		title_hb->add_child(project_unsupported_features);
		project_unsupported_features->hide();

		project_different_version = memnew(TextureRect);
		project_different_version->set_name("ProjectDifferentVersion");
		project_different_version->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		title_hb->add_child(project_different_version);
		project_different_version->hide();
	}

	// Bottom row: icono carpeta + ruta recortada en medio.
	{
		path_hb = memnew(HBoxContainer);
		path_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		path_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
		main_vbox->add_child(path_hb);

		explore_button = memnew(Button);
		explore_button->set_name("ExploreButton");
		explore_button->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
		explore_button->set_mouse_filter(MOUSE_FILTER_PASS);
		explore_button->set_tooltip_text(TTRC("Open in file manager"));
		explore_button->set_flat(true);
		path_hb->add_child(explore_button);
		explore_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_explore_button_pressed));

		project_path = memnew(Label);
		project_path->set_name("ProjectPath");
		project_path->set_focus_mode(FOCUS_ACCESSIBILITY);
		project_path->set_structured_text_bidi_override(TextServer::STRUCTURED_TEXT_FILE);
		project_path->set_clip_text(true);
		project_path->set_custom_minimum_size(Size2(1, 0));
		project_path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		path_hb->add_child(project_path);
	}

	right_meta_vb = memnew(VBoxContainer);
	right_meta_vb->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	right_meta_vb->add_theme_constant_override("separation", int(4 * EDSCALE));
	root_hbox->add_child(right_meta_vb);

	last_edited_info = memnew(Label);
	last_edited_info->set_focus_mode(FOCUS_ACCESSIBILITY);
	last_edited_info->set_name("LastEditedInfo");
	last_edited_info->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	last_edited_info->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	last_edited_info->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
	last_edited_info->set_tooltip_text(TTRC("Last edited timestamp"));
	right_meta_vb->add_child(last_edited_info);

	tag_container = memnew(HBoxContainer);
	tag_container->set_alignment(BoxContainer::ALIGNMENT_END);
	tag_container->add_theme_constant_override("separation", int(4 * EDSCALE));
	right_meta_vb->add_child(tag_container);

	if (DisplayServer::get_singleton()->is_touchscreen_available()) {
		touch_menu_button = memnew(Button);
		touch_menu_button->set_theme_type_variation(SceneStringName(FlatButton));
		touch_menu_button->set_v_size_flags(SIZE_SHRINK_CENTER);
		root_hbox->add_child(touch_menu_button);
		touch_menu_button->set_mouse_filter(MOUSE_FILTER_PASS);
		touch_menu_button->connect(SceneStringName(pressed), callable_mp(this, &ProjectListItemControl::_request_menu));
	}
}

struct ProjectListComparator {
	ProjectList::FilterOption order_option = ProjectList::FilterOption::EDIT_DATE;

	// operator<
	_FORCE_INLINE_ bool operator()(const ProjectList::Item &a, const ProjectList::Item &b) const {
		if (a.favorite && !b.favorite) {
			return true;
		}
		if (b.favorite && !a.favorite) {
			return false;
		}
		switch (order_option) {
			case ProjectList::PATH:
				return a.path < b.path;
			case ProjectList::EDIT_DATE:
				return a.last_edited > b.last_edited;
			case ProjectList::TAGS:
				return a.tag_sort_string < b.tag_sort_string;
			default:
				return a.project_name < b.project_name;
		}
	}
};

String ProjectList::Item::get_last_edited_string() const {
	if (missing) {
		return TTR("Missing Date");
	}

	OS::TimeZoneInfo tz = OS::get_singleton()->get_time_zone_info();
	return Time::get_singleton()->get_datetime_string_from_unix_time(last_edited + tz.bias * 60, true);
}

// Helpers.

bool ProjectList::project_feature_looks_like_version(const String &p_feature) {
	return p_feature.contains_char('.') && p_feature.substr(0, 3).is_numeric();
}

String ProjectList::elide_path_middle(const String &p_path, const Ref<Font> &p_font, int p_font_size, float p_max_width) {
	if (p_path.is_empty() || p_font.is_null() || p_max_width <= 8.0f) {
		return p_path;
	}
	if (p_font->get_string_size(p_path, HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size).x <= p_max_width) {
		return p_path;
	}
	const String ellipsis = String::utf8("…");
	const int len = p_path.length();
	if (len <= 6) {
		return p_path;
	}

	// Conservar prefijo inicial (~35%) y carpeta final del proyecto (~65%) con "…" en medio.
	int best_head = 3;
	int best_tail = 3;
	for (int total_keep = len - 1; total_keep >= 6; total_keep--) {
		int head = MAX(3, (int)Math::round((double)total_keep * 0.35));
		int tail = MAX(3, total_keep - head);
		if (head + tail >= len) {
			continue;
		}
		const String cand = p_path.substr(0, head) + ellipsis + p_path.substr(len - tail, tail);
		if (p_font->get_string_size(cand, HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size).x <= p_max_width) {
			best_head = head;
			best_tail = tail;
			return cand;
		}
	}
	return p_path.substr(0, best_head) + ellipsis + p_path.substr(len - best_tail, best_tail);
}

// Notifications.

void ProjectList::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			if (is_ready()) {
				for (const Item &item : _projects) {
					if (item.control) {
						_update_project_control_translatable_fields(item);
					}
				}
				update_dock_menu();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			if (project_context_menu) {
				_update_menu_icons();
			}
			_layout_visible_cards();
		} break;

		case NOTIFICATION_RESIZED: {
			_layout_visible_cards();
		} break;

		case NOTIFICATION_PROCESS: {
			// Priorizar carga de iconos visibles primero; luego continuar en segundo plano con el resto.
			bool loaded_visible = false;
			for (ProjectListItemControl *ctrl : _card_pool) {
				if (!ctrl->is_visible()) {
					continue;
				}
				const int pidx = ctrl->get_bound_project_index();
				if (pidx >= 0 && pidx < _projects.size() && !_projects[pidx].icon_loaded) {
					_load_project_icon(pidx);
					loaded_visible = true;
					break;
				}
			}
			if (loaded_visible) {
				break;
			}
			while (_icon_load_index < _projects.size() && _projects[_icon_load_index].icon_loaded) {
				_icon_load_index++;
			}
			if (_icon_load_index < _projects.size()) {
				_load_project_icon(_icon_load_index);
				_icon_load_index++;
			} else if (scan_data && scan_data->scan_in_progress.is_set()) {
				// Wait for the thread.
			} else {
				set_process(false);
				if (scan_data) {
					_scan_finished();
				}
			}
		} break;

		case NOTIFICATION_ACCESSIBILITY_UPDATE: {
			RID ae = get_accessibility_element();
			ERR_FAIL_COND(ae.is_null());

			AccessibilityServer::get_singleton()->update_set_role(ae, AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_BOX);
			AccessibilityServer::get_singleton()->update_set_list_item_count(ae, _projects.size());
			AccessibilityServer::get_singleton()->update_set_flag(ae, AccessibilityServerEnums::AccessibilityFlags::FLAG_MULTISELECTABLE, false);
		}
	}
}

// Projects scan.

void ProjectList::_scan_thread(void *p_scan_data) {
	ScanData *scan_data = static_cast<ScanData *>(p_scan_data);

	for (const String &base_path : scan_data->paths_to_scan) {
		print_verbose(vformat("Scanning for projects in \"%s\".", base_path));
		_scan_folder_recursive(base_path, &scan_data->found_projects, scan_data->scan_in_progress);

		if (!scan_data->scan_in_progress.is_set()) {
			print_verbose("Scan aborted.");
			break;
		}
	}
	print_verbose(vformat("Found %d project(s).", scan_data->found_projects.size()));
	scan_data->scan_in_progress.clear();
}

void ProjectList::_scan_finished() {
	if (scan_data->scan_in_progress.is_set()) {
		// Abort scanning.
		scan_data->scan_in_progress.clear();
	}

	scan_data->thread->wait_to_finish();
	memdelete(scan_data->thread);
	if (scan_progress) {
		scan_progress->hide();
	}

	for (const String &E : scan_data->found_projects) {
		add_project(E, false);
	}
	memdelete(scan_data);
	scan_data = nullptr;

	save_config();

	if (ProjectManager::get_singleton()->is_initialized()) {
		update_project_list();
	}
}

// Initialization & loading.

void ProjectList::_migrate_config() {
	// Proposal #1637 moved the project list from editor settings to a separate config file
	// If the new config file doesn't exist, populate it from EditorSettings
	if (FileAccess::exists(_config_path)) {
		return;
	}

	List<PropertyInfo> properties;
	EditorSettings::get_singleton()->get_property_list(&properties);

	for (const PropertyInfo &E : properties) {
		// This is actually something like "projects/C:::Documents::Godot::Projects::MyGame"
		String property_key = E.name;
		if (!property_key.begins_with("projects/")) {
			continue;
		}

		String path = EDITOR_GET(property_key);
		print_line("Migrating legacy project '" + path + "'.");

		String favoriteKey = "favorite_projects/" + property_key.get_slicec('/', 1);
		bool favorite = EditorSettings::get_singleton()->has_setting(favoriteKey);
		add_project(path, favorite);
		if (favorite) {
			EditorSettings::get_singleton()->erase(favoriteKey);
		}
		EditorSettings::get_singleton()->erase(property_key);
	}

	save_config();
}

void ProjectList::save_config() {
	_config.save(_config_path);
}

// Load project data from p_property_key and return it in a ProjectList::Item.
// p_favorite is passed directly into the Item.
ProjectList::Item ProjectList::load_project_data(const String &p_path, bool p_favorite) {
	String conf = p_path.path_join("project.godot");
	bool grayed = false;
	bool missing = false;
	bool recovery_mode = false;

	Ref<ConfigFile> cf = memnew(ConfigFile);
	Error cf_err = cf->load(conf);

	int config_version = 0;
	String cf_project_name;
	String project_name = TTR("Unnamed Project");
	if (cf_err == OK) {
		cf_project_name = cf->get_value("application", "config/name", "");
		if (!cf_project_name.is_empty()) {
			project_name = cf_project_name.xml_unescape();
		}
		config_version = (int)cf->get_value("", "config_version", 0);
	}

	if (config_version > ProjectSettings::CONFIG_VERSION) {
		// Comes from an incompatible (more recent) Godot version, gray it out.
		grayed = true;
	}

	const String description = cf->get_value("application", "config/description", "");
	const PackedStringArray tags = cf->get_value("application", "config/tags", PackedStringArray());
	const String main_scene = cf->get_value("application", "run/main_scene", "");

	String icon = cf->get_value("application", "config/icon", "");
	if (icon.begins_with("uid://")) {
		Error err;
		Ref<FileAccess> file = FileAccess::open(p_path.path_join(".godot/uid_cache.bin"), FileAccess::READ, &err);
		if (err == OK) {
			icon = ResourceUID::get_path_from_cache(file, icon);
			if (icon.is_empty()) {
				WARN_PRINT(vformat("Could not load icon from UID for project at path \"%s\". Make sure UID cache exists.", p_path));
			}
		} else {
			// Cache does not exist yet, so ignore and fallback to default icon.
			icon = "";
		}
	}

	PackedStringArray project_features = cf->get_value("application", "config/features", PackedStringArray());
	PackedStringArray unsupported_features = ProjectSettings::get_unsupported_features(project_features);

	String project_version = "?";
	for (int i = 0; i < project_features.size(); i++) {
		if (ProjectList::project_feature_looks_like_version(project_features[i])) {
			project_version = project_features[i];
			break;
		}
	}

	if (config_version < ProjectSettings::CONFIG_VERSION) {
		// Previous versions may not have unsupported features.
		if (config_version == 4) {
			unsupported_features.push_back("3.x");
			project_version = "3.x";
		} else {
			unsupported_features.push_back("u-ver");
		}
	}

	uint64_t last_edited = 0;
	if (cf_err == OK) {
		int *cached_time = modified_time_cache.getptr(p_path);
		if (cached_time) {
			// Modified time may change as a result of Project Manager actions, which will affect sorting.
			// For that reason, the time is read only once.
			last_edited = *cached_time;
		} else {
			// The modification date marks the date the project was last edited.
			// This is because the `project.godot` file will always be modified
			// when editing a project (but not when running it).
			last_edited = FileAccess::get_modified_time(conf);
			modified_time_cache[p_path] = last_edited;
		}
	} else {
		grayed = true;
		missing = true;
	}

	for (const String &tag : tags) {
		ProjectManager::get_singleton()->add_new_tag(tag);
	}

	// We can't use OS::get_user_dir() because it attempts to load paths from the current loaded project through ProjectSettings,
	// while here we're parsing project files externally. Therefore, we have to replicate its behavior.
	String user_dir;
	if (!cf_project_name.is_empty()) {
		String appname = OS::get_singleton()->get_safe_dir_name(cf_project_name);
		bool use_custom_dir = cf->get_value("application", "config/use_custom_user_dir", false);
		if (use_custom_dir) {
			String custom_dir = OS::get_singleton()->get_safe_dir_name(cf->get_value("application", "config/custom_user_dir_name", ""), true);
			if (custom_dir.is_empty()) {
				custom_dir = appname;
			}
			user_dir = custom_dir;
		} else {
			user_dir = OS::get_singleton()->get_godot_dir_name().path_join("app_userdata").path_join(appname);
		}
	} else {
		user_dir = OS::get_singleton()->get_godot_dir_name().path_join("app_userdata").path_join("[unnamed project]");
	}

	String recovery_mode_lock_file = OS::get_singleton()->get_user_data_dir(user_dir).path_join(".recovery_mode_lock");
	recovery_mode = FileAccess::exists(recovery_mode_lock_file);

	return Item(project_name, description, project_version, tags, p_path, icon, main_scene, unsupported_features, last_edited, p_favorite, grayed, missing, recovery_mode, config_version);
}

void ProjectList::_update_icons_async() {
	_icon_load_index = 0;
	set_process(true);
}

void ProjectList::_load_project_icon(int p_index) {
	Item &item = _projects.write[p_index];

	Ref<Texture2D> default_icon = get_editor_theme_icon(SNAME("DefaultProjectIcon"));
	Ref<Texture2D> icon;
	if (!item.icon.is_empty()) {
		Ref<Image> img;
		img.instantiate();
		const String icon_file = item.icon.replace_first("res://", item.path + "/");
		// ClockGears Engine: proyectos que aún usan el icono antiguo por defecto (SVG o PNG antiguo o robot Godot)
		// muestran el nuevo icono por defecto sin modificar el archivo original del proyecto.
		const String ext = icon_file.get_extension().to_lower();
		const String sha = FileAccess::exists(icon_file) ? FileAccess::get_sha256(icon_file).to_lower() : String();
		const bool is_old_default_svg = (ext == "svg") && (
				sha == "9fea9f6c539dbb932419a16655e15c2bc628a18dbefb0db96e03aa33f586683d" ||
				FileAccess::get_file_as_string(icon_file).contains("M105 673v33q407 354 814 0v-33z"));
		const bool is_old_default_png = (ext == "png") && (sha == "3bbc01c14134041a4184acd8244b432a008ad5966d10af74e47656267c798358");
		Error err = (is_old_default_svg || is_old_default_png) ? ERR_UNAVAILABLE : img->load(icon_file);
		if (err == OK) {
			const bool is_int_pixel = (img->get_width() > 0 && img->get_width() == img->get_height() && (default_icon->get_width() % img->get_width()) == 0);
			img->resize(default_icon->get_width(), default_icon->get_height(), is_int_pixel ? Image::INTERPOLATE_NEAREST : Image::INTERPOLATE_LANCZOS);
			icon = ImageTexture::create_from_image(img);
		}
	}
	if (icon.is_null()) {
		icon = default_icon;
	}

	item.loaded_icon = icon;
	item.icon_loaded = true;
	if (item.control) {
		item.control->set_project_icon(icon);
	}
	if (_selected_project_paths.has(item.path)) {
		emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
	}
}

// Project list updates.

void ProjectList::update_project_list() {
	if (ProjectManager::get_singleton() && ProjectManager::get_singleton()->is_initialized()) {
		for (int i = 0; i < _projects.size(); ++i) {
			_projects.write[i].control = nullptr;
		}

		_projects.clear();
		_filtered_indices.clear();
		_last_clicked = "";
		_selected_project_paths.clear();

		load_project_list();
	}

	sort_projects();
	_update_icons_async();
	update_dock_menu();

	set_v_scroll(0);
	_layout_visible_cards();
	emit_signal(SNAME(SIGNAL_LIST_CHANGED));
	queue_accessibility_update();
}

void ProjectList::sort_projects() {
	for (int i = 0; i < _projects.size(); ++i) {
		_projects.write[i].control = nullptr;
	}

	SortArray<Item, ProjectListComparator> sorter;
	sorter.compare.order_option = _order_option;
	sorter.sort(_projects.ptrw(), _projects.size());

	String search_term;
	PackedStringArray tags;

	if (!_search_term.is_empty()) {
		PackedStringArray search_parts = _search_term.split(" ");
		if (search_parts.size() > 1 || search_parts[0].begins_with("tag:")) {
			PackedStringArray remaining;
			for (const String &part : search_parts) {
				if (part.begins_with("tag:")) {
					tags.push_back(part.get_slicec(':', 1));
				} else {
					remaining.append(part);
				}
			}
			search_term = String(" ").join(remaining);
		} else {
			search_term = _search_term;
		}
	}

	_filtered_indices.clear();
	for (int i = 0; i < _projects.size(); ++i) {
		const Item &item = _projects[i];

		bool item_visible = true;
		if (!_search_term.is_empty()) {
			String search_path = search_term.contains_char('/') ? item.path : item.path.get_file();

			bool missing_tags = false;
			for (const String &tag : tags) {
				if (!item.tags.has(tag)) {
					missing_tags = true;
					break;
				}
			}

			item_visible = !missing_tags && (search_term.is_empty() || item.project_name.containsn(search_term) || search_path.containsn(search_term));
		}

		if (item_visible) {
			_filtered_indices.push_back(i);
		}
	}

	_layout_visible_cards();
	_update_icons_async();
	update_dock_menu();
	emit_signal(SNAME(SIGNAL_LIST_CHANGED));
	queue_accessibility_update();
}

int ProjectList::get_project_count() const {
	return _projects.size();
}

void ProjectList::find_projects(const String &p_path) {
	PackedStringArray paths = { p_path };
	find_projects_multiple(paths);
}

void ProjectList::find_projects_multiple(const PackedStringArray &p_paths) {
	if (!scan_progress && is_inside_tree()) {
		scan_progress = memnew(AcceptDialog);
		scan_progress->set_title(TTRC("Scanning"));
		scan_progress->set_ok_button_text(TTRC("Cancel"));

		VBoxContainer *vb = memnew(VBoxContainer);
		scan_progress->add_child(vb);

		Label *label = memnew(Label);
		label->set_text(TTRC("Scanning for projects..."));
		vb->add_child(label);

		ProgressBar *progress = memnew(ProgressBar);
		progress->set_indeterminate(true);
		vb->add_child(progress);

		add_child(scan_progress);
		scan_progress->connect(SceneStringName(confirmed), callable_mp(this, &ProjectList::_scan_finished));
		scan_progress->connect("canceled", callable_mp(this, &ProjectList::_scan_finished));
	}

	scan_data = memnew(ScanData);
	scan_data->paths_to_scan = p_paths;
	scan_data->scan_in_progress.set();

	scan_data->thread = memnew(Thread);
	scan_data->thread->start(_scan_thread, scan_data);

	if (scan_progress) {
		scan_progress->reset_size();
		scan_progress->popup_centered();
	}
	set_process(true);
}

void ProjectList::load_project_list() {
	_config.load(_config_path);
	Vector<String> sections = _config.get_sections();

	for (const String &path : sections) {
		bool favorite = _config.get_value(path, "favorite", false);
		_projects.push_back(load_project_data(path, favorite));
	}
}

void ProjectList::_scan_folder_recursive(const String &p_path, List<String> *r_projects, const SafeFlag &p_scan_active) {
	if (!p_scan_active.is_set()) {
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error error = da->change_dir(p_path);
	ERR_FAIL_COND_MSG(error != OK, vformat("Failed to open the path \"%s\" for scanning (code %d).", p_path, error));

	da->list_dir_begin();
	String n = da->get_next();
	while (!n.is_empty()) {
		if (!p_scan_active.is_set()) {
			return;
		}

		if (da->current_is_dir() && n[0] != '.') {
			_scan_folder_recursive(da->get_current_dir().path_join(n), r_projects, p_scan_active);
		} else if (n == "project.godot") {
			r_projects->push_back(da->get_current_dir());
		}
		n = da->get_next();
	}
	da->list_dir_end();
}

// Project list items.

void ProjectList::add_project(const String &dir_path, bool favorite) {
	if (!_config.has_section(dir_path)) {
		_config.set_value(dir_path, "favorite", favorite);
	}
	queue_accessibility_update();
}

void ProjectList::set_project_version(const String &p_project_path, int p_version) {
	for (ProjectList::Item &E : _projects) {
		if (E.path == p_project_path) {
			E.version = p_version;
			break;
		}
	}
}

int ProjectList::refresh_project(const String &dir_path) {
	// Reloads information about a specific project.
	// If it wasn't loaded and should be in the list, it is added (i.e new project).
	// If it isn't in the list anymore, it is removed.
	// If it is in the list but doesn't exist anymore, it is marked as missing.

	bool should_be_in_list = _config.has_section(dir_path);
	bool is_favorite = _config.get_value(dir_path, "favorite", false);

	bool was_selected = _selected_project_paths.has(dir_path);

	int temp_title_index = -1;

	// Remove item in any case
	for (int i = 0; i < _projects.size(); ++i) {
		const Item &existing_item = _projects[i];
		if (existing_item.path == dir_path) {
			temp_title_index = existing_item.project_title_index;

			_remove_project(i, false);
			break;
		}
	}

	int index = -1;
	if (should_be_in_list) {
		Item item = load_project_data(dir_path, is_favorite);
		item.project_title_index = temp_title_index;
		_projects.push_back(item);

		sort_projects();

		for (int i = 0; i < _projects.size(); ++i) {
			if (_projects[i].path == dir_path) {
				if (was_selected) {
					_selected_project_paths.insert(dir_path);
					ensure_project_visible(i);
				}
				_load_project_icon(i);
				index = i;
				break;
			}
		}
	} else {
		sort_projects();
	}

	return index;
}

int ProjectList::get_index(const ProjectListItemControl *p_control) const {
	if (!p_control) {
		return -1;
	}
	return p_control->get_bound_project_index();
}

int ProjectList::get_visible_bound_count() const {
	int c = 0;
	for (ProjectListItemControl *ctrl : _card_pool) {
		if (ctrl && ctrl->is_visible() && ctrl->get_bound_project_index() >= 0) {
			c++;
		}
	}
	return c;
}

ProjectListItemControl *ProjectList::get_bound_control_for_project(int p_project_index) const {
	if (p_project_index < 0 || p_project_index >= _projects.size()) {
		return nullptr;
	}
	return _projects[p_project_index].control;
}

void ProjectList::ensure_project_visible(int p_index) {
	if (p_index < 0 || p_index >= _projects.size()) {
		return;
	}
	int filtered_pos = -1;
	for (int i = 0; i < _filtered_indices.size(); i++) {
		if (_filtered_indices[i] == p_index) {
			filtered_pos = i;
			break;
		}
	}
	if (filtered_pos >= 0) {
		const int cols = MAX(1, _layout_cols);
		const int row = filtered_pos / cols;
		const float row_top = (float)row * (_layout_card_h + _layout_gap);
		const float row_bot = row_top + _layout_card_h;
		const float view_h = get_size().y > 10.0f ? get_size().y : 420.0f * EDSCALE;
		const float cur_scroll = (float)get_v_scroll();
		if (row_top < cur_scroll) {
			set_v_scroll((int)Math::floor(row_top));
		} else if (row_bot > cur_scroll + view_h) {
			set_v_scroll((int)Math::ceil(row_bot - view_h));
		}
	}
	_layout_visible_cards();
	if (_projects[p_index].control) {
		_projects[p_index].control->grab_focus(true);
	}
}

void ProjectList::_ensure_pool_size(int p_needed) {
	while (_card_pool.size() < p_needed) {
		ProjectListItemControl *hb = memnew(ProjectListItemControl);
		hb->hide();
		hb->connect(SceneStringName(gui_input), callable_mp(this, &ProjectList::_list_item_input).bind(hb));
		hb->connect("favorite_pressed", callable_mp(this, &ProjectList::_on_favorite_pressed).bind(hb));
		hb->connect("explore_pressed", callable_mp(this, &ProjectList::_on_explore_item_pressed).bind(hb));
		hb->connect("request_menu", callable_mp(this, &ProjectList::_open_menu).bind(hb));
		content_canvas->add_child(hb);
		_card_pool.push_back(hb);
	}
}

void ProjectList::_bind_item_to_control(int p_project_index, int p_filtered_index, ProjectListItemControl *p_ctrl) {
	Item &item = _projects.write[p_project_index];
	item.control = p_ctrl;
	p_ctrl->set_bound_indices(p_project_index, p_filtered_index);
	p_ctrl->apply_view_mode((int)_view_mode);

	_update_project_control_translatable_fields(item);
	p_ctrl->set_project_path(item.path);
	p_ctrl->set_tooltip_text(item.description.is_empty() ? item.path : item.description + "\n" + item.path);
	p_ctrl->set_tags(item.tags, this);
	p_ctrl->set_project_version(item.project_version);
	p_ctrl->set_is_favorite(item.favorite);
	p_ctrl->set_is_missing(item.missing);
	p_ctrl->set_is_grayed(item.grayed);
	p_ctrl->set_selected(_selected_project_paths.has(item.path), true);

	if (item.icon_loaded && item.loaded_icon.is_valid()) {
		p_ctrl->set_project_icon(item.loaded_icon);
	} else {
		p_ctrl->set_project_icon(Ref<Texture2D>());
	}
}

void ProjectList::_layout_visible_cards() {
	if (!content_canvas) {
		return;
	}
	for (int i = 0; i < _projects.size(); i++) {
		_projects.write[i].control = nullptr;
	}

	const float sb_w = (get_v_scroll_bar() && get_v_scroll_bar()->is_visible()) ? get_v_scroll_bar()->get_combined_minimum_size().x : 0.0f;
	const float avail_w = MAX(220.0f * EDSCALE, get_size().x - sb_w - 6.0f * EDSCALE);
	const float view_h = get_size().y > 10.0f ? get_size().y : 440.0f * EDSCALE;

	_layout_gap = 8.0f * EDSCALE;
	if (_view_mode == VIEW_GRID) {
		_layout_cols = MAX(2, (int)Math::floor((avail_w + _layout_gap) / (230.0f * EDSCALE)));
		_layout_card_w = MAX(180.0f * EDSCALE, Math::floor((avail_w - _layout_gap * (_layout_cols - 1)) / _layout_cols));
		_layout_card_h = 224.0f * EDSCALE;
	} else {
		_layout_cols = 1;
		_layout_card_w = avail_w;
		_layout_card_h = 76.0f * EDSCALE;
	}

	const int total_items = _filtered_indices.size();
	const int rows = total_items > 0 ? ((total_items + _layout_cols - 1) / _layout_cols) : 0;
	const float total_h = rows > 0 ? ((float)rows * _layout_card_h + (float)(rows - 1) * _layout_gap) : 0.0f;
	content_canvas->set_custom_minimum_size(Size2(0, total_h));
	content_canvas->set_size(Size2(avail_w, total_h));

	if (total_items == 0) {
		for (ProjectListItemControl *ctrl : _card_pool) {
			ctrl->set_bound_indices(-1, -1);
			ctrl->hide();
		}
		return;
	}

	const float scroll_y = CLAMP((float)get_v_scroll(), 0.0f, MAX(0.0f, total_h - view_h));
	const float stride = _layout_card_h + _layout_gap;
	const int first_row = MAX(0, (int)Math::floor(scroll_y / stride) - 1);
	const int last_row = MIN(rows - 1, (int)Math::ceil((scroll_y + view_h) / stride) + 1);
	const int first_vis = first_row * _layout_cols;
	const int last_vis = MIN(total_items - 1, (last_row + 1) * _layout_cols - 1);
	const int needed = MAX(0, last_vis - first_vis + 1);

	_ensure_pool_size(needed);

	for (int p = 0; p < _card_pool.size(); p++) {
		ProjectListItemControl *ctrl = _card_pool[p];
		if (p >= needed) {
			ctrl->set_bound_indices(-1, -1);
			ctrl->hide();
			continue;
		}
		const int fi = first_vis + p;
		const int pidx = _filtered_indices[fi];
		const int row = fi / _layout_cols;
		const int col = fi % _layout_cols;
		const Vector2 pos((float)col * (_layout_card_w + _layout_gap), (float)row * stride);
		const Size2 sz(_layout_card_w, _layout_card_h);

		_bind_item_to_control(pidx, fi, ctrl);
		ctrl->set_custom_minimum_size(Size2());
		ctrl->set_position(pos);
		ctrl->set_size(sz);
		ctrl->show();
		ctrl->resize_project_title(0);
	}
}

void ProjectList::_on_vscroll_changed(double p_val) {
	_layout_visible_cards();
}

void ProjectList::_update_project_control_translatable_fields(const Item &item) {
	ProjectListItemControl *control = item.control;
	if (!control) {
		return;
	}

	control->set_project_title(!item.missing ? item.project_name : TTR("Missing Project"));
	control->set_last_edited_info(item.get_last_edited_string());
	control->set_unsupported_features(item.unsupported_features.duplicate());
}

void ProjectList::_toggle_project(int p_index) {
	Item &item = _projects.write[p_index];

	if (_selected_project_paths.has(item.path)) {
		_deselect_project_nocheck(p_index);
	} else {
		_select_project_nocheck(p_index);
	}
}

void ProjectList::_remove_project(int p_index, bool p_update_config) {
	Item &item_ref = _projects.write[p_index];
	item_ref.control = nullptr;
	const String item_path = item_ref.path;

	_selected_project_paths.erase(item_path);

	if (_last_clicked == item_path) {
		_last_clicked = "";
	}

	_projects.remove_at(p_index);

	if (p_update_config) {
		_config.erase_section(item_path);
	}

	queue_accessibility_update();
	update_dock_menu();
}

void ProjectList::_list_item_input(const Ref<InputEvent> &p_ev, Control *p_hb) {
	ProjectListItemControl *item_ctrl = Object::cast_to<ProjectListItemControl>(p_hb);
	if (!item_ctrl) {
		return;
	}
	int clicked_index = item_ctrl->get_bound_project_index();
	int clicked_filtered = item_ctrl->get_bound_filtered_index();
	if (clicked_index < 0 || clicked_index >= _projects.size()) {
		return;
	}
	const Item &clicked_project = _projects[clicked_index];

	Ref<InputEventMouseButton> mb = p_ev;
	if (mb.is_valid() && mb->is_pressed()) {
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_shift_pressed() && _selected_project_paths.size() > 0 && !_last_clicked.is_empty() && clicked_project.path != _last_clicked) {
				int anchor_filtered = -1;
				for (int fi = 0; fi < _filtered_indices.size(); fi++) {
					if (_projects[_filtered_indices[fi]].path == _last_clicked) {
						anchor_filtered = fi;
						break;
					}
				}
				if (anchor_filtered >= 0 && clicked_filtered >= 0) {
					_select_project_range(anchor_filtered, clicked_filtered);
				} else {
					select_project(clicked_index, true);
				}

			} else if (mb->is_command_or_control_pressed()) {
				_toggle_project(clicked_index);

			} else {
				_last_clicked = clicked_project.path;
				select_project(clicked_index, true);
			}

			emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));

			if (!mb->is_command_or_control_pressed() && mb->is_double_click() && !project_opening_initiated) {
				emit_signal(SNAME(SIGNAL_PROJECT_ASK_OPEN));
			}
		} else if (mb->get_button_index() == MouseButton::RIGHT) {
			_open_menu(mb->get_position(), p_hb);
		}
	}

	Ref<InputEventKey> kev = p_ev;

	if (kev.is_valid() && kev->is_pressed()) {
		switch (kev->get_keycode()) {
			case Key::UP:
			case Key::DOWN:
			case Key::LEFT:
			case Key::RIGHT: {
				if (clicked_filtered >= 0 && !_filtered_indices.is_empty()) {
					int step = 0;
					if (kev->get_keycode() == Key::UP) {
						step = -MAX(1, _layout_cols);
					} else if (kev->get_keycode() == Key::DOWN) {
						step = MAX(1, _layout_cols);
					} else if (kev->get_keycode() == Key::LEFT && _layout_cols > 1) {
						step = -1;
					} else if (kev->get_keycode() == Key::RIGHT && _layout_cols > 1) {
						step = 1;
					}
					if (step != 0) {
						const int next_fi = CLAMP(clicked_filtered + step, 0, _filtered_indices.size() - 1);
						const int next_pidx = _filtered_indices[next_fi];
						_last_clicked = _projects[next_pidx].path;
						select_project(next_pidx, false);
						ensure_project_visible(next_pidx);
						emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
						accept_event();
					}
				}
			} break;
			case Key::ENTER: {
				if (!project_opening_initiated) {
					emit_signal(SNAME(SIGNAL_PROJECT_ASK_OPEN));
				}
				accept_event();
			} break;
			case Key::E: {
				_on_explore_pressed(clicked_project.path);
				accept_event();
			} break;
			case Key::F: {
				if (kev->is_command_or_control_pressed()) {
					return;
				}
				_on_favorite_pressed(p_hb);
				accept_event();
			} break;
			default: {
			} break;
		}
	}
}

void ProjectList::_on_favorite_pressed(Node *p_hb) {
	ProjectListItemControl *control = Object::cast_to<ProjectListItemControl>(p_hb);
	if (!control) {
		return;
	}
	const int index = control->get_bound_project_index();
	if (index < 0 || index >= _projects.size()) {
		return;
	}
	const String target_path = _projects[index].path;
	Item item = _projects.write[index];
	item.favorite = !item.favorite;

	_config.set_value(item.path, "favorite", item.favorite);
	save_config();

	_projects.write[index] = item;
	control->set_is_favorite(item.favorite);

	sort_projects();

	for (int i = 0; i < _projects.size(); i++) {
		if (_projects[i].path == target_path) {
			callable_mp(this, &ProjectList::ensure_project_visible).call_deferred(i);
			break;
		}
	}
	update_dock_menu();
}

void ProjectList::_on_explore_pressed(const String &p_path) {
	OS::get_singleton()->shell_show_in_file_manager(p_path, true);
}

void ProjectList::_on_explore_item_pressed(Node *p_hb) {
	ProjectListItemControl *control = Object::cast_to<ProjectListItemControl>(p_hb);
	if (!control) {
		return;
	}
	const int index = control->get_bound_project_index();
	if (index >= 0 && index < _projects.size()) {
		_on_explore_pressed(_projects[index].path);
	}
}

void ProjectList::_open_menu(const Vector2 &p_at, Control *p_hb) {
	ProjectListItemControl *control = Object::cast_to<ProjectListItemControl>(p_hb);
	if (!control) {
		return;
	}
	int clicked_index = control->get_bound_project_index();
	if (clicked_index < 0 || clicked_index >= _projects.size()) {
		return;
	}
	const Item &clicked_project = _projects[clicked_index];

	if (!project_context_menu) {
		project_context_menu = memnew(PopupMenu);
		project_context_menu->add_item(TTRC("Open in Editor"), MENU_EDIT);
		project_context_menu->add_item(TTRC("Open in Editor (Verbose Mode)"), MENU_EDIT_VERBOSE);
		project_context_menu->add_item(TTRC("Open in Editor (Recovery Mode)"), MENU_EDIT_RECOVERY);
		project_context_menu->add_item(TTRC("Run Project"), MENU_RUN);
		project_context_menu->add_separator();
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		project_context_menu->add_item(OS::get_singleton()->get_platform_string(OS::PLATFORM_STRING_FILE_MANAGER_OPEN), MENU_SHOW_IN_FILE_MANAGER);
#endif // !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
		project_context_menu->add_item(TTRC("Copy Path"), MENU_COPY_PATH);
		project_context_menu->add_separator();
		project_context_menu->add_item(TTRC("Rename"), MENU_RENAME);
		project_context_menu->add_item(TTRC("Manage Tags"), MENU_MANAGE_TAGS);
		project_context_menu->add_item(TTRC("Duplicate"), MENU_DUPLICATE);
		project_context_menu->add_item(TTRC("Remove from Project List"), MENU_REMOVE);
		add_child(project_context_menu);
		project_context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &ProjectList::_menu_option));
		_update_menu_icons();
	}
	select_project(clicked_index, true);
	emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
	control->grab_focus(true);

	for (int id : Vector<int>{
				 MENU_EDIT,
				 MENU_EDIT_VERBOSE,
				 MENU_EDIT_RECOVERY,
				 MENU_RUN,
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
				 MENU_SHOW_IN_FILE_MANAGER,
#endif
				 MENU_RENAME,
				 MENU_MANAGE_TAGS,
				 MENU_DUPLICATE }) {
		project_context_menu->set_item_disabled(project_context_menu->get_item_index(id), clicked_project.missing);
	}

	project_context_menu->set_position(p_hb->get_screen_position() + p_at);
	project_context_menu->reset_size();
	project_context_menu->popup();
}

void ProjectList::_menu_option(int p_option) {
	emit_signal(SIGNAL_MENU_OPTION_SELECTED, p_option);
}

void ProjectList::_update_menu_icons() {
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT), get_editor_theme_icon("Edit"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_VERBOSE), get_editor_theme_icon("Notification"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_EDIT_RECOVERY), get_editor_theme_icon("NodeWarning"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_RUN), get_editor_theme_icon("Play"));
#if !defined(ANDROID_ENABLED) && !defined(WEB_ENABLED)
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_SHOW_IN_FILE_MANAGER), get_editor_theme_icon("ClockGearsFolderLine"));
#endif
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_COPY_PATH), get_editor_theme_icon("ActionCopy"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_RENAME), get_editor_theme_icon("Rename"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_MANAGE_TAGS), get_editor_theme_icon("ClockGearsTag"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_DUPLICATE), get_editor_theme_icon("Duplicate"));
	project_context_menu->set_item_icon(project_context_menu->get_item_index(MENU_REMOVE), get_editor_theme_icon("Remove"));
}

// Project list selection.

void ProjectList::_clear_project_selection() {
	_selected_project_paths.clear();

	for (ProjectListItemControl *ctrl : _card_pool) {
		if (ctrl) {
			ctrl->set_selected(false);
		}
	}
	queue_accessibility_update();
}

void ProjectList::_select_project_nocheck(int p_index, bool p_hide_focus) {
	if (p_index < 0 || p_index >= _projects.size()) {
		return;
	}
	Item &item = _projects.write[p_index];
	_selected_project_paths.insert(item.path);
	if (item.control) {
		item.control->set_selected(true, p_hide_focus);
	}
	queue_accessibility_update();
}

void ProjectList::_deselect_project_nocheck(int p_index) {
	if (p_index < 0 || p_index >= _projects.size()) {
		return;
	}
	Item &item = _projects.write[p_index];
	_selected_project_paths.erase(item.path);
	if (item.control) {
		item.control->set_selected(false);
	}
	queue_accessibility_update();
}

inline void _sort_project_range(int &a, int &b) {
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
}

void ProjectList::_select_project_range(int p_begin_filtered, int p_end_filtered) {
	_clear_project_selection();

	_sort_project_range(p_begin_filtered, p_end_filtered);
	p_begin_filtered = MAX(0, p_begin_filtered);
	p_end_filtered = MIN(_filtered_indices.size() - 1, p_end_filtered);
	for (int fi = p_begin_filtered; fi <= p_end_filtered; ++fi) {
		_select_project_nocheck(_filtered_indices[fi]);
	}
}

void ProjectList::select_project(int p_index, bool p_hide_focus) {
	_clear_project_selection();
	_select_project_nocheck(p_index, p_hide_focus);
	emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
}

void ProjectList::deselect_project(int p_index) {
	_deselect_project_nocheck(p_index);
	emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
}

void ProjectList::select_first_visible_project() {
	_clear_project_selection();

	if (!_filtered_indices.is_empty()) {
		_select_project_nocheck(_filtered_indices[0]);
	}
	emit_signal(SNAME(SIGNAL_SELECTION_CHANGED));
}

void ProjectList::deselect_all_visible_projects() {
	for (int pidx : _filtered_indices) {
		_deselect_project_nocheck(pidx);
	}
}

void ProjectList::select_all_visible_projects() {
	for (int pidx : _filtered_indices) {
		_select_project_nocheck(pidx);
	}
}

Vector<ProjectList::Item> ProjectList::get_selected_projects() const {
	Vector<Item> items;
	if (_selected_project_paths.is_empty()) {
		return items;
	}
	items.resize(_selected_project_paths.size());
	int j = 0;
	for (int i = 0; i < _projects.size(); ++i) {
		const Item &item = _projects[i];
		if (_selected_project_paths.has(item.path)) {
			items.write[j++] = item;
		}
	}
	ERR_FAIL_COND_V(j != items.size(), items);
	return items;
}

const HashSet<String> &ProjectList::get_selected_project_keys() const {
	// Faster if that's all you need
	return _selected_project_paths;
}

int ProjectList::get_single_selected_index() const {
	if (_selected_project_paths.is_empty()) {
		// Default selection
		return 0;
	}
	String key;
	if (_selected_project_paths.size() == 1) {
		// Only one selected
		key = *_selected_project_paths.begin();
	} else {
		// Multiple selected, consider the last clicked one as "main"
		key = _last_clicked;
	}
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].path == key) {
			return i;
		}
	}
	return 0;
}

void ProjectList::erase_selected_projects(bool p_delete_project_contents) {
	if (_selected_project_paths.is_empty()) {
		return;
	}

	HashSet<int> visible_set;
	for (int pidx : _filtered_indices) {
		visible_set.insert(pidx);
	}

	for (int i = _projects.size() - 1; i >= 0; --i) {
		Item &item = _projects.write[i];
		if (_selected_project_paths.has(item.path) && visible_set.has(i)) {
			_config.erase_section(item.path);
			item.control = nullptr;
			_projects.remove_at(i);
		}
	}

	save_config();
	_selected_project_paths.clear();
	_last_clicked = "";

	sort_projects();
	update_dock_menu();
}

// Resize project titles.

void ProjectList::resize_project_titles() {
	for (ProjectListItemControl *ctrl : _card_pool) {
		if (ctrl && ctrl->is_visible()) {
			ctrl->resize_project_title(0);
		}
	}
	_layout_visible_cards();
}

// Missing projects.

bool ProjectList::is_any_project_missing() const {
	for (int i = 0; i < _projects.size(); ++i) {
		if (_projects[i].missing) {
			return true;
		}
	}
	return false;
}

void ProjectList::erase_missing_projects() {
	if (_projects.is_empty()) {
		return;
	}

	int deleted_count = 0;
	int remaining_count = 0;

	for (int i = 0; i < _projects.size(); ++i) {
		const Item &item = _projects[i];

		if (item.missing) {
			_remove_project(i, true);
			--i;
			++deleted_count;

		} else {
			++remaining_count;
		}
	}

	print_line("Removed " + itos(deleted_count) + " projects from the list, remaining " + itos(remaining_count) + " projects");
	save_config();
	sort_projects();
}

// Project list sorting and filtering.

void ProjectList::set_view_mode(int p_mode, bool p_save) {
	const ViewMode target = (p_mode == VIEW_GRID) ? VIEW_GRID : VIEW_LIST;
	if (p_save && EditorSettings::get_singleton()) {
		EditorSettings::get_singleton()->set("project_manager/view_mode", (int)target);
		EditorSettings::get_singleton()->save();
	}
	if (_view_mode == target) {
		return;
	}
	_view_mode = target;
	set_v_scroll(0);
	_layout_visible_cards();
}

void ProjectList::set_search_term(String p_search_term) {
	_search_term = p_search_term;
}

void ProjectList::add_search_tag(const String &p_tag) {
	const String tag_string = "tag:" + p_tag;

	int exists = _search_term.find(tag_string);
	if (exists > -1) {
		_search_term = _search_term.erase(exists, tag_string.length() + 1);
	} else if (_search_term.is_empty() || _search_term.ends_with(" ")) {
		_search_term += tag_string;
	} else {
		_search_term += " " + tag_string;
	}
	ProjectManager::get_singleton()->get_search_box()->set_text(_search_term);

	sort_projects();
}

void ProjectList::set_order_option(int p_option, bool p_save) {
	FilterOption selected = (FilterOption)p_option;
	if (p_save) {
		EditorSettings::get_singleton()->set("project_manager/sorting_order", p_option);
		EditorSettings::get_singleton()->save();
	}
	_order_option = selected;

	sort_projects();
}

// Global menu integration.

void ProjectList::update_dock_menu() {
	if (!NativeMenu::get_singleton()->has_feature(NativeMenu::FEATURE_GLOBAL_MENU)) {
		return;
	}
	RID dock_rid = NativeMenu::get_singleton()->get_system_menu(NativeMenu::DOCK_MENU_ID);
	NativeMenu::get_singleton()->clear(dock_rid);

	int favs_added = 0;
	int total_added = 0;
	for (int i = 0; i < _projects.size(); ++i) {
		if (!_projects[i].grayed && !_projects[i].missing) {
			if (_projects[i].favorite) {
				favs_added++;
			} else {
				if (favs_added != 0) {
					NativeMenu::get_singleton()->add_separator(dock_rid);
				}
				favs_added = 0;
			}
			NativeMenu::get_singleton()->add_item(dock_rid, _projects[i].project_name + " ( " + _projects[i].path + " )", callable_mp(this, &ProjectList::_global_menu_open_project), Callable(), i);
			total_added++;
		}
	}
	if (total_added != 0) {
		NativeMenu::get_singleton()->add_separator(dock_rid);
	}
	NativeMenu::get_singleton()->add_item(dock_rid, TTR("New Window"), callable_mp(this, &ProjectList::_global_menu_new_window));
}

void ProjectList::_global_menu_new_window(const Variant &p_tag) {
	List<String> args;
	args.push_back("-p");
	OS::get_singleton()->create_instance(args);
}

void ProjectList::_global_menu_open_project(const Variant &p_tag) {
	int idx = (int)p_tag;

	if (idx >= 0 && idx < _projects.size()) {
		String conf = _projects[idx].path.path_join("project.godot");
		List<String> args;
		args.push_back(conf);
		OS::get_singleton()->create_instance(args);
	}
}

// Object methods.

void ProjectList::_bind_methods() {
	ADD_SIGNAL(MethodInfo(SIGNAL_LIST_CHANGED));
	ADD_SIGNAL(MethodInfo(SIGNAL_SELECTION_CHANGED));
	ADD_SIGNAL(MethodInfo(SIGNAL_PROJECT_ASK_OPEN));
	ADD_SIGNAL(MethodInfo(SIGNAL_MENU_OPTION_SELECTED));

	ClassDB::bind_method(D_METHOD("get_project_count"), &ProjectList::get_project_count);
	ClassDB::bind_method(D_METHOD("get_filtered_project_count"), &ProjectList::get_filtered_project_count);
	ClassDB::bind_method(D_METHOD("get_pool_control_count"), &ProjectList::get_pool_control_count);
	ClassDB::bind_method(D_METHOD("get_visible_bound_count"), &ProjectList::get_visible_bound_count);
	ClassDB::bind_method(D_METHOD("set_view_mode", "mode", "save"), &ProjectList::set_view_mode, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_view_mode"), &ProjectList::get_view_mode);
	ClassDB::bind_method(D_METHOD("set_order_option", "option", "save"), &ProjectList::set_order_option);
	ClassDB::bind_method(D_METHOD("set_search_term", "term"), &ProjectList::set_search_term);
	ClassDB::bind_method(D_METHOD("sort_projects"), &ProjectList::sort_projects);
	ClassDB::bind_method(D_METHOD("select_project", "index", "hide_focus"), &ProjectList::select_project, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("select_first_visible_project"), &ProjectList::select_first_visible_project);
	ClassDB::bind_method(D_METHOD("get_selected_project_paths"), &ProjectList::get_selected_project_paths);
	ClassDB::bind_method(D_METHOD("erase_selected_projects", "delete_contents"), &ProjectList::erase_selected_projects);
	ClassDB::bind_method(D_METHOD("is_any_project_missing"), &ProjectList::is_any_project_missing);
	ClassDB::bind_method(D_METHOD("erase_missing_projects"), &ProjectList::erase_missing_projects);
}

PackedStringArray ProjectList::get_selected_project_paths() const {
	PackedStringArray out;
	for (const String &k : _selected_project_paths) {
		out.push_back(k);
	}
	return out;
}

void ProjectList::set_config_path_for_test(const String &p_cfg_path) {
	_config_path = p_cfg_path;
	_config.clear();
	_config.load(_config_path);
	for (int i = 0; i < _projects.size(); i++) {
		_projects.write[i].control = nullptr;
	}
	_projects.clear();
	_filtered_indices.clear();
	_selected_project_paths.clear();
	_last_clicked = "";
	load_project_list();
	sort_projects();
	_update_icons_async();
	set_v_scroll(0);
	_layout_visible_cards();
	emit_signal(SNAME(SIGNAL_LIST_CHANGED));
}

ProjectList::ProjectList() {
	if (EditorNode::get_singleton()) {
		return;
	}
	set_follow_focus(false);
	if (!EditorPaths::get_singleton()) {
		EditorPaths::create();
	}

	content_canvas = memnew(Control);
	content_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(content_canvas);

	if (get_v_scroll_bar()) {
		get_v_scroll_bar()->connect(SceneStringName(value_changed), callable_mp(this, &ProjectList::_on_vscroll_changed));
	}

	_config_path = EditorPaths::get_singleton()->get_data_dir().path_join("projects.cfg");
	_migrate_config();
}
