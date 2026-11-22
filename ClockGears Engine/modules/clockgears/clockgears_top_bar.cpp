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
#include "editor/clockgears_theme.h"
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
	ClassDB::bind_method(D_METHOD("select_layer_button", "layer"), &ClockGearsTopBar::select_layer_button);
	ClassDB::bind_method(D_METHOD("get_layer_button", "idx"), &ClockGearsTopBar::get_layer_button);
	ClassDB::bind_method(D_METHOD("get_tool_button", "idx"), &ClockGearsTopBar::get_tool_button);
	ClassDB::bind_method(D_METHOD("get_playtest_button"), &ClockGearsTopBar::get_playtest_button);
	ClassDB::bind_method(D_METHOD("get_overflow_button"), &ClockGearsTopBar::get_overflow_button);
	ClassDB::bind_method(D_METHOD("get_visible_group_count"), &ClockGearsTopBar::get_visible_group_count);
	ClassDB::bind_method(D_METHOD("get_total_group_count"), &ClockGearsTopBar::get_total_group_count);
	ClassDB::bind_method(D_METHOD("get_overflow_group_count"), &ClockGearsTopBar::get_overflow_group_count);
	ClassDB::bind_method(D_METHOD("is_single_row"), &ClockGearsTopBar::is_single_row);
	ClassDB::bind_method(D_METHOD("get_toolbar_icons_audit"), &ClockGearsTopBar::get_toolbar_icons_audit);
}

Ref<Texture2D> ClockGearsTopBar::_make_svg_icon(const String &p_svg, int p_int_scale) const {
	Ref<Image> image;
	image.instantiate();
	if (image->load_svg_from_buffer(p_svg.to_utf8_buffer(), float(MAX(1, p_int_scale))) != OK || image->is_empty()) {
		return Ref<Texture2D>();
	}
	return ImageTexture::create_from_image(image);
}

Ref<Texture2D> ClockGearsTopBar::_get_png_icon(const String &p_name, int p_int_scale, const String &p_fallback) const {
	if (ClockGearsIcons::get_singleton()) {
		if (!p_fallback.is_empty()) {
			return ClockGearsIcons::get_singleton()->get_icon_with_fallback(p_name, p_fallback, float(MAX(1, p_int_scale)));
		}
		return ClockGearsIcons::get_singleton()->get_icon(p_name, float(MAX(1, p_int_scale)));
	}
	return Ref<Texture2D>();
}

Array ClockGearsTopBar::get_toolbar_icons_audit() const {
	Array out;
	for (int i = 0; i < icon_entries.size(); i++) {
		const ToolbarIconEntry &e = icon_entries[i];
		if (!e.button) {
			continue;
		}
		Dictionary d;
		d["tooltip"] = e.button->get_tooltip_text();
		d["png_name"] = e.png_name;
		d["has_icon"] = e.button->get_button_icon().is_valid();
		if (e.button->get_button_icon().is_valid()) {
			d["icon_size"] = e.button->get_button_icon()->get_size();
		}
		d["filter"] = int(e.button->get_texture_filter());
		out.push_back(d);
	}
	return out;
}

