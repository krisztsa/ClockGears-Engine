#pragma once

#include "clockgears_types.h"
#include "scene/2d/node_2d.h"
#include "scene/main/canvas_layer.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"

class ClockGearsMap;
class ClockGearsPlayer;
class ClockGearsPixelCamera2D;

// Lienzo en coordenadas de mundo para dibujar rejilla, colisiones y eventos
class ClockGearsDebugWorldCanvas : public Node2D {
	GDCLASS(ClockGearsDebugWorldCanvas, Node2D);

private:
	ClockGearsMap *map = nullptr;
	ClockGearsPixelCamera2D *camera = nullptr;

	bool show_events = false;
	bool show_collisions = false;
	bool show_grid = false;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void setup(ClockGearsMap *p_map, ClockGearsPixelCamera2D *p_camera);

	void set_show_events(bool p_show);
	bool is_show_events() const { return show_events; }

	void set_show_collisions(bool p_show);
	bool is_show_collisions() const { return show_collisions; }

	void set_show_grid(bool p_show);
	bool is_show_grid() const { return show_grid; }

	ClockGearsDebugWorldCanvas();
};

// Capa de interfaz de depuración (HUD + Ventana flotante F3)
class ClockGearsDebugOverlay : public CanvasLayer {
	GDCLASS(ClockGearsDebugOverlay, CanvasLayer);

private:
	ClockGearsMap *map = nullptr;
	ClockGearsPlayer *player = nullptr;
	ClockGearsPixelCamera2D *camera = nullptr;
	ClockGearsDebugWorldCanvas *world_canvas = nullptr;

	// Ventana flotante (F3)
	PanelContainer *floating_window = nullptr;
	bool is_dragging = false;
	Vector2 drag_offset;

	// Casillas de Visualización (Punto 2)
	CheckBox *chk_events = nullptr;
	CheckBox *chk_collisions = nullptr;
	CheckBox *chk_grid = nullptr;
	CheckBox *chk_fps = nullptr;
	CheckBox *chk_player = nullptr;
	CheckBox *chk_camera = nullptr;

	// Espacio preparado: Herramientas (Punto 3)
	CheckBox *chk_noclip = nullptr;
	Tree *vars_tree = nullptr;
	LineEdit *vars_search = nullptr;

	// HUD de información en pantalla
	PanelContainer *hud_card = nullptr;
	Label *lbl_fps = nullptr;
	Label *lbl_player = nullptr;
	Label *lbl_camera = nullptr;

	bool is_loading_settings = false;

	// Log de Batalla (Fase 8A)
	RichTextLabel *battle_log_rtl = nullptr;

	void _on_chk_toggled(bool p_val);
	void _on_noclip_toggled(bool p_val);
	void _on_title_gui_input(const Ref<InputEvent> &p_event);
	void _save_settings();
	void _load_settings();
	void _update_hud();
	void _populate_vars_tree();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static bool is_playtest_mode();
	static void log_battle(const String &p_entry);

	void setup_overlay(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsPixelCamera2D *p_camera);
	void toggle_debug_panel();
	bool is_panel_open() const;

	void set_show_events(bool p_show);
	bool is_show_events() const;

	void set_show_collisions(bool p_show);
	bool is_show_collisions() const;

	void set_show_grid(bool p_show);
	bool is_show_grid() const;

	void set_show_fps(bool p_show);
	bool is_show_fps() const;

	void set_show_player(bool p_show);
	bool is_show_player() const;

	void set_show_camera(bool p_show);
	bool is_show_camera() const;

	ClockGearsDebugWorldCanvas *get_world_canvas() const { return world_canvas; }

	ClockGearsDebugOverlay();
	~ClockGearsDebugOverlay();
};
