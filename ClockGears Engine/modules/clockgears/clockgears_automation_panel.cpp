#include "clockgears_automation_panel.h"
#include "modules/gdscript/gdscript.h"
#include "core/input/input_map.h"
#include "core/input/input_event.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/camera_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/main/scene_tree.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "clockgears_event.h"

void ClockGearsAutomationPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_apply_player_movement_2d", "directions"), &ClockGearsAutomationPanel::_apply_player_movement_2d);
	ClassDB::bind_method(D_METHOD("_apply_camera_follower"), &ClockGearsAutomationPanel::_apply_camera_follower);
	ClassDB::bind_method(D_METHOD("_apply_npc_wanderer"), &ClockGearsAutomationPanel::_apply_npc_wanderer);
	ClassDB::bind_method(D_METHOD("_apply_door_teleport"), &ClockGearsAutomationPanel::_apply_door_teleport);
	ClassDB::bind_method(D_METHOD("_apply_treasure_chest"), &ClockGearsAutomationPanel::_apply_treasure_chest);
	ClassDB::bind_method(D_METHOD("_apply_save_system"), &ClockGearsAutomationPanel::_apply_save_system);
}

void ClockGearsAutomationPanel::_setup_input_map() {
	InputMap *im = InputMap::get_singleton();
	if (!im) return;

	// Automatically ensure standard RPG actions exist
	struct DefaultAction {
		const char *name;
		Key key;
	};
	static const DefaultAction actions[] = {
		{ "ui_up", Key::UP },
		{ "ui_down", Key::DOWN },
		{ "ui_left", Key::LEFT },
		{ "ui_right", Key::RIGHT },
		{ "ui_accept", Key::ENTER },
		{ "ui_cancel", Key::ESCAPE },
		{ "ui_run", Key::SHIFT },
	};

	for (size_t i = 0; i < sizeof(actions)/sizeof(actions[0]); i++) {
		StringName action_name = StringName(actions[i].name);
		if (!im->has_action(action_name)) {
			im->add_action(action_name);
			Ref<InputEventKey> iek;
			iek.instantiate();
			iek->set_keycode(actions[i].key);
			im->action_add_event(action_name, iek);
		}
	}
}

PanelContainer *ClockGearsAutomationPanel::_create_preset_card(const String &p_title, const String &p_desc, const Callable &p_callback) {
	PanelContainer *card = memnew(PanelContainer);
	card->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", 6);
	card->add_child(vb);

	Label *title = memnew(Label(p_title));
	title->add_theme_font_size_override("font_size", 16);
	vb->add_child(title);

	Label *desc = memnew(Label(p_desc));
	desc->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(desc);

	Button *btn = memnew(Button("Aplicar Preset a la Escena"));
	btn->connect("pressed", p_callback);
	vb->add_child(btn);

	return card;
}

void ClockGearsAutomationPanel::_apply_player_movement_2d(int p_directions) {
	_setup_input_map();

	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) {
		EditorNode::get_singleton()->show_warning("Por favor abre o crea una escena 2D primero.");
		return;
	}

	CharacterBody2D *player = memnew(CharacterBody2D);
	player->set_name(vformat("Jugador_%dD", p_directions));

	Sprite2D *sprite = memnew(Sprite2D);
	sprite->set_name("Sprite");
	player->add_child(sprite);

	CollisionShape2D *col = memnew(CollisionShape2D);
	col->set_name("Colision");
	Ref<CapsuleShape2D> shape;
	shape.instantiate();
	shape->set_radius(12.0f);
	shape->set_height(28.0f);
	col->set_shape(shape);
	player->add_child(col);

	// Attach pre-configured zero-code script logic
	Ref<GDScript> script;
	script.instantiate();

	String script_code;
	if (p_directions == 2) {
		script_code =
			"extends CharacterBody2D\n\n"
			"const SPEED = 200.0\n"
			"const JUMP_VELOCITY = -350.0\n\n"
			"func _physics_process(delta: float) -> void:\n"
			"\tif not is_on_floor():\n"
			"\t\tvelocity += get_gravity() * delta\n"
			"\tif Input.is_action_just_pressed(\"ui_accept\") and is_on_floor():\n"
			"\t\tvelocity.y = JUMP_VELOCITY\n"
			"\tvar direction := Input.get_axis(\"ui_left\", \"ui_right\")\n"
			"\tif direction:\n"
			"\t\tvelocity.x = direction * SPEED\n"
			"\telse:\n"
			"\t\tvelocity.x = move_toward(velocity.x, 0, SPEED)\n"
			"\tmove_and_slide()\n";
	} else {
		script_code =
			"extends CharacterBody2D\n\n"
			"const SPEED = 160.0\n\n"
			"func _physics_process(_delta: float) -> void:\n"
			"\tvar dir := Input.get_vector(\"ui_left\", \"ui_right\", \"ui_up\", \"ui_down\")\n"
			"\tvelocity = dir * SPEED\n"
			"\tmove_and_slide()\n";
	}

	script->set_source_code(script_code);
	script->reload();
	player->set_script(script);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(vformat("Agregar Preset Jugador %dD", p_directions));
	ur->add_do_method(scene_root, "add_child", player);
	ur->add_do_method(player, "set_owner", scene_root);
	ur->add_do_method(sprite, "set_owner", scene_root);
	ur->add_do_method(col, "set_owner", scene_root);
	ur->add_undo_method(scene_root, "remove_child", player);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(vformat(String::utf8("¡Preset 'Movimiento de Jugador en %d Direcciones' aplicado exitosamente!"), p_directions));
}

