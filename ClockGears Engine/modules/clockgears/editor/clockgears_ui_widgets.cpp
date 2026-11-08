#include "clockgears_ui_widgets.h"
#include "clockgears_theme.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_id_registry.h"
#include "core/object/callable_mp.h"
#include "icons/clockgears_icons.h"
#include "scene/gui/separator.h"

#ifdef TOOLS_ENABLED
#include "editor/themes/editor_scale.h"
#define CG_WSCALE EDSCALE
#else
#define CG_WSCALE 1.0f
#endif

// ============================================================================
// 1. ClockGearsFormulaLineEdit
// ============================================================================
void ClockGearsFormulaLineEdit::_on_formula_text_changed(const String &p_new_text) {
	revalidate();
}

void ClockGearsFormulaLineEdit::revalidate() {
	String expr = get_text().strip_edges();
	if (expr.is_empty()) {
		formula_valid = false;
		last_error_message = String::utf8("La fórmula no puede estar vacía");
		last_error_pos = 0;
	} else if (ClockGearsFormulas::get_singleton()) {
		Dictionary res = ClockGearsFormulas::get_singleton()->validate_formula(expr);
		formula_valid = bool(res.get("valid", false));
		last_error_message = res.get("error", "");
		last_error_pos = int(res.get("error_pos", -1));
	} else {
		formula_valid = true;
		last_error_message = "";
		last_error_pos = -1;
	}
	_update_visual_state();
	emit_signal("validation_changed", formula_valid, last_error_message, last_error_pos);
}

void ClockGearsFormulaLineEdit::_update_visual_state() {
	Ref<StyleBoxFlat> sb_normal = ClockGearsTheme::make_input_stylebox(!formula_valid, false, CG_WSCALE);
	Ref<StyleBoxFlat> sb_focus = ClockGearsTheme::make_input_stylebox(!formula_valid, true, CG_WSCALE);
	add_theme_style_override("normal", sb_normal);
	add_theme_style_override("focus", sb_focus);

	if (formula_valid) {
		add_theme_color_override("font_color", ClockGearsTheme::color_text_primary());
		set_tooltip_text(String::utf8("Fórmula válida (Variables: a.atk, b.def, v[n], s[n]; Funciones: max, min, abs, floor, ceil, round, rand)"));
		if (ClockGearsIcons::get_singleton()) {
			set_right_icon(ClockGearsIcons::get_singleton()->get_icon("ok", CG_WSCALE));
		}
	} else {
		add_theme_color_override("font_color", ClockGearsTheme::color_error());
		set_tooltip_text(vformat(String::utf8("Error de sintaxis: %s"), last_error_message));
		if (ClockGearsIcons::get_singleton()) {
			set_right_icon(ClockGearsIcons::get_singleton()->get_icon("error", CG_WSCALE));
		}
	}
}

void ClockGearsFormulaLineEdit::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		revalidate();
	}
}

void ClockGearsFormulaLineEdit::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_formula_valid"), &ClockGearsFormulaLineEdit::is_formula_valid);
	ClassDB::bind_method(D_METHOD("get_last_error_message"), &ClockGearsFormulaLineEdit::get_last_error_message);
	ClassDB::bind_method(D_METHOD("get_last_error_pos"), &ClockGearsFormulaLineEdit::get_last_error_pos);
	ClassDB::bind_method(D_METHOD("revalidate"), &ClockGearsFormulaLineEdit::revalidate);

	ADD_SIGNAL(MethodInfo("validation_changed", PropertyInfo(Variant::BOOL, "is_valid"), PropertyInfo(Variant::STRING, "error_message"), PropertyInfo(Variant::INT, "error_pos")));
}

ClockGearsFormulaLineEdit::ClockGearsFormulaLineEdit() {
	set_placeholder("max(a.atk * 4 - b.def * 2, 0)");
	connect("text_changed", callable_mp(this, &ClockGearsFormulaLineEdit::_on_formula_text_changed));
}

// ============================================================================
// 2. ClockGearsSwitchVarPickerDialog
// ============================================================================
Dictionary ClockGearsSwitchVarPickerDialog::_get_source_dict() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return Dictionary();
	}
	return variable_mode ? dm->get_variables_names() : dm->get_switches_names();
}

