#include "clockgears_system_catalog.h"

ClockGearsSystemCatalog *ClockGearsSystemCatalog::singleton = nullptr;

static Dictionary _make_def(int p_code, const char *p_name, const char *p_category, const char *p_desc = "") {
	Dictionary d;
	d["code"] = p_code;
	d["name"] = String::utf8(p_name);
	d["category"] = String::utf8(p_category);
	d["description"] = String::utf8(p_desc);
	return d;
}

void ClockGearsSystemCatalog::_init_system_defaults() {
	// Elementos base del sistema (estructura editable en la pestaña "Tipos", igual que RPG Maker MZ)
	elements.clear();
	const char *default_elements[] = {
		"Físico", "Fuego", "Hielo", "Rayo", "Agua", "Tierra", "Viento", "Luz", "Oscuridad"
	};
	for (size_t i = 0; i < sizeof(default_elements) / sizeof(default_elements[0]); i++) {
		elements.push_back(String::utf8(default_elements[i]));
	}

	// Tipos de arma del sistema
	weapon_types.clear();
	const char *default_weapons[] = {
		"Daga", "Espada", "Mandoble", "Hacha", "Lanza", "Arco", "Bastón", "Arma de Fuego", "Garras", "Guantelete", "Látigo"
	};
	for (size_t i = 0; i < sizeof(default_weapons) / sizeof(default_weapons[0]); i++) {
		weapon_types.push_back(String::utf8(default_weapons[i]));
	}

	// Tipos de armadura del sistema
	armor_types.clear();
	const char *default_armors[] = {
		"Armadura General", "Armadura Mágica", "Armadura Ligera", "Armadura Pesada", "Escudo Pequeño", "Escudo Grande"
	};
	for (size_t i = 0; i < sizeof(default_armors) / sizeof(default_armors[0]); i++) {
		armor_types.push_back(String::utf8(default_armors[i]));
	}

	// Ranuras de equipamiento
	equip_slots.clear();
	const char *default_slots[] = {
		"Mano Derecha (Arma)", "Mano Izquierda (Escudo)", "Cabeza (Casco)", "Cuerpo (Pechera)", "Accesorio"
	};
	for (size_t i = 0; i < sizeof(default_slots) / sizeof(default_slots[0]); i++) {
		equip_slots.push_back(String::utf8(default_slots[i]));
	}

	// Ámbitos de alcance (Scopes)
	scopes.clear();
	const char *default_scopes[] = {
		"Ninguno", "Un Enemigo", "Todos los Enemigos", "1 Enemigo Aleatorio", "2 Enemigos Aleatorios",
		"3 Enemigos Aleatorios", "4 Enemigos Aleatorios", "Un Aliado", "Todos los Aliados",
		"Un Aliado (K.O.)", "Todos los Aliados (K.O.)", "El Usuario", "Todos (Aliados y Enemigos)"
	};
	for (size_t i = 0; i < sizeof(default_scopes) / sizeof(default_scopes[0]); i++) {
		scopes.push_back(String::utf8(default_scopes[i]));
	}

	// Ocasiones de uso
	occasions.clear();
	const char *default_occasions[] = {
		"Siempre", "Solo en Batalla", "Solo en el Menú", "Nunca"
	};
	for (size_t i = 0; i < sizeof(default_occasions) / sizeof(default_occasions[0]); i++) {
		occasions.push_back(String::utf8(default_occasions[i]));
	}

	// Disparadores de evento
	triggers.clear();
	const char *default_triggers[] = {
		"Botón de Acción", "Contacto con Jugador", "Contacto con Evento", "Ejecución Automática", "Proceso Paralelo"
	};
	for (size_t i = 0; i < sizeof(default_triggers) / sizeof(default_triggers[0]); i++) {
		triggers.push_back(String::utf8(default_triggers[i]));
	}

	// Tipos de movimiento autónomo
	move_types.clear();
	const char *default_moves[] = {
		"Fijo", "Aleatorio", "Acercarse", "Alejarse", "Ruta Personalizada"
	};
	for (size_t i = 0; i < sizeof(default_moves) / sizeof(default_moves[0]); i++) {
		move_types.push_back(String::utf8(default_moves[i]));
	}

	// 8 Parámetros Base (0..7)
	base_param_names.clear();
	const char *default_bparams[] = {
		"Max HP", "Max MP", "ATK", "DEF", "MAT", "MDF", "AGI", "LUK"
	};
	for (size_t i = 0; i < 8; i++) {
		base_param_names.push_back(String::utf8(default_bparams[i]));
	}

	// 10 Parámetros Extra (XParam 0..9)
	ex_param_names.clear();
	const char *default_xparams[] = {
		"HIT (Precisión)", "EVA (Evasión)", "CRI (Crítico)", "CEV (Evasión Crítico)",
		"MEV (Evasión Mágica)", "MRF (Reflejo Mágico)", "CNT (Contraataque)",
		"HRG (Regen HP)", "MRG (Regen MP)", "TRG (Regen TP)"
	};
	for (size_t i = 0; i < 10; i++) {
		ex_param_names.push_back(String::utf8(default_xparams[i]));
	}

	// 10 Parámetros Especiales (SParam 0..9)
	sp_param_names.clear();
	const char *default_sparams[] = {
		"TGR (Tasa de Objetivo)", "GRD (Efecto Guardia)", "REC (Efecto Recuperación)",
		"PHA (Farmacología)", "MCR (Costo MP)", "TCR (Carga TP)",
		"PDR (Daño Físico Recibido)", "MDR (Daño Mágico Recibido)",
		"FDR (Daño de Suelo)", "EXR (Tasa de EXP)"
	};
	for (size_t i = 0; i < 10; i++) {
		sp_param_names.push_back(String::utf8(default_sparams[i]));
	}

	// Catálogo de Rasgos (Traits) - Sección 4.2
	trait_definitions.clear();
	trait_definitions.push_back(_make_def(CG_TRAIT_ELEMENT_RATE, "Tasa Elemental (%)", "Resistencia"));
	trait_definitions.push_back(_make_def(CG_TRAIT_DEBUFF_RATE, "Tasa de Debuff (%)", "Resistencia"));
	trait_definitions.push_back(_make_def(CG_TRAIT_STATE_RATE, "Tasa de Estado (%)", "Resistencia"));
	trait_definitions.push_back(_make_def(CG_TRAIT_STATE_RESIST, "Inmunidad a Estado", "Resistencia"));
	trait_definitions.push_back(_make_def(CG_TRAIT_PARAM, "Parámetro Base (+/×)", "Parámetros"));
	trait_definitions.push_back(_make_def(CG_TRAIT_XPARAM, "Parámetro Extra (XParam)", "Parámetros"));
	trait_definitions.push_back(_make_def(CG_TRAIT_SPARAM, "Parámetro Especial (SParam)", "Parámetros"));
	trait_definitions.push_back(_make_def(CG_TRAIT_ATTACK_ELEMENT, "Elemento de Ataque", "Ataque"));
	trait_definitions.push_back(_make_def(CG_TRAIT_ATTACK_STATE, "Estado de Ataque", "Ataque"));
	trait_definitions.push_back(_make_def(CG_TRAIT_ATTACK_SPEED, "Velocidad de Ataque", "Ataque"));
	trait_definitions.push_back(_make_def(CG_TRAIT_ATTACK_TIMES, "Ataques Extra", "Ataque"));
	trait_definitions.push_back(_make_def(CG_TRAIT_STYPE_ADD, "Añadir Tipo de Habilidad", "Habilidades"));
	trait_definitions.push_back(_make_def(CG_TRAIT_STYPE_SEAL, "Sellar Tipo de Habilidad", "Habilidades"));
	trait_definitions.push_back(_make_def(CG_TRAIT_SKILL_ADD, "Habilitar Habilidad", "Habilidades"));
	trait_definitions.push_back(_make_def(CG_TRAIT_SKILL_SEAL, "Sellar Habilidad", "Habilidades"));
	trait_definitions.push_back(_make_def(CG_TRAIT_EQUIP_WTYPE, "Equipar Tipo de Arma", "Equipo"));
	trait_definitions.push_back(_make_def(CG_TRAIT_EQUIP_ATYPE, "Equipar Tipo de Armadura", "Equipo"));
	trait_definitions.push_back(_make_def(CG_TRAIT_EQUIP_LOCK, "Bloquear Equipamiento", "Equipo"));
	trait_definitions.push_back(_make_def(CG_TRAIT_EQUIP_SEAL, "Sellar Ranura", "Equipo"));
	trait_definitions.push_back(_make_def(CG_TRAIT_SLOT_TYPE, "Tipo de Ranura (Doble Empuñadura)", "Equipo"));
	trait_definitions.push_back(_make_def(CG_TRAIT_ACTION_PLUS, "Acción Extra (%)", "Otros"));
	trait_definitions.push_back(_make_def(CG_TRAIT_SPECIAL_FLAG, "Bandera Especial (Autobatalla/Guardia/Sustituto/Conservar TP)", "Otros"));
	trait_definitions.push_back(_make_def(CG_TRAIT_PARTY_ABILITY, "Habilidad de Grupo (EXP/Oro/Dropx2)", "Otros"));

	// Catálogo de Efectos - Sección 4.3
	effect_definitions.clear();
	effect_definitions.push_back(_make_def(CG_EFFECT_RECOVER_HP, "Recuperar PV (HP % + Fijo)", "Recuperación"));
	effect_definitions.push_back(_make_def(CG_EFFECT_RECOVER_MP, "Recuperar PM (MP % + Fijo)", "Recuperación"));
	effect_definitions.push_back(_make_def(CG_EFFECT_GAIN_TP, "Ganar PT (TP)", "Recuperación"));
	effect_definitions.push_back(_make_def(CG_EFFECT_ADD_STATE, "Añadir Estado", "Estado"));
	effect_definitions.push_back(_make_def(CG_EFFECT_REMOVE_STATE, "Quitar Estado", "Estado"));
	effect_definitions.push_back(_make_def(CG_EFFECT_ADD_BUFF, "Añadir Buff", "Parámetros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_ADD_DEBUFF, "Añadir Debuff", "Parámetros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_REMOVE_BUFF, "Quitar Buff", "Parámetros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_REMOVE_DEBUFF, "Quitar Debuff", "Parámetros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_SPECIAL_ESCAPE, "Efecto Especial: Escapar", "Otros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_GROW_PARAM, "Crecimiento Permanente de Parámetro", "Otros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_LEARN_SKILL, "Aprender Habilidad", "Otros"));
	effect_definitions.push_back(_make_def(CG_EFFECT_COMMON_EVENT, "Ejecutar Evento Común", "Otros"));

	// Condiciones de IA de Enemigo - Sección 5.3 / 9.7
	ai_condition_definitions.clear();
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_ALWAYS, "Siempre", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_TURN, "Turno (A + B * X)", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_HP_RANGE, "PV (HP %) entre A% y B%", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_MP_RANGE, "PM (MP %) entre A% y B%", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_STATE, "Estado Activo", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_PARTY_LEVEL, "Nivel del Grupo >= X", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_SWITCH, "Interruptor ON", "IA"));

	// Catálogo de Comandos de Evento - Sección 3.5
	command_definitions.clear();
	// Pestaña 1
	command_definitions.push_back(_make_def(CG_CMD_SHOW_TEXT, "Mostrar Texto...", "Mensajes"));
	command_definitions.push_back(_make_def(CG_CMD_SHOW_CHOICES, "Mostrar Opciones...", "Mensajes"));
	command_definitions.push_back(_make_def(CG_CMD_INPUT_NUMBER, "Entrada de Número...", "Mensajes"));
	command_definitions.push_back(_make_def(CG_CMD_SELECT_KEY_ITEM, "Seleccionar Objeto Clave...", "Mensajes"));
	command_definitions.push_back(_make_def(CG_CMD_SHOW_SCROLLING_TEXT, "Mostrar Texto Desplazable...", "Mensajes"));
	command_definitions.push_back(_make_def(CG_CMD_CONTROL_SWITCHES, "Control de Interruptores...", "Progresión"));
	command_definitions.push_back(_make_def(CG_CMD_CONTROL_VARIABLES, "Control de Variables...", "Progresión"));
	command_definitions.push_back(_make_def(CG_CMD_CONTROL_SELF_SWITCH, "Control de Interruptor Local...", "Progresión"));
	command_definitions.push_back(_make_def(CG_CMD_CONTROL_TIMER, "Control de Temporizador...", "Progresión"));
	command_definitions.push_back(_make_def(CG_CMD_CONDITIONAL_BRANCH, "Derivación Condicional (Si/Sino)...", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_LOOP, "Bucle", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_BREAK_LOOP, "Romper Bucle", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_EXIT_EVENT, "Salir del Procesamiento de Evento", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_COMMON_EVENT, "Evento Común...", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_LABEL, "Etiqueta...", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_JUMP_TO_LABEL, "Saltar a Etiqueta...", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_COMMENT, "Comentario...", "Control de Flujo"));
	command_definitions.push_back(_make_def(CG_CMD_SCRIPT_FORMULA, "Evaluar Fórmula...", "Control de Flujo"));

	// Pestaña 2
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_GOLD, "Cambiar Oro...", "Grupo e Inventario"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_ITEMS, "Cambiar Objetos...", "Grupo e Inventario"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_WEAPONS, "Cambiar Armas...", "Grupo e Inventario"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_ARMORS, "Cambiar Armaduras...", "Grupo e Inventario"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_PARTY_MEMBER, "Cambiar Miembros del Grupo...", "Grupo e Inventario"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_HP, "Cambiar PV (HP)...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_MP, "Cambiar PM (MP)...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_STATE, "Cambiar Estado...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_RECOVER_ALL, "Recuperar Todo...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_EXP, "Cambiar EXP...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_LEVEL, "Cambiar Nivel...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_PARAMETER, "Cambiar Parámetro...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_SKILL, "Cambiar Habilidad...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_EQUIPMENT, "Cambiar Equipamiento...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_NAME, "Cambiar Nombre...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_CLASS, "Cambiar Clase...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_NICKNAME, "Cambiar Apodo...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_ACTOR_GRAPHIC, "Cambiar Gráfico de Actor...", "Actor"));
	command_definitions.push_back(_make_def(CG_CMD_TRANSFER_PLAYER, "Transferir Jugador...", "Movimiento"));
	command_definitions.push_back(_make_def(CG_CMD_SET_EVENT_LOCATION, "Establecer Posición de Evento...", "Movimiento"));
	command_definitions.push_back(_make_def(CG_CMD_SCROLL_MAP, "Desplazar Mapa...", "Movimiento"));
	command_definitions.push_back(_make_def(CG_CMD_SET_MOVEMENT_ROUTE, "Establecer Ruta de Movimiento...", "Movimiento"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_TRANSPARENCY, "Cambiar Transparencia...", "Personaje"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_FOLLOWERS, "Mostrar/Ocultar Seguidores...", "Seguidores"));
	command_definitions.push_back(_make_def(CG_CMD_GATHER_FOLLOWERS, "Reunir Seguidores", "Seguidores"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_PARTY_ORDER, "Cambiar Orden del Grupo...", "Seguidores"));
	command_definitions.push_back(_make_def(CG_CMD_TOGGLE_FOLLOWER, "Habilitar/Deshabilitar Seguidor...", "Seguidores"));
	command_definitions.push_back(_make_def(CG_CMD_SHOW_ANIMATION, "Mostrar Animación...", "Personaje"));
	command_definitions.push_back(_make_def(CG_CMD_SHOW_BALLOON, "Mostrar Globo de Expresión...", "Personaje"));

	// Pestaña 3
	command_definitions.push_back(_make_def(CG_CMD_FADEOUT_SCREEN, "Fundir Pantalla a Negro", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_FADEIN_SCREEN, "Mostrar Pantalla desde Negro", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_TINT_SCREEN, "Teñir Pantalla...", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_FLASH_SCREEN, "Destello de Pantalla (Flash)...", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_SHAKE_SCREEN, "Sacudir Pantalla (Temblor)...", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_CAMERA_ZOOM, "Cambiar Zoom de Cámara...", "Pantalla"));
	command_definitions.push_back(_make_def(CG_CMD_WAIT, "Esperar...", "Tiempo"));
	command_definitions.push_back(_make_def(CG_CMD_SHOW_PICTURE, "Mostrar Imagen...", "Imágenes y Clima"));
	command_definitions.push_back(_make_def(CG_CMD_MOVE_PICTURE, "Mover Imagen...", "Imágenes y Clima"));
	command_definitions.push_back(_make_def(CG_CMD_ROTATE_PICTURE, "Rotar Imagen...", "Imágenes y Clima"));
	command_definitions.push_back(_make_def(CG_CMD_ERASE_PICTURE, "Borrar Imagen...", "Imágenes y Clima"));
	command_definitions.push_back(_make_def(CG_CMD_SET_WEATHER, "Establecer Efecto de Clima...", "Imágenes y Clima"));
	command_definitions.push_back(_make_def(CG_CMD_PLAY_BGM, "Reproducir BGM...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_FADEOUT_BGM, "Fundir BGM...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_SAVE_BGM, "Guardar BGM", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_RESUME_BGM, "Reanudar BGM", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_PLAY_BGS, "Reproducir BGS...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_FADEOUT_BGS, "Fundir BGS...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_PLAY_ME, "Reproducir ME...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_PLAY_SE, "Reproducir SE...", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_STOP_SE, "Detener SE", "Audio"));
	command_definitions.push_back(_make_def(CG_CMD_BATTLE_PROCESSING, "Procesamiento de Batalla...", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_SHOP_PROCESSING, "Procesamiento de Tienda...", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_NAME_INPUT, "Procesamiento de Entrada de Nombre...", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_OPEN_MENU, "Abrir Pantalla de Menú", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_OPEN_SAVE, "Abrir Pantalla de Guardado", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_GAME_OVER, "Game Over", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_RETURN_TO_TITLE, "Volver a la Pantalla de Título", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_TILESET, "Cambiar Tileset...", "Sistema"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_BATTLE_BG, "Cambiar Fondo de Batalla...", "Sistema"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_SYSTEM_OPTIONS, "Cambiar Opciones del Sistema...", "Sistema"));

	// Códigos de escape de texto - Sección 8.2
	text_escape_definitions.clear();
	struct EscapeDef {
		const char *code;
		const char *desc;
	};
	const EscapeDef escapes[] = {
		{ "\\V[n]", "Reemplaza por el valor de la variable n" },
		{ "\\N[n]", "Reemplaza por el nombre del actor n" },
		{ "\\P[n]", "Reemplaza por el miembro del grupo n" },
		{ "\\G", "Reemplaza por el nombre de la moneda" },
		{ "\\C[n]", "Cambia el color del texto a la paleta n" },
		{ "\\I[n]", "Dibuja el icono n" },
		{ "\\{", "Aumenta el tamaño de fuente en un paso" },
		{ "\\}", "Reduce el tamaño de fuente en un paso" },
		{ "\\\\", "Carácter de barra invertida literal" },
		{ "\\$", "Abre la ventana de oro actual" },
		{ "\\.", "Espera 1/4 de segundo (15 fotogramas)" },
		{ "\\|", "Espera 1 segundo (60 fotogramas)" },
		{ "\\!", "Espera la pulsación del botón de acción" },
		{ "\\>", "Muestra el tramo restante de línea de forma instantánea" },
		{ "\\<", "Desactiva la visualización instantánea" },
		{ "\\^", "No espera entrada tras mostrar el texto" },
		{ "\\S[n]", "Cambia la velocidad de escritura del texto" },
		{ "\\B", "Activa/desactiva texto en negrita" },
		{ "\\IT", "Activa/desactiva texto en cursiva" },
		{ "\\FS[n]", "Fija el tamaño de fuente a n px" },
		{ "\\SH", "Efecto de temblor en el texto" },
		{ "\\WV", "Efecto de onda en el texto" },
		{ "\\RB", "Efecto de color arcoíris" },
		{ "\\FACE[n]", "Cambia el índice del gráfico de rostro a n" },
		{ "\\NM[texto]", "Cambia la etiqueta de la caja de nombre" },
		{ "\\SE[n]", "Reproduce el efecto de sonido n" },
		{ "\\W[n]", "Espera exactamente n fotogramas" }
	};
	for (size_t i = 0; i < sizeof(escapes) / sizeof(escapes[0]); i++) {
		Dictionary ed;
		ed["code"] = String::utf8(escapes[i].code);
		ed["description"] = String::utf8(escapes[i].desc);
		text_escape_definitions.push_back(ed);
	}
}

void ClockGearsSystemCatalog::reset_to_defaults() {
	_init_system_defaults();
}

Dictionary ClockGearsSystemCatalog::to_dict() const {
	Dictionary d;
	d["elements"] = elements;
	d["weapon_types"] = weapon_types;
	d["armor_types"] = armor_types;
	d["equip_slots"] = equip_slots;
	return d;
}

void ClockGearsSystemCatalog::from_dict(const Dictionary &p_dict) {
	if (p_dict.has("elements")) {
		elements = p_dict["elements"];
	}
	if (p_dict.has("weapon_types")) {
		weapon_types = p_dict["weapon_types"];
	}
	if (p_dict.has("armor_types")) {
		armor_types = p_dict["armor_types"];
	}
	if (p_dict.has("equip_slots")) {
		equip_slots = p_dict["equip_slots"];
	}
}

void ClockGearsSystemCatalog::_bind_methods() {
	ClassDB::bind_method(D_METHOD("reset_to_defaults"), &ClockGearsSystemCatalog::reset_to_defaults);
	ClassDB::bind_method(D_METHOD("get_elements"), &ClockGearsSystemCatalog::get_elements);
	ClassDB::bind_method(D_METHOD("set_elements", "elements"), &ClockGearsSystemCatalog::set_elements);
	ClassDB::bind_method(D_METHOD("get_weapon_types"), &ClockGearsSystemCatalog::get_weapon_types);
	ClassDB::bind_method(D_METHOD("set_weapon_types", "types"), &ClockGearsSystemCatalog::set_weapon_types);
	ClassDB::bind_method(D_METHOD("get_armor_types"), &ClockGearsSystemCatalog::get_armor_types);
	ClassDB::bind_method(D_METHOD("set_armor_types", "types"), &ClockGearsSystemCatalog::set_armor_types);
	ClassDB::bind_method(D_METHOD("get_equip_slots"), &ClockGearsSystemCatalog::get_equip_slots);
	ClassDB::bind_method(D_METHOD("set_equip_slots", "slots"), &ClockGearsSystemCatalog::set_equip_slots);
	ClassDB::bind_method(D_METHOD("get_scopes"), &ClockGearsSystemCatalog::get_scopes);
	ClassDB::bind_method(D_METHOD("get_occasions"), &ClockGearsSystemCatalog::get_occasions);
	ClassDB::bind_method(D_METHOD("get_triggers"), &ClockGearsSystemCatalog::get_triggers);
	ClassDB::bind_method(D_METHOD("get_move_types"), &ClockGearsSystemCatalog::get_move_types);
	ClassDB::bind_method(D_METHOD("get_base_param_names"), &ClockGearsSystemCatalog::get_base_param_names);
	ClassDB::bind_method(D_METHOD("get_ex_param_names"), &ClockGearsSystemCatalog::get_ex_param_names);
	ClassDB::bind_method(D_METHOD("get_sp_param_names"), &ClockGearsSystemCatalog::get_sp_param_names);
	ClassDB::bind_method(D_METHOD("get_trait_definitions"), &ClockGearsSystemCatalog::get_trait_definitions);
	ClassDB::bind_method(D_METHOD("get_effect_definitions"), &ClockGearsSystemCatalog::get_effect_definitions);
	ClassDB::bind_method(D_METHOD("get_ai_condition_definitions"), &ClockGearsSystemCatalog::get_ai_condition_definitions);
	ClassDB::bind_method(D_METHOD("get_command_definitions"), &ClockGearsSystemCatalog::get_command_definitions);
	ClassDB::bind_method(D_METHOD("get_text_escape_definitions"), &ClockGearsSystemCatalog::get_text_escape_definitions);
	ClassDB::bind_method(D_METHOD("to_dict"), &ClockGearsSystemCatalog::to_dict);
	ClassDB::bind_method(D_METHOD("from_dict", "dict"), &ClockGearsSystemCatalog::from_dict);
}

ClockGearsSystemCatalog::ClockGearsSystemCatalog() {
	singleton = this;
	_init_system_defaults();
}

ClockGearsSystemCatalog::~ClockGearsSystemCatalog() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