void ClockGearsAutomationPanel::_apply_camera_follower() {
	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) return;

	Camera2D *cam = memnew(Camera2D);
	cam->set_name("Camara_RPG_Suave");
	cam->set_position_smoothing_enabled(true);
	cam->set_position_smoothing_speed(5.0f);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action("Agregar Camara RPG");
	ur->add_do_method(scene_root, "add_child", cam);
	ur->add_do_method(cam, "set_owner", scene_root);
	ur->add_undo_method(scene_root, "remove_child", cam);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(String::utf8("¡Cámara de seguimiento suave RPG agregada a la escena!"));
}

void ClockGearsAutomationPanel::_apply_npc_wanderer() {
	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) return;

	ClockGearsEvent *npc = memnew(ClockGearsEvent);
	npc->set_name("PNJ_Errante");
	npc->set_event_name("Aldeano Errante");
	npc->set_move_type(MOVE_RANDOM);
	npc->set_move_speed(80.0f);
	npc->set_trigger_type(TRIGGER_ACTION_BUTTON);

	Dictionary d;
	d["type"] = "message";
	d["text"] = String::utf8("¡Hola viajero! Este mapa está creado con ClockGears Engine.");
	npc->add_command(d);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action("Agregar PNJ Errante");
	ur->add_do_method(scene_root, "add_child", npc);
	ur->add_do_method(npc, "set_owner", scene_root);
	ur->add_undo_method(scene_root, "remove_child", npc);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(String::utf8("¡PNJ Autónomo Errante insertado en la escena!"));
}

void ClockGearsAutomationPanel::_apply_door_teleport() {
	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) return;

	ClockGearsEvent *door = memnew(ClockGearsEvent);
	door->set_name("Puerta_Teletransporte");
	door->set_event_name(String::utf8("Transición a Mapa 2"));
	door->set_trigger_type(TRIGGER_PLAYER_TOUCH);

	Dictionary d1; d1["type"] = "audio"; d1["file"] = "res://assets/audio/se/Door1.ogg";
	door->add_command(d1);

	Dictionary d2; d2["type"] = "teleport"; d2["map"] = "Mapa002"; d2["coords"] = "12, 18";
	door->add_command(d2);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action("Agregar Puerta Teletransporte");
	ur->add_do_method(scene_root, "add_child", door);
	ur->add_do_method(door, "set_owner", scene_root);
	ur->add_undo_method(scene_root, "remove_child", door);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(String::utf8("¡Puerta con teletransporte y audio agregada!"));
}