void ClockGearsSwitchVarPickerDialog::_set_source_dict(const Dictionary &p_dict) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	if (variable_mode) {
		dm->set_variables_names(p_dict);
	} else {
		dm->set_switches_names(p_dict);
	}
}

void ClockGearsSwitchVarPickerDialog::_rebuild_list() {
	if (!entry_list) {
		return;
	}
	entry_list->clear();
	Dictionary dict = _get_source_dict();
	String filter = search_edit ? search_edit->get_text().strip_edges().to_lower() : "";

	Array keys = dict.keys();
	Vector<int> sorted_ids;
	for (int i = 0; i < keys.size(); i++) {
		int id = int(keys[i]);
		if (id >= 1) {
			sorted_ids.push_back(id);
		}
	}
	sorted_ids.sort();

	int selected_idx = -1;
	for (int i = 0; i < sorted_ids.size(); i++) {
		int id = sorted_ids[i];
		String nm = dict.has(id) ? String(dict[id]) : String(dict.get(itos(id), ""));
		String label_text = vformat("%04d: %s", id, nm.is_empty() ? String::utf8("(Sin nombre)") : nm);
		if (!filter.is_empty() && !label_text.to_lower().contains(filter)) {
			continue;
		}
		int idx = entry_list->add_item(label_text);
		entry_list->set_item_metadata(idx, id);
		if (id == current_selected_id) {
			selected_idx = idx;
		}
	}

	bool is_empty = (sorted_ids.is_empty());
	if (empty_state_label) {
		empty_state_label->set_visible(is_empty);
		empty_state_label->set_text(variable_mode
						? String::utf8("Lista vacía. Pulsa '+' para crear tu primera Variable.")
						: String::utf8("Lista vacía. Pulsa '+' para crear tu primer Interruptor."));
	}

	if (selected_idx >= 0) {
		entry_list->select(selected_idx);
		_on_item_selected(selected_idx);
	} else if (entry_list->get_item_count() > 0) {
		entry_list->select(0);
		_on_item_selected(0);
	} else {
		current_selected_id = 0;
		if (rename_edit) {
			rename_edit->set_text("");
			rename_edit->set_editable(false);
		}
		if (btn_remove) {
			btn_remove->set_disabled(true);
		}
	}
}

void ClockGearsSwitchVarPickerDialog::_on_search_changed(const String &p_filter) {
	_rebuild_list();
}

void ClockGearsSwitchVarPickerDialog::_on_item_selected(int p_idx) {
	if (!entry_list || p_idx < 0 || p_idx >= entry_list->get_item_count()) {
		return;
	}
	current_selected_id = int(entry_list->get_item_metadata(p_idx));
	Dictionary dict = _get_source_dict();
	String nm = dict.has(current_selected_id) ? String(dict[current_selected_id]) : String(dict.get(itos(current_selected_id), ""));
	if (rename_edit) {
		rename_edit->set_editable(true);
		rename_edit->set_text(nm);
	}
	if (btn_remove) {
		btn_remove->set_disabled(false);
	}
}

void ClockGearsSwitchVarPickerDialog::_on_item_activated(int p_idx) {
	_on_item_selected(p_idx);
	_on_confirmed();
	hide();
}

void ClockGearsSwitchVarPickerDialog::_on_add_pressed() {
	Dictionary dict = _get_source_dict();
	String pool_key = variable_mode ? "variables" : "switches";
	int next_id = 1;
	if (ClockGearsIdRegistry::get_singleton()) {
		next_id = ClockGearsIdRegistry::get_singleton()->allocate_id(pool_key);
	} else {
		while (dict.has(next_id) || dict.has(itos(next_id))) {
			next_id++;
		}
	}
	dict[next_id] = "";
	_set_source_dict(dict);
	current_selected_id = next_id;
	_rebuild_list();
	if (rename_edit) {
		rename_edit->grab_focus();
	}
}

void ClockGearsSwitchVarPickerDialog::_on_remove_pressed() {
	if (current_selected_id <= 0) {
		return;
	}
	Dictionary dict = _get_source_dict();
	dict.erase(current_selected_id);
	dict.erase(itos(current_selected_id));
	if (ClockGearsIdRegistry::get_singleton()) {
		ClockGearsIdRegistry::get_singleton()->retire_id(variable_mode ? "variables" : "switches", current_selected_id);
	}
	_set_source_dict(dict);
	current_selected_id = 0;
	_rebuild_list();
}

