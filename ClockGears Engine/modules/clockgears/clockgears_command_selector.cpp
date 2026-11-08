#include "clockgears_command_selector.h"
#include "editor/clockgears_theme.h"
#include "editor/themes/editor_scale.h"
#include "icons/clockgears_icons.h"
#include "scene/gui/box_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/resources/style_box_flat.h"

void ClockGearsCommandSelector::_bind_methods() {
	ADD_SIGNAL(MethodInfo("command_selected", PropertyInfo(Variant::INT, "command_id")));
	BIND_CONSTANT(CMD_SHOW_PICTURE);
	BIND_CONSTANT(CMD_MOVE_PICTURE);
	BIND_CONSTANT(CMD_ROTATE_PICTURE);
	BIND_CONSTANT(CMD_TINT_PICTURE);
	BIND_CONSTANT(CMD_ERASE_PICTURE);
	BIND_CONSTANT(CMD_SHAKE_SCREEN);
	BIND_CONSTANT(CMD_WEATHER);
	BIND_CONSTANT(CMD_CHANGE_CAMERA_ZOOM);
	BIND_CONSTANT(CMD_PLAY_BGS);
	BIND_CONSTANT(CMD_PLAY_ME);
	BIND_CONSTANT(CMD_STOP_FADE_BGM);
	BIND_CONSTANT(CMD_SAVE_BGM);
	BIND_CONSTANT(CMD_RESTORE_BGM);
	BIND_CONSTANT(CMD_AUDIO_STEMS);
	BIND_CONSTANT(CMD_PLAY_CUTSCENE);
	BIND_CONSTANT(CMD_CREATE_CUTSCENE);
	BIND_CONSTANT(CMD_BATTLE_PROCESSING);
}

Button *ClockGearsCommandSelector::_create_cmd_button(GridContainer *p_parent, const String &p_text, int p_cmd_id) {
	Button *btn = memnew(Button(p_text));
	btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn->set_custom_minimum_size(Size2(220, 42) * EDSCALE);
	btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	btn->connect("pressed", callable_mp(this, &ClockGearsCommandSelector::_on_btn_clicked).bind(p_cmd_id));
	p_parent->add_child(btn);
	return btn;
}

void ClockGearsCommandSelector::_on_btn_clicked(int p_cmd_id) {
	hide();
	emit_signal("command_selected", p_cmd_id);
}

void ClockGearsCommandSelector::popup_catalog() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	popup_centered(Size2(620, 500) * EDSCALE);
}

