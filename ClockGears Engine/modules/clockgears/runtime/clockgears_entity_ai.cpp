#include "clockgears_entity_ai.h"
#include "../clockgears_event.h"
#include "../clockgears_map.h"
#include "../core/clockgears_data_manager.h"
#include "clockgears_runtime.h"

void ClockGearsEntityNav::_bind_methods() {
}

ClockGearsMap *ClockGearsEntityNav::find_parent_map(const Node *p_node) {
	const Node *cur = p_node ? p_node->get_parent() : nullptr;
	while (cur) {
		ClockGearsMap *m = Object::cast_to<ClockGearsMap>(const_cast<Node *>(cur));
		if (m) {
			return m;
		}
		cur = cur->get_parent();
	}
	return nullptr;
}

uint32_t ClockGearsEntityNav::target_filter_to_mask(const String &p_filter) {
	String f = p_filter.strip_edges().to_lower();
	if (f == "enemies" || f == "enemy" || f == "enemigos") {
		return CG_PHYSICS_LAYER_ENEMY;
	}
	if (f == "npc" || f == "npcs") {
		return CG_PHYSICS_LAYER_NPC;
	}
	if (f == "all" || f == "todos") {
		return CG_PHYSICS_LAYER_PLAYER | CG_PHYSICS_LAYER_ENEMY | CG_PHYSICS_LAYER_NPC;
	}
	return CG_PHYSICS_LAYER_PLAYER;
}

static bool s_circle_overlaps_rect(const Vector2 &p_center, float p_radius, const Rect2 &p_rect) {
	Vector2 closest(
			CLAMP(p_center.x, p_rect.position.x, p_rect.position.x + p_rect.size.x),
			CLAMP(p_center.y, p_rect.position.y, p_rect.position.y + p_rect.size.y));
	Vector2 diff = p_center - closest;
	return diff.length_squared() < (p_radius * p_radius);
}

static float s_approx_shape_radius(const ClockGearsHitShape &p_shape) {
	Rect2 b = ClockGearsShapeGeometry::compute_shape_local_bounds(p_shape);
	return MAX(Math::abs(b.position.x), Math::abs(b.position.x + b.size.x)) +
			MAX(Math::abs(b.position.y), Math::abs(b.position.y + b.size.y));
}

static Vector2 s_entity_body_local_origin(const Node *p_node, int p_ts) {
	if (const ClockGearsPlayer *pl = Object::cast_to<ClockGearsPlayer>(p_node)) {
		return pl->get_body_local_origin();
	}
	if (const ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(p_node)) {
		if (ev->get_behavior_type() != 0) {
			return ev->get_body_local_origin();
		}
	}
	return Vector2(p_ts * 0.5f, p_ts * 0.5f);
}

