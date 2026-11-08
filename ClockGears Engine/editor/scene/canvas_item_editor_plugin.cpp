/**************************************************************************/
/*  canvas_item_editor_plugin.cpp                                         */
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

#include "canvas_item_editor_plugin.h"
#include "scene/resources/world_2d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/create_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/editor_toolbar_group.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/gui/control_editor_plugin.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_preview_button.h"
#include "editor/translations/editor_translation_preview_menu.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/physics/touch_screen_button.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/debugger/canvas_item_manipulator.h"
#include "scene/debugger/debugger_helpers.h"
#include "scene/gui/base_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/gui/texture_button.h"
#include "scene/gui/view_panner.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/property_utils.h"
#include "scene/resources/gradient.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_texture.h"
#include "servers/rendering/rendering_server.h"
#include "modules/tilemap/tile_map_layer.h"

class SnapDialog : public ConfirmationDialog {
	GDCLASS(SnapDialog, ConfirmationDialog);

	friend class CanvasItemEditor;

	SpinBox *grid_offset_x;
	SpinBox *grid_offset_y;
	SpinBox *grid_step_x;
	SpinBox *grid_step_y;
	SpinBox *primary_grid_step_x;
	SpinBox *primary_grid_step_y;
	SpinBox *rotation_offset;
	SpinBox *rotation_step;
	SpinBox *scale_step;

public:
	SnapDialog() {
		const int SPIN_BOX_GRID_RANGE = 16384;
		const int SPIN_BOX_ROTATION_RANGE = 360;
		const real_t SPIN_BOX_SCALE_MIN = 0.01;
		const real_t SPIN_BOX_SCALE_MAX = 100;

		Label *label;
		VBoxContainer *container;
		GridContainer *child_container;

		set_title(TTRC("Configure Snap"));

		container = memnew(VBoxContainer);
		add_child(container);

		child_container = memnew(GridContainer);
		child_container->set_columns(3);
		container->add_child(child_container);

		label = memnew(Label);
		label->set_text(TTRC("Grid Offset:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		grid_offset_x = memnew(SpinBox);
		grid_offset_x->set_min(-SPIN_BOX_GRID_RANGE);
		grid_offset_x->set_max(SPIN_BOX_GRID_RANGE);
		grid_offset_x->set_allow_lesser(true);
		grid_offset_x->set_allow_greater(true);
		grid_offset_x->set_format("%s px");
		grid_offset_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_offset_x->set_select_all_on_focus(true);
		grid_offset_x->set_accessibility_name(TTRC("X Offset"));
		child_container->add_child(grid_offset_x);

		grid_offset_y = memnew(SpinBox);
		grid_offset_y->set_min(-SPIN_BOX_GRID_RANGE);
		grid_offset_y->set_max(SPIN_BOX_GRID_RANGE);
		grid_offset_y->set_allow_lesser(true);
		grid_offset_y->set_allow_greater(true);
		grid_offset_y->set_format("%s px");
		grid_offset_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_offset_y->set_select_all_on_focus(true);
		grid_offset_y->set_accessibility_name(TTRC("Y Offset"));
		child_container->add_child(grid_offset_y);

		label = memnew(Label);
		label->set_text(TTRC("Grid Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		grid_step_x = memnew(SpinBox);
		grid_step_x->set_min(1);
		grid_step_x->set_max(SPIN_BOX_GRID_RANGE);
		grid_step_x->set_allow_greater(true);
		grid_step_x->set_format("%s px");
		grid_step_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_step_x->set_select_all_on_focus(true);
		grid_step_x->set_accessibility_name(TTRC("X Step"));
		child_container->add_child(grid_step_x);

		grid_step_y = memnew(SpinBox);
		grid_step_y->set_min(1);
		grid_step_y->set_max(SPIN_BOX_GRID_RANGE);
		grid_step_y->set_allow_greater(true);
		grid_step_y->set_format("%s px");
		grid_step_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid_step_y->set_select_all_on_focus(true);
		grid_step_y->set_accessibility_name(TTRC("X Step"));
		child_container->add_child(grid_step_y);

		label = memnew(Label);
		label->set_text(TTRC("Primary Line Every:"));
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_container->add_child(label);

		primary_grid_step_x = memnew(SpinBox);
		primary_grid_step_x->set_min(1);
		primary_grid_step_x->set_step(1);
		primary_grid_step_x->set_max(SPIN_BOX_GRID_RANGE);
		primary_grid_step_x->set_allow_greater(true);
		primary_grid_step_x->set_format_with_plural("%d step", "%d steps"); // TTRN("%s step", "%s steps")
		primary_grid_step_x->set_format_auto_translate_mode(AUTO_TRANSLATE_MODE_INHERIT);
		primary_grid_step_x->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		primary_grid_step_x->set_select_all_on_focus(true);
		primary_grid_step_x->set_accessibility_name(TTRC("X Primary Step"));
		child_container->add_child(primary_grid_step_x);

		primary_grid_step_y = memnew(SpinBox);
		primary_grid_step_y->set_min(1);
		primary_grid_step_y->set_step(1);
		primary_grid_step_y->set_max(SPIN_BOX_GRID_RANGE);
		primary_grid_step_y->set_allow_greater(true);
		primary_grid_step_y->set_format_with_plural("%d step", "%d steps"); // TTRN("%s step", "%s steps")
		primary_grid_step_y->set_format_auto_translate_mode(AUTO_TRANSLATE_MODE_INHERIT);
		primary_grid_step_y->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		primary_grid_step_y->set_select_all_on_focus(true);
		primary_grid_step_y->set_accessibility_name(TTRC("Y Primary Step"));
		child_container->add_child(primary_grid_step_y);

		container->add_child(memnew(HSeparator));

		// We need to create another GridContainer with the same column count,
		// so we can put an HSeparator above
		child_container = memnew(GridContainer);
		child_container->set_columns(2);
		container->add_child(child_container);

		label = memnew(Label);
		label->set_text(TTRC("Rotation Offset:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		rotation_offset = memnew(SpinBox);
		rotation_offset->set_min(-SPIN_BOX_ROTATION_RANGE);
		rotation_offset->set_max(SPIN_BOX_ROTATION_RANGE);
		rotation_offset->set_format(U"%s°");
		rotation_offset->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rotation_offset->set_select_all_on_focus(true);
		rotation_offset->set_accessibility_name(TTRC("Rotation Offset:"));
		child_container->add_child(rotation_offset);

		label = memnew(Label);
		label->set_text(TTRC("Rotation Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		rotation_step = memnew(SpinBox);
		rotation_step->set_min(-SPIN_BOX_ROTATION_RANGE);
		rotation_step->set_max(SPIN_BOX_ROTATION_RANGE);
		rotation_step->set_format(U"%s°");
		rotation_step->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		rotation_step->set_select_all_on_focus(true);
		rotation_step->set_accessibility_name(TTRC("Rotation Step:"));
		child_container->add_child(rotation_step);

		container->add_child(memnew(HSeparator));

		child_container = memnew(GridContainer);
		child_container->set_columns(2);
		container->add_child(child_container);
		label = memnew(Label);
		label->set_text(TTRC("Scale Step:"));
		child_container->add_child(label);
		label->set_h_size_flags(Control::SIZE_EXPAND_FILL);

		scale_step = memnew(SpinBox);
		scale_step->set_min(SPIN_BOX_SCALE_MIN);
		scale_step->set_max(SPIN_BOX_SCALE_MAX);
		scale_step->set_allow_greater(true);
		scale_step->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		scale_step->set_step(0.01f);
		scale_step->set_select_all_on_focus(true);
		scale_step->set_accessibility_name(TTRC("Scale Step:"));
		child_container->add_child(scale_step);
	}

	void set_fields(const Point2 p_grid_offset, const Point2 p_grid_step, const Vector2i p_primary_grid_step, const real_t p_rotation_offset, const real_t p_rotation_step, const real_t p_scale_step) {
		grid_offset_x->set_value(p_grid_offset.x);
		grid_offset_y->set_value(p_grid_offset.y);
		grid_step_x->set_value(p_grid_step.x);
		grid_step_y->set_value(p_grid_step.y);
		primary_grid_step_x->set_value(p_primary_grid_step.x);
		primary_grid_step_y->set_value(p_primary_grid_step.y);
		rotation_offset->set_value(Math::rad_to_deg(p_rotation_offset));
		rotation_step->set_value(Math::rad_to_deg(p_rotation_step));
		scale_step->set_value(p_scale_step);
	}

	void get_fields(Point2 &p_grid_offset, Point2 &p_grid_step, Vector2i &p_primary_grid_step, real_t &p_rotation_offset, real_t &p_rotation_step, real_t &p_scale_step) {
		p_grid_offset = Point2(grid_offset_x->get_value(), grid_offset_y->get_value());
		p_grid_step = Point2(grid_step_x->get_value(), grid_step_y->get_value());
		p_primary_grid_step = Vector2i(primary_grid_step_x->get_value(), primary_grid_step_y->get_value());
		p_rotation_offset = Math::deg_to_rad(rotation_offset->get_value());
		p_rotation_step = Math::deg_to_rad(rotation_step->get_value());
		p_scale_step = scale_step->get_value();
	}
};

void CanvasItemEditor::shortcut_input(const Ref<InputEvent> &p_ev) {
	ERR_FAIL_COND(p_ev.is_null());

	Ref<InputEventKey> k = p_ev;

	if (!is_visible_in_tree()) {
		return;
	}

	if (k.is_valid()) {
		if (!k->is_echo() && (k->get_keycode() == Key::CTRL || k->get_keycode() == Key::ALT || k->get_keycode() == Key::SHIFT)) {
			viewport->queue_redraw();
		}

		if (k->is_pressed() && !k->is_command_or_control_pressed() && !k->is_echo() && (ci_manipulator->is_grid_snap_enabled() || is_grid_visible())) {
			if (multiply_grid_step_shortcut.is_valid() && multiply_grid_step_shortcut->matches_event(p_ev)) {
				// Multiply the grid size
				ci_manipulator->set_grid_step_multiplier(MIN(ci_manipulator->get_grid_step_multiplier() + 1, 12));
				viewport->queue_redraw();
			} else if (divide_grid_step_shortcut.is_valid() && divide_grid_step_shortcut->matches_event(p_ev)) {
				// Divide the grid size
				Point2 new_grid_step = ci_manipulator->get_grid_step() * Math::pow(2.0, ci_manipulator->get_grid_step_multiplier() - 1);
				if (new_grid_step.x >= 1.0 && new_grid_step.y >= 1.0) {
					ci_manipulator->set_grid_step_multiplier(ci_manipulator->get_grid_step_multiplier() - 1);
				}
				viewport->queue_redraw();
			}
		}

		if (k->is_pressed() && !k->is_echo()) {
			if (reset_transform_position_shortcut.is_valid() && reset_transform_position_shortcut->matches_event(p_ev)) {
				_reset_transform(TransformType::POSITION);
			}
			if (reset_transform_rotation_shortcut.is_valid() && reset_transform_rotation_shortcut->matches_event(p_ev)) {
				_reset_transform(TransformType::ROTATION);
			}
			if (reset_transform_scale_shortcut.is_valid() && reset_transform_scale_shortcut->matches_event(p_ev)) {
				_reset_transform(TransformType::SCALE);
			}

			const Callable custom_callback = EditorContextMenuPluginManager::get_singleton()->match_custom_shortcut(EditorContextMenuPlugin::CONTEXT_SLOT_2D_EDITOR, p_ev);
			if (custom_callback.is_valid()) {
				EditorContextMenuPlugin::OptionsData context_data = _get_context_data();
#ifndef DISABLE_DEPRECATED
				if (p_ev->get_meta("_legacy_shortcut", false)) {
					EditorContextMenuPluginManager::get_singleton()->invoke_callback(custom_callback, context_data["hovered_nodes"]);
					accept_event();
					return;
				}
#endif
				EditorContextMenuPluginManager::get_singleton()->invoke_callback(custom_callback, context_data);
				accept_event();
			}
		}
	}
}

Object *CanvasItemEditor::_get_editor_data(Object *p_what) {
	CanvasItem *ci = Object::cast_to<CanvasItem>(p_what);
	if (!ci) {
		return nullptr;
	}

	return memnew(CanvasItemEditorSelectedItem);
}

void CanvasItemEditor::_keying_changed() {
	AnimationTrackEditor *te = AnimationPlayerEditor::get_singleton()->get_track_editor();
	animation_hb->set_visible(te && te->has_keying());
}

Rect2 CanvasItemEditor::_get_encompassing_rect_from_list(const List<CanvasItem *> &p_list) {
	ERR_FAIL_COND_V(p_list.is_empty(), Rect2());

	// Handles the first element
	CanvasItem *ci = p_list.front()->get();
	Rect2 rect = Rect2(ci->get_global_transform_with_canvas().xform(ci->_edit_get_rect().get_center()), Size2());

	// Expand with the other ones
	for (CanvasItem *ci2 : p_list) {
		Transform2D xform = ci2->get_global_transform_with_canvas();

		Rect2 current_rect = ci2->_edit_get_rect();
		rect.expand_to(xform.xform(current_rect.position));
		rect.expand_to(xform.xform(current_rect.position + Vector2(current_rect.size.x, 0)));
		rect.expand_to(xform.xform(current_rect.position + current_rect.size));
		rect.expand_to(xform.xform(current_rect.position + Vector2(0, current_rect.size.y)));
	}

	return rect;
}

void CanvasItemEditor::_expand_encompassing_rect_using_children(Rect2 &r_rect, const Node *p_node, bool &r_first, const Transform2D &p_parent_xform, const Transform2D &p_canvas_xform, bool include_locked_nodes) {
	if (!p_node) {
		return;
	}
	if (Object::cast_to<Viewport>(p_node)) {
		return;
	}

	const CanvasItem *ci = Object::cast_to<CanvasItem>(p_node);

	for (int i = p_node->get_child_count() - 1; i >= 0; i--) {
		if (ci && !ci->is_set_as_top_level()) {
			_expand_encompassing_rect_using_children(r_rect, p_node->get_child(i), r_first, p_parent_xform * ci->get_transform(), p_canvas_xform);
		} else {
			const CanvasLayer *cl = Object::cast_to<CanvasLayer>(p_node);
			_expand_encompassing_rect_using_children(r_rect, p_node->get_child(i), r_first, Transform2D(), cl ? cl->get_transform() : p_canvas_xform);
		}
	}

	if (ci && ci->is_visible_in_tree() && (include_locked_nodes || !CanvasItemManipulator::is_node_locked(ci))) {
		Transform2D xform = p_canvas_xform;
		if (!ci->is_set_as_top_level()) {
			xform *= p_parent_xform;
		}
		xform *= ci->get_transform();
		Rect2 rect = ci->_edit_get_rect();
		if (r_first) {
			r_rect = Rect2(xform.xform(rect.get_center()), Size2());
			r_first = false;
		}
		r_rect.expand_to(xform.xform(rect.position));
		r_rect.expand_to(xform.xform(rect.position + Point2(rect.size.x, 0)));
		r_rect.expand_to(xform.xform(rect.position + Point2(0, rect.size.y)));
		r_rect.expand_to(xform.xform(rect.position + rect.size));
	}
}

Rect2 CanvasItemEditor::_get_encompassing_rect(const Node *p_node) {
	Rect2 rect;
	bool first = true;
	_expand_encompassing_rect_using_children(rect, p_node, first);

	return rect;
}

Dictionary CanvasItemEditor::_get_context_data() {
	const Vector2 mouse_pos = transform.affine_inverse().xform(viewport->get_local_mouse_position());

	Vector<DebuggerHelpers::SelectResult> currently_hovered;
	ci_manipulator->get_canvas_items_at_pos(mouse_pos, currently_hovered, true);

	TypedArray<CanvasItem *> items;
	items.reserve(currently_hovered.size());
	for (const DebuggerHelpers::SelectResult &result : currently_hovered) {
		items.push_back(result.item);
	}

	EditorContextMenuPlugin::OptionsData context_data;
	context_data["hovered_nodes"] = items;
	context_data["mouse_position"] = mouse_pos;
	return context_data;
}

bool CanvasItemEditor::_select_click_on_item(const Variant &p_node, bool p_append) {
	Node *node = Object::cast_to<Node>(p_node);
	bool still_selected = true;
	const List<Node *> &top_node_list = editor_selection->get_top_selected_node_list();

	if (p_append && !top_node_list.is_empty()) {
		if (editor_selection->is_selected(node)) {
			// Already in the selection, remove it from the selected nodes
			editor_selection->remove_node(node);
			still_selected = false;

			if (top_node_list.size() == 1) {
				EditorNode::get_singleton()->push_item(top_node_list.front()->get());
			}
		} else {
			// Add the item to the selection
			editor_selection->add_node(node);
		}
	} else {
		if (!editor_selection->is_selected(node)) {
			// Select a new one and clear previous selection
			editor_selection->clear();
			editor_selection->add_node(node);
			// Reselect
			selected_from_canvas = true;
		}
	}

	viewport->queue_redraw();
	return still_selected;
}

List<CanvasItem *> CanvasItemEditor::_get_edited_canvas_items(bool p_retrieve_locked, bool p_remove_canvas_item_if_parent_in_selection, bool *r_has_locked_items) const {
	List<CanvasItem *> selection;
	for (const KeyValue<ObjectID, Object *> &E : editor_selection->get_selection()) {
		CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
		if (ci) {
			if (ci->is_visible_in_tree() && (p_retrieve_locked || !CanvasItemManipulator::is_node_locked(ci))) {
				Viewport *vp = ci->get_viewport();
				if (vp && !vp->is_visible_subviewport()) {
					continue;
				}
				CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);
				if (se) {
					selection.push_back(ci);
				}
			} else if (r_has_locked_items) {
				// CanvasItem is selected, but can't be interacted with.
				*r_has_locked_items = true;
			}
		}
	}

	if (p_remove_canvas_item_if_parent_in_selection) {
		List<CanvasItem *> filtered_selection;
		HashSet<const Node *> nodes_in_selection;
		for (CanvasItem *E : selection) {
			nodes_in_selection.insert(E);
		}
		for (CanvasItem *E : selection) {
			if (!nodes_in_selection.has(E->get_parent())) {
				filtered_selection.push_back(E);
			}
		}
		return filtered_selection;
	} else {
		return selection;
	}
}

Vector2 CanvasItemEditor::_anchor_to_position(const Control *p_control, Vector2 anchor) {
	ERR_FAIL_NULL_V(p_control, Vector2());

	Transform2D parent_transform = p_control->get_transform().affine_inverse();
	Rect2 parent_rect = p_control->get_parent_anchorable_rect();

	if (p_control->is_layout_rtl()) {
		return parent_transform.xform(parent_rect.position + Vector2(parent_rect.size.x - parent_rect.size.x * anchor.x, parent_rect.size.y * anchor.y));
	} else {
		return parent_transform.xform(parent_rect.position + Vector2(parent_rect.size.x * anchor.x, parent_rect.size.y * anchor.y));
	}
}

Vector2 CanvasItemEditor::_position_to_anchor(const Control *p_control, Vector2 position) {
	ERR_FAIL_NULL_V(p_control, Vector2());

	Rect2 parent_rect = p_control->get_parent_anchorable_rect();

	Vector2 output;
	if (p_control->is_layout_rtl()) {
		output.x = (parent_rect.size.x == 0) ? 0.0 : (parent_rect.size.x - p_control->get_transform().xform(position).x - parent_rect.position.x) / parent_rect.size.x;
	} else {
		output.x = (parent_rect.size.x == 0) ? 0.0 : (p_control->get_transform().xform(position).x - parent_rect.position.x) / parent_rect.size.x;
	}
	output.y = (parent_rect.size.y == 0) ? 0.0 : (p_control->get_transform().xform(position).y - parent_rect.position.y) / parent_rect.size.y;
	return output;
}

bool CanvasItemEditor::_get_selection_callback(Array r_selection) {
	bool has_locked_items = false;
	List<CanvasItem *> selection = _get_edited_canvas_items(false, true, &has_locked_items);
	for (const CanvasItem *ci : selection) {
		r_selection.append(ci);
	}
	return has_locked_items;
}

void CanvasItemEditor::_box_selection_updated(const Rect2 &p_rect) {
	box_selection_rect = transform.xform(p_rect);
	viewport->queue_redraw();
}

void CanvasItemEditor::_box_selected(const Array &p_selection) {
	if (p_selection.size() == 1 && editor_selection->get_selection().is_empty()) {
		EditorNode::get_singleton()->push_item(p_selection.front());
	}

	for (const Variant &var : p_selection) {
		CanvasItem *ci = Object::cast_to<CanvasItem>(var);
		editor_selection->add_node(ci);
	}
}

void CanvasItemEditor::_clear_selection_requested(bool p_selected_from_canvas) {
	editor_selection->clear();
	viewport->queue_redraw();
	selected_from_canvas = p_selected_from_canvas;
}

void CanvasItemEditor::_selection_menu_requested(const Array &p_selection, const Point2 &p_pos, bool p_append) {
	if (clockgears_workspace) {
		return;
	}
	NodePath root_path = get_tree()->get_edited_scene_root()->get_path();
	StringName root_name = root_path.get_name(root_path.get_name_count() - 1);
	int icon_max_width = EditorNode::get_singleton()->get_editor_theme()->get_constant(SNAME("class_icon_size"), EditorStringName(Editor));

	for (const Variant &var : p_selection) {
		Node *node = Object::cast_to<Node>(var);
		Ref<Texture2D> icon = EditorNode::get_singleton()->get_object_icon(node);
		String node_path = "/" + root_name + "/" + String(root_path.rel_path_to(node->get_path()));

		int locked = 0;
		if (CanvasItemManipulator::is_node_locked(node)) {
			locked = 1;
		} else {
			Node *scene = EditorNode::get_singleton()->get_edited_scene();
			Node *node_tmp = node;

			while (node_tmp && node_tmp != scene->get_parent()) {
				if (node_tmp->has_meta("_edit_group_")) {
					locked = 2;
				}
				node_tmp = node_tmp->get_parent();
			}
		}

		String suffix;
		if (locked == 1) {
			suffix = " (" + TTR("Locked") + ")";
		} else if (locked == 2) {
			suffix = " (" + TTR("Grouped") + ")";
		}

		int idx = selection_menu->get_item_count();
		selection_menu->add_item((String)node->get_name() + suffix);
		selection_menu->set_item_icon(idx, icon);
		selection_menu->set_item_icon_max_width(idx, icon_max_width);
		selection_menu->set_item_metadata(idx, node_path);
		selection_menu->set_item_tooltip(idx, String(node->get_name()) + "\nType: " + node->get_class() + "\nPath: " + node_path);
	}

	selection_results_menu = p_selection;
	selection_menu_additive_selection = p_append;
	selection_menu->set_position(viewport->get_screen_transform().xform(p_pos));
	selection_menu->reset_size();
	selection_menu->popup();
}

void CanvasItemEditor::_tool_menu_requested(const Point2 &p_pos) {
	if (clockgears_workspace) {
		return;
	}
	add_node_menu->clear();
	add_node_menu->add_icon_item(get_editor_theme_icon(SNAME("Add")), TTRC("Add 2D Node Here..."), ADD_NODE);
	add_node_menu->add_icon_item(get_editor_theme_icon(SNAME("Instance")), TTRC("Instantiate Scene Here..."), ADD_INSTANCE);
	if (SceneTreeDock::get_singleton()) {
		for (Node *node : SceneTreeDock::get_singleton()->get_node_clipboard()) {
			if (Object::cast_to<CanvasItem>(node)) {
				add_node_menu->add_icon_item(get_editor_theme_icon(SNAME("ActionPaste")), TTRC("Paste Node(s) Here"), ADD_PASTE);
				break;
			}
		}
	}
	for (Node *node : EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list()) {
		if (Object::cast_to<CanvasItem>(node) && ci_manipulator->is_node_movable(node, false)) {
			add_node_menu->add_icon_item(get_editor_theme_icon(SNAME("ToolMove")), TTRC("Move Node(s) Here"), ADD_MOVE);
			break;
		}
	}

	if (EditorContextMenuPluginManager::get_singleton()->has_plugins_for_slot(EditorContextMenuPlugin::CONTEXT_SLOT_2D_EDITOR)) {
		EditorContextMenuPlugin::OptionsData context_data = _get_context_data();
		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(add_node_menu, EditorContextMenuPlugin::CONTEXT_SLOT_2D_EDITOR, context_data);

#ifndef DISABLE_DEPRECATED
		Array items = context_data["hovered_nodes"];
		PackedStringArray paths;
		paths.reserve_exact(items.size());
		for (const Variant &var : items) {
			CanvasItem *ci = Object::cast_to<CanvasItem>(var);
			paths.append((String)ci->get_path());
		}
		EditorContextMenuPluginManager::get_singleton()->add_options_from_plugins(add_node_menu, EditorContextMenuPlugin::CONTEXT_SLOT_2D_EDITOR, paths, items, 500);
#endif
	}

	add_node_menu->reset_size();
	add_node_menu->set_position(viewport->get_screen_transform().xform(p_pos));
	add_node_menu->popup();
	node_create_position = transform.affine_inverse().xform(p_pos);
}

void CanvasItemEditor::_commit_guide_meta_requested(const String &p_msg, const Dictionary &p_meta) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_msg);

	for (const KeyValue<Variant, Variant> &kv : p_meta) {
		Array guides = kv.value;
		if (guides.is_empty()) {
			undo_redo->add_do_method(EditorNode::get_singleton()->get_edited_scene(), "remove_meta", kv.key);
		} else {
			undo_redo->add_do_method(EditorNode::get_singleton()->get_edited_scene(), "set_meta", kv.key, guides);
		}

		Array prev_guides = EditorNode::get_singleton()->get_edited_scene()->get_meta(kv.key, Array());
		if (prev_guides.is_empty()) {
			undo_redo->add_undo_method(EditorNode::get_singleton()->get_edited_scene(), "remove_meta", kv.key);
		} else {
			undo_redo->add_undo_method(EditorNode::get_singleton()->get_edited_scene(), "set_meta", kv.key, prev_guides);
		}
	}

	undo_redo->add_undo_method(viewport, "queue_redraw");
	undo_redo->commit_action();
}

void CanvasItemEditor::_save_canvas_state_requested(const Array &p_selection, bool p_save_bones) {
	original_transform = Transform2D();
	bool transform_stored = false;

	for (const Variant &var : p_selection) {
		CanvasItem *ci = Object::cast_to<CanvasItem>(var);
		CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);
		if (se) {
			if (!transform_stored) {
				original_transform = ci->get_global_transform();
				transform_stored = true;
			}

			se->undo_state = ci->_edit_get_state();
			se->pre_drag_xform = ci->get_screen_transform();
			if (ci->_edit_use_rect()) {
				se->pre_drag_rect = ci->_edit_get_rect();
			} else {
				se->pre_drag_rect = Rect2();
			}
		}
	}
}

void CanvasItemEditor::_restore_canvas_state_requested(const Array &p_selection, bool p_restore_bones) {
	message = "";

	for (const Variant &var : p_selection) {
		CanvasItem *ci = Object::cast_to<CanvasItem>(var);
		CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);
		ci->_edit_set_state(se->undo_state);
	}
}

void CanvasItemEditor::_commit_canvas_state_requested(const Array &p_selection, const String &p_message, bool p_commit_bones) {
	message = "";

	List<CanvasItem *> modified_canvas_items;
	for (const Variant &var : p_selection) {
		CanvasItem *ci = Object::cast_to<CanvasItem>(var);
		Dictionary old_state = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci)->undo_state;
		Dictionary new_state = ci->_edit_get_state();

		if (old_state.hash() != new_state.hash()) {
			modified_canvas_items.push_back(ci);
		}
	}

	if (modified_canvas_items.is_empty()) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_message);
	for (CanvasItem *ci : modified_canvas_items) {
		CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);
		if (se) {
			undo_redo->add_do_method(ci, "_edit_set_state", ci->_edit_get_state());
			undo_redo->add_undo_method(ci, "_edit_set_state", se->undo_state);
			if (p_commit_bones) {
				for (const Dictionary &F : se->pre_drag_bones_undo_state) {
					ci = Object::cast_to<CanvasItem>(ci->get_parent());
					undo_redo->add_do_method(ci, "_edit_set_state", ci->_edit_get_state());
					undo_redo->add_undo_method(ci, "_edit_set_state", F);
				}
			}
		}
	}
	undo_redo->add_do_method(viewport, "queue_redraw");
	undo_redo->add_undo_method(viewport, "queue_redraw");
	undo_redo->commit_action();

	if (!key_auto_insert_button->is_pressed() || !key_auto_insert_button->is_visible_in_tree()) {
		return;
	}

	switch (ci_manipulator->get_drag_type()) {
		case CanvasItemManipulator::DRAG_ROTATE: {
			_insert_animation_keys(false, true, false, true);
		} break;

		case CanvasItemManipulator::DRAG_LEFT:
		case CanvasItemManipulator::DRAG_RIGHT:
		case CanvasItemManipulator::DRAG_TOP:
		case CanvasItemManipulator::DRAG_BOTTOM:
		case CanvasItemManipulator::DRAG_TOP_LEFT:
		case CanvasItemManipulator::DRAG_TOP_RIGHT:
		case CanvasItemManipulator::DRAG_BOTTOM_LEFT:
		case CanvasItemManipulator::DRAG_BOTTOM_RIGHT:
		case CanvasItemManipulator::DRAG_SCALE_BOTH:
		case CanvasItemManipulator::DRAG_SCALE_X:
		case CanvasItemManipulator::DRAG_SCALE_Y: {
			_insert_animation_keys(false, false, true, true);
		} break;

		case CanvasItemManipulator::DRAG_MOVE:
		case CanvasItemManipulator::DRAG_MOVE_X:
		case CanvasItemManipulator::DRAG_MOVE_Y: {
			_insert_animation_keys(true, false, false, true);
		} break;

		default:
			break;
	}
}

void CanvasItemEditor::_snap_changed() {
	Point2 grid_offset;
	Point2 grid_step;
	real_t snap_rotation_offset = 0;
	real_t snap_rotation_step = 0;
	real_t snap_scale_step = 0;
	static_cast<SnapDialog *>(snap_dialog)->get_fields(grid_offset, grid_step, primary_grid_step, snap_rotation_offset, snap_rotation_step, snap_scale_step);

	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "grid_offset", grid_offset);
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "grid_step", grid_step);
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "primary_grid_step", primary_grid_step);
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "snap_rotation_offset", snap_rotation_offset);
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "snap_rotation_step", snap_rotation_step);
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "snap_scale_step", snap_scale_step);

	ci_manipulator->set_grid_offset(grid_offset);
	ci_manipulator->set_grid_step(grid_step);
	ci_manipulator->set_grid_step_multiplier(0);
	ci_manipulator->set_snap_rotation_offset(snap_rotation_offset);
	ci_manipulator->set_snap_rotation_step(snap_rotation_step);
	ci_manipulator->set_snap_scale_step(snap_scale_step);

	viewport->queue_redraw();
	emit_signal("snap_changed");
}

void CanvasItemEditor::_selection_result_pressed(int p_result) {
	if (selection_results_menu.size() <= p_result) {
		return;
	}

	Variant var = selection_results_menu[p_result];
	if (var) {
		_select_click_on_item(var, selection_menu_additive_selection);
	}

	selection_results_menu.clear();
}

void CanvasItemEditor::_selection_menu_hide() {
	selection_results_menu.clear();
	selection_menu->clear();
	selection_menu->reset_size();
}

void CanvasItemEditor::_add_node_pressed(int p_result) {
	switch (p_result) {
		case ADD_NODE: {
			add_node_dialog->popup_create(true);
		} break;
		case ADD_INSTANCE: {
			EditorNode::get_singleton()->get_quick_open_dialog()->popup_dialog(Vector<StringName>{ "PackedScene" }, callable_mp(this, &CanvasItemEditor::_instantiate_scene));
		} break;
		case ADD_PASTE: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->create_action_for_history(TTR("Paste Node(s) at Position"), EditorNode::get_editor_data().get_current_edited_scene_history_id());

			List<Node *> pasted_nodes;
			if (SceneTreeDock::get_singleton()) {
				pasted_nodes = SceneTreeDock::get_singleton()->paste_nodes();
			}
			if (pasted_nodes.is_empty()) {
				return;
			}

			for (Node *node : pasted_nodes) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(node);
				if (ci) {
					Transform2D xform = ci->get_transform() * ci->get_global_transform_with_canvas().affine_inverse();
					undo_redo->add_do_method(ci, "_edit_set_position", xform.xform(node_create_position));
					undo_redo->add_undo_method(ci, "_edit_set_position", ci->_edit_get_position());
				}
			}
			undo_redo->commit_action();
		} break;
		case ADD_MOVE: {
			List<Node *> nodes_to_move = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
			if (nodes_to_move.is_empty()) {
				return;
			}

			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->create_action(TTR("Move Node(s) to Position"));
			for (Node *node : nodes_to_move) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(node);
				if (ci) {
					if (!ci_manipulator->is_node_movable(ci, true)) {
						continue;
					}
					Transform2D xform = ci->get_transform() * ci->get_global_transform_with_canvas().affine_inverse();
					undo_redo->add_do_method(ci, "_edit_set_position", xform.xform(node_create_position));
					undo_redo->add_undo_method(ci, "_edit_set_position", ci->_edit_get_position());
				}
			}
			undo_redo->commit_action();
		} break;
		default: {
			if (p_result >= EditorContextMenuPlugin::BASE_ID) {
				EditorContextMenuPluginManager::get_singleton()->activate_custom_option(EditorContextMenuPlugin::CONTEXT_SLOT_2D_EDITOR, p_result);
			}
		}
	}
}

void CanvasItemEditor::_create_node() {
	CanvasItem *child = add_node_dialog->instantiate_selected<CanvasItem>();
	ERR_FAIL_NULL(child);

	Node *parent = nullptr;
	if (SceneTreeDock::get_singleton() && SceneTreeDock::get_singleton()->get_tree_editor()) {
		parent = SceneTreeDock::get_singleton()->get_tree_editor()->get_selected();
	}
	if (!parent) {
		parent = EditorNode::get_editor_data().get_edited_scene_root();
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action_for_history(TTR("Create 2D Node"), EditorNode::get_editor_data().get_current_edited_scene_history_id());
	undo_redo->add_do_method(editor_selection, "clear");
	add_node_to_scene(parent, child, node_create_position);
	undo_redo->commit_action();
}

void CanvasItemEditor::_instantiate_scene(const String &p_path) {
	Node *parent = nullptr;
	if (SceneTreeDock::get_singleton() && SceneTreeDock::get_singleton()->get_tree_editor()) {
		parent = SceneTreeDock::get_singleton()->get_tree_editor()->get_selected();
	}
	if (!parent) {
		parent = EditorNode::get_editor_data().get_edited_scene_root();
	}

	Ref<PackedScene> sdata = ResourceLoader::load(p_path);
	if (sdata.is_null()) {
		ERR_FAIL_MSG("Loading scene failed.");
	}

	Node *instantiated_scene = sdata->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
	if (!instantiated_scene) {
		ERR_FAIL_MSG("Instantiating scene failed.");
	}

	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (!edited_scene->get_scene_file_path().is_empty()) { // Cyclic instantiation.
		if (cyclical_dependency_exists(edited_scene->get_scene_file_path(), instantiated_scene)) {
			memdelete(instantiated_scene);
			ERR_FAIL_MSG("Instantiate scene failed due to cyclic references.");
		}
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action_for_history(TTR("Instantiate Scene"), EditorNode::get_editor_data().get_current_edited_scene_history_id());
	undo_redo->add_do_method(editor_selection, "clear");
	add_node_to_scene(parent, instantiated_scene, node_create_position);
	undo_redo->commit_action();
}

bool CanvasItemEditor::is_grid_visible() const {
	switch (grid_visibility) {
		case GRID_VISIBILITY_SHOW:
			return true;
		case GRID_VISIBILITY_SHOW_WHEN_SNAPPING:
			return ci_manipulator->is_grid_snap_enabled();
		case GRID_VISIBILITY_HIDE:
			return false;
	}
	ERR_FAIL_V_MSG(true, "Unexpected grid_visibility value");
}

void CanvasItemEditor::_prepare_grid_menu() {
	for (int i = GRID_VISIBILITY_SHOW; i <= GRID_VISIBILITY_HIDE; i++) {
		grid_menu->set_item_checked(i, i == grid_visibility);
	}
}

void CanvasItemEditor::_on_grid_menu_id_pressed(int p_id) {
	switch (p_id) {
		case GRID_VISIBILITY_SHOW:
		case GRID_VISIBILITY_SHOW_WHEN_SNAPPING:
		case GRID_VISIBILITY_HIDE:
			grid_visibility = (GridVisibility)p_id;
			viewport->queue_redraw();
			view_menu->get_popup()->hide();
			emit_signal("grid_visibility_changed", is_grid_visible());
			return;
	}

	// Toggle grid: go to the least restrictive option possible.
	if (ci_manipulator->is_grid_snap_enabled()) {
		switch (grid_visibility) {
			case GRID_VISIBILITY_SHOW:
			case GRID_VISIBILITY_SHOW_WHEN_SNAPPING:
				grid_visibility = GRID_VISIBILITY_HIDE;
				break;
			case GRID_VISIBILITY_HIDE:
				grid_visibility = GRID_VISIBILITY_SHOW_WHEN_SNAPPING;
				break;
		}
	} else {
		switch (grid_visibility) {
			case GRID_VISIBILITY_SHOW:
				grid_visibility = GRID_VISIBILITY_SHOW_WHEN_SNAPPING;
				break;
			case GRID_VISIBILITY_SHOW_WHEN_SNAPPING:
			case GRID_VISIBILITY_HIDE:
				grid_visibility = GRID_VISIBILITY_SHOW;
				break;
		}
	}
	viewport->queue_redraw();
	emit_signal("grid_visibility_changed", is_grid_visible());
}

void CanvasItemEditor::_reset_transform(TransformType p_type) {
	List<Node *> selection = editor_selection->get_full_selected_node_list();
	if (selection.is_empty()) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Reset Transform"));
	for (Node *node : selection) {
		Node2D *res_node = Object::cast_to<Node2D>(node);
		if (res_node) {
			switch (p_type) {
				case TransformType::POSITION:
					undo_redo->add_undo_method(res_node, "set_position", res_node->get_position());
					undo_redo->add_do_method(res_node, "set_position", Vector2());
					break;
				case TransformType::ROTATION:
					undo_redo->add_undo_method(res_node, "set_rotation", res_node->get_rotation());
					undo_redo->add_do_method(res_node, "set_rotation", 0);
					break;
				case TransformType::SCALE:
					undo_redo->add_undo_method(res_node, "set_scale", res_node->get_scale());
					undo_redo->add_do_method(res_node, "set_scale", Size2(1, 1));
					break;
			}
			continue;
		}
		Control *res_control = Object::cast_to<Control>(node);
		if (res_control) {
			switch (p_type) {
				case TransformType::POSITION:
					undo_redo->add_undo_method(res_control, "set_position", res_control->get_position());
					undo_redo->add_do_method(res_control, "set_position", Vector2());
					break;
				case TransformType::ROTATION:
					undo_redo->add_undo_method(res_control, "set_rotation", res_control->get_rotation());
					undo_redo->add_do_method(res_control, "set_rotation", 0);
					break;
				case TransformType::SCALE:
					undo_redo->add_undo_method(res_control, "set_scale", res_control->get_scale());
					undo_redo->add_do_method(res_control, "set_scale", Size2(1, 1));
					break;
			}
		}
	}
	undo_redo->commit_action();
}

void CanvasItemEditor::_switch_theme_preview(int p_mode) {
	view_menu->get_popup()->hide();

	if (theme_preview == p_mode) {
		return;
	}
	theme_preview = (ThemePreviewMode)p_mode;
	EditorSettings::get_singleton()->set_project_metadata("2d_editor", "theme_preview", theme_preview);

	for (int i = 0; i < THEME_PREVIEW_MAX; i++) {
		theme_menu->set_item_checked(i, i == theme_preview);
	}

	EditorNode::get_singleton()->update_preview_themes(theme_preview);
}

bool CanvasItemEditor::_gui_input_zoom_or_pan(const Ref<InputEvent> &p_event, bool p_already_accepted) {
	panner->set_force_drag(ci_manipulator->get_tool() == CanvasItemManipulator::TOOL_PAN);
	bool panner_active = panner->gui_input(p_event, viewport->get_global_rect());
	if (panner->is_panning() != pan_pressed) {
		pan_pressed = panner->is_panning();
		_update_cursor();
	}

	if (panner_active) {
		return true;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		if (k->is_pressed()) {
			if (ED_IS_SHORTCUT("canvas_item_editor/zoom_3.125_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 32.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_6.25_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 16.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_12.5_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 8.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_25_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 4.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_50_percent", p_event)) {
				_shortcut_zoom_set(1.0 / 2.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_100_percent", p_event)) {
				_shortcut_zoom_set(1.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_200_percent", p_event)) {
				_shortcut_zoom_set(2.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_400_percent", p_event)) {
				_shortcut_zoom_set(4.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_800_percent", p_event)) {
				_shortcut_zoom_set(8.0);
			} else if (ED_IS_SHORTCUT("canvas_item_editor/zoom_1600_percent", p_event)) {
				_shortcut_zoom_set(16.0);
			}
		}
	}

	return false;
}

void CanvasItemEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event) {
	view_offset.x -= p_scroll_vec.x / zoom;
	view_offset.y -= p_scroll_vec.y / zoom;
	ci_manipulator->set_view_offset(view_offset);
	update_viewport();
}

void CanvasItemEditor::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Special behavior for scroll events, as the zoom_by_increment method can smartly end up on powers of two.
		int increment = p_zoom_factor > 1.0 ? 1 : -1;
		bool by_integer = mb->is_alt_pressed();

		if (EDITOR_GET("editors/2d/use_integer_zoom_by_default")) {
			by_integer = !by_integer;
		}

		zoom_widget->set_zoom_by_increments(increment, by_integer);
	} else {
		zoom_widget->set_zoom(zoom_widget->get_zoom() * p_zoom_factor);
	}

	_zoom_on_position(zoom_widget->get_zoom(), p_origin);
}

bool CanvasItemEditor::_gui_input_hover(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseMotion> m = p_event;
	if (m.is_valid()) {
		Point2 click = transform.affine_inverse().xform(m->get_position());

		// Checks if the hovered items changed, redraw the viewport if so.
		Vector<DebuggerHelpers::SelectResult> hovering_results_items;
		ci_manipulator->get_canvas_items_at_pos(click, hovering_results_items);
		hovering_results_items.sort();

		// Compute the nodes names and icon position.
		Vector<_HoverResult> hovering_results_tmp;
		for (int i = 0; i < hovering_results_items.size(); i++) {
			CanvasItem *ci = Object::cast_to<CanvasItem>(hovering_results_items[i].item);

			if (ci->_edit_use_rect()) {
				continue;
			}

			_HoverResult hover_result;
			hover_result.position = ci->get_screen_transform().get_origin();
			hover_result.icon = EditorNode::get_singleton()->get_object_icon(ci);
			hover_result.name = ci->get_name();

			hovering_results_tmp.push_back(hover_result);
		}

		// Check if changed, if so, redraw.
		bool changed = false;
		if (hovering_results_tmp.size() == hovering_results.size()) {
			for (int i = 0; i < hovering_results_tmp.size(); i++) {
				_HoverResult a = hovering_results_tmp[i];
				_HoverResult b = hovering_results[i];
				if (a.icon != b.icon || a.name != b.name || a.position != b.position) {
					changed = true;
					break;
				}
			}
		} else {
			changed = true;
		}

		if (changed) {
			hovering_results = hovering_results_tmp;
			viewport->queue_redraw();
		}

		return true;
	}

	return false;
}

void CanvasItemEditor::_gui_input_viewport(const Ref<InputEvent> &p_event) {
	bool accepted = false;

	Ref<InputEventMouseButton> mb = p_event;
	bool release_lmb = (mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT); // Required to properly release some stuff (e.g. selection box) while panning.

	if (clockgears_workspace) {
		if (clockgears_event_mode) {
			// ==========================================
			// MODO EVENTOS: Clic derecho (menú), doble clic (editar), hover
			// ==========================================
			Ref<InputEventMouseButton> mb_ev = p_event;
			if (mb_ev.is_valid() && mb_ev->get_button_index() == MouseButton::RIGHT && mb_ev->is_pressed()) {
				Point2 click = transform.affine_inverse().xform(mb_ev->get_position());
				int cx = Math::floor(click.x / (real_t)clockgears_grid_size);
				int cy = Math::floor(click.y / (real_t)clockgears_grid_size);
				Point2 screen_pos = viewport->get_screen_transform().xform(mb_ev->get_position());
				if (clockgears_event_cell_rmb_cb.is_valid()) {
					clockgears_event_cell_rmb_cb.call(cx, cy, screen_pos);
				}
				accept_event();
				return;
			}

			Ref<InputEventMouseMotion> mm_ev = p_event;
			if (mm_ev.is_valid()) {
				Point2 click = transform.affine_inverse().xform(mm_ev->get_position());
				int cx = Math::floor(click.x / (real_t)clockgears_grid_size);
				int cy = Math::floor(click.y / (real_t)clockgears_grid_size);
				if (cx != clockgears_hovered_cell.x || cy != clockgears_hovered_cell.y) {
					clockgears_hovered_cell = Point2i(cx, cy);
					viewport->queue_redraw();
				}
			}

			if (mb_ev.is_valid() && mb_ev->is_pressed() && mb_ev->get_button_index() == MouseButton::LEFT && mb_ev->is_double_click()) {
				Point2 click = transform.affine_inverse().xform(mb_ev->get_position());
				int cx = Math::floor(click.x / (real_t)clockgears_grid_size);
				int cy = Math::floor(click.y / (real_t)clockgears_grid_size);
				if (clockgears_event_double_clicked_cb.is_valid()) {
					clockgears_event_double_clicked_cb.call(cx, cy);
					accept_event();
					return;
				}
			}

			if (mb_ev.is_valid() && mb_ev->get_button_index() == MouseButton::LEFT && mb_ev->is_pressed()) {
				accept_event();
				return;
			}
		} else {
			// =========================================================================
			// MODO MAPA (LÁPIZ, RECTÁNGULO, ELIPSE, CUBO, SOMBRA, SELECCIÓN, REGIONES)
			// =========================================================================
			Ref<InputEventKey> key_ev = p_event;
			if (key_ev.is_valid() && key_ev->is_pressed() && !key_ev->is_echo()) {
				if (key_ev->is_ctrl_pressed() && key_ev->get_keycode() == Key::C) {
					clockgears_copy_map_selection();
					accept_event();
					return;
				}
				if (key_ev->is_ctrl_pressed() && key_ev->get_keycode() == Key::V) {
					clockgears_paste_map_selection();
					accept_event();
					return;
				}
				if (key_ev->get_keycode() == Key::KEY_DELETE || key_ev->get_keycode() == Key::BACKSPACE) {
					if (clockgears_has_map_selection) {
						clockgears_delete_map_selection();
						accept_event();
						return;
					}
				}
			}

			Ref<InputEventMouseButton> mb_tile = p_event;
			if (mb_tile.is_valid()) {
				if (mb_tile->get_button_index() == MouseButton::LEFT || mb_tile->get_button_index() == MouseButton::RIGHT) {
					bool is_erase = (mb_tile->get_button_index() == MouseButton::RIGHT);
					Point2 click = transform.affine_inverse().xform(mb_tile->get_position());
					TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
					Point2i cell;
					if (layer) {
						cell = layer->local_to_map(layer->to_local(click));
					} else {
						cell = Point2i(Math::floor(click.x / (real_t)clockgears_grid_size), Math::floor(click.y / (real_t)clockgears_grid_size));
					}
					clockgears_hovered_cell = cell;

					static const char *tool_names[] = { "Lápiz", "Rectángulo", "Elipse", "Cubo de Pintura", "Sombra", "Selección Rectangular", "Posición Inicial", "Goma" };
					String cur_tool_str = (current_draw_tool >= 0 && current_draw_tool < 8) ? tool_names[current_draw_tool] : "Desconocido";

					if (mb_tile->is_pressed()) {
						clockgears_is_painting = !is_erase && (current_draw_tool != 7);
						clockgears_is_erasing = is_erase || (current_draw_tool == 7);
						clockgears_drag_start_cell = cell;
						clockgears_last_painted_cell = cell;
						clockgears_stroke_before_cells.clear();
						clockgears_stroke_after_cells.clear();

						String lpath = (layer && layer->is_inside_tree()) ? String(layer->get_path()) : (layer ? String(layer->get_name()) : String("Ninguna"));
						print_line(vformat(String::utf8("[ClockGears Draw] Presionar - Herramienta: %s, Capa: '%s' (Ruta: %s, TileSet: %s), Celda: (%d, %d), Modo: %s"),
								cur_tool_str, layer ? String(layer->get_name()) : String("Ninguna"),
								lpath,
								(layer && layer->get_tile_set().is_valid()) ? "Sí" : "No",
								cell.x, cell.y, (is_erase || current_draw_tool == 7) ? "Goma/Borrar" : "Pintar"));

						if (clockgears_region_mode) {
							_paint_region_cell(cell, is_erase);
						} else if (current_draw_tool == 4) {
							_paint_shadow_cell(cell, is_erase);
						} else if (current_draw_tool == 3) {
							// Cubo de Pintura (Flood Fill)
							_paint_tile_bucket(cell, is_erase);
							_commit_tile_stroke_undo_redo(layer, is_erase ? TTR("Borrar Área con Cubo") : TTR("Rellenar con Cubo"));
							clockgears_is_painting = false;
							clockgears_is_erasing = false;
						} else if (current_draw_tool == 5) {
							// Selección Rectangular
							clockgears_has_map_selection = true;
							clockgears_map_selection_rect = Rect2i(cell, Vector2i(1, 1));
							viewport->queue_redraw();
						} else if (current_draw_tool == 6) {
							// Herramienta 6: Posición Inicial del Jugador (único punto de inicio)
							Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
							Object *dm = Engine::get_singleton()->get_singleton_object("ClockGearsDataManager");
							if (scene && dm) {
								Dictionary sys = dm->call("get_system_config");
								String map_path = scene->get_scene_file_path();
								if (map_path.is_empty()) {
									map_path = "res://maps/map001.tscn";
								}
								if (is_erase) {
									if (bool(sys.get("has_start_pos", false)) && String(sys.get("start_map", "")) == map_path && int(sys.get("start_x", -1)) == cell.x && int(sys.get("start_y", -1)) == cell.y) {
										sys["has_start_pos"] = false;
										dm->call("set_system_config", sys);
										dm->call("save_all", "res://clockgears_data");
										print_line(String::utf8("[ClockGears StartPos] Posición inicial del jugador eliminada."));
									}
								} else {
									int next_dir = 2; // 2 = Abajo por defecto
									if (bool(sys.get("has_start_pos", false)) && String(sys.get("start_map", "")) == map_path && int(sys.get("start_x", -1)) == cell.x && int(sys.get("start_y", -1)) == cell.y) {
										int cur_dir = int(sys.get("start_dir", 2));
										if (cur_dir == 2) {
											next_dir = 4;
										} else if (cur_dir == 4) {
											next_dir = 8;
										} else if (cur_dir == 8) {
											next_dir = 6;
										} else {
											next_dir = 2;
										}
									}
									if (scene->has_method("set_player_start_position")) {
										scene->call("set_player_start_position", Vector2i(cell.x, cell.y), next_dir);
									} else {
										sys["has_start_pos"] = true;
										sys["start_map"] = map_path;
										sys["start_x"] = cell.x;
										sys["start_y"] = cell.y;
										sys["start_dir"] = next_dir;
										dm->call("set_system_config", sys);
										dm->call("save_all", "res://clockgears_data");
									}
								}
							}
							clockgears_is_painting = false;
							clockgears_is_erasing = false;
							viewport->queue_redraw();
						} else if (current_draw_tool == 0 || current_draw_tool == 7) {
							// Lápiz o Goma
							_paint_tile_cell(cell, is_erase || (current_draw_tool == 7));
						} else {
							viewport->queue_redraw();
						}
						accept_event();
						return;
					} else {
						if (clockgears_is_painting || clockgears_is_erasing) {
							bool was_erase = clockgears_is_erasing;
							if (!clockgears_region_mode) {
								if (current_draw_tool == 1) {
									// Rectángulo
									_paint_tile_rect(clockgears_drag_start_cell, cell, was_erase);
									_commit_tile_stroke_undo_redo(layer, was_erase ? TTR("Borrar Rectángulo de Tiles") : TTR("Pintar Rectángulo de Tiles"));
								} else if (current_draw_tool == 2) {
									// Elipse
									_paint_tile_ellipse(clockgears_drag_start_cell, cell, was_erase);
									_commit_tile_stroke_undo_redo(layer, was_erase ? TTR("Borrar Elipse de Tiles") : TTR("Pintar Elipse de Tiles"));
								} else if (current_draw_tool == 0 || current_draw_tool == 7) {
									_commit_tile_stroke_undo_redo(layer, (was_erase || current_draw_tool == 7) ? TTR("Borrar Tiles") : TTR("Pintar Tiles"));
								} else if (current_draw_tool == 5) {
									int x0 = MIN(clockgears_drag_start_cell.x, cell.x);
									int y0 = MIN(clockgears_drag_start_cell.y, cell.y);
									int x1 = MAX(clockgears_drag_start_cell.x, cell.x);
									int y1 = MAX(clockgears_drag_start_cell.y, cell.y);
									clockgears_has_map_selection = true;
									clockgears_map_selection_rect = Rect2i(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
								}
							}
							String lpath = (layer && layer->is_inside_tree()) ? String(layer->get_path()) : (layer ? String(layer->get_name()) : String("Ninguna"));
							print_line(vformat(String::utf8("[ClockGears Draw] Soltar - Herramienta: %s, Capa: '%s' (Ruta: %s, TileSet: %s), Celda final: (%d, %d)"),
									cur_tool_str, layer ? String(layer->get_name()) : String("Ninguna"),
									lpath,
									(layer && layer->get_tile_set().is_valid()) ? "Sí" : "No",
									cell.x, cell.y));

							clockgears_is_painting = false;
							clockgears_is_erasing = false;
							clockgears_last_painted_cell = Point2i(-999999, -999999);
							viewport->queue_redraw();
						}
						accept_event();
						return;
					}
				}
			}

			Ref<InputEventMouseMotion> mm_tile = p_event;
			if (mm_tile.is_valid()) {
				Point2 click = transform.affine_inverse().xform(mm_tile->get_position());
				TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
				Point2i cell;
				if (layer) {
					cell = layer->local_to_map(layer->to_local(click));
				} else {
					cell = Point2i(Math::floor(click.x / (real_t)clockgears_grid_size), Math::floor(click.y / (real_t)clockgears_grid_size));
				}

				if (cell != clockgears_hovered_cell) {
					clockgears_hovered_cell = cell;
					viewport->queue_redraw();
				}

				if (clockgears_is_painting || clockgears_is_erasing) {
					bool is_erase = clockgears_is_erasing;
					if (clockgears_region_mode) {
						_paint_region_cell(cell, is_erase);
					} else if (current_draw_tool == 4) {
						_paint_shadow_cell(cell, is_erase);
					} else if (current_draw_tool == 0 || current_draw_tool == 7) {
						if (clockgears_last_painted_cell.x != -999999) {
							_paint_tile_line(clockgears_last_painted_cell, cell, is_erase);
						} else {
							_paint_tile_cell(cell, is_erase);
						}
						clockgears_last_painted_cell = cell;
					} else if (current_draw_tool == 5) {
						int x0 = MIN(clockgears_drag_start_cell.x, cell.x);
						int y0 = MIN(clockgears_drag_start_cell.y, cell.y);
						int x1 = MAX(clockgears_drag_start_cell.x, cell.x);
						int y1 = MAX(clockgears_drag_start_cell.y, cell.y);
						clockgears_has_map_selection = true;
						clockgears_map_selection_rect = Rect2i(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
						viewport->queue_redraw();
					} else {
						viewport->queue_redraw();
					}
					clockgears_last_painted_cell = cell;
					accept_event();
					return;
				}
			}
		}
	}

	if (simple_panning || !pan_pressed || release_lmb) {
		accepted = ci_manipulator->gui_input(p_event);
	}

	accepted = (_gui_input_zoom_or_pan(p_event, accepted) || accepted);

	if (accepted) {
		accept_event();
	}

	// Handles the mouse hovering
	_gui_input_hover(p_event);

	if (mb.is_valid()) {
		// Update the default cursor.
		_update_cursor();
	}

	// Grab focus
	if (!viewport->has_focus() && (!get_viewport()->gui_get_focus_owner() || !get_viewport()->gui_get_focus_owner()->is_text_field())) {
		callable_mp((Control *)viewport, &Control::grab_focus).call_deferred(false);
	}
}

void CanvasItemEditor::_update_cursor() {
	if (cursor_shape_override != CURSOR_ARROW) {
		set_default_cursor_shape(cursor_shape_override);
		return;
	}
	set_default_cursor_shape((Control::CursorShape)ci_manipulator->get_cursor_shape());
}

void CanvasItemEditor::_update_lock_and_group_button() {
	bool all_locked = true;
	bool all_group = true;
	bool has_canvas_item = false;
	const List<Node *> &selection = editor_selection->get_top_selected_node_list();
	if (selection.is_empty()) {
		all_locked = false;
		all_group = false;
	} else {
		for (Node *E : selection) {
			CanvasItem *item = Object::cast_to<CanvasItem>(E);
			if (item) {
				if (all_locked && !item->has_meta("_edit_lock_")) {
					all_locked = false;
				}
				if (all_group && !item->has_meta("_edit_group_")) {
					all_group = false;
				}
				has_canvas_item = true;
			}
			if (!all_locked && !all_group) {
				break;
			}
		}
	}

	all_locked = all_locked && has_canvas_item;
	all_group = all_group && has_canvas_item;

	lock_button->set_visible(!all_locked);
	lock_button->set_disabled(!has_canvas_item);
	unlock_button->set_visible(all_locked);
	unlock_button->set_disabled(!has_canvas_item);
	group_button->set_visible(!all_group);
	group_button->set_disabled(!has_canvas_item);
	ungroup_button->set_visible(all_group);
	ungroup_button->set_disabled(!has_canvas_item);
}

void CanvasItemEditor::set_cursor_shape_override(CursorShape p_shape) {
	if (cursor_shape_override != p_shape) {
		cursor_shape_override = p_shape;
		_update_cursor();
	}
}

Control::CursorShape CanvasItemEditor::get_cursor_shape(const Point2 &p_pos) const {
	return pan_pressed ? CURSOR_DRAG : get_default_cursor_shape();
}

bool CanvasItemEditor::cyclical_dependency_exists(const String &p_target_scene_path, Node *p_desired_node) const {
	if (p_desired_node->get_scene_file_path() == p_target_scene_path) {
		return true;
	}

	for (Node *child : p_desired_node->iterate_children()) {
		if (cyclical_dependency_exists(p_target_scene_path, child)) {
			return true;
		}
	}
	return false;
}

static Transform2D get_parent_screen_transform(Node *p_node) {
	CanvasItem *ci = Object::cast_to<CanvasItem>(p_node);
	if (ci) {
		return ci->get_screen_transform();
	}
	SubViewport *vp = Object::cast_to<SubViewport>(p_node);
	if (vp) {
		return vp->get_popup_base_transform();
	}
	return Transform2D();
}

void CanvasItemEditor::add_node_to_scene(Node *p_parent, Node *p_child, const Vector2 &p_target_position) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (p_parent) {
		undo_redo->add_do_method(p_parent, "add_child", p_child, true);
		undo_redo->add_do_method(p_child, "set_owner", EditorNode::get_singleton()->get_edited_scene());
		undo_redo->add_do_reference(p_child);
		undo_redo->add_undo_method(p_parent, "remove_child", p_child);

		const String new_name = p_parent->validate_child_name(p_child);
		EditorDebuggerNode *ed = EditorDebuggerNode::get_singleton();
		const NodePath path_to_child = EditorNode::get_singleton()->get_edited_scene()->get_path_to(p_parent);
		if (p_child->is_instance()) {
			undo_redo->add_do_method(ed, "live_debug_instantiate_node", path_to_child, p_child->get_scene_file_path(), new_name);
		} else {
			undo_redo->add_do_method(ed, "live_debug_create_node", path_to_child, p_child->get_class(), new_name);
		}
		undo_redo->add_undo_method(ed, "live_debug_remove_node", NodePath(String(path_to_child) + "/" + new_name));
	} else { // If no parent is selected, set as root node of the scene.
		undo_redo->add_do_method(EditorNode::get_singleton(), "set_edited_scene", p_child);
		undo_redo->add_do_reference(p_child);
		undo_redo->add_undo_method(EditorNode::get_singleton(), "set_edited_scene", (Object *)nullptr);
	}

	if (Object::cast_to<CanvasItem>(p_child)) {
		// There's nothing to be used as source position, so snapping will work as absolute if enabled.
		Vector2 target_position = ci_manipulator->snap_point(p_target_position);
		// Set position via undo_redo for proper live editing support.
		undo_redo->add_do_method(p_child, "set_position", get_parent_screen_transform(p_parent).affine_inverse().xform(target_position));
	}

	undo_redo->add_do_method(editor_selection, "add_node", p_child);
}

void CanvasItemEditor::_draw_text_at_position(Point2 p_position, const String &p_string, Side p_side) {
	Color color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	color.a = 0.8;
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	Size2 text_size = font->get_string_size(p_string, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
	switch (p_side) {
		case SIDE_LEFT:
			p_position += Vector2(-text_size.x - 5, text_size.y / 2);
			break;
		case SIDE_TOP:
			p_position += Vector2(-text_size.x / 2, -5);
			break;
		case SIDE_RIGHT:
			p_position += Vector2(5, text_size.y / 2);
			break;
		case SIDE_BOTTOM:
			p_position += Vector2(-text_size.x / 2, text_size.y + 5);
			break;
	}
	viewport->draw_string(font, p_position, p_string, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color);
}

void CanvasItemEditor::_draw_margin_at_position(int p_value, Point2 p_position, Side p_side) {
	String str = TranslationServer::get_singleton()->format_number(vformat("%d " + TTR("px"), p_value), _get_locale());
	if (p_value != 0) {
		_draw_text_at_position(p_position, str, p_side);
	}
}

void CanvasItemEditor::_draw_percentage_at_position(real_t p_value, Point2 p_position, Side p_side) {
	const String &lang = _get_locale();
	String str = TranslationServer::get_singleton()->format_number(vformat("%.1f ", p_value * 100.0), lang) + TranslationServer::get_singleton()->get_percent_sign(lang);
	if (p_value != 0) {
		_draw_text_at_position(p_position, str, p_side);
	}
}

void CanvasItemEditor::_draw_focus() {
	// Draw the focus around the base viewport
	if (viewport->has_focus()) {
		get_theme_stylebox(SNAME("FocusViewport"), EditorStringName(EditorStyles))->draw(viewport->get_canvas_item(), Rect2(Point2(), viewport->get_size()));
	}
}

void CanvasItemEditor::_draw_guides() {
	Color guide_color = EDITOR_GET("editors/2d/guides_color");
	Transform2D xform = viewport_scrollable->get_transform() * transform;
	CanvasItemManipulator::DragType drag = ci_manipulator->get_drag_type();

	// Guides already there.
	if (Node *scene = EditorNode::get_singleton()->get_edited_scene()) {
		Array vguides = scene->get_meta("_edit_vertical_guides_", Array());
		int dragged_guide_index = ci_manipulator->get_dragged_guide_index();

		for (int i = 0; i < vguides.size(); i++) {
			if (drag == CanvasItemManipulator::DRAG_V_GUIDE && i == dragged_guide_index) {
				continue;
			}
			real_t x = xform.xform(Point2(vguides[i], 0)).x;
			viewport->draw_line(Point2(x, 0), Point2(x, viewport->get_size().y), guide_color, Math::round(EDSCALE));
		}

		Array hguides = scene->get_meta("_edit_horizontal_guides_", Array());
		for (int i = 0; i < hguides.size(); i++) {
			if (drag == CanvasItemManipulator::DRAG_H_GUIDE && i == dragged_guide_index) {
				continue;
			}
			real_t y = xform.xform(Point2(0, hguides[i])).y;
			viewport->draw_line(Point2(0, y), Point2(viewport->get_size().x, y), guide_color, Math::round(EDSCALE));
		}
	}

	// Dragged guide.
	Color text_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	Color outline_color = text_color.inverted();
	const float outline_size = 2;
	const String &lang = _get_locale();
	Point2 dragged_guide_pos = ci_manipulator->get_dragged_guide_position();
	real_t ruler_width = ci_manipulator->get_ruler_width();

	if (drag == CanvasItemManipulator::DRAG_DOUBLE_GUIDE || drag == CanvasItemManipulator::DRAG_V_GUIDE) {
		String str = TranslationServer::get_singleton()->format_number(vformat("%d px", Math::round(xform.affine_inverse().xform(dragged_guide_pos).x)), lang);
		Ref<Font> font = get_theme_font(SNAME("bold"), EditorStringName(EditorFonts));
		int font_size = 1.3 * get_theme_font_size(SNAME("bold_size"), EditorStringName(EditorFonts));
		Size2 text_size = font->get_string_size(str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		viewport->draw_string_outline(font, Point2(dragged_guide_pos.x + 10, ruler_width + text_size.y / 2 + 10), str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
		viewport->draw_string(font, Point2(dragged_guide_pos.x + 10, ruler_width + text_size.y / 2 + 10), str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_color);
		viewport->draw_line(Point2(dragged_guide_pos.x, 0), Point2(dragged_guide_pos.x, viewport->get_size().y), guide_color, Math::round(EDSCALE));
	}
	if (drag == CanvasItemManipulator::DRAG_DOUBLE_GUIDE || drag == CanvasItemManipulator::DRAG_H_GUIDE) {
		String str = TranslationServer::get_singleton()->format_number(vformat("%d px", Math::round(xform.affine_inverse().xform(dragged_guide_pos).y)), lang);
		Ref<Font> font = get_theme_font(SNAME("bold"), EditorStringName(EditorFonts));
		int font_size = 1.3 * get_theme_font_size(SNAME("bold_size"), EditorStringName(EditorFonts));
		Size2 text_size = font->get_string_size(str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		viewport->draw_string_outline(font, Point2(ruler_width + 10, dragged_guide_pos.y + text_size.y / 2 + 10), str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
		viewport->draw_string(font, Point2(ruler_width + 10, dragged_guide_pos.y + text_size.y / 2 + 10), str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_color);
		viewport->draw_line(Point2(0, dragged_guide_pos.y), Point2(viewport->get_size().x, dragged_guide_pos.y), guide_color, Math::round(EDSCALE));
	}
}

void CanvasItemEditor::_draw_smart_snapping() {
	Color line_color = EDITOR_GET("editors/2d/smart_snapping_line_color");
	Transform2D snap_transform = ci_manipulator->get_snap_transform();
	Pair<CanvasItemManipulator::SnapTarget, CanvasItemManipulator::SnapTarget> snap_target = ci_manipulator->get_snap_target();

	if (snap_target.first != CanvasItemManipulator::SNAP_TARGET_NONE && snap_target.first != CanvasItemManipulator::SNAP_TARGET_GRID) {
		viewport->draw_set_transform_matrix(viewport->get_transform() * transform * snap_transform);
		viewport->draw_line(Point2(0, -1.0e+10F), Point2(0, 1.0e+10F), line_color);
		viewport->draw_set_transform_matrix(viewport->get_transform());
	}
	if (snap_target.second != CanvasItemManipulator::SNAP_TARGET_NONE && snap_target.second != CanvasItemManipulator::SNAP_TARGET_GRID) {
		viewport->draw_set_transform_matrix(viewport->get_transform() * transform * snap_transform);
		viewport->draw_line(Point2(-1.0e+10F, 0), Point2(1.0e+10F, 0), line_color);
		viewport->draw_set_transform_matrix(viewport->get_transform());
	}
}

void CanvasItemEditor::_draw_rulers() {
	Color bg_color = get_theme_color(SNAME("ruler_color"), EditorStringName(Editor));
	Color graduation_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor)).lerp(bg_color, 0.5);
	Color font_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	font_color.a = 0.9;
	Ref<Font> font = get_theme_font(SNAME("rulers"), EditorStringName(EditorFonts));
	real_t ruler_width = ci_manipulator->get_ruler_width();
	real_t ruler_tick_scale = ruler_width / 15.0;
	const String lang = _get_locale();

	// The rule transform.
	Transform2D ruler_transform;
	if (ci_manipulator->is_grid_snap_enabled() || is_grid_visible()) {
		List<CanvasItem *> selection = _get_edited_canvas_items();
		if (ci_manipulator->is_snap_relative_enabled() && selection.size() > 0) {
			ruler_transform.translate_local(_get_encompassing_rect_from_list(selection).position);
			ruler_transform.scale_basis(ci_manipulator->get_grid_step() * Math::pow(2.0, ci_manipulator->get_grid_step_multiplier()));
		} else {
			ruler_transform.translate_local(ci_manipulator->get_grid_offset());
			ruler_transform.scale_basis(ci_manipulator->get_grid_step() * Math::pow(2.0, ci_manipulator->get_grid_step_multiplier()));
		}
		while ((transform * ruler_transform).get_scale().x < 50.0 * ruler_tick_scale || (transform * ruler_transform).get_scale().y < 50.0 * ruler_tick_scale) {
			ruler_transform.scale_basis(Point2(2, 2));
		}
	} else {
		real_t basic_rule = 100;
		for (int i = 0; basic_rule * zoom > 100 * ruler_tick_scale; i++) {
			basic_rule /= (i % 2) ? 5.0 : 2.0;
		}
		for (int i = 0; basic_rule * zoom < 60 * ruler_tick_scale; i++) {
			basic_rule *= (i % 2) ? 2.0 : 5.0;
		}
		ruler_transform.scale(Size2(basic_rule, basic_rule));
	}

	// Subdivisions.
	int major_subdivision = 2;
	Transform2D major_subdivide;
	major_subdivide.scale(Size2(1.0 / major_subdivision, 1.0 / major_subdivision));

	int minor_subdivision = 5;
	Transform2D minor_subdivide;
	minor_subdivide.scale(Size2(1.0 / minor_subdivision, 1.0 / minor_subdivision));

	// First and last graduations to draw (in the ruler space).
	Point2 first = (transform * ruler_transform * major_subdivide * minor_subdivide).affine_inverse().xform(Point2(ruler_width, ruler_width));
	Point2 last = (transform * ruler_transform * major_subdivide * minor_subdivide).affine_inverse().xform(viewport->get_size());

	// Draw top ruler.
	viewport->draw_rect(Rect2(Point2(ruler_width, 0), Size2(viewport->get_size().x, ruler_width)), bg_color);
	for (int i = Math::ceil(first.x); i < last.x; i++) {
		Point2 position = (transform * ruler_transform * major_subdivide * minor_subdivide).xform(Point2(i, 0)).round();
		if (i % (major_subdivision * minor_subdivision) == 0) {
			viewport->draw_line(Point2(position.x, 0), Point2(position.x, ruler_width), graduation_color, Math::round(EDSCALE));
			real_t val = (ruler_transform * major_subdivide * minor_subdivide).xform(Point2(i, 0)).x;
			const String &formatted = TranslationServer::get_singleton()->format_number(vformat(((int)val == val) ? "%d" : "%.1f", val), lang);
			viewport->draw_string(font, Point2(position.x + MAX(Math::round(ruler_font_size / 8.0), 2), font->get_ascent(ruler_font_size) + Math::round(EDSCALE)), formatted, HORIZONTAL_ALIGNMENT_LEFT, -1, ruler_font_size, font_color);
		} else {
			if (i % minor_subdivision == 0) {
				viewport->draw_line(Point2(position.x, ruler_width * 0.33), Point2(position.x, ruler_width), graduation_color, Math::round(EDSCALE));
			} else {
				viewport->draw_line(Point2(position.x, ruler_width * 0.75), Point2(position.x, ruler_width), graduation_color, Math::round(EDSCALE));
			}
		}
	}

	// Draw left ruler.
	viewport->draw_rect(Rect2(Point2(0, ruler_width), Size2(ruler_width, viewport->get_size().y)), bg_color);
	for (int i = Math::ceil(first.y); i < last.y; i++) {
		Point2 position = (transform * ruler_transform * major_subdivide * minor_subdivide).xform(Point2(0, i)).round();
		if (i % (major_subdivision * minor_subdivision) == 0) {
			viewport->draw_line(Point2(0, position.y), Point2(ruler_width, position.y), graduation_color, Math::round(EDSCALE));
			real_t val = (ruler_transform * major_subdivide * minor_subdivide).xform(Point2(0, i)).y;

			Transform2D text_xform = Transform2D(-Math::PI / 2.0, Point2(font->get_ascent(ruler_font_size) + Math::round(EDSCALE), position.y - 2));
			viewport->draw_set_transform_matrix(viewport->get_transform() * text_xform);
			const String &formatted = TranslationServer::get_singleton()->format_number(vformat(((int)val == val) ? "%d" : "%.1f", val), lang);
			viewport->draw_string(font, Point2(), formatted, HORIZONTAL_ALIGNMENT_LEFT, -1, ruler_font_size, font_color);
			viewport->draw_set_transform_matrix(viewport->get_transform());

		} else {
			if (i % minor_subdivision == 0) {
				viewport->draw_line(Point2(ruler_width * 0.33, position.y), Point2(ruler_width, position.y), graduation_color, Math::round(EDSCALE));
			} else {
				viewport->draw_line(Point2(ruler_width * 0.75, position.y), Point2(ruler_width, position.y), graduation_color, Math::round(EDSCALE));
			}
		}
	}

	// Draw the top left corner.
	viewport->draw_rect(Rect2(Point2(), Size2(ruler_width, ruler_width)), graduation_color);
}

void CanvasItemEditor::_draw_grid() {
	if (is_grid_visible()) {
		// Draw the grid.
		Vector2 real_grid_offset;
		const List<CanvasItem *> selection = _get_edited_canvas_items();
		Point2 grid_step = ci_manipulator->get_grid_step();
		Point2 grid_offset = ci_manipulator->get_grid_offset();
		int grid_step_multiplier = ci_manipulator->get_grid_step_multiplier();

		if (ci_manipulator->is_snap_relative_enabled() && selection.size() > 0) {
			const Vector2 topleft = _get_encompassing_rect_from_list(selection).position;
			real_grid_offset.x = std::fmod(topleft.x, grid_step.x * (real_t)Math::pow(2.0, grid_step_multiplier));
			real_grid_offset.y = std::fmod(topleft.y, grid_step.y * (real_t)Math::pow(2.0, grid_step_multiplier));
		} else {
			real_grid_offset = grid_offset;
		}

		// Draw a "primary" line every several lines to make measurements easier.
		// The step is configurable in the Configure Snap dialog.
		const Color secondary_grid_color = EDITOR_GET("editors/2d/grid_color");
		const Color primary_grid_color =
				Color(secondary_grid_color.r, secondary_grid_color.g, secondary_grid_color.b, secondary_grid_color.a * 2.5);

		const Size2 viewport_size = viewport->get_size();
		const Transform2D xform = transform.affine_inverse();
		int last_cell = 0;

		if (grid_step.x != 0) {
			for (int i = 0; i < viewport_size.width; i++) {
				const int cell =
						Math::fast_ftoi(Math::floor((xform.xform(Vector2(i, 0)).x - real_grid_offset.x) / (grid_step.x * Math::pow(2.0, grid_step_multiplier))));

				if (i == 0) {
					last_cell = cell;
				}

				if (last_cell != cell) {
					Color grid_color;
					if (primary_grid_step.x <= 1) {
						grid_color = secondary_grid_color;
					} else {
						grid_color = cell % primary_grid_step.x == 0 ? primary_grid_color : secondary_grid_color;
					}

					viewport->draw_line(Point2(i, 0), Point2(i, viewport_size.height), grid_color, Math::round(EDSCALE));
				}
				last_cell = cell;
			}
		}

		if (grid_step.y != 0) {
			for (int i = 0; i < viewport_size.height; i++) {
				const int cell =
						Math::fast_ftoi(Math::floor((xform.xform(Vector2(0, i)).y - real_grid_offset.y) / (grid_step.y * Math::pow(2.0, grid_step_multiplier))));

				if (i == 0) {
					last_cell = cell;
				}

				if (last_cell != cell) {
					Color grid_color;
					if (primary_grid_step.y <= 1) {
						grid_color = secondary_grid_color;
					} else {
						grid_color = cell % primary_grid_step.y == 0 ? primary_grid_color : secondary_grid_color;
					}

					viewport->draw_line(Point2(0, i), Point2(viewport_size.width, i), grid_color, Math::round(EDSCALE));
				}
				last_cell = cell;
			}
		}
	}
}

void CanvasItemEditor::_draw_ruler_tool() {
	if (ci_manipulator->get_tool() != CanvasItemManipulator::TOOL_RULER) {
		return;
	}

	Point2 ruler_tool_origin = ci_manipulator->get_ruler_tool_origin();
	bool grid_snap_active = ci_manipulator->is_grid_snap_enabled();
	const Ref<Texture2D> position_icon = get_editor_theme_icon(SNAME("EditorPosition"));
	if (ci_manipulator->is_ruler_tool_active()) {
		const String &lang = _get_locale();
		const TranslationServer *ts = TranslationServer::get_singleton();

		Color ruler_primary_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		Color ruler_secondary_color = ruler_primary_color;
		ruler_secondary_color.a = 0.5;

		Point2 begin = (ruler_tool_origin - view_offset) * zoom;
		Point2 end = ci_manipulator->snap_point(viewport->get_local_mouse_position() / zoom + view_offset) * zoom - view_offset * zoom;
		Point2 corner = Point2(begin.x, end.y);
		Vector2 length_vector = (begin - end).abs() / zoom;

		const real_t horizontal_angle_rad = length_vector.angle();
		const real_t vertical_angle_rad = Math::PI / 2.0 - horizontal_angle_rad;

		Ref<Font> font = get_theme_font(SNAME("bold"), EditorStringName(EditorFonts));
		int font_size = 1.3 * get_theme_font_size(SNAME("bold_size"), EditorStringName(EditorFonts));
		Color font_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
		Color font_secondary_color = font_color;
		font_secondary_color.set_v(font_secondary_color.get_v() > 0.5 ? 0.7 : 0.3);
		Color outline_color = font_color.inverted();
		float text_height = font->get_height(font_size);

		const float outline_size = 4;
		const float text_width = 76;
		const float angle_text_width = 54;

		Point2 text_pos = (begin + end) / 2 - Vector2(text_width / 2, text_height / 2);
		text_pos.x = CLAMP(text_pos.x, text_width / 2, viewport->get_rect().size.x - text_width * 1.5);
		text_pos.y = CLAMP(text_pos.y, text_height * 1.5, viewport->get_rect().size.y - text_height * 1.5);

		// Draw lines.
		viewport->draw_line(begin, end, ruler_primary_color, Math::round(EDSCALE * 3));

		bool draw_secondary_lines = !(Math::is_equal_approx(begin.y, corner.y) || Math::is_equal_approx(end.x, corner.x));
		if (draw_secondary_lines) {
			viewport->draw_line(begin, corner, ruler_secondary_color, Math::round(EDSCALE));
			viewport->draw_line(corner, end, ruler_secondary_color, Math::round(EDSCALE));

			// Angle arcs.
			int arc_point_count = 8;
			real_t arc_radius_max_length_percent = 0.1;
			real_t ruler_length = length_vector.length() * zoom;
			real_t arc_max_radius = 50.0;
			real_t arc_line_width = 2.0;

			const Vector2 end_to_begin = (end - begin);

			real_t arc_1_start_angle = end_to_begin.x < 0
					? (end_to_begin.y < 0 ? 3.0 * Math::PI / 2.0 - vertical_angle_rad : Math::PI / 2.0)
					: (end_to_begin.y < 0 ? 3.0 * Math::PI / 2.0 : Math::PI / 2.0 - vertical_angle_rad);
			real_t arc_1_end_angle = arc_1_start_angle + vertical_angle_rad;
			// Constrain arc to triangle height & max size.
			real_t arc_1_radius = MIN(MIN(arc_radius_max_length_percent * ruler_length, Math::abs(end_to_begin.y)), arc_max_radius);

			real_t arc_2_start_angle = end_to_begin.x < 0
					? (end_to_begin.y < 0 ? 0.0 : -horizontal_angle_rad)
					: (end_to_begin.y < 0 ? Math::PI - horizontal_angle_rad : Math::PI);
			real_t arc_2_end_angle = arc_2_start_angle + horizontal_angle_rad;
			// Constrain arc to triangle width & max size.
			real_t arc_2_radius = MIN(MIN(arc_radius_max_length_percent * ruler_length, Math::abs(end_to_begin.x)), arc_max_radius);

			viewport->draw_arc(begin, arc_1_radius, arc_1_start_angle, arc_1_end_angle, arc_point_count, ruler_primary_color, Math::round(EDSCALE * arc_line_width));
			viewport->draw_arc(end, arc_2_radius, arc_2_start_angle, arc_2_end_angle, arc_point_count, ruler_primary_color, Math::round(EDSCALE * arc_line_width));
		}

		// Draw text.
		if (begin.is_equal_approx(end)) {
			viewport->draw_string_outline(font, text_pos, (String)ruler_tool_origin, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
			viewport->draw_string(font, text_pos, (String)ruler_tool_origin, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_color);
			viewport->draw_texture(position_icon, (ruler_tool_origin - view_offset) * zoom - position_icon->get_size() / 2);
			return;
		}

		viewport->draw_string_outline(font, text_pos, ts->format_number(vformat("%.1f px", length_vector.length()), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
		viewport->draw_string(font, text_pos, ts->format_number(vformat("%.1f px", length_vector.length()), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_color);

		if (draw_secondary_lines) {
			const int horizontal_angle = std::round(180 * horizontal_angle_rad / Math::PI);
			const int vertical_angle = std::round(180 * vertical_angle_rad / Math::PI);

			Point2 text_pos2 = text_pos;
			text_pos2.x = begin.x < text_pos.x ? MIN(text_pos.x - text_width, begin.x - text_width / 2) : MAX(text_pos.x + text_width, begin.x - text_width / 2);
			viewport->draw_string_outline(font, text_pos2, ts->format_number(vformat("%.1f px", length_vector.y), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
			viewport->draw_string(font, text_pos2, ts->format_number(vformat("%.1f px", length_vector.y), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);

			Point2 v_angle_text_pos;
			v_angle_text_pos.x = CLAMP(begin.x - angle_text_width / 2, angle_text_width / 2, viewport->get_rect().size.x - angle_text_width);
			v_angle_text_pos.y = begin.y < end.y ? MIN(text_pos2.y - 2 * text_height, begin.y - text_height * 0.5) : MAX(text_pos2.y + text_height * 3, begin.y + text_height * 1.5);
			viewport->draw_string_outline(font, v_angle_text_pos, ts->format_number(vformat(U"%d°", vertical_angle), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
			viewport->draw_string(font, v_angle_text_pos, ts->format_number(vformat(U"%d°", vertical_angle), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);

			text_pos2 = text_pos;
			text_pos2.y = end.y < text_pos.y ? MIN(text_pos.y - text_height * 2, end.y - text_height / 2) : MAX(text_pos.y + text_height * 2, end.y - text_height / 2);
			viewport->draw_string_outline(font, text_pos2, ts->format_number(vformat("%.1f px", length_vector.x), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
			viewport->draw_string(font, text_pos2, ts->format_number(vformat("%.1f px", length_vector.x), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);

			Point2 h_angle_text_pos;
			h_angle_text_pos.x = CLAMP(end.x - angle_text_width / 2, angle_text_width / 2, viewport->get_rect().size.x - angle_text_width);
			if (begin.y < end.y) {
				h_angle_text_pos.y = end.y + text_height * 1.5;
				if (Math::abs(text_pos2.x - h_angle_text_pos.x) < text_width) {
					int height_multiplier = 1.5 + (int)grid_snap_active;
					h_angle_text_pos.y = MAX(text_pos.y + height_multiplier * text_height, MAX(end.y + text_height * 1.5, text_pos2.y + height_multiplier * text_height));
				}
			} else {
				h_angle_text_pos.y = end.y - text_height * 0.5;
				if (Math::abs(text_pos2.x - h_angle_text_pos.x) < text_width) {
					int height_multiplier = 1 + (int)grid_snap_active;
					h_angle_text_pos.y = MIN(text_pos.y - height_multiplier * text_height, MIN(end.y - text_height * 0.5, text_pos2.y - height_multiplier * text_height));
				}
			}
			viewport->draw_string_outline(font, h_angle_text_pos, ts->format_number(vformat(U"%d°", horizontal_angle), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
			viewport->draw_string(font, h_angle_text_pos, ts->format_number(vformat(U"%d°", horizontal_angle), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);
		}

		if (grid_snap_active) {
			text_pos = (begin + end) / 2 + Vector2(-text_width / 2, text_height / 2);
			text_pos.x = CLAMP(text_pos.x, text_width / 2, viewport->get_rect().size.x - text_width * 1.5);
			text_pos.y = CLAMP(text_pos.y, text_height * 2.5, viewport->get_rect().size.y - text_height / 2);

			Point2 grid_step = ci_manipulator->get_grid_step();
			if (draw_secondary_lines) {
				viewport->draw_string_outline(font, text_pos, ts->format_number(vformat("%.2f " + TTR("units"), (length_vector / grid_step).length()), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
				viewport->draw_string(font, text_pos, ts->format_number(vformat("%.2f " + TTR("units"), (length_vector / grid_step).length()), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_color);

				Point2 text_pos2 = text_pos;
				text_pos2.x = begin.x < text_pos.x ? MIN(text_pos.x - text_width, begin.x - text_width / 2) : MAX(text_pos.x + text_width, begin.x - text_width / 2);
				viewport->draw_string_outline(font, text_pos2, ts->format_number(vformat("%d " + TTR("units"), std::round(length_vector.y / grid_step.y)), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
				viewport->draw_string(font, text_pos2, ts->format_number(vformat("%d " + TTR("units"), std::round(length_vector.y / grid_step.y)), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);

				text_pos2 = text_pos;
				text_pos2.y = end.y < text_pos.y ? MIN(text_pos.y - text_height * 2, end.y + text_height / 2) : MAX(text_pos.y + text_height * 2, end.y + text_height / 2);
				viewport->draw_string_outline(font, text_pos2, ts->format_number(vformat("%d " + TTR("units"), std::round(length_vector.x / grid_step.x)), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
				viewport->draw_string(font, text_pos2, ts->format_number(vformat("%d " + TTR("units"), std::round(length_vector.x / grid_step.x)), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_secondary_color);
			} else {
				viewport->draw_string_outline(font, text_pos, ts->format_number(vformat("%d " + TTR("units"), std::round((length_vector / grid_step).length())), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, outline_size, outline_color);
				viewport->draw_string(font, text_pos, ts->format_number(vformat("%d " + TTR("units"), std::round((length_vector / grid_step).length())), lang), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, font_color);
			}
		}
	} else if (grid_snap_active) {
		viewport->draw_texture(position_icon, (ruler_tool_origin - view_offset) * zoom - position_icon->get_size() / 2);
	}
}

void CanvasItemEditor::_draw_control_anchors(Control *control) {
	Transform2D xform = transform * control->get_screen_transform();
	RID ci = viewport->get_canvas_item();
	if (ci_manipulator->get_tool() == CanvasItemManipulator::TOOL_SELECT && !Object::cast_to<Container>(control->get_parent())) {
		// Compute the anchors.
		real_t anchors_values[4];
		anchors_values[0] = control->get_anchor(SIDE_LEFT);
		anchors_values[1] = control->get_anchor(SIDE_TOP);
		anchors_values[2] = control->get_anchor(SIDE_RIGHT);
		anchors_values[3] = control->get_anchor(SIDE_BOTTOM);

		Vector2 anchors_pos[4];
		for (int i = 0; i < 4; i++) {
			Vector2 value = Vector2((i % 2 == 0) ? anchors_values[i] : anchors_values[(i + 1) % 4], (i % 2 == 1) ? anchors_values[i] : anchors_values[(i + 1) % 4]);
			anchors_pos[i] = xform.xform(_anchor_to_position(control, value));
		}

		// Draw the anchors handles.
		Rect2 anchor_rects[4];
		if (control->is_layout_rtl()) {
			anchor_rects[0] = Rect2(anchors_pos[0] - Vector2(0.0, anchor_handle->get_size().y), Point2(-anchor_handle->get_size().x, anchor_handle->get_size().y));
			anchor_rects[1] = Rect2(anchors_pos[1] - anchor_handle->get_size(), anchor_handle->get_size());
			anchor_rects[2] = Rect2(anchors_pos[2] - Vector2(anchor_handle->get_size().x, 0.0), Point2(anchor_handle->get_size().x, -anchor_handle->get_size().y));
			anchor_rects[3] = Rect2(anchors_pos[3], -anchor_handle->get_size());
		} else {
			anchor_rects[0] = Rect2(anchors_pos[0] - anchor_handle->get_size(), anchor_handle->get_size());
			anchor_rects[1] = Rect2(anchors_pos[1] - Vector2(0.0, anchor_handle->get_size().y), Point2(-anchor_handle->get_size().x, anchor_handle->get_size().y));
			anchor_rects[2] = Rect2(anchors_pos[2], -anchor_handle->get_size());
			anchor_rects[3] = Rect2(anchors_pos[3] - Vector2(anchor_handle->get_size().x, 0.0), Point2(anchor_handle->get_size().x, -anchor_handle->get_size().y));
		}

		for (int i = 0; i < 4; i++) {
			anchor_handle->draw_rect(ci, anchor_rects[i]);
		}
	}
}

void CanvasItemEditor::_draw_control_helpers(Control *control) {
	Transform2D xform = transform * control->get_screen_transform();
	if (ci_manipulator->get_tool() == CanvasItemManipulator::TOOL_SELECT && show_helpers && !Object::cast_to<Container>(control->get_parent())) {
		// Draw the helpers.
		Color color_base = Color(0.8, 0.8, 0.8, 0.5);

		// Compute the anchors.
		real_t anchors_values[4];
		anchors_values[0] = control->get_anchor(SIDE_LEFT);
		anchors_values[1] = control->get_anchor(SIDE_TOP);
		anchors_values[2] = control->get_anchor(SIDE_RIGHT);
		anchors_values[3] = control->get_anchor(SIDE_BOTTOM);

		Vector2 anchors[4];
		Vector2 anchors_pos[4];
		for (int i = 0; i < 4; i++) {
			anchors[i] = Vector2((i % 2 == 0) ? anchors_values[i] : anchors_values[(i + 1) % 4], (i % 2 == 1) ? anchors_values[i] : anchors_values[(i + 1) % 4]);
			anchors_pos[i] = xform.xform(_anchor_to_position(control, anchors[i]));
		}

		CanvasItemManipulator::DragType drag_type = ci_manipulator->get_drag_type();

		// Get which anchor is dragged.
		int dragged_anchor = -1;
		switch (drag_type) {
			case CanvasItemManipulator::DRAG_ANCHOR_ALL:
			case CanvasItemManipulator::DRAG_ANCHOR_TOP_LEFT:
				dragged_anchor = 0;
				break;
			case CanvasItemManipulator::DRAG_ANCHOR_TOP_RIGHT:
				dragged_anchor = 1;
				break;
			case CanvasItemManipulator::DRAG_ANCHOR_BOTTOM_RIGHT:
				dragged_anchor = 2;
				break;
			case CanvasItemManipulator::DRAG_ANCHOR_BOTTOM_LEFT:
				dragged_anchor = 3;
				break;
			default:
				break;
		}

		if (dragged_anchor >= 0) {
			// Draw the 4 lines when dragged.
			Color color_snapped = Color(0.64, 0.93, 0.67, 0.5);

			Vector2 corners_pos[4];
			for (int i = 0; i < 4; i++) {
				corners_pos[i] = xform.xform(_anchor_to_position(control, Vector2((i == 0 || i == 3) ? ANCHOR_BEGIN : ANCHOR_END, (i <= 1) ? ANCHOR_BEGIN : ANCHOR_END)));
			}

			Vector2 line_starts[4];
			Vector2 line_ends[4];
			for (int i = 0; i < 4; i++) {
				real_t anchor_val = (i >= 2) ? (real_t)ANCHOR_END - anchors_values[i] : anchors_values[i];
				line_starts[i] = corners_pos[i].lerp(corners_pos[(i + 1) % 4], anchor_val);
				line_ends[i] = corners_pos[(i + 3) % 4].lerp(corners_pos[(i + 2) % 4], anchor_val);
				bool anchor_snapped = anchors_values[i] == 0.0 || anchors_values[i] == 0.5 || anchors_values[i] == 1.0;
				viewport->draw_line(line_starts[i], line_ends[i], anchor_snapped ? color_snapped : color_base, (i == dragged_anchor || (i + 3) % 4 == dragged_anchor) ? 2 : 1);
			}

			// Display the percentages next to the lines.
			real_t percent_val;
			percent_val = anchors_values[(dragged_anchor + 2) % 4] - anchors_values[dragged_anchor];
			percent_val = (dragged_anchor >= 2) ? -percent_val : percent_val;
			_draw_percentage_at_position(percent_val, (anchors_pos[dragged_anchor] + anchors_pos[(dragged_anchor + 1) % 4]) / 2, (Side)((dragged_anchor + 1) % 4));

			percent_val = anchors_values[(dragged_anchor + 3) % 4] - anchors_values[(dragged_anchor + 1) % 4];
			percent_val = ((dragged_anchor + 1) % 4 >= 2) ? -percent_val : percent_val;
			_draw_percentage_at_position(percent_val, (anchors_pos[dragged_anchor] + anchors_pos[(dragged_anchor + 3) % 4]) / 2, (Side)(dragged_anchor));

			percent_val = anchors_values[(dragged_anchor + 1) % 4];
			percent_val = ((dragged_anchor + 1) % 4 >= 2) ? (real_t)ANCHOR_END - percent_val : percent_val;
			_draw_percentage_at_position(percent_val, (line_starts[dragged_anchor] + anchors_pos[dragged_anchor]) / 2, (Side)(dragged_anchor));

			percent_val = anchors_values[dragged_anchor];
			percent_val = (dragged_anchor >= 2) ? (real_t)ANCHOR_END - percent_val : percent_val;
			_draw_percentage_at_position(percent_val, (line_ends[(dragged_anchor + 1) % 4] + anchors_pos[dragged_anchor]) / 2, (Side)((dragged_anchor + 1) % 4));
		}

		// Draw the margin values and the node width/height when dragging control side.
		const real_t ratio = 0.33;
		Transform2D parent_transform = xform * control->get_transform().affine_inverse();
		real_t node_pos_in_parent[4];

		Rect2 parent_rect = control->get_parent_anchorable_rect();

		node_pos_in_parent[1] = control->get_anchor(SIDE_TOP) * parent_rect.size.height + control->get_offset(SIDE_TOP) + parent_rect.position.y;
		node_pos_in_parent[3] = control->get_anchor(SIDE_BOTTOM) * parent_rect.size.height + control->get_offset(SIDE_BOTTOM) + parent_rect.position.y;

		bool rtl = control->is_layout_rtl();
		if (rtl) {
			node_pos_in_parent[0] = (parent_rect.size.width - control->get_anchor(SIDE_LEFT) * parent_rect.size.width) - control->get_offset(SIDE_LEFT) + parent_rect.position.x;
			node_pos_in_parent[2] = (parent_rect.size.width - control->get_anchor(SIDE_RIGHT) * parent_rect.size.width) - control->get_offset(SIDE_RIGHT) + parent_rect.position.x;
		} else {
			node_pos_in_parent[0] = control->get_anchor(SIDE_LEFT) * parent_rect.size.width + control->get_offset(SIDE_LEFT) + parent_rect.position.x;
			node_pos_in_parent[2] = control->get_anchor(SIDE_RIGHT) * parent_rect.size.width + control->get_offset(SIDE_RIGHT) + parent_rect.position.x;
		}

		Point2 start, end;
		switch (drag_type) {
			case CanvasItemManipulator::DRAG_LEFT:
			case CanvasItemManipulator::DRAG_TOP_LEFT:
			case CanvasItemManipulator::DRAG_BOTTOM_LEFT:
				_draw_margin_at_position(control->get_size().width, parent_transform.xform(Vector2((node_pos_in_parent[0] + node_pos_in_parent[2]) / 2, node_pos_in_parent[3])) + Vector2(0, 5), SIDE_BOTTOM);
				[[fallthrough]];
			case CanvasItemManipulator::DRAG_MOVE:
				if (rtl) {
					start = Vector2(node_pos_in_parent[2], Math::lerp(node_pos_in_parent[3], node_pos_in_parent[1], ratio));
					end = start + Vector2(control->get_offset(SIDE_RIGHT), 0);
					_draw_margin_at_position(control->get_offset(SIDE_RIGHT), parent_transform.xform((start + end) / 2), SIDE_BOTTOM);
				} else {
					start = Vector2(node_pos_in_parent[0], Math::lerp(node_pos_in_parent[1], node_pos_in_parent[3], ratio));
					end = start - Vector2(control->get_offset(SIDE_LEFT), 0);
					_draw_margin_at_position(control->get_offset(SIDE_LEFT), parent_transform.xform((start + end) / 2), SIDE_TOP);
				}
				viewport->draw_line(parent_transform.xform(start), parent_transform.xform(end), color_base, Math::round(EDSCALE));
				break;
			default:
				break;
		}
		switch (drag_type) {
			case CanvasItemManipulator::DRAG_RIGHT:
			case CanvasItemManipulator::DRAG_TOP_RIGHT:
			case CanvasItemManipulator::DRAG_BOTTOM_RIGHT:
				_draw_margin_at_position(control->get_size().width, parent_transform.xform(Vector2((node_pos_in_parent[0] + node_pos_in_parent[2]) / 2, node_pos_in_parent[3])) + Vector2(0, 5), SIDE_BOTTOM);
				[[fallthrough]];
			case CanvasItemManipulator::DRAG_MOVE:
				if (rtl) {
					start = Vector2(node_pos_in_parent[0], Math::lerp(node_pos_in_parent[1], node_pos_in_parent[3], ratio));
					end = start + Vector2(control->get_offset(SIDE_LEFT), 0);
					_draw_margin_at_position(control->get_offset(SIDE_LEFT), parent_transform.xform((start + end) / 2), SIDE_TOP);
				} else {
					start = Vector2(node_pos_in_parent[2], Math::lerp(node_pos_in_parent[3], node_pos_in_parent[1], ratio));
					end = start - Vector2(control->get_offset(SIDE_RIGHT), 0);
					_draw_margin_at_position(control->get_offset(SIDE_RIGHT), parent_transform.xform((start + end) / 2), SIDE_BOTTOM);
				}
				viewport->draw_line(parent_transform.xform(start), parent_transform.xform(end), color_base, Math::round(EDSCALE));
				break;
			default:
				break;
		}
		switch (drag_type) {
			case CanvasItemManipulator::DRAG_TOP:
			case CanvasItemManipulator::DRAG_TOP_LEFT:
			case CanvasItemManipulator::DRAG_TOP_RIGHT:
				_draw_margin_at_position(control->get_size().height, parent_transform.xform(Vector2(node_pos_in_parent[2], (node_pos_in_parent[1] + node_pos_in_parent[3]) / 2)) + Vector2(5, 0), SIDE_RIGHT);
				[[fallthrough]];
			case CanvasItemManipulator::DRAG_MOVE:
				start = Vector2(Math::lerp(node_pos_in_parent[0], node_pos_in_parent[2], ratio), node_pos_in_parent[1]);
				end = start - Vector2(0, control->get_offset(SIDE_TOP));
				_draw_margin_at_position(control->get_offset(SIDE_TOP), parent_transform.xform((start + end) / 2), SIDE_LEFT);
				viewport->draw_line(parent_transform.xform(start), parent_transform.xform(end), color_base, Math::round(EDSCALE));
				break;
			default:
				break;
		}
		switch (drag_type) {
			case CanvasItemManipulator::DRAG_BOTTOM:
			case CanvasItemManipulator::DRAG_BOTTOM_LEFT:
			case CanvasItemManipulator::DRAG_BOTTOM_RIGHT:
				_draw_margin_at_position(control->get_size().height, parent_transform.xform(Vector2(node_pos_in_parent[2], (node_pos_in_parent[1] + node_pos_in_parent[3]) / 2) + Vector2(5, 0)), SIDE_RIGHT);
				[[fallthrough]];
			case CanvasItemManipulator::DRAG_MOVE:
				start = Vector2(Math::lerp(node_pos_in_parent[2], node_pos_in_parent[0], ratio), node_pos_in_parent[3]);
				end = start - Vector2(0, control->get_offset(SIDE_BOTTOM));
				_draw_margin_at_position(control->get_offset(SIDE_BOTTOM), parent_transform.xform((start + end) / 2), SIDE_RIGHT);
				viewport->draw_line(parent_transform.xform(start), parent_transform.xform(end), color_base, Math::round(EDSCALE));
				break;
			default:
				break;
		}

		switch (drag_type) {
			// Draw the ghost rect if the node if rotated/scaled.
			case CanvasItemManipulator::DRAG_LEFT:
			case CanvasItemManipulator::DRAG_TOP_LEFT:
			case CanvasItemManipulator::DRAG_TOP:
			case CanvasItemManipulator::DRAG_TOP_RIGHT:
			case CanvasItemManipulator::DRAG_RIGHT:
			case CanvasItemManipulator::DRAG_BOTTOM_RIGHT:
			case CanvasItemManipulator::DRAG_BOTTOM:
			case CanvasItemManipulator::DRAG_BOTTOM_LEFT:
			case CanvasItemManipulator::DRAG_MOVE:
				if (control->get_rotation() != 0.0 || control->get_scale() != Vector2(1, 1)) {
					Rect2 rect = Rect2(Vector2(node_pos_in_parent[0], node_pos_in_parent[1]), control->get_size());
					viewport->draw_rect(parent_transform.xform(rect), color_base, false, Math::round(EDSCALE));
				}
				break;
			default:
				break;
		}
	}
}

void CanvasItemEditor::_draw_selection() {
	Ref<Texture2D> pivot_icon = get_editor_theme_icon(SNAME("EditorPivot"));
	Ref<Texture2D> position_icon = get_editor_theme_icon(SNAME("EditorPosition"));
	Ref<Texture2D> previous_position_icon = get_editor_theme_icon(SNAME("EditorPositionPrevious"));

	RID vp_ci = viewport->get_canvas_item();
	List<CanvasItem *> selection = _get_edited_canvas_items(true, false);
	bool single = selection.size() == 1;
	CanvasItemManipulator::Tool tool = ci_manipulator->get_tool();
	bool transform_tool = tool == CanvasItemManipulator::TOOL_SELECT || tool == CanvasItemManipulator::TOOL_MOVE || tool == CanvasItemManipulator::TOOL_SCALE || tool == CanvasItemManipulator::TOOL_ROTATE || tool == CanvasItemManipulator::TOOL_EDIT_PIVOT;
	CanvasItemManipulator::DragType drag_type = ci_manipulator->get_drag_type();
	bool use_local_space = ci_manipulator->is_local_space_enabled();

	for (CanvasItem *E : selection) {
		CanvasItem *ci = Object::cast_to<CanvasItem>(E);
		CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);

		// Draw the previous position if we are dragging the node
		if (show_helpers &&
				(drag_type == CanvasItemManipulator::DRAG_MOVE || drag_type == CanvasItemManipulator::DRAG_ROTATE ||
						drag_type == CanvasItemManipulator::DRAG_LEFT || drag_type == CanvasItemManipulator::DRAG_RIGHT || drag_type == CanvasItemManipulator::DRAG_TOP || drag_type == CanvasItemManipulator::DRAG_BOTTOM ||
						drag_type == CanvasItemManipulator::DRAG_TOP_LEFT || drag_type == CanvasItemManipulator::DRAG_TOP_RIGHT || drag_type == CanvasItemManipulator::DRAG_BOTTOM_LEFT || drag_type == CanvasItemManipulator::DRAG_BOTTOM_RIGHT)) {
			const Transform2D pre_drag_xform = transform * se->pre_drag_xform;
			const Color pre_drag_color = Color(0.4, 0.6, 1, 0.7);

			if (ci->_edit_use_rect()) {
				Vector2 pre_drag_endpoints[4] = {
					pre_drag_xform.xform(se->pre_drag_rect.position),
					pre_drag_xform.xform(se->pre_drag_rect.position + Vector2(se->pre_drag_rect.size.x, 0)),
					pre_drag_xform.xform(se->pre_drag_rect.position + se->pre_drag_rect.size),
					pre_drag_xform.xform(se->pre_drag_rect.position + Vector2(0, se->pre_drag_rect.size.y))
				};

				for (int i = 0; i < 4; i++) {
					viewport->draw_line(pre_drag_endpoints[i], pre_drag_endpoints[(i + 1) % 4], pre_drag_color, Math::round(2 * EDSCALE));
				}
			} else {
				viewport->draw_texture(previous_position_icon, (pre_drag_xform.xform(Point2()) - (previous_position_icon->get_size() / 2)).floor());
			}
		}

		bool item_locked = ci->has_meta("_edit_lock_");
		Transform2D xform = transform * ci->get_screen_transform();

		// Draw the selected items position / surrounding boxes
		if (ci->_edit_use_rect()) {
			Rect2 rect = ci->_edit_get_rect();
			const Vector2 endpoints[4] = {
				xform.xform(rect.position),
				xform.xform(rect.position + Vector2(rect.size.x, 0)),
				xform.xform(rect.position + rect.size),
				xform.xform(rect.position + Vector2(0, rect.size.y))
			};

			for (int i = 0; i < 4; i++) {
				viewport->draw_line(endpoints[i], endpoints[(i + 1) % 4], item_locked ? locked_selection_rectangle_color : selection_rectangle_color, Math::round(2 * EDSCALE));
			}
		} else {
			Transform2D unscaled_transform = (xform * ci->get_transform().affine_inverse() * ci->_edit_get_transform()).orthonormalized();
			Transform2D simple_xform;
			if (use_local_space) {
				simple_xform = viewport->get_transform() * unscaled_transform;
			} else {
				Transform2D translation = Transform2D(0.0f, unscaled_transform.get_origin());
				simple_xform = viewport->get_transform() * translation;
			}

			viewport->draw_set_transform_matrix(simple_xform);
			viewport->draw_texture(position_icon, -(position_icon->get_size() / 2));
			viewport->draw_set_transform_matrix(viewport->get_transform());
		}

		if (single && !item_locked && transform_tool) {
			// Draw the pivot
			if (ci->_edit_use_pivot()) {
				// Draw the node's pivot
				Transform2D unscaled_transform = (xform * ci->get_transform().affine_inverse() * ci->_edit_get_transform()).orthonormalized();
				Transform2D simple_xform;
				if (use_local_space) {
					simple_xform = viewport->get_transform() * unscaled_transform;
				} else {
					Transform2D translation = Transform2D(0.0f, unscaled_transform.get_origin());
					simple_xform = viewport->get_transform() * translation;
				}

				viewport->draw_set_transform_matrix(simple_xform);
				viewport->draw_texture(pivot_icon, -(pivot_icon->get_size() / 2).floor());
				viewport->draw_set_transform_matrix(viewport->get_transform());
			}

			// Draw control-related helpers
			Control *control = Object::cast_to<Control>(ci);
			if (control && ci_manipulator->is_node_movable(control)) {
				_draw_control_anchors(control);
				_draw_control_helpers(control);
			}

			// Draw the resize handles
			if (tool == CanvasItemManipulator::TOOL_SELECT && ci->_edit_use_rect() && ci_manipulator->is_node_movable(ci)) {
				Rect2 rect = ci->_edit_get_rect();
				const Vector2 endpoints[4] = {
					xform.xform(rect.position),
					xform.xform(rect.position + Vector2(rect.size.x, 0)),
					xform.xform(rect.position + rect.size),
					xform.xform(rect.position + Vector2(0, rect.size.y))
				};
				for (int i = 0; i < 4; i++) {
					int prev = (i + 3) % 4;
					int next = (i + 1) % 4;

					Vector2 ofs = ((endpoints[i] - endpoints[prev]).normalized() + ((endpoints[i] - endpoints[next]).normalized())).normalized();
					ofs *= Math::SQRT2 * (select_handle->get_size().width / 2);

					select_handle->draw(vp_ci, (endpoints[i] + ofs - (select_handle->get_size() / 2)).floor());

					ofs = (endpoints[i] + endpoints[next]) / 2;
					ofs += (endpoints[next] - endpoints[i]).orthogonal().normalized() * (select_handle->get_size().width / 2);

					select_handle->draw(vp_ci, (ofs - (select_handle->get_size() / 2)).floor());
				}
			}
		}
	}

	// Remove non-movable nodes.
	for (List<CanvasItem *>::Element *E = selection.front(); E;) {
		List<CanvasItem *>::Element *N = E->next();
		if (!ci_manipulator->is_node_movable(E->get())) {
			selection.erase(E);
		}
		E = N;
	}

	if (!selection.is_empty() && transform_tool && ci_manipulator->is_showing_transformation_gizmos()) {
		CanvasItem *ci = selection.front()->get();

		Transform2D xform = transform * ci->get_screen_transform();
		bool is_ctrl = Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL);
		bool is_alt = Input::get_singleton()->is_key_pressed(Key::ALT);

		// Draw the move handles.
		if ((tool == CanvasItemManipulator::TOOL_SELECT && is_alt && !is_ctrl) || tool == CanvasItemManipulator::TOOL_MOVE) {
			Transform2D unscaled_transform = (xform * ci->get_transform().affine_inverse() * ci->_edit_get_transform()).orthonormalized();
			Transform2D simple_xform;
			if (use_local_space) {
				simple_xform = viewport->get_transform() * unscaled_transform;
			} else {
				Transform2D translation = Transform2D(0.0f, unscaled_transform.get_origin());
				simple_xform = viewport->get_transform() * translation;
			}

			Size2 move_factor = Size2(CanvasItemManipulator::GIZMO_HANDLE_DISTANCE, CanvasItemManipulator::GIZMO_HANDLE_DISTANCE);
			viewport->draw_set_transform_matrix(simple_xform);

			Vector<Point2> points = {
				Vector2(move_factor.x * EDSCALE, 5 * EDSCALE),
				Vector2(move_factor.x * EDSCALE, -5 * EDSCALE),
				Vector2((move_factor.x + 10) * EDSCALE, 0)
			};

			viewport->draw_colored_polygon(points, get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)));
			viewport->draw_line(Point2(), Point2(move_factor.x * EDSCALE, 0), get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)), Math::round(EDSCALE));

			points.clear();
			points.push_back(Vector2(5 * EDSCALE, move_factor.y * EDSCALE));
			points.push_back(Vector2(-5 * EDSCALE, move_factor.y * EDSCALE));
			points.push_back(Vector2(0, (move_factor.y + 10) * EDSCALE));

			viewport->draw_colored_polygon(points, get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)));
			viewport->draw_line(Point2(), Point2(0, move_factor.y * EDSCALE), get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)), Math::round(EDSCALE));

			viewport->draw_set_transform_matrix(viewport->get_transform());
		}

		// Draw the rescale handles.
		if ((tool == CanvasItemManipulator::TOOL_SELECT && is_alt && is_ctrl) || tool == CanvasItemManipulator::TOOL_SCALE || drag_type == CanvasItemManipulator::DRAG_SCALE_X || drag_type == CanvasItemManipulator::DRAG_SCALE_Y) {
			Transform2D edit_transform;
			Point2 temp_pivot = ci_manipulator->get_temp_pivot();
			if (!Math::is_inf(temp_pivot.x) || !Math::is_inf(temp_pivot.y)) {
				edit_transform = Transform2D(ci->_edit_get_rotation(), temp_pivot);
			} else {
				edit_transform = ci->_edit_get_transform();
			}
			Transform2D unscaled_transform = (xform * ci->get_transform().affine_inverse() * edit_transform).orthonormalized();
			Transform2D simple_xform;
			if (use_local_space) {
				simple_xform = viewport->get_transform() * unscaled_transform;
			} else {
				Transform2D translation = Transform2D(0.0f, unscaled_transform.get_origin());
				simple_xform = viewport->get_transform() * translation;
			}

			Size2 scale_factor = Size2(CanvasItemManipulator::GIZMO_HANDLE_DISTANCE, CanvasItemManipulator::GIZMO_HANDLE_DISTANCE);
			bool uniform = Input::get_singleton()->is_key_pressed(Key::SHIFT);
			Point2 offset = (simple_xform.affine_inverse().xform(ci_manipulator->get_drag_to()) - simple_xform.affine_inverse().xform(ci_manipulator->get_drag_from())) * zoom;

			if (drag_type == CanvasItemManipulator::DRAG_SCALE_X) {
				scale_factor.x += offset.x;
				if (uniform) {
					scale_factor.y += offset.x;
				}
			} else if (drag_type == CanvasItemManipulator::DRAG_SCALE_Y) {
				scale_factor.y += offset.y;
				if (uniform) {
					scale_factor.x += offset.y;
				}
			}

			viewport->draw_set_transform_matrix(simple_xform);
			Rect2 x_handle_rect = Rect2(scale_factor.x * EDSCALE, -5 * EDSCALE, 10 * EDSCALE, 10 * EDSCALE);
			viewport->draw_rect(x_handle_rect, get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)));
			viewport->draw_line(Point2(), Point2(scale_factor.x * EDSCALE, 0), get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)), Math::round(EDSCALE));

			Rect2 y_handle_rect = Rect2(-5 * EDSCALE, scale_factor.y * EDSCALE, 10 * EDSCALE, 10 * EDSCALE);
			viewport->draw_rect(y_handle_rect, get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)));
			viewport->draw_line(Point2(), Point2(0, scale_factor.y * EDSCALE), get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)), Math::round(EDSCALE));

			viewport->draw_set_transform_matrix(viewport->get_transform());
		}
	}

	if (drag_type == CanvasItemManipulator::DRAG_BOX_SELECTION) {
		// Draw the dragging box.
		viewport->draw_rect(
				box_selection_rect,
				get_theme_color(SNAME("box_selection_fill_color"), EditorStringName(Editor)));

		viewport->draw_rect(
				box_selection_rect,
				get_theme_color(SNAME("box_selection_stroke_color"), EditorStringName(Editor)),
				false,
				Math::round(EDSCALE));
	}

	if (drag_type == CanvasItemManipulator::DRAG_ROTATE) {
		// Draw the line when rotating a node
		viewport->draw_line(
				transform.xform(ci_manipulator->get_drag_rotation_center()),
				transform.xform(ci_manipulator->get_drag_to()),
				get_theme_color(SNAME("accent_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.6),
				Math::round(2 * EDSCALE));
	}

	Point2 temp_pivot = ci_manipulator->get_temp_pivot();
	if (!Math::is_inf(temp_pivot.x) || !Math::is_inf(temp_pivot.y)) {
		viewport->draw_texture(pivot_icon, (temp_pivot - view_offset) * zoom - (pivot_icon->get_size() / 2).floor(), get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
	}
}

void CanvasItemEditor::_draw_straight_line(Point2 p_from, Point2 p_to, Color p_color) {
	// Draw a line going through the whole screen from a vector
	RID ci = viewport->get_canvas_item();
	Vector<Point2> points;
	Point2 from = transform.xform(p_from);
	Point2 to = transform.xform(p_to);
	Size2 viewport_size = viewport->get_size();

	if (to.x == from.x) {
		// Vertical line
		points.push_back(Point2(to.x, 0));
		points.push_back(Point2(to.x, viewport_size.y));
	} else if (to.y == from.y) {
		// Horizontal line
		points.push_back(Point2(0, to.y));
		points.push_back(Point2(viewport_size.x, to.y));
	} else {
		real_t y_for_zero_x = (to.y * from.x - from.y * to.x) / (from.x - to.x);
		real_t x_for_zero_y = (to.x * from.y - from.x * to.y) / (from.y - to.y);
		real_t y_for_viewport_x = ((to.y - from.y) * (viewport_size.x - from.x)) / (to.x - from.x) + from.y;
		real_t x_for_viewport_y = ((to.x - from.x) * (viewport_size.y - from.y)) / (to.y - from.y) + from.x; // faux

		//bool start_set = false;
		if (y_for_zero_x >= 0 && y_for_zero_x <= viewport_size.y) {
			points.push_back(Point2(0, y_for_zero_x));
		}
		if (x_for_zero_y >= 0 && x_for_zero_y <= viewport_size.x) {
			points.push_back(Point2(x_for_zero_y, 0));
		}
		if (y_for_viewport_x >= 0 && y_for_viewport_x <= viewport_size.y) {
			points.push_back(Point2(viewport_size.x, y_for_viewport_x));
		}
		if (x_for_viewport_y >= 0 && x_for_viewport_y <= viewport_size.x) {
			points.push_back(Point2(x_for_viewport_y, viewport_size.y));
		}
	}
	if (points.size() >= 2) {
		RenderingServer::get_singleton()->canvas_item_add_line(ci, points[0], points[1], p_color);
	}
}

void CanvasItemEditor::_draw_axis() {
	if (show_origin) {
		_draw_straight_line(Point2(), Point2(1, 0), get_theme_color(SNAME("axis_x_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.75));
		_draw_straight_line(Point2(), Point2(0, 1), get_theme_color(SNAME("axis_y_color"), EditorStringName(Editor)) * Color(1, 1, 1, 0.75));
	}

	if (show_viewport) {
		RID ci = viewport->get_canvas_item();

		Color area_axis_color = EDITOR_GET("editors/2d/viewport_border_color");

		Size2 screen_size = Size2(GLOBAL_GET("display/window/size/viewport_width"), GLOBAL_GET("display/window/size/viewport_height"));

		Vector2 screen_endpoints[4] = {
			transform.xform(Vector2(0, 0)),
			transform.xform(Vector2(screen_size.width, 0)),
			transform.xform(Vector2(screen_size.width, screen_size.height)),
			transform.xform(Vector2(0, screen_size.height))
		};

		for (int i = 0; i < 4; i++) {
			RenderingServer::get_singleton()->canvas_item_add_line(ci, screen_endpoints[i], screen_endpoints[(i + 1) % 4], area_axis_color);
		}
	}
}

void CanvasItemEditor::_draw_invisible_nodes_positions(Node *p_node, const Transform2D &p_parent_xform, const Transform2D &p_canvas_xform) {
	ERR_FAIL_NULL(p_node);

	Node *scene = EditorNode::get_singleton()->get_edited_scene();
	if (p_node != scene && p_node->get_owner() != scene && !scene->is_editable_instance(p_node->get_owner())) {
		return;
	}
	CanvasItem *ci = Object::cast_to<CanvasItem>(p_node);
	if (ci && !ci->is_visible_in_tree()) {
		return;
	}

	Transform2D parent_xform = p_parent_xform;
	Transform2D canvas_xform = p_canvas_xform;

	if (ci && !ci->is_set_as_top_level()) {
		parent_xform = parent_xform * ci->get_transform();
	} else if (CanvasLayer *cl = Object::cast_to<CanvasLayer>(p_node)) {
		parent_xform = Transform2D();
		canvas_xform = cl->get_transform();
	} else if (Viewport *vp = Object::cast_to<Viewport>(p_node)) {
		if (!vp->is_visible_subviewport()) {
			return;
		}
		parent_xform = Transform2D();
		canvas_xform = vp->get_popup_base_transform();
	}

	for (int i = p_node->get_child_count() - 1; i >= 0; i--) {
		_draw_invisible_nodes_positions(p_node->get_child(i), parent_xform, canvas_xform);
	}

	if (show_position_gizmos && ci && !ci->_edit_use_rect() && (!editor_selection->is_selected(ci) || CanvasItemManipulator::is_node_locked(ci))) {
		Transform2D xform = transform * canvas_xform * parent_xform;

		// Draw the node's position
		Ref<Texture2D> position_icon = get_editor_theme_icon(SNAME("EditorPositionUnselected"));
		Transform2D unscaled_transform = (xform * ci->get_transform().affine_inverse() * ci->_edit_get_transform()).orthonormalized();
		Transform2D simple_xform;
		if (ci_manipulator->is_local_space_enabled()) {
			simple_xform = viewport->get_transform() * unscaled_transform;
		} else {
			Transform2D translation = Transform2D(0.0f, unscaled_transform.get_origin());
			simple_xform = viewport->get_transform() * translation;
		}

		viewport->draw_set_transform_matrix(simple_xform);
		viewport->draw_texture(position_icon, -position_icon->get_size() / 2, Color(1.0, 1.0, 1.0, 0.5));
		viewport->draw_set_transform_matrix(viewport->get_transform());
	}
}

void CanvasItemEditor::_draw_hover() {
	List<Rect2> previous_rects;
	Vector2 icon_size = Vector2(1, 1) * get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

	for (int i = 0; i < hovering_results.size(); i++) {
		Ref<Texture2D> node_icon = hovering_results[i].icon;
		String node_name = hovering_results[i].name;

		Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
		Size2 node_name_size = font->get_string_size(node_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
		Size2 item_size = Size2(icon_size.x + 4 + node_name_size.x, MAX(icon_size.y, node_name_size.y - 3));

		Point2 pos = transform.xform(hovering_results[i].position) - Point2(0, item_size.y) + (Point2(icon_size.x, -icon_size.y) / 4);
		// Rectify the position to avoid overlapping items
		for (const Rect2 &E : previous_rects) {
			if (E.intersects(Rect2(pos, item_size))) {
				pos.y = E.get_position().y - item_size.y;
			}
		}

		previous_rects.push_back(Rect2(pos, item_size));

		// Draw icon
		viewport->draw_texture_rect(node_icon, Rect2(pos, icon_size), false, Color(1.0, 1.0, 1.0, 0.5));

		// Draw name
		viewport->draw_string(font, pos + Point2(icon_size.x + 4, item_size.y - 3), node_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1.0, 1.0, 1.0, 0.5));
	}
}

void CanvasItemEditor::_draw_message() {
	CanvasItemManipulator::DragType drag_type = ci_manipulator->get_drag_type();

	if (drag_type != CanvasItemManipulator::DRAG_NONE) {
		TypedArray<CanvasItem> drag_selection = ci_manipulator->get_drag_selection();

		if (!drag_selection.is_empty() && drag_selection.front()) {
			Transform2D current_transform = Object::cast_to<CanvasItem>(drag_selection.front())->get_global_transform();
			bool use_local_space = ci_manipulator->is_local_space_enabled();

			double snap = EDITOR_GET("interface/inspector/default_float_step");
			int snap_step_decimals = Math::range_step_decimals(snap);
			const String &lang = _get_locale();
#define FORMAT(value) (TranslationServer::get_singleton()->format_number(String::num(value, snap_step_decimals), lang))

			switch (drag_type) {
				case CanvasItemManipulator::DRAG_MOVE:
				case CanvasItemManipulator::DRAG_MOVE_X:
				case CanvasItemManipulator::DRAG_MOVE_Y: {
					Vector2 delta = current_transform.get_origin() - original_transform.get_origin();
					if (drag_type == CanvasItemManipulator::DRAG_MOVE || use_local_space) {
						message = TTR("Moving:") + " (" + FORMAT(delta.x) + ", " + FORMAT(delta.y) + ") px";
					} else if (drag_type == CanvasItemManipulator::DRAG_MOVE_X) {
						message = TTR("Moving:") + " " + FORMAT(delta.x) + " px";
					} else if (drag_type == CanvasItemManipulator::DRAG_MOVE_Y) {
						message = TTR("Moving:") + " " + FORMAT(delta.y) + " px";
					}
				} break;

				case CanvasItemManipulator::DRAG_ROTATE: {
					real_t delta = Math::rad_to_deg(current_transform.get_rotation() - original_transform.get_rotation());
					message = TTR("Rotating:") + " " + FORMAT(delta) + String::utf8(" °");
				} break;

				case CanvasItemManipulator::DRAG_SCALE_X:
				case CanvasItemManipulator::DRAG_SCALE_Y:
				case CanvasItemManipulator::DRAG_SCALE_BOTH: {
					Vector2 original_scale = (Math::is_zero_approx(original_transform.get_scale().x) || Math::is_zero_approx(original_transform.get_scale().y)) ? Vector2(CMP_EPSILON, CMP_EPSILON) : original_transform.get_scale();
					Vector2 delta = current_transform.get_scale() / original_scale;
					if (drag_type == CanvasItemManipulator::DRAG_SCALE_BOTH || !use_local_space) {
						message = TTR("Scaling:") + String::utf8(" ×(") + FORMAT(delta.x) + ", " + FORMAT(delta.y) + ")";
					} else if (drag_type == CanvasItemManipulator::DRAG_SCALE_X) {
						message = TTR("Scaling:") + String::utf8(" ×") + FORMAT(delta.x);
					} else if (drag_type == CanvasItemManipulator::DRAG_SCALE_Y) {
						message = TTR("Scaling:") + String::utf8(" ×") + FORMAT(delta.y);
					}
				} break;

				default:
					break;
			}
		}
#undef FORMAT
	}

	if (message.is_empty()) {
		return;
	}

	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));
	Point2 msgpos = Point2(ci_manipulator->get_ruler_width() + 10 * EDSCALE, viewport->get_size().y - 14 * EDSCALE);
	viewport->draw_string(font, msgpos + Point2(1, 1), message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0, 0, 0, 0.8));
	viewport->draw_string(font, msgpos + Point2(-1, -1), message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(0, 0, 0, 0.8));
	viewport->draw_string(font, msgpos, message, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1, 1));
}

void CanvasItemEditor::_draw_locks_and_groups(Node *p_node, const Transform2D &p_parent_xform, const Transform2D &p_canvas_xform) {
	ERR_FAIL_NULL(p_node);

	Node *scene = EditorNode::get_singleton()->get_edited_scene();
	if (p_node != scene && p_node->get_owner() != scene && !scene->is_editable_instance(p_node->get_owner())) {
		return;
	}
	CanvasItem *ci = Object::cast_to<CanvasItem>(p_node);
	if (ci && !ci->is_visible_in_tree()) {
		return;
	}

	Transform2D parent_xform = p_parent_xform;
	Transform2D canvas_xform = p_canvas_xform;

	if (ci && !ci->is_set_as_top_level()) {
		parent_xform = parent_xform * ci->get_transform();
	} else if (CanvasLayer *cl = Object::cast_to<CanvasLayer>(p_node)) {
		parent_xform = Transform2D();
		canvas_xform = cl->get_transform();
	} else if (Viewport *vp = Object::cast_to<Viewport>(p_node)) {
		if (!vp->is_visible_subviewport()) {
			return;
		}
		parent_xform = Transform2D();
		canvas_xform = vp->get_popup_base_transform();
	}

	for (int i = p_node->get_child_count() - 1; i >= 0; i--) {
		_draw_locks_and_groups(p_node->get_child(i), parent_xform, canvas_xform);
	}

	RID viewport_ci = viewport->get_canvas_item();
	if (ci) {
		real_t offset = 0;

		Ref<Texture2D> lock = get_editor_theme_icon(SNAME("LockViewport"));
		if (show_lock_gizmos && p_node->has_meta("_edit_lock_")) {
			lock->draw(viewport_ci, (transform * canvas_xform * parent_xform).xform(Point2(0, 0)) + Point2(offset, 0));
			offset += lock->get_size().x;
		}

		Ref<Texture2D> group = get_editor_theme_icon(SNAME("GroupViewport"));
		if (show_group_gizmos && ci->has_meta("_edit_group_")) {
			group->draw(viewport_ci, (transform * canvas_xform * parent_xform).xform(Point2(0, 0)) + Point2(offset, 0));
			//offset += group->get_size().x;
		}
	}
}

void CanvasItemEditor::_draw_viewport() {
	// Update the transform
	transform = Transform2D();
	transform.scale_basis(Size2(zoom, zoom));
	transform.columns[2] = -view_offset * zoom;
	EditorNode::get_singleton()->get_scene_root()->set_global_canvas_transform(transform);
	RenderingServer::get_singleton()->viewport_set_disable_2d(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), false);

	_draw_grid();
	_draw_ruler_tool();
	_draw_axis();
	if (EditorNode::get_singleton()->get_edited_scene()) {
		_draw_locks_and_groups(EditorNode::get_singleton()->get_edited_scene());
		_draw_invisible_nodes_positions(EditorNode::get_singleton()->get_edited_scene());
	}
	_draw_selection();

	RID ci = viewport->get_canvas_item();
	RenderingServer::get_singleton()->canvas_item_add_set_transform(ci, Transform2D());

	EditorNode::get_singleton()->get_editor_plugins_over()->forward_canvas_draw_over_viewport(viewport);
	EditorNode::get_singleton()->get_editor_plugins_force_over()->forward_canvas_force_draw_over_viewport(viewport);

	if (ci_manipulator->is_showing_rulers()) {
		_draw_rulers();
	}
	if (ci_manipulator->is_showing_guides()) {
		_draw_guides();
	}
	_draw_smart_snapping();
	_draw_focus();
	_draw_hover();
	_draw_message();

	if (clockgears_workspace) {
		_draw_clockgears_map_overlays();
	}
	if (clockgears_event_mode) {
		_draw_clockgears_event_grid();
	}
}

void CanvasItemEditor::update_viewport() {
	_update_scrollbars();
	viewport->queue_redraw();
}

void CanvasItemEditor::set_current_tool(CanvasItemManipulator::Tool p_tool) {
	_button_tool_select(p_tool);
}

void CanvasItemEditor::_update_editor_settings() {
	button_center_view->set_button_icon(get_editor_theme_icon(SNAME("CenterView")));
	select_button->set_button_icon(get_editor_theme_icon(SNAME("ToolSelect")));
	scene_paint_button->set_button_icon(get_editor_theme_icon(SNAME("Paint")));
	select_sb->set_texture(get_editor_theme_icon(SNAME("EditorRect2D")));
	list_select_button->set_button_icon(get_editor_theme_icon(SNAME("ListSelect")));
	move_button->set_button_icon(get_editor_theme_icon(SNAME("ToolMove")));
	scale_button->set_button_icon(get_editor_theme_icon(SNAME("ToolScale")));
	rotate_button->set_button_icon(get_editor_theme_icon(SNAME("ToolRotate")));
	local_space_button->set_button_icon(get_editor_theme_icon(SNAME("Object")));
	smart_snap_button->set_button_icon(get_editor_theme_icon(SNAME("Snap")));
	grid_snap_button->set_button_icon(get_editor_theme_icon(SNAME("SnapGrid")));
	snap_config_menu->set_button_icon(get_editor_theme_icon(SNAME("GuiDropdown")));
	skeleton_menu->set_button_icon(get_editor_theme_icon(SNAME("Bone")));
	pan_button->set_button_icon(get_editor_theme_icon(SNAME("ToolPan")));
	ruler_button->set_button_icon(get_editor_theme_icon(SNAME("Ruler")));
	pivot_button->set_button_icon(get_editor_theme_icon(SNAME("EditPivot")));
	select_handle = get_editor_theme_icon(SNAME("EditorHandle"));
	anchor_handle = get_editor_theme_icon(SNAME("EditorControlAnchor"));
	lock_button->set_button_icon(get_editor_theme_icon(SNAME("Lock")));
	unlock_button->set_button_icon(get_editor_theme_icon(SNAME("Unlock")));
	group_button->set_button_icon(get_editor_theme_icon(SNAME("Group")));
	ungroup_button->set_button_icon(get_editor_theme_icon(SNAME("Ungroup")));
	key_loc_button->set_button_icon(get_editor_theme_icon(SNAME("KeyPosition")));
	key_rot_button->set_button_icon(get_editor_theme_icon(SNAME("KeyRotation")));
	key_scale_button->set_button_icon(get_editor_theme_icon(SNAME("KeyScale")));
	key_insert_button->set_button_icon(get_editor_theme_icon(SNAME("Key")));
	key_auto_insert_button->set_button_icon(get_editor_theme_icon(SNAME("AutoKey")));
	// Use a different color for the active autokey icon to make them easier
	// to distinguish from the other key icons at the top. On a light theme,
	// the icon will be dark, so we need to lighten it before blending it
	// with the red color.
	const Color key_auto_color = EditorThemeManager::is_dark_icon_and_font() ? Color(1, 1, 1) : Color(4.25, 4.25, 4.25);
	key_auto_insert_button->add_theme_color_override("icon_pressed_color", key_auto_color.lerp(Color(1, 0, 0), 0.55));
	animation_menu->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));

	selection_rectangle_color = EDITOR_GET("editors/2d/selection_rectangle_color");
	locked_selection_rectangle_color = EDITOR_GET("editors/2d/locked_selection_rectangle_color");

	context_toolbar_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("ContextualToolbar"), EditorStringName(EditorStyles)));

	simple_panning = EDITOR_GET("editors/panning/simple_panning");
	panner->setup((ViewPanner::ControlScheme)EDITOR_GET("editors/panning/2d_editor_panning_scheme").operator int(), ED_GET_SHORTCUT("canvas_item_editor/pan_view"), simple_panning);
	panner->set_scroll_speed(EDITOR_GET("editors/panning/2d_editor_pan_speed"));
	panner->setup_warped_panning(this, EDITOR_GET("editors/panning/warped_mouse_panning"));
	panner->set_zoom_style((ViewPanner::ZoomStyle)EDITOR_GET("editors/panning/zoom_style").operator int());

	// Compute the ruler width here so we can reuse the result throughout the various draw functions.
	real_t ruler_width_unscaled = EDITOR_GET("editors/2d/ruler_width");
	ruler_font_size = MAX(get_theme_font_size(SNAME("rulers_size"), EditorStringName(EditorFonts)) * ruler_width_unscaled / 15.0, 8);
	ci_manipulator->set_ruler_width(MAX(ruler_width_unscaled * EDSCALE, ruler_font_size * 2.0));

	ci_manipulator->set_grab_distance(EDITOR_GET("editors/polygon_editor/point_grab_radius"));
	ci_manipulator->set_shortcut(CanvasItemManipulator::SHORTCUT_CANCEL_TRANSFORM, ED_GET_SHORTCUT("canvas_item_editor/cancel_transform"));

	resample_delay = EDITOR_GET("editors/2d/auto_resample_delay");
	resample_timer->set_wait_time(resample_delay);
}

void CanvasItemEditor::_project_settings_changed() {
	EditorNode::get_singleton()->get_scene_root()->set_snap_controls_to_pixels(GLOBAL_GET("gui/common/snap_controls_to_pixels"));
}

void CanvasItemEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED: {
			const String show_list_tooltip = vformat(TTR("%s+RMB: Show list of all nodes at position clicked, including locked."), keycode_get_string((Key)KeyModifierMask::ALT));
			select_button->set_tooltip_text(vformat(TTR("%s+Drag: Rotate selected node around pivot."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)) + "\n" + vformat(TTR("%s+Drag: Move selected node."), keycode_get_string((Key)KeyModifierMask::ALT)) + "\n" + vformat(TTR("%s+%s+Drag: Scale selected node."), keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL), keycode_get_string((Key)KeyModifierMask::ALT)) + "\n" + TTR("V: Set selected node's pivot position.") + "\n" + show_list_tooltip + "\n" + TTR("RMB: Add node at position clicked."));
			move_button->set_tooltip_text(show_list_tooltip);
			rotate_button->set_tooltip_text(show_list_tooltip);
			scale_button->set_tooltip_text(TTR("Shift: Scale proportionally.") + "\n" + show_list_tooltip);
			list_select_button->set_tooltip_text(TTR("Show list of selectable nodes at position clicked.") + "\n" + show_list_tooltip);
			pivot_button->set_tooltip_text(TTR("Click to change object's pivot.") + "\n" + TTR("Shift: Set temporary pivot.") + "\n" + TTR("Click this button while holding Shift to put the temporary pivot in the center of the selected nodes.") + "\n" + show_list_tooltip);
			pan_button->set_tooltip_text(TTR("You can also use Pan View shortcut (Space by default) to pan in any mode.") + "\n" + show_list_tooltip);
			ruler_button->set_tooltip_text(TTR("LMB+Drag: Measure the distance between two points in 2D space.") + "\n" + show_list_tooltip);
		} break;

		case NOTIFICATION_READY: {
			select_sb->set_texture(get_editor_theme_icon(SNAME("EditorRect2D")));
			select_sb->set_texture_margin_all(4);
			select_sb->set_content_margin_all(4);

			AnimationPlayerEditor::get_singleton()->get_track_editor()->connect("keying_changed", callable_mp(this, &CanvasItemEditor::_keying_changed));
			AnimationPlayerEditor::get_singleton()->connect("animation_selected", callable_mp(this, &CanvasItemEditor::_keying_changed).unbind(1));
			_update_editor_settings();

			ci_manipulator->set_viewport(get_viewport());
			ci_manipulator->set_callbacks(
					callable_mp(EditorNode::get_singleton(), &EditorNode::get_edited_scene),
					callable_mp(this, &CanvasItemEditor::_select_click_on_item),
					callable_mp(this, &CanvasItemEditor::_get_selection_callback),
					callable_mp(viewport_scrollable, &Control::get_transform),
					callable_mp((Viewport *)EditorNode::get_singleton()->get_scene_root(), &Viewport::get_global_canvas_transform),
					callable_mp((CanvasItem *)viewport, &CanvasItem::get_local_mouse_position),
					callable_mp(EditorNode::get_singleton()->get_editor_plugins_over(), &EditorPluginList::forward_gui_input));

			ControlEditorToolbar::get_singleton()->connect("anchors_mode_toggled", callable_mp(*ci_manipulator, &CanvasItemManipulator::set_anchors_mode_enabled));

			connect("item_lock_status_changed", callable_mp(this, &CanvasItemEditor::_update_lock_and_group_button));
			connect("item_group_status_changed", callable_mp(this, &CanvasItemEditor::_update_lock_and_group_button));

			_update_lock_and_group_button();

			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &CanvasItemEditor::_project_settings_changed));
		} break;

		case NOTIFICATION_PROCESS: {
			// Update the viewport if the canvas_item changes
			List<CanvasItem *> selection = _get_edited_canvas_items(true);
			for (CanvasItem *ci : selection) {
				CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);

				Rect2 rect;
				if (ci->_edit_use_rect()) {
					rect = ci->_edit_get_rect();
				} else {
					rect = Rect2();
				}
				Transform2D xform = ci->get_global_transform();

				if (rect != se->prev_rect || xform != se->prev_xform) {
					viewport->queue_redraw();
					se->prev_rect = rect;
					se->prev_xform = xform;
				}

				Control *control = Object::cast_to<Control>(ci);
				if (control) {
					Vector2 pivot = control->get_pivot_offset();
					Vector2 pivot_ratio = control->get_pivot_offset_ratio();

					real_t anchors[4];
					anchors[SIDE_LEFT] = control->get_anchor(SIDE_LEFT);
					anchors[SIDE_RIGHT] = control->get_anchor(SIDE_RIGHT);
					anchors[SIDE_TOP] = control->get_anchor(SIDE_TOP);
					anchors[SIDE_BOTTOM] = control->get_anchor(SIDE_BOTTOM);

					if (pivot != se->prev_pivot || pivot_ratio != se->prev_pivot_ratio || anchors[SIDE_LEFT] != se->prev_anchors[SIDE_LEFT] || anchors[SIDE_RIGHT] != se->prev_anchors[SIDE_RIGHT] || anchors[SIDE_TOP] != se->prev_anchors[SIDE_TOP] || anchors[SIDE_BOTTOM] != se->prev_anchors[SIDE_BOTTOM]) {
						se->prev_pivot = pivot;
						se->prev_pivot_ratio = pivot_ratio;
						se->prev_anchors[SIDE_LEFT] = anchors[SIDE_LEFT];
						se->prev_anchors[SIDE_RIGHT] = anchors[SIDE_RIGHT];
						se->prev_anchors[SIDE_TOP] = anchors[SIDE_TOP];
						se->prev_anchors[SIDE_BOTTOM] = anchors[SIDE_BOTTOM];
						viewport->queue_redraw();
					}
				}
			}

			// Activate / Deactivate the pivot tool.
			pivot_button->set_disabled(selection.is_empty());

			// Update the viewport if bones changes
			for (KeyValue<BoneKey, BoneList> &E : bone_list) {
				Object *b = ObjectDB::get_instance(E.key.from);
				if (!b) {
					viewport->queue_redraw();
					break;
				}

				Node2D *b2 = Object::cast_to<Node2D>(b);
				if (!b2 || !b2->is_inside_tree()) {
					continue;
				}

				Transform2D global_xform = b2->get_global_transform();

				if (global_xform != E.value.xform) {
					E.value.xform = global_xform;
					viewport->queue_redraw();
				}

				Bone2D *bone = Object::cast_to<Bone2D>(b);
				if (bone && bone->get_length() != E.value.length) {
					E.value.length = bone->get_length();
					viewport->queue_redraw();
				}
			}
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorThemeManager::is_generated_theme_outdated() ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("editors/panning") ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("editors/2d") ||
					EditorSettings::get_singleton()->check_changed_settings_in_group("editors/polygon_editor")) {
				_update_editor_settings();
				update_viewport();
			}
		} break;

		case NOTIFICATION_APPLICATION_FOCUS_OUT:
		case NOTIFICATION_WM_WINDOW_FOCUS_OUT: {
			if (ci_manipulator->get_drag_type() != CanvasItemManipulator::DRAG_NONE) {
				ci_manipulator->commit_drag();
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				set_process(true);
				RenderingServer::get_singleton()->viewport_set_disable_2d(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), false);
				RenderingServer::get_singleton()->viewport_set_environment_mode(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), RSE::VIEWPORT_ENVIRONMENT_ENABLED);
			} else {
				set_process(false);
				RenderingServer::get_singleton()->viewport_set_disable_2d(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), true);
				RenderingServer::get_singleton()->viewport_set_environment_mode(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), RSE::VIEWPORT_ENVIRONMENT_DISABLED);
			}
		} break;
	}
}

void CanvasItemEditor::_selection_changed() {
	_update_lock_and_group_button();
	if (!selected_from_canvas) {
		ci_manipulator->reset_drag();
	}
	selected_from_canvas = false;

	if (ci_manipulator->reset_temp_pivot()) {
		viewport->queue_redraw();
	}

	// Check to redraw to clear anything drawn from visible selections if no selections are visible.
	bool has_visible = false;
	for (const KeyValue<ObjectID, Object *> &E : editor_selection->get_selection()) {
		CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
		if (!ci || !ci->is_visible_in_tree()) {
			continue;
		}

		Viewport *vp = ci->get_viewport();
		if (vp && !vp->is_visible_subviewport()) {
			continue;
		}

		CanvasItemEditorSelectedItem *se = editor_selection->get_node_editor_data<CanvasItemEditorSelectedItem>(ci);
		if (se) {
			has_visible = true;
			break;
		}
	}
	if (had_visible_selection != has_visible) {
		if (!has_visible) {
			viewport->queue_redraw();
		}
		had_visible_selection = has_visible;
	}
}

void CanvasItemEditor::edit(CanvasItem *p_canvas_item) {
	if (!p_canvas_item) {
		return;
	}

	Array selection = editor_selection->get_selected_nodes();
	if (selection.size() != 1 || Object::cast_to<Node>(selection[0]) != p_canvas_item) {
		ci_manipulator->reset_drag();
	}
}

void CanvasItemEditor::_update_scrollbars() {
	updating_scroll = true;

	if (clockgears_workspace) {
		// The ClockGears map canvas is navigated with pan/zoom only, never with
		// the stock Godot scrollbars and rulers.
		h_scroll->hide();
		v_scroll->hide();
		updating_scroll = false;
		return;
	}

	// Move the zoom buttons (top-left corner).
	Point2 controls_vb_begin = Point2(5, 5) * EDSCALE;
	controls_vb_begin += (ci_manipulator->is_showing_rulers()) ? Point2(ci_manipulator->get_ruler_width(), ci_manipulator->get_ruler_width()) : Point2();
	controls_vb->set_begin(controls_vb_begin);

	// Move the information panel (bottom-right corner).
	// Does not depend on rulers, since rulers are only on the top and left sides of the viewport.
	Point2 info_panel_end = Point2(-10, -10) * EDSCALE;
	info_panel_container->set_end(Point2(info_panel_end.x, info_panel_end.y));

	// Move the frame time panel to account for rulers (top-right corner).
	float frame_time_panel_end_y = 10 * EDSCALE;
	if (ci_manipulator->is_showing_rulers()) {
		frame_time_panel_end_y += ci_manipulator->get_ruler_width();
	}
	frame_time_panel_container->set_begin(Point2(frame_time_panel_container->get_begin().x, frame_time_panel_end_y));
	frame_time_panel_container->set_end(Point2(-10 * EDSCALE, frame_time_panel_container->get_end().y));

	Size2 hmin = h_scroll->get_minimum_size();
	Size2 vmin = v_scroll->get_minimum_size();

	// Get the visible frame.
	Size2 screen_rect = Size2(GLOBAL_GET("display/window/size/viewport_width"), GLOBAL_GET("display/window/size/viewport_height"));
	Rect2 local_rect = Rect2(Point2(), viewport->get_size() - Size2(vmin.width, hmin.height));

	// Calculate scrollable area.
	Rect2 canvas_item_rect = Rect2(Point2(), screen_rect);
	if (EditorNode::get_singleton()->is_inside_tree() && EditorNode::get_singleton()->get_edited_scene()) {
		Rect2 content_rect = _get_encompassing_rect(EditorNode::get_singleton()->get_edited_scene());
		canvas_item_rect.expand_to(content_rect.position);
		canvas_item_rect.expand_to(content_rect.position + content_rect.size);
	}
	canvas_item_rect.size += screen_rect * 2;
	canvas_item_rect.position -= screen_rect;

	// Updates the scrollbars.
	const Size2 size = viewport->get_size();
	const Point2 begin = canvas_item_rect.position;
	const Point2 end = canvas_item_rect.position + canvas_item_rect.size - local_rect.size / zoom;

	if (canvas_item_rect.size.height <= (local_rect.size.y / zoom)) {
		v_scroll->hide();
	} else {
		v_scroll->show();
		v_scroll->set_min(MIN(view_offset.y, begin.y));
		v_scroll->set_max(MAX(view_offset.y, end.y) + screen_rect.y);
		v_scroll->set_page(screen_rect.y);
	}

	if (canvas_item_rect.size.width <= (local_rect.size.x / zoom)) {
		h_scroll->hide();
	} else {
		h_scroll->show();
		h_scroll->set_min(MIN(view_offset.x, begin.x));
		h_scroll->set_max(MAX(view_offset.x, end.x) + screen_rect.x);
		h_scroll->set_page(screen_rect.x);
	}

	bool show_rulers = ci_manipulator->is_showing_rulers();
	float_t ruler_width = ci_manipulator->get_ruler_width();

	// Move and resize the scrollbars, avoiding overlap.
	if (is_layout_rtl()) {
		v_scroll->set_begin(Point2(0, (show_rulers) ? ruler_width : 0));
		v_scroll->set_end(Point2(vmin.width, size.height - (h_scroll->is_visible() ? hmin.height : 0)));
	} else {
		v_scroll->set_begin(Point2(size.width - vmin.width, (show_rulers) ? ruler_width : 0));
		v_scroll->set_end(Point2(size.width, size.height - (h_scroll->is_visible() ? hmin.height : 0)));
	}
	h_scroll->set_begin(Point2((show_rulers) ? ruler_width : 0, size.height - hmin.height));
	h_scroll->set_end(Point2(size.width - (v_scroll->is_visible() ? vmin.width : 0), size.height));

	// Calculate scrollable area.
	v_scroll->set_value(view_offset.y);
	h_scroll->set_value(view_offset.x);

	previous_update_view_offset = view_offset;
	updating_scroll = false;
}

void CanvasItemEditor::_update_scroll(real_t) {
	if (updating_scroll) {
		return;
	}

	view_offset.x = h_scroll->get_value();
	view_offset.y = v_scroll->get_value();
	ci_manipulator->set_view_offset(view_offset);
	viewport->queue_redraw();
}

void CanvasItemEditor::_zoom_on_position(real_t p_zoom, Point2 p_position) {
	p_zoom = CLAMP(p_zoom, zoom_widget->get_min_zoom(), zoom_widget->get_max_zoom());

	if (p_zoom == zoom) {
		return;
	}

	real_t prev_zoom = zoom;
	zoom = p_zoom;
	ci_manipulator->set_zoom(zoom);

	view_offset += p_position / prev_zoom - p_position / zoom;

	// We want to align in-scene pixels to screen pixels, this prevents blurry rendering
	// of small details (texts, lines).
	// This correction adds a jitter movement when zooming, so we correct only when the
	// zoom factor is an integer. (in the other cases, all pixels won't be aligned anyway)
	const real_t closest_zoom_factor = Math::round(zoom);
	if (Math::is_zero_approx(zoom - closest_zoom_factor)) {
		// Make sure scene pixel at view_offset is aligned on a screen pixel.
		Vector2 view_offset_int = view_offset.floor();
		Vector2 view_offset_frac = view_offset - view_offset_int;
		view_offset = view_offset_int + (view_offset_frac * closest_zoom_factor).round() / closest_zoom_factor;
	}

	ci_manipulator->set_view_offset(view_offset);
	zoom_widget->set_zoom(zoom);
	update_viewport();
	if (auto_resampling_enabled) {
		resample_timer->start();
	}
}

void CanvasItemEditor::_update_zoom(real_t p_zoom) {
	_zoom_on_position(p_zoom, viewport_scrollable->get_size() / 2.0);
}

void CanvasItemEditor::_update_oversampling() {
	EditorNode::get_singleton()->get_scene_root()->set_oversampling_override(auto_resampling_enabled ? zoom : 0.0);
}

void CanvasItemEditor::_shortcut_zoom_set(real_t p_zoom) {
	_zoom_on_position(p_zoom * MAX(1, EDSCALE), viewport->get_local_mouse_position());
}

void CanvasItemEditor::_button_toggle_local_space(bool p_status) {
	ci_manipulator->set_local_space_enabled(p_status);
	viewport->queue_redraw();
}

void CanvasItemEditor::_button_toggle_smart_snap(bool p_status) {
	ci_manipulator->set_smart_snap_enabled(p_status);
	viewport->queue_redraw();
}

void CanvasItemEditor::_button_toggle_grid_snap(bool p_status) {
	ci_manipulator->set_grid_snap_enabled(p_status);
	viewport->queue_redraw();
	emit_signal("grid_visibility_changed", is_grid_visible());
}

void CanvasItemEditor::_button_tool_select(int p_index) {
	Button *tb[CanvasItemManipulator::TOOL_MAX] = { select_button, scene_paint_button, list_select_button, move_button, scale_button, rotate_button, pivot_button, pan_button, ruler_button };
	for (int i = 0; i < CanvasItemManipulator::TOOL_MAX; i++) {
		tb[i]->set_pressed(i == p_index);
	}

	ci_manipulator->set_tool((CanvasItemManipulator::Tool)p_index);
	if (ci_manipulator->get_tool() == CanvasItemManipulator::TOOL_EDIT_PIVOT && Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		ci_manipulator->center_temp_pivot_on_selection();
	}

	viewport->queue_redraw();
	emit_signal("canvas_item_tool_changed", ci_manipulator->get_tool());
}

void CanvasItemEditor::_insert_animation_keys(bool p_location, bool p_rotation, bool p_scale, bool p_on_existing) {
	const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();

	AnimationTrackEditor *te = AnimationPlayerEditor::get_singleton()->get_track_editor();
	ERR_FAIL_COND_MSG(te->get_current_animation().is_null(), "Cannot insert animation key. No animation selected.");

	bool is_read_only = te->is_read_only();
	if (is_read_only) {
		te->popup_read_only_dialog();
		return;
	}
	te->make_insert_queue();
	for (const KeyValue<ObjectID, Object *> &E : selection) {
		CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
		if (!ci || !ci->is_visible_in_tree()) {
			continue;
		}

		if (Object::cast_to<Node2D>(ci)) {
			Node2D *n2d = Object::cast_to<Node2D>(ci);

			if (key_pos && p_location) {
				te->insert_node_value_key(n2d, "position", p_on_existing);
			}
			if (key_rot && p_rotation) {
				te->insert_node_value_key(n2d, "rotation", p_on_existing);
			}
			if (key_scale && p_scale) {
				te->insert_node_value_key(n2d, "scale", p_on_existing);
			}

			if (n2d->has_meta("_edit_bone_") && n2d->get_parent_item()) {
				//look for an IK chain
				List<Node2D *> ik_chain;

				Node2D *n = Object::cast_to<Node2D>(n2d->get_parent_item());
				bool has_chain = false;

				while (n) {
					ik_chain.push_back(n);
					if (n->has_meta("_edit_ik_")) {
						has_chain = true;
						break;
					}

					if (!n->get_parent_item()) {
						break;
					}
					n = Object::cast_to<Node2D>(n->get_parent_item());
				}

				if (has_chain && ik_chain.size()) {
					for (Node2D *&F : ik_chain) {
						if (key_pos) {
							te->insert_node_value_key(F, "position", p_on_existing);
						}
						if (key_rot) {
							te->insert_node_value_key(F, "rotation", p_on_existing);
						}
						if (key_scale) {
							te->insert_node_value_key(F, "scale", p_on_existing);
						}
					}
				}
			}

		} else if (Object::cast_to<Control>(ci)) {
			Control *ctrl = Object::cast_to<Control>(ci);

			if (key_pos) {
				te->insert_node_value_key(ctrl, "position", p_on_existing);
			}
			if (key_rot) {
				te->insert_node_value_key(ctrl, "rotation", p_on_existing);
			}
			if (key_scale) {
				te->insert_node_value_key(ctrl, "size", p_on_existing);
			}
		}
	}
	te->commit_insert_queue();
}

void CanvasItemEditor::_prepare_view_menu() {
	PopupMenu *popup = view_menu->get_popup();

	Node *root = EditorNode::get_singleton()->get_edited_scene();
	bool has_guides = root && (root->has_meta("_edit_horizontal_guides_") || root->has_meta("_edit_vertical_guides_"));
	popup->set_item_disabled(popup->get_item_index(CLEAR_GUIDES), !has_guides);
}

void CanvasItemEditor::_popup_callback(int p_op) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	last_option = MenuOption(p_op);
	switch (p_op) {
		case SHOW_ORIGIN: {
			show_origin = !show_origin;
			int idx = view_menu->get_popup()->get_item_index(SHOW_ORIGIN);
			view_menu->get_popup()->set_item_checked(idx, show_origin);
			viewport->queue_redraw();
		} break;
		case SHOW_VIEWPORT: {
			show_viewport = !show_viewport;
			int idx = view_menu->get_popup()->get_item_index(SHOW_VIEWPORT);
			view_menu->get_popup()->set_item_checked(idx, show_viewport);
			viewport->queue_redraw();
		} break;
		case SHOW_INFORMATION: {
			viewport->set_show_information(!viewport->is_showing_information());
			int idx = view_menu->get_popup()->get_item_index(SHOW_INFORMATION);
			view_menu->get_popup()->set_item_checked(idx, viewport->is_showing_information());
			viewport->queue_redraw();
		} break;
		case SHOW_FRAME_TIME: {
			viewport->set_show_frame_time(!viewport->is_showing_frame_time());
			int idx = view_menu->get_popup()->get_item_index(SHOW_FRAME_TIME);
			view_menu->get_popup()->set_item_checked(idx, viewport->is_showing_frame_time());
			viewport->queue_redraw();
		} break;
		case SHOW_POSITION_GIZMOS: {
			show_position_gizmos = !show_position_gizmos;
			int idx = gizmos_menu->get_item_index(SHOW_POSITION_GIZMOS);
			gizmos_menu->set_item_checked(idx, show_position_gizmos);
			viewport->queue_redraw();
		} break;
		case SHOW_LOCK_GIZMOS: {
			show_lock_gizmos = !show_lock_gizmos;
			int idx = gizmos_menu->get_item_index(SHOW_LOCK_GIZMOS);
			gizmos_menu->set_item_checked(idx, show_lock_gizmos);
			viewport->queue_redraw();
		} break;
		case SHOW_GROUP_GIZMOS: {
			show_group_gizmos = !show_group_gizmos;
			int idx = gizmos_menu->get_item_index(SHOW_GROUP_GIZMOS);
			gizmos_menu->set_item_checked(idx, show_group_gizmos);
			viewport->queue_redraw();
		} break;
		case SHOW_TRANSFORMATION_GIZMOS: {
			bool show_transformation_gizmos = !ci_manipulator->is_showing_transformation_gizmos();
			ci_manipulator->set_show_transformation_gizmos(show_transformation_gizmos);
			int idx = gizmos_menu->get_item_index(SHOW_TRANSFORMATION_GIZMOS);
			gizmos_menu->set_item_checked(idx, show_transformation_gizmos);
			viewport->queue_redraw();
		} break;
		case SNAP_USE_NODE_PARENT: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_NODE_PARENT;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_PARENT);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_NODE_PARENT);
		} break;
		case SNAP_USE_NODE_ANCHORS: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_NODE_ANCHORS;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_ANCHORS);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_NODE_ANCHORS);
		} break;
		case SNAP_USE_NODE_SIDES: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_NODE_SIDES;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_SIDES);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_NODE_SIDES);
		} break;
		case SNAP_USE_NODE_CENTER: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_NODE_CENTER;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_CENTER);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_NODE_CENTER);
		} break;
		case SNAP_USE_OTHER_NODES: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_OTHER_NODES;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_OTHER_NODES);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_OTHER_NODES);
		} break;
		case SNAP_USE_GUIDES: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_GUIDES;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = smartsnap_config_popup->get_item_index(SNAP_USE_GUIDES);
			smartsnap_config_popup->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_GUIDES);
		} break;
		case SNAP_USE_ROTATION: {
			bool snap_rotation = !ci_manipulator->is_snap_rotation_enabled();
			ci_manipulator->set_snap_rotation_enabled(snap_rotation);
			int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_ROTATION);
			snap_config_menu->get_popup()->set_item_checked(idx, snap_rotation);
		} break;
		case SNAP_USE_SCALE: {
			bool snap_scale = !ci_manipulator->is_snap_scale_enabled();
			ci_manipulator->set_snap_scale_enabled(snap_scale);
			int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_SCALE);
			snap_config_menu->get_popup()->set_item_checked(idx, snap_scale);
		} break;
		case SNAP_RELATIVE: {
			bool snap_relative = !ci_manipulator->is_snap_relative_enabled();
			ci_manipulator->set_snap_relative_enabled(snap_relative);
			int idx = snap_config_menu->get_popup()->get_item_index(SNAP_RELATIVE);
			snap_config_menu->get_popup()->set_item_checked(idx, snap_relative);
			viewport->queue_redraw();
		} break;
		case SNAP_USE_PIXEL: {
			int snap_mode = ci_manipulator->get_snap_mode() ^ CanvasItemManipulator::SNAP_PIXEL;
			ci_manipulator->set_snap_mode(snap_mode);
			int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_PIXEL);
			snap_config_menu->get_popup()->set_item_checked(idx, snap_mode & CanvasItemManipulator::SNAP_PIXEL);
		} break;
		case SNAP_CONFIGURE: {
			static_cast<SnapDialog *>(snap_dialog)->set_fields(ci_manipulator->get_grid_offset(), ci_manipulator->get_grid_step(), primary_grid_step, ci_manipulator->get_snap_rotation_offset(), ci_manipulator->get_snap_rotation_step(), ci_manipulator->get_snap_scale_step());
			snap_dialog->popup_centered(Size2(320, 160) * EDSCALE);
		} break;
		case SKELETON_SHOW_BONES: {
			List<Node *> selection = editor_selection->get_top_selected_node_list();
			for (Node *E : selection) {
				// Add children nodes so they are processed
				for (int child = 0; child < E->get_child_count(); child++) {
					selection.push_back(E->get_child(child));
				}

				Bone2D *bone_2d = Object::cast_to<Bone2D>(E);
				if (!bone_2d || !bone_2d->is_inside_tree()) {
					continue;
				}
				bone_2d->_editor_set_show_bone_gizmo(!bone_2d->_editor_get_show_bone_gizmo());
			}
		} break;
		case SHOW_HELPERS: {
			show_helpers = !show_helpers;
			int idx = view_menu->get_popup()->get_item_index(SHOW_HELPERS);
			view_menu->get_popup()->set_item_checked(idx, show_helpers);
			viewport->queue_redraw();
		} break;
		case SHOW_RULERS: {
			bool show_rulers = !ci_manipulator->is_showing_rulers();
			ci_manipulator->set_show_rulers(show_rulers);
			int idx = view_menu->get_popup()->get_item_index(SHOW_RULERS);
			view_menu->get_popup()->set_item_checked(idx, show_rulers);
			update_viewport();
		} break;
		case SHOW_GUIDES: {
			bool show_guides = !ci_manipulator->is_showing_guides();
			ci_manipulator->set_show_guides(show_guides);
			int idx = view_menu->get_popup()->get_item_index(SHOW_GUIDES);
			view_menu->get_popup()->set_item_checked(idx, show_guides);
			viewport->queue_redraw();
		} break;
		case LOCK_SELECTED: {
			undo_redo->create_action(TTR("Lock Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();
			for (Node *E : selection) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(E);
				if (!ci || !ci->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(ci, "set_meta", "_edit_lock_", true);
				undo_redo->add_undo_method(ci, "remove_meta", "_edit_lock_");
				undo_redo->add_do_method(this, "emit_signal", "item_lock_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_lock_status_changed");
			}
			undo_redo->add_do_method(viewport, "queue_redraw");
			undo_redo->add_undo_method(viewport, "queue_redraw");
			undo_redo->commit_action();
		} break;
		case UNLOCK_SELECTED: {
			undo_redo->create_action(TTR("Unlock Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();
			for (Node *E : selection) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(E);
				if (!ci || !ci->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(ci, "remove_meta", "_edit_lock_");
				undo_redo->add_undo_method(ci, "set_meta", "_edit_lock_", true);
				undo_redo->add_do_method(this, "emit_signal", "item_lock_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_lock_status_changed");
			}
			undo_redo->add_do_method(viewport, "queue_redraw");
			undo_redo->add_undo_method(viewport, "queue_redraw");
			undo_redo->commit_action();
		} break;
		case GROUP_SELECTED: {
			undo_redo->create_action(TTR("Group Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();
			for (Node *E : selection) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(E);
				if (!ci || !ci->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(ci, "set_meta", "_edit_group_", true);
				undo_redo->add_undo_method(ci, "remove_meta", "_edit_group_");
				undo_redo->add_do_method(this, "emit_signal", "item_group_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_group_status_changed");
			}
			undo_redo->add_do_method(viewport, "queue_redraw");
			undo_redo->add_undo_method(viewport, "queue_redraw");
			undo_redo->commit_action();
		} break;
		case UNGROUP_SELECTED: {
			undo_redo->create_action(TTR("Ungroup Selected"));

			const List<Node *> &selection = editor_selection->get_top_selected_node_list();
			for (Node *E : selection) {
				CanvasItem *ci = Object::cast_to<CanvasItem>(E);
				if (!ci || !ci->is_inside_tree()) {
					continue;
				}

				undo_redo->add_do_method(ci, "remove_meta", "_edit_group_");
				undo_redo->add_undo_method(ci, "set_meta", "_edit_group_", true);
				undo_redo->add_do_method(this, "emit_signal", "item_group_status_changed");
				undo_redo->add_undo_method(this, "emit_signal", "item_group_status_changed");
			}
			undo_redo->add_do_method(viewport, "queue_redraw");
			undo_redo->add_undo_method(viewport, "queue_redraw");
			undo_redo->commit_action();
		} break;

		case ANIM_INSERT_KEY:
		case ANIM_INSERT_KEY_EXISTING: {
			bool existing = p_op == ANIM_INSERT_KEY_EXISTING;

			_insert_animation_keys(true, true, true, existing);

		} break;
		case ANIM_INSERT_POS: {
			key_pos = key_loc_button->is_pressed();
		} break;
		case ANIM_INSERT_ROT: {
			key_rot = key_rot_button->is_pressed();
		} break;
		case ANIM_INSERT_SCALE: {
			key_scale = key_scale_button->is_pressed();
		} break;
		case ANIM_COPY_POSE: {
			pose_clipboard.clear();

			const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();

			for (const KeyValue<ObjectID, Object *> &E : selection) {
				CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
				if (!ci || !ci->is_visible_in_tree()) {
					continue;
				}

				if (Object::cast_to<Node2D>(ci)) {
					Node2D *n2d = Object::cast_to<Node2D>(ci);
					PoseClipboard pc;
					pc.pos = n2d->get_position();
					pc.rot = n2d->get_rotation();
					pc.scale = n2d->get_scale();
					pc.id = n2d->get_instance_id();
					pose_clipboard.push_back(pc);
				}
			}

		} break;
		case ANIM_PASTE_POSE: {
			if (!pose_clipboard.size()) {
				break;
			}

			undo_redo->create_action(TTR("Paste Pose"));
			for (const PoseClipboard &E : pose_clipboard) {
				Node2D *n2d = ObjectDB::get_instance<Node2D>(E.id);
				if (!n2d) {
					continue;
				}
				undo_redo->add_do_method(n2d, "set_position", E.pos);
				undo_redo->add_do_method(n2d, "set_rotation", E.rot);
				undo_redo->add_do_method(n2d, "set_scale", E.scale);
				undo_redo->add_undo_method(n2d, "set_position", n2d->get_position());
				undo_redo->add_undo_method(n2d, "set_rotation", n2d->get_rotation());
				undo_redo->add_undo_method(n2d, "set_scale", n2d->get_scale());
			}
			undo_redo->commit_action();

		} break;
		case ANIM_CLEAR_POSE: {
			HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();

			for (const KeyValue<ObjectID, Object *> &E : selection) {
				CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
				if (!ci || !ci->is_visible_in_tree()) {
					continue;
				}

				if (Object::cast_to<Node2D>(ci)) {
					Node2D *n2d = Object::cast_to<Node2D>(ci);

					if (key_pos) {
						n2d->set_position(Vector2());
					}
					if (key_rot) {
						n2d->set_rotation(0);
					}
					if (key_scale) {
						n2d->set_scale(Vector2(1, 1));
					}
				} else if (Object::cast_to<Control>(ci)) {
					Control *ctrl = Object::cast_to<Control>(ci);

					if (key_pos) {
						ctrl->set_position(Point2());
					}
				}
			}

		} break;
		case CLEAR_GUIDES: {
			Node *const root = EditorNode::get_singleton()->get_edited_scene();

			if (root && (root->has_meta("_edit_horizontal_guides_") || root->has_meta("_edit_vertical_guides_"))) {
				undo_redo->create_action(TTR("Clear Guides"));
				if (root->has_meta("_edit_horizontal_guides_")) {
					Array hguides = root->get_meta("_edit_horizontal_guides_");

					undo_redo->add_do_method(root, "remove_meta", "_edit_horizontal_guides_");
					undo_redo->add_undo_method(root, "set_meta", "_edit_horizontal_guides_", hguides);
				}
				if (root->has_meta("_edit_vertical_guides_")) {
					Array vguides = root->get_meta("_edit_vertical_guides_");

					undo_redo->add_do_method(root, "remove_meta", "_edit_vertical_guides_");
					undo_redo->add_undo_method(root, "set_meta", "_edit_vertical_guides_", vguides);
				}
				undo_redo->add_do_method(viewport, "queue_redraw");
				undo_redo->add_undo_method(viewport, "queue_redraw");
				undo_redo->commit_action();
			}

		} break;
		case VIEW_CENTER_TO_SELECTION:
		case VIEW_FRAME_TO_SELECTION: {
			_focus_selection(p_op);

		} break;
		case PREVIEW_CANVAS_SCALE: {
			bool preview = view_menu->get_popup()->is_item_checked(view_menu->get_popup()->get_item_index(PREVIEW_CANVAS_SCALE));
			preview = !preview;
			RS::get_singleton()->canvas_set_disable_scale(!preview);
			view_menu->get_popup()->set_item_checked(view_menu->get_popup()->get_item_index(PREVIEW_CANVAS_SCALE), preview);

		} break;
		case SKELETON_MAKE_BONES: {
			HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();
			Node *editor_root = get_tree()->get_edited_scene_root();

			if (!editor_root || selection.is_empty()) {
				return;
			}

			undo_redo->create_action(TTR("Create Custom Bone2D(s) from Node(s)"));
			for (const KeyValue<ObjectID, Object *> &E : selection) {
				Node2D *n2d = ObjectDB::get_instance<Node2D>(E.key);
				if (!n2d) {
					continue;
				}

				Bone2D *new_bone = memnew(Bone2D);
				String new_bone_name = n2d->get_name();
				new_bone_name += "Bone2D";
				new_bone->set_name(new_bone_name);
				new_bone->set_transform(n2d->get_transform());

				Node *n2d_parent = n2d->get_parent();
				if (!n2d_parent) {
					continue;
				}

				undo_redo->add_do_method(n2d_parent, "add_child", new_bone);
				undo_redo->add_do_method(n2d_parent, "remove_child", n2d);
				undo_redo->add_do_method(new_bone, "add_child", n2d);
				undo_redo->add_do_method(n2d, "set_transform", Transform2D());
				undo_redo->add_do_method(this, "_set_owner_for_node_and_children", new_bone, editor_root);
				undo_redo->add_do_reference(new_bone);

				undo_redo->add_undo_method(new_bone, "remove_child", n2d);
				undo_redo->add_undo_method(n2d_parent, "add_child", n2d);
				undo_redo->add_undo_method(n2d_parent, "remove_child", new_bone);
				undo_redo->add_undo_method(n2d, "set_transform", new_bone->get_transform());
				undo_redo->add_undo_method(this, "_set_owner_for_node_and_children", n2d, editor_root);
			}
			undo_redo->commit_action();

		} break;
		case AUTO_RESAMPLE_CANVAS_ITEMS: {
			auto_resampling_enabled = !auto_resampling_enabled;
			int idx = view_menu->get_popup()->get_item_index(AUTO_RESAMPLE_CANVAS_ITEMS);
			view_menu->get_popup()->set_item_checked(idx, auto_resampling_enabled);
			if (!resample_timer->is_stopped()) {
				resample_timer->stop();
			}
			_update_oversampling();
			EditorSettings::get_singleton()->set_project_metadata("2d_editor", "auto_resampling_enabled", auto_resampling_enabled);
		} break;
	}
}

void CanvasItemEditor::_set_owner_for_node_and_children(Node *p_node, Node *p_owner) {
	p_node->set_owner(p_owner);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_set_owner_for_node_and_children(p_node->get_child(i), p_owner);
	}
}

void CanvasItemEditor::_focus_selection(int p_op) {
	Rect2 rect;
	int count = 0;

	const HashMap<ObjectID, Object *> &selection = editor_selection->get_selection();
	for (const KeyValue<ObjectID, Object *> &E : selection) {
		CanvasItem *ci = ObjectDB::get_instance<CanvasItem>(E.key);
		if (!ci) {
			continue;
		}
		const Transform2D canvas_item_transform = ci->get_global_transform();
		if (!canvas_item_transform.is_finite()) {
			continue;
		}
		Rect2 item_rect;
		if (ci->_edit_use_rect()) {
			item_rect = ci->_edit_get_rect();
		} else {
			item_rect = Rect2();
		}
		Vector2 pos = canvas_item_transform.get_origin();
		const Vector2 scale = canvas_item_transform.get_scale();
		const real_t angle = canvas_item_transform.get_rotation();
		pos = ci->get_viewport()->get_popup_base_transform().xform(pos);

		Transform2D t(angle, Vector2(0.f, 0.f));
		item_rect = t.xform(item_rect);
		Rect2 canvas_item_rect(pos + scale * item_rect.position, scale * item_rect.size);
		if (count == 0) {
			rect = canvas_item_rect;
		} else {
			rect = rect.merge(canvas_item_rect);
		}
		count++;
	}

	if (p_op == VIEW_FRAME_TO_SELECTION && rect.size.x > CMP_EPSILON && rect.size.y > CMP_EPSILON) {
		real_t scale_x = viewport->get_size().x / rect.size.x;
		real_t scale_y = viewport->get_size().y / rect.size.y;
		zoom = scale_x < scale_y ? scale_x : scale_y;
		zoom *= 0.90;
		zoom_widget->set_zoom(zoom);
		ci_manipulator->set_zoom(zoom);
		viewport->queue_redraw(); // Redraw to update the global canvas transform after zoom changes.
		callable_mp(this, &CanvasItemEditor::center_at).call_deferred(rect.get_center()); // Defer because the updated transform is needed.
		if (auto_resampling_enabled) {
			resample_timer->start();
		}
	} else {
		center_at(rect.get_center());
	}
}

void CanvasItemEditor::_bind_methods() {
	ClassDB::bind_method("_get_editor_data", &CanvasItemEditor::_get_editor_data);

	ClassDB::bind_method(D_METHOD("update_viewport"), &CanvasItemEditor::update_viewport);
	ClassDB::bind_method(D_METHOD("center_at", "position"), &CanvasItemEditor::center_at);

	ClassDB::bind_method("_set_owner_for_node_and_children", &CanvasItemEditor::_set_owner_for_node_and_children);

	ClassDB::bind_method(D_METHOD("set_clockgears_layer", "layer"), &CanvasItemEditor::set_clockgears_current_layer);
	ClassDB::bind_method(D_METHOD("get_clockgears_layer"), &CanvasItemEditor::get_clockgears_current_layer);
	ClassDB::bind_method(D_METHOD("set_clockgears_draw_tool", "tool"), &CanvasItemEditor::set_clockgears_draw_tool);
	ClassDB::bind_method(D_METHOD("get_clockgears_draw_tool"), &CanvasItemEditor::get_clockgears_draw_tool);
	ClassDB::bind_method(D_METHOD("set_clockgears_selected_tile", "tile_id", "atlas_coords", "texture", "source_id"), &CanvasItemEditor::set_clockgears_selected_tile, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("simulate_viewport_gui_input", "event"), &CanvasItemEditor::simulate_gui_input);
	ClassDB::bind_method(D_METHOD("simulate_mouse_press", "canvas_pos", "right_button"), &CanvasItemEditor::simulate_mouse_press, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_mouse_motion", "canvas_pos"), &CanvasItemEditor::simulate_mouse_motion);
	ClassDB::bind_method(D_METHOD("simulate_mouse_release", "canvas_pos", "right_button"), &CanvasItemEditor::simulate_mouse_release, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("trigger_undo"), &CanvasItemEditor::trigger_undo);
	ClassDB::bind_method(D_METHOD("trigger_redo"), &CanvasItemEditor::trigger_redo);
	ClassDB::bind_method(D_METHOD("canvas_to_screen", "canvas_pos"), &CanvasItemEditor::canvas_to_screen);
	ClassDB::bind_method(D_METHOD("screen_to_canvas", "screen_pos"), &CanvasItemEditor::screen_to_canvas);

	ADD_SIGNAL(MethodInfo("snap_changed"));
	ADD_SIGNAL(MethodInfo("grid_visibility_changed", PropertyInfo(Variant::BOOL, "visible")));
	ADD_SIGNAL(MethodInfo("item_lock_status_changed"));
	ADD_SIGNAL(MethodInfo("item_group_status_changed"));
	ADD_SIGNAL(MethodInfo("canvas_item_tool_changed", PropertyInfo(Variant::INT, "tool")));
}

Dictionary CanvasItemEditor::get_state() const {
	Dictionary state;
	// Take the editor scale into account.
	state["zoom"] = zoom / MAX(1, EDSCALE);
	state["ofs"] = view_offset;
	state["grid_offset"] = ci_manipulator->get_grid_offset();
	state["grid_step"] = ci_manipulator->get_grid_step();
	state["primary_grid_step"] = primary_grid_step;
	state["snap_rotation_offset"] = ci_manipulator->get_snap_rotation_offset();
	state["snap_rotation_step"] = ci_manipulator->get_snap_rotation_step();
	state["snap_scale_step"] = ci_manipulator->get_snap_scale_step();
	state["smart_snap_active"] = ci_manipulator->is_smart_snap_enabled();
	state["grid_snap_active"] = ci_manipulator->is_grid_snap_enabled();
	int snap_mode = ci_manipulator->get_snap_mode();
	state["snap_node_parent"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_NODE_PARENT);
	state["snap_node_anchors"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_NODE_ANCHORS);
	state["snap_node_sides"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_NODE_SIDES);
	state["snap_node_center"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_NODE_CENTER);
	state["snap_other_nodes"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_OTHER_NODES);
	state["snap_guides"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_GUIDES);
	state["grid_visibility"] = grid_visibility;
	state["show_origin"] = show_origin;
	state["show_viewport"] = show_viewport;
	state["show_information"] = viewport->is_showing_information();
	state["show_frame_time"] = viewport->is_showing_frame_time();
	state["show_rulers"] = ci_manipulator->is_showing_rulers();
	state["show_guides"] = ci_manipulator->is_showing_guides();
	state["show_helpers"] = show_helpers;
	state["show_zoom_control"] = zoom_widget->is_visible();
	state["show_position_gizmos"] = show_position_gizmos;
	state["show_lock_gizmos"] = show_lock_gizmos;
	state["show_group_gizmos"] = show_group_gizmos;
	state["show_transformation_gizmos"] = ci_manipulator->is_showing_transformation_gizmos();
	state["snap_rotation"] = ci_manipulator->is_snap_rotation_enabled();
	state["snap_scale"] = ci_manipulator->is_snap_scale_enabled();
	state["snap_relative"] = ci_manipulator->is_snap_relative_enabled();
	state["snap_pixel"] = static_cast<bool>(snap_mode & CanvasItemManipulator::SNAP_PIXEL);
	return state;
}

void CanvasItemEditor::set_state(const Dictionary &p_state) {
	bool update_scrollbars = false;
	Dictionary state = p_state;
	if (state.has("zoom")) {
		// Compensate the editor scale, so that the editor scale can be changed
		// and the zoom level will still be the same (relative to the editor scale).
		zoom = real_t(p_state["zoom"]) * MAX(1, EDSCALE);
		zoom_widget->set_zoom(zoom);
		ci_manipulator->set_zoom(zoom);
		if (auto_resampling_enabled) {
			resample_timer->start();
		}
	}

	if (state.has("ofs")) {
		view_offset = p_state["ofs"];
		previous_update_view_offset = view_offset;
		ci_manipulator->set_view_offset(view_offset);
		update_scrollbars = true;
	}

	if (state.has("grid_offset")) {
		ci_manipulator->set_grid_offset(state["grid_offset"]);
	}

	if (state.has("grid_step")) {
		ci_manipulator->set_grid_step(state["grid_step"]);
	}

#ifndef DISABLE_DEPRECATED
	if (state.has("primary_grid_steps")) {
		primary_grid_step.x = state["primary_grid_steps"];
		primary_grid_step.y = state["primary_grid_steps"];
	}
#endif // DISABLE_DEPRECATED

	if (state.has("primary_grid_step")) {
		primary_grid_step = state["primary_grid_step"];
	}

	if (state.has("snap_rotation_step")) {
		ci_manipulator->set_snap_rotation_step(state["snap_rotation_step"]);
	}

	if (state.has("snap_rotation_offset")) {
		ci_manipulator->set_snap_rotation_offset(state["snap_rotation_offset"]);
	}

	if (state.has("snap_scale_step")) {
		ci_manipulator->set_snap_scale_step(state["snap_scale_step"]);
	}

	if (state.has("smart_snap_active")) {
		ci_manipulator->set_smart_snap_enabled(state["smart_snap_active"]);
		smart_snap_button->set_pressed(ci_manipulator->is_smart_snap_enabled());
	}

	if (state.has("grid_snap_active")) {
		ci_manipulator->set_grid_snap_enabled(state["grid_snap_active"]);
		grid_snap_button->set_pressed(ci_manipulator->is_grid_snap_enabled());
	}

	if (state.has("snap_node_parent")) {
		int val = static_cast<int>(state["snap_node_parent"]) * CanvasItemManipulator::SNAP_NODE_PARENT;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_NODE_PARENT) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_PARENT);
		smartsnap_config_popup->set_item_checked(idx, state["snap_node_parent"]);
	}

	if (state.has("snap_node_anchors")) {
		int val = static_cast<int>(state["snap_node_anchors"]) * CanvasItemManipulator::SNAP_NODE_ANCHORS;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_NODE_ANCHORS) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_ANCHORS);
		smartsnap_config_popup->set_item_checked(idx, state["snap_node_anchors"]);
	}

	if (state.has("snap_node_sides")) {
		int val = static_cast<int>(state["snap_node_sides"]) * CanvasItemManipulator::SNAP_NODE_SIDES;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_NODE_SIDES) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_SIDES);
		smartsnap_config_popup->set_item_checked(idx, state["snap_node_sides"]);
	}

	if (state.has("snap_node_center")) {
		int val = static_cast<int>(state["snap_node_center"]) * CanvasItemManipulator::SNAP_NODE_CENTER;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_NODE_CENTER) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_NODE_CENTER);
		smartsnap_config_popup->set_item_checked(idx, state["snap_node_center"]);
	}

	if (state.has("snap_other_nodes")) {
		int val = static_cast<int>(state["snap_other_nodes"]) * CanvasItemManipulator::SNAP_OTHER_NODES;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_OTHER_NODES) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_OTHER_NODES);
		smartsnap_config_popup->set_item_checked(idx, state["snap_other_nodes"]);
	}

	if (state.has("snap_guides")) {
		int val = static_cast<int>(state["snap_guides"]) * CanvasItemManipulator::SNAP_GUIDES;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_GUIDES) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = smartsnap_config_popup->get_item_index(SNAP_USE_GUIDES);
		smartsnap_config_popup->set_item_checked(idx, state["snap_guides"]);
	}

	if (state.has("grid_visibility")) {
		grid_visibility = (GridVisibility)(int)(state["grid_visibility"]);
	}

	if (state.has("show_origin")) {
		show_origin = state["show_origin"];
		int idx = view_menu->get_popup()->get_item_index(SHOW_ORIGIN);
		view_menu->get_popup()->set_item_checked(idx, show_origin);
	}

	if (state.has("show_viewport")) {
		show_viewport = state["show_viewport"];
		int idx = view_menu->get_popup()->get_item_index(SHOW_VIEWPORT);
		view_menu->get_popup()->set_item_checked(idx, show_viewport);
	}

	if (state.has("show_information")) {
		viewport->set_show_information(state["show_information"]);
		int idx = view_menu->get_popup()->get_item_index(SHOW_INFORMATION);
		view_menu->get_popup()->set_item_checked(idx, viewport->is_showing_information());
	}

	if (state.has("show_frame_time")) {
		viewport->set_show_frame_time(state["show_frame_time"]);
		int idx = view_menu->get_popup()->get_item_index(SHOW_FRAME_TIME);
		view_menu->get_popup()->set_item_checked(idx, viewport->is_showing_frame_time());
	}

	if (state.has("show_rulers")) {
		ci_manipulator->set_show_rulers(state["show_rulers"]);
		int idx = view_menu->get_popup()->get_item_index(SHOW_RULERS);
		view_menu->get_popup()->set_item_checked(idx, ci_manipulator->is_showing_rulers());
		update_scrollbars = true;
	}

	if (state.has("show_guides")) {
		ci_manipulator->set_show_guides(state["show_guides"]);
		int idx = view_menu->get_popup()->get_item_index(SHOW_GUIDES);
		view_menu->get_popup()->set_item_checked(idx, ci_manipulator->is_showing_guides());
	}

	if (state.has("show_helpers")) {
		show_helpers = state["show_helpers"];
		int idx = view_menu->get_popup()->get_item_index(SHOW_HELPERS);
		view_menu->get_popup()->set_item_checked(idx, show_helpers);
	}

	if (state.has("show_position_gizmos")) {
		show_position_gizmos = state["show_position_gizmos"];
		int idx = gizmos_menu->get_item_index(SHOW_POSITION_GIZMOS);
		gizmos_menu->set_item_checked(idx, show_position_gizmos);
	}

	if (state.has("show_lock_gizmos")) {
		show_lock_gizmos = state["show_lock_gizmos"];
		int idx = gizmos_menu->get_item_index(SHOW_LOCK_GIZMOS);
		gizmos_menu->set_item_checked(idx, show_lock_gizmos);
	}

	if (state.has("show_group_gizmos")) {
		show_group_gizmos = state["show_group_gizmos"];
		int idx = gizmos_menu->get_item_index(SHOW_GROUP_GIZMOS);
		gizmos_menu->set_item_checked(idx, show_group_gizmos);
	}

	if (state.has("show_transformation_gizmos")) {
		ci_manipulator->set_show_transformation_gizmos(state["show_transformation_gizmos"]);
		int idx = gizmos_menu->get_item_index(SHOW_TRANSFORMATION_GIZMOS);
		gizmos_menu->set_item_checked(idx, ci_manipulator->is_showing_transformation_gizmos());
	}

	if (state.has("show_zoom_control")) {
		// This one is not user-controllable, but instrumentable
		zoom_widget->set_visible(state["show_zoom_control"]);
	}

	if (state.has("snap_rotation")) {
		bool snap_rotation = state["snap_rotation"];
		ci_manipulator->set_snap_rotation_enabled(snap_rotation);
		int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_ROTATION);
		snap_config_menu->get_popup()->set_item_checked(idx, snap_rotation);
	}

	if (state.has("snap_scale")) {
		bool snap_scale = state["snap_scale"];
		ci_manipulator->set_snap_scale_enabled(snap_scale);
		int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_SCALE);
		snap_config_menu->get_popup()->set_item_checked(idx, snap_scale);
	}

	if (state.has("snap_relative")) {
		bool snap_relative = state["snap_relative"];
		ci_manipulator->set_snap_relative_enabled(snap_relative);
		int idx = snap_config_menu->get_popup()->get_item_index(SNAP_RELATIVE);
		snap_config_menu->get_popup()->set_item_checked(idx, snap_relative);
	}

	if (state.has("snap_pixel")) {
		int val = static_cast<int>(state["snap_pixel"]) * CanvasItemManipulator::SNAP_PIXEL;
		int snap_mode = (ci_manipulator->get_snap_mode() & ~CanvasItemManipulator::SNAP_PIXEL) | val;
		ci_manipulator->set_snap_mode(snap_mode);
		int idx = snap_config_menu->get_popup()->get_item_index(SNAP_USE_PIXEL);
		snap_config_menu->get_popup()->set_item_checked(idx, state["snap_pixel"]);
	}

	if (update_scrollbars) {
		_update_scrollbars();
	}
	viewport->queue_redraw();
}

void CanvasItemEditor::clear() {
	zoom = 1.0 / MAX(1, EDSCALE);
	zoom_widget->set_zoom(zoom);
	ci_manipulator->set_zoom(zoom);

	view_offset = Point2(-150 - ci_manipulator->get_ruler_width(), -95 - ci_manipulator->get_ruler_width());
	previous_update_view_offset = view_offset; // Moves the view a little bit to the left so that (0,0) is visible. The values a relative to a 16/10 screen.
	ci_manipulator->set_view_offset(view_offset);
	_update_scrollbars();

	primary_grid_step = EditorSettings::get_singleton()->get_project_metadata("2d_editor", "primary_grid_step", Vector2i(8, 8));
	ci_manipulator->set_grid_offset(EditorSettings::get_singleton()->get_project_metadata("2d_editor", "grid_offset", Vector2()));
	ci_manipulator->set_grid_step(EditorSettings::get_singleton()->get_project_metadata("2d_editor", "grid_step", Vector2(8, 8)));
	ci_manipulator->set_snap_rotation_step(EditorSettings::get_singleton()->get_project_metadata("2d_editor", "snap_rotation_step", Math::deg_to_rad(15.0)));
	ci_manipulator->set_snap_rotation_offset(EditorSettings::get_singleton()->get_project_metadata("2d_editor", "snap_rotation_offset", 0.0));
	ci_manipulator->set_snap_scale_step(EditorSettings::get_singleton()->get_project_metadata("2d_editor", "snap_scale_step", 0.1));

	if (auto_resampling_enabled) {
		if (resample_timer->is_inside_tree()) {
			resample_timer->start();
		} else {
			_update_oversampling();
		}
	}
}

void CanvasItemEditor::add_control_to_menu_panel(Control *p_control) {
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(p_control->get_parent());

	VSeparator *sep = memnew(VSeparator);
	context_toolbar_hbox->add_child(sep);
	context_toolbar_hbox->add_child(p_control);
	context_toolbar_separators[p_control] = sep;

	p_control->connect(SceneStringName(visibility_changed), callable_mp(this, &CanvasItemEditor::_update_context_toolbar));

	_update_context_toolbar();
}

void CanvasItemEditor::remove_control_from_menu_panel(Control *p_control) {
	ERR_FAIL_NULL(p_control);
	ERR_FAIL_COND(p_control->get_parent() != context_toolbar_hbox);

	p_control->disconnect(SceneStringName(visibility_changed), callable_mp(this, &CanvasItemEditor::_update_context_toolbar));

	VSeparator *sep = context_toolbar_separators[p_control];
	context_toolbar_hbox->remove_child(sep);
	context_toolbar_hbox->remove_child(p_control);
	context_toolbar_separators.erase(p_control);
	memdelete(sep);

	_update_context_toolbar();
}

void CanvasItemEditor::_update_context_toolbar() {
	bool has_visible = false;
	bool first_visible = false;

	for (int i = 0; i < context_toolbar_hbox->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(context_toolbar_hbox->get_child(i));
		if (!child || !context_toolbar_separators.has(child)) {
			continue;
		}
		if (child->is_visible()) {
			first_visible = !has_visible;
			has_visible = true;
		}

		VSeparator *sep = context_toolbar_separators[child];
		sep->set_visible(!first_visible && child->is_visible());
	}

	context_toolbar_panel->set_visible(has_visible);
}

void CanvasItemEditor::add_control_to_left_panel(Control *p_control) {
	left_panel_split->add_child(p_control);
	left_panel_split->move_child(p_control, 0);
}

void CanvasItemEditor::add_control_to_right_panel(Control *p_control) {
	right_panel_split->add_child(p_control);
	right_panel_split->move_child(p_control, 1);
}

void CanvasItemEditor::remove_control_from_left_panel(Control *p_control) {
	left_panel_split->remove_child(p_control);
}

void CanvasItemEditor::remove_control_from_right_panel(Control *p_control) {
	right_panel_split->remove_child(p_control);
}

VSplitContainer *CanvasItemEditor::get_bottom_split() {
	return bottom_split;
}

void CanvasItemEditor::set_clockgears_workspace(bool p_enabled) {
	clockgears_workspace = p_enabled;
	if (!p_enabled) {
		return;
	}

	if (toolbar_margin) {
		toolbar_margin->hide();
	}
	if (controls_vb) {
		controls_vb->hide();
	}
	if (info_panel_container) {
		info_panel_container->hide();
	}
	if (frame_time_panel_container) {
		frame_time_panel_container->hide();
	}
	if (top_ruler) {
		top_ruler->hide();
	}
	if (left_ruler) {
		left_ruler->hide();
	}
	if (h_scroll) {
		h_scroll->hide();
	}
	if (v_scroll) {
		v_scroll->hide();
	}

	// Keep only the viewport: the side panels hold stock Godot editors.
	if (left_panel_split) {
		for (int i = 0; i < left_panel_split->get_child_count(); i++) {
			Control *child = Object::cast_to<Control>(left_panel_split->get_child(i));
			if (child && child != right_panel_split) {
				child->hide();
			}
		}
	}
	if (right_panel_split) {
		for (int i = 0; i < right_panel_split->get_child_count(); i++) {
			Control *child = Object::cast_to<Control>(right_panel_split->get_child(i));
			if (child && child != viewport_scrollable) {
				child->hide();
			}
		}
	}

	// No transformation gizmos: this is a tile canvas, not a scene editor.
	show_position_gizmos = false;
	show_lock_gizmos = false;
	show_group_gizmos = false;

	// Strict map snapping on a 32x32 grid.
	ci_manipulator->set_show_rulers(false);
	ci_manipulator->set_grid_snap_enabled(true);
	ci_manipulator->set_smart_snap_enabled(false);
	ci_manipulator->set_grid_step(Point2(32, 32));
	grid_visibility = GRID_VISIBILITY_SHOW;

	if (EditorNode::get_singleton() && EditorNode::get_singleton()->get_scene_root()) {
		RenderingServer::get_singleton()->viewport_set_disable_2d(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), false);
		RenderingServer::get_singleton()->viewport_set_environment_mode(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), RSE::VIEWPORT_ENVIRONMENT_ENABLED);
	}

	update_viewport();
	queue_redraw();
}

void CanvasItemEditor::set_clockgears_event_mode(bool p_enabled) {
	clockgears_event_mode = p_enabled;
	if (!p_enabled) {
		clockgears_hovered_cell = Point2i(-1, -1);
	}
	update_viewport();
}

void CanvasItemEditor::set_clockgears_grid_size(int p_size) {
	if (p_size > 0) {
		clockgears_grid_size = p_size;
		ci_manipulator->set_grid_step(Point2(p_size, p_size));
		update_viewport();
	}
}

void CanvasItemEditor::_draw_clockgears_event_grid() {
	if (!viewport) {
		return;
	}

	Size2 vp_size = viewport->get_size();
	Transform2D inv = transform.affine_inverse();

	Point2 top_left = inv.xform(Point2(0, 0));
	Point2 bottom_right = inv.xform(vp_size);

	int start_x = Math::floor(top_left.x / (real_t)clockgears_grid_size) * clockgears_grid_size;
	int end_x = Math::ceil(bottom_right.x / (real_t)clockgears_grid_size) * clockgears_grid_size;
	int start_y = Math::floor(top_left.y / (real_t)clockgears_grid_size) * clockgears_grid_size;
	int end_y = Math::ceil(bottom_right.y / (real_t)clockgears_grid_size) * clockgears_grid_size;

	Color grid_color = Color(0.9, 0.72, 0.26, 0.35);
	Color highlight_color = Color(0.95, 0.75, 0.2, 0.25);
	Color highlight_border = Color(1.0, 0.85, 0.3, 0.8);

	for (int x = start_x; x <= end_x; x += clockgears_grid_size) {
		Point2 p1 = transform.xform(Point2(x, start_y));
		Point2 p2 = transform.xform(Point2(x, end_y));
		viewport->draw_line(p1, p2, grid_color, 1.0);
	}

	for (int y = start_y; y <= end_y; y += clockgears_grid_size) {
		Point2 p1 = transform.xform(Point2(start_x, y));
		Point2 p2 = transform.xform(Point2(end_x, y));
		viewport->draw_line(p1, p2, grid_color, 1.0);
	}

	if (clockgears_hovered_cell.x >= 0 && clockgears_hovered_cell.y >= 0) {
		Point2 cell_pos = Point2(clockgears_hovered_cell.x * clockgears_grid_size, clockgears_hovered_cell.y * clockgears_grid_size);
		Point2 screen_tl = transform.xform(cell_pos);
		Point2 screen_br = transform.xform(cell_pos + Point2(clockgears_grid_size, clockgears_grid_size));
		Rect2 cell_rect(screen_tl, screen_br - screen_tl);
		viewport->draw_rect(cell_rect, highlight_color, true);
		viewport->draw_rect(cell_rect, highlight_border, false, 2.0);
	}

	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (scene) {
		Ref<Font> font = get_theme_font(SNAME("font"), EditorStringName(EditorFonts));
		int font_sz = CLAMP(int(10 * EDSCALE), 9, 13);
		TypedArray<Node> events = scene->find_children("*", "ClockGearsEvent", true, false);
		for (int i = 0; i < events.size(); i++) {
			Node2D *ev = Object::cast_to<Node2D>(events[i]);
			if (ev) {
				Point2 ev_pos = ev->get_position();
				int cx = Math::floor(ev_pos.x / (real_t)clockgears_grid_size);
				int cy = Math::floor(ev_pos.y / (real_t)clockgears_grid_size);
				Point2 cell_pos = Point2(cx * clockgears_grid_size, cy * clockgears_grid_size);
				Point2 screen_tl = transform.xform(cell_pos);
				Point2 screen_br = transform.xform(cell_pos + Point2(clockgears_grid_size, clockgears_grid_size));
				Rect2 ev_rect(screen_tl, screen_br - screen_tl);
				viewport->draw_rect(ev_rect, Color(0.18f, 0.55f, 0.95f, 0.35f), true);
				viewport->draw_rect(ev_rect, Color(0.35f, 0.85f, 1.0f, 0.95f), false, 2.0f);

				String ev_label = ev->has_method("get_event_name") ? String(ev->call("get_event_name")) : "EV";
				if (ev_label.begins_with("EV_")) {
					ev_label = ev_label.substr(3);
				}
				if (ev_label.length() > 8) {
					ev_label = ev_label.substr(0, 7) + "…";
				}
				if (font.is_valid() && !ev_label.is_empty()) {
					Size2 str_sz = font->get_string_size(ev_label, HORIZONTAL_ALIGNMENT_CENTER, -1, font_sz);
					Point2 text_pos = Point2(screen_tl.x + (ev_rect.size.x - str_sz.x) * 0.5f, screen_tl.y + (ev_rect.size.y + str_sz.y * 0.7f) * 0.5f);
					viewport->draw_string(font, text_pos + Point2(1, 1), ev_label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_sz, Color(0, 0, 0, 0.95f));
					viewport->draw_string(font, text_pos, ev_label, HORIZONTAL_ALIGNMENT_LEFT, -1, font_sz, Color(1, 1, 1, 1.0f));
				}
			}
		}
	}
}

static Color _clockgears_region_color(int p_region_id) {
	if (p_region_id <= 0) {
		return Color(0, 0, 0, 0);
	}
	float hue = Math::fmod(float(p_region_id * 47 % 360) / 360.0f, 1.0f);
	return Color::from_hsv(hue, 0.78f, 0.92f, 0.55f);
}

void CanvasItemEditor::_draw_clockgears_map_overlays() {
	if (!viewport) {
		return;
	}
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!scene) {
		return;
	}

	Dictionary shadow_map;
	if (scene->has_method("get_shadow_map")) {
		shadow_map = scene->call("get_shadow_map");
	} else if (scene->has_meta("cg_shadow_map")) {
		shadow_map = scene->get_meta("cg_shadow_map");
	}

	// 1. Capa de Sombras
	if (!shadow_map.is_empty()) {
		Array skeys = shadow_map.keys();
		for (int i = 0; i < skeys.size(); i++) {
			String k = skeys[i];
			if (!bool(shadow_map[k])) {
				continue;
			}
			PackedStringArray sp = k.split(",");
			if (sp.size() == 2) {
				int cx = sp[0].to_int();
				int cy = sp[1].to_int();
				Point2 cell_pos(cx * clockgears_grid_size, cy * clockgears_grid_size);
				Point2 tl = transform.xform(cell_pos);
				Point2 br = transform.xform(cell_pos + Point2(clockgears_grid_size, clockgears_grid_size));
				viewport->draw_rect(Rect2(tl, br - tl), Color(0.0f, 0.0f, 0.0f, 0.42f), true);
			}
		}
	}

	// 2. Capa de Regiones (IDs 1..255) visible cuando clockgears_region_mode está activo
	if (clockgears_region_mode) {
		Dictionary region_map;
		if (scene->has_method("get_region_map")) {
			region_map = scene->call("get_region_map");
		} else if (scene->has_meta("cg_region_map")) {
			region_map = scene->get_meta("cg_region_map");
		}

		Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("Label"));

		Array rkeys = region_map.keys();
		for (int i = 0; i < rkeys.size(); i++) {
			String k = rkeys[i];
			int rid = int(region_map[k]);
			if (rid <= 0) {
				continue;
			}
			PackedStringArray sp = k.split(",");
			if (sp.size() == 2) {
				int cx = sp[0].to_int();
				int cy = sp[1].to_int();
				Point2 cell_pos(cx * clockgears_grid_size, cy * clockgears_grid_size);
				Point2 tl = transform.xform(cell_pos);
				Point2 br = transform.xform(cell_pos + Point2(clockgears_grid_size, clockgears_grid_size));
				Rect2 rrect(tl, br - tl);
				Color rcol = _clockgears_region_color(rid);
				viewport->draw_rect(rrect, rcol, true);
				viewport->draw_rect(rrect, Color(1, 1, 1, 0.65f), false, 1.0f);
				if (font.is_valid() && rrect.size.x >= 18.0f) {
					Point2 tpos = rrect.position + rrect.size * 0.5f + Point2(-8, 5);
					viewport->draw_string(font, tpos, String::num_int64(rid), HORIZONTAL_ALIGNMENT_CENTER, -1, CLAMP(font_size, 10, 14), Color(1, 1, 1, 0.98f));
				}
			}
		}
	}

	// 2.5. Marcador de Posición Inicial del Jugador (visible en Modo Mapa y Modo Eventos)
	Object *dm_obj = Engine::get_singleton()->get_singleton_object("ClockGearsDataManager");
	if (dm_obj) {
		Dictionary sys = dm_obj->call("get_system_config");
		String cur_map_path = scene->get_scene_file_path();
		if (cur_map_path.is_empty()) {
			cur_map_path = "res://maps/map001.tscn";
		}
		String start_map = sys.get("start_map", "");
		bool has_start = bool(sys.get("has_start_pos", false));
		if (has_start && (start_map == cur_map_path || start_map.is_empty())) {
			int sx = int(sys.get("start_x", 0));
			int sy = int(sys.get("start_y", 0));
			int sdir = int(sys.get("start_dir", 2));
			Point2 cell_pos(sx * clockgears_grid_size, sy * clockgears_grid_size);
			Point2 tl = transform.xform(cell_pos);
			Point2 br = transform.xform(cell_pos + Point2(clockgears_grid_size, clockgears_grid_size));
			Rect2 srect(tl, br - tl);

			viewport->draw_rect(srect, Color(0.18f, 0.85f, 0.42f, 0.38f), true);
			viewport->draw_rect(srect, Color(0.35f, 1.0f, 0.55f, 0.98f), false, 2.5f);

			// Bandera y silueta de jugador
			Point2 pole_top = tl + Point2(srect.size.x * 0.25f, srect.size.y * 0.16f);
			Point2 pole_bot = tl + Point2(srect.size.x * 0.25f, srect.size.y * 0.84f);
			viewport->draw_line(pole_top, pole_bot, Color(1.0f, 0.92f, 0.45f, 1.0f), 2.0f);
			Vector<Point2> flag_pts;
			flag_pts.push_back(pole_top);
			flag_pts.push_back(pole_top + Point2(srect.size.x * 0.52f, srect.size.y * 0.18f));
			flag_pts.push_back(pole_top + Point2(0, srect.size.y * 0.36f));
			Vector<Color> flag_cols;
			flag_cols.push_back(Color(0.25f, 0.95f, 0.50f, 0.95f));
			flag_cols.push_back(Color(0.95f, 0.78f, 0.25f, 0.95f));
			flag_cols.push_back(Color(0.25f, 0.95f, 0.50f, 0.95f));
			viewport->draw_polygon(flag_pts, flag_cols);

			Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
			if (font.is_valid() && srect.size.x >= 20.0f) {
				String dir_arrow = "v";
				if (sdir == 4) {
					dir_arrow = "<";
				} else if (sdir == 6) {
					dir_arrow = ">";
				} else if (sdir == 8) {
					dir_arrow = "^";
				}
				String badge = "PJ " + dir_arrow;
				int fsz = CLAMP(int(10 * EDSCALE), 9, 12);
				Point2 tpos = Point2(tl.x + 3, br.y - 3);
				viewport->draw_string(font, tpos + Point2(1, 1), badge, HORIZONTAL_ALIGNMENT_LEFT, -1, fsz, Color(0, 0, 0, 0.95f));
				viewport->draw_string(font, tpos, badge, HORIZONTAL_ALIGNMENT_LEFT, -1, fsz, Color(1.0f, 0.96f, 0.65f, 1.0f));
			}

			// Si en la celda del jugador hay un tile con ★ (z_index > 3) o en Capa_Techo, dibujarlo encima del marcador de PJ en el editor
			for (int l = 0; l < 4; l++) {
				TileMapLayer *tlayer = nullptr;
				if (l == 0) tlayer = Object::cast_to<TileMapLayer>(scene->find_child("Capa_Suelo", false, false));
				else if (l == 1) tlayer = Object::cast_to<TileMapLayer>(scene->find_child("Capa_Decoracion", false, false));
				else if (l == 2) tlayer = Object::cast_to<TileMapLayer>(scene->find_child("Capa_Objetos", false, false));
				else if (l == 3) tlayer = Object::cast_to<TileMapLayer>(scene->find_child("Capa_Techo", false, false));
				if (!tlayer) continue;
				Vector2i pcell(sx, sy);
				int src_id = tlayer->get_cell_source_id(pcell);
				if (src_id == TileSet::INVALID_SOURCE) continue;
				TileData *td = tlayer->get_cell_tile_data(pcell);
				bool is_overhead = (l == 3) || (td && td->get_z_index() > 3);
				if (is_overhead) {
					Ref<TileSet> ts = tlayer->get_tile_set();
					if (ts.is_valid() && ts->has_source(src_id)) {
						Ref<TileSetAtlasSource> atlas = ts->get_source(src_id);
						if (atlas.is_valid()) {
							Ref<Texture2D> atex = atlas->get_texture();
							if (atex.is_valid()) {
								Vector2i acoords = tlayer->get_cell_atlas_coords(pcell);
								Rect2 src_r(acoords.x * clockgears_grid_size, acoords.y * clockgears_grid_size, clockgears_grid_size, clockgears_grid_size);
								viewport->draw_texture_rect_region(atex, srect, src_r);
							}
						}
					}
				}
			}
		}
	}

	if (clockgears_event_mode) {
		return;
	}

	// 3. Vista previa de Selección Rectangular en el Mapa (Ctrl+C / Ctrl+V / Supr)
	if (clockgears_has_map_selection && clockgears_map_selection_rect.size.x > 0 && clockgears_map_selection_rect.size.y > 0) {
		Point2 pos_world(clockgears_map_selection_rect.position.x * clockgears_grid_size, clockgears_map_selection_rect.position.y * clockgears_grid_size);
		Point2 sz_world(clockgears_map_selection_rect.size.x * clockgears_grid_size, clockgears_map_selection_rect.size.y * clockgears_grid_size);
		Point2 tl = transform.xform(pos_world);
		Point2 br = transform.xform(pos_world + sz_world);
		Rect2 srect(tl, br - tl);
		viewport->draw_rect(srect, Color(0.20f, 0.78f, 1.0f, 0.20f), true);
		viewport->draw_rect(srect, Color(0.30f, 0.88f, 1.0f, 0.95f), false, 2.0f);
	}

	// 4. Vista previa de arrastre de Rectángulo (1) o Elipse (2)
	if ((clockgears_is_painting || clockgears_is_erasing) && !clockgears_region_mode && (current_draw_tool == 1 || current_draw_tool == 2)) {
		int x0 = MIN(clockgears_drag_start_cell.x, clockgears_hovered_cell.x);
		int y0 = MIN(clockgears_drag_start_cell.y, clockgears_hovered_cell.y);
		int x1 = MAX(clockgears_drag_start_cell.x, clockgears_hovered_cell.x);
		int y1 = MAX(clockgears_drag_start_cell.y, clockgears_hovered_cell.y);
		Color prev_fill = clockgears_is_erasing ? Color(1.0f, 0.30f, 0.30f, 0.25f) : Color(0.95f, 0.76f, 0.24f, 0.25f);
		Color prev_border = clockgears_is_erasing ? Color(1.0f, 0.40f, 0.40f, 0.90f) : Color(1.0f, 0.86f, 0.30f, 0.90f);

		if (current_draw_tool == 1) {
			Point2 tl = transform.xform(Point2(x0 * clockgears_grid_size, y0 * clockgears_grid_size));
			Point2 br = transform.xform(Point2((x1 + 1) * clockgears_grid_size, (y1 + 1) * clockgears_grid_size));
			if (!clockgears_is_erasing && current_tileset_texture.is_valid()) {
				Vector2i bsz = current_selected_tile_block_size.max(Vector2i(1, 1));
				bool has_block = ((bsz.x > 1 || bsz.y > 1) && current_selected_tile_coords_list.size() == bsz.x * bsz.y);
				for (int ty = y0; ty <= y1; ty++) {
					for (int tx = x0; tx <= x1; tx++) {
						Vector2i ac = current_selected_tile_coords;
						if (has_block) {
							int bx = (tx - x0) % bsz.x;
							int by = (ty - y0) % bsz.y;
							ac = current_selected_tile_coords_list[by * bsz.x + bx];
						}
						Point2 c_tl = transform.xform(Point2(tx * clockgears_grid_size, ty * clockgears_grid_size));
						Point2 c_br = transform.xform(Point2((tx + 1) * clockgears_grid_size, (ty + 1) * clockgears_grid_size));
						Rect2 d_rect(c_tl, c_br - c_tl);
						Rect2 s_rect(ac.x * clockgears_grid_size, ac.y * clockgears_grid_size, clockgears_grid_size, clockgears_grid_size);
						viewport->draw_texture_rect_region(current_tileset_texture, d_rect, s_rect, Color(1.0f, 1.0f, 1.0f, 0.70f));
					}
				}
			} else {
				viewport->draw_rect(Rect2(tl, br - tl), prev_fill, true);
			}
			viewport->draw_rect(Rect2(tl, br - tl), prev_border, false, 2.0f);
		} else {
			float rx = MAX(0.5f, (x1 - x0) * 0.5f);
			float ry = MAX(0.5f, (y1 - y0) * 0.5f);
			float cx = (x0 + x1) * 0.5f;
			float cy = (y0 + y1) * 0.5f;
			for (int ty = y0; ty <= y1; ty++) {
				for (int tx = x0; tx <= x1; tx++) {
					float nx = (tx - cx) / rx;
					float ny = (ty - cy) / ry;
					if (nx * nx + ny * ny <= 1.05f) {
						Point2 tl = transform.xform(Point2(tx * clockgears_grid_size, ty * clockgears_grid_size));
						Point2 br = transform.xform(Point2((tx + 1) * clockgears_grid_size, (ty + 1) * clockgears_grid_size));
						Rect2 d_rect(tl, br - tl);
						if (!clockgears_is_erasing && current_tileset_texture.is_valid()) {
							Rect2 s_rect(current_selected_tile_coords.x * clockgears_grid_size, current_selected_tile_coords.y * clockgears_grid_size, clockgears_grid_size, clockgears_grid_size);
							viewport->draw_texture_rect_region(current_tileset_texture, d_rect, s_rect, Color(1.0f, 1.0f, 1.0f, 0.70f));
						} else {
							viewport->draw_rect(d_rect, prev_fill, true);
						}
						viewport->draw_rect(d_rect, prev_border, false, 1.0f);
					}
				}
			}
		}
	} else if (clockgears_hovered_cell.x > -900000 && (current_draw_tool == 0 || current_draw_tool == 3 || current_draw_tool == 7) && !clockgears_region_mode) {
		// 5. Vista previa del sello (stamp) multi-tile, cubo o goma bajo el cursor en tiempo real (75% opacidad)
		Vector2i bsz = (current_draw_tool == 0) ? current_selected_tile_block_size.max(Vector2i(1, 1)) : Vector2i(1, 1);
		TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
		Point2 local_tl = Point2(clockgears_hovered_cell.x * clockgears_grid_size, clockgears_hovered_cell.y * clockgears_grid_size);
		Point2 local_br = Point2((clockgears_hovered_cell.x + bsz.x) * clockgears_grid_size, (clockgears_hovered_cell.y + bsz.y) * clockgears_grid_size);
		Point2 world_tl = (layer && layer->is_inside_tree()) ? layer->to_global(local_tl) : local_tl;
		Point2 world_br = (layer && layer->is_inside_tree()) ? layer->to_global(local_br) : local_br;
		Point2 tl = transform.xform(world_tl);
		Point2 br = transform.xform(world_br);

		if (clockgears_is_erasing || current_draw_tool == 7) {
			viewport->draw_rect(Rect2(tl, br - tl), Color(1.0f, 0.30f, 0.30f, 0.25f), true);
			viewport->draw_rect(Rect2(tl, br - tl), Color(1.0f, 0.40f, 0.40f, 0.90f), false, 2.0f);
		} else if (current_tileset_texture.is_valid()) {
			bool has_block = (bsz.x > 1 || bsz.y > 1) && (current_selected_tile_coords_list.size() == bsz.x * bsz.y);
			int idx = 0;
			for (int dy = 0; dy < bsz.y; dy++) {
				for (int dx = 0; dx < bsz.x; dx++) {
					Vector2i ac = has_block ? current_selected_tile_coords_list[idx++] : current_selected_tile_coords;
					Point2 l_c_tl = Point2((clockgears_hovered_cell.x + dx) * clockgears_grid_size, (clockgears_hovered_cell.y + dy) * clockgears_grid_size);
					Point2 l_c_br = Point2((clockgears_hovered_cell.x + dx + 1) * clockgears_grid_size, (clockgears_hovered_cell.y + dy + 1) * clockgears_grid_size);
					Point2 c_tl = transform.xform((layer && layer->is_inside_tree()) ? layer->to_global(l_c_tl) : l_c_tl);
					Point2 c_br = transform.xform((layer && layer->is_inside_tree()) ? layer->to_global(l_c_br) : l_c_br);
					Rect2 d_rect(c_tl, c_br - c_tl);
					Rect2 s_rect(ac.x * clockgears_grid_size, ac.y * clockgears_grid_size, clockgears_grid_size, clockgears_grid_size);
					viewport->draw_texture_rect_region(current_tileset_texture, d_rect, s_rect, Color(1.0f, 1.0f, 1.0f, 0.75f));
				}
			}
			viewport->draw_rect(Rect2(tl, br - tl), Color(1.0f, 0.86f, 0.30f, 0.95f), false, 2.0f);
		} else {
			viewport->draw_rect(Rect2(tl, br - tl), Color(1.0f, 0.86f, 0.30f, 0.65f), false, 1.5f);
		}
	} else if (clockgears_hovered_cell.x > -900000 && current_draw_tool == 6 && !clockgears_region_mode) {
		// 6. Vista previa de la herramienta Posición Inicial del Jugador
		TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
		Point2 local_tl = Point2(clockgears_hovered_cell.x * clockgears_grid_size, clockgears_hovered_cell.y * clockgears_grid_size);
		Point2 local_br = Point2((clockgears_hovered_cell.x + 1) * clockgears_grid_size, (clockgears_hovered_cell.y + 1) * clockgears_grid_size);
		Point2 world_tl = (layer && layer->is_inside_tree()) ? layer->to_global(local_tl) : local_tl;
		Point2 world_br = (layer && layer->is_inside_tree()) ? layer->to_global(local_br) : local_br;
		Point2 tl = transform.xform(world_tl);
		Point2 br = transform.xform(world_br);
		viewport->draw_rect(Rect2(tl, br - tl), Color(0.30f, 0.95f, 0.50f, 0.25f), true);
		viewport->draw_rect(Rect2(tl, br - tl), Color(0.35f, 1.0f, 0.55f, 0.90f), false, 2.0f);
	}

	// 7. Marco de Vista Previa de Cámara (Fase 7B: Vista en vivo sobre el mapa)
	if (clockgears_camera_preview_active && clockgears_camera_preview_rect.size.x > 0 && clockgears_camera_preview_rect.size.y > 0) {
		Point2 world_tl = clockgears_camera_preview_rect.position;
		Point2 world_br = clockgears_camera_preview_rect.position + clockgears_camera_preview_rect.size;
		Point2 tl = transform.xform(world_tl);
		Point2 br = transform.xform(world_br);
		Rect2 r(tl, br - tl);
		viewport->draw_rect(r, Color(0.15f, 0.55f, 1.0f, 0.12f), true);
		viewport->draw_rect(r, Color(0.25f, 0.80f, 1.0f, 0.95f), false, 2.5f);
		float csz = 14.0f;
		Color c_corner = Color(1.0f, 0.85f, 0.25f, 1.0f);
		viewport->draw_line(tl, tl + Vector2(csz, 0), c_corner, 3.0f);
		viewport->draw_line(tl, tl + Vector2(0, csz), c_corner, 3.0f);
		viewport->draw_line(Vector2(br.x, tl.y), Vector2(br.x - csz, tl.y), c_corner, 3.0f);
		viewport->draw_line(Vector2(br.x, tl.y), Vector2(br.x, tl.y + csz), c_corner, 3.0f);
		viewport->draw_line(Vector2(tl.x, br.y), Vector2(tl.x + csz, br.y), c_corner, 3.0f);
		viewport->draw_line(Vector2(tl.x, br.y), Vector2(tl.x, br.y - csz), c_corner, 3.0f);
		viewport->draw_line(br, br - Vector2(csz, 0), c_corner, 3.0f);
		viewport->draw_line(br, br - Vector2(0, csz), c_corner, 3.0f);
	}
}

void CanvasItemEditor::set_clockgears_camera_preview(bool p_active, const Rect2 &p_rect) {
	clockgears_camera_preview_active = p_active;
	clockgears_camera_preview_rect = p_rect;
	if (viewport) {
		viewport->queue_redraw();
	}
}

void CanvasItemEditor::set_clockgears_selected_tile(int p_tile_id, const Vector2i &p_atlas_coords, const Ref<Texture2D> &p_texture, int p_source_id) {
	current_selected_tile_id = p_tile_id;
	current_selected_source_id = p_source_id;
	current_selected_tile_coords = p_atlas_coords;
	current_selected_tile_block_size = Vector2i(1, 1);
	current_selected_tile_coords_list.clear();
	current_selected_tile_coords_list.push_back(p_atlas_coords);
	current_tileset_texture = p_texture;
	update_viewport();
}

void CanvasItemEditor::set_clockgears_selected_tile_block(const Vector2i &p_primary_coords, const Vector2i &p_block_size, const Vector<Vector2i> &p_coords_list, const Ref<Texture2D> &p_texture, int p_source_id) {
	current_selected_source_id = p_source_id;
	current_selected_tile_coords = p_primary_coords;
	current_selected_tile_block_size = p_block_size.max(Vector2i(1, 1));
	current_selected_tile_coords_list = p_coords_list;
	if (current_selected_tile_coords_list.is_empty()) {
		current_selected_tile_coords_list.push_back(p_primary_coords);
	}
	current_tileset_texture = p_texture;
	update_viewport();
}

void CanvasItemEditor::set_clockgears_current_layer(int p_layer) {
	if (p_layer == 6) {
		clockgears_region_mode = true;
	} else {
		clockgears_region_mode = false;
		current_layer = CLAMP(p_layer, 1, 4);
	}
	update_viewport();
}

void CanvasItemEditor::set_clockgears_draw_tool(int p_tool) {
	current_draw_tool = p_tool;
	if (p_tool != 5) {
		clockgears_has_map_selection = false;
	}
	update_viewport();
}

void CanvasItemEditor::set_clockgears_region_mode(bool p_enabled, int p_region_id) {
	clockgears_region_mode = p_enabled;
	if (p_region_id >= 0) {
		current_selected_region_id = CLAMP(p_region_id, 0, 255);
	}
	update_viewport();
}

TileMapLayer *CanvasItemEditor::_get_or_create_tile_map_layer(int p_layer_idx) {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	Object *dm = Engine::get_singleton()->get_singleton_object("ClockGearsDataManager");
	if (!scene && dm && dm->has_method("get_editor_map")) {
		scene = Object::cast_to<Node>(dm->call("get_editor_map"));
	}
	if (!scene && SceneTree::get_singleton()) {
		Node *root = SceneTree::get_singleton()->get_root();
		if (root) {
			TypedArray<Node> maps = root->find_children("*", "ClockGearsMap", true, false);
			if (!maps.is_empty()) {
				scene = Object::cast_to<Node>(maps[0]);
			}
		}
	}
	if (scene && EditorNode::get_singleton() && !EditorNode::get_singleton()->get_edited_scene() && !scene->get_parent()) {
		EditorNode::get_singleton()->set_edited_scene(scene);
	}
	if (!scene) {
		return nullptr;
	}

	const char *cg_names[4] = { "Capa_Suelo", "Capa_Decoracion", "Capa_Objetos", "Capa_Techo" };
	int clamped_idx = CLAMP(p_layer_idx, 1, 4);
	String cg_layer_name = cg_names[clamped_idx - 1];
	String fallback_name = vformat("Layer%d", clamped_idx);
	TileMapLayer *target_layer = nullptr;

	if (scene->has_method("get_layer")) {
		target_layer = Object::cast_to<TileMapLayer>((Object *)scene->call("get_layer", clamped_idx - 1));
	}

	if (!target_layer) {
		for (int i = 0; i < scene->get_child_count(); i++) {
			TileMapLayer *tml = Object::cast_to<TileMapLayer>(scene->get_child(i));
			if (tml && (tml->get_name() == cg_layer_name || tml->get_name() == fallback_name)) {
				target_layer = tml;
				break;
			}
		}
	}

	if (!target_layer) {
		target_layer = memnew(TileMapLayer);
		target_layer->set_name(cg_layer_name);
		target_layer->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		scene->add_child(target_layer);
		target_layer->set_owner(scene);
		target_layer->set_y_sort_enabled(clamped_idx >= 2);
	} else {
		target_layer->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		if (target_layer->get_owner() != scene) {
			target_layer->set_owner(scene);
		}
	}

	Ref<TileSet> ts = target_layer->get_tile_set();
	int map_ts_id = 0;
	if (scene->has_method("get_tileset_id")) {
		map_ts_id = scene->call("get_tileset_id");
	}
	if (map_ts_id <= 0 && dm) {
		Array ts_arr = dm->call("get_category_array", 13); // CAT_TILESETS = 13
		if (!ts_arr.is_empty()) {
			map_ts_id = 1;
			if (scene->has_method("set_tileset_id")) {
				scene->call("set_tileset_id", 1);
			}
		}
	}

	if (ts.is_null()) {
		if (map_ts_id > 0 && scene->has_method("setup_tileset_from_database")) {
			scene->call("setup_tileset_from_database", map_ts_id);
			ts = target_layer->get_tile_set();
		}
	}

	if (ts.is_null()) {
		for (int i = 0; i < scene->get_child_count(); i++) {
			TileMapLayer *other = Object::cast_to<TileMapLayer>(scene->get_child(i));
			if (other && other != target_layer && other->get_tile_set().is_valid()) {
				ts = other->get_tile_set();
				target_layer->set_tile_set(ts);
				break;
			}
		}
	}

	if (ts.is_null() && current_tileset_texture.is_valid()) {
		ts.instantiate();
		ts->set_tile_size(Size2i(clockgears_grid_size, clockgears_grid_size));
		target_layer->set_tile_set(ts);
	}

	if (current_tileset_texture.is_null() && ts.is_valid() && ts->get_source_count() > 0) {
		int src_id = (current_selected_source_id >= 0 && ts->has_source(current_selected_source_id)) ? current_selected_source_id : ts->get_source_id(0);
		if (ts->has_source(src_id)) {
			Ref<TileSetAtlasSource> existing_atlas = ts->get_source(src_id);
			if (existing_atlas.is_valid() && existing_atlas->get_texture().is_valid()) {
				current_tileset_texture = existing_atlas->get_texture();
				current_selected_source_id = src_id;
				if (current_selected_tile_coords.x < 0 || current_selected_tile_coords.y < 0) {
					current_selected_tile_coords = Vector2i(0, 0);
				}
			}
		}
	}

	if (current_tileset_texture.is_valid()) {
		if (ts.is_null()) {
			ts.instantiate();
			ts->set_tile_size(Size2i(clockgears_grid_size, clockgears_grid_size));
			target_layer->set_tile_set(ts);
		}
		if (current_selected_source_id < 0) {
			current_selected_source_id = 0;
		}
		Ref<TileSetAtlasSource> atlas;
		if (ts->has_source(current_selected_source_id)) {
			atlas = ts->get_source(current_selected_source_id);
		}
		if (atlas.is_null()) {
			atlas.instantiate();
			atlas->set_texture(current_tileset_texture);
			atlas->set_texture_region_size(Size2i(clockgears_grid_size, clockgears_grid_size));
			atlas->set_use_texture_padding(true); // A4.2: Sin sangrado de bordes
			ts->add_source(atlas, current_selected_source_id);
		} else if (atlas->get_texture() != current_tileset_texture) {
			atlas->set_texture(current_tileset_texture);
			atlas->set_texture_region_size(Size2i(clockgears_grid_size, clockgears_grid_size));
			atlas->set_use_texture_padding(true);
		}
		if (atlas.is_valid()) {
			Size2 tex_sz = current_tileset_texture->get_size();
			int cols = MAX(1, int(tex_sz.x) / clockgears_grid_size);
			int rows = MAX(1, int(tex_sz.y) / clockgears_grid_size);
			for (int ry = 0; ry < rows; ry++) {
				for (int rx = 0; rx < cols; rx++) {
					Vector2i tc(rx, ry);
					if (!atlas->has_tile(tc)) {
						atlas->create_tile(tc);
					}
				}
			}
			if (current_selected_tile_coords.x >= 0 && current_selected_tile_coords.y >= 0 && !atlas->has_tile(current_selected_tile_coords)) {
				atlas->create_tile(current_selected_tile_coords);
			}
		}
		// Sincronizar TileSet compartido en todas las capas TileMapLayer del mapa
		for (int i = 0; i < scene->get_child_count(); i++) {
			TileMapLayer *other = Object::cast_to<TileMapLayer>(scene->get_child(i));
			if (other && other->get_tile_set() != ts) {
				other->set_tile_set(ts);
			}
		}
	}

	return target_layer;
}

bool CanvasItemEditor::_is_tile_star_overhead(const Vector2i &p_coords) const {
	if (p_coords.x < 0 || p_coords.y < 0) {
		return false;
	}
	Object *dm = Engine::get_singleton()->get_singleton_object("ClockGearsDataManager");
	if (!dm) {
		return false;
	}
	Array ts_arr = dm->call("get_category_array", 13); // CAT_TILESETS = 13
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	int cur_ts_id = 0;
	if (scene && scene->has_method("get_tileset_id")) {
		cur_ts_id = scene->call("get_tileset_id");
	}
	String key = vformat("%d,%d", p_coords.x, p_coords.y);
	static const char *skeys[9] = { "A5", "B", "C", "D", "E", "A1", "A2", "A3", "A4" };
	for (int i = 0; i < ts_arr.size(); i++) {
		if (ts_arr[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = ts_arr[i];
			int id = int(d.get("id", i + 1));
			if (cur_ts_id == 0 || id == cur_ts_id) {
				Dictionary slot_cols = d.get("slot_collisions", Dictionary());
				for (int s = 0; s < 9; s++) {
					if (slot_cols.has(skeys[s])) {
						Dictionary smap = slot_cols[skeys[s]];
						if (smap.has(key) && int(Dictionary(smap[key]).get("layer", 0)) == 1) {
							return true;
						}
					}
				}
				break;
			}
		}
	}
	return false;
}

void CanvasItemEditor::_record_and_set_tile_cell(TileMapLayer *p_layer, const Point2i &p_cell, int p_source_id, const Vector2i &p_atlas_coords, int p_alt) {
	if (!p_layer) {
		return;
	}
	// Requerimiento 6: no llamar a set_cell si la celda ya tiene ese mismo tile en el trazo actual
	if (p_layer->get_cell_source_id(p_cell) == p_source_id &&
			p_layer->get_cell_atlas_coords(p_cell) == p_atlas_coords &&
			p_layer->get_cell_alternative_tile(p_cell) == p_alt) {
		return;
	}
	if (!clockgears_stroke_before_cells.has(p_cell)) {
		Dictionary before;
		before["layer"] = p_layer;
		before["source_id"] = p_layer->get_cell_source_id(p_cell);
		before["atlas_coords"] = p_layer->get_cell_atlas_coords(p_cell);
		before["alt"] = p_layer->get_cell_alternative_tile(p_cell);
		clockgears_stroke_before_cells[p_cell] = before;
	}

	if (p_source_id != TileSet::INVALID_SOURCE) {
		Ref<TileSet> ts = p_layer->get_tile_set();
		if (ts.is_valid()) {
			if (!ts->has_source(p_source_id) && current_tileset_texture.is_valid()) {
				Ref<TileSetAtlasSource> atlas;
				atlas.instantiate();
				atlas->set_texture(current_tileset_texture);
				atlas->set_texture_region_size(Size2i(clockgears_grid_size, clockgears_grid_size));
				atlas->set_use_texture_padding(true);
				ts->add_source(atlas, p_source_id);
			}
			if (ts->has_source(p_source_id)) {
				Ref<TileSetAtlasSource> atlas = ts->get_source(p_source_id);
				if (atlas.is_valid()) {
					if (!atlas->has_tile(p_atlas_coords)) {
						atlas->create_tile(p_atlas_coords);
					}
					TileData *td = atlas->get_tile_data(p_atlas_coords, 0);
					if (td) {
						bool is_star = false;
						Object *dm_obj = Engine::get_singleton()->get_singleton_object("ClockGearsDataManager");
						if (dm_obj) {
							Array ts_arr = dm_obj->call("get_category_array", 13); // CAT_TILESETS = 13
							if (!ts_arr.is_empty()) {
								Dictionary d = ts_arr[0];
								Dictionary scols = d.get("slot_collisions", Dictionary());
								String key = vformat("%d,%d", p_atlas_coords.x, p_atlas_coords.y);
								static const char *skeys[9] = { "A5", "B", "C", "D", "E", "A1", "A2", "A3", "A4" };
								for (int s = 0; s < 9; s++) {
									if (scols.has(skeys[s])) {
										Dictionary sm = scols[skeys[s]];
										if (sm.has(key)) {
											Dictionary md = sm[key];
											if (int(md.get("layer", 0)) == 1 || int(md.get("mode", 0)) == 2) {
												is_star = true;
											}
											break;
										}
									}
								}
								if (!is_star && d.has("collisions")) {
									Dictionary lcols = d.get("collisions", Dictionary());
									if (lcols.has(key)) {
										Variant lm = lcols[key];
										if (lm.get_type() == Variant::DICTIONARY) {
											Dictionary md = lm;
											if (int(md.get("layer", 0)) == 1 || int(md.get("mode", 0)) == 2) {
												is_star = true;
											}
										} else if (int(lm) == 2) {
											is_star = true;
										}
									}
								}
							}
						}
						td->set_z_index(is_star ? 5 : 0);
					}
				}
			}
		}
	}

	p_layer->set_cell(p_cell, p_source_id, p_atlas_coords, p_alt);
	p_layer->update_internals();
	p_layer->queue_redraw();

	int read_source_id = p_layer->get_cell_source_id(p_cell);
	Vector2i read_coords = p_layer->get_cell_atlas_coords(p_cell);
	bool write_ok = false;
	if (p_source_id == TileSet::INVALID_SOURCE) {
		write_ok = (read_source_id == TileSet::INVALID_SOURCE);
	} else {
		write_ok = (read_source_id == p_source_id && read_coords == p_atlas_coords);
	}

	static const char *tool_names[] = { "Lápiz", "Rectángulo", "Elipse", "Cubo de Pintura", "Sombra", "Selección Rectangular", "Posición Inicial", "Goma" };
	String cur_tool_str = (current_draw_tool >= 0 && current_draw_tool < 8) ? tool_names[current_draw_tool] : "Desconocido";

	String lpath = p_layer->is_inside_tree() ? String(p_layer->get_path()) : String(p_layer->get_name());
	print_line(vformat(String::utf8("[ClockGears Draw] set_cell: Herramienta: %s, Capa: '%s' (Ruta: %s, TileSet: %s), Celda: (%d, %d), Tile: (%d, %d), Source: %d -> get_cell_source_id=%d, coords=(%d, %d), Resultado: %s"),
			cur_tool_str, p_layer->get_name(), lpath, p_layer->get_tile_set().is_valid() ? "Sí" : "No",
			p_cell.x, p_cell.y, p_atlas_coords.x, p_atlas_coords.y, p_source_id, read_source_id, read_coords.x, read_coords.y,
			write_ok ? "Éxito (OK)" : "Fallo"));

	Dictionary after;
	after["layer"] = p_layer;
	after["source_id"] = p_source_id;
	after["atlas_coords"] = p_atlas_coords;
	after["alt"] = p_alt;
	clockgears_stroke_after_cells[p_cell] = after;
}

void CanvasItemEditor::_commit_tile_stroke_undo_redo(TileMapLayer *p_layer, const String &p_action_name) {
	if (!p_layer || clockgears_stroke_after_cells.is_empty()) {
		clockgears_stroke_before_cells.clear();
		clockgears_stroke_after_cells.clear();
		return;
	}
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	if (!ur) {
		clockgears_stroke_before_cells.clear();
		clockgears_stroke_after_cells.clear();
		return;
	}

	int history_id = EditorNode::get_editor_data().get_current_edited_scene_history_id();
	ur->create_action(p_action_name, UndoRedo::MERGE_DISABLE, p_layer);

	HashSet<TileMapLayer *> affected_layers;
	Array cells = clockgears_stroke_after_cells.keys();
	for (int i = 0; i < cells.size(); i++) {
		Vector2i c = cells[i];
		Dictionary before = clockgears_stroke_before_cells.get(c, Dictionary());
		Dictionary after = clockgears_stroke_after_cells[c];
		TileMapLayer *cell_layer = Object::cast_to<TileMapLayer>(after.get("layer", Variant()));
		if (!cell_layer) {
			cell_layer = p_layer;
		}
		affected_layers.insert(cell_layer);

		int b_src = before.get("source_id", TileSet::INVALID_SOURCE);
		Vector2i b_coords = before.get("atlas_coords", TileSetSource::INVALID_ATLAS_COORDS);
		int b_alt = before.get("alt", 0);

		int a_src = after.get("source_id", TileSet::INVALID_SOURCE);
		Vector2i a_coords = after.get("atlas_coords", TileSetSource::INVALID_ATLAS_COORDS);
		int a_alt = after.get("alt", 0);

		ur->add_do_method(cell_layer, "set_cell", c, a_src, a_coords, a_alt);
		ur->add_undo_method(cell_layer, "set_cell", c, b_src, b_coords, b_alt);
	}
	for (TileMapLayer *al : affected_layers) {
		ur->add_do_method(al, "update_internals");
		ur->add_undo_method(al, "update_internals");
		ur->add_do_method(al, "queue_redraw");
		ur->add_undo_method(al, "queue_redraw");
	}
	ur->commit_action(false);
	if (EditorNode::get_singleton()) {
		ur->set_history_as_unsaved(history_id);
	}
	print_line(vformat(String::utf8("[ClockGears Draw] Acción UndoRedo confirmada: '%s' (%d celdas modificadas en historial %d)"),
			p_action_name, cells.size(), history_id));

	clockgears_stroke_before_cells.clear();
	clockgears_stroke_after_cells.clear();
}

void CanvasItemEditor::_paint_tile_cell(const Point2i &p_cell, bool p_erase) {
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		print_line(String::utf8("[ClockGears Draw] ERROR: Capa nula al pintar celda."));
		return;
	}

	if (p_erase) {
		_record_and_set_tile_cell(layer, p_cell, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, 0);
	} else {
		if (current_tileset_texture.is_null()) {
			Ref<TileSet> ts = layer->get_tile_set();
			if (ts.is_valid() && ts->get_source_count() > 0) {
				int sid = (current_selected_source_id >= 0 && ts->has_source(current_selected_source_id)) ? current_selected_source_id : ts->get_source_id(0);
				current_selected_source_id = sid;
				Ref<TileSetAtlasSource> at = ts->get_source(sid);
				if (at.is_valid() && at->get_texture().is_valid()) {
					current_tileset_texture = at->get_texture();
				}
			}
		}
		if (current_tileset_texture.is_null()) {
			print_line(String::utf8("[ClockGears Draw] AVISO: No hay textura de tileset disponible para pintar celda."));
			return; // Regla Cero: No pintar tiles de prueba si no hay tileset cargado
		}
		if (current_selected_tile_coords.x < 0 || current_selected_tile_coords.y < 0) {
			current_selected_tile_coords = Vector2i(0, 0);
		}
		Vector2i bsz = current_selected_tile_block_size.max(Vector2i(1, 1));
		if ((bsz.x > 1 || bsz.y > 1) && current_selected_tile_coords_list.size() == bsz.x * bsz.y) {
			int idx = 0;
			for (int dy = 0; dy < bsz.y; dy++) {
				for (int dx = 0; dx < bsz.x; dx++) {
					Vector2i ac = current_selected_tile_coords_list[idx++];
					TileMapLayer *target_layer = _is_tile_star_overhead(ac) ? _get_or_create_tile_map_layer(4) : layer;
					_record_and_set_tile_cell(target_layer, p_cell + Point2i(dx, dy), current_selected_source_id, ac, 0);
				}
			}
		} else {
			TileMapLayer *target_layer = _is_tile_star_overhead(current_selected_tile_coords) ? _get_or_create_tile_map_layer(4) : layer;
			_record_and_set_tile_cell(target_layer, p_cell, current_selected_source_id, current_selected_tile_coords, 0);
		}
	}

	layer->update_internals();
	layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::_paint_tile_line(const Point2i &p_from, const Point2i &p_to, bool p_erase) {
	int dx = Math::abs(p_to.x - p_from.x);
	int dy = Math::abs(p_to.y - p_from.y);
	int sx = (p_from.x < p_to.x) ? 1 : -1;
	int sy = (p_from.y < p_to.y) ? 1 : -1;
	int err = dx - dy;
	Point2i curr = p_from;

	while (true) {
		_paint_tile_cell(curr, p_erase);
		if (curr == p_to) {
			break;
		}
		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			curr.x += sx;
		}
		if (e2 < dx) {
			err += dx;
			curr.y += sy;
		}
	}
}

void CanvasItemEditor::_paint_tile_rect(const Point2i &p_from, const Point2i &p_to, bool p_erase) {
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		print_line(String::utf8("[ClockGears Draw] ERROR: Capa nula al pintar rectángulo."));
		return;
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		Ref<TileSet> ts = layer->get_tile_set();
		if (ts.is_valid() && ts->get_source_count() > 0) {
			int sid = (current_selected_source_id >= 0 && ts->has_source(current_selected_source_id)) ? current_selected_source_id : ts->get_source_id(0);
			current_selected_source_id = sid;
			Ref<TileSetAtlasSource> at = ts->get_source(sid);
			if (at.is_valid() && at->get_texture().is_valid()) {
				current_tileset_texture = at->get_texture();
			}
		}
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		print_line(String::utf8("[ClockGears Draw] AVISO: No hay textura de tileset disponible para pintar rectángulo."));
		return;
	}
	if (current_selected_tile_coords.x < 0 || current_selected_tile_coords.y < 0) {
		current_selected_tile_coords = Vector2i(0, 0);
	}
	int x0 = MIN(p_from.x, p_to.x);
	int y0 = MIN(p_from.y, p_to.y);
	int x1 = MAX(p_from.x, p_to.x);
	int y1 = MAX(p_from.y, p_to.y);
	Vector2i bsz = current_selected_tile_block_size.max(Vector2i(1, 1));
	bool has_block = ((bsz.x > 1 || bsz.y > 1) && current_selected_tile_coords_list.size() == bsz.x * bsz.y);

	for (int ty = y0; ty <= y1; ty++) {
		for (int tx = x0; tx <= x1; tx++) {
			Point2i cell(tx, ty);
			if (p_erase) {
				_record_and_set_tile_cell(layer, cell, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, 0);
			} else if (has_block) {
				int bx = (tx - x0) % bsz.x;
				int by = (ty - y0) % bsz.y;
				Vector2i ac = current_selected_tile_coords_list[by * bsz.x + bx];
				TileMapLayer *target_layer = _is_tile_star_overhead(ac) ? _get_or_create_tile_map_layer(4) : layer;
				_record_and_set_tile_cell(target_layer, cell, current_selected_source_id, ac, 0);
			} else {
				TileMapLayer *target_layer = _is_tile_star_overhead(current_selected_tile_coords) ? _get_or_create_tile_map_layer(4) : layer;
				_record_and_set_tile_cell(target_layer, cell, current_selected_source_id, current_selected_tile_coords, 0);
			}
		}
	}
	layer->update_internals();
	layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::_paint_tile_ellipse(const Point2i &p_from, const Point2i &p_to, bool p_erase) {
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		print_line(String::utf8("[ClockGears Draw] ERROR: Capa nula al pintar elipse."));
		return;
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		Ref<TileSet> ts = layer->get_tile_set();
		if (ts.is_valid() && ts->get_source_count() > 0) {
			int sid = (current_selected_source_id >= 0 && ts->has_source(current_selected_source_id)) ? current_selected_source_id : ts->get_source_id(0);
			current_selected_source_id = sid;
			Ref<TileSetAtlasSource> at = ts->get_source(sid);
			if (at.is_valid() && at->get_texture().is_valid()) {
				current_tileset_texture = at->get_texture();
			}
		}
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		print_line(String::utf8("[ClockGears Draw] AVISO: No hay textura de tileset disponible para pintar elipse."));
		return;
	}
	if (current_selected_tile_coords.x < 0 || current_selected_tile_coords.y < 0) {
		current_selected_tile_coords = Vector2i(0, 0);
	}
	int x0 = MIN(p_from.x, p_to.x);
	int y0 = MIN(p_from.y, p_to.y);
	int x1 = MAX(p_from.x, p_to.x);
	int y1 = MAX(p_from.y, p_to.y);
	float rx = MAX(0.5f, (x1 - x0) * 0.5f);
	float ry = MAX(0.5f, (y1 - y0) * 0.5f);
	float cx = (x0 + x1) * 0.5f;
	float cy = (y0 + y1) * 0.5f;

	for (int ty = y0; ty <= y1; ty++) {
		for (int tx = x0; tx <= x1; tx++) {
			float nx = (tx - cx) / rx;
			float ny = (ty - cy) / ry;
			if (nx * nx + ny * ny <= 1.05f) {
				Point2i cell(tx, ty);
				if (p_erase) {
					_record_and_set_tile_cell(layer, cell, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, 0);
				} else {
					TileMapLayer *target_layer = _is_tile_star_overhead(current_selected_tile_coords) ? _get_or_create_tile_map_layer(4) : layer;
					_record_and_set_tile_cell(target_layer, cell, current_selected_source_id, current_selected_tile_coords, 0);
				}
			}
		}
	}
	layer->update_internals();
	layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::_paint_tile_bucket(const Point2i &p_seed, bool p_erase) {
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		print_line(String::utf8("[ClockGears Draw] ERROR: Capa nula al rellenar con cubo."));
		return;
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		Ref<TileSet> ts = layer->get_tile_set();
		if (ts.is_valid() && ts->get_source_count() > 0) {
			int sid = (current_selected_source_id >= 0 && ts->has_source(current_selected_source_id)) ? current_selected_source_id : ts->get_source_id(0);
			current_selected_source_id = sid;
			Ref<TileSetAtlasSource> at = ts->get_source(sid);
			if (at.is_valid() && at->get_texture().is_valid()) {
				current_tileset_texture = at->get_texture();
			}
		}
	}
	if (!p_erase && current_tileset_texture.is_null()) {
		print_line(String::utf8("[ClockGears Draw] AVISO: No hay textura de tileset disponible para pintar cubo."));
		return;
	}
	if (current_selected_tile_coords.x < 0 || current_selected_tile_coords.y < 0) {
		current_selected_tile_coords = Vector2i(0, 0);
	}

	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	Vector2i map_sz(25, 20);
	if (scene && scene->has_method("get_map_size")) {
		map_sz = scene->call("get_map_size");
	}
	int min_x = MIN(0, p_seed.x - 32);
	int min_y = MIN(0, p_seed.y - 32);
	int max_x = MAX(map_sz.x - 1, p_seed.x + 32);
	int max_y = MAX(map_sz.y - 1, p_seed.y + 32);
	if (p_seed.x >= 0 && p_seed.y >= 0 && p_seed.x < map_sz.x && p_seed.y < map_sz.y) {
		min_x = 0;
		min_y = 0;
		max_x = map_sz.x - 1;
		max_y = map_sz.y - 1;
	}

	int target_src = layer->get_cell_source_id(p_seed);
	Vector2i target_coords = layer->get_cell_atlas_coords(p_seed);
	int new_src = p_erase ? TileSet::INVALID_SOURCE : current_selected_source_id;
	Vector2i new_coords = p_erase ? TileSetSource::INVALID_ATLAS_COORDS : current_selected_tile_coords;

	if (target_src == new_src && target_coords == new_coords) {
		return;
	}

	TileMapLayer *target_layer = (!p_erase && _is_tile_star_overhead(new_coords)) ? _get_or_create_tile_map_layer(4) : layer;

	Vector<Point2i> queue;
	HashSet<Point2i> visited;
	queue.push_back(p_seed);
	visited.insert(p_seed);

	int head = 0;
	const int max_cells = 4096;
	while (head < queue.size() && head < max_cells) {
		Point2i curr = queue[head++];
		_record_and_set_tile_cell(target_layer, curr, new_src, new_coords, 0);

		const Point2i dirs[4] = { Point2i(1, 0), Point2i(-1, 0), Point2i(0, 1), Point2i(0, -1) };
		for (int d = 0; d < 4; d++) {
			Point2i nb = curr + dirs[d];
			if (nb.x < min_x || nb.y < min_y || nb.x > max_x || nb.y > max_y) {
				continue;
			}
			if (visited.has(nb)) {
				continue;
			}
			if (layer->get_cell_source_id(nb) == target_src && layer->get_cell_atlas_coords(nb) == target_coords) {
				visited.insert(nb);
				queue.push_back(nb);
			}
		}
	}

	target_layer->update_internals();
	target_layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::_paint_shadow_cell(const Point2i &p_cell, bool p_erase) {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!scene) {
		return;
	}
	bool new_val = !p_erase;
	if (scene->has_method("set_cell_shadow")) {
		scene->call("set_cell_shadow", Vector2i(p_cell), new_val);
	} else {
		Dictionary sm = scene->has_meta("cg_shadow_map") ? Dictionary(scene->get_meta("cg_shadow_map")) : Dictionary();
		String key = vformat("%d,%d", p_cell.x, p_cell.y);
		if (new_val) {
			sm[key] = true;
		} else {
			sm.erase(key);
		}
		scene->set_meta("cg_shadow_map", sm);
	}
	viewport->queue_redraw();
}

void CanvasItemEditor::_paint_region_cell(const Point2i &p_cell, bool p_erase) {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!scene) {
		return;
	}
	int rid = p_erase ? 0 : current_selected_region_id;
	if (scene->has_method("set_cell_region")) {
		scene->call("set_cell_region", Vector2i(p_cell), rid);
	} else {
		Dictionary rm = scene->has_meta("cg_region_map") ? Dictionary(scene->get_meta("cg_region_map")) : Dictionary();
		String key = vformat("%d,%d", p_cell.x, p_cell.y);
		if (rid <= 0) {
			rm.erase(key);
		} else {
			rm[key] = rid;
		}
		scene->set_meta("cg_region_map", rm);
	}
	viewport->queue_redraw();
}

void CanvasItemEditor::clockgears_copy_map_selection() {
	if (!clockgears_has_map_selection) {
		return;
	}
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		return;
	}
	Dictionary cells;
	Rect2i r = clockgears_map_selection_rect;
	for (int dy = 0; dy < r.size.y; dy++) {
		for (int dx = 0; dx < r.size.x; dx++) {
			Point2i c = r.position + Point2i(dx, dy);
			Dictionary cd;
			cd["source_id"] = layer->get_cell_source_id(c);
			cd["atlas_coords"] = layer->get_cell_atlas_coords(c);
			cd["alt"] = layer->get_cell_alternative_tile(c);
			cells[vformat("%d,%d", dx, dy)] = cd;
		}
	}
	clockgears_map_clipboard.clear();
	clockgears_map_clipboard["size"] = r.size;
	clockgears_map_clipboard["cells"] = cells;
}

void CanvasItemEditor::clockgears_paste_map_selection() {
	if (clockgears_map_clipboard.is_empty()) {
		return;
	}
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		return;
	}
	Vector2i sz = clockgears_map_clipboard.get("size", Vector2i(1, 1));
	Dictionary cells = clockgears_map_clipboard.get("cells", Dictionary());
	Point2i origin = (clockgears_hovered_cell.x > -900000) ? clockgears_hovered_cell : clockgears_map_selection_rect.position;

	clockgears_stroke_before_cells.clear();
	clockgears_stroke_after_cells.clear();
	for (int dy = 0; dy < sz.y; dy++) {
		for (int dx = 0; dx < sz.x; dx++) {
			String k = vformat("%d,%d", dx, dy);
			if (!cells.has(k)) {
				continue;
			}
			Dictionary cd = cells[k];
			int src = cd.get("source_id", TileSet::INVALID_SOURCE);
			Vector2i ac = cd.get("atlas_coords", TileSetSource::INVALID_ATLAS_COORDS);
			int alt = cd.get("alt", 0);
			_record_and_set_tile_cell(layer, origin + Point2i(dx, dy), src, ac, alt);
		}
	}
	_commit_tile_stroke_undo_redo(layer, TTR("Pegar Selección de Mapa"));
	layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::clockgears_delete_map_selection() {
	if (!clockgears_has_map_selection) {
		return;
	}
	TileMapLayer *layer = _get_or_create_tile_map_layer(current_layer);
	if (!layer) {
		return;
	}
	clockgears_stroke_before_cells.clear();
	clockgears_stroke_after_cells.clear();
	Rect2i r = clockgears_map_selection_rect;
	for (int dy = 0; dy < r.size.y; dy++) {
		for (int dx = 0; dx < r.size.x; dx++) {
			_record_and_set_tile_cell(layer, r.position + Point2i(dx, dy), TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS, 0);
		}
	}
	_commit_tile_stroke_undo_redo(layer, TTR("Borrar Selección de Mapa"));
	layer->queue_redraw();
	viewport->queue_redraw();
}

void CanvasItemEditor::simulate_mouse_press(const Point2 &p_canvas_pos, bool p_right_button) {
	Ref<InputEventMouseButton> mb;
	mb.instantiate();
	mb->set_button_index(p_right_button ? MouseButton::RIGHT : MouseButton::LEFT);
	mb->set_pressed(true);
	mb->set_position(transform.xform(p_canvas_pos));
	_gui_input_viewport(mb);
}

void CanvasItemEditor::simulate_mouse_motion(const Point2 &p_canvas_pos) {
	Ref<InputEventMouseMotion> mm;
	mm.instantiate();
	mm->set_position(transform.xform(p_canvas_pos));
	_gui_input_viewport(mm);
}

void CanvasItemEditor::simulate_mouse_release(const Point2 &p_canvas_pos, bool p_right_button) {
	Ref<InputEventMouseButton> mb;
	mb.instantiate();
	mb->set_button_index(p_right_button ? MouseButton::RIGHT : MouseButton::LEFT);
	mb->set_pressed(false);
	mb->set_position(transform.xform(p_canvas_pos));
	_gui_input_viewport(mb);
}

void CanvasItemEditor::simulate_gui_input(const Ref<InputEvent> &p_event) {
	_gui_input_viewport(p_event);
}

void CanvasItemEditor::trigger_undo() {
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager::get_singleton()->undo();
	}
}

void CanvasItemEditor::trigger_redo() {
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager::get_singleton()->redo();
	}
}

void CanvasItemEditor::focus_selection() {
	_focus_selection(VIEW_CENTER_TO_SELECTION);
}

void CanvasItemEditor::center_at(const Point2 &p_pos) {
	Vector2 offset = viewport->get_size() / 2 - EditorNode::get_singleton()->get_scene_root()->get_global_canvas_transform().xform(p_pos);
	view_offset = (view_offset - offset / zoom).round();
	ci_manipulator->set_view_offset(view_offset);
	update_viewport();
}

CanvasItemEditor::CanvasItemEditor() {
	set_name(TTRC("2D"));
	set_icon_name("2D");
	set_available_layouts(EditorDock::DOCK_LAYOUT_MAIN_SCREEN | EditorDock::DOCK_LAYOUT_FLOATING);
	set_default_slot(EditorDock::DOCK_SLOT_MAIN_SCREEN);
	set_dock_shortcut(ED_GET_SHORTCUT("editor/editor_2d"));
	set_allow_switch_screen(true);

	editor_selection = EditorNode::get_singleton()->get_editor_selection();
	editor_selection->add_editor_plugin(this);
	editor_selection->connect("selection_changed", callable_mp((CanvasItem *)this, &CanvasItem::queue_redraw));
	editor_selection->connect("selection_changed", callable_mp(this, &CanvasItemEditor::_selection_changed));

	VBoxContainer *main_vb = memnew(VBoxContainer);
	add_child(main_vb);

	toolbar_margin = memnew(MarginContainer);
	toolbar_margin->set_theme_type_variation("MainToolBarMargin");
	main_vb->add_child(toolbar_margin);

	// A fluid container for all toolbars.
	HFlowContainer *main_flow = memnew(HFlowContainer);
	toolbar_margin->add_child(main_flow);

	// Main toolbars.
	// Split into separate `HBoxContainer` so they can wrap onto multiple lines as the window width decreases (the parent is a `FlowContainer`).
	// These are not grouped by any particular criteria. Only some of the end children are grouped separately, based on their separators.
	HBoxContainer *input_hbox = EditorToolbarGroup::create(main_flow);

	bottom_split = memnew(VSplitContainer);
	main_vb->add_child(bottom_split);
	bottom_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	left_panel_split = memnew(HSplitContainer);
	bottom_split->add_child(left_panel_split);
	left_panel_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	right_panel_split = memnew(HSplitContainer);
	left_panel_split->add_child(right_panel_split);
	right_panel_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	viewport_scrollable = memnew(Control);
	right_panel_split->add_child(viewport_scrollable);
	viewport_scrollable->set_mouse_filter(MOUSE_FILTER_PASS);
	viewport_scrollable->set_clip_contents(true);
	viewport_scrollable->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	viewport_scrollable->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	viewport_scrollable->connect(SceneStringName(draw), callable_mp(this, &CanvasItemEditor::_update_scrollbars));

	SubViewportContainer *scene_tree = memnew(SubViewportContainer);
	viewport_scrollable->add_child(scene_tree);
	scene_tree->set_stretch(true);
	scene_tree->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	scene_tree->add_child(EditorNode::get_singleton()->get_scene_root());

	controls_vb = memnew(VBoxContainer);
	controls_vb->set_begin(Point2(5, 5) * EDSCALE);

	info_panel_container = memnew(VBoxContainer);
	info_panel_container->set_anchors_and_offsets_preset(Control::PRESET_BOTTOM_RIGHT);
	info_panel_container->set_grow_direction_preset(Control::PRESET_BOTTOM_RIGHT);
	info_panel_container->set_end(Point2(5, -5) * EDSCALE);

	frame_time_panel_container = memnew(VBoxContainer);
	frame_time_panel_container->set_anchors_and_offsets_preset(Control::PRESET_TOP_RIGHT);
	frame_time_panel_container->set_grow_direction_preset(Control::PRESET_TOP_RIGHT);
	frame_time_panel_container->set_end(Point2(5, 5) * EDSCALE);

	ED_SHORTCUT("canvas_item_editor/cancel_transform", TTRC("Cancel Transformation"), Key::ESCAPE);

	// To ensure that scripts can parse the list of shortcuts correctly, we have to define
	// those shortcuts one by one. Define shortcut before using it (by EditorZoomWidget).
	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_3.125_percent", TTRC("Zoom to 3.125%"),
			{ int32_t(KeyModifierMask::SHIFT | Key::KEY_5), int32_t(KeyModifierMask::SHIFT | Key::KP_5) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_6.25_percent", TTRC("Zoom to 6.25%"),
			{ int32_t(KeyModifierMask::SHIFT | Key::KEY_4), int32_t(KeyModifierMask::SHIFT | Key::KP_4) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_12.5_percent", TTRC("Zoom to 12.5%"),
			{ int32_t(KeyModifierMask::SHIFT | Key::KEY_3), int32_t(KeyModifierMask::SHIFT | Key::KP_3) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_25_percent", TTRC("Zoom to 25%"),
			{ int32_t(KeyModifierMask::SHIFT | Key::KEY_2), int32_t(KeyModifierMask::SHIFT | Key::KP_2) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_50_percent", TTRC("Zoom to 50%"),
			{ int32_t(KeyModifierMask::SHIFT | Key::KEY_1), int32_t(KeyModifierMask::SHIFT | Key::KP_1) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_100_percent", TTRC("Zoom to 100%"),
			{ int32_t(Key::KEY_1), int32_t(KeyModifierMask::CMD_OR_CTRL | Key::KEY_0), int32_t(Key::KP_1), int32_t(KeyModifierMask::CMD_OR_CTRL | Key::KP_0) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_200_percent", TTRC("Zoom to 200%"),
			{ int32_t(Key::KEY_2), int32_t(Key::KP_2) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_400_percent", TTRC("Zoom to 400%"),
			{ int32_t(Key::KEY_3), int32_t(Key::KP_3) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_800_percent", TTRC("Zoom to 800%"),
			{ int32_t(Key::KEY_4), int32_t(Key::KP_4) });

	ED_SHORTCUT_ARRAY("canvas_item_editor/zoom_1600_percent", TTRC("Zoom to 1600%"),
			{ int32_t(Key::KEY_5), int32_t(Key::KP_5) });

	HBoxContainer *controls_hb = memnew(HBoxContainer);
	controls_vb->add_child(controls_hb);

	button_center_view = memnew(Button);
	controls_hb->add_child(button_center_view);
	button_center_view->set_flat(true);
	button_center_view->set_tooltip_text(TTR("Center View"));
	button_center_view->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(VIEW_CENTER_TO_SELECTION));

	zoom_widget = memnew(EditorZoomWidget);
	zoom_widget->set_anchors_and_offsets_preset(Control::PRESET_TOP_LEFT, Control::PRESET_MODE_MINSIZE, 2 * EDSCALE);
	zoom_widget->set_shortcut_context(this);
	controls_hb->add_child(zoom_widget);
	zoom_widget->connect("zoom_changed", callable_mp(this, &CanvasItemEditor::_update_zoom));

	EditorTranslationPreviewButton *translation_preview_button = memnew(EditorTranslationPreviewButton);
	translation_preview_button->set_flat(true);
	translation_preview_button->add_theme_constant_override("outline_size", Math::ceil(2 * EDSCALE));
	translation_preview_button->add_theme_color_override("font_outline_color", Color(0, 0, 0));
	translation_preview_button->add_theme_color_override(SceneStringName(font_color), Color(1, 1, 1));
	controls_hb->add_child(translation_preview_button);

	panner.instantiate();
	panner->set_callbacks(callable_mp(this, &CanvasItemEditor::_pan_callback), callable_mp(this, &CanvasItemEditor::_zoom_callback));

	viewport = memnew(CanvasItemEditorViewport(this));
	viewport->set_mouse_filter(MOUSE_FILTER_PASS);
	viewport->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	viewport->set_clip_contents(true);
	viewport->set_focus_mode(FOCUS_ALL);
	viewport_scrollable->add_child(viewport);
	viewport->connect(SceneStringName(draw), callable_mp(this, &CanvasItemEditor::_draw_viewport));
	viewport->connect(SceneStringName(gui_input), callable_mp(this, &CanvasItemEditor::_gui_input_viewport));
	viewport->connect(SceneStringName(focus_exited), callable_mp(panner.ptr(), &ViewPanner::release_pan_key));

	h_scroll = memnew(HScrollBar);
	viewport->add_child(h_scroll);
	h_scroll->connect(SceneStringName(value_changed), callable_mp(this, &CanvasItemEditor::_update_scroll));
	h_scroll->hide();

	v_scroll = memnew(VScrollBar);
	viewport->add_child(v_scroll);
	v_scroll->connect(SceneStringName(value_changed), callable_mp(this, &CanvasItemEditor::_update_scroll));
	v_scroll->hide();

	viewport->add_child(controls_vb);
	viewport->add_child(info_panel_container);
	viewport->add_child(frame_time_panel_container);

	ci_manipulator.instantiate();
	ci_manipulator->set_editor_mode_enabled(true);
	ci_manipulator->set_scale(EDSCALE);
	// Enable pixel snapping even if pixel snap rendering is disabled in the Project Settings.
	// This results in crisper visuals by preventing 2D nodes from being placed at subpixel coordinates.
	ci_manipulator->set_snap_mode(SNAP_NODE_PARENT | SNAP_NODE_ANCHORS | SNAP_NODE_SIDES | SNAP_NODE_CENTER | SNAP_OTHER_NODES | SNAP_GUIDES | SNAP_PIXEL);
	ci_manipulator->connect("box_selected", callable_mp(this, &CanvasItemEditor::_box_selected));
	ci_manipulator->connect("box_selection_updated", callable_mp(this, &CanvasItemEditor::_box_selection_updated));
	ci_manipulator->connect("clear_selection_requested", callable_mp(this, &CanvasItemEditor::_clear_selection_requested));
	ci_manipulator->connect("selection_menu_requested", callable_mp(this, &CanvasItemEditor::_selection_menu_requested));
	ci_manipulator->connect("update_canvas_requested", callable_mp((CanvasItem *)viewport, &CanvasItem::queue_redraw));
	ci_manipulator->connect("tool_menu_requested", callable_mp(this, &CanvasItemEditor::_tool_menu_requested));
	ci_manipulator->connect("scene_double_clicked", callable_mp(EditorNode::get_singleton(), &EditorNode::open_scene).bind(false, false, false));
	ci_manipulator->connect("save_canvas_state_requested", callable_mp(this, &CanvasItemEditor::_save_canvas_state_requested));
	ci_manipulator->connect("restore_canvas_state_requested", callable_mp(this, &CanvasItemEditor::_restore_canvas_state_requested));
	ci_manipulator->connect("commit_canvas_state_requested", callable_mp(this, &CanvasItemEditor::_commit_canvas_state_requested));
	ci_manipulator->connect("commit_guide_meta_requested", callable_mp(this, &CanvasItemEditor::_commit_guide_meta_requested));
	ci_manipulator->connect("cursor_shape_changed", callable_mp(this, &CanvasItemEditor::_update_cursor));
	ci_manipulator->connect("warning_emitted", callable_mp(EditorToaster::get_singleton(), &EditorToaster::popup_str).bind(EditorToaster::SEVERITY_WARNING, ""));

	select_button = memnew(Button);
	select_button->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	select_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(select_button);
	select_button->set_toggle_mode(true);
	select_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_SELECT));
	select_button->set_pressed(true);
	select_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/select_mode", TTRC("Select Mode"), Key::Q, true));
	select_button->set_shortcut_context(this);
	select_button->set_accessibility_name(TTRC("Select Mode"));

	scene_paint_button = memnew(Button);
	scene_paint_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(scene_paint_button);
	scene_paint_button->set_toggle_mode(true);
	scene_paint_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_SCENE_PAINT));
	scene_paint_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/scene_paint_mode", TTRC("Scene Paint Mode"), Key::B, true));
	scene_paint_button->set_shortcut_context(this);
	scene_paint_button->set_accessibility_name(TTRC("Scene Paint Mode"));

	VSeparator *vsep = memnew(VSeparator);
	vsep->set_theme_type_variation("VSeparatorButtonGroup");
	input_hbox->add_child(vsep);

	move_button = memnew(Button);
	move_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(move_button);
	move_button->set_toggle_mode(true);
	move_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_MOVE));
	move_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/move_mode", TTRC("Move Mode"), Key::W, true));
	move_button->set_shortcut_context(this);
	move_button->set_accessibility_name(TTRC("Move Mode"));

	rotate_button = memnew(Button);
	rotate_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(rotate_button);
	rotate_button->set_toggle_mode(true);
	rotate_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_ROTATE));
	rotate_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/rotate_mode", TTRC("Rotate Mode"), Key::E, true));
	rotate_button->set_shortcut_context(this);
	rotate_button->set_accessibility_name(TTRC("Rotate Mode"));

	scale_button = memnew(Button);
	scale_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(scale_button);
	scale_button->set_toggle_mode(true);
	scale_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_SCALE));
	scale_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/scale_mode", TTRC("Scale Mode"), Key::R, true));
	scale_button->set_shortcut_context(this);
	scale_button->set_accessibility_name(TTRC("Scale Mode"));

	vsep = memnew(VSeparator);
	vsep->set_theme_type_variation("VSeparatorButtonGroup");
	input_hbox->add_child(vsep);

	list_select_button = memnew(Button);
	list_select_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(list_select_button);
	list_select_button->set_toggle_mode(true);
	list_select_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_LIST_SELECT));
	list_select_button->set_accessibility_name(TTRC("List Selectable Nodes"));

	pivot_button = memnew(Button);
	pivot_button->set_tooltip_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	pivot_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(pivot_button);
	pivot_button->set_toggle_mode(true);
	pivot_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_EDIT_PIVOT));
	pivot_button->set_accessibility_name(TTRC("Change Pivot"));

	pan_button = memnew(Button);
	pan_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(pan_button);
	pan_button->set_toggle_mode(true);
	pan_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_PAN));
	pan_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/pan_mode", TTRC("Pan Mode"), Key::G));
	pan_button->set_shortcut_context(this);
	pan_button->set_accessibility_name(TTRC("Pan View"));

	ruler_button = memnew(Button);
	ruler_button->set_theme_type_variation(SceneStringName(FlatButton));
	input_hbox->add_child(ruler_button);
	ruler_button->set_toggle_mode(true);
	ruler_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_button_tool_select).bind(CanvasItemManipulator::TOOL_RULER));
	ruler_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/ruler_mode", TTRC("Ruler Mode"), Key::M));
	ruler_button->set_shortcut_context(this);
	ruler_button->set_accessibility_name(TTRC("Ruler Mode"));

	main_flow->add_child(memnew(VSeparator));

	HBoxContainer *selection_control_hbox = memnew(HBoxContainer);
	selection_control_hbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_flow->add_child(selection_control_hbox);

	lock_button = memnew(Button);
	lock_button->set_theme_type_variation(SceneStringName(FlatButton));
	lock_button->set_accessibility_name(TTRC("Lock"));
	selection_control_hbox->add_child(lock_button);

	lock_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(LOCK_SELECTED));
	lock_button->set_tooltip_text(TTRC("Lock selected node, preventing selection and movement."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	lock_button->set_shortcut(ED_GET_SHORTCUT("editor/lock_selected_nodes"));

	unlock_button = memnew(Button);
	unlock_button->set_accessibility_name(TTRC("Unlock"));
	unlock_button->set_theme_type_variation(SceneStringName(FlatButton));
	selection_control_hbox->add_child(unlock_button);
	unlock_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(UNLOCK_SELECTED));
	unlock_button->set_tooltip_text(TTRC("Unlock selected node, allowing selection and movement."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	unlock_button->set_shortcut(ED_GET_SHORTCUT("editor/unlock_selected_nodes"));

	group_button = memnew(Button);
	group_button->set_accessibility_name(TTRC("Group"));
	group_button->set_theme_type_variation(SceneStringName(FlatButton));
	selection_control_hbox->add_child(group_button);
	group_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(GROUP_SELECTED));
	group_button->set_tooltip_text(TTRC("Groups the selected node with its children. This causes the parent to be selected when any child node is clicked in 2D and 3D view."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	group_button->set_shortcut(ED_GET_SHORTCUT("editor/group_selected_nodes"));

	ungroup_button = memnew(Button);
	ungroup_button->set_accessibility_name(TTRC("Ungroup"));
	ungroup_button->set_theme_type_variation(SceneStringName(FlatButton));
	selection_control_hbox->add_child(ungroup_button);
	ungroup_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(UNGROUP_SELECTED));
	ungroup_button->set_tooltip_text(TTRC("Ungroups the selected node from its children. Child nodes will be individual items in 2D and 3D view."));
	// Define the shortcut globally (without a context) so that it works if the Scene tree dock is currently focused.
	ungroup_button->set_shortcut(ED_GET_SHORTCUT("editor/ungroup_selected_nodes"));

	main_flow->add_child(memnew(VSeparator));

	HBoxContainer *tool_button_hbox = memnew(HBoxContainer);
	tool_button_hbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_flow->add_child(tool_button_hbox);

	local_space_button = memnew(Button);
	local_space_button->set_theme_type_variation(SceneStringName(FlatButton));
	tool_button_hbox->add_child(local_space_button);
	local_space_button->set_toggle_mode(true);
	local_space_button->set_pressed_no_signal(true);
	local_space_button->connect(SceneStringName(toggled), callable_mp(this, &CanvasItemEditor::_button_toggle_local_space));
	local_space_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/use_local_space", TTRC("Use Local Space"), Key::T));
	local_space_button->set_shortcut_context(this);
	local_space_button->set_accessibility_name(TTRC("Use Local Space"));

	main_flow->add_child(memnew(VSeparator));

	HBoxContainer *snap_hbox = EditorToolbarGroup::create(main_flow);

	smart_snap_button = memnew(Button);
	smart_snap_button->set_theme_type_variation(SceneStringName(FlatButton));
	snap_hbox->add_child(smart_snap_button);
	smart_snap_button->set_toggle_mode(true);
	smart_snap_button->connect(SceneStringName(toggled), callable_mp(this, &CanvasItemEditor::_button_toggle_smart_snap));
	smart_snap_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/use_smart_snap", TTRC("Use Smart Snap"), KeyModifierMask::SHIFT | Key::S));
	smart_snap_button->set_shortcut_context(this);

	grid_snap_button = memnew(Button);
	grid_snap_button->set_theme_type_variation(SceneStringName(FlatButton));
	snap_hbox->add_child(grid_snap_button);
	grid_snap_button->set_toggle_mode(true);
	grid_snap_button->connect(SceneStringName(toggled), callable_mp(this, &CanvasItemEditor::_button_toggle_grid_snap));
	grid_snap_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/use_grid_snap", TTRC("Use Grid Snap"), KeyModifierMask::SHIFT | Key::G));
	grid_snap_button->set_shortcut_context(this);

	snap_config_menu = memnew(MenuButton);
	snap_config_menu->set_flat(false);
	snap_config_menu->set_theme_type_variation("FlatMenuButton");
	snap_config_menu->set_shortcut_context(this);
	snap_hbox->add_child(snap_config_menu);
	snap_config_menu->set_h_size_flags(SIZE_SHRINK_END);
	snap_config_menu->set_tooltip_text(TTRC("Snapping Options"));
	snap_config_menu->set_switch_on_hover(true);

	PopupMenu *p = snap_config_menu->get_popup();
	p->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));
	p->set_hide_on_checkable_item_selection(false);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/use_rotation_snap", TTRC("Use Rotation Snap")), SNAP_USE_ROTATION);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/use_scale_snap", TTRC("Use Scale Snap")), SNAP_USE_SCALE);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_relative", TTRC("Snap Relative")), SNAP_RELATIVE);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/use_pixel_snap", TTRC("Use Pixel Snap")), SNAP_USE_PIXEL);

	smartsnap_config_popup = memnew(PopupMenu);
	smartsnap_config_popup->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));
	smartsnap_config_popup->set_hide_on_checkable_item_selection(false);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_node_parent", TTRC("Snap to Parent")), SNAP_USE_NODE_PARENT);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_node_anchors", TTRC("Snap to Node Anchor")), SNAP_USE_NODE_ANCHORS);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_node_sides", TTRC("Snap to Node Sides")), SNAP_USE_NODE_SIDES);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_node_center", TTRC("Snap to Node Center")), SNAP_USE_NODE_CENTER);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_other_nodes", TTRC("Snap to Other Nodes")), SNAP_USE_OTHER_NODES);
	smartsnap_config_popup->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/snap_guides", TTRC("Snap to Guides")), SNAP_USE_GUIDES);
	p->add_submenu_node_item(TTRC("Smart Snapping"), smartsnap_config_popup);

	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/configure_snap", TTRC("Configure Snap...")), SNAP_CONFIGURE);

	main_flow->add_child(memnew(VSeparator));

	HBoxContainer *skeleton_menu_hbox = memnew(HBoxContainer);
	skeleton_menu_hbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_flow->add_child(skeleton_menu_hbox);

	skeleton_menu = memnew(MenuButton);
	skeleton_menu->set_flat(false);
	skeleton_menu->set_theme_type_variation("FlatMenuButton");
	skeleton_menu->set_shortcut_context(this);
	skeleton_menu_hbox->add_child(skeleton_menu);
	skeleton_menu->set_tooltip_text(TTRC("Skeleton Options"));
	skeleton_menu->set_switch_on_hover(true);

	p = skeleton_menu->get_popup();
	p->set_hide_on_checkable_item_selection(false);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/skeleton_show_bones", TTRC("Show Bones")), SKELETON_SHOW_BONES);
	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/skeleton_make_bones", TTRC("Make Bone2D Node(s) from Node(s)"), KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::B), SKELETON_MAKE_BONES);
	p->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));

	main_flow->add_child(memnew(VSeparator));

	HBoxContainer *view_menu_hbox = memnew(HBoxContainer);
	view_menu_hbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	main_flow->add_child(view_menu_hbox);

	view_menu = memnew(MenuButton);
	view_menu->set_flat(false);
	view_menu->set_theme_type_variation("FlatMenuButton");
	// TRANSLATORS: Noun, name of the 2D/3D View menus.
	view_menu->set_text(TTRC("View"));
	view_menu->set_switch_on_hover(true);
	view_menu->set_shortcut_context(this);
	view_menu_hbox->add_child(view_menu);

	p = view_menu->get_popup();
	p->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));
	p->connect("about_to_popup", callable_mp(this, &CanvasItemEditor::_prepare_view_menu));
	p->set_hide_on_checkable_item_selection(false);

	grid_menu = memnew(PopupMenu);
	grid_menu->connect("about_to_popup", callable_mp(this, &CanvasItemEditor::_prepare_grid_menu));
	grid_menu->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_on_grid_menu_id_pressed));
	grid_menu->add_radio_check_item(TTRC("Show"), GRID_VISIBILITY_SHOW);
	grid_menu->add_radio_check_item(TTRC("Show When Snapping"), GRID_VISIBILITY_SHOW_WHEN_SNAPPING);
	grid_menu->add_radio_check_item(TTRC("Hide"), GRID_VISIBILITY_HIDE);
	grid_menu->add_separator();
	grid_menu->add_shortcut(ED_SHORTCUT("canvas_item_editor/toggle_grid", TTRC("Toggle Grid"), KeyModifierMask::CMD_OR_CTRL | Key::APOSTROPHE));
	p->add_submenu_node_item(TTRC("Grid"), grid_menu);

	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_helpers", TTRC("Show Helpers"), KeyModifierMask::SHIFT | Key::H), SHOW_HELPERS);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_rulers", TTRC("Show Rulers")), SHOW_RULERS);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_guides", TTRC("Show Guides"), Key::Y), SHOW_GUIDES);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_origin", TTRC("Show Origin")), SHOW_ORIGIN);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_viewport", TTRC("Show Viewport")), SHOW_VIEWPORT);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_information", TTRC("Show Information")), SHOW_INFORMATION);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_frame_time", TTRC("Show Frame Time")), SHOW_FRAME_TIME);
	p->add_separator();

	gizmos_menu = memnew(PopupMenu);
	gizmos_menu->set_name("GizmosMenu");
	gizmos_menu->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));
	gizmos_menu->set_hide_on_checkable_item_selection(false);
	gizmos_menu->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_position_gizmos", TTRC("Position")), SHOW_POSITION_GIZMOS);
	gizmos_menu->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_lock_gizmos", TTRC("Lock")), SHOW_LOCK_GIZMOS);
	gizmos_menu->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_group_gizmos", TTRC("Group")), SHOW_GROUP_GIZMOS);
	gizmos_menu->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/show_transformation_gizmos", TTRC("Transformation")), SHOW_TRANSFORMATION_GIZMOS);
	p->add_child(gizmos_menu);
	p->add_submenu_item(TTRC("Gizmos"), "GizmosMenu");

	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/center_selection", TTRC("Center Selection"), Key::F), VIEW_CENTER_TO_SELECTION);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/frame_selection", TTRC("Frame Selection"), KeyModifierMask::SHIFT | Key::F), VIEW_FRAME_TO_SELECTION);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/clear_guides", TTRC("Clear Guides")), CLEAR_GUIDES);

	p->add_separator();
	auto_resampling_enabled = EditorSettings::get_singleton()->get_project_metadata("2d_editor", "auto_resampling_enabled", true);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/auto_resample_canvas_items", TTRC("Auto Resample CanvasItems")), AUTO_RESAMPLE_CANVAS_ITEMS);
	p->set_item_checked(p->get_item_index(AUTO_RESAMPLE_CANVAS_ITEMS), auto_resampling_enabled);
	p->add_check_shortcut(ED_SHORTCUT("canvas_item_editor/preview_canvas_scale", TTRC("Preview Canvas Scale")), PREVIEW_CANVAS_SCALE);

	theme_menu = memnew(PopupMenu);
	theme_menu->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_switch_theme_preview));
	theme_menu->add_radio_check_item(TTRC("Project theme"), THEME_PREVIEW_PROJECT);
	theme_menu->add_radio_check_item(TTRC("Editor theme"), THEME_PREVIEW_EDITOR);
	theme_menu->add_radio_check_item(TTRC("Default theme"), THEME_PREVIEW_DEFAULT);
	p->add_submenu_node_item(TTRC("Preview Theme"), theme_menu);

	theme_preview = (ThemePreviewMode)(int)EditorSettings::get_singleton()->get_project_metadata("2d_editor", "theme_preview", THEME_PREVIEW_PROJECT);
	for (int i = 0; i < THEME_PREVIEW_MAX; i++) {
		theme_menu->set_item_checked(i, i == theme_preview);
	}

	p->add_submenu_node_item(TTRC("Preview Translation"), memnew(EditorTranslationPreviewMenu));

	main_flow->add_child(memnew(VSeparator));

	// Contextual toolbars.
	context_toolbar_panel = memnew(PanelContainer);
	context_toolbar_hbox = memnew(HBoxContainer);
	context_toolbar_panel->add_child(context_toolbar_hbox);
	main_flow->add_child(context_toolbar_panel);

	// Animation controls.
	animation_hb = memnew(HBoxContainer);
	add_control_to_menu_panel(animation_hb);
	animation_hb->hide();

	key_loc_button = memnew(Button);
	key_loc_button->set_theme_type_variation(SceneStringName(FlatButton));
	key_loc_button->set_toggle_mode(true);
	key_loc_button->set_pressed(true);
	key_loc_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	key_loc_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(ANIM_INSERT_POS));
	key_loc_button->set_tooltip_text(TTRC("Translation mask for inserting keys."));
	animation_hb->add_child(key_loc_button);

	key_rot_button = memnew(Button);
	key_rot_button->set_theme_type_variation(SceneStringName(FlatButton));
	key_rot_button->set_toggle_mode(true);
	key_rot_button->set_pressed(true);
	key_rot_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	key_rot_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(ANIM_INSERT_ROT));
	key_rot_button->set_tooltip_text(TTRC("Rotation mask for inserting keys."));
	animation_hb->add_child(key_rot_button);

	key_scale_button = memnew(Button);
	key_scale_button->set_theme_type_variation(SceneStringName(FlatButton));
	key_scale_button->set_toggle_mode(true);
	key_scale_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	key_scale_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(ANIM_INSERT_SCALE));
	key_scale_button->set_tooltip_text(TTRC("Scale mask for inserting keys."));
	animation_hb->add_child(key_scale_button);

	key_insert_button = memnew(Button);
	key_insert_button->set_theme_type_variation(SceneStringName(FlatButton));
	key_insert_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	key_insert_button->connect(SceneStringName(pressed), callable_mp(this, &CanvasItemEditor::_popup_callback).bind(ANIM_INSERT_KEY));
	key_insert_button->set_tooltip_text(TTRC("Insert keys (based on mask)."));
	key_insert_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/anim_insert_key", TTRC("Insert Key"), Key::INSERT));
	key_insert_button->set_shortcut_context(this);
	animation_hb->add_child(key_insert_button);

	key_auto_insert_button = memnew(Button);
	key_auto_insert_button->set_theme_type_variation(SceneStringName(FlatButton));
	key_auto_insert_button->set_toggle_mode(true);
	key_auto_insert_button->set_focus_mode(FOCUS_ACCESSIBILITY);
	key_auto_insert_button->set_tooltip_text(TTRC("Auto insert keys when objects are translated, rotated or scaled (based on mask).\nKeys are only added to existing tracks, no new tracks will be created.\nKeys must be inserted manually for the first time."));
	key_auto_insert_button->set_shortcut(ED_SHORTCUT("canvas_item_editor/anim_auto_insert_key", TTRC("Auto Insert Key")));
	key_auto_insert_button->set_accessibility_name(TTRC("Auto Insert Key"));
	key_auto_insert_button->set_shortcut_context(this);
	animation_hb->add_child(key_auto_insert_button);

	animation_menu = memnew(MenuButton);
	animation_menu->set_flat(false);
	animation_menu->set_theme_type_variation("FlatMenuButton");
	animation_menu->set_shortcut_context(this);
	animation_menu->set_tooltip_text(TTRC("Animation Key and Pose Options"));
	animation_hb->add_child(animation_menu);
	animation_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_popup_callback));
	animation_menu->set_switch_on_hover(true);

	p = animation_menu->get_popup();

	p->add_shortcut(ED_GET_SHORTCUT("canvas_item_editor/anim_insert_key"), ANIM_INSERT_KEY);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/anim_insert_key_existing_tracks", TTRC("Insert Key (Existing Tracks)"), KeyModifierMask::CMD_OR_CTRL + Key::INSERT), ANIM_INSERT_KEY_EXISTING);
	p->add_separator();
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/anim_copy_pose", TTRC("Copy Pose")), ANIM_COPY_POSE);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/anim_paste_pose", TTRC("Paste Pose")), ANIM_PASTE_POSE);
	p->add_shortcut(ED_SHORTCUT("canvas_item_editor/anim_clear_pose", TTRC("Clear Pose"), KeyModifierMask::SHIFT | Key::K), ANIM_CLEAR_POSE);

	snap_dialog = memnew(SnapDialog);
	snap_dialog->connect(SceneStringName(confirmed), callable_mp(this, &CanvasItemEditor::_snap_changed));
	add_child(snap_dialog);

	select_sb.instantiate();

	selection_menu = memnew(PopupMenu);
	add_child(selection_menu);
	selection_menu->set_min_size(Vector2(100, 0));
	selection_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	selection_menu->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_selection_result_pressed));
	selection_menu->connect("popup_hide", callable_mp(this, &CanvasItemEditor::_selection_menu_hide), CONNECT_DEFERRED);

	add_node_menu = memnew(PopupMenu);
	add_child(add_node_menu);
	add_node_menu->connect(SceneStringName(id_pressed), callable_mp(this, &CanvasItemEditor::_add_node_pressed));

	add_node_dialog = memnew(CreateDialog);
	add_node_dialog->set_base_type("CanvasItem");
	add_child(add_node_dialog);
	add_node_dialog->connect("create", callable_mp(this, &CanvasItemEditor::_create_node));

	resample_timer = memnew(Timer);
	resample_timer->set_wait_time(resample_delay);
	resample_timer->set_one_shot(true);
	add_child(resample_timer);
	resample_timer->connect("timeout", callable_mp(this, &CanvasItemEditor::_update_oversampling));

	multiply_grid_step_shortcut = ED_SHORTCUT("canvas_item_editor/multiply_grid_step", TTRC("Multiply grid step by 2"), Key::KP_MULTIPLY);
	divide_grid_step_shortcut = ED_SHORTCUT("canvas_item_editor/divide_grid_step", TTRC("Divide grid step by 2"), Key::KP_DIVIDE);
	reset_transform_position_shortcut = ED_SHORTCUT("canvas_item_editor/reset_transform_position", TTRC("Reset Position"), KeyModifierMask::ALT + Key::W);
	reset_transform_rotation_shortcut = ED_SHORTCUT("canvas_item_editor/reset_transform_rotation", TTRC("Reset Rotation"), KeyModifierMask::ALT + Key::E);
	reset_transform_scale_shortcut = ED_SHORTCUT("canvas_item_editor/reset_transform_scale", TTRC("Reset Scale"), KeyModifierMask::ALT + Key::R);

	skeleton_menu->get_popup()->set_item_checked(skeleton_menu->get_popup()->get_item_index(SKELETON_SHOW_BONES), true);

	// Store the singleton instance.
	singleton = this;

	set_process_shortcut_input(true);
	clear(); // Make sure values are initialized.

	// Update the menus' checkboxes.
	callable_mp(this, &CanvasItemEditor::set_state).call_deferred(get_state());
}

CanvasItemEditor *CanvasItemEditor::singleton = nullptr;

void CanvasItemEditorPlugin::edit(Object *p_object) {
	canvas_item_editor->edit(Object::cast_to<CanvasItem>(p_object));
}

bool CanvasItemEditorPlugin::handles(Object *p_object) const {
	return Object::cast_to<CanvasItem>(p_object);
}

void CanvasItemEditorPlugin::make_visible(bool p_visible) {
	if (p_visible) {
		canvas_item_editor->make_visible();
	}
}

Dictionary CanvasItemEditorPlugin::get_state() const {
	return canvas_item_editor->get_state();
}

void CanvasItemEditorPlugin::set_state(const Dictionary &p_state) {
	canvas_item_editor->set_state(p_state);
}

void CanvasItemEditorPlugin::clear() {
	canvas_item_editor->clear();
}

void CanvasItemEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			connect("scene_changed", callable_mp((CanvasItem *)canvas_item_editor->get_viewport_control(), &CanvasItem::queue_redraw).unbind(1));
			connect("scene_closed", callable_mp((CanvasItem *)canvas_item_editor->get_viewport_control(), &CanvasItem::queue_redraw).unbind(1));
		} break;
	}
}

CanvasItemEditorPlugin::CanvasItemEditorPlugin() {
	canvas_item_editor = memnew(CanvasItemEditor);
	EditorDockManager::get_singleton()->add_dock(canvas_item_editor);
}

void CanvasItemEditorViewport::_on_mouse_exit() {
	if (!texture_node_type_selector->is_visible() && preview_node->get_parent()) {
		_remove_preview();
	}
}

void CanvasItemEditorViewport::_on_select_texture_node_type(Object *selected) {
	CheckBox *check = Object::cast_to<CheckBox>(selected);
	String type = check->get_text();
	texture_node_type_selector->set_title(vformat(TTR("Add %s"), type));
	tooltip_panel->set_text(vformat(TTR("Adding %s..."), type));
}

void CanvasItemEditorViewport::_on_change_type_confirmed() {
	if (!button_group->get_pressed_button()) {
		return;
	}

	CheckBox *check = Object::cast_to<CheckBox>(button_group->get_pressed_button());
	default_texture_node_type = check->get_text();
	_perform_drop_data();
	texture_node_type_selector->hide();
}

void CanvasItemEditorViewport::_create_preview(const Vector<String> &files) const {
	bool add_preview = false;

	for (int i = 0; i < files.size(); i++) {
		Ref<Resource> res = ResourceLoader::load(files[i]);
		ERR_CONTINUE(res.is_null());

		Ref<Texture2D> texture = res;
		if (texture.is_valid()) {
			Sprite2D *sprite = memnew(Sprite2D);
			sprite->set_texture(texture);
			sprite->set_modulate(Color(1, 1, 1, 0.7f));
			preview_node->add_child(sprite);
			add_preview = true;
		}

		Ref<PackedScene> scene = res;
		if (scene.is_valid()) {
			Node *instance = scene->instantiate();
			if (instance) {
				preview_node->add_child(instance);
			}
			add_preview = true;
		}

		Ref<Mesh> mesh = res;
		if (mesh.is_valid()) {
			MeshInstance2D *mesh_instance = memnew(MeshInstance2D);
			mesh_instance->set_mesh(mesh);
			preview_node->add_child(mesh_instance);
			add_preview = true;
		}

		Ref<AudioStream> audio = res;
		if (audio.is_valid()) {
			Sprite2D *sprite = memnew(Sprite2D);
			sprite->set_texture(get_editor_theme_icon(SNAME("AudioStreamPlayer2D")));
			sprite->set_modulate(Color(1, 1, 1, 0.7f));
			sprite->set_position(Vector2(0, -sprite->get_texture()->get_size().height) * EDSCALE);
			preview_node->add_child(sprite);
			add_preview = true;
		}

		Ref<Script> script = res;
		if (script.is_valid()) {
			String class_name = script->get_global_name();
			String base_type = script->get_instance_base_type();
			Sprite2D *sprite = memnew(Sprite2D);
			sprite->set_texture(EditorNode::get_singleton()->get_class_icon(
					class_name.is_empty() ? base_type : class_name));
			sprite->set_modulate(Color(1, 1, 1, 0.7f));
			sprite->set_position(Vector2(0, -sprite->get_texture()->get_size().height) * EDSCALE);
			preview_node->add_child(sprite);
			add_preview = true;
		}
	}

	if (add_preview) {
		EditorNode::get_singleton()->get_scene_root()->add_child(preview_node);
	}
}

void CanvasItemEditorViewport::_remove_preview() {
	if (!canvas_item_editor->message.is_empty()) {
		canvas_item_editor->message = "";
		canvas_item_editor->update_viewport();
	}
	tooltip_panel->hide();
	if (preview_node->get_parent()) {
		for (int i = preview_node->get_child_count() - 1; i >= 0; i--) {
			Node *node = preview_node->get_child(i);
			node->queue_free();
			preview_node->remove_child(node);
		}
		EditorNode::get_singleton()->get_scene_root()->remove_child(preview_node);
	}
}

void CanvasItemEditorViewport::_create_texture_node(Node *p_parent, Node *p_child, const String &p_path, const Point2 &p_point) {
	// Adjust casing according to project setting. The file name is expected to be in snake_case, but will work for others.
	const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
	if (!node_name.is_empty()) {
		p_child->set_name(node_name);
	}
	// Compute the global position.
	Transform2D xform = canvas_item_editor->get_canvas_transform();
	Point2 target_position = xform.affine_inverse().xform(p_point);

	// Adjust position for Control and TouchScreenButton.
	Ref<Texture2D> texture = ResourceCache::get_ref(p_path);
	if (Object::cast_to<Control>(p_child) || Object::cast_to<TouchScreenButton>(p_child) || Object::cast_to<Polygon2D>(p_child)) {
		target_position -= texture->get_size() / 2;
	}
	canvas_item_editor->add_node_to_scene(p_parent, p_child, target_position);

	// Use undo_redo for properties to support live editing.
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (Object::cast_to<TouchScreenButton>(p_child) || Object::cast_to<TextureButton>(p_child)) {
		undo_redo->add_do_property(p_child, "texture_normal", texture);
	} else {
		undo_redo->add_do_property(p_child, "texture", texture);
	}

	// Make visible for certain node type.
	if (Object::cast_to<Control>(p_child)) {
		Size2 texture_size = texture->get_size();
		undo_redo->add_do_property(p_child, "size", texture_size);
	} else if (Object::cast_to<Polygon2D>(p_child)) {
		Size2 texture_size = texture->get_size();
		Vector<Vector2> list = {
			Vector2(0, 0),
			Vector2(texture_size.width, 0),
			Vector2(texture_size.width, texture_size.height),
			Vector2(0, texture_size.height)
		};
		undo_redo->add_do_property(p_child, "polygon", list);
	}
}

void CanvasItemEditorViewport::_create_audio_node(Node *p_parent, const String &p_path, const Point2 &p_point) {
	AudioStreamPlayer2D *child = memnew(AudioStreamPlayer2D);

	// Adjust casing according to project setting. The file name is expected to be in snake_case, but will work for others.
	const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
	if (!node_name.is_empty()) {
		child->set_name(node_name);
	}

	// Compute the global position.
	Transform2D xform = canvas_item_editor->get_canvas_transform();
	Point2 target_position = xform.affine_inverse().xform(p_point);

	canvas_item_editor->add_node_to_scene(p_parent, child, target_position);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->add_do_property(child, "stream", ResourceCache::get_ref(p_path));
}

void CanvasItemEditorViewport::_create_mesh_node(Node *p_parent, const String &p_path, const Point2 &p_point) {
	MeshInstance2D *child = memnew(MeshInstance2D);

	// Adjust casing according to project setting. The file name is expected to be in snake_case, but will work for others.
	const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
	if (!node_name.is_empty()) {
		child->set_name(node_name);
	}

	// Compute the global position.
	Transform2D xform = canvas_item_editor->get_canvas_transform();
	Point2 target_position = xform.affine_inverse().xform(p_point);

	canvas_item_editor->add_node_to_scene(p_parent, child, target_position);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->add_do_property(child, "mesh", ResourceCache::get_ref(p_path));
}

bool CanvasItemEditorViewport::_create_instance(Node *p_parent, const String &p_path, const Point2 &p_point) {
	Ref<PackedScene> sdata = ResourceLoader::load(p_path);
	if (sdata.is_null()) { // invalid scene
		return false;
	}

	Node *instantiated_scene = sdata->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
	if (!instantiated_scene) { // Error on instantiation.
		return false;
	}

	Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();

	if (!edited_scene->get_scene_file_path().is_empty()) { // Cyclic instantiation.
		if (canvas_item_editor->cyclical_dependency_exists(edited_scene->get_scene_file_path(), instantiated_scene)) {
			memdelete(instantiated_scene);
			return false;
		}
	}

	instantiated_scene->set_scene_file_path(ProjectSettings::get_singleton()->localize_path(p_path));

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	EditorSelection *editor_selection = EditorNode::get_singleton()->get_editor_selection();
	undo_redo->add_do_method(p_parent, "add_child", instantiated_scene, true);
	undo_redo->add_do_method(instantiated_scene, "set_owner", edited_scene);
	undo_redo->add_do_reference(instantiated_scene);
	undo_redo->add_undo_method(p_parent, "remove_child", instantiated_scene);
	undo_redo->add_do_method(editor_selection, "add_node", instantiated_scene);

	String new_name = p_parent->validate_child_name(instantiated_scene);
	EditorDebuggerNode *ed = EditorDebuggerNode::get_singleton();
	undo_redo->add_do_method(ed, "live_debug_instantiate_node", edited_scene->get_path_to(p_parent), p_path, new_name);
	undo_redo->add_undo_method(ed, "live_debug_remove_node", NodePath(String(edited_scene->get_path_to(p_parent)) + "/" + new_name));

	CanvasItem *instance_ci = Object::cast_to<CanvasItem>(instantiated_scene);
	if (instance_ci) {
		Vector2 target_pos = canvas_item_editor->get_canvas_transform().affine_inverse().xform(p_point);
		target_pos = canvas_item_editor->get_canvas_item_manipulator()->snap_point(target_pos);
		target_pos = get_parent_screen_transform(p_parent).affine_inverse().xform(target_pos);
		// Preserve instance position of the original scene.
		target_pos += instance_ci->_edit_get_position();

		undo_redo->add_do_method(instantiated_scene, "set_position", target_pos);
	}

	return true;
}

void CanvasItemEditorViewport::_create_script_node(Node *p_parent, const String &p_path, const Point2 &p_point) {
	Ref<Script> script = ResourceLoader::load(p_path);
	if (script.is_null()) {
		return;
	}

	String class_name = script->get_global_name();
	String base_type = script->get_instance_base_type();
	Object *ob = ClassDB::instantiate(base_type);
	Node *instantiated_node = Object::cast_to<Node>(ob);
	if (!instantiated_node) { // Error on instantiation.
		return;
	}
	if (class_name.is_empty()) {
		const String &node_name = Node::adjust_name_casing(p_path.get_file().get_basename());
		if (!node_name.is_empty()) {
			instantiated_node->set_name(node_name);
		}
	} else {
		instantiated_node->set_name(class_name);
		PropertyUtils::assign_custom_type_script(ob, script);
	}
	instantiated_node->set_script(script);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();

	if (p_parent) {
		undo_redo->add_do_method(p_parent, "add_child", instantiated_node, true);
		undo_redo->add_do_method(instantiated_node, "set_owner", EditorNode::get_singleton()->get_edited_scene());
		undo_redo->add_do_reference(instantiated_node);
		undo_redo->add_undo_method(p_parent, "remove_child", instantiated_node);
	} else { // If no parent is selected, set as root node of the scene.
		undo_redo->add_do_method(EditorNode::get_singleton(), "set_edited_scene", instantiated_node);
		undo_redo->add_do_method(instantiated_node, "set_owner", EditorNode::get_singleton()->get_edited_scene());
		undo_redo->add_do_reference(instantiated_node);
		undo_redo->add_undo_method(EditorNode::get_singleton(), "set_edited_scene", (Object *)nullptr);
	}

	if (p_parent) {
		String new_name = p_parent->validate_child_name(instantiated_node);
		EditorDebuggerNode *ed = EditorDebuggerNode::get_singleton();
		undo_redo->add_do_method(ed, "live_debug_create_node", EditorNode::get_singleton()->get_edited_scene()->get_path_to(p_parent), instantiated_node->get_class(), new_name);
		undo_redo->add_undo_method(ed, "live_debug_remove_node", NodePath(String(EditorNode::get_singleton()->get_edited_scene()->get_path_to(p_parent)) + "/" + new_name));
	}

	// Compute the global position.
	Transform2D xform = canvas_item_editor->get_canvas_transform();
	Point2 target_position = xform.affine_inverse().xform(p_point);

	// There's nothing to be used as source position, so snapping will work as absolute if enabled.
	target_position = canvas_item_editor->get_canvas_item_manipulator()->snap_point(target_position);

	CanvasItem *parent_ci = Object::cast_to<CanvasItem>(p_parent);
	Point2 local_target_pos = parent_ci ? parent_ci->get_global_transform().affine_inverse().xform(target_position) : target_position;

	if (ClassDB::has_property(instantiated_node->get_class(), "position")) {
		undo_redo->add_do_method(instantiated_node, "set_position", local_target_pos);
	}

	EditorSelection *editor_selection = EditorNode::get_singleton()->get_editor_selection();
	undo_redo->add_do_method(editor_selection, "add_node", instantiated_node);
}

void CanvasItemEditorViewport::_perform_drop_data() {
	ERR_FAIL_COND(selected_files.is_empty());

	_remove_preview();

	if (!target_node) {
		// Should already be handled by `can_drop_data`.
		ERR_FAIL_COND_MSG(selected_files.size() > 1, "Can't instantiate multiple nodes without root.");

		const String &path = selected_files[0];
		Ref<Resource> res = ResourceLoader::load(path);
		if (res.is_null()) {
			return;
		}

		Ref<PackedScene> scene = res;
		if (scene.is_valid()) {
			// Without root node act the same as "Load Inherited Scene".
			Error err = EditorNode::get_singleton()->open_scene(path, false, true);
			if (err != OK) {
				accept->set_text(vformat(TTR("Error instantiating scene from %s."), path.get_file()));
				accept->popup_centered();
			}
			return;
		}
	}

	PackedStringArray error_files;

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action_for_history(TTR("Create Node"), EditorNode::get_editor_data().get_current_edited_scene_history_id());
	EditorSelection *editor_selection = EditorNode::get_singleton()->get_editor_selection();
	undo_redo->add_do_method(editor_selection, "clear");

	for (int i = 0; i < selected_files.size(); i++) {
		String path = selected_files[i];
		Ref<Resource> res = ResourceLoader::load(path);
		if (res.is_null()) {
			continue;
		}

		Ref<PackedScene> scene = res;
		if (scene.is_valid()) {
			bool success = _create_instance(target_node, path, drop_pos);
			if (!success) {
				error_files.push_back(path.get_file());
			}
			continue;
		}

		Ref<Texture2D> texture = res;
		if (texture.is_valid()) {
			Node *child = Object::cast_to<Node>(ClassDB::instantiate(default_texture_node_type));
			_create_texture_node(target_node, child, path, drop_pos);
		}

		Ref<AudioStream> audio = res;
		if (audio.is_valid()) {
			_create_audio_node(target_node, path, drop_pos);
		}

		Ref<Mesh> mesh = res;
		if (mesh.is_valid()) {
			_create_mesh_node(target_node, path, drop_pos);
		}

		Ref<Script> script = res;
		if (script.is_valid()) {
			_create_script_node(target_node, path, drop_pos);
		}
	}

	undo_redo->commit_action();

	if (error_files.size() > 0) {
		accept->set_text(vformat(TTR("Error instantiating scene from %s."), String(", ").join(error_files)));
		accept->popup_centered();
	}
}

void CanvasItemEditorViewport::set_hint_label(const String &p_title, const String &p_description) const {
	if (p_title.is_empty() && p_description.is_empty()) {
		tooltip_panel->hide();
		return;
	}

	tooltip_panel->set_text(
			vformat("[font_size=%s][b][color=%s]%s[/color][/b][/font_size]\n%s",
					get_theme_default_font_size() + 2,
					get_theme_color(SNAME("accent_color"), EditorStringName(Editor)).to_html(false),
					p_title, p_description));
	tooltip_panel->show();
}

bool CanvasItemEditorViewport::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (p_point == Vector2(Math::INF, Math::INF)) {
		tooltip_panel->hide();
		return false;
	}
	Dictionary d = p_data;
	if (!d.has("type") || (String(d["type"]) != "files")) {
		tooltip_panel->hide();
		return false;
	}

	Vector<String> files = d["files"];

	const Node *edited_scene = EditorNode::get_singleton()->get_edited_scene();
	if (!edited_scene && files.size() > 1) {
		canvas_item_editor->message = TTR("Can't instantiate multiple nodes without root.");
		canvas_item_editor->update_viewport();
		return false;
	}

	enum {
		SCENE = 1 << 0,
		TEXTURE = 1 << 1,
		AUDIO = 1 << 2,
		MESH = 1 << 3,
		SCRIPT = 1 << 4,
	};
	int instantiate_type = 0;

	String error_message;

	StringName script_node_type;

	for (const String &path : files) {
		const StringName res_type = ResourceLoader::get_resource_type(path);

		if (ClassDB::is_parent_class(res_type, SNAME("PackedScene"))) {
			Ref<PackedScene> scn = ResourceLoader::load(path);
			ERR_CONTINUE(scn.is_null());

			Node *instantiated_scene = scn->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
			if (!instantiated_scene) {
				continue;
			}
			if (edited_scene && !edited_scene->get_scene_file_path().is_empty() && canvas_item_editor->cyclical_dependency_exists(edited_scene->get_scene_file_path(), instantiated_scene)) {
				error_message = vformat(TTR("Circular dependency found at %s."), path.get_file());
				break;
			}
			memdelete(instantiated_scene);
			instantiate_type |= SCENE;
		} else if (ClassDB::is_parent_class(res_type, SNAME("Texture2D"))) {
			instantiate_type |= TEXTURE;
		} else if (ClassDB::is_parent_class(res_type, SNAME("AudioStream"))) {
			instantiate_type |= AUDIO;
		} else if (ClassDB::is_parent_class(res_type, "Mesh")) {
			instantiate_type |= MESH;
		} else if (ClassDB::is_parent_class(res_type, "Script")) {
			Ref<Script> script = ResourceLoader::load(path);
			ERR_CONTINUE(script.is_null());
			StringName base_type = script->get_instance_base_type();
			if (ClassDB::is_parent_class(base_type, "Node")) {
				StringName global_name = script->get_global_name();
				script_node_type = global_name.is_empty() ? base_type : global_name;
				instantiate_type |= SCRIPT;
			} else {
				error_message = TTR("This script is not a valid node.");
			}
		}
	}

	String title = TPL(files.size(), TTR("Can't drop the file..."), TTR("Can't drop the files..."));
	if (!error_message.is_empty()) {
		set_hint_label(title, error_message);
		canvas_item_editor->update_viewport();
		return false;
	}
	if (instantiate_type == 0) {
		set_hint_label(title, TTR("This file format is not supported."));
		return false;
	}

	if (!preview_node->get_parent()) { // create preview only once
		_create_preview(files);
	}
	ERR_FAIL_COND_V(preview_node->get_child_count() == 0, false);

	const Transform2D trans = canvas_item_editor->get_canvas_transform();
	preview_node->set_position((p_point - trans.get_origin()) / trans.get_scale().x);

	if (!edited_scene && instantiate_type & SCENE) {
		String scene_file_path = preview_node->get_child(0)->get_scene_file_path();
		// TRANSLATORS: The placeholder is the file path of the scene being instantiated.
		canvas_item_editor->message = vformat(TTR("Creating inherited scene from: %s"), scene_file_path);
	} else {
		double snap = EDITOR_GET("interface/inspector/default_float_step");
		int snap_step_decimals = Math::range_step_decimals(snap);
		const String &lang = _get_locale();
#define FORMAT(value) (TranslationServer::get_singleton()->format_number(String::num(value, snap_step_decimals), lang))
		Vector2 preview_node_pos = preview_node->get_global_position();
		canvas_item_editor->message = TTR("Instantiating: ") + "(" + FORMAT(preview_node_pos.x) + ", " + FORMAT(preview_node_pos.y) + ") px";
	}
	canvas_item_editor->update_viewport();

	String desc = "[ul]" +
			TPL(files.size(), TTR("[b]Default:[/b] Add as sibling of selected node (except when root is selected)."), TTR("[b]Default:[/b] Add as siblings of selected node (except when root is selected).")) + "\n" +
			TPL(files.size(), TTR("[b]Hold Shift:[/b] Add as child of selected node."), TTR("[b]Hold Shift:[/b] Add as children of selected node.")) + "\n" +
			vformat(TPL(files.size(), TTR("[b]Hold %s:[/b] Add as child of root node."), TTR("[b]Hold %s:[/b] Add as children of root node.")), keycode_get_string((Key)KeyModifierMask::ALT));

	if (files.size() > 1) {
		title = TTR("Dropping multiple files...");
	} else if (instantiate_type & SCENE) {
		title = TTR("Dropping a Scene file...");
	} else if (instantiate_type & AUDIO) {
		title = TTR("Dropping an Audio file...");
	} else if (instantiate_type & TEXTURE) {
		// TRANSLATORS: The placeholders are the types of nodes being instantiated.
		title = vformat(TTR("Dropping a Texture file as a %s node..."), default_texture_node_type);
	} else if (instantiate_type & MESH) {
		title = TTR("Dropping a Mesh file...");
	} else if (instantiate_type & SCRIPT) {
		title = vformat(TTR("Dropping a Script as a %s node..."), script_node_type);
	}
	if (instantiate_type & TEXTURE) {
		desc += "\n" + vformat(TTR("[b]Hold %s + Shift:[/b] Add Texture as a different node type."), keycode_get_string((Key)KeyModifierMask::ALT));
	}
	desc += "[/ul]";

	set_hint_label(title, desc);

	return true;
}

void CanvasItemEditorViewport::_show_texture_node_type_selector() {
	_remove_preview();
	List<BaseButton *> btn_list;
	button_group->get_buttons(&btn_list);

	for (BaseButton *btn : btn_list) {
		CheckBox *check = Object::cast_to<CheckBox>(btn);
		check->set_pressed(check->get_text() == default_texture_node_type);
	}
	texture_node_type_selector->set_title(vformat(TTR("Add %s"), default_texture_node_type));
	texture_node_type_selector->popup_centered();
}

bool CanvasItemEditorViewport::_is_any_texture_selected() const {
	for (int i = 0; i < selected_files.size(); ++i) {
		if (ClassDB::is_parent_class(ResourceLoader::get_resource_type(selected_files[i]), "Texture2D")) {
			return true;
		}
	}
	return false;
}

void CanvasItemEditorViewport::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (p_point == Vector2(Math::INF, Math::INF)) {
		return;
	}
	bool is_shift = Input::get_singleton()->is_key_pressed(Key::SHIFT);
	bool is_alt = Input::get_singleton()->is_key_pressed(Key::ALT);

	selected_files.clear();
	Dictionary d = p_data;
	if (d.has("type") && String(d["type"]) == "files") {
		selected_files = d["files"];
	}
	if (selected_files.is_empty()) {
		return;
	}

	const List<Node *> &selected_nodes = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	Node *root_node = EditorNode::get_singleton()->get_edited_scene();
	if (selected_nodes.size() > 0) {
		Node *selected_node = selected_nodes.front()->get();
		if (is_alt) {
			target_node = root_node;
		} else if (is_shift) {
			target_node = selected_node;
		} else { // Default behavior.
			target_node = (selected_node != root_node) ? selected_node->get_parent() : root_node;
		}
	} else {
		if (root_node) {
			target_node = root_node;
		} else {
			target_node = nullptr;
		}
	}

	drop_pos = p_point;

	if (is_alt && is_shift && _is_any_texture_selected()) {
		_show_texture_node_type_selector();
	} else {
		_perform_drop_data();
	}
}

void CanvasItemEditorViewport::set_show_information(bool p_enable) {
	show_information = p_enable;

	set_process(show_information || show_frame_time);
	info_panel->set_visible(show_information);
}

bool CanvasItemEditorViewport::is_showing_information() const {
	return show_information;
}

void CanvasItemEditorViewport::set_show_frame_time(bool p_enable) {
	show_frame_time = p_enable;

	set_process(show_information || show_frame_time);

	if (show_frame_time) {
		frame_time_panel->show();
		RS::get_singleton()->viewport_set_measure_render_time(EditorNode::get_singleton()->get_scene_root()->get_viewport_rid(), true);
		for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
			// Initialize to 120 FPS, so that the initial estimation until we get enough data is always reasonable.
			cpu_time_history[i] = 8.333333;
			gpu_time_history[i] = 8.333333;
		}
		cpu_time_history_index = 0;
		gpu_time_history_index = 0;
	} else {
		frame_time_panel->hide();
	}
}

bool CanvasItemEditorViewport::is_showing_frame_time() const {
	return show_frame_time;
}

static void override_label_colors(Control *p_control) {
	p_control->begin_bulk_theme_override();
	p_control->add_theme_color_override(SceneStringName(font_color), p_control->get_theme_color(SNAME("font_dark_background_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_hover_color", p_control->get_theme_color(SNAME("font_dark_background_hover_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_focus_color", p_control->get_theme_color(SNAME("font_dark_background_focus_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_pressed_color", p_control->get_theme_color(SNAME("font_dark_background_pressed_color"), EditorStringName(Editor)));
	p_control->add_theme_color_override("font_hover_pressed_color", p_control->get_theme_color(SNAME("font_dark_background_hover_pressed_color"), EditorStringName(Editor)));
	p_control->end_bulk_theme_override();
}

void CanvasItemEditorViewport::_update_theme() {
	List<BaseButton *> btn_list;
	button_group->get_buttons(&btn_list);

	for (BaseButton *btn : btn_list) {
		CheckBox *check = Object::cast_to<CheckBox>(btn);
		check->set_button_icon(get_editor_theme_icon(check->get_text()));
	}
	Control *gui_base = EditorNode::get_singleton()->get_gui_base();
	const Ref<StyleBox> &information_3d_stylebox = gui_base->get_theme_stylebox(SNAME("Information3dViewport"), EditorStringName(EditorStyles));
	tooltip_panel->add_theme_style_override(CoreStringName(normal), information_3d_stylebox);

	frame_time_gradient->set_color(0, get_theme_color(SNAME("success_color_dark_background"), EditorStringName(Editor)));
	frame_time_gradient->set_color(1, get_theme_color(SNAME("warning_color_dark_background"), EditorStringName(Editor)));
	frame_time_gradient->set_color(2, get_theme_color(SNAME("error_color_dark_background"), EditorStringName(Editor)));

	info_panel->add_theme_style_override(SceneStringName(panel), information_3d_stylebox);
	override_label_colors(info_label);
	tooltip_panel->add_theme_style_override(CoreStringName(normal), information_3d_stylebox);

	frame_time_panel->add_theme_style_override(SceneStringName(panel), information_3d_stylebox);
	// Set a minimum width to prevent the width from changing all the time
	// when numbers vary rapidly. This minimum width is set based on a
	// GPU time of 999.99 ms in the current editor language.
	const float min_width = get_theme_font(SNAME("main"), EditorStringName(EditorFonts))->get_string_size(vformat(TTR("GPU Time: %s ms"), 999.99)).x;
	frame_time_panel->set_custom_minimum_size(Size2(min_width, 0) * EDSCALE);
	frame_time_vbox->add_theme_constant_override("separation", Math::round(-1 * EDSCALE));
}

void CanvasItemEditorViewport::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_theme();
		} break;

		case NOTIFICATION_ENTER_TREE: {
			_update_theme();
			connect(SceneStringName(mouse_exited), callable_mp(this, &CanvasItemEditorViewport::_on_mouse_exit));
		} break;

		case NOTIFICATION_EXIT_TREE: {
			disconnect(SceneStringName(mouse_exited), callable_mp(this, &CanvasItemEditorViewport::_on_mouse_exit));
		} break;

		case NOTIFICATION_DRAG_END: {
			_remove_preview();
		} break;

		case NOTIFICATION_PROCESS: {
			SubViewport *edited_scene_viewport = EditorNode::get_singleton()->get_scene_root();
			const RID edited_scene_viewport_rid = edited_scene_viewport->get_viewport_rid();

			if (show_information) {
				const String viewport_size = vformat(U"%d × %d", get_size().x, get_size().y);
				String text;
				text += vformat(
						TTR("Size: %s (%.1fMP)") + "\n",
						viewport_size,
						get_size().x * get_size().y * 0.000001);

				text += "\n";
				text += vformat(TTR("Objects: %d"), edited_scene_viewport->get_render_info(Viewport::RENDER_INFO_TYPE_CANVAS, Viewport::RENDER_INFO_OBJECTS_IN_FRAME)) + "\n";
				text += vformat(TTR("Primitives: %d"), edited_scene_viewport->get_render_info(Viewport::RENDER_INFO_TYPE_CANVAS, Viewport::RENDER_INFO_PRIMITIVES_IN_FRAME)) + "\n";
				text += vformat(TTR("Draw Calls: %d"), edited_scene_viewport->get_render_info(Viewport::RENDER_INFO_TYPE_CANVAS, Viewport::RENDER_INFO_DRAW_CALLS_IN_FRAME));

				info_label->set_text(text);
			}

			// FPS Counter.
			if (show_frame_time) {
				cpu_time_history[cpu_time_history_index] = RS::get_singleton()->viewport_get_measured_render_time_cpu(edited_scene_viewport_rid);
				cpu_time_history_index = (cpu_time_history_index + 1) % FRAME_TIME_HISTORY;
				double cpu_time = 0.0;
				for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
					cpu_time += cpu_time_history[i];
				}
				cpu_time /= FRAME_TIME_HISTORY;
				// Prevent unrealistically low values.
				cpu_time = MAX(0.01, cpu_time);

				gpu_time_history[gpu_time_history_index] = RS::get_singleton()->viewport_get_measured_render_time_gpu(edited_scene_viewport_rid);
				gpu_time_history_index = (gpu_time_history_index + 1) % FRAME_TIME_HISTORY;
				double gpu_time = 0.0;
				for (int i = 0; i < FRAME_TIME_HISTORY; i++) {
					gpu_time += gpu_time_history[i];
				}
				gpu_time /= FRAME_TIME_HISTORY;
				// Prevent division by zero for the FPS counter (and unrealistically low values).
				// This limits the reported FPS to 100000.
				gpu_time = MAX(0.01, gpu_time);

				// Color labels depending on performance level ("good" = green, "OK" = yellow, "bad" = red).
				// Middle point is at 15 ms.
				cpu_time_label->set_text(vformat(TTR("CPU Time: %s ms"), rtos(cpu_time).pad_decimals(2)));
				cpu_time_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(cpu_time, 0, 30, 0, 1)));

				gpu_time_label->set_text(vformat(TTR("GPU Time: %s ms"), rtos(gpu_time).pad_decimals(2)));
				// Middle point is at 15 ms.
				gpu_time_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(gpu_time, 0, 30, 0, 1)));

				const double fps = 1000.0 / gpu_time;
				fps_label->set_text(vformat(TTR("FPS: %d"), fps));
				// Middle point is at 60 FPS.
				fps_label->add_theme_color_override(
						SceneStringName(font_color),
						frame_time_gradient->get_color_at_offset(
								Math::remap(fps, 110, 10, 0, 1)));
			}
		} break;
	}
}

CanvasItemEditorViewport::CanvasItemEditorViewport(CanvasItemEditor *p_canvas_item_editor) {
	default_texture_node_type = "Sprite2D";
	// Node2D
	texture_node_types.push_back("Sprite2D");
	texture_node_types.push_back("PointLight2D");
	texture_node_types.push_back("CPUParticles2D");
	texture_node_types.push_back("GPUParticles2D");
	texture_node_types.push_back("Polygon2D");
	texture_node_types.push_back("TouchScreenButton");
	// Control
	texture_node_types.push_back("TextureRect");
	texture_node_types.push_back("TextureButton");
	texture_node_types.push_back("NinePatchRect");

	target_node = nullptr;
	canvas_item_editor = p_canvas_item_editor;
	preview_node = memnew(Control);

	accept = memnew(AcceptDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(accept);

	texture_node_type_selector = memnew(AcceptDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(texture_node_type_selector);
	texture_node_type_selector->connect(SceneStringName(confirmed), callable_mp(this, &CanvasItemEditorViewport::_on_change_type_confirmed));
	texture_node_type_selector->connect("canceled", callable_mp(this, &CanvasItemEditorViewport::_remove_preview));

	VBoxContainer *vbc = memnew(VBoxContainer);
	texture_node_type_selector->add_child(vbc);
	vbc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vbc->set_custom_minimum_size(Size2(240, 260) * EDSCALE);

	VBoxContainer *btn_group = memnew(VBoxContainer);
	vbc->add_child(btn_group);
	btn_group->set_h_size_flags(SIZE_EXPAND_FILL);

	button_group.instantiate();
	for (int i = 0; i < texture_node_types.size(); i++) {
		CheckBox *check = memnew(CheckBox);
		check->set_text(texture_node_types[i]);
		check->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		check->set_button_group(button_group);
		btn_group->add_child(check);
		check->connect("button_down", callable_mp(this, &CanvasItemEditorViewport::_on_select_texture_node_type).bind(check));
	}

	tooltip_panel = memnew(RichTextLabel);
	canvas_item_editor->get_controls_container()->add_child(tooltip_panel);
	tooltip_panel->hide();
	tooltip_panel->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	tooltip_panel->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	tooltip_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	tooltip_panel->set_focus_mode(FOCUS_ACCESSIBILITY);
	tooltip_panel->set_use_bbcode(true);
	tooltip_panel->set_fit_content(true);
	tooltip_panel->set_scroll_active(false);
	tooltip_panel->set_tab_size(0);
	tooltip_panel->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	tooltip_panel->set_anchors_and_offsets_preset(LayoutPreset::PRESET_TOP_LEFT);
	tooltip_panel->add_theme_color_override(SceneStringName(font_color), Color(0.8f, 0.8f, 0.8f, 1));
	tooltip_panel->add_theme_constant_override("paragraph_separation", 5);

	info_panel = memnew(PanelContainer);
	info_panel->set_anchor_and_offset(SIDE_LEFT, ANCHOR_END, -90 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_TOP, ANCHOR_END, -90 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_RIGHT, ANCHOR_END, -10 * EDSCALE);
	info_panel->set_anchor_and_offset(SIDE_BOTTOM, ANCHOR_END, -10 * EDSCALE);
	info_panel->set_h_grow_direction(GROW_DIRECTION_BEGIN);
	info_panel->set_v_grow_direction(GROW_DIRECTION_BEGIN);
	info_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	canvas_item_editor->get_info_panel_container()->add_child(info_panel);
	info_panel->hide();

	info_label = memnew(Label);
	info_label->set_focus_mode(FOCUS_ACCESSIBILITY);
	info_panel->add_child(info_label);

	frame_time_gradient = memnew(Gradient);
	// The color is set when the theme changes.
	frame_time_gradient->add_point(0.5, Color());
	frame_time_panel = memnew(PanelContainer);
	frame_time_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	canvas_item_editor->get_frame_time_panel_container()->add_child(frame_time_panel);
	frame_time_panel->hide();

	frame_time_vbox = memnew(VBoxContainer);
	frame_time_panel->add_child(frame_time_vbox);

	// Individual Labels are used to allow coloring each label with its own color.
	cpu_time_label = memnew(Label);
	frame_time_vbox->add_child(cpu_time_label);

	gpu_time_label = memnew(Label);
	frame_time_vbox->add_child(gpu_time_label);

	fps_label = memnew(Label);
	frame_time_vbox->add_child(fps_label);

	RS::get_singleton()->canvas_set_disable_scale(true);
}

CanvasItemEditorViewport::~CanvasItemEditorViewport() {
	memdelete(preview_node);
}
