#pragma once

#include "clockgears_types.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/progress_bar.h"

class ClockGearsSimplifiedExportDialog : public ConfirmationDialog {
	GDCLASS(ClockGearsSimplifiedExportDialog, ConfirmationDialog);

private:
	ProgressBar *progress_bar = nullptr;
	Label *status_label = nullptr;

	void _export_pc();
	void _export_android();
	void _export_consoles();

protected:
	static void _bind_methods();

public:
	void popup_simplified_export();

	ClockGearsSimplifiedExportDialog();
};