bool ClockGearsEntityNav::can_body_occupy_position(
		ClockGearsMap *p_map,
		const ClockGearsHitShape &p_shape,
		int p_dir,
		const Vector2 &p_pos,
		const Vector2 &p_prev_pos,
		bool p_through,
		const Node *p_self_owner,
		int p_tile_size,
		Vector2 *r_blocker_center) {
	if (r_blocker_center) {
		*r_blocker_center = Vector2();
	}
	if (!p_map || p_through) {
		return true;
	}

	int ts = MAX(16, p_tile_size);
	Vector2i msize = p_map->get_map_size();
	Vector2 self_local_origin = s_entity_body_local_origin(p_self_owner, ts);
	Vector2 world_center = p_pos + self_local_origin;
	Vector2 prev_center = p_prev_pos + self_local_origin;

	Rect2 local_bounds = ClockGearsShapeGeometry::compute_shape_local_bounds(p_shape);
	const float skin = 0.35f;
	Vector2 bpos = world_center + local_bounds.position + Vector2(skin, skin);
	Vector2 bsize = (local_bounds.size - Vector2(skin * 2.0f, skin * 2.0f)).maxf(1.0f);
	Rect2 tile_query_rect(bpos, bsize);

	int min_cx = int(Math::floor(tile_query_rect.position.x / float(ts)));
	int max_cx = int(Math::floor((tile_query_rect.position.x + tile_query_rect.size.x - 0.001f) / float(ts)));
	int min_cy = int(Math::floor(tile_query_rect.position.y / float(ts)));
	int max_cy = int(Math::floor((tile_query_rect.position.y + tile_query_rect.size.y - 0.001f) / float(ts)));

	if (min_cx < 0 || min_cy < 0 || max_cx >= msize.x || max_cy >= msize.y) {
		return false;
	}

	// 1. Pasabilidad de tiles (X bloqueado, y forma exacta para círculos/cápsulas/polígonos)
	ClockGearsHitShape shrunken_shape = p_shape;
	if (shrunken_shape.type == CG_SHAPE_CIRCLE) {
		shrunken_shape.size.x = MAX(0.5f, p_shape.get_radius() - 0.25f);
		shrunken_shape.size.y = shrunken_shape.size.x;
	}
	for (int cy = min_cy; cy <= max_cy; cy++) {
		for (int cx = min_cx; cx <= max_cx; cx++) {
			int d4 = 15;
			if (p_map->get_tile_passage_mode(Vector2i(cx, cy), d4) == 1) {
				Rect2 cell_rect(cx * float(ts), cy * float(ts), float(ts), float(ts));
				bool hits_tile = true;
				if (p_shape.type == CG_SHAPE_CIRCLE) {
					hits_tile = s_circle_overlaps_rect(world_center + p_shape.offset, MAX(0.5f, p_shape.get_radius() - 0.25f), cell_rect);
				} else if (p_shape.type != CG_SHAPE_RECT || !Math::is_zero_approx(p_shape.rotation)) {
					ClockGearsHitShape tile_rect_shape = ClockGearsHitShape::make_rect(Vector2(float(ts) - 0.5f, float(ts) - 0.5f));
					hits_tile = ClockGearsShapeGeometry::shapes_overlap_native(shrunken_shape, world_center, p_dir, tile_rect_shape, Vector2((cx + 0.5f) * ts, (cy + 0.5f) * ts), 0);
				}
				if (hits_tile) {
					if (r_blocker_center) {
						*r_blocker_center = Vector2((cx + 0.5f) * ts, (cy + 0.5f) * ts);
					}
					return false;
				}
			}
		}
	}

	// 2. Pasabilidad direccional de 4 direcciones + regla de esquinas diagonales (modo 8)
	Vector2i prev_cell(int(Math::floor(prev_center.x / float(ts))), int(Math::floor(prev_center.y / float(ts))));
	Vector2i cur_cell(int(Math::floor(world_center.x / float(ts))), int(Math::floor(world_center.y / float(ts))));
	if (cur_cell != prev_cell) {
		// Pasos de celda adyacente (o diagonal) heredan exactamente can_pass_tile_step
		if (Math::abs(cur_cell.x - prev_cell.x) <= 1 && Math::abs(cur_cell.y - prev_cell.y) <= 1) {
			if (!p_map->can_pass_tile_step(prev_cell, cur_cell, p_dir)) {
				return false;
			}
		}
	}
	// En movimiento diagonal suave: evitar atravesar esquina cuando 2 cardinales adyacentes están bloqueadas
	Vector2 move_vec = p_pos - p_prev_pos;
	if (Math::abs(move_vec.x) > 0.001f && Math::abs(move_vec.y) > 0.001f && min_cx != max_cx && min_cy != max_cy) {
		int dummy_d4 = 15;
		bool b00 = p_map->get_tile_passage_mode(Vector2i(min_cx, min_cy), dummy_d4) == 1;
		bool b10 = p_map->get_tile_passage_mode(Vector2i(max_cx, min_cy), dummy_d4) == 1;
		bool b01 = p_map->get_tile_passage_mode(Vector2i(min_cx, max_cy), dummy_d4) == 1;
		bool b11 = p_map->get_tile_passage_mode(Vector2i(max_cx, max_cy), dummy_d4) == 1;
		if (b00 || b10 || b01 || b11) {
			return false;
		}
	}

	// 3. Colisión con cuerpos de otras entidades (jugador, enemigos, NPCs, eventos bloqueantes)
	float self_r = s_approx_shape_radius(p_shape);

	auto overlaps_other = [&](const ClockGearsHitShape &p_other_shape, const Vector2 &p_other_pos, const Vector2 &p_other_local_origin, int p_other_dir) -> bool {
		Vector2 other_center = p_other_pos + p_other_local_origin;
		float other_r = s_approx_shape_radius(p_other_shape);
		float max_d = self_r + other_r + 4.0f;
		float dist_sq = world_center.distance_squared_to(other_center);
		if (dist_sq > max_d * max_d) {
			return false;
		}
		if (!ClockGearsShapeGeometry::shapes_overlap_native(p_shape, world_center, p_dir, p_other_shape, other_center, p_other_dir)) {
			return false;
		}
		// Permitir alejarse si ya estaban ligeramente solapados previamente (evita quedarse pegado), bloquear si aumenta penetración
		float prev_dist_sq = prev_center.distance_squared_to(other_center);
		if (ClockGearsShapeGeometry::shapes_overlap_native(p_shape, prev_center, p_dir, p_other_shape, other_center, p_other_dir) && dist_sq >= prev_dist_sq) {
			return false;
		}
		if (r_blocker_center) {
			*r_blocker_center = other_center;
		}
		return true;
	};

	ClockGearsPlayer *player = p_map->get_runtime_player();
	if (player && player != p_self_owner && !player->is_through_enabled() && !player->is_body_through()) {
		if (overlaps_other(player->get_body_shape_struct(), player->get_position(), player->get_body_local_origin(), player->get_direction())) {
			return false;
		}
	}

	Node2D *ev_cont = p_map->get_events_container();
	if (ev_cont) {
		int ccount = ev_cont->get_child_count();
		for (int i = 0; i < ccount; i++) {
			ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(ev_cont->get_child(i));
			if (!ev || ev == p_self_owner || !ev->is_visible()) {
				continue;
			}
			if (ev->get_behavior_type() == 0) {
				if (ev->is_through() || ev->get_priority_type() != 1) {
					continue;
				}
				ClockGearsHitShape std_box = ClockGearsHitShape::make_rect(Vector2(float(ts) - 1.0f, float(ts) - 1.0f));
				if (overlaps_other(std_box, ev->get_position(), Vector2(ts * 0.5f, ts * 0.5f), ev->get_direction())) {
					return false;
				}
			} else {
				if (ev->is_through() || ev->is_body_through()) {
					continue;
				}
				if (overlaps_other(ev->get_body_shape_struct(), ev->get_position(), ev->get_body_local_origin(), ev->get_direction())) {
					return false;
				}
			}
		}
	}

	return true;
}

