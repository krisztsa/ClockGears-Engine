#ifndef CLOCKGEARS_UI_DESIGN_EDITOR_H
#define CLOCKGEARS_UI_DESIGN_EDITOR_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"

class AspectRatioContainer;
class ClockGearsCreditsPlayer;
class ClockGearsDatabaseDialog;
class ClockGearsGameMenu;
class ClockGearsHudOverlay;
class ClockGearsTitleScreen;
class ClockGearsUiDesignEditor;
class ClockGearsUiDesignLayer;
class ClockGearsUiElementFloatingWindow;
class SubViewport;

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
	void _on_pressed();
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
		HANDLE_BR = 8,
		HANDLE_RADIUS = 9,
		HANDLE_RADIAL_RADIUS = 10,
		HANDLE_RADIAL_ANGLE = 11
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
	bool show_grid_overlay = false;
	bool snap_to_grid = false;
	int grid_step = 16;
	bool show_smart_guides = true;
	Vector<float> guide_lines_x;
	Vector<float> guide_lines_y;
	bool marquee_selecting = false;
	Vector2 marquee_start;
	Vector2 marquee_end;
	HashMap<String, Dictionary> drag_multi_initial;

	Rect2 _get_handle_rect(const Rect2 &p_elem_rect, HandleType p_handle) const;
	HandleType _hit_test_handle(const Rect2 &p_elem_rect, const Vector2 &p_pt) const;
	bool _get_radial_overlay_geometry(Vector2 &r_center, float &r_radius, float &r_start_rad, float p_scale) const;
	HandleType _hit_test_radial_handle(const Vector2 &p_pt) const;
	String _hit_test_element(const Vector2 &p_pt) const;
	HashMap<String, Rect2> _compute_all_overlay_rects(const Array &p_elems) const;
	Vector2 _apply_snap_and_guides(const Rect2 &p_base_rect, const String &p_ignore_id, Vector2 &r_delta);

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_editor(ClockGearsUiDesignEditor *p_owner) { owner_editor = p_owner; }
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
	Rect2 get_element_widget_rect(const String &p_elem_id) const;
	Rect2 get_selected_widget_rect() const;
	Rect2 get_handle_widget_rect(int p_handle) const;
	bool is_dragging_canvas() const { return dragging; }
	void set_grid_overlay_enabled(bool p_en);
	bool is_grid_overlay_enabled() const { return show_grid_overlay; }
	void set_snap_enabled(bool p_en);
	bool is_snap_enabled() const { return snap_to_grid; }
	void set_grid_step(int p_step) { grid_step = CLAMP(p_step, 4, 64); queue_redraw(); }
	int get_grid_step() const { return grid_step; }
	void set_smart_guides_enabled(bool p_en) {
		show_smart_guides = p_en;
		if (!show_smart_guides) {
			guide_lines_x.clear();
			guide_lines_y.clear();
		}
		queue_redraw();
	}
	bool is_smart_guides_enabled() const { return show_smart_guides; }
	bool is_marquee_selecting() const { return marquee_selecting; }
	int get_active_guide_line_count() const { return guide_lines_x.size() + guide_lines_y.size(); }
};

// Lista de capas de dibujo propio para listas largas (> 32 capas, 0 nodos extra por fila) (5C)
class ClockGearsLayersCanvas : public Control {
	GDCLASS(ClockGearsLayersCanvas, Control);

public:
	struct Row {
		String elem_id;
		String label;
		bool visible = true;
		bool selected = false;
	};

private:
	ClockGearsUiDesignEditor *owner_editor = nullptr;
	Vector<Row> rows;
	float row_height = 28.0f;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_editor(ClockGearsUiDesignEditor *p_owner) { owner_editor = p_owner; }
	void set_rows(const Vector<Row> &p_rows, float p_row_height);
	int get_row_count() const { return rows.size(); }
};

class ClockGearsDesignChildArrow;
class ClockGearsDesignCardsContainer;
class ClockGearsDesignCardRow;

class ClockGearsDesignChildArrow : public Control {
	GDCLASS(ClockGearsDesignChildArrow, Control);

protected:
	void _notification(int p_what);
};

class ClockGearsDesignCardRow : public PanelContainer {
	GDCLASS(ClockGearsDesignCardRow, PanelContainer);

	ClockGearsUiDesignEditor *owner_editor = nullptr;
	ClockGearsDesignCardsContainer *cards_container = nullptr;
	String elem_id;
	String parent_id;
	String elem_type;
	int depth = 0;
	bool has_children = false;
	bool collapsed = false;
	bool elem_visible = true;
	bool elem_locked = false;
	bool elem_selected = false;
	bool hovered = false;
	int drop_section_preview = -999; // -1=above, 0=child, 1=below, -999=none

	HBoxContainer *row_hb = nullptr;
	Button *collapse_btn = nullptr;
	TextureRect *type_icon_rect = nullptr;
	Label *name_lbl = nullptr;
	LineEdit *rename_edit = nullptr;
	Button *eye_btn = nullptr;
	Button *lock_btn = nullptr;

	Ref<StyleBoxFlat> sb_normal;
	Ref<StyleBoxFlat> sb_hover;
	Ref<StyleBoxFlat> sb_selected;
	Ref<StyleBoxFlat> sb_drop_child;

	void _on_mouse_entered();
	void _on_mouse_exited();
	void _on_collapse_pressed();
	void _on_eye_pressed();
	void _on_lock_pressed();
	void _on_rename_submitted(const String &p_text);
	void _on_rename_focus_exited();

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void setup(ClockGearsUiDesignEditor *p_owner, ClockGearsDesignCardsContainer *p_container,
			const Dictionary &p_elem, int p_depth, bool p_has_children,
			const Ref<Texture2D> &p_type_icon, const Ref<Texture2D> &p_eye_icon, const Ref<Texture2D> &p_lock_icon,
			bool p_selected, const String &p_parent_name);
	void update_visual_state(bool p_selected);
	void set_drop_section_preview(int p_sec);
	int get_drop_section_preview() const { return drop_section_preview; }
	String get_elem_id() const { return elem_id; }
	int get_depth() const { return depth; }
	void begin_inline_rename();
	int compute_drop_section(const Point2 &p_local_pos) const;

