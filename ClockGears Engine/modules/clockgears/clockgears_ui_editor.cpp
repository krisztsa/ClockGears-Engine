#include "clockgears_ui_editor.h"
#include "scene/gui/split_container.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/panel.h"
#include "scene/main/canvas_layer.h"
#include "scene/resources/style_box_flat.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"

void ClockGearsUIEditorPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_widget_palette_activated", "idx"), &ClockGearsUIEditorPanel::_on_widget_palette_activated);
	ClassDB::bind_method(D_METHOD("_select_widget", "idx"), &ClockGearsUIEditorPanel::_select_widget);
	ClassDB::bind_method(D_METHOD("_update_widget_properties"), &ClockGearsUIEditorPanel::_update_widget_properties);
	ClassDB::bind_method(D_METHOD("_compile_hud_to_scene"), &ClockGearsUIEditorPanel::_compile_hud_to_scene);
}

void ClockGearsUIEditorPanel::_on_widget_palette_activated(int p_idx) {
	Dictionary w;
	switch (p_idx) {
		case 0: // HP Bar
			w["type"] = "hp_bar";
			w["name"] = "Barra_HP";
			w["x"] = 20; w["y"] = 20; w["w"] = 200; w["h"] = 24;
			w["color"] = Color(0.85, 0.15, 0.15);
			break;
		case 1: // MP Bar
			w["type"] = "mp_bar";
			w["name"] = "Barra_MP";
			w["x"] = 20; w["y"] = 50; w["w"] = 160; w["h"] = 18;
			w["color"] = Color(0.2, 0.45, 0.95);
			break;
		case 2: // Dialogue Window
			w["type"] = "dialogue";
			w["name"] = "Ventana_Dialogo";
			w["x"] = 100; w["y"] = 480; w["w"] = 1080; w["h"] = 180;
			w["color"] = Color(0.08, 0.10, 0.18, 0.9);
			break;
		case 3: // Pause Menu
			w["type"] = "menu";
			w["name"] = "Menu_Pausa";
			w["x"] = 920; w["y"] = 40; w["w"] = 280; w["h"] = 380;
			w["color"] = Color(0.05, 0.08, 0.15, 0.95);
			break;
		case 4: // Gold Counter
			w["type"] = "gold";
			w["name"] = "Contador_Oro";
			w["x"] = 1060; w["y"] = 660; w["w"] = 180; w["h"] = 36;
			w["color"] = Color(0.95, 0.75, 0.2);
			break;
		case 5: // Minimap
			w["type"] = "minimap";
			w["name"] = "Minimapa";
			w["x"] = 1060; w["y"] = 20; w["w"] = 180; w["h"] = 180;
			w["color"] = Color(0.12, 0.16, 0.25, 0.85);
			break;
		case 6: // Battle Action Menu
			w["type"] = "battle_actions";
			w["name"] = "Menu_Batalla";
			w["x"] = 40; w["y"] = 520; w["w"] = 320; w["h"] = 160;
			w["color"] = Color(0.1, 0.14, 0.24, 0.92);
			break;
	}

	placed_widgets.push_back(w);
	_refresh_canvas();
	_select_widget(placed_widgets.size() - 1);
}

void ClockGearsUIEditorPanel::_refresh_canvas() {
	// Clear previous preview elements
	while (wysiwyg_canvas->get_child_count() > 0) {
		Node *child = wysiwyg_canvas->get_child(0);
		wysiwyg_canvas->remove_child(child);
		memdelete(child);
	}

	for (int i = 0; i < placed_widgets.size(); i++) {
		Dictionary w = placed_widgets[i];
		String type = w["type"];
		Color col = w["color"];
		float x = w["x"];
		float y = w["y"];
		float width = w["w"];
		float height = w["h"];

		// Scale for the 640x360 preview canvas (original 1280x720)
		float scale = 0.5f;

		Panel *box = memnew(Panel);
		box->set_position(Vector2(x * scale, y * scale));
		box->set_size(Vector2(width * scale, height * scale));

		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_bg_color(col);
		sb->set_corner_radius_all(3);
		if (i == selected_widget_idx) {
			sb->set_border_width_all(2);
			sb->set_border_color(Color(1, 0.8, 0.2));
		}
		box->add_theme_style_override("panel", sb);

		Label *lbl = memnew(Label(String(w["name"])));
		lbl->add_theme_font_size_override("font_size", 11);
		box->add_child(lbl);

		Button *pick_btn = memnew(Button);
		pick_btn->set_flat(true);
		pick_btn->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		pick_btn->connect("pressed", callable_mp(this, &ClockGearsUIEditorPanel::_select_widget).bind(i));
		box->add_child(pick_btn);

		wysiwyg_canvas->add_child(box);
	}
}

