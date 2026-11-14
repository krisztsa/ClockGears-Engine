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
#include "scene/gui/option_button.h"
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
	bool show_bodies_zones = false;
	bool show_damage_numbers = true;

	struct FloatingDamageEntry {
		Vector2 world_pos;
		int amount = 0;
		bool is_crit = false;
		float remaining = 0.85f;
		float max_time = 0.85f;
	};
	Vector<FloatingDamageEntry> floating_damages;
	Array recent_damage_log;

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

	void set_show_bodies_zones(bool p_show);
	bool is_show_bodies_zones() const { return show_bodies_zones; }

	void set_show_damage_numbers(bool p_show);
	bool is_show_damage_numbers() const { return show_damage_numbers; }
	void push_damage_number(const Vector2 &p_world_pos, int p_amount, bool p_is_crit = false);
	Array get_recent_damage_numbers() const { return recent_damage_log.duplicate(true); }
	void clear_damage_numbers();

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
	CheckBox *chk_bodies_zones = nullptr;
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
	CheckBox *chk_battle_turns = nullptr;
	CheckBox *chk_battle_formulas = nullptr;
	RichTextLabel *battle_log_rtl = nullptr;

	// Equipo ARPG (Tanda 1C)
	OptionButton *arpg_actor_opt = nullptr;
	Vector<OptionButton *> arpg_slot_opts;
	Vector<String> arpg_slot_keys;
	Label *lbl_arpg_stats = nullptr;

	void _on_chk_toggled(bool p_val);
	void _on_battle_chk_toggled(bool p_val);
	void _on_noclip_toggled(bool p_val);
	void _on_title_gui_input(const Ref<InputEvent> &p_event);
	void _save_settings();
	void _load_settings();
	void _update_hud();
	void _populate_vars_tree();
	void _populate_arpg_equip_tab();
	void _on_arpg_actor_selected(int p_idx);
	void _on_arpg_slot_item_selected(int p_idx, int p_slot_idx);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	static ClockGearsDebugOverlay *get_singleton();
	static bool is_playtest_mode();
	static void log_battle(const String &p_entry);
	static String get_battle_log_text();
	static void clear_battle_log();

	static void set_show_battle_turns(bool p_show);
	static bool is_show_battle_turns();

	static void set_show_battle_formulas(bool p_show);
	static bool is_show_battle_formulas();

	void setup_overlay(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsPixelCamera2D *p_camera);
	void toggle_debug_panel();
	bool is_panel_open() const;
	void refresh_arpg_equip_tab();
	void set_arpg_equip_debug_slot(int p_actor_id, const String &p_slot_key, int p_item_id);

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

	void set_show_bodies_zones(bool p_show);
	bool is_show_bodies_zones() const;

	void push_damage_number(const Vector2 &p_world_pos, int p_amount, bool p_is_crit = false);
	Array get_recent_damage_numbers() const;
	void clear_damage_numbers();
	Dictionary get_player_hp_debug_info() const;
	String get_player_hp_summary() const;

	ClockGearsDebugWorldCanvas *get_world_canvas() const { return world_canvas; }

	ClockGearsDebugOverlay();
	~ClockGearsDebugOverlay();
};
