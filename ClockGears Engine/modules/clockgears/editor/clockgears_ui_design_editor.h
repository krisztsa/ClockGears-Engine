#ifndef CLOCKGEARS_UI_DESIGN_EDITOR_H
#define CLOCKGEARS_UI_DESIGN_EDITOR_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"

class ClockGearsCreditsPlayer;
class ClockGearsDatabaseDialog;
class ClockGearsGameMenu;
class ClockGearsHudOverlay;
class ClockGearsTitleScreen;
class ClockGearsUiDesignEditor;
class ClockGearsUiDesignLayer;
class SubViewport;
class SubViewportContainer;

// ============================================================================
// UiDesignPaletteButton — Elemento arrastrable de la paleta al lienzo 960x540
// ============================================================================
class ClockGearsUiPaletteButton : public Button {
	GDCLASS(ClockGearsUiPaletteButton, Button);

	String element_type;
	ClockGearsUiDesignEditor *owner_editor = nullptr;

protected:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void setup(ClockGearsUiDesignEditor *p_owner, const String &p_type, const String &p_label);
	String get_element_type() const { return element_type; }
	virtual Variant get_drag_data(const Point2 &p_at_position) override;
};

// ============================================================================
// UiDesignCanvasOverlay — Selección, arrastre (mover), tiradores de tamaño
// (esquinas y lados), recepción drag & drop desde paleta y bloqueo de zoom
// con rueda del ratón.
// ============================================================================
class ClockGearsUiCanvasOverlay : public Control {
	GDCLASS(ClockGearsUiCanvasOverlay, Control);

public:
	enum HandleType {
		HANDLE_NONE = -1,
		HANDLE_MOVE = 0,
		HANDLE_TL = 1,
		HANDLE_T = 2,
		HANDLE_TR = 3,
		HANDLE_L = 4,
		HANDLE_R = 5,
		HANDLE_BL = 6,
		HANDLE_B = 7,
		HANDLE_BR = 8
	};

private:
	ClockGearsUiDesignEditor *owner_editor = nullptr;
	bool dragging = false;
	HandleType active_handle = HANDLE_NONE;
	String drag_elem_id;
	Vector2 drag_start_pos;
	Dictionary drag_initial_elem;
	Dictionary drag_initial_screen_snapshot;
	bool drag_changed = false;
	String hover_elem_id;
	HandleType hover_handle = HANDLE_NONE;

	Rect2 _get_handle_rect(const Rect2 &p_elem_rect, HandleType p_handle) const;
	HandleType _hit_test_handle(const Rect2 &p_elem_rect, const Vector2 &p_pt) const;
	String _hit_test_element(const Vector2 &p_pt) const;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_editor(ClockGearsUiDesignEditor *p_owner) { owner_editor = p_owner; }
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
	Rect2 get_selected_widget_rect() const;
	Rect2 get_handle_widget_rect(int p_handle) const;
	bool is_dragging_canvas() const { return dragging; }
};

// ============================================================================
// ClockGearsUiDesignEditor — Categoría "Diseño de interfaz" (9B Tanda 6B-1)
// Reutiliza exclusivamente ClockGearsDataManager (Tanda 6A) y ClockGearsUiDesignLayer.
// ============================================================================
class ClockGearsUiDesignEditor : public VBoxContainer {
	GDCLASS(ClockGearsUiDesignEditor, VBoxContainer);

public:
	static constexpr int BASE_W = 960;
	static constexpr int BASE_H = 540;

private:
	ClockGearsDatabaseDialog *owner_db = nullptr;
	String current_screen_id = "title";
	String selected_elem_id;
	int canvas_zoom = 1;
	bool updating_ui = false;
	bool compact_two_row_layout = false;
	int last_adapt_width = 1920;

	// Cabecera / barra superior
	HBoxContainer *top_bar = nullptr;
	OptionButton *screen_opt = nullptr;
	Button *zoom_btns[3] = { nullptr, nullptr, nullptr };
	Label *zoom_info_lbl = nullptr;
	Button *undo_btn = nullptr;
	Button *redo_btn = nullptr;
	Button *reset_btn = nullptr;
	Button *test_btn = nullptr;
	Label *screen_status_lbl = nullptr;

	// Contenedores responsivos (3 columnas >= 1460px; 2 filas < 1460px sin scroll H)
	VBoxContainer *layout_root = nullptr;
	HBoxContainer *wide_hbox = nullptr;
	VBoxContainer *wide_left_col = nullptr;
	VBoxContainer *wide_center_col = nullptr;
	VBoxContainer *wide_right_col = nullptr;
	VBoxContainer *compact_vbox = nullptr;
	HBoxContainer *compact_top_hbox = nullptr;
	HBoxContainer *compact_bottom_hbox = nullptr;

	// Tarjetas principales
	PanelContainer *palette_card = nullptr;
	PanelContainer *canvas_card = nullptr;
	PanelContainer *layers_card = nullptr;
	PanelContainer *props_card = nullptr;

	// Paleta lateral (9 tipos)
	GridContainer *palette_grid = nullptr;
	Vector<ClockGearsUiPaletteButton *> palette_buttons;

	// Lienzo 960x540 con ClockGearsUiDesignLayer
	ScrollContainer *canvas_scroll = nullptr;
	Control *canvas_stage = nullptr;
	ColorRect *canvas_bg_color = nullptr;
	TextureRect *canvas_bg_tex = nullptr;
	Control *canvas_hud_grid_bg = nullptr;
	ClockGearsTitleScreen *default_title_preview = nullptr;
	SubViewportContainer *default_gmenu_svc = nullptr;
	SubViewport *default_gmenu_vp = nullptr;
	ClockGearsGameMenu *default_gmenu_preview = nullptr;
	ClockGearsCreditsPlayer *default_credits_preview = nullptr;
	ClockGearsUiDesignLayer *design_layer = nullptr;
	ClockGearsUiCanvasOverlay *canvas_overlay = nullptr;

