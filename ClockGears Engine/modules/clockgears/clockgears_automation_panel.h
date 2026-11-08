#pragma once

#include "clockgears_types.h"
#include "scene/gui/control.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/panel_container.h"

class ClockGearsAutomationPanel : public PanelContainer {
	GDCLASS(ClockGearsAutomationPanel, PanelContainer);

private:
	void _setup_input_map();
	PanelContainer *_create_preset_card(const String &p_title, const String &p_desc, const Callable &p_callback);

	void _apply_player_movement_2d(int p_directions);
	void _apply_camera_follower();
	void _apply_npc_wanderer();
	void _apply_door_teleport();
	void _apply_treasure_chest();
	void _apply_save_system();

protected:
	static void _bind_methods();

public:
	ClockGearsAutomationPanel();
};
