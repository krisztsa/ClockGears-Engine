#ifndef CLOCKGEARS_SHAPE_EDITOR_H
#define CLOCKGEARS_SHAPE_EDITOR_H

#include "clockgears_animation.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/texture.h"

class ClockGearsShapeEditor;

// ============================================================================
// Lienzo interactivo para formas 2D (Hitboxes, Hurtbox, Maniquí y Ancla)
// Reutilizable independientemente de la lógica de animaciones (cuerpos, zonas...).
// ============================================================================
class ClockGearsShapeCanvas : public Control {
	GDCLASS(ClockGearsShapeCanvas, Control);

	friend class ClockGearsShapeEditor;

	ClockGearsShapeEditor *owner_editor = nullptr;

	static Ref<StyleBoxFlat> _make_pill_style(const Color &p_bg, const Color &p_border, int p_radius = 4);
	void _draw_mannequin_silhouette(const Point2 &p_origin, int p_zoom, int p_size);
	void _draw_single_shape(const ClockGearsHitShape &p_shape, const Point2 &p_origin, int p_zoom, const Color &p_fill, const Color &p_border, const String &p_label, bool p_selected);
	void _draw_weapon_anchor(const ClockGearsWeaponAnchor &p_anchor, const Point2 &p_origin, int p_zoom, bool p_selected);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	virtual Size2 get_minimum_size() const override;
	Point2 get_canvas_origin() const;
	Vector2 screen_to_local(const Point2 &p_screen_pos) const;
	Point2 local_to_screen(const Vector2 &p_local_pos) const;
	void handle_canvas_event(const Ref<InputEvent> &p_event);

	ClockGearsShapeCanvas(ClockGearsShapeEditor *p_owner = nullptr);
};

// ============================================================================
// ClockGearsShapeEditor: Widget completo y desacoplado de edición de formas
// ============================================================================
class ClockGearsShapeEditor : public VBoxContainer {
	GDCLASS(ClockGearsShapeEditor, VBoxContainer);

	friend class ClockGearsShapeCanvas;

public:
	enum EditorMode {
		MODE_ANIMATION = 0,
		MODE_BODY_ZONES = 1,
		MODE_MOVEMENT = 2,
	};

	enum ToolMode {
		TOOL_SELECT = 0,
		TOOL_CIRCLE = 1,
		TOOL_RECT = 2,
		TOOL_CAPSULE = 3,
		TOOL_POLYGON = 4,
	};

	enum EditTarget {
		TARGET_HITBOX = 0,
		TARGET_HURTBOX = 1,
		TARGET_ANCHOR = 2,
	};

	enum SelectedKind {
		SEL_NONE = 0,
		SEL_HITBOX = 1,
		SEL_HURTBOX = 2,
		SEL_ANCHOR = 3,
		SEL_BODY = 4,
		SEL_ZONE = 5,
		SEL_WANDER_RADIUS = 6,
		SEL_PATROL_POINT = 7,
	};

private:
	enum DragKind {
		DRAG_NONE = 0,
		DRAG_MOVE_SHAPE = 1,
		DRAG_RESIZE_HANDLE = 2,
		DRAG_MOVE_VERTEX = 3,
		DRAG_CREATE_SHAPE = 4,
		DRAG_MOVE_ANCHOR = 5,
		DRAG_WANDER_RADIUS = 6,
		DRAG_PATROL_POINT = 7,
		DRAG_PAN_VIEW = 8,
	};

	struct ZoneEntry {
		Dictionary raw;
		String name;
		ClockGearsHitShape shape;
	};

	EditorMode editor_mode = MODE_ANIMATION;
	Vector2 view_pan;
	Point2 pan_start_screen;
	Vector2 pan_start_offset;

	// Contexto visual
	Ref<Texture2D> preview_texture;
	Rect2i preview_region = Rect2i(0, 0, 32, 32);
	bool show_mannequin = false;
	int mannequin_size = 32;