void ClockGearsUIEditorPanel::_on_add_button_pressed() {
	Vector<int> sel = widgets_palette->get_selected_items();
	if (!sel.is_empty()) _on_widget_palette_activated(sel[0]);
}

void ClockGearsUIEditorPanel::_on_spin_value_changed(double) {
	_update_widget_properties();
}

void ClockGearsUIEditorPanel::_on_color_changed(const Color &) {
	_update_widget_properties();
}

void ClockGearsUIEditorPanel::_select_widget(int p_idx) {
	if (p_idx < 0 || p_idx >= placed_widgets.size()) return;
	selected_widget_idx = p_idx;

	Dictionary w = placed_widgets[p_idx];
	pos_x_spin->set_value(w["x"]);
	pos_y_spin->set_value(w["y"]);
	size_w_spin->set_value(w["w"]);
	size_h_spin->set_value(w["h"]);
	color_btn->set_pick_color(w["color"]);

	_refresh_canvas();
}

void ClockGearsUIEditorPanel::_update_widget_properties() {
	if (selected_widget_idx < 0 || selected_widget_idx >= placed_widgets.size()) return;

	Dictionary w = placed_widgets[selected_widget_idx];
	w["x"] = pos_x_spin->get_value();
	w["y"] = pos_y_spin->get_value();
	w["w"] = size_w_spin->get_value();
	w["h"] = size_h_spin->get_value();
	w["color"] = color_btn->get_pick_color();
	placed_widgets[selected_widget_idx] = w;

	_refresh_canvas();
}

void ClockGearsUIEditorPanel::_compile_hud_to_scene() {
	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) {
		EditorNode::get_singleton()->show_warning("Por favor abre o crea una escena primero.");
		return;
	}

	CanvasLayer *hud = memnew(CanvasLayer);
	hud->set_name("HUD_Juego_RPG");

	for (int i = 0; i < placed_widgets.size(); i++) {
		Dictionary w = placed_widgets[i];
		String type = w["type"];
		Color col = w["color"];
		float x = w["x"];
		float y = w["y"];
		float width = w["w"];
		float height = w["h"];

		if (type == "hp_bar" || type == "mp_bar") {
			ProgressBar *pb = memnew(ProgressBar);
			pb->set_name(String(w["name"]));
			pb->set_position(Vector2(x, y));
			pb->set_size(Vector2(width, height));
			pb->set_value(100.0);
			hud->add_child(pb);
		} else {
			Panel *p = memnew(Panel);
			p->set_name(String(w["name"]));
			p->set_position(Vector2(x, y));
			p->set_size(Vector2(width, height));
			Ref<StyleBoxFlat> sb;
			sb.instantiate();
			sb->set_bg_color(col);
			p->add_theme_style_override("panel", sb);

			Label *l = memnew(Label(String(w["name"])));
			p->add_child(l);
			hud->add_child(p);
		}
	}

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action("Inyectar HUD RPG Generado");
	ur->add_do_method(scene_root, "add_child", hud);
	ur->add_do_method(hud, "set_owner", scene_root);
	for (int i = 0; i < hud->get_child_count(); i++) {
		ur->add_do_method(hud->get_child(i), "set_owner", scene_root);
	}
	ur->add_undo_method(scene_root, "remove_child", hud);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(String::utf8("¡HUD compilado y añadido a la escena exitosamente con soporte Drag & Drop WYSIWYG!"));
}

