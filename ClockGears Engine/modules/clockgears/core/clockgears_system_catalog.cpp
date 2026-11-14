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
	// Elementos base del sistema (identificador interno fijo, nombre mostrable editable en "Tipos")
	elements.clear();
	element_keys.clear();
	struct DefaultElem {
		const char *key;
		const char *name;
	};
	const DefaultElem default_elements[] = {
		{ "agua", "Agua" },
		{ "fuego", "Fuego" },
		{ "hielo", "Hielo" },
		{ "electricidad", "Electricidad" },
		{ "viento", "Viento" },
		{ "luz", "Luz" },
		{ "oscuridad", "Oscuridad" },
		{ "astral", "Astral" },
		{ "tierra", "Tierra" },
		{ "naturaleza", "Naturaleza" }
	};
	for (size_t i = 0; i < sizeof(default_elements) / sizeof(default_elements[0]); i++) {
		element_keys.push_back(String::utf8(default_elements[i].key));
		elements.push_back(String::utf8(default_elements[i].name));
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
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_TURN, "Turno N", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_HP_RANGE, "Vida por debajo de X%", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_STATE, "Estado", "IA"));
	ai_condition_definitions.push_back(_make_def(CG_AI_COND_SWITCH, "Interruptor", "IA"));

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
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_ARPG_EQUIP, "Cambiar Equipo ARPG...", "Actor"));
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
	command_definitions.push_back(_make_def(CG_CMD_SHOW_CREDITS, "Mostrar Créditos...", "Escena"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_MENU_ACCESS, "Bloquear / Permitir Menú del Juego...", "Sistema"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_WINDOW_STYLE, "Cambiar Estilo de Ventana...", "Sistema"));
	command_definitions.push_back(_make_def(CG_CMD_CHANGE_HUD_VISIBILITY, "Mostrar / Ocultar HUD...", "Sistema"));
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

static int _match_canonical_element_index(const String &p_str) {
	String s = p_str.strip_edges().to_lower();
	if (s.is_empty()) {
		return -1;
	}
	if (s == "agua" || s == "water" || s == "aqua") {
		return 0;
	}
	if (s == "fuego" || s == "fire" || s == "piro" || s == "llama") {
		return 1;
	}
	if (s == "hielo" || s == "ice" || s == "crio" || s == "escarcha") {
		return 2;
	}
	if (s == "electricidad" || s == "rayo" || s == "trueno" || s == "electro" || s == "relampago" || s == String::utf8("relámpago") || s == "thunder" || s == "lightning") {
		return 3;
	}
	if (s == "viento" || s == "aire" || s == "aero" || s == "wind") {
		return 4;
	}
	if (s == "luz" || s == "sagrado" || s == "santo" || s == "holy" || s == "light") {
		return 5;
	}
	if (s == "oscuridad" || s == "sombra" || s == "tinieblas" || s == "dark" || s == "darkness") {
		return 6;
	}
	if (s == "astral" || s == "cosmos" || s == "arcano" || s == "estelar") {
		return 7;
	}
	if (s == "tierra" || s == "roca" || s == "piedra" || s == "geo" || s == "earth") {
		return 8;
	}
	if (s == "naturaleza" || s == "planta" || s == "flora" || s == "bosque" || s == "hierba" || s == "nature") {
		return 9;
	}
	return -1;
}

static bool _is_legacy_alias_name(const String &p_name, int p_canonical_idx) {
	String s = p_name.strip_edges().to_lower();
	switch (p_canonical_idx) {
		case 3:
			return (s == "rayo" || s == "trueno" || s == "relampago" || s == String::utf8("relámpago"));
		case 5:
			return (s == "sagrado" || s == "santo");
		case 6:
			return (s == "sombra" || s == "tinieblas");
		case 9:
			return (s == "planta" || s == "hierba");
		default:
			return false;
	}
}

void ClockGearsSystemCatalog::reset_to_defaults() {
	legacy_element_id_map.clear();
	_init_system_defaults();
}

