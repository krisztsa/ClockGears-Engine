#pragma once

#include "clockgears_types.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/button.h"

class ClockGearsCommandSelector : public AcceptDialog {
	GDCLASS(ClockGearsCommandSelector, AcceptDialog);

private:
	TabContainer *tabs = nullptr;

	Button *_create_cmd_button(GridContainer *p_parent, const String &p_text, int p_cmd_id);
	void _on_btn_clicked(int p_cmd_id);

protected:
	static void _bind_methods();

public:
	void popup_catalog();

	ClockGearsCommandSelector();
};
