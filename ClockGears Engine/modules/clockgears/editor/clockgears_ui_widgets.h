#pragma once

#include "core/clockgears_data_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"

// ============================================================================
// 1. ClockGearsFormulaLineEdit: Validación en vivo de fórmulas (Sección 9.9 / 11.1)
// ============================================================================
class ClockGearsFormulaLineEdit : public LineEdit {
	GDCLASS(ClockGearsFormulaLineEdit, LineEdit);

private:
	bool formula_valid = true;
	String last_error_message;
	int last_error_pos = -1;

	void _on_formula_text_changed(const String &p_new_text);
	void _update_visual_state();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	bool is_formula_valid() const { return formula_valid; }
	String get_last_error_message() const { return last_error_message; }
	int get_last_error_pos() const { return last_error_pos; }

	void revalidate();

	ClockGearsFormulaLineEdit();
};

// ============================================================================
// 2. ClockGearsSwitchVarPickerDialog: Selector modal de Interruptores / Variables
// ============================================================================
class ClockGearsSwitchVarPickerDialog : public AcceptDialog {
	GDCLASS(ClockGearsSwitchVarPickerDialog, AcceptDialog);

private:
	bool variable_mode = false;
	int current_selected_id = 0;

	LineEdit *search_edit = nullptr;
	ItemList *entry_list = nullptr;
	Label *empty_state_label = nullptr;
	LineEdit *rename_edit = nullptr;
	Button *btn_add = nullptr;
	Button *btn_remove = nullptr;

	Dictionary _get_source_dict() const;
	void _set_source_dict(const Dictionary &p_dict);
	void _rebuild_list();

	void _on_search_changed(const String &p_filter);
	void _on_item_selected(int p_idx);
	void _on_item_activated(int p_idx);
	void _on_add_pressed();
	void _on_remove_pressed();
	void _on_rename_changed(const String &p_new_name);
	void _on_confirmed();

protected:
	static void _bind_methods();

public:
	void open_picker(bool p_variable_mode, int p_initial_id = 0);
	int get_selected_entry_id() const { return current_selected_id; }

	ClockGearsSwitchVarPickerDialog();
};

// ============================================================================
// 3. ClockGearsSwitchVarPickerButton: Botón compacto que abre el Picker
// ============================================================================
class ClockGearsSwitchVarPickerButton : public HBoxContainer {
	GDCLASS(ClockGearsSwitchVarPickerButton, HBoxContainer);

private:
	bool variable_mode = false;
	int selected_id = 0;
	String selected_name;

	Button *pick_button = nullptr;
	Button *clear_button = nullptr;
	ClockGearsSwitchVarPickerDialog *picker_dialog = nullptr;

	void _update_button_label();
	void _on_pick_pressed();
	void _on_clear_pressed();
	void _on_dialog_entry_selected(int p_id, const String &p_name);

protected:
	static void _bind_methods();

public:
	void set_variable_mode(bool p_var_mode);
	bool is_variable_mode() const { return variable_mode; }

	void set_selected_id(int p_id);
	int get_selected_id() const { return selected_id; }

	void set_selected_name(const String &p_name);
	String get_selected_name() const { return selected_name; }

	ClockGearsSwitchVarPickerButton(bool p_variable_mode = false);
};

// ============================================================================
// 4. ClockGearsRecordPickerButton: Selector de registros existentes de la DB
// ============================================================================
class ClockGearsRecordPickerButton : public OptionButton {
	GDCLASS(ClockGearsRecordPickerButton, OptionButton);

private:
	int target_category = CAT_ITEMS;
	int selected_record_id = 0;
	bool updating_items = false;

	void _on_item_idx_selected(int p_idx);
	void _on_data_manager_changed(const String &p_category_key, int p_id);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_target_category(int p_category);
	int get_target_category() const { return target_category; }

	void refresh_options();
	void set_selected_record_id(int p_id);
	int get_selected_record_id() const { return selected_record_id; }

	ClockGearsRecordPickerButton(int p_category = CAT_ITEMS);
};