Vector2 ClockGearsEntityNav::move_entity_with_slide(
		ClockGearsMap *p_map,
		const ClockGearsHitShape &p_shape,
		int p_dir,
		const Vector2 &p_pos,
		const Vector2 &p_delta,
		bool p_through,
		const Node *p_self_owner,
		int p_tile_size) {
	if (!p_map) {
		return p_pos + p_delta;
	}
	float total_len = p_delta.length();
	if (total_len < 0.0001f) {
		return p_pos;
	}
	if (p_through) {
		return p_pos + p_delta;
	}

	int substeps = MAX(1, int(Math::ceil(total_len / 4.0f)));
	Vector2 step = p_delta / float(substeps);
	Vector2 cur = p_pos;
	Vector2 half_tile(p_tile_size * 0.5f, p_tile_size * 0.5f);

	for (int s = 0; s < substeps; s++) {
		Vector2 blocker_center;
		Vector2 full_next = cur + step;
		if (can_body_occupy_position(p_map, p_shape, p_dir, full_next, cur, false, p_self_owner, p_tile_size, &blocker_center)) {
			cur = full_next;
			continue;
		}

		// Deslizamiento por ejes (intentar primero el eje dominante)
		bool moved_axis = false;
		if (Math::abs(step.x) >= Math::abs(step.y)) {
			if (Math::abs(step.x) > 0.001f && can_body_occupy_position(p_map, p_shape, p_dir, cur + Vector2(step.x, 0.0f), cur, false, p_self_owner, p_tile_size)) {
				cur.x += step.x;
				moved_axis = true;
			}
			if (Math::abs(step.y) > 0.001f && can_body_occupy_position(p_map, p_shape, p_dir, cur + Vector2(0.0f, step.y), cur, false, p_self_owner, p_tile_size)) {
				cur.y += step.y;
				moved_axis = true;
			}
		} else {
			if (Math::abs(step.y) > 0.001f && can_body_occupy_position(p_map, p_shape, p_dir, cur + Vector2(0.0f, step.y), cur, false, p_self_owner, p_tile_size)) {
				cur.y += step.y;
				moved_axis = true;
			}
			if (Math::abs(step.x) > 0.001f && can_body_occupy_position(p_map, p_shape, p_dir, cur + Vector2(step.x, 0.0f), cur, false, p_self_owner, p_tile_size)) {
				cur.x += step.x;
				moved_axis = true;
			}
		}

		// Si chocó de frente contra un cuerpo/curva (no contra pared plana) proyectar sobre la tangente para deslizar sin atascarse
		if (!moved_axis && blocker_center != Vector2()) {
			Vector2 normal = ((cur + s_entity_body_local_origin(p_self_owner, p_tile_size)) - blocker_center);
			float nlen = normal.length();
			if (nlen > 0.01f) {
				normal /= nlen;
				Vector2 tangent = step - normal * step.dot(normal);
				if (tangent.length() > step.length() * 0.08f) {
					if (can_body_occupy_position(p_map, p_shape, p_dir, cur + tangent, cur, false, p_self_owner, p_tile_size)) {
						cur += tangent;
					}
				}
			}
		}
	}

	return cur;
}