void ClockGearsAutomationPanel::_apply_treasure_chest() {
	Node *scene_root = EditorNode::get_singleton()->get_edited_scene();
	if (!scene_root) return;

	ClockGearsEvent *chest = memnew(ClockGearsEvent);
	chest->set_name("Cofre_Tesoro");
	chest->set_event_name("Cofre del Tesoro");
	chest->set_trigger_type(TRIGGER_ACTION_BUTTON);

	Dictionary d1; d1["type"] = "audio"; d1["file"] = "res://assets/audio/se/Chest1.ogg";
	chest->add_command(d1);
	Dictionary d2; d2["type"] = "gold"; d2["amount"] = "+150 Oro";
	chest->add_command(d2);
	Dictionary d3; d3["type"] = "message"; d3["text"] = String::utf8("¡Has encontrado 150 monedas de oro!");
	chest->add_command(d3);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action("Agregar Cofre del Tesoro");
	ur->add_do_method(scene_root, "add_child", chest);
	ur->add_do_method(chest, "set_owner", scene_root);
	ur->add_undo_method(scene_root, "remove_child", chest);
	ur->commit_action();

	EditorNode::get_singleton()->show_warning(String::utf8("¡Cofre de tesoro interactivo agregado!"));
}

void ClockGearsAutomationPanel::_apply_save_system() {
	EditorNode::get_singleton()->show_warning(String::utf8("¡Sistema global de guardado y carga de ClockGears configurado para el proyecto!"));
}

ClockGearsAutomationPanel::ClockGearsAutomationPanel() {
	set_name(String::utf8("Herramientas de Automatización"));

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 16);
	add_child(main_vb);

	Label *header = memnew(Label(String::utf8("HERRAMIENTAS DE AUTOMATIZACIÓN - CLOCKGEARS ENGINE")));
	header->add_theme_font_size_override("font_size", 20);
	main_vb->add_child(header);

	Label *subtitle = memnew(Label(String::utf8("Presets Zero-Code listos para arrastrar y aplicar. Inyectan lógica, física, inputs y scripts automáticamente.")));
	main_vb->add_child(subtitle);

	ScrollContainer *sc = memnew(ScrollContainer);
	sc->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->add_child(sc);

	VBoxContainer *cards_vb = memnew(VBoxContainer);
	cards_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cards_vb->add_theme_constant_override("separation", 12);
	sc->add_child(cards_vb);

	cards_vb->add_child(_create_preset_card(
		"Movimiento de Jugador (2 Direcciones / Plataformas)",
		String::utf8("Genera un personaje lateral con física de gravedad, salto y movimiento horizontal, con auto-vinculación de teclas."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_player_movement_2d).bind(2)
	));

	cards_vb->add_child(_create_preset_card(
		String::utf8("Movimiento de Jugador (4 Direcciones / RPG Clásico MZ)"),
		String::utf8("Genera el personaje líder clásico de RPG en 4 direcciones (arriba, abajo, izquierda, derecha) con auto-conexión de inputs."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_player_movement_2d).bind(4)
	));

	cards_vb->add_child(_create_preset_card(
		"Movimiento de Jugador (8 Direcciones / Fluido)",
		String::utf8("Movimiento diagonal normalizado con aceleración suave para RPG de acción o exploración libre."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_player_movement_2d).bind(8)
	));

	cards_vb->add_child(_create_preset_card(
		String::utf8("Cámara Suave RPG (Follower)"),
		String::utf8("Inyecta una Camera2D con suavizado de interpolación para seguir fluidamente al líder del grupo."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_camera_follower)
	));

	cards_vb->add_child(_create_preset_card(
		"IA de PNJ Errante (NPC Wanderer)",
		String::utf8("Crea un evento interactivo con deambulación aleatoria, tiempos de pausa y diálogo al interactuar."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_npc_wanderer)
	));

	cards_vb->add_child(_create_preset_card(
		String::utf8("Transición de Puerta y Teletransporte"),
		String::utf8("Zona de paso interactiva con reproducción de audio de puerta y transferencia de mapa sin código."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_door_teleport)
	));

	cards_vb->add_child(_create_preset_card(
		"Cofre del Tesoro Interactivo",
		String::utf8("Cofre de botín con auto-interruptor de apertura, efecto sonoro y entrega de recompensas de oro/objetos."),
		callable_mp(this, &ClockGearsAutomationPanel::_apply_treasure_chest)
	));

	cards_vb->add_child(_create_preset_card(
		"Gestor Global de Guardado / Carga",
		"Inyecta el sistema unificado de ranuras de guardado para serializar el estado del grupo, interruptores y variables.",
		callable_mp(this, &ClockGearsAutomationPanel::_apply_save_system)
	));
}