void ClockGearsSwitchVarPickerDialog::_on_rename_changed(const String &p_new_name) {
	if (current_selected_id <= 0) {
		return;
	}
	Dictionary dict = _get_source_dict();
	dict[current_selected_id] = p_new_name;
	_set_source_dict(dict);

	for (int i = 0; i < entry_list->get_item_count(); i++) {
		if (int(entry_list->get_item_metadata(i)) == current_selected_id) {
			entry_list->set_item_text(i, vformat("%04d: %s", current_selected_id, p_new_name.is_empty() ? String::utf8("(Sin nombre)") : p_new_name));
			break;
		}
	}
}

void ClockGearsSwitchVarPickerDialog::_on_confirmed() {
	if (current_selected_id <= 0) {
		return;
	}
	Dictionary dict = _get_source_dict();
	String nm = dict.has(current_selected_id) ? String(dict[current_selected_id]) : String(dict.get(itos(current_selected_id), ""));
	emit_signal("entry_selected", current_selected_id, nm);
}

void ClockGearsSwitchVarPickerDialog::open_picker(bool p_variable_mode, int p_initial_id) {
	variable_mode = p_variable_mode;
	current_selected_id = p_initial_id;
	set_title(variable_mode
					? String::utf8("Selector de Variables — ClockGears Engine")
					: String::utf8("Selector de Interruptores — ClockGears Engine"));
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	_rebuild_list();
	popup_centered(Size2(460, 420) * CG_WSCALE);
}

void ClockGearsSwitchVarPickerDialog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_picker", "variable_mode", "initial_id"), &ClockGearsSwitchVarPickerDialog::open_picker, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_selected_entry_id"), &ClockGearsSwitchVarPickerDialog::get_selected_entry_id);

	ADD_SIGNAL(MethodInfo("entry_selected", PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::STRING, "name")));
}

ClockGearsSwitchVarPickerDialog::ClockGearsSwitchVarPickerDialog() {
	set_ok_button_text(String::utf8("Seleccionar"));
	add_cancel_button(String::utf8("Cancelar"));

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", int(8 * CG_WSCALE));
	add_child(vb);

	// Barra de búsqueda superior
	HBoxContainer *search_hb = memnew(HBoxContainer);
	vb->add_child(search_hb);

	search_edit = memnew(LineEdit);
	search_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	search_edit->set_placeholder(String::utf8("Buscar por ID o nombre..."));
	search_edit->set_clear_button_enabled(true);
	if (ClockGearsIcons::get_singleton()) {
		search_edit->set_right_icon(ClockGearsIcons::get_singleton()->get_icon("search", CG_WSCALE));
	}
	search_edit->connect("text_changed", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_search_changed));
	search_hb->add_child(search_edit);

	btn_add = memnew(Button(String::utf8("+ Nuevo")));
	if (ClockGearsIcons::get_singleton()) {
		btn_add->set_button_icon(ClockGearsIcons::get_singleton()->get_icon("add", CG_WSCALE));
	}
	btn_add->connect("pressed", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_add_pressed));
	search_hb->add_child(btn_add);

	btn_remove = memnew(Button(String::utf8("Borrar")));
	if (ClockGearsIcons::get_singleton()) {
		btn_remove->set_button_icon(ClockGearsIcons::get_singleton()->get_icon("remove", CG_WSCALE));
	}
	btn_remove->connect("pressed", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_remove_pressed));
	search_hb->add_child(btn_remove);

	// Mensaje de estado vacío claro (Sección 11.1)
	empty_state_label = memnew(Label);
	empty_state_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	empty_state_label->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	vb->add_child(empty_state_label);

	entry_list = memnew(ItemList);
	entry_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	entry_list->set_custom_minimum_size(Size2(0, 220 * CG_WSCALE));
	entry_list->connect("item_selected", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_item_selected));
	entry_list->connect("item_activated", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_item_activated));
	vb->add_child(entry_list);

	// Editor de nombre del elemento seleccionado
	HBoxContainer *rename_hb = memnew(HBoxContainer);
	rename_hb->add_child(memnew(Label(String::utf8("Nombre:"))));
	rename_edit = memnew(LineEdit);
	rename_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rename_edit->set_placeholder(String::utf8("Nombre descriptivo..."));
	rename_edit->connect("text_changed", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_rename_changed));
	rename_hb->add_child(rename_edit);
	vb->add_child(rename_hb);

	connect("confirmed", callable_mp(this, &ClockGearsSwitchVarPickerDialog::_on_confirmed));
}