// ============================================================================
// Búsqueda de Ruta A* en Rejilla (con sello incremental O(1) para >60 FPS)
// ============================================================================
namespace {
struct CG_AStarHeapNode {
	int idx;
	int f_score;
};

struct CG_AStarScratch {
	Vector<uint32_t> stamp_arr;
	Vector<int> g_score;
	Vector<int> parent_idx;
	Vector<uint8_t> closed;
	Vector<CG_AStarHeapNode> heap;
	uint32_t current_stamp = 1;

	void prepare(int p_total_cells) {
		if (stamp_arr.size() < p_total_cells) {
			int old_sz = stamp_arr.size();
			stamp_arr.resize(p_total_cells);
			g_score.resize(p_total_cells);
			parent_idx.resize(p_total_cells);
			closed.resize(p_total_cells);
			uint32_t *sw = stamp_arr.ptrw();
			for (int i = old_sz; i < p_total_cells; i++) {
				sw[i] = 0;
			}
		}
		current_stamp++;
		if (current_stamp == 0) {
			uint32_t *sw = stamp_arr.ptrw();
			for (int i = 0; i < stamp_arr.size(); i++) {
				sw[i] = 0;
			}
			current_stamp = 1;
		}
		heap.clear();
	}

	void heap_push(int p_idx, int p_f) {
		heap.push_back({ p_idx, p_f });
		int i = heap.size() - 1;
		CG_AStarHeapNode *h = heap.ptrw();
		while (i > 0) {
			int p = (i - 1) >> 1;
			if (h[p].f_score <= h[i].f_score) {
				break;
			}
			CG_AStarHeapNode tmp = h[p];
			h[p] = h[i];
			h[i] = tmp;
			i = p;
		}
	}