	virtual Variant get_drag_data(const Point2 &p_at_position) override;
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
};

class ClockGearsDesignCardsContainer : public VBoxContainer {
	GDCLASS(ClockGearsDesignCardsContainer, VBoxContainer);

	ClockGearsUiDesignEditor *owner_editor = nullptr;
	ClockGearsDesignCardRow *drop_preview_row = nullptr;
	int drop_preview_section = -999;

protected:
	void _notification(int p_what);

public:
	void set_owner_editor(ClockGearsUiDesignEditor *p_owner) { owner_editor = p_owner; }
	void set_drop_preview(ClockGearsDesignCardRow *p_row, int p_sec);
	void clear_drop_preview();
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
};

class ClockGearsLayersTree : public Tree {
	GDCLASS(ClockGearsLayersTree, Tree);

	ClockGearsUiDesignEditor *owner_editor = nullptr;
	String last_click_id;
	uint64_t last_click_msec = 0;

protected:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_editor(ClockGearsUiDesignEditor *p_owner) { owner_editor = p_owner; }
	virtual Variant get_drag_data(const Point2 &p_point) override;
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;
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
	String system_scope_root;
	String current_screen_id = "title";
	String selected_elem_id;
	PackedStringArray selected_elem_ids;
	int canvas_zoom = 1;
	int canvas_zoom_mode = 0; // 0=Ajustar, 1=100%, 2=200%, 3=50%, 4=300%
	Vector2i preview_resolution = Vector2i(960, 540);
	bool updating_ui = false;
	bool updating_props_panel = false;
	struct ScreenZoomState {
		int zoom_mode = 0;
		Vector2i scroll_pos;
	};
	HashMap<String, ScreenZoomState> per_screen_zoom_state;
	Size2 last_canvas_viewport_sz = Size2(-1, -1);
	bool compact_two_row_layout = false;
	int last_adapt_width = 1920;