	// Lista de capas con ojo de visibilidad y reordenado
	VBoxContainer *layers_list_vbox = nullptr;
	Button *layer_up_btn = nullptr;
	Button *layer_down_btn = nullptr;
	Button *layer_del_btn = nullptr;

	// Panel de propiedades
	Label *props_sel_lbl = nullptr;
	VBoxContainer *props_fields_box = nullptr;
	LineEdit *prop_name_edit = nullptr;
	Button *anchor_btns[9] = { nullptr };
	SpinBox *prop_ox_spin = nullptr;
	SpinBox *prop_oy_spin = nullptr;
	SpinBox *prop_w_spin = nullptr;
	SpinBox *prop_h_spin = nullptr;
	SpinBox *prop_layer_spin = nullptr;
	CheckBox *prop_visible_chk = nullptr;
	OptionButton *prop_style_opt = nullptr;

	// Texto / Imagen / Enlace / Acción botón
	VBoxContainer *prop_text_row = nullptr;
	LineEdit *prop_text_edit = nullptr;
	VBoxContainer *prop_image_row = nullptr;
	LineEdit *prop_image_edit = nullptr;
	Button *prop_image_import_btn = nullptr;
	Button *prop_image_clear_btn = nullptr;
	VBoxContainer *prop_binding_row = nullptr;
	OptionButton *prop_binding_opt = nullptr;
	HBoxContainer *prop_var_row = nullptr;
	OptionButton *prop_var_opt = nullptr;
	LineEdit *prop_var_edit = nullptr;
	HBoxContainer *prop_item_row = nullptr;
	OptionButton *prop_item_opt = nullptr;
	VBoxContainer *prop_action_row = nullptr;
	OptionButton *prop_action_opt = nullptr;
	SpinBox *prop_focus_spin = nullptr;

	// Diálogos auxiliares
	FileDialog *import_image_dialog = nullptr;
	ConfirmationDialog *reset_confirm_dialog = nullptr;
	Window *test_window = nullptr;
	ClockGearsTitleScreen *test_title = nullptr;
	ClockGearsGameMenu *test_gmenu = nullptr;
	ClockGearsCreditsPlayer *test_credits = nullptr;
	ClockGearsHudOverlay *test_hud = nullptr;

	void _build_ui();
	void _rebalance_responsive_layout(int p_avail_w);
	void _refresh_canvas_stage();
	void _refresh_background_and_default_preview(bool p_has_custom);
	void _refresh_layers_list();
	void _refresh_properties_panel();
	void _refresh_style_binding_action_options();
	void _refresh_undo_redo_buttons();
	void _notify_data_mutated(bool p_mark_dirty = true);

	Dictionary _get_selected_element_copy() const;
	String _get_selected_anchor_key() const;
	static String _anchor_key_by_index(int p_idx);
	static int _anchor_index_by_key(const String &p_key);

	void _on_screen_selected(int p_idx);
	void _on_zoom_pressed(int p_zoom);
	void _on_undo_pressed();
	void _on_redo_pressed();
	void _on_reset_pressed();
	void _on_reset_confirmed();
	void _on_test_pressed();
	void _on_test_window_close_requested();
	void _on_test_window_input(const Ref<InputEvent> &p_event);
	void _on_layer_eye_toggled(const String &p_elem_id);
	void _on_layer_row_pressed(const String &p_elem_id);
	void _on_layer_move_pressed(int p_delta);
	void _on_layer_delete_pressed();
	void _on_anchor_btn_pressed(int p_idx);
	void _on_prop_field_changed();
	void _on_prop_binding_changed(int p_idx);
	void _on_prop_var_opt_selected(int p_idx);
	void _on_prop_item_opt_selected(int p_idx);
	void _on_prop_action_changed(int p_idx);
	void _on_prop_import_image_pressed();
	void _on_prop_clear_image_pressed();
	void _on_image_file_selected(const String &p_path);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_owner_database(ClockGearsDatabaseDialog *p_db) { owner_db = p_db; }
	void refresh_from_data_manager();
	void adapt_to_width(int p_window_width);

	// API pública clara para interacción directa y pruebas automatizadas
	void select_screen(const String &p_screen_id);
	String get_current_screen() const { return current_screen_id; }

	void set_canvas_zoom(int p_zoom);
	int get_canvas_zoom() const { return canvas_zoom; }

	String add_element_at(const String &p_type, const Vector2i &p_canvas_pos = Vector2i(-1, -1));
	bool select_element(const String &p_elem_id);
	String get_selected_element_id() const { return selected_elem_id; }
	bool delete_selected_element();
	bool toggle_element_visibility(const String &p_elem_id);
	bool move_selected_layer(int p_delta);

	bool apply_undo();
	bool apply_redo();
	void request_reset_to_default(bool p_confirmed_directly = false);
	Window *open_test_popup();
	void close_test_popup();

	ClockGearsUiDesignLayer *get_canvas_design_layer() const { return design_layer; }
	Control *get_canvas_stage() const { return canvas_stage; }
	ClockGearsUiCanvasOverlay *get_canvas_overlay() const { return canvas_overlay; }
	Dictionary get_ui_handles() const;
	Dictionary get_test_popup_info() const;

	ClockGearsUiDesignEditor();
	virtual ~ClockGearsUiDesignEditor();
};

#endif // CLOCKGEARS_UI_DESIGN_EDITOR_H