	// Capacidades configurables por el contenedor padre
	bool allow_hitboxes = true;
	bool allow_hurtbox = true;
	bool allow_weapon_anchor = true;
	bool bz_allow_zones = true;

	// Datos editados (genéricos, sin conocimiento de slots ni base de datos)
	ClockGearsFrameHitboxData hitbox_data;
	ClockGearsHurtboxData hurtbox_data;
	bool has_hurtbox_override = false;
	ClockGearsWeaponAnchor weapon_anchor_data;

	// Datos MODE_BODY_ZONES (cuerpo en px, zonas en px)
	ClockGearsHitShape body_shape = ClockGearsHitShape::make_default_body();
	bool body_through = false;
	Vector<ZoneEntry> bz_zones;
	int selected_bz_idx = -1; // -1 = Cuerpo, >= 0 = índice de zona

	// Datos MODE_MOVEMENT (radio/puntos en casillas relativas al origen; 1 casilla = 32 px)
	String move_mode = "quieto";
	double wander_radius = 3.0;
	Vector<Vector2> patrol_points;
	bool patrol_loop = false;
	int selected_patrol_idx = -1;
	Vector2 drag_initial_patrol_pt;

	// Estado de lienzo y herramienta
	int zoom = 4; // Entero 1..8
	bool grid_enabled = true;
	bool snap_enabled = false;
	ToolMode current_tool = TOOL_SELECT;
	EditTarget current_target = TARGET_HITBOX;
	SelectedKind selected_kind = SEL_NONE;
	int selected_hitbox_idx = -1;

	// Dibujo / arrastre en progreso
	DragKind drag_kind = DRAG_NONE;
	int drag_handle_id = -1; // -1=ninguno, o índice de tirador/vértice
	Vector2 drag_start_local;
	Point2 drag_press_screen;
	ClockGearsHitShape drag_initial_shape;
	ClockGearsHitShape create_draft_shape;
	bool create_drag_active = false;
	Vector2 drag_initial_anchor_pos;
	Vector<Vector2> poly_draft_vertices;
	Vector2 poly_hover_local;
	bool poly_hover_valid = false;

	bool updating_controls = false;

	// Widgets de barra superior (zoom, rejilla, imán, destino, herramientas)
	HFlowContainer *top_bar = nullptr;
	Button *zoom_btns[8] = { nullptr };
	Button *btn_grid = nullptr;
	Button *btn_snap = nullptr;
	Button *btn_center_view = nullptr;

	HFlowContainer *tools_bar = nullptr;
	Label *lbl_tgt_hdr = nullptr;
	Button *btn_target_hitbox = nullptr;
	Button *btn_target_hurtbox = nullptr;
	Button *btn_tool_select = nullptr;
	Button *btn_tool_circle = nullptr;
	Button *btn_tool_rect = nullptr;
	Button *btn_tool_capsule = nullptr;
	Button *btn_tool_polygon = nullptr;
	Button *btn_finish_polygon = nullptr;
	Button *btn_delete_shape = nullptr;
	Label *status_label = nullptr;

	// Área principal (Lienzo + Inspector)
	HBoxContainer *workspace_hb = nullptr;
	PanelContainer *canvas_frame = nullptr;
	ClockGearsShapeCanvas *canvas = nullptr;

	PanelContainer *inspector_panel = nullptr;
	ScrollContainer *inspector_scroll = nullptr;
	VBoxContainer *inspector_vbox = nullptr;

	// Grupo Cuerpo y Zonas (MODE_BODY_ZONES)
	VBoxContainer *bz_sec = nullptr;
	VBoxContainer *bz_list_box = nullptr;
	ItemList *bz_list = nullptr;
	Button *btn_bz_add = nullptr;
	Button *btn_bz_remove = nullptr;
	Button *btn_bz_dup = nullptr;
	CheckBox *chk_body_through = nullptr;
	HBoxContainer *bz_name_row = nullptr;
	LineEdit *bz_name_edit = nullptr;
	VBoxContainer *bz_zone_props_host = nullptr;