	// Cabecera / barra superior
	HBoxContainer *top_bar = nullptr;
	OptionButton *screen_opt = nullptr;
	OptionButton *preview_res_opt = nullptr;
	Button *zoom_btns[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	Label *zoom_info_lbl = nullptr;
	Button *undo_btn = nullptr;
	Button *redo_btn = nullptr;
	Button *reset_btn = nullptr;
	Button *test_btn = nullptr;
	Button *new_menu_wizard_btn = nullptr;
	Button *add_submenu_btn = nullptr;
	Button *add_button_bar_btn = nullptr;
	Button *templates_btn = nullptr;
	Label *screen_status_lbl = nullptr;

	// Barra de edición rápida (rejilla, paso, imán, guías, alinear, distribuir, duplicar, copiar/pegar)
	CheckBox *grid_chk = nullptr;
	OptionButton *grid_step_opt = nullptr;
	CheckBox *snap_chk = nullptr;
	CheckBox *guides_chk = nullptr;
	Button *align_center_h_btn = nullptr;
	Button *align_center_v_btn = nullptr;
	Button *dist_v_btn = nullptr;
	Button *dup_elem_btn = nullptr;
	Button *copy_elem_btn = nullptr;
	Button *paste_elem_btn = nullptr;

	// Contenedores responsivos (3 columnas >= 780px con divisores arrastrables; 1 col < 780px sin scroll H)
	VBoxContainer *layout_root = nullptr;
	HSplitContainer *wide_hbox = nullptr;
	HSplitContainer *wide_right_split = nullptr;
	VBoxContainer *wide_left_col = nullptr;
	VBoxContainer *wide_center_col = nullptr;
	VBoxContainer *wide_right_col = nullptr;
	ScrollContainer *compact_scroll = nullptr;
	VBoxContainer *compact_vbox = nullptr;
	HBoxContainer *compact_top_hbox = nullptr;
	HBoxContainer *compact_bottom_hbox = nullptr;

	// Tarjetas principales
	PanelContainer *menus_card = nullptr;
	PanelContainer *palette_card = nullptr;
	PanelContainer *canvas_card = nullptr;
	PanelContainer *layers_card = nullptr;
	PanelContainer *props_card = nullptr;

	// Panel "Pantallas" (4A / Consolidación 9B)
	ScrollContainer *left_col_scroll = nullptr;
	Tree *screens_tree = nullptr;
	VBoxContainer *menus_list_vbox = nullptr;
	Button *menu_dup_btn = nullptr;
	Button *menu_up_btn = nullptr;
	Button *menu_down_btn = nullptr;
	Button *menu_del_btn = nullptr;
	Button *menu_restore_btn = nullptr;

	// Paleta lateral (9 tipos)
	GridContainer *palette_grid = nullptr;
	Vector<ClockGearsUiPaletteButton *> palette_buttons;

	// Lienzo 16:9 escalable con ClockGearsUiDesignLayer
	ScrollContainer *canvas_scroll = nullptr;
	AspectRatioContainer *canvas_aspect = nullptr;
	Control *canvas_fit_holder = nullptr;
	Control *canvas_stage = nullptr;
	ColorRect *canvas_bg_color = nullptr;
	TextureRect *canvas_bg_tex = nullptr;
	Control *canvas_hud_grid_bg = nullptr;
	ClockGearsTitleScreen *default_title_preview = nullptr;
	SubViewport *default_gmenu_vp = nullptr;
	TextureRect *default_gmenu_disp = nullptr;
	ClockGearsGameMenu *default_gmenu_preview = nullptr;
	ClockGearsCreditsPlayer *default_credits_preview = nullptr;
	ClockGearsUiDesignLayer *design_layer = nullptr;
	ClockGearsUiCanvasOverlay *canvas_overlay = nullptr;
	LineEdit *inline_text_edit = nullptr;
	String inline_edit_elem_id;
	PopupMenu *canvas_context_menu = nullptr;
	Vector2 canvas_context_pos;

	// Lista de tarjetas de elementos dentro de "Diseños" (reemplaza árbol Capas)
	ScrollContainer *design_cards_scroll = nullptr;
	ClockGearsDesignCardsContainer *design_cards_vbox = nullptr;
	HashMap<String, ClockGearsDesignCardRow *> design_card_rows;
	Vector<String> design_cards_order;
	ClockGearsLayersTree *layers_tree = nullptr;
	HashMap<String, TreeItem *> layer_tree_items;
	PopupMenu *layer_context_menu = nullptr;
	PopupMenu *add_child_submenu = nullptr;
	PopupMenu *reparent_submenu = nullptr;
	Vector<String> reparent_candidate_ids;
	VBoxContainer *layers_list_vbox = nullptr;
	ClockGearsLayersCanvas *layers_long_canvas = nullptr;
	Button *layer_add_folder_btn = nullptr;
	Button *layer_group_btn = nullptr;
	Button *layer_ungroup_btn = nullptr;
	Button *layer_up_btn = nullptr;
	Button *layer_down_btn = nullptr;
	Button *layer_del_btn = nullptr;

	// Panel de propiedades (4C: Simple/Avanzado, Buscador, Secciones plegables, Sliders, Selectores)
	bool props_advanced_mode = false;
	String props_search_query;
	String current_selected_elem_type;
	Button *props_mode_simple_btn = nullptr;
	Button *props_mode_adv_btn = nullptr;
	LineEdit *props_search_edit = nullptr;
	Label *props_sel_lbl = nullptr;
	VBoxContainer *props_fields_box = nullptr;
	struct PropRowFilterEntry {
		Control *row = nullptr;
		String search_text;
		bool advanced_only = false;
		String elem_types; // CSV de tipos aplicables; vacío = todos
	};
	struct PropSectionGroup {
		Button *header = nullptr;
		VBoxContainer *body = nullptr;
		bool advanced_only = false;
	};
	Vector<PropRowFilterEntry> prop_filter_entries;
	Vector<PropSectionGroup> prop_sections;
	OptionButton *prop_anim_opt = nullptr;
	LineEdit *prop_name_edit = nullptr;
	Button *anchor_btns[9] = { nullptr };
	SpinBox *prop_ox_spin = nullptr;
	SpinBox *prop_oy_spin = nullptr;
	SpinBox *prop_w_spin = nullptr;
	SpinBox *prop_h_spin = nullptr;
	SpinBox *prop_layer_spin = nullptr;
	CheckBox *prop_visible_chk = nullptr;
	CheckBox *prop_locked_chk = nullptr;
	CheckBox *prop_clip_children_chk = nullptr;
	OptionButton *prop_parent_layout_opt = nullptr;
	SpinBox *prop_parent_cols_spin = nullptr;
	SpinBox *prop_parent_space_spin = nullptr;
	CheckBox *prop_show_bg_chk = nullptr;
	OptionButton *prop_style_opt = nullptr;
	Button *prop_edit_style_btn = nullptr;
	AcceptDialog *style_inline_dialog = nullptr;
	String style_editing_id;
	LineEdit *style_edit_name = nullptr;
	ColorPickerButton *style_edit_fill_col = nullptr;
	ColorPickerButton *style_edit_border_col = nullptr;
	SpinBox *style_edit_border_w = nullptr;
	SpinBox *style_edit_radius = nullptr;
	SpinBox *style_edit_pad = nullptr;
	HSlider *style_edit_opacity = nullptr;
	HashMap<String, Ref<Texture2D>> style_thumb_cache;

	// Forma, relleno, bordes, sombra, brillo y fuente (3F / 4C)
	OptionButton *prop_shape_opt = nullptr;
	HSlider *prop_radius_slider = nullptr;
	Label *prop_radius_val_lbl = nullptr;
	HSlider *prop_opacity_slider = nullptr;
	Label *prop_opacity_val_lbl = nullptr;
	ColorPickerButton *prop_fill_color_btn = nullptr;
	HBoxContainer *prop_recent_colors_box = nullptr;
	CheckBox *prop_grad_chk = nullptr;
	ColorPickerButton *prop_grad_top_btn = nullptr;
	ColorPickerButton *prop_grad_bot_btn = nullptr;
	HSlider *prop_border_w_slider = nullptr;
	ColorPickerButton *prop_border_col_btn = nullptr;
	CheckBox *prop_shadow_chk = nullptr;
	CheckBox *prop_glow_chk = nullptr;
	ColorPickerButton *prop_glow_col_btn = nullptr;
	CheckBox *prop_clip_chk = nullptr;
	CheckBox *prop_pixel_art_chk = nullptr;
	OptionButton *prop_font_opt = nullptr;
	Button *prop_font_import_btn = nullptr;
	HSlider *prop_font_size_slider = nullptr;
	ColorPickerButton *prop_font_color_btn = nullptr;

	// Propiedades globales del menú (4F fondo/animación, 3B entradas/contexto, 3E radial)
	VBoxContainer *menu_props_box = nullptr;
	OptionButton *menu_bg_type_opt = nullptr;
	ColorPickerButton *menu_bg_col_btn = nullptr;
	ColorPickerButton *menu_bg_grad_bot_btn = nullptr;
	LineEdit *menu_bg_img_edit = nullptr;
	Button *menu_bg_img_btn = nullptr;
	OptionButton *menu_bg_scale_opt = nullptr;
	CheckBox *menu_bg_pixel_chk = nullptr;
	SpinBox *menu_bg_scroll_x_spin = nullptr;
	SpinBox *menu_bg_scroll_y_spin = nullptr;
	CheckBox *menu_bg_blur_chk = nullptr;
	HSlider *menu_bg_blur_slider = nullptr;
	CheckBox *menu_bg_veil_chk = nullptr;
	ColorPickerButton *menu_bg_veil_col_btn = nullptr;
	HSlider *menu_bg_veil_opacity_slider = nullptr;
	OptionButton *menu_trans_in_opt = nullptr;
	OptionButton *menu_trans_out_opt = nullptr;
	HSlider *menu_trans_dur_slider = nullptr;
	OptionButton *menu_trans_curve_opt = nullptr;
	OptionButton *menu_ctx_opt = nullptr;
	CheckBox *menu_pause_chk = nullptr;
	Button *menu_capture_key_btn = nullptr;
	Label *menu_triggers_lbl = nullptr;
	Label *menu_conflict_lbl = nullptr;
	OptionButton *menu_layout_mode_opt = nullptr;
	HSlider *menu_radial_radius_slider = nullptr;
	HSlider *menu_radial_start_slider = nullptr;
	HSlider *menu_radial_sweep_slider = nullptr;
	CheckBox *menu_radial_cw_chk = nullptr;
	Button *menu_radial_add_slot_btn = nullptr;
	LineEdit *menu_snd_open_edit = nullptr;
	LineEdit *menu_snd_close_edit = nullptr;
	LineEdit *menu_snd_move_edit = nullptr;
	LineEdit *menu_snd_accept_edit = nullptr;
	LineEdit *menu_snd_cancel_edit = nullptr;
	LineEdit *menu_bgm_edit = nullptr;

	// Sección "Ajustes de la pantalla" consolidada en Propiedades (9B Corr Punto 5)
	VBoxContainer *screen_settings_box = nullptr;
	VBoxContainer *screen_settings_body = nullptr;
	FileDialog *screen_asset_dialog = nullptr;
	String screen_asset_target;
	int ss_selected_cred_idx = -1;
	String cached_screens_tree_sig;
	OptionButton *ss_hud_add_type_opt = nullptr;
	SpinBox *ss_hud_sample_hp_spin = nullptr;
	SpinBox *ss_hud_sample_gold_spin = nullptr;
	SpinBox *ss_hud_sample_qty_spin = nullptr;
	OptionButton *ss_cred_add_type_opt = nullptr;
	LineEdit *ss_cred_item_text_edit = nullptr;
	LineEdit *ss_cred_item_role_edit = nullptr;
	LineEdit *ss_cred_item_name_edit = nullptr;
	LineEdit *ss_cred_item_img_edit = nullptr;
	SpinBox *ss_cred_item_spacer_spin = nullptr;
	OptionButton *ss_cred_item_align_opt = nullptr;

	// Cachés de rendimiento (5C)
	String cached_opts_screen_id;
	int cached_opts_styles_cnt = -1;
	int cached_opts_vars_cnt = -1;
	int cached_opts_items_cnt = -1;
	bool cached_fonts_scanned = false;
	bool cached_thumbs_scanned = false;
	Ref<StyleBoxFlat> cached_action_sb_normal;
	Ref<StyleBoxFlat> cached_action_sb_hover;
	Ref<StyleBoxFlat> cached_action_sb_pressed;

	// Condiciones por interruptor y constructor multi-acción (3D / 4C)
	OptionButton *prop_vis_mode_opt = nullptr;
	LineEdit *prop_vis_sw_edit = nullptr;
	OptionButton *prop_en_mode_opt = nullptr;
	LineEdit *prop_en_sw_edit = nullptr;
	VBoxContainer *prop_multi_actions_vbox = nullptr;
	OptionButton *prop_add_action_opt = nullptr;
	Button *prop_add_action_btn = nullptr;

	// Miniaturas rápidas de imagen
	HFlowContainer *prop_img_thumbs_flow = nullptr;
	HashMap<String, Ref<Texture2D>> thumb_tex_cache;

	// Texto / Imagen / Enlace / Acción botón
	VBoxContainer *prop_text_row = nullptr;
	LineEdit *prop_text_edit = nullptr;
	VBoxContainer *prop_image_row = nullptr;
	LineEdit *prop_image_edit = nullptr;
	Button *prop_image_import_btn = nullptr;
	Button *prop_image_clear_btn = nullptr;
	OptionButton *prop_image_scale_opt = nullptr;
	SpinBox *prop_nine_slice_spin = nullptr;
	VBoxContainer *prop_btn_state_imgs_box = nullptr;
	LineEdit *prop_btn_state_img_edits[4] = { nullptr, nullptr, nullptr, nullptr };
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
	LineEdit *prop_sound_hover_edit = nullptr;
	LineEdit *prop_sound_click_edit = nullptr;
	Label *prop_btn_shortcut_lbl = nullptr;
	Button *prop_btn_shortcut_capture_btn = nullptr;
	Button *prop_btn_shortcut_clear_btn = nullptr;

	// Inspector completo de Rueda radial (Punto 7)
	VBoxContainer *prop_radial_box = nullptr;
	HSlider *prop_rad_count_slider = nullptr;
	Label *prop_rad_count_lbl = nullptr;
	HSlider *prop_rad_radius_slider = nullptr;
	HSlider *prop_rad_start_slider = nullptr;
	CheckBox *prop_rad_full_chk = nullptr;
	HSlider *prop_rad_sweep_slider = nullptr;
	CheckBox *prop_rad_cw_chk = nullptr;
	HSlider *prop_rad_slot_size_slider = nullptr;
	CheckBox *prop_rad_center_info_chk = nullptr;
	OptionButton *prop_rad_open_anim_opt = nullptr;
	OptionButton *prop_rad_slot_opt = nullptr;
	int prop_rad_selected_slot = 0;
	Button *prop_rad_add_slot_btn = nullptr;
	Button *prop_rad_del_slot_btn = nullptr;
	LineEdit *prop_rad_slot_name_edit = nullptr;
	LineEdit *prop_rad_slot_desc_edit = nullptr;
	LineEdit *prop_rad_slot_icon_edit = nullptr;
	Button *prop_rad_slot_icon_btn = nullptr;
	OptionButton *prop_rad_slot_add_act_opt = nullptr;
	Button *prop_rad_slot_add_act_btn = nullptr;
	VBoxContainer *prop_rad_slot_actions_vbox = nullptr;

	// Diálogos auxiliares, Asistente 3 pasos (4A) y Galería de Plantillas (4B)
	FileDialog *import_image_dialog = nullptr;
	int image_import_target = 0; // 0=elemento, 1=fondo menu, 2=fuente TTF, 10..13=estados boton
	FileDialog *import_font_dialog = nullptr;
	FileDialog *export_pkg_dialog = nullptr;
	FileDialog *import_pkg_dialog = nullptr;
	ConfirmationDialog *reset_confirm_dialog = nullptr;
	ConfirmationDialog *delete_menu_confirm_dialog = nullptr;
	ConfirmationDialog *delete_parent_dialog = nullptr;
	PackedStringArray pending_delete_parent_ids;
	ConfirmationDialog *template_apply_confirm_dialog = nullptr;
	ConfirmationDialog *save_tpl_name_dialog = nullptr;
	LineEdit *save_tpl_name_edit = nullptr;
	bool save_tpl_is_component = false;
	String pending_delete_menu_id;
	String pending_apply_template_id;
	bool capturing_key_for_screen = false;
	bool capturing_key_for_wizard = false;
	bool capturing_key_for_btn_shortcut = false;

	AcceptDialog *menu_wizard_dialog = nullptr;
	int wizard_step = 0;
	String wizard_selected_template_id = "classic_jrpg";
	VBoxContainer *wizard_step_panels[3] = { nullptr, nullptr, nullptr };
	Label *wizard_step_lbl = nullptr;
	Button *wizard_prev_btn = nullptr;
	Button *wizard_next_btn = nullptr;
	Button *wizard_finish_btn = nullptr;
	GridContainer *wizard_tpl_grid = nullptr;
	LineEdit *wizard_name_edit = nullptr;
	Vector<CheckBox *> wizard_btn_checks;
	OptionButton *wizard_ctx_opt = nullptr;
	CheckBox *wizard_pause_chk = nullptr;
	Button *wizard_capture_key_btn = nullptr;
	Label *wizard_triggers_lbl = nullptr;
	Label *wizard_conflict_lbl = nullptr;
	Array wizard_triggers;

	AcceptDialog *template_gallery_dialog = nullptr;
	TabContainer *template_tabs = nullptr;
	GridContainer *tpl_screen_grid = nullptr;
	GridContainer *tpl_comp_grid = nullptr;
	GridContainer *tpl_user_grid = nullptr;
	CheckBox *tpl_preserve_btns_chk = nullptr;
	String selected_gallery_template_id;
	Label *tpl_selected_info_lbl = nullptr;
	Button *tpl_apply_btn = nullptr;
	Button *tpl_export_btn = nullptr;
	Button *tpl_delete_user_btn = nullptr;

	Window *test_window = nullptr;
	ClockGearsTitleScreen *test_title = nullptr;
	ClockGearsGameMenu *test_gmenu = nullptr;
	ClockGearsCreditsPlayer *test_credits = nullptr;
	ClockGearsHudOverlay *test_hud = nullptr;
	bool test_injected_sample_state = false;
	Dictionary test_saved_runtime_state;

	// Ventanas flotantes no modales por elemento (9B Capas P4)
	HashMap<String, ClockGearsUiElementFloatingWindow *> open_floating_windows;
	Vector2i last_floating_window_pos = Vector2i(-1, -1);
	Size2i last_floating_window_size = Size2i(0, 0);

	void _sync_open_floating_windows();
	void _on_floating_window_closed(const String &p_key, ClockGearsUiElementFloatingWindow *p_win);
	void _on_layers_tree_item_activated();
	void _on_float_window_elem_edited(const String &p_screen_id, const String &p_elem_id, const Dictionary &p_next_elem);

	void _build_ui();
	void _trace_log(const String &p_event, const String &p_detail = "") const;
	static float persisted_left_split_w;
	static float persisted_right_split_w;
	static bool persisted_split_loaded;
	bool updating_splits = false;
	void _load_saved_split_widths();
	void _save_split_widths();
	void _apply_wide_split_widths(float p_avail_w);
	void _on_wide_split_dragged(int p_offset, int p_split_idx);
	void _capture_dragged_split_widths(int p_split_idx);
	void _on_split_dragger_gui_input(const Ref<InputEvent> &p_ev, int p_split_idx);
	void _reset_wide_split(int p_split_idx);
	void _rebalance_responsive_layout(int p_avail_w);
	void _update_canvas_fit_scale();
	void _apply_canvas_zoom_layout(bool p_recalc_fit);
	void _on_canvas_scroll_resized();
	void _on_canvas_fit_holder_resized();
	void _restore_canvas_scroll_pos(const Vector2i &p_pos);
	Button *_attach_collapsible_header(VBoxContainer *p_card_vb, const String &p_title, Control *p_body, bool p_expanded = true);
	void _on_collapsible_section_toggled(bool p_expanded, Control *p_body, Button *p_btn, const String &p_title);
	void _on_preview_res_selected(int p_idx);
	void _refresh_canvas_stage();
	void _refresh_background_and_default_preview(bool p_has_custom);
	void _refresh_menus_list();
	void _refresh_screen_dropdown();
	void _refresh_layers_list();
	void _refresh_properties_panel();
	void _refresh_screen_settings_panel();
	void _refresh_menu_global_properties();
	void _on_screens_tree_selected();
	void _on_ss_asset_pick_pressed(const String &p_target, bool p_is_audio);
	void _on_ss_asset_file_selected(const String &p_path);
	void _on_ss_title_changed();
	void _on_ss_entry_toggled(bool p_enabled, const String &p_entry_id, bool p_is_game_menu);
	void _on_ss_entry_text_changed(const String &p_text, const String &p_entry_id, bool p_is_game_menu);
	void _on_ss_entry_move(const String &p_entry_id, int p_delta, bool p_is_game_menu);
	void _on_ss_gm_item_preview_changed();
	void _on_ss_credits_meta_changed();
	void _on_ss_credits_item_selected(int p_idx);
	void _on_ss_credits_add_pressed();
	void _on_ss_credits_del_pressed();
	void _on_ss_credits_move_pressed(int p_delta);
	void _on_ss_credits_item_field_changed();
	void _on_ss_hud_add_widget_pressed();
	void _on_ss_hud_sample_changed(double p_val = 0.0);
	void _on_ss_gameover_changed();
	void _refresh_multi_actions_ui(const Dictionary &p_elem);
	void _populate_multi_actions_list(VBoxContainer *p_vbox, const Array &p_acts);
	Dictionary _get_active_radial_layout(bool *r_is_elem = nullptr) const;
	void _commit_active_radial_layout(const Dictionary &p_lay, bool p_push_undo = true, bool p_refresh_inspector = true);
	void _refresh_radial_inspector();
	void _on_prop_radial_changed();
	void _on_prop_radial_count_changed(double p_val);
	void _on_prop_radial_slot_selected(int p_idx);
	void _on_prop_radial_slot_edited();
	void _on_prop_radial_del_slot_pressed();
	void _on_prop_radial_slot_icon_import_pressed();
	void _refresh_recent_colors_palette();
	void _refresh_image_thumbs_gallery();
	void _apply_properties_filter();
	void _register_prop_filter_row(Control *p_row, const String &p_keywords, bool p_advanced_only, const String &p_elem_types = String());
	void _refresh_style_binding_action_options();
	void _refresh_undo_redo_buttons();
	void _notify_data_mutated(bool p_mark_dirty = true);
	void _begin_inline_text_edit(const String &p_elem_id);
	void _commit_inline_text_edit();
	void _populate_template_gallery();
	void _open_menu_wizard();
	void _update_wizard_step_ui();
	void _refresh_wizard_conflict_label();
	void _on_wizard_tpl_selected(const String &p_tpl_id);
	void _on_wizard_prev_pressed();
	void _on_wizard_next_pressed();
	void _on_wizard_finish_pressed();
	void _on_wizard_capture_key_pressed();
	void _on_add_submenu_pressed();
	void _on_add_button_bar_pressed();
	void _on_templates_btn_pressed();
	void _on_tpl_card_selected(const String &p_tpl_id);
	void _on_tpl_apply_pressed();
	void _on_tpl_apply_confirmed();
	void _on_tpl_save_current_pressed();
	void _on_tpl_save_name_confirmed();
	void _on_tpl_export_pressed();
	void _on_tpl_import_pressed();
	void _on_tpl_delete_user_pressed();
	void _on_export_pkg_file_selected(const String &p_path);
	void _on_import_pkg_file_selected(const String &p_path);
	void _on_props_mode_changed(bool p_advanced);
	void _on_props_search_changed(const String &p_text);
	void _on_menu_list_select_pressed(const String &p_menu_id);
	void _on_menu_list_dup_pressed();
	void _on_menu_list_move_pressed(int p_delta);
	void _on_menu_list_del_pressed();
	void _on_menu_del_confirmed();
	void _on_menu_restore_pressed();
	void _on_menu_global_changed();
	void _on_menu_capture_key_pressed();
	void _on_menu_radial_add_slot_pressed();
	void _on_menu_bg_import_pressed();
	void _on_prop_font_import_pressed();
	void _on_font_file_selected(const String &p_path);
	void _on_prop_add_multi_action_pressed();
	void _on_prop_remove_multi_action_pressed(int p_idx);
	void _on_prop_move_multi_action_pressed(int p_idx, int p_delta);
	void _on_prop_edit_multi_action_step(const Variant &p_val, int p_idx, const String &p_key);
	void _on_prop_edit_multi_action_menu(int p_opt_idx, ObjectID p_opt_id, int p_idx, const String &p_key);
	void _on_prop_btn_state_import_pressed(int p_state);
	void _on_prop_btn_state_clear_pressed(int p_state);
	void _on_prop_btn_shortcut_capture_pressed();
	void _on_prop_btn_shortcut_clear_pressed();
	void _on_prop_edit_style_pressed();
	void _on_inline_style_applied();
	void _on_window_files_dropped(const Vector<String> &p_files);
	void _on_canvas_context_menu_id(int p_id);
	void _on_align_center_pressed(bool p_vertical);
	void _on_distribute_vertical_pressed();
	void _on_duplicate_elem_pressed();
	void _on_copy_elem_pressed();
	void _on_paste_elem_pressed();
	ClockGearsTitleScreen *_ensure_default_title_preview();
	ClockGearsGameMenu *_ensure_default_gmenu_preview();
	ClockGearsCreditsPlayer *_ensure_default_credits_preview();

	Dictionary _get_selected_element_copy() const;
	String _get_selected_anchor_key() const;
	static String _anchor_key_by_index(int p_idx);
	static int _anchor_index_by_key(const String &p_key);

	void _on_screen_selected(int p_idx);
	void _on_grid_step_selected(int p_idx);
	void _on_zoom_pressed(int p_zoom);
	void _on_undo_pressed();
	void _on_redo_pressed();
	void _on_reset_pressed();
	void _on_reset_confirmed();
	void _on_delete_parent_keep_confirmed();
	void _on_delete_parent_custom_action(const String &p_action);
	void _on_test_pressed();
	void _on_test_window_close_requested();
	void _on_test_window_input(const Ref<InputEvent> &p_event);
	void _on_layer_eye_toggled(const String &p_elem_id);
	void _on_layer_row_pressed(const String &p_elem_id);
	void _on_pooled_layer_eye_toggled(Button *p_btn);
	void _on_pooled_layer_row_pressed(Button *p_btn);
	void _on_pooled_menu_btn_pressed(Button *p_btn);
	void _on_layers_tree_multi_selected(Object *p_item, int p_col, bool p_selected);
	void _sync_selection_from_layers_tree();
	void _on_layers_tree_button_clicked(Object *p_item, int p_column, int p_id, int p_mouse_button_index);
	void _on_layers_tree_item_edited();
	void _on_layers_tree_item_collapsed(Object *p_item);
	void _on_layers_tree_item_mouse_selected(const Vector2 &p_pos, int p_mouse_button_index);
	void _on_layer_context_menu_id(int p_id);
	void _on_add_child_submenu_id(int p_id);
	void _on_reparent_submenu_id(int p_id);
	void _ensure_design_card_visible(const String &p_elem_id);
	void _on_layer_add_folder_pressed();
	void _on_layer_group_pressed();
	void _on_layer_ungroup_pressed();
	void _apply_rounded_btn_style(Button *p_btn);
	void _refresh_theme_icons();
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
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_database(ClockGearsDatabaseDialog *p_db) { owner_db = p_db; }
	void refresh_from_data_manager();
	void refresh_geometry_fast(bool p_refresh_props = true);
	void update_dragged_element_fast(const String &p_elem_id, const Dictionary &p_elem);
	void update_multi_dragged_elements_fast(const Array &p_pairs);
	void set_grid_step(int p_step);
	int get_grid_step() const;
	bool select_elements(const PackedStringArray &p_ids, const String &p_primary_id = "");
	bool toggle_element_in_selection(const String &p_elem_id);
	PackedStringArray get_selected_element_ids() const { return selected_elem_ids; }
	bool is_element_selected(const String &p_elem_id) const;
	Dictionary get_canvas_draw_stats() const;
	void adapt_to_width(int p_window_width);

	// API pública clara para interacción directa y pruebas automatizadas (Etapa 4 + Reconst 9B P6)
	void set_system_scope(const String &p_root_screen_id);
	String get_system_scope() const { return system_scope_root; }
	String add_submenu_to_current(const String &p_name = "");
	String quick_add_button_element(const String &p_text = "");
	void set_canvas_zoom_mode(int p_mode);
	int get_canvas_zoom_mode() const { return canvas_zoom_mode; }
	void forward_canvas_wheel(const Vector2 &p_delta, bool p_horizontal);
	void select_screen(const String &p_screen_id);
	String get_current_screen() const { return current_screen_id; }
	void set_props_advanced_mode(bool p_adv);
	bool is_props_advanced_mode() const { return props_advanced_mode; }
	void set_props_search_filter(const String &p_query);
	int get_visible_prop_row_count() const;
	String create_menu_via_wizard(const Dictionary &p_opts);
	bool apply_template_by_id(const String &p_tpl_id, bool p_preserve_buttons = true, bool p_record_undo = true);
	String save_current_as_user_template(const String &p_name);
	bool begin_inline_edit_for_selected();
	void show_canvas_context_menu(const Vector2 &p_screen_pos);
	bool align_selected_elements(const String &p_mode);
	bool duplicate_selected_element();
	void copy_selected_to_clipboard();
	PackedStringArray paste_from_clipboard();
	void set_radial_layout_live(int p_radius, float p_start_deg);
	bool add_radial_slot_from_editor();

	void set_canvas_zoom(int p_zoom);
	int get_canvas_zoom() const { return canvas_zoom; }
	float get_canvas_stage_scale() const;
	void set_preview_resolution(int p_width, int p_height, bool p_sync_owner_db = true);
	Vector2i get_preview_resolution() const { return preview_resolution; }

	Array get_effective_screen_elements() const;
	Dictionary get_effective_element(const String &p_elem_id) const;
	bool ensure_screen_customizable_for_edit(bool p_record_undo = false);
	void commit_canvas_drag_undo(const Dictionary &p_initial_snapshot);
	void log_trace_action(const String &p_event, const String &p_detail = "") const { _trace_log(p_event, p_detail); }

	String add_element_at(const String &p_type, const Vector2i &p_canvas_pos = Vector2i(-1, -1));
	String drop_image_file_on_canvas(const String &p_path, const Vector2i &p_canvas_pos = Vector2i(-1, -1));
	bool select_element(const String &p_elem_id);
	String get_selected_element_id() const { return selected_elem_id; }
	bool delete_selected_element();
	bool delete_selected_element_with_mode(int p_mode);
	bool toggle_element_visibility(const String &p_elem_id);
	bool toggle_element_locked(const String &p_elem_id);
	bool rename_element_inline(const String &p_elem_id, const String &p_new_name);
	bool begin_selected_tree_rename();
	String create_folder_element(const String &p_name = "", const String &p_parent_id = "");
	String group_selected_into_folder(const String &p_name = "");
	bool ungroup_selected_folder();
	bool reorder_layer_tree(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section);
	bool can_drop_in_layer_tree(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section) const;
	void show_layer_context_menu(const Vector2 &p_screen_pos);
	bool set_element_collapsed(const String &p_elem_id, bool p_collapsed);
	Array get_layer_tree_info() const;
	Array get_design_cards_info() const;
	Control *get_design_card_control(const String &p_elem_id) const;
	String add_child_element_of_type(const String &p_parent_id, const String &p_type);
	bool reparent_element_to(const String &p_elem_id, const String &p_new_parent_id);
	bool unparent_element(const String &p_elem_id);
	bool drop_card_on_target(const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section);
	bool simulate_card_drop_at(const String &p_target_id, const Vector2 &p_local_pos, const PackedStringArray &p_drag_ids);
	bool move_selected_layer(int p_delta);

	bool apply_undo();
	bool apply_redo();
	void request_reset_to_default(bool p_confirmed_directly = false);
	Window *open_test_popup();
	void close_test_popup();

	Window *open_floating_element_window(const String &p_elem_id = "");
	Window *get_floating_window_for_element(const String &p_elem_id) const;
	void close_all_floating_element_windows();
	int get_open_floating_window_count() const;
	void apply_floating_window_element_update(const String &p_screen_id, const String &p_elem_id, const Dictionary &p_next_elem);

	ClockGearsUiDesignLayer *get_canvas_design_layer() const { return design_layer; }
	Control *get_canvas_stage() const { return canvas_stage; }
	ClockGearsUiCanvasOverlay *get_canvas_overlay() const { return canvas_overlay; }
	Rect2 get_overlay_element_rect(const String &p_elem_id) const;
	Rect2 get_overlay_selected_rect() const;
	Rect2 get_overlay_handle_rect(int p_handle) const;
	Rect2 get_rendered_element_stage_rect(const String &p_elem_id) const;
	Dictionary get_ui_handles() const;
	Dictionary get_test_popup_info() const;

	ClockGearsUiDesignEditor();
	virtual ~ClockGearsUiDesignEditor();
};

// ============================================================================
// ClockGearsUiElementFloatingWindow — Ventana flotante no modal por elemento
// 5 pestañas: General, Aspecto, Imagen, Texto, Acciones (sincronizada 2 vías)
// ============================================================================
class ClockGearsUiElementFloatingWindow : public Window {
	GDCLASS(ClockGearsUiElementFloatingWindow, Window);

private:
	ClockGearsUiDesignEditor *owner_editor = nullptr;
	String target_screen_id;
	String target_elem_id;
	Dictionary working_elem;
	bool updating = false;