void ClockGearsSystemCatalog::set_elements(const PackedStringArray &p_elements) {
	const char *def_keys[] = {
		"agua", "fuego", "hielo", "electricidad", "viento", "luz", "oscuridad", "astral", "tierra", "naturaleza"
	};
	const char *def_names[] = {
		"Agua", "Fuego", "Hielo", "Electricidad", "Viento", "Luz", "Oscuridad", "Astral", "Tierra", "Naturaleza"
	};

	if (p_elements.is_empty()) {
		ensure_default_elements_if_empty();
		return;
	}

	// Check if p_elements is already aligned with the 10 canonical elements (e.g. user edited a display name)
	bool already_canonical_order = (p_elements.size() == 10 && element_keys.size() == 10);
	if (already_canonical_order) {
		int matched_in_place = 0;
		bool looks_like_shifted_legacy = false;
		for (int i = 0; i < 10; i++) {
			int m = _match_canonical_element_index(p_elements[i]);
			if (m == i) {
				matched_in_place++;
			} else if (m >= 0 && m != i) {
				// If multiple standard element names sit at wrong indices (e.g. Fuego at index 0), it's legacy order
				looks_like_shifted_legacy = true;
			}
		}
		if (!looks_like_shifted_legacy && matched_in_place >= 7) {
			element_keys.clear();
			elements.clear();
			for (int i = 0; i < 10; i++) {
				element_keys.push_back(String::utf8(def_keys[i]));
				String nm = p_elements[i].strip_edges();
				elements.push_back(nm.is_empty() ? String::utf8(def_names[i]) : nm);
			}
			return;
		}
	}

	// Migrate legacy element list by name into the 10 fixed internal keys
	PackedStringArray final_keys;
	PackedStringArray final_names;
	Vector<bool> slot_assigned;
	for (int i = 0; i < 10; i++) {
		final_keys.push_back(String::utf8(def_keys[i]));
		final_names.push_back(String::utf8(def_names[i]));
		slot_assigned.push_back(false);
	}

	legacy_element_id_map.clear();
	for (int i = 0; i < p_elements.size(); i++) {
		String raw_name = p_elements[i].strip_edges();
		if (raw_name.is_empty()) {
			continue;
		}
		int c_idx = _match_canonical_element_index(raw_name);
		if (c_idx >= 0) {
			int old_id_1based = i + 1;
			legacy_element_id_map[old_id_1based] = c_idx + 1;
			if (!slot_assigned[c_idx]) {
				slot_assigned.write[c_idx] = true;
				if (!_is_legacy_alias_name(raw_name, c_idx)) {
					final_names.write[c_idx] = raw_name;
				}
			}
		}
	}

	element_keys = final_keys;
	elements = final_names;
}

void ClockGearsSystemCatalog::ensure_default_elements_if_empty() {
	if (elements.size() == 10 && element_keys.size() == 10) {
		const char *def_keys[] = {
			"agua", "fuego", "hielo", "electricidad", "viento", "luz", "oscuridad", "astral", "tierra", "naturaleza"
		};
		bool keys_ok = true;
		for (int i = 0; i < 10; i++) {
			if (element_keys[i] != String::utf8(def_keys[i])) {
				keys_ok = false;
				break;
			}
		}
		if (keys_ok) {
			return;
		}
	}
	if (!elements.is_empty()) {
		PackedStringArray copy = elements;
		set_elements(copy);
		return;
	}
	const char *def_keys[] = {
		"agua", "fuego", "hielo", "electricidad", "viento", "luz", "oscuridad", "astral", "tierra", "naturaleza"
	};
	const char *def_names[] = {
		"Agua", "Fuego", "Hielo", "Electricidad", "Viento", "Luz", "Oscuridad", "Astral", "Tierra", "Naturaleza"
	};
	element_keys.clear();
	elements.clear();
	for (size_t i = 0; i < 10; i++) {
		element_keys.push_back(String::utf8(def_keys[i]));
		elements.push_back(String::utf8(def_names[i]));
	}
}

Array ClockGearsSystemCatalog::get_element_records() const {
	Array arr;
	for (int i = 0; i < elements.size(); i++) {
		Dictionary d;
		d["id"] = i + 1;
		d["key"] = (i < element_keys.size()) ? element_keys[i] : elements[i].to_lower();
		d["name"] = elements[i];
		d["type"] = 0;
		d["desc"] = "";
		arr.push_back(d);
	}
	return arr;
}