	// Grupo Hitbox
	VBoxContainer *hitbox_sec = nullptr;
	HSlider *hb_dmg_slider = nullptr;
	SpinBox *hb_dmg_spin = nullptr;
	HSlider *hb_kb_slider = nullptr;
	SpinBox *hb_kb_spin = nullptr;
	HSlider *hb_hitstop_slider = nullptr;
	SpinBox *hb_hitstop_spin = nullptr;

	// Grupo Hurtbox
	VBoxContainer *hurtbox_sec = nullptr;
	CheckBox *chk_hurtbox_override = nullptr;
	CheckBox *chk_hurtbox_enabled = nullptr;
	Label *lbl_hurtbox_mode = nullptr;
	HSlider *hurt_dmg_slider = nullptr;
	SpinBox *hurt_dmg_spin = nullptr;

	// Grupo Forma seleccionada (coordenadas/dimensiones precisas)
	VBoxContainer *sel_shape_sec = nullptr;
	Label *lbl_sel_shape_title = nullptr;
	SpinBox *sel_off_x_spin = nullptr;
	SpinBox *sel_off_y_spin = nullptr;
	Label *lbl_sel_dim1 = nullptr;
	SpinBox *sel_dim1_spin = nullptr;
	Label *lbl_sel_dim2 = nullptr;
	SpinBox *sel_dim2_spin = nullptr;
	SpinBox *sel_rot_spin = nullptr;

	// Grupo Ancla de arma
	VBoxContainer *anchor_sec = nullptr;
	CheckBox *chk_anchor_enabled = nullptr;
	SpinBox *anchor_x_spin = nullptr;
	SpinBox *anchor_y_spin = nullptr;
	HSlider *anchor_rot_slider = nullptr;
	SpinBox *anchor_rot_spin = nullptr;

	static Ref<StyleBoxFlat> _make_rounded_btn_style(const Color &p_bg, const Color &p_border, int p_radius = 8);
	void _apply_rounded_button_styles(Button *p_btn, bool p_active = false, const Color &p_accent = Color(0.24f, 0.50f, 0.88f, 1.0f));
	void _apply_spinbox_round_style(SpinBox *p_sb);

	Vector2 _maybe_snap_local(const Vector2 &p_local) const;
	ClockGearsHitShape *_get_mutable_selected_shape();
	const ClockGearsHitShape *_get_const_selected_shape() const;
	bool _hit_test_shape(const ClockGearsHitShape &p_shape, const Vector2 &p_local) const;
	int _hit_test_handles(const ClockGearsHitShape &p_shape, const Point2 &p_screen_pos, const Point2 &p_origin) const;
	void _get_shape_handle_points(const ClockGearsHitShape &p_shape, Vector<Vector2> &r_local_points) const;
	void _commit_created_shape(const ClockGearsHitShape &p_shape);
	static float _estimate_shape_area(const ClockGearsHitShape &p_shape);

	void _sync_inspector_from_data();
	void _sync_selected_shape_controls();
	void _refresh_bz_list();
	void _update_toolbar_states();
	void _emit_shape_changed();

	void _on_zoom_btn_pressed(int p_zoom);
	void _on_grid_toggled(bool p_pressed);
	void _on_snap_toggled(bool p_pressed);
	void _on_center_view_pressed();
	void _on_target_btn_pressed(int p_target);
	void _on_tool_btn_pressed(int p_tool);
	void _on_finish_polygon_pressed();
	void _on_delete_shape_pressed();

	void _on_bz_list_selected(int p_idx);
	void _on_bz_add_pressed();
	void _on_bz_remove_pressed();
	void _on_bz_dup_pressed();
	void _on_body_through_toggled(bool p_pressed);
	void _on_bz_name_changed(const String &p_text);

	void _on_hb_dmg_slider_changed(double p_val);
	void _on_hb_dmg_spin_changed(double p_val);
	void _on_hb_kb_slider_changed(double p_val);
	void _on_hb_kb_spin_changed(double p_val);
	void _on_hb_hitstop_slider_changed(double p_val);
	void _on_hb_hitstop_spin_changed(double p_val);