	PanelContainer *bg_root = nullptr;
	TabContainer *tabs = nullptr;

	// General
	LineEdit *name_edit = nullptr;
	Label *type_lbl = nullptr;
	CheckBox *visible_chk = nullptr;
	CheckBox *locked_chk = nullptr;
	CheckBox *clip_children_chk = nullptr;
	Button *anchor_btns[9] = { nullptr };
	SpinBox *ox_spin = nullptr;
	SpinBox *oy_spin = nullptr;
	SpinBox *w_spin = nullptr;
	SpinBox *h_spin = nullptr;
	SpinBox *layer_spin = nullptr;

	// Aspecto
	CheckBox *show_bg_chk = nullptr;
	OptionButton *style_opt = nullptr;
	OptionButton *shape_opt = nullptr;
	HSlider *radius_slider = nullptr;
	Label *radius_val_lbl = nullptr;
	HSlider *opacity_slider = nullptr;
	Label *opacity_val_lbl = nullptr;
	ColorPickerButton *fill_color_btn = nullptr;
	CheckBox *grad_chk = nullptr;
	ColorPickerButton *grad_top_btn = nullptr;
	ColorPickerButton *grad_bot_btn = nullptr;
	HSlider *border_w_slider = nullptr;
	ColorPickerButton *border_col_btn = nullptr;
	CheckBox *shadow_chk = nullptr;
	CheckBox *glow_chk = nullptr;
	ColorPickerButton *glow_col_btn = nullptr;

