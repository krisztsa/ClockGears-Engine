#pragma once

#include "../clockgears_animation.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/character_body_2d.h"

class ClockGearsMap;
class ClockGearsEvent;
class ClockGearsPlayer;

// Capas internas de colisión 2D de ClockGears para cuerpos y zonas de detección
enum ClockGearsPhysicsLayerBits {
	CG_PHYSICS_LAYER_PLAYER = 1 << 0,
	CG_PHYSICS_LAYER_ENEMY = 1 << 1,
	CG_PHYSICS_LAYER_NPC = 1 << 2,
};

enum ClockGearsEntityAIState {
	CG_AI_STATE_BASE = 0,
	CG_AI_STATE_CHASE = 1,
	CG_AI_STATE_FLEE = 2,
	CG_AI_STATE_RETURN_HOME = 3,
	CG_AI_STATE_STOPPED = 4,
};

class ClockGearsEntityNav : public Object {
	GDCLASS(ClockGearsEntityNav, Object);

protected:
	static void _bind_methods();

public:
	static ClockGearsMap *find_parent_map(const Node *p_node);
	static uint32_t target_filter_to_mask(const String &p_filter);

	static bool can_body_occupy_position(
			ClockGearsMap *p_map,
			const ClockGearsHitShape &p_shape,
			int p_dir,
			const Vector2 &p_pos,
			const Vector2 &p_prev_pos,
			bool p_through,
			const Node *p_self_owner,
			int p_tile_size = 32,
			Vector2 *r_blocker_center = nullptr);

	static Vector2 move_entity_with_slide(
			ClockGearsMap *p_map,
			const ClockGearsHitShape &p_shape,
			int p_dir,
			const Vector2 &p_pos,
			const Vector2 &p_delta,
			bool p_through,
			const Node *p_self_owner,
			int p_tile_size = 32);

	static bool compute_grid_astar_path(
			ClockGearsMap *p_map,
			const Vector2i &p_start,
			const Vector2i &p_goal,
			bool p_allow_diagonal,
			int p_max_nodes,
			Vector<Vector2i> &r_path);
};

class ClockGearsDetectionZone2D : public Area2D {
	GDCLASS(ClockGearsDetectionZone2D, Area2D);

private:
	ClockGearsEvent *owner_event = nullptr;
	int zone_index = 0;
	String zone_name;
	String target_filter = "player";
	String trigger = "enter"; // enter, exit, while_inside
	float interval_sec = 0.5f;
	Array actions;
	uint32_t shape_owner_id = 0;

	Vector<ObjectID> inside_targets;
	float while_timer = 0.0f;

	void _on_body_entered(Node *p_body);
	void _on_body_exited(Node *p_body);
	bool _matches_target(Node *p_body, Node **r_entity_node = nullptr) const;

protected:
	static void _bind_methods();

public:
	void configure(ClockGearsEvent *p_owner, int p_index, const Dictionary &p_zone_dict, int p_tile_size = 32);
	void tick_zone(float p_delta);

	String get_zone_name() const { return zone_name; }
	String get_target_filter() const { return target_filter; }
	String get_trigger() const { return trigger; }
	float get_interval_sec() const { return interval_sec; }
	Array get_actions() const { return actions; }
	int get_inside_count() const { return inside_targets.size(); }
	bool has_any_inside() const { return !inside_targets.is_empty(); }
	Node *get_first_inside_entity() const;

	ClockGearsDetectionZone2D();
};