// ============================================================================
// 3. ClockGearsSwitchVarPickerButton
// ============================================================================
void ClockGearsSwitchVarPickerButton::_update_button_label() {
	if (!pick_button) {
		return;
	}
	if (selected_id <= 0 && selected_name.is_empty()) {
		pick_button->set_text(String::utf8("(ninguno)"));
	} else if (selected_id > 0) {
		pick_button->set_text(vformat("%04d: %s", selected_id, selected_name.is_empty() ? String::utf8("(Sin nombre)") : selected_name));
	} else {
		pick_button->set_text(selected_name);
	}
}

void ClockGearsSwitchVarPickerButton::_on_pick_pressed() {
	if (picker_dialog) {
		picker_dialog->open_picker(variable_mode, selected_id);
	}
}

void ClockGearsSwitchVarPickerButton::_on_clear_pressed() {
	selected_id = 0;
	selected_name = "";
	_update_button_label();
	emit_signal("value_changed", selected_id, selected_name);
}

void ClockGearsSwitchVarPickerButton::_on_dialog_entry_selected(int p_id, const String &p_name) {
	selected_id = p_id;
	selected_name = p_name;
	_update_button_label();
	emit_signal("value_changed", selected_id, selected_name);
}

void ClockGearsSwitchVarPickerButton::set_variable_mode(bool p_var_mode) {
	variable_mode = p_var_mode;
}

void ClockGearsSwitchVarPickerButton::set_selected_id(int p_id) {
	selected_id = MAX(0, p_id);
	if (selected_id > 0 && ClockGearsDataManager::get_singleton()) {
		Dictionary dict = variable_mode
				? ClockGearsDataManager::get_singleton()->get_variables_names()
				: ClockGearsDataManager::get_singleton()->get_switches_names();
		if (dict.has(selected_id)) {
			selected_name = String(dict[selected_id]);
		} else if (dict.has(itos(selected_id))) {
			selected_name = String(dict[itos(selected_id)]);
		}
	}
	_update_button_label();
}

void ClockGearsSwitchVarPickerButton::set_selected_name(const String &p_name) {
	selected_name = p_name;
	_update_button_label();
}

void ClockGearsSwitchVarPickerButton::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_variable_mode", "var_mode"), &ClockGearsSwitchVarPickerButton::set_variable_mode);
	ClassDB::bind_method(D_METHOD("is_variable_mode"), &ClockGearsSwitchVarPickerButton::is_variable_mode);
	ClassDB::bind_method(D_METHOD("set_selected_id", "id"), &ClockGearsSwitchVarPickerButton::set_selected_id);
	ClassDB::bind_method(D_METHOD("get_selected_id"), &ClockGearsSwitchVarPickerButton::get_selected_id);
	ClassDB::bind_method(D_METHOD("set_selected_name", "name"), &ClockGearsSwitchVarPickerButton::set_selected_name);
	ClassDB::bind_method(D_METHOD("get_selected_name"), &ClockGearsSwitchVarPickerButton::get_selected_name);

	ADD_SIGNAL(MethodInfo("value_changed", PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::STRING, "name")));
}

ClockGearsSwitchVarPickerButton::ClockGearsSwitchVarPickerButton(bool p_variable_mode) {
	variable_mode = p_variable_mode;
	add_theme_constant_override("separation", int(4 * CG_WSCALE));

	pick_button = memnew(Button);
	pick_button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pick_button->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	pick_button->set_clip_text(true);
	pick_button->connect("pressed", callable_mp(this, &ClockGearsSwitchVarPickerButton::_on_pick_pressed));
	add_child(pick_button);

	clear_button = memnew(Button("X"));
	clear_button->set_tooltip_text(String::utf8("Limpiar selección"));
	clear_button->connect("pressed", callable_mp(this, &ClockGearsSwitchVarPickerButton::_on_clear_pressed));
	add_child(clear_button);

	picker_dialog = memnew(ClockGearsSwitchVarPickerDialog);
	picker_dialog->connect("entry_selected", callable_mp(this, &ClockGearsSwitchVarPickerButton::_on_dialog_entry_selected));
	add_child(picker_dialog);

	_update_button_label();
}