	int heap_pop() {
		CG_AStarHeapNode *h = heap.ptrw();
		int top = h[0].idx;
		int last = heap.size() - 1;
		if (last > 0) {
			h[0] = h[last];
			heap.resize(last);
			int i = 0;
			while (true) {
				int l = (i << 1) + 1;
				int r = l + 1;
				int smallest = i;
				if (l < last && h[l].f_score < h[smallest].f_score) smallest = l;
				if (r < last && h[r].f_score < h[smallest].f_score) smallest = r;
				if (smallest == i) break;
				CG_AStarHeapNode tmp = h[i];
				h[i] = h[smallest];
				h[smallest] = tmp;
				i = smallest;
			}
		} else {
			heap.clear();
		}
		return top;
	}
};

static CG_AStarScratch s_astar_scratch;
} // namespace

bool ClockGearsEntityNav::compute_grid_astar_path(
		ClockGearsMap *p_map,
		const Vector2i &p_start,
		const Vector2i &p_goal,
		bool p_allow_diagonal,
		int p_max_nodes,
		Vector<Vector2i> &r_path) {
	r_path.clear();
	if (!p_map) {
		return false;
	}
	Vector2i msize = p_map->get_map_size();
	int w = msize.x;
	int h = msize.y;
	if (w <= 0 || h <= 0) {
		return false;
	}
	if (p_start.x < 0 || p_start.y < 0 || p_start.x >= w || p_start.y >= h) {
		return false;
	}
	if (p_goal.x < 0 || p_goal.y < 0 || p_goal.x >= w || p_goal.y >= h) {
		return false;
	}
	if (p_start == p_goal) {
		r_path.push_back(p_goal);
		return true;
	}

	int dummy_d4 = 15;
	if (p_map->get_tile_passage_mode(p_goal, dummy_d4) == 1) {
		return false;
	}

	int total = w * h;
	s_astar_scratch.prepare(total);
	uint32_t st = s_astar_scratch.current_stamp;
	uint32_t *stamps = s_astar_scratch.stamp_arr.ptrw();
	int *g = s_astar_scratch.g_score.ptrw();
	int *parent = s_astar_scratch.parent_idx.ptrw();
	uint8_t *closed = s_astar_scratch.closed.ptrw();

	auto heuristic = [&](int cx, int cy) -> int {
		int dx = Math::abs(p_goal.x - cx);
		int dy = Math::abs(p_goal.y - cy);
		if (p_allow_diagonal) {
			int mn = MIN(dx, dy);
			int mx = MAX(dx, dy);
			return mn * 14 + (mx - mn) * 10;
		}
		return (dx + dy) * 10;
	};

	int start_idx = p_start.y * w + p_start.x;
	int goal_idx = p_goal.y * w + p_goal.x;
	stamps[start_idx] = st;
	g[start_idx] = 0;
	parent[start_idx] = -1;
	closed[start_idx] = 0;
	s_astar_scratch.heap_push(start_idx, heuristic(p_start.x, p_start.y));

	static const int dx8[8] = { 0, -1, 1, 0, -1, 1, -1, 1 };
	static const int dy8[8] = { 1, 0, 0, -1, 1, 1, -1, -1 };
	static const int dir8[8] = { 2, 4, 6, 8, 1, 3, 7, 9 };
	int n_dirs = p_allow_diagonal ? 8 : 4;

	int expanded = 0;
	int max_n = MAX(16, p_max_nodes);
	bool found = false;

	while (!s_astar_scratch.heap.is_empty()) {
		int u = s_astar_scratch.heap_pop();
		if (u == goal_idx) {
			found = true;
			break;
		}
		if (stamps[u] == st && closed[u]) {
			continue;
		}
		closed[u] = 1;
		expanded++;
		if (expanded > max_n) {
			break;
		}

		int ux = u % w;
		int uy = u / w;
		Vector2i ucell(ux, uy);

		for (int d = 0; d < n_dirs; d++) {
			int vx = ux + dx8[d];
			int vy = uy + dy8[d];
			if (vx < 0 || vy < 0 || vx >= w || vy >= h) {
				continue;
			}
			int v = vy * w + vx;
			if (stamps[v] == st && closed[v]) {
				continue;
			}
			if (!p_map->can_pass_tile_step(ucell, Vector2i(vx, vy), dir8[d])) {
				continue;
			}
			int step_cost = (d >= 4) ? 14 : 10;
			int cand_g = g[u] + step_cost;
			if (stamps[v] != st || cand_g < g[v]) {
				stamps[v] = st;
				g[v] = cand_g;
				parent[v] = u;
				closed[v] = 0;
				s_astar_scratch.heap_push(v, cand_g + heuristic(vx, vy));
			}
		}
	}

	if (!found) {
		return false;
	}

	Vector<Vector2i> rev;
	int trace = goal_idx;
	while (trace != -1 && trace != start_idx) {
		rev.push_back(Vector2i(trace % w, trace / w));
		trace = parent[trace];
	}
	r_path.resize(rev.size());
	for (int i = 0; i < rev.size(); i++) {
		r_path.write[i] = rev[rev.size() - 1 - i];
	}
	return !r_path.is_empty();
}

