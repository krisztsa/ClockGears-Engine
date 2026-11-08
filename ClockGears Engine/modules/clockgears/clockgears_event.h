#pragma once

#include "clockgears_types.h"
#include "core/variant/array.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"

class ClockGearsEvent : public Node2D {
	GDCLASS(ClockGearsEvent, Node2D);

private:
	int event_id = 1;
	String event_name = "EV001";
	int trigger_type = TRIGGER_ACTION_BUTTON;
	int move_type = MOVE_FIXED;
	float move_speed = 100.0f;
	float move_frequency = 1.0f;
	bool through = false;
	bool direction_fix = false;
	int direction = 2; // Down
	String graphic_path;
	int sprite_hframes = 1;
	int sprite_vframes = 1;
	int sprite_frame = 0;
	Array commands;
	Array pages;

	bool condition_switch_1_enabled = false;
	String condition_switch_1_name;
	bool condition_switch_2_enabled = false;
	String condition_switch_2_name;
	bool condition_self_switch_enabled = false;
	String condition_self_switch_letter = "A";
	Dictionary self_switches;
	int active_page_index = 0;
	bool condition_var_enabled = false;
	String condition_var_name;
	int condition_var_value = 0;
	bool condition_item_enabled = false;
	String condition_item_name;
	bool condition_actor_enabled = false;
	String condition_actor_name;
	int priority_type = 1; // 0: Below, 1: Same, 2: Above

	Sprite2D *sprite = nullptr;
	void _update_sprite_region();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_event_id(int p_id) { event_id = p_id; }
	int get_event_id() const { return event_id; }

	void set_event_name(const String &p_name) { event_name = p_name; }
	String get_event_name() const { return event_name; }

	void set_trigger_type(int p_trigger) { trigger_type = p_trigger; }
	int get_trigger_type() const { return trigger_type; }

	void set_move_type(int p_move) { move_type = p_move; }
	int get_move_type() const { return move_type; }

	void set_move_speed(float p_speed) { move_speed = p_speed; }
	float get_move_speed() const { return move_speed; }

	void set_move_frequency(float p_freq) { move_frequency = p_freq; }
	float get_move_frequency() const { return move_frequency; }

	void set_through(bool p_through) { through = p_through; }
	bool is_through() const { return through; }

	void set_direction_fix(bool p_fix) { direction_fix = p_fix; }
	bool is_direction_fix() const { return direction_fix; }

	void set_direction(int p_dir) { direction = p_dir; }
	int get_direction() const { return direction; }

	void set_graphic_path(const String &p_path);
	String get_graphic_path() const { return graphic_path; }

	void set_sprite_hframes(int p_hframes);
	int get_sprite_hframes() const { return sprite_hframes; }

	void set_sprite_vframes(int p_vframes);
	int get_sprite_vframes() const { return sprite_vframes; }

	void set_sprite_frame(int p_frame);
	int get_sprite_frame() const { return sprite_frame; }

	void set_condition_switch_1_enabled(bool p_en) { condition_switch_1_enabled = p_en; }
	bool is_condition_switch_1_enabled() const { return condition_switch_1_enabled; }
	void set_condition_switch_1_name(const String &p_name) { condition_switch_1_name = p_name; }
	String get_condition_switch_1_name() const { return condition_switch_1_name; }

	void set_condition_switch_2_enabled(bool p_en) { condition_switch_2_enabled = p_en; }
	bool is_condition_switch_2_enabled() const { return condition_switch_2_enabled; }
	void set_condition_switch_2_name(const String &p_name) { condition_switch_2_name = p_name; }
	String get_condition_switch_2_name() const { return condition_switch_2_name; }

	void set_condition_self_switch_enabled(bool p_en) { condition_self_switch_enabled = p_en; }
	bool is_condition_self_switch_enabled() const { return condition_self_switch_enabled; }
	void set_condition_self_switch_letter(const String &p_letter) { condition_self_switch_letter = p_letter; }
	String get_condition_self_switch_letter() const { return condition_self_switch_letter; }

	void set_self_switch(const String &p_letter, bool p_val);
	bool get_self_switch(const String &p_letter) const;
	Dictionary get_self_switches() const { return self_switches; }
	void set_self_switches(const Dictionary &p_sw);

	int get_active_page_index() const { return active_page_index; }
	void update_active_page();

	void set_condition_var_enabled(bool p_en) { condition_var_enabled = p_en; }
	bool is_condition_var_enabled() const { return condition_var_enabled; }
	void set_condition_var_name(const String &p_name) { condition_var_name = p_name; }
	String get_condition_var_name() const { return condition_var_name; }
	void set_condition_var_value(int p_val) { condition_var_value = p_val; }
	int get_condition_var_value() const { return condition_var_value; }

	void set_condition_item_enabled(bool p_en) { condition_item_enabled = p_en; }
	bool is_condition_item_enabled() const { return condition_item_enabled; }
	void set_condition_item_name(const String &p_name) { condition_item_name = p_name; }
	String get_condition_item_name() const { return condition_item_name; }

	void set_condition_actor_enabled(bool p_en) { condition_actor_enabled = p_en; }
	bool is_condition_actor_enabled() const { return condition_actor_enabled; }
	void set_condition_actor_name(const String &p_name) { condition_actor_name = p_name; }
	String get_condition_actor_name() const { return condition_actor_name; }

	void set_priority_type(int p_priority) { priority_type = p_priority; }
	int get_priority_type() const { return priority_type; }

	void set_commands(const Array &p_commands) { commands = p_commands; }
	Array get_commands() const { return commands; }

	void set_pages(const Array &p_pages) { pages = p_pages; }
	Array get_pages() const { return pages; }

	void add_command(const Dictionary &p_cmd);
	void clear_commands() { commands.clear(); }

	void serialize_to_meta();
	void deserialize_from_meta();

	void execute();

	ClockGearsEvent();
};
