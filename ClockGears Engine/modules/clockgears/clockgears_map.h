#pragma once

#include "clockgears_types.h"
#include "clockgears_event.h"
#include "scene/2d/node_2d.h"
#include "modules/tilemap/tile_map_layer.h"
#include "scene/2d/sprite_2d.h"

class ClockGearsPlayer;
class ClockGearsFollowerManager;
class ClockGearsPixelCamera2D;
class ClockGearsEventInterpreter;
class ClockGearsDebugOverlay;

class ClockGearsMap : public Node2D {
	GDCLASS(ClockGearsMap, Node2D);

private:
	int grid_resolution = GRID_32X32;
	bool snap_to_grid = true;
	int combat_mode = COMBAT_MODE_TURN_BASED;
	int map_width = 25;
	int map_height = 20;
	String map_name = "Mapa001";
	String bgm_path;
	int tileset_id = 0;

	TileMapLayer *layer_ground = nullptr;
	TileMapLayer *layer_decoration = nullptr;
	TileMapLayer *layer_objects = nullptr;
	TileMapLayer *layer_overhead = nullptr;
	Node2D *events_container = nullptr;

	// Sección 7 (Fase 6): Instancias de Runtime Base
	ClockGearsPlayer *runtime_player = nullptr;
	ClockGearsFollowerManager *runtime_followers = nullptr;
	ClockGearsPixelCamera2D *runtime_camera = nullptr;
	ClockGearsEventInterpreter *runtime_interpreter = nullptr;
	ClockGearsDebugOverlay *runtime_debug_overlay = nullptr;

	// Sección 6.3: Capa de Regiones (IDs 0-255) y Capa de Sombras
	Dictionary region_map;
	Dictionary shadow_map;

	void _bind_layer_pointers();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void ensure_default_layers();
	void create_default_layers() { ensure_default_layers(); }

	void set_grid_resolution(int p_res);
	int get_grid_resolution() const { return grid_resolution; }

	void set_snap_to_grid(bool p_snap) { snap_to_grid = p_snap; }
	bool is_snap_to_grid() const { return snap_to_grid; }

	void set_combat_mode(int p_mode) { combat_mode = p_mode; }
	int get_combat_mode() const { return combat_mode; }

	void set_map_name(const String &p_name) { map_name = p_name; }
	String get_map_name() const { return map_name; }

	void set_map_size(Vector2i p_size);
	Vector2i get_map_size() const { return Vector2i(map_width, map_height); }

	void set_region_map(const Dictionary &p_map) { region_map = p_map.duplicate(true); }
	Dictionary get_region_map() const { return region_map; }
	void set_cell_region(const Vector2i &p_cell, int p_region_id);
	int get_cell_region(const Vector2i &p_cell) const;

	void set_shadow_map(const Dictionary &p_map) { shadow_map = p_map.duplicate(true); }
	Dictionary get_shadow_map() const { return shadow_map; }
	void set_cell_shadow(const Vector2i &p_cell, bool p_enabled);
	bool get_cell_shadow(const Vector2i &p_cell) const;

	void set_bgm_path(const String &p_bgm) { bgm_path = p_bgm; }
	String get_bgm_path() const { return bgm_path; }

	void set_tileset_id(int p_id) { tileset_id = p_id; }
	int get_tileset_id() const { return tileset_id; }

	bool is_tile_passable(const Vector2i &p_cell, int p_dir = 0) const;
	int get_cell_collision_type(const Vector2i &p_cell) const;

	TileMapLayer *get_layer(int p_layer_idx);
	Node2D *get_events_container();

	void setup_tileset_from_database(int p_tileset_id);

	// Key functions from Requirement 3:
	Node2D *import_image_to_map(const String &p_image_path, int p_layer_idx, Vector2 p_world_pos);
	Vector2 snap_position(Vector2 p_pos) const;

	void add_clockgears_event(ClockGearsEvent *p_event, Vector2 p_pos);

	bool import_tileset_png(const String &p_path);
	void set_tile_at(int p_layer_idx, const Vector2i &p_cell, const Vector2i &p_atlas_coords);
	Vector2i get_tile_at(int p_layer_idx, const Vector2i &p_cell);
	void set_player_start_position(const Vector2i &p_cell, int p_dir = 2);

	void init_runtime_session();
	ClockGearsPlayer *get_runtime_player() const { return runtime_player; }
	ClockGearsFollowerManager *get_runtime_followers() const { return runtime_followers; }
	ClockGearsPixelCamera2D *get_runtime_camera() const { return runtime_camera; }
	void set_runtime_camera(ClockGearsPixelCamera2D *p_camera) { runtime_camera = p_camera; }
	ClockGearsEventInterpreter *get_runtime_interpreter() const { return runtime_interpreter; }
	ClockGearsDebugOverlay *get_runtime_debug_overlay() const { return runtime_debug_overlay; }
	void set_runtime_debug_overlay(ClockGearsDebugOverlay *p_overlay) { runtime_debug_overlay = p_overlay; }

	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

	ClockGearsMap();
};