void ClockGearsTopBar::_apply_toolbar_button_style(Button *p_button, const Ref<Theme> &p_editor_theme, int p_int_scale) const {
	const int ui_scale = _get_ui_scale_int();
	const int radius = 6 * ui_scale;
	const int margin_h = 4 * ui_scale;
	const int margin_v = 4 * ui_scale;
	const int btn_side = 32 * p_int_scale + margin_h * 2;
	const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
	const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
	const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, float(ui_scale));
	const bool is_playtest = (p_button == playtest_btn);
	const bool is_overflow = (p_button == overflow_btn);
	const bool playtest_has_txt = is_playtest && !p_button->get_text().is_empty();

	Ref<StyleBoxFlat> normal;
	normal.instantiate();
	normal->set_anti_aliased(true);
	if (is_playtest) {
		const Color gcol = ClockGearsTheme::color_success();
		normal->set_bg_color(Color(gcol.r, gcol.g, gcol.b, 0.14f));
		normal->set_border_width_all(1);
		normal->set_border_color(Color(gcol.r, gcol.g, gcol.b, 0.72f));
	} else {
		// En reposo: SIN borde ni caja (fondo transparente); el borde cobrizo y el tinte solo salen en hover/activo.
		normal->set_draw_center(false);
		normal->set_bg_color(Color(0, 0, 0, 0));
		normal->set_border_width_all(0);
		normal->set_border_color(Color(0, 0, 0, 0));
	}
	normal->set_corner_radius_all(radius);
	normal->set_content_margin_individual(playtest_has_txt ? (margin_h + 4 * ui_scale) : margin_h, margin_v, playtest_has_txt ? (margin_h + 6 * ui_scale) : margin_h, margin_v);

	Ref<StyleBoxFlat> hover = normal->duplicate();
	hover->set_draw_center(true);
	hover->set_border_width_all(1);
	if (is_playtest) {
		const Color gcol = ClockGearsTheme::color_success().lightened(0.14f);
		hover->set_bg_color(Color(gcol.r, gcol.g, gcol.b, 0.24f));
		hover->set_border_color(gcol);
		hover->set_shadow_color(Color(gcol.r, gcol.g, gcol.b, tk.halo_color.a));
		hover->set_shadow_size(tk.halo_radius_px);
		hover->set_shadow_offset(Vector2(0, 0));
	} else {
		hover->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, 0.14f));
		hover->set_border_color(tk.border_copper_bright);
		hover->set_shadow_color(tk.halo_color);
		hover->set_shadow_size(tk.halo_radius_px);
		hover->set_shadow_offset(Vector2(0, 0));
	}

	// Herramienta o capa activa: borde neón (2 px), halo y tinte ámbar
	Ref<StyleBoxFlat> pressed = normal->duplicate();
	pressed->set_draw_center(true);
	pressed->set_border_width_all(2);
	if (is_playtest) {
		const Color gcol = ClockGearsTheme::color_success();
		pressed->set_bg_color(Color(gcol.r, gcol.g, gcol.b, 0.34f));
		pressed->set_border_color(gcol.lightened(0.18f));
		pressed->set_shadow_color(Color(gcol.r, gcol.g, gcol.b, tk.halo_color.a));
		pressed->set_shadow_size(tk.halo_radius_px);
		pressed->set_shadow_offset(Vector2(0, 0));
	} else {
		pressed->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, 0.24f));
		pressed->set_border_color(tk.accent_neon);
		pressed->set_shadow_color(tk.halo_color);
		pressed->set_shadow_size(tk.halo_radius_px);
		pressed->set_shadow_offset(Vector2(0, 0));
	}

	Ref<StyleBoxFlat> hover_pressed = pressed->duplicate();
	if (!is_playtest) {
		hover_pressed->set_bg_color(tk.surface_panel.lerp(tk.accent_neon, 0.32f));
	}

	Ref<StyleBoxFlat> disabled = normal->duplicate();
	disabled->set_draw_center(false);
	disabled->set_bg_color(Color(0, 0, 0, 0));
	disabled->set_border_width_all(0);
	disabled->set_border_color(Color(0, 0, 0, 0));
	disabled->set_shadow_size(0);

	Ref<StyleBoxFlat> focus_ring = normal->duplicate();
	focus_ring->set_draw_center(false);
	focus_ring->set_border_width_all(1);
	focus_ring->set_border_color(tk.accent_neon);
	focus_ring->set_shadow_color(tk.halo_color);
	focus_ring->set_shadow_size(tk.halo_radius_px);
	focus_ring->set_shadow_offset(Vector2(0, 0));

	p_button->set_meta("_cg_theme_managed_container", true);
	p_button->add_theme_style_override("normal", normal);
	p_button->add_theme_style_override("hover", hover);
	p_button->add_theme_style_override("pressed", pressed);
	p_button->add_theme_style_override("hover_pressed", hover_pressed);
	p_button->add_theme_style_override("disabled", disabled);
	p_button->add_theme_style_override("focus", focus_ring);

	// NO teñir los iconos: conservan su color propio; al deshabilitarse van al 40%.
	p_button->add_theme_color_override("icon_normal_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_hover_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_hover_pressed_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_focus_color", Color(1, 1, 1, 1));
	p_button->add_theme_color_override("icon_disabled_color", Color(1, 1, 1, 0.40f));

	if (is_overflow) {
		Ref<Theme> ath = ClockGearsTheme::get_singleton() ? ClockGearsTheme::get_singleton()->get_active_theme() : Ref<Theme>();
		Color ov_fg = (ath.is_valid() && ath->has_color("font_color", "Button")) ? ath->get_color("font_color", "Button") : ClockGearsTheme::color_text_primary();
		p_button->add_theme_color_override("font_color", ov_fg);
		p_button->add_theme_color_override("font_hover_color", tk.accent_neon);
		p_button->add_theme_color_override("font_pressed_color", tk.accent_neon);
		p_button->set_custom_minimum_size(Size2(30 * ui_scale, btn_side));
		return;
	}

	// 32 px (o multiplo entero exacto) + filtro nearest sin suavizado.
	p_button->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	p_button->add_theme_constant_override("icon_max_width", 32 * p_int_scale);
	p_button->add_theme_constant_override("h_separation", playtest_has_txt ? (6 * ui_scale) : 0);
	p_button->set_custom_minimum_size(Size2(btn_side, btn_side));
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
	const int ui_scale = _get_ui_scale_int();
	MarginContainer *sep_wrap = memnew(MarginContainer);
	sep_wrap->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	sep_wrap->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	// 8 px de margen exacto a cada lado de los separadores
	sep_wrap->add_theme_constant_override("margin_left", 8 * ui_scale);
	sep_wrap->add_theme_constant_override("margin_right", 8 * ui_scale);
	sep_wrap->add_theme_constant_override("margin_top", 6 * p_int_scale);
	sep_wrap->add_theme_constant_override("margin_bottom", 6 * p_int_scale);

	const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
	const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
	const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, 1.0f);

	ColorRect *line = memnew(ColorRect);
	line->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	line->set_color(tk.border_subtle);
	line->set_custom_minimum_size(Size2(MAX(1, ui_scale), 0));
	line->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	sep_wrap->add_child(line);
	return sep_wrap;
}