// ============================================================================
// ClockGearsDetectionZone2D — Area2D con señales nativas de Godot
// ============================================================================
void ClockGearsDetectionZone2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_zone_name"), &ClockGearsDetectionZone2D::get_zone_name);
	ClassDB::bind_method(D_METHOD("get_target_filter"), &ClockGearsDetectionZone2D::get_target_filter);
	ClassDB::bind_method(D_METHOD("get_trigger"), &ClockGearsDetectionZone2D::get_trigger);
	ClassDB::bind_method(D_METHOD("get_interval_sec"), &ClockGearsDetectionZone2D::get_interval_sec);
	ClassDB::bind_method(D_METHOD("get_actions"), &ClockGearsDetectionZone2D::get_actions);
	ClassDB::bind_method(D_METHOD("get_inside_count"), &ClockGearsDetectionZone2D::get_inside_count);
}

ClockGearsDetectionZone2D::ClockGearsDetectionZone2D() {
	set_monitoring(true);
	set_monitorable(false);
	set_collision_layer(0);
	shape_owner_id = create_shape_owner(this);
	connect("body_entered", callable_mp(this, &ClockGearsDetectionZone2D::_on_body_entered));
	connect("body_exited", callable_mp(this, &ClockGearsDetectionZone2D::_on_body_exited));
}

void ClockGearsDetectionZone2D::configure(ClockGearsEvent *p_owner, int p_index, const Dictionary &p_zone_dict, int p_tile_size) {
	owner_event = p_owner;
	zone_index = p_index;
	zone_name = String(p_zone_dict.get("name", vformat("Zona %d", p_index + 1)));
	target_filter = String(p_zone_dict.get("target_filter", "player")).strip_edges().to_lower();
	trigger = String(p_zone_dict.get("trigger", "enter")).strip_edges().to_lower();
	interval_sec = MAX(0.05f, float(p_zone_dict.get("interval_sec", 0.5f)));
	Variant acts_var = p_zone_dict.get("actions", Array());
	actions = (acts_var.get_type() == Variant::ARRAY) ? Array(acts_var).duplicate(true) : Array();
	set_collision_mask(ClockGearsEntityNav::target_filter_to_mask(target_filter));
	set_name(vformat("DetectionZone_%d", p_index));

	ClockGearsHitShape s = ClockGearsShapeGeometry::shape_from_variant(p_zone_dict.get("shape", Dictionary()));
	if (!p_zone_dict.has("shape") || Dictionary(p_zone_dict["shape"]).is_empty()) {
		s = ClockGearsHitShape::make_circle(96.0f);
	}
	ClockGearsShapeGeometry::populate_collision_object_shapes(this, shape_owner_id, s, Vector2());
}