// ============================================================================
// 4. ClockGearsRecordPickerButton
// ============================================================================
void ClockGearsRecordPickerButton::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm && !dm->is_connected("data_changed", callable_mp(this, &ClockGearsRecordPickerButton::_on_data_manager_changed))) {
			dm->connect("data_changed", callable_mp(this, &ClockGearsRecordPickerButton::_on_data_manager_changed));
		}
		refresh_options();
	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		// Bug #3: Desconectar señales al salir del árbol para evitar punteros colgantes
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm && dm->is_connected("data_changed", callable_mp(this, &ClockGearsRecordPickerButton::_on_data_manager_changed))) {
			dm->disconnect("data_changed", callable_mp(this, &ClockGearsRecordPickerButton::_on_data_manager_changed));
		}
	}
}

void ClockGearsRecordPickerButton::_on_data_manager_changed(const String &p_category_key, int p_id) {
	if (p_category_key == ClockGearsDataManager::get_category_key(target_category)) {
		refresh_options();
	}
}

void ClockGearsRecordPickerButton::set_target_category(int p_category) {
	target_category = CLAMP(p_category, 0, CAT_MAX - 1);
	refresh_options();
}

void ClockGearsRecordPickerButton::refresh_options() {
	updating_items = true;
	clear();
	// Opción 0 siempre es "(ninguno)" con ID 0 (Sección 5.3)
	add_item(String::utf8("(ninguno)"), 0);
	set_item_metadata(0, 0);

	int found_idx = 0;
	if (ClockGearsDataManager::get_singleton()) {
		Array records = ClockGearsDataManager::get_singleton()->get_category_array(target_category);
		for (int i = 0; i < records.size(); i++) {
			if (records[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary rec = records[i];
			int id = int(rec.get("id", i + 1));
			String nm = rec.get("name", "");
			int idx = get_item_count();
			add_item(vformat("%03d: %s", id, nm.is_empty() ? String::utf8("(Sin nombre)") : nm), id);
			set_item_metadata(idx, id);
			if (id == selected_record_id) {
				found_idx = idx;
			}
		}
	}

	select(found_idx);
	if (found_idx == 0) {
		selected_record_id = 0;
	}
	updating_items = false;
}

void ClockGearsRecordPickerButton::set_selected_record_id(int p_id) {
	selected_record_id = MAX(0, p_id);
	refresh_options();
}

void ClockGearsRecordPickerButton::_on_item_idx_selected(int p_idx) {
	if (updating_items || p_idx < 0 || p_idx >= get_item_count()) {
		return;
	}
	selected_record_id = int(get_item_metadata(p_idx));
	emit_signal("record_id_selected", selected_record_id);
}

void ClockGearsRecordPickerButton::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_category", "category"), &ClockGearsRecordPickerButton::set_target_category);
	ClassDB::bind_method(D_METHOD("get_target_category"), &ClockGearsRecordPickerButton::get_target_category);
	ClassDB::bind_method(D_METHOD("refresh_options"), &ClockGearsRecordPickerButton::refresh_options);
	ClassDB::bind_method(D_METHOD("set_selected_record_id", "id"), &ClockGearsRecordPickerButton::set_selected_record_id);
	ClassDB::bind_method(D_METHOD("get_selected_record_id"), &ClockGearsRecordPickerButton::get_selected_record_id);

	ADD_SIGNAL(MethodInfo("record_id_selected", PropertyInfo(Variant::INT, "id")));
}

ClockGearsRecordPickerButton::ClockGearsRecordPickerButton(int p_category) {
	target_category = CLAMP(p_category, 0, CAT_MAX - 1);
	connect("item_selected", callable_mp(this, &ClockGearsRecordPickerButton::_on_item_idx_selected));
	refresh_options();
}