void ClockGearsTopBar::_relayout_rows() {
	if (!row1_box || all_groups.is_empty()) {
		return;
	}

	const int ui_scale = _get_ui_scale_int();
	const int int_scale = ClockGearsIcons::get_editor_icon_scale(this);
	const float avail_width = get_size().x - bar_h_padding;
	Ref<Theme> editor_theme = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_theme() : Ref<Theme>();

	auto sum_prefix_width = [&](int p_group_count) -> float {
		float w = 0.0f;
		for (int i = 0; i < p_group_count; i++) {
			if (i > 0 && i - 1 < all_separators.size() && all_separators[i - 1]) {
				w += all_separators[i - 1]->get_combined_minimum_size().x;
			}
			if (all_groups[i]) {
				w += all_groups[i]->get_combined_minimum_size().x;
			}
		}
		return w;
	};

	auto set_playtest_label = [&](bool p_show_text) {
		if (!playtest_btn) {
			return;
		}
		const String want_txt = p_show_text ? CG_TR("Probar (F5)") : String();
		if (playtest_btn->get_text() != want_txt) {
			playtest_btn->set_text(want_txt);
			playtest_btn->set_icon_alignment(p_show_text ? HORIZONTAL_ALIGNMENT_LEFT : HORIZONTAL_ALIGNMENT_CENTER);
			_apply_toolbar_button_style(playtest_btn, editor_theme, int_scale);
		}
	};

	// 1. Si no tiene tamaño aún o cabe con "Probar (F5)" en texto completo, mostrar todos en 1 fila.
	bool show_pt_text = false;
	int first_overflow = int(all_groups.size());
	if (avail_width <= 0.0f) {
		show_pt_text = true;
		set_playtest_label(true);
		first_overflow = int(all_groups.size());
	} else {
		set_playtest_label(true);
		const float w_with_txt = sum_prefix_width(int(all_groups.size()));
		if (w_with_txt <= avail_width + 0.5f) {
			show_pt_text = true;
			first_overflow = int(all_groups.size());
		} else {
			set_playtest_label(false);
			const float w_icons_only = sum_prefix_width(int(all_groups.size()));
			if (w_icons_only <= avail_width + 0.5f) {
				show_pt_text = false;
				first_overflow = int(all_groups.size());
			} else {
				// Pantalla estrecha (p. ej. 960x540): NUNCA envolver en 2 filas; mandar últimos grupos al menú "»".
				show_pt_text = false;
				const float ov_reserve = (overflow_btn ? overflow_btn->get_combined_minimum_size().x : float(30 * ui_scale)) + float(8 * ui_scale);
				float used = 0.0f;
				int fitted = 0;
				for (int i = 0; i < all_groups.size(); i++) {
					const float sep_w = (i > 0 && i - 1 < all_separators.size() && all_separators[i - 1]) ? all_separators[i - 1]->get_combined_minimum_size().x : 0.0f;
					const float grp_w = all_groups[i] ? all_groups[i]->get_combined_minimum_size().x : 0.0f;
					if (i == 0 || (used + sep_w + grp_w + ov_reserve) <= avail_width + 0.5f) {
						used += sep_w + grp_w;
						fitted = i + 1;
					} else {
						break;
					}
				}
				first_overflow = MAX(1, fitted);
			}
		}
	}

	const int pt_state = show_pt_text ? 1 : 0;
	if (first_overflow == last_overflow_start && pt_state == last_playtest_txt_state) {
		return;
	}
	last_overflow_start = first_overflow;
	last_playtest_txt_state = pt_state;
	visible_group_count = first_overflow;

	for (int i = 0; i < all_groups.size(); i++) {
		const bool vis = (i < first_overflow);
		if (all_groups[i]) {
			all_groups[i]->set_visible(vis);
		}
		if (i > 0 && i - 1 < all_separators.size() && all_separators[i - 1]) {
			all_separators[i - 1]->set_visible(vis);
		}
	}

	const bool has_overflow = (first_overflow < int(all_groups.size()));
	if (overflow_spacer) {
		overflow_spacer->set_visible(has_overflow);
	}
	if (overflow_btn) {
		overflow_btn->set_visible(has_overflow);
	}
	if (overflow_popup) {
		overflow_popup->clear();
		overflow_item_buttons.clear();
		if (has_overflow) {
			overflow_popup->add_theme_constant_override("icon_max_width", 20 * ui_scale);
			for (int gi = first_overflow; gi < all_groups.size(); gi++) {
				HBoxContainer *g = all_groups[gi];
				if (!g) {
					continue;
				}
				if (gi > first_overflow) {
					overflow_popup->add_separator((gi < group_names.size()) ? group_names[gi] : String());
				} else if (gi < group_names.size() && !group_names[gi].is_empty()) {
					overflow_popup->add_separator(group_names[gi]);
				}
				for (int bi = 0; bi < g->get_child_count(); bi++) {
					Button *b = Object::cast_to<Button>(g->get_child(bi));
					if (!b) {
						continue;
					}
					const int item_id = overflow_item_buttons.size();
					String label = b->get_text().strip_edges();
					if (label.is_empty()) {
						label = b->get_tooltip_text().strip_edges();
					}
					if (label.is_empty()) {
						label = CG_TR("Herramienta");
					}
					if (b->is_toggle_mode()) {
						overflow_popup->add_check_item(label, item_id);
						overflow_popup->set_item_checked(overflow_popup->get_item_index(item_id), b->is_pressed());
					} else {
						overflow_popup->add_item(label, item_id);
					}
					const int idx = overflow_popup->get_item_index(item_id);
					if (idx >= 0) {
						if (b->get_button_icon().is_valid()) {
							overflow_popup->set_item_icon(idx, b->get_button_icon());
						}
						overflow_popup->set_item_disabled(idx, b->is_disabled());
					}
					overflow_item_buttons.push_back(b);
				}
			}
		}
	}
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
		Ref<Texture2D> resolved_icon;
		if (!entry.png_name.is_empty()) {
			resolved_icon = _get_png_icon(entry.png_name, icon_scale, entry.cg_fallback_icon);
			if (resolved_icon.is_null() && !entry.svg_src.is_empty()) {
				resolved_icon = _make_svg_icon(entry.svg_src, icon_scale);
			}
		} else if (!entry.svg_src.is_empty()) {
			resolved_icon = _make_svg_icon(entry.svg_src, icon_scale);
		} else if (entry.editor_icon_name != StringName()) {
			if (is_inside_tree() && entry.button->has_theme_icon(entry.editor_icon_name, SNAME("EditorIcons"))) {
				resolved_icon = entry.button->get_theme_icon(entry.editor_icon_name, SNAME("EditorIcons"));
			} else if (editor_theme.is_valid() && editor_theme->has_icon(entry.editor_icon_name, SNAME("EditorIcons"))) {
				resolved_icon = editor_theme->get_icon(entry.editor_icon_name, SNAME("EditorIcons"));
			}
			if (resolved_icon.is_null() && !entry.cg_fallback_icon.is_empty() && ClockGearsIcons::get_singleton()) {
				resolved_icon = ClockGearsIcons::get_singleton()->get_icon(entry.cg_fallback_icon, float(icon_scale));
			}
		}
		entry.button->set_button_icon(resolved_icon);
		if (entry.button == playtest_btn) {
			// El texto "Probar (F5)" se decide en _relayout_rows() según el ancho disponible en 1 fila.
			entry.button->set_icon_alignment(entry.button->get_text().is_empty() ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
		} else if (resolved_icon.is_null() && !entry.fallback_text.is_empty()) {
			entry.button->set_text(entry.fallback_text);
		} else {
			entry.button->set_text(String());
		}
		_apply_toolbar_button_style(entry.button, editor_theme, icon_scale);
	}
	if (overflow_btn) {
		_apply_toolbar_button_style(overflow_btn, editor_theme, icon_scale);
	}
	const bool is_brass = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "brass");
	const bool is_light = ClockGearsTheme::get_singleton() && (ClockGearsTheme::get_singleton()->get_active_theme_id() == "light");
	const ClockGearsTheme::NeonCopperTokens tk = ClockGearsTheme::get_neon_copper_tokens(is_brass, is_light, 1.0f);
	for (int si = 0; si < all_separators.size(); si++) {
		if (all_separators[si] && all_separators[si]->get_child_count() > 0) {
			if (ColorRect *cr = Object::cast_to<ColorRect>(all_separators[si]->get_child(0))) {
				cr->set_color(tk.border_subtle);
			}
		}
	}
	last_overflow_start = -1;
	last_playtest_txt_state = -1;
	last_layout_width = -1.0f;
	_relayout_rows();
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
		_apply_bar_container_style();
		_refresh_icon_scale();
	} else if (p_what == NOTIFICATION_THEME_CHANGED) {
		_apply_bar_container_style();
		_refresh_icon_scale();
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

void ClockGearsTopBar::select_layer_button(int p_layer) {
	_on_layer_pressed(CLAMP(p_layer, 1, 6));
}

void ClockGearsTopBar::_on_dm_data_changed_topbar(const String &p_category, int p_id) {
	if (p_category == "custom_layers" || p_category == "system" || p_category.is_empty()) {
		sync_active_layer_display();
	}
}

void ClockGearsTopBar::sync_active_layer_display() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
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
	EditorNode::sync_clockgears_active_layer_indicator_all();
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

void ClockGearsTopBar::_on_overflow_about_to_popup() {
	if (!overflow_popup) {
		return;
	}
	for (int id = 0; id < overflow_item_buttons.size(); id++) {
		Button *b = overflow_item_buttons[id];
		if (!b) {
			continue;
		}
		const int idx = overflow_popup->get_item_index(id);
		if (idx < 0) {
			continue;
		}
		if (b->is_toggle_mode()) {
			overflow_popup->set_item_checked(idx, b->is_pressed());
		}
		overflow_popup->set_item_disabled(idx, b->is_disabled());
	}
}

void ClockGearsTopBar::_on_overflow_id_pressed(int p_id) {
	if (p_id < 0 || p_id >= overflow_item_buttons.size()) {
		return;
	}
	Button *b = overflow_item_buttons[p_id];
	if (!b || b->is_disabled()) {
		return;
	}
	if (b == folders_btn && folders_popup && overflow_btn) {
		const Rect2 r = overflow_btn->get_screen_rect();
		folders_popup->set_position(Vector2i(int(r.position.x), int(r.position.y + r.size.y)));
		folders_popup->popup();
		return;
	}
	if (b->is_toggle_mode()) {
		if (b->get_button_group().is_valid()) {
			b->set_pressed(true);
		} else {
			b->set_pressed(!b->is_pressed());
		}
	}
	b->emit_signal(SceneStringName(pressed));
}

void ClockGearsTopBar::_apply_bar_container_style() {
	if (applying_bar_style) {
		return;
	}
	applying_bar_style = true;
	const int ui_scale = _get_ui_scale_int();
	set_meta("_cg_theme_managed_container", true);
	add_theme_style_override(SceneStringName(panel), ClockGearsTheme::make_container_stylebox(float(ui_scale), 6, 2, 1));
	bar_h_padding = float(12 * ui_scale + 2);
	applying_bar_style = false;
}

ClockGearsTopBar::ClockGearsTopBar() {
	singleton = this;
	set_process_shortcut_input(true);

	const int ui_scale = _get_ui_scale_int();
	const int int_scale = ClockGearsIcons::get_editor_icon_scale(this);
	last_icon_scale = int_scale;

	_apply_bar_container_style();
	set_custom_minimum_size(Size2(0, 44 * ui_scale));
	set_clip_contents(true);

	clip_host = memnew(Control);
	clip_host->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	clip_host->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	clip_host->set_custom_minimum_size(Size2(0, 40 * ui_scale));
	clip_host->set_clip_contents(true);
	clip_host->set_mouse_filter(Control::MOUSE_FILTER_PASS);
	add_child(clip_host);

	row1_box = memnew(HBoxContainer);
	row1_box->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	row1_box->add_theme_constant_override("separation", 0);
	clip_host->add_child(row1_box);

	EditorNode *en = EditorNode::get_singleton();
	Ref<Theme> editor_theme = en ? en->get_editor_theme() : Ref<Theme>();
	const auto make_group = [ui_scale, this](const String &p_group_name) -> HBoxContainer * {
		HBoxContainer *g = memnew(HBoxContainer);
		g->add_theme_constant_override("separation", 2 * ui_scale);
		all_groups.push_back(g);
		group_names.push_back(p_group_name);
		return g;
	};
	const auto track_png = [this](Button *p_btn, const String &p_png, const String &p_fallback_txt = String(), const String &p_cg_fallback = String(), const String &p_svg_fallback = String()) -> Button * {
		ToolbarIconEntry e;
		e.button = p_btn;
		e.png_name = p_png;
		e.cg_fallback_icon = p_cg_fallback;
		e.svg_src = p_svg_fallback;
		e.fallback_text = p_fallback_txt;
		if (p_btn && p_btn->get_button_icon().is_null() && !p_fallback_txt.is_empty()) {
			p_btn->set_text(p_fallback_txt);
		}
		icon_entries.push_back(e);
		return p_btn;
	};
	const auto track_svg = [this](Button *p_btn, const String &p_svg, const String &p_fallback_txt = String()) -> Button * {
		ToolbarIconEntry e;
		e.button = p_btn;
		e.svg_src = p_svg;
		e.fallback_text = p_fallback_txt;
		if (p_btn && p_btn->get_button_icon().is_null() && !p_fallback_txt.is_empty()) {
			p_btn->set_text(p_fallback_txt);
		}
		icon_entries.push_back(e);
		return p_btn;
	};

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 0: Archivo
	// ------------------------------------------------------------------
	HBoxContainer *grp_file = make_group(CG_TR("Archivo"));
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

	folders_popup->add_item(CG_TR("Abrir Carpeta Raíz del Proyecto (res://)"), 100);
	folders_popup->add_item(CG_TR("Gestor de Recursos... (F10)"), 101);
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
	HBoxContainer *grp_edit = make_group(CG_TR("Edición"));
	track_png(_make_icon_button(grp_edit, _get_png_icon("cortar", int_scale, "cut"), CG_TR("Cortar (Ctrl+X)"), callable_mp(this, &ClockGearsTopBar::_on_cut_pressed), editor_theme, int_scale), "cortar", CG_TR("Cort."), "cut");
	track_png(_make_icon_button(grp_edit, _get_png_icon("copiar", int_scale, "copy"), CG_TR("Copiar (Ctrl+C)"), callable_mp(this, &ClockGearsTopBar::_on_copy_pressed), editor_theme, int_scale), "copiar", CG_TR("Cop."), "copy");
	track_png(_make_icon_button(grp_edit, _get_png_icon("pegar", int_scale, "paste"), CG_TR("Pegar (Ctrl+V)"), callable_mp(this, &ClockGearsTopBar::_on_paste_pressed), editor_theme, int_scale), "pegar", CG_TR("Peg."), "paste");
	undo_btn = track_png(_make_icon_button(grp_edit, _get_png_icon("deshacer_icono", int_scale), CG_TR("Deshacer (Ctrl+Z)"), callable_mp(this, &ClockGearsTopBar::_on_undo_pressed), editor_theme, int_scale), "deshacer_icono");
	redo_btn = track_png(_make_icon_button(grp_edit, _get_png_icon("rehacer_icono", int_scale), CG_TR("Rehacer (Ctrl+Y)"), callable_mp(this, &ClockGearsTopBar::_on_redo_pressed), editor_theme, int_scale), "rehacer_icono");

	// ------------------------------------------------------------------
	// Fila 1 (en 2 filas) - Grupo 2: Base de Datos y Modos
	// ------------------------------------------------------------------
	HBoxContainer *grp_db_modes = make_group(CG_TR("Base de Datos y Modos"));
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
	HBoxContainer *grp_layers = make_group(CG_TR("Capas"));
	Ref<ButtonGroup> layer_group;
	layer_group.instantiate();
	const char *layer_icons[6] = { "capa1_icono", "capa2_icono", "capa3_icono", "capa4_icono", "capa_auto_icono", "capa_r_icono" };
	const char *layer_tooltips[6] = { "Capa 1: Suelo (F5)", "Capa 2: Decoración (Shift+F6)", "Capa 3: Objetos (F7)", "Capa 4: Techo (★)", "Capa Automática", "Capa R: Regiones 0-255 (F8)" };
	for (int i = 0; i < 6; i++) {
		layer_buttons[i] = track_png(_make_icon_toggle_button(grp_layers, _get_png_icon(layer_icons[i], int_scale), CG_TR(layer_tooltips[i]), layer_group, editor_theme, int_scale), layer_icons[i]);
		layer_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_layer_pressed).bind(i + 1));
	}
	layer_buttons[0]->set_pressed(true);
	sync_active_layer_display();

	// ------------------------------------------------------------------
	// Grupos 4 y 5: Herramientas (dibujo y selección)
	// ------------------------------------------------------------------
	Ref<ButtonGroup> tool_group;
	tool_group.instantiate();

	HBoxContainer *grp_tools_draw = make_group(CG_TR("Dibujo"));
	tool_buttons[0] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("lapiz_icono", int_scale), CG_TR("Lápiz (Individual o Bloque)"), tool_group, editor_theme, int_scale), "lapiz_icono");
	tool_buttons[1] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("rectangulo_icono", int_scale), CG_TR("Rectángulo"), tool_group, editor_theme, int_scale), "rectangulo_icono");
	tool_buttons[2] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("elipse_icono", int_scale), CG_TR("Elipse"), tool_group, editor_theme, int_scale), "elipse_icono");
	tool_buttons[3] = track_png(_make_icon_toggle_button(grp_tools_draw, _get_png_icon("rellenar", int_scale), CG_TR("Cubo de Pintura (Flood Fill)"), tool_group, editor_theme, int_scale), "rellenar");
	Ref<Texture2D> shadow_ic = _get_png_icon("pincel_de_sombra", int_scale);
	if (shadow_ic.is_null()) {
		shadow_ic = _make_svg_icon(CLOCKGEARS_SHADOW_SVG, int_scale);
	}
	tool_buttons[4] = track_png(_make_icon_toggle_button(grp_tools_draw, shadow_ic, CG_TR("Pincel de Sombra"), tool_group, editor_theme, int_scale), "pincel_de_sombra", String(), String(), CLOCKGEARS_SHADOW_SVG);

	HBoxContainer *grp_tools_map = make_group(CG_TR("Selección y Mapa"));
	tool_buttons[5] = track_svg(_make_icon_toggle_button(grp_tools_map, _make_svg_icon(CLOCKGEARS_SELECT_SVG, int_scale), CG_TR("Selección Rectangular (Ctrl+C / Ctrl+V / Supr)"), tool_group, editor_theme, int_scale), CLOCKGEARS_SELECT_SVG);
	tool_buttons[6] = track_png(_make_icon_toggle_button(grp_tools_map, _get_png_icon("posicion_jugador", int_scale), CG_TR("Posición Inicial del Jugador (Clic para colocar / Clic repetido para rotar dirección)"), tool_group, editor_theme, int_scale), "posicion_jugador");
	tool_buttons[7] = track_png(_make_icon_toggle_button(grp_tools_map, _get_png_icon("goma_borrar", int_scale), CG_TR("Goma de Borrar (Eliminar tiles con clic o arrastre)"), tool_group, editor_theme, int_scale), "goma_borrar");

	for (int i = 0; i < 8; i++) {
		tool_buttons[i]->connect(SceneStringName(pressed), callable_mp(this, &ClockGearsTopBar::_on_tool_pressed).bind(i));
	}
	tool_buttons[0]->set_pressed(true);

	// ------------------------------------------------------------------
	// Grupos 6 y 7: Zoom y Ejecutar
	// ------------------------------------------------------------------
	HBoxContainer *grp_zoom = make_group(CG_TR("Zoom"));
	track_png(_make_icon_button(grp_zoom, _get_png_icon("zoom_icono", int_scale), CG_TR("Acercar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_in_pressed), editor_theme, int_scale), "zoom_icono");
	track_png(_make_icon_button(grp_zoom, _get_png_icon("alejar_icono", int_scale), CG_TR("Alejar Zoom"), callable_mp(this, &ClockGearsTopBar::_on_zoom_out_pressed), editor_theme, int_scale), "alejar_icono");

	HBoxContainer *grp_play = make_group(CG_TR("Ejecutar"));
	playtest_btn = track_png(_make_icon_button(grp_play, _get_png_icon("test_game", int_scale), CG_TR("Probar Juego (Playtest F5)"), callable_mp(this, &ClockGearsTopBar::_on_playtest_pressed), editor_theme, int_scale), "test_game");
	playtest_btn->set_text(CG_TR("Probar (F5)"));
	playtest_btn->set_icon_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	_apply_toolbar_button_style(playtest_btn, editor_theme, int_scale);

	for (int i = 0; i < all_groups.size(); i++) {
		if (i > 0) {
			Control *sep = _make_group_separator(ui_scale);
			all_separators.push_back(sep);
			row1_box->add_child(sep);
		}
		row1_box->add_child(all_groups[i]);
	}

	overflow_spacer = memnew(Control);
	overflow_spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	overflow_spacer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	overflow_spacer->hide();
	row1_box->add_child(overflow_spacer);

	overflow_btn = memnew(MenuButton);
	overflow_btn->set_flat(false);
	overflow_btn->set_text(String::utf8("»"));
	overflow_btn->set_tooltip_text(CG_TR("Más herramientas..."));
	overflow_btn->set_focus_mode(Control::FOCUS_NONE);
	overflow_btn->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	_apply_toolbar_button_style(overflow_btn, editor_theme, int_scale);
	overflow_btn->hide();
	row1_box->add_child(overflow_btn);

	overflow_popup = overflow_btn->get_popup();
	overflow_popup->connect("about_to_popup", callable_mp(this, &ClockGearsTopBar::_on_overflow_about_to_popup));
	overflow_popup->connect("id_pressed", callable_mp(this, &ClockGearsTopBar::_on_overflow_id_pressed));

	_relayout_rows();
}

ClockGearsTopBar::~ClockGearsTopBar() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
