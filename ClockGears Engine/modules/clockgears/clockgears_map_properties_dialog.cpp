#include "clockgears_map_properties_dialog.h"
#include "core/clockgears_data_manager.h"
#include "scene/gui/grid_container.h"

void ClockGearsMapPropertiesDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("popup_for_new_map", "next_id", "default_tileset_id"), &ClockGearsMapPropertiesDialog::popup_for_new_map, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("popup_for_edit_map", "id", "name", "display_name", "bgm", "width", "height", "tileset_id"), &ClockGearsMapPropertiesDialog::popup_for_edit_map, DEFVAL(0));

	ADD_SIGNAL(MethodInfo("map_properties_saved",
			PropertyInfo(Variant::INT, "id"),
			PropertyInfo(Variant::STRING, "name"),
			PropertyInfo(Variant::STRING, "display_name"),
			PropertyInfo(Variant::STRING, "bgm"),
			PropertyInfo(Variant::INT, "width"),
			PropertyInfo(Variant::INT, "height"),
			PropertyInfo(Variant::INT, "tileset_id"),
			PropertyInfo(Variant::BOOL, "is_new")));
}

void ClockGearsMapPropertiesDialog::_refresh_tileset_options(int p_selected_id) {
	if (!tileset_option) {
		return;
	}
	tileset_option->clear();
	tileset_option->add_item(String::utf8("(000: Ninguno)"), 0);

	int sel_idx = 0;
	if (ClockGearsDataManager::get_singleton()) {
		Array ts_arr = ClockGearsDataManager::get_singleton()->get_category_array(CAT_TILESETS);
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				String name = d.get("name", vformat("Tileset %03d", id));
				int item_idx = tileset_option->get_item_count();
				tileset_option->add_item(vformat("%03d: %s", id, name), id);
				if (id == p_selected_id) {
					sel_idx = item_idx;
				}
			}
		}
	}
	tileset_option->select(sel_idx);
}

void ClockGearsMapPropertiesDialog::_on_confirmed() {
	int selected_tileset_id = tileset_option ? tileset_option->get_selected_id() : 0;
	emit_signal("map_properties_saved",
			editing_map_id,
			name_edit->get_text(),
			display_name_edit->get_text(),
			bgm_option->get_item_text(bgm_option->get_selected()),
			(int)width_spin->get_value(),
			(int)height_spin->get_value(),
			selected_tileset_id,
			is_new_map);
}

void ClockGearsMapPropertiesDialog::popup_for_new_map(int p_next_id, int p_default_tileset_id) {
	editing_map_id = p_next_id;
	is_new_map = true;
	set_title(vformat(String::utf8("Propiedades del Mapa - Nuevo Mapa (ID: %03d)"), p_next_id));

	name_edit->set_text(vformat("MAP%03d", p_next_id));
	display_name_edit->set_text(vformat("Mapa %03d", p_next_id));
	_refresh_tileset_options(p_default_tileset_id);
	bgm_option->select(0);
	width_spin->set_value(25);
	height_spin->set_value(20);

	popup_centered(Size2(500, 360));
	display_name_edit->grab_focus();
}

void ClockGearsMapPropertiesDialog::popup_for_edit_map(int p_id, const String &p_name, const String &p_display_name, const String &p_bgm, int p_width, int p_height, int p_tileset_id) {
	editing_map_id = p_id;
	is_new_map = false;
	set_title(vformat(String::utf8("Propiedades del Mapa - Editar (ID: %03d: %s)"), p_id, p_display_name));

	name_edit->set_text(p_name);
	display_name_edit->set_text(p_display_name);
	_refresh_tileset_options(p_tileset_id);

	int bgm_idx = 0;
	for (int i = 0; i < bgm_option->get_item_count(); i++) {
		if (bgm_option->get_item_text(i) == p_bgm) {
			bgm_idx = i;
			break;
		}
	}
	bgm_option->select(bgm_idx);
	width_spin->set_value(MAX(10, p_width));
	height_spin->set_value(MAX(10, p_height));

	popup_centered(Size2(500, 360));
	display_name_edit->grab_focus();
}

ClockGearsMapPropertiesDialog::ClockGearsMapPropertiesDialog() {
	set_title(String::utf8("Propiedades del Mapa"));
	set_min_size(Size2(500, 360));

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->set_anchors_preset(Control::PRESET_FULL_RECT);
	main_vb->add_theme_constant_override("separation", 14);
	add_child(main_vb);

	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(2);
	grid->add_theme_constant_override("h_separation", 14);
	grid->add_theme_constant_override("v_separation", 12);
	main_vb->add_child(grid);

	// 1. Nombre interno
	grid->add_child(memnew(Label("Nombre Interno:")));
	name_edit = memnew(LineEdit);
	name_edit->set_placeholder("MAP001");
	name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_child(name_edit);

	// 2. Nombre a mostrar
	grid->add_child(memnew(Label("Nombre a Mostrar (In-Game):")));
	display_name_edit = memnew(LineEdit);
	display_name_edit->set_placeholder("Nombre del mapa...");
	display_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_child(display_name_edit);

	// 3. Tileset selector (Problema 3)
	grid->add_child(memnew(Label("TileSet:")));
	tileset_option = memnew(OptionButton);
	tileset_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_child(tileset_option);

	// 4. BGM (Regla Cero: sin pistas de prueba precreadas)
	grid->add_child(memnew(Label(String::utf8("M\xc3\xbasica de Fondo (BGM):"))));
	bgm_option = memnew(OptionButton);
	bgm_option->add_item("(Ninguna)", 0);
	bgm_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_child(bgm_option);

	// 5. Tipo de Scroll
	grid->add_child(memnew(Label("Desplazamiento:")));
	scroll_option = memnew(OptionButton);
	scroll_option->add_item(String::utf8("Sin bucle (Est\xc3\xa1ndar)"), 0);
	scroll_option->add_item("Bucle Horizontal", 1);
	scroll_option->add_item("Bucle Vertical", 2);
	scroll_option->add_item("Bucle Ambos (Mundial)", 3);
	scroll_option->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid->add_child(scroll_option);

	// 6. Dimensiones (Ancho x Alto en tiles)
	grid->add_child(memnew(Label("Dimensiones (Tiles):")));
	HBoxContainer *dim_hb = memnew(HBoxContainer);
	dim_hb->add_theme_constant_override("separation", 8);

	dim_hb->add_child(memnew(Label("Ancho:")));
	width_spin = memnew(SpinBox);
	width_spin->set_min(10);
	width_spin->set_max(256);
	width_spin->set_value(25);
	dim_hb->add_child(width_spin);

	dim_hb->add_child(memnew(Label("Alto:")));
	height_spin = memnew(SpinBox);
	height_spin->set_min(10);
	height_spin->set_max(256);
	height_spin->set_value(20);
	dim_hb->add_child(height_spin);

	grid->add_child(dim_hb);

	connect(SceneStringName(confirmed), callable_mp(this, &ClockGearsMapPropertiesDialog::_on_confirmed));
}