bool ClockGearsDetectionZone2D::_matches_target(Node *p_body, Node **r_entity_node) const {
	if (r_entity_node) {
		*r_entity_node = nullptr;
	}
	if (!p_body || !owner_event) {
		return false;
	}
	if (ClockGearsPlayer *pl = Object::cast_to<ClockGearsPlayer>(p_body)) {
		if (target_filter == "player" || target_filter == "all") {
			if (r_entity_node) *r_entity_node = pl;
			return true;
		}
		return false;
	}
	if (p_body->has_meta("cg_event_owner")) {
		ObjectID ev_oid = ObjectID(uint64_t(p_body->get_meta("cg_event_owner")));
		ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(ObjectDB::get_instance(ev_oid));
		if (!ev || ev == owner_event || !ev->is_visible()) {
			return false;
		}
		int bt = ev->get_behavior_type();
		bool is_enemy = (bt == 3);
		bool is_npc = (bt == 1 || bt == 2);
		if ((target_filter == "enemies" && is_enemy) ||
				(target_filter == "npc" && is_npc) ||
				(target_filter == "all" && (is_enemy || is_npc))) {
			if (r_entity_node) *r_entity_node = ev;
			return true;
		}
	}
	return false;
}

Node *ClockGearsDetectionZone2D::get_first_inside_entity() const {
	for (int i = 0; i < inside_targets.size(); i++) {
		Node *n = Object::cast_to<Node>(ObjectDB::get_instance(inside_targets[i]));
		if (n) {
			return n;
		}
	}
	return nullptr;
}

void ClockGearsDetectionZone2D::_on_body_entered(Node *p_body) {
	Node *target_entity = nullptr;
	if (!_matches_target(p_body, &target_entity) || !target_entity) {
		return;
	}
	ObjectID oid = target_entity->get_instance_id();
	if (!inside_targets.has(oid)) {
		inside_targets.push_back(oid);
	}
	if (owner_event) {
		owner_event->notify_zone_sight(true, target_entity);
		if (trigger == "enter") {
			owner_event->execute_zone_actions(actions, target_entity);
		} else if (trigger == "while_inside") {
			while_timer = 0.0f;
		}
	}
}

void ClockGearsDetectionZone2D::_on_body_exited(Node *p_body) {
	Node *target_entity = nullptr;
	if (!_matches_target(p_body, &target_entity) || !target_entity) {
		return;
	}
	ObjectID oid = target_entity->get_instance_id();
	inside_targets.erase(oid);
	if (owner_event) {
		owner_event->notify_zone_sight(false, target_entity);
		if (trigger == "exit") {
			owner_event->execute_zone_actions(actions, target_entity);
		}
	}
}

void ClockGearsDetectionZone2D::tick_zone(float p_delta) {
	if (inside_targets.is_empty()) {
		return;
	}
	// Limpiar referencias inválidas si algún nodo salió del árbol
	for (int i = inside_targets.size() - 1; i >= 0; i--) {
		Node *n = Object::cast_to<Node>(ObjectDB::get_instance(inside_targets[i]));
		if (!n || !n->is_inside_tree()) {
			inside_targets.remove_at(i);
		}
	}
	if (inside_targets.is_empty() || trigger != "while_inside" || !owner_event) {
		return;
	}
	while_timer -= p_delta;
	if (while_timer <= 0.0f) {
		while_timer = interval_sec;
		Node *target_entity = get_first_inside_entity();
		if (target_entity) {
			owner_event->execute_zone_actions(actions, target_entity);
		}
	}
}