ClockGearsCommandSelector::ClockGearsCommandSelector() {
	set_title(CG_TR("Catálogo de Comandos de Evento - RPG Maker Paradigm"));
	set_min_size(Size2(580, 460) * EDSCALE);
	set_hide_on_ok(true);

	tabs = memnew(TabContainer);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(tabs);

	// =============================================================
	// Pestaña 1: Mensajes y Control
	// =============================================================
	MarginContainer *tab1_margin = memnew(MarginContainer);
	tab1_margin->set_name(CG_TR("1. Mensajes y Control"));
	tab1_margin->add_theme_constant_override("margin_left", 8);
	tab1_margin->add_theme_constant_override("margin_top", 8);
	tab1_margin->add_theme_constant_override("margin_right", 8);
	tab1_margin->add_theme_constant_override("margin_bottom", 8);
	tabs->add_child(tab1_margin);

	GridContainer *grid1 = memnew(GridContainer);
	grid1->set_columns(2);
	grid1->add_theme_constant_override("h_separation", 10);
	grid1->add_theme_constant_override("v_separation", 10);
	tab1_margin->add_child(grid1);

	_create_cmd_button(grid1, CG_TR("◆ Mostrar Mensaje..."), CMD_SHOW_MESSAGE);
	_create_cmd_button(grid1, CG_TR("◆ Mostrar Elecciones..."), CMD_SHOW_CHOICES);
	_create_cmd_button(grid1, CG_TR("◆ Control Interruptores..."), CMD_CONTROL_SWITCH);
	_create_cmd_button(grid1, CG_TR("◆ Control Variables..."), CMD_CONTROL_VARIABLE);
	_create_cmd_button(grid1, CG_TR("◆ Control Interruptor Local..."), CMD_CONTROL_SELF_SWITCH);
	_create_cmd_button(grid1, CG_TR("◆ Esperar (Frames)..."), CMD_WAIT_FRAMES);
	_create_cmd_button(grid1, CG_TR("◆ Entrada de Número..."), CMD_INPUT_NUMBER);

	// =============================================================
	// Pestaña 2: Movimiento y Personaje
	// =============================================================
	MarginContainer *tab2_margin = memnew(MarginContainer);
	tab2_margin->set_name(CG_TR("2. Movimiento y Personaje"));
	tab2_margin->add_theme_constant_override("margin_left", 8);
	tab2_margin->add_theme_constant_override("margin_top", 8);
	tab2_margin->add_theme_constant_override("margin_right", 8);
	tab2_margin->add_theme_constant_override("margin_bottom", 8);
	tabs->add_child(tab2_margin);

	GridContainer *grid2 = memnew(GridContainer);
	grid2->set_columns(2);
	grid2->add_theme_constant_override("h_separation", 10);
	grid2->add_theme_constant_override("v_separation", 10);
	tab2_margin->add_child(grid2);

	_create_cmd_button(grid2, CG_TR("◆ Transferir Jugador..."), CMD_TRANSFER_PLAYER);
	_create_cmd_button(grid2, CG_TR("◆ Fijar Ruta Movimiento..."), CMD_SET_MOVE_ROUTE);
	_create_cmd_button(grid2, CG_TR("◆ Añadir Objeto..."), CMD_ADD_ITEM);
	_create_cmd_button(grid2, CG_TR("◆ Cambiar Dinero..."), CMD_CHANGE_GOLD);
	_create_cmd_button(grid2, CG_TR("◆ Cambiar HP/MP..."), CMD_CHANGE_HP_MP);

	// =============================================================
	// Pestaña 3: Escena y Multimedia
	// =============================================================
	MarginContainer *tab3_margin = memnew(MarginContainer);
	tab3_margin->set_name(CG_TR("3. Escena y Multimedia"));
	tab3_margin->add_theme_constant_override("margin_left", 8);
	tab3_margin->add_theme_constant_override("margin_top", 8);
	tab3_margin->add_theme_constant_override("margin_right", 8);
	tab3_margin->add_theme_constant_override("margin_bottom", 8);
	tabs->add_child(tab3_margin);

	ScrollContainer *tab3_scroll = memnew(ScrollContainer);
	tab3_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tab3_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab3_margin->add_child(tab3_scroll);

	GridContainer *grid3 = memnew(GridContainer);
	grid3->set_columns(2);
	grid3->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	grid3->add_theme_constant_override("h_separation", 10);
	grid3->add_theme_constant_override("v_separation", 8);
	tab3_scroll->add_child(grid3);

	// Imágenes (Anexo A1.3)
	_create_cmd_button(grid3, CG_TR("◆ Mostrar Imagen..."), CMD_SHOW_PICTURE);
	_create_cmd_button(grid3, CG_TR("◆ Mover Imagen..."), CMD_MOVE_PICTURE);
	_create_cmd_button(grid3, CG_TR("◆ Rotar Imagen..."), CMD_ROTATE_PICTURE);
	_create_cmd_button(grid3, CG_TR("◆ Teñir Imagen..."), CMD_TINT_PICTURE);
	_create_cmd_button(grid3, CG_TR("◆ Borrar Imagen..."), CMD_ERASE_PICTURE);

	// Pantalla (Anexo A1.3)
	_create_cmd_button(grid3, CG_TR("◆ Cambiar Zoom de Cámara..."), CMD_CHANGE_CAMERA_ZOOM);
	_create_cmd_button(grid3, CG_TR("◆ Temblor de Pantalla..."), CMD_SHAKE_SCREEN);
	_create_cmd_button(grid3, CG_TR("◆ Fijar Clima..."), CMD_WEATHER);
	_create_cmd_button(grid3, CG_TR("◆ Teñir Pantalla..."), CMD_TINT_SCREEN);
	_create_cmd_button(grid3, CG_TR("◆ Fundido de Pantalla..."), CMD_FADE_SCREEN);
	_create_cmd_button(grid3, CG_TR("◆ Efecto de Flash..."), CMD_FLASH_EFFECT);

	// Audio (Anexo A1.4)
	_create_cmd_button(grid3, CG_TR("◆ Reproducir BGM..."), CMD_PLAY_BGM);
	_create_cmd_button(grid3, CG_TR("◆ Parar / Fundir BGM..."), CMD_STOP_FADE_BGM);
	_create_cmd_button(grid3, CG_TR("◆ Guardar BGM"), CMD_SAVE_BGM);
	_create_cmd_button(grid3, CG_TR("◆ Restaurar BGM"), CMD_RESTORE_BGM);
	_create_cmd_button(grid3, CG_TR("◆ Reproducir BGS..."), CMD_PLAY_BGS);
	_create_cmd_button(grid3, CG_TR("◆ Reproducir ME..."), CMD_PLAY_ME);
	_create_cmd_button(grid3, CG_TR("◆ Reproducir SE..."), CMD_PLAY_SE);
	_create_cmd_button(grid3, CG_TR("◆ Stems de Audio..."), CMD_AUDIO_STEMS);

	// Cinemática & Batalla (Anexo A1.2)
	_create_cmd_button(grid3, CG_TR("◆ Reproducir Cinemática..."), CMD_PLAY_CUTSCENE);
	_create_cmd_button(grid3, CG_TR("◆ Crear Cinemática..."), CMD_CREATE_CUTSCENE);
	_create_cmd_button(grid3, CG_TR("◆ Iniciar Batalla..."), CMD_BATTLE_PROCESSING);
}