	void _on_hurtbox_override_toggled(bool p_pressed);
	void _on_hurtbox_enabled_toggled(bool p_pressed);
	void _on_hurt_dmg_slider_changed(double p_val);
	void _on_hurt_dmg_spin_changed(double p_val);

	void _on_sel_shape_prop_changed(double p_unused = 0.0);

	void _on_anchor_enabled_toggled(bool p_pressed);
	void _on_anchor_pos_changed(double p_unused = 0.0);
	void _on_anchor_rot_slider_changed(double p_val);
	void _on_anchor_rot_spin_changed(double p_val);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static Color get_zone_color(int p_index);

	void set_editor_mode(int p_mode);
	int get_editor_mode() const { return (int)editor_mode; }

	void set_preview_texture(const Ref<Texture2D> &p_tex, const Rect2i &p_region);
	void set_mannequin(bool p_show, int p_size);
	bool is_mannequin_shown() const { return show_mannequin; }
	int get_mannequin_size() const { return mannequin_size; }

	void set_feature_flags(bool p_allow_hitboxes, bool p_allow_hurtbox, bool p_allow_weapon_anchor);
	void set_shape_data(const Dictionary &p_hitbox_dict, const Dictionary &p_hurtbox_dict, bool p_has_hurtbox_override, const Dictionary &p_weapon_anchor_dict);

	Dictionary get_hitbox_data() const;
	Dictionary get_hurtbox_data() const;
	bool has_hurtbox_override_enabled() const { return has_hurtbox_override; }
	Dictionary get_weapon_anchor_data() const;

	// API MODE_BODY_ZONES
	void set_body_zones_config(bool p_allow_zones);
	void set_body_zones_data(const Dictionary &p_body_shape, bool p_body_through, const Array &p_zones);
	Dictionary get_body_shape_data() const;
	bool is_body_through() const { return body_through; }
	Array get_zones_data() const;
	void set_selected_zone_metadata(int p_zone_idx, const Dictionary &p_zone_dict);
	Dictionary get_selected_zone_metadata() const;
	void select_entity_item(int p_bz_index);
	int get_selected_bz_index() const { return selected_bz_idx; }
	int add_entity_zone(const Dictionary &p_zone_dict = Dictionary());
	bool remove_selected_entity_zone();
	int duplicate_selected_entity_zone();
	VBoxContainer *get_zone_props_host() const { return bz_zone_props_host; }

	// API MODE_MOVEMENT
	void set_movement_visual_data(const String &p_mode, double p_wander_radius, const Array &p_patrol_points, bool p_patrol_loop);
	String get_movement_mode() const { return move_mode; }
	double get_wander_radius() const { return wander_radius; }
	Array get_patrol_points_data() const;
	void select_patrol_point(int p_idx);
	int get_selected_patrol_index() const { return selected_patrol_idx; }
	int add_patrol_point(const Vector2 &p_tile_offset);
	bool remove_selected_patrol_point();

	void set_zoom(int p_zoom);
	int get_zoom() const { return zoom; }
	void set_grid_enabled(bool p_enabled);
	bool is_grid_enabled() const { return grid_enabled; }
	void set_snap_enabled(bool p_enabled);
	bool is_snap_enabled() const { return snap_enabled; }

	void set_edit_target(int p_target);
	int get_edit_target() const { return (int)current_target; }
	void set_tool(int p_tool);
	int get_tool() const { return (int)current_tool; }

	int add_hitbox_shape(const Dictionary &p_shape_dict);
	void set_hurtbox_shape(const Dictionary &p_shape_dict);
	bool finish_polygon();
	bool delete_selected_shape();
	void select_shape(int p_kind, int p_index = 0);
	int get_selected_kind() const { return (int)selected_kind; }
	int get_selected_index() const { return selected_hitbox_idx; }
	Control *get_shape_canvas() const { return canvas; }

	ClockGearsShapeEditor();
	virtual ~ClockGearsShapeEditor() {}
};

#endif // CLOCKGEARS_SHAPE_EDITOR_H