void ClockGearsSystemCatalog::sync_elements_from_records(const Array &p_records) {
	const char *def_keys[] = {
		"agua", "fuego", "hielo", "electricidad", "viento", "luz", "oscuridad", "astral", "tierra", "naturaleza"
	};
	const char *def_names[] = {
		"Agua", "Fuego", "Hielo", "Electricidad", "Viento", "Luz", "Oscuridad", "Astral", "Tierra", "Naturaleza"
	};

	Vector<Dictionary> elem_recs;
	for (int i = 0; i < p_records.size(); i++) {
		if (p_records[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary d = p_records[i];
		if (int(d.get("type", 0)) != 0) {
			continue;
		}
		elem_recs.push_back(d);
	}

	if (elem_recs.is_empty()) {
		ensure_default_elements_if_empty();
		return;
	}

	// Check if records already have all 10 canonical keys in exact order
	bool has_exact_canonical_keys = (elem_recs.size() == 10);
	if (has_exact_canonical_keys) {
		for (int i = 0; i < 10; i++) {
			String ky = String(elem_recs[i].get("key", "")).strip_edges().to_lower();
			if (ky != String::utf8(def_keys[i])) {
				has_exact_canonical_keys = false;
				break;
			}
			// Also verify it wasn't a blind key stamp over legacy order (e.g. key="agua" with name="Fuego")
			String nm = String(elem_recs[i].get("name", "")).strip_edges();
			int nm_idx = _match_canonical_element_index(nm);
			if (nm_idx >= 0 && nm_idx != i) {
				has_exact_canonical_keys = false;
				break;
			}
		}
	}

	if (has_exact_canonical_keys) {
		element_keys.clear();
		elements.clear();
		for (int i = 0; i < 10; i++) {
			element_keys.push_back(String::utf8(def_keys[i]));
			String nm = String(elem_recs[i].get("name", "")).strip_edges();
			elements.push_back(nm.is_empty() ? String::utf8(def_names[i]) : nm);
		}
		return;
	}

	// Migrate legacy records by name/key into the 10 fixed internal keys
	PackedStringArray final_keys;
	PackedStringArray final_names;
	Vector<bool> slot_assigned;
	for (int i = 0; i < 10; i++) {
		final_keys.push_back(String::utf8(def_keys[i]));
		final_names.push_back(String::utf8(def_names[i]));
		slot_assigned.push_back(false);
	}

	legacy_element_id_map.clear();
	for (int i = 0; i < elem_recs.size(); i++) {
		Dictionary d = elem_recs[i];
		String nm = String(d.get("name", "")).strip_edges();
		String ky = String(d.get("key", "")).strip_edges().to_lower();
		int old_id = int(d.get("id", i + 1));
		if (old_id <= 0) {
			old_id = i + 1;
		}

		int c_idx = _match_canonical_element_index(nm);
		if (c_idx < 0 && !ky.is_empty()) {
			c_idx = _match_canonical_element_index(ky);
		}
		if (c_idx >= 0) {
			legacy_element_id_map[old_id] = c_idx + 1;
			if (!slot_assigned[c_idx]) {
				slot_assigned.write[c_idx] = true;
				if (!nm.is_empty() && !_is_legacy_alias_name(nm, c_idx)) {
					final_names.write[c_idx] = nm;
				}
			}
		}
	}

	element_keys = final_keys;
	elements = final_names;
}

int ClockGearsSystemCatalog::resolve_element_id(const Variant &p_id_or_name) const {
	if (p_id_or_name.get_type() == Variant::INT || p_id_or_name.get_type() == Variant::FLOAT) {
		int id = int(p_id_or_name);
		if (id >= 1 && id <= elements.size()) {
			return id;
		}
		return 0;
	}
	String s = String(p_id_or_name).strip_edges().to_lower();
	if (s.is_empty()) {
		return 0;
	}
	for (int i = 0; i < elements.size(); i++) {
		if (elements[i].to_lower() == s) {
			return i + 1;
		}
		if (i < element_keys.size() && element_keys[i].to_lower() == s) {
			return i + 1;
		}
	}
	int c_idx = _match_canonical_element_index(s);
	if (c_idx >= 0 && c_idx < elements.size()) {
		return c_idx + 1;
	}
	return 0;
}

int ClockGearsSystemCatalog::map_legacy_element_id(int p_old_id) const {
	if (legacy_element_id_map.has(p_old_id)) {
		return legacy_element_id_map[p_old_id];
	}
	if (p_old_id >= 1 && p_old_id <= elements.size()) {
		return p_old_id;
	}
	return 0;
}

void ClockGearsSystemCatalog::migrate_record_elements(Dictionary &r_record) const {
	if (r_record.has("traits") && r_record["traits"].get_type() == Variant::ARRAY) {
		Array traits = r_record["traits"];
		bool changed = false;
		for (int i = 0; i < traits.size(); i++) {
			if (traits[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary tr = traits[i];
			int code = int(tr.get("code", 0));
			if (code == CG_TRAIT_ELEMENT_RATE || code == CG_TRAIT_ATTACK_ELEMENT) {
				int new_id = 0;
				String elem_key = String(tr.get("element_key", "")).strip_edges();
				String elem_name = String(tr.get("element_name", "")).strip_edges();
				String elem_key_norm = elem_key.to_lower();
				int old_id = int(tr.get("data_id", 0));
				bool already_migrated = bool(tr.get("element_migrated", false));
				if (elem_key_norm == "fisico" || elem_key_norm == String::utf8("físico") || elem_key_norm == "physical" || (old_id == 0 && already_migrated)) {
					tr["data_id"] = 0;
					tr["element_key"] = "fisico";
					tr["element_migrated"] = true;
					traits[i] = tr;
					changed = true;
					continue;
				}
				if (!elem_key.is_empty()) {
					new_id = resolve_element_id(elem_key);
				}
				if (new_id <= 0 && !elem_name.is_empty()) {
					new_id = resolve_element_id(elem_name);
				}
				if (new_id <= 0) {
					if (!already_migrated && legacy_element_id_map.has(old_id)) {
						new_id = legacy_element_id_map[old_id];
					} else if (old_id >= 1 && old_id <= elements.size()) {
						new_id = old_id;
					} else if (old_id == 0 && !already_migrated && legacy_element_id_map.has(1)) {
						new_id = legacy_element_id_map[1];
					}
				}
				if (new_id >= 1 && new_id <= elements.size()) {
					tr["data_id"] = new_id;
					tr["element_key"] = get_element_key(new_id);
					tr["element_migrated"] = true;
					traits[i] = tr;
					changed = true;
				}
			}
		}
		if (changed) {
			r_record["traits"] = traits;
		}
	}
}

String ClockGearsSystemCatalog::get_element_name(int p_element_id) const {
	if (p_element_id >= 1 && p_element_id <= elements.size()) {
		return elements[p_element_id - 1];
	}
	return "";
}

String ClockGearsSystemCatalog::get_element_key(int p_element_id) const {
	if (p_element_id >= 1 && p_element_id <= element_keys.size()) {
		return element_keys[p_element_id - 1];
	}
	if (p_element_id >= 1 && p_element_id <= elements.size()) {
		return elements[p_element_id - 1].to_lower();
	}
	return "";
}

Dictionary ClockGearsSystemCatalog::to_dict() const {
	Dictionary d;
	d["elements"] = elements;
	d["element_keys"] = element_keys;
	d["element_records"] = get_element_records();
	d["weapon_types"] = weapon_types;
	d["armor_types"] = armor_types;
	d["equip_slots"] = equip_slots;
	return d;
}

void ClockGearsSystemCatalog::from_dict(const Dictionary &p_dict) {
	if (p_dict.has("element_records") && p_dict["element_records"].get_type() == Variant::ARRAY) {
		sync_elements_from_records(p_dict["element_records"]);
	} else if (p_dict.has("elements")) {
		if (p_dict.has("element_keys")) {
			element_keys = p_dict["element_keys"];
		}
		set_elements(p_dict["elements"]);
	} else {
		ensure_default_elements_if_empty();
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
	ClassDB::bind_method(D_METHOD("get_element_keys"), &ClockGearsSystemCatalog::get_element_keys);
	ClassDB::bind_method(D_METHOD("get_element_records"), &ClockGearsSystemCatalog::get_element_records);
	ClassDB::bind_method(D_METHOD("sync_elements_from_records", "records"), &ClockGearsSystemCatalog::sync_elements_from_records);
	ClassDB::bind_method(D_METHOD("ensure_default_elements_if_empty"), &ClockGearsSystemCatalog::ensure_default_elements_if_empty);
	ClassDB::bind_method(D_METHOD("resolve_element_id", "id_or_name"), &ClockGearsSystemCatalog::resolve_element_id);
	ClassDB::bind_method(D_METHOD("get_element_name", "element_id"), &ClockGearsSystemCatalog::get_element_name);
	ClassDB::bind_method(D_METHOD("get_element_key", "element_id"), &ClockGearsSystemCatalog::get_element_key);
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