	// Imagen
	LineEdit *image_edit = nullptr;
	OptionButton *scale_opt = nullptr;
	SpinBox *nine_slice_spin = nullptr;
	OptionButton *filter_opt = nullptr;
	LineEdit *btn_state_edits[4] = { nullptr, nullptr, nullptr, nullptr };
	FileDialog *img_file_dialog = nullptr;
	int picking_img_target = -1; // -1=main, 0..3=btn state

	// Texto
	TextEdit *text_edit = nullptr;
	OptionButton *font_opt = nullptr;
	SpinBox *font_size_spin = nullptr;
	ColorPickerButton *font_color_btn = nullptr;
	OptionButton *align_opt = nullptr;
	OptionButton *binding_opt = nullptr;
	LineEdit *var_edit = nullptr;
	FileDialog *font_file_dialog = nullptr;

	// Acciones (host reutilizado en P4 + P5 lista de comandos)
	VBoxContainer *actions_host = nullptr;
	OptionButton *quick_action_opt = nullptr;
	Label *actions_summary_lbl = nullptr;

	void _build_ui();
	void _emit_commit();
	void _on_anchor_pressed(int p_idx);
	void _on_any_changed();
	void _on_open_img_picker(int p_target);
	void _on_clear_img_picker(int p_target);
	void _on_img_file_selected(const String &p_path);
	void _on_open_font_picker();
	void _on_font_file_selected(const String &p_path);
	void _on_theme_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void configure(ClockGearsUiDesignEditor *p_owner, const String &p_screen_id, const String &p_elem_id, const Dictionary &p_elem);
	void sync_from_element(const Dictionary &p_elem);
	void refresh_theme();
	String get_target_screen_id() const { return target_screen_id; }
	String get_target_elem_id() const { return target_elem_id; }
	Dictionary get_ui_handles() const;
	VBoxContainer *get_actions_host() const { return actions_host; }

	ClockGearsUiElementFloatingWindow();
};

#endif // CLOCKGEARS_UI_DESIGN_EDITOR_H