ClockGearsUIEditorPanel::ClockGearsUIEditorPanel() {
	set_name(String::utf8("Diseñador UI/HUD"));

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 10);
	add_child(main_vb);

	Label *hdr = memnew(Label(String::utf8("DISEÑADOR WYSIWYG DE UI Y HUD (DRAG & DROP)")));
	hdr->add_theme_font_size_override("font_size", 18);
	main_vb->add_child(hdr);

	HSplitContainer *hsplit = memnew(HSplitContainer);
	hsplit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->add_child(hsplit);

	// Left Palette
	VBoxContainer *left_pal = memnew(VBoxContainer);
	left_pal->set_custom_minimum_size(Size2(220, 0));
	hsplit->add_child(left_pal);

	left_pal->add_child(memnew(Label("Paleta de Widgets RPG:")));
	widgets_palette = memnew(ItemList);
	widgets_palette->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	widgets_palette->add_item("Barra de Vida (HP)");
	widgets_palette->add_item("Barra de Mana (MP)");
	widgets_palette->add_item(String::utf8("Ventana de Diálogo"));
	widgets_palette->add_item(String::utf8("Menú de Pausa"));
	widgets_palette->add_item("Contador de Oro");
	widgets_palette->add_item("Minimapa HUD");
	widgets_palette->add_item("Acciones de Batalla");
	widgets_palette->connect("item_activated", callable_mp(this, &ClockGearsUIEditorPanel::_on_widget_palette_activated));
	left_pal->add_child(widgets_palette);

	Button *add_btn = memnew(Button("+ Insertar al Lienzo"));
	add_btn->connect("pressed", callable_mp(this, &ClockGearsUIEditorPanel::_on_add_button_pressed));
	left_pal->add_child(add_btn);

	// Center WYSIWYG Canvas Preview
	VBoxContainer *center_vb = memnew(VBoxContainer);
	center_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hsplit->add_child(center_vb);

	center_vb->add_child(memnew(Label(String::utf8("Lienzo de Previsualización (Escala 16:9 - 1280x720):"))));

	PanelContainer *canvas_frame = memnew(PanelContainer);
	canvas_frame->set_custom_minimum_size(Size2(640, 360));
	center_vb->add_child(canvas_frame);

	Ref<StyleBoxFlat> canvas_sb;
	canvas_sb.instantiate();
	canvas_sb->set_bg_color(Color(0.06, 0.07, 0.10));
	canvas_sb->set_border_width_all(2);
	canvas_sb->set_border_color(Color(0.3, 0.35, 0.5));
	canvas_frame->add_theme_style_override("panel", canvas_sb);

	wysiwyg_canvas = memnew(Control);
	wysiwyg_canvas->set_custom_minimum_size(Size2(640, 360));
	canvas_frame->add_child(wysiwyg_canvas);

	// Right Inspector
	inspector_vb = memnew(VBoxContainer);
	inspector_vb->set_custom_minimum_size(Size2(220, 0));
	hsplit->add_child(inspector_vb);

	inspector_vb->add_child(memnew(Label("Propiedades del Widget:")));

	inspector_vb->add_child(memnew(Label(String::utf8("Posición X:"))));
	pos_x_spin = memnew(SpinBox);
	pos_x_spin->set_max(1920);
	pos_x_spin->connect("value_changed", callable_mp(this, &ClockGearsUIEditorPanel::_on_spin_value_changed));
	inspector_vb->add_child(pos_x_spin);

	inspector_vb->add_child(memnew(Label(String::utf8("Posición Y:"))));
	pos_y_spin = memnew(SpinBox);
	pos_y_spin->set_max(1080);
	pos_y_spin->connect("value_changed", callable_mp(this, &ClockGearsUIEditorPanel::_on_spin_value_changed));
	inspector_vb->add_child(pos_y_spin);

	inspector_vb->add_child(memnew(Label("Ancho:")));
	size_w_spin = memnew(SpinBox);
	size_w_spin->set_max(1920);
	size_w_spin->connect("value_changed", callable_mp(this, &ClockGearsUIEditorPanel::_on_spin_value_changed));
	inspector_vb->add_child(size_w_spin);

	inspector_vb->add_child(memnew(Label("Alto:")));
	size_h_spin = memnew(SpinBox);
	size_h_spin->set_max(1080);
	size_h_spin->connect("value_changed", callable_mp(this, &ClockGearsUIEditorPanel::_on_spin_value_changed));
	inspector_vb->add_child(size_h_spin);

	inspector_vb->add_child(memnew(Label(String::utf8("Color / Modulación:"))));
	color_btn = memnew(ColorPickerButton);
	color_btn->connect("color_changed", callable_mp(this, &ClockGearsUIEditorPanel::_on_color_changed));
	inspector_vb->add_child(color_btn);

	Button *compile_btn = memnew(Button("APLICAR HUD AL JUEGO"));
	compile_btn->connect("pressed", callable_mp(this, &ClockGearsUIEditorPanel::_compile_hud_to_scene));
	inspector_vb->add_child(compile_btn);

	// Add initial demo widgets
	_on_widget_palette_activated(0); // HP
	_on_widget_palette_activated(1); // MP
	_on_widget_palette_activated(2); // Dialogue
}
