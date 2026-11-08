#pragma once

#include "core/object/class_db.h"
#include "core/object/callable_mp.h"
#include "core/io/resource.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"

enum ClockGearsGridSize {
	GRID_16X16 = 16,
	GRID_32X32 = 32,
	GRID_48X48 = 48,
};

enum ClockGearsLayer {
	LAYER_GROUND = 0,
	LAYER_DECORATION = 1,
	LAYER_OBJECTS = 2,
	LAYER_OVERHEAD = 3,
};

enum ClockGearsEventTrigger {
	TRIGGER_ACTION_BUTTON = 0,
	TRIGGER_PLAYER_TOUCH = 1,
	TRIGGER_EVENT_TOUCH = 2,
	TRIGGER_AUTORUN = 3,
	TRIGGER_PARALLEL = 4,
};

enum ClockGearsMoveType {
	MOVE_FIXED = 0,
	MOVE_RANDOM = 1,
	MOVE_APPROACH_PLAYER = 2,
	MOVE_CUSTOM_ROUTE = 3,
};

enum ClockGearsThemePreset {
	THEME_RETRO_PIXELART = 0,
	THEME_DARK_OLED = 1,
	THEME_CLASSIC_JRPG_BLUE = 2,
};

enum ClockGearsCombatMode {
	COMBAT_MODE_TURN_BASED = 0,
	COMBAT_MODE_ACTION_RPG = 1,
};

enum ClockGearsElement {
	ELEMENT_PHYSICAL = 0,
	ELEMENT_FIRE = 1,
	ELEMENT_ICE = 2,
	ELEMENT_THUNDER = 3,
	ELEMENT_WATER = 4,
	ELEMENT_EARTH = 5,
	ELEMENT_WIND = 6,
	ELEMENT_LIGHT = 7,
	ELEMENT_DARKNESS = 8,
	ELEMENT_COUNT = 9,
};

enum ClockGearsElementAffinity {
	AFFINITY_IMMUNE = 0,     // 0% daño
	AFFINITY_RESISTANT = 1,  // 50% de daño
	AFFINITY_NEUTRAL = 2,    // 100% de daño (estándar)
	AFFINITY_WEAK = 3,       // 150% de daño
	AFFINITY_SUPER_WEAK = 4, // 200% de daño
	AFFINITY_ABSORB = 5,     // Absorbe daño como cura
};

enum ClockGearsStat {
	STAT_MAX_HP = 0,
	STAT_MAX_MP = 1,
	STAT_ATK = 2,
	STAT_DEF = 3,
	STAT_MAT = 4,
	STAT_MDF = 5,
	STAT_AGI = 6,
	STAT_LUK = 7,
	STAT_COUNT = 8,
};

enum ClockGearsCommandID {
	// Pestaña 1: Mensajes y Control
	CMD_SHOW_MESSAGE = 101,
	CMD_SHOW_CHOICES = 102,
	CMD_CONTROL_SWITCH = 103,
	CMD_CONTROL_VARIABLE = 104,
	CMD_CONTROL_SELF_SWITCH = 105,
	CMD_WAIT_FRAMES = 106,
	CMD_INPUT_NUMBER = 107,

	// Pestaña 2: Movimiento y Personaje
	CMD_TRANSFER_PLAYER = 201,
	CMD_SET_MOVE_ROUTE = 202,
	CMD_ADD_ITEM = 203,
	CMD_CHANGE_GOLD = 204,
	CMD_CHANGE_HP_MP = 205,

	// Pestaña 3: Escena y Multimedia
	CMD_PLAY_BGM = 301,
	CMD_PLAY_SE = 302,
	CMD_FLASH_EFFECT = 303,
	CMD_TINT_SCREEN = 304,
	CMD_FADE_SCREEN = 305,
	CMD_BOSS_BATTLE = 306,
	CMD_BATTLE_PROCESSING = 306,

	// Imágenes (Anexo A1.3)
	CMD_SHOW_PICTURE = 307,
	CMD_MOVE_PICTURE = 308,
	CMD_ROTATE_PICTURE = 309,
	CMD_TINT_PICTURE = 310,
	CMD_ERASE_PICTURE = 311,

	// Pantalla (Anexo A1.3)
	CMD_SHAKE_SCREEN = 312,
	CMD_WEATHER = 313,
	CMD_CHANGE_CAMERA_ZOOM = 322,

	// Audio (Anexo A1.4)
	CMD_PLAY_BGS = 314,
	CMD_PLAY_ME = 315,
	CMD_STOP_FADE_BGM = 316,
	CMD_SAVE_BGM = 317,
	CMD_RESTORE_BGM = 318,
	CMD_AUDIO_STEMS = 319,

	// Cinemática (Anexo A1.2)
	CMD_PLAY_CUTSCENE = 320,
	CMD_CREATE_CUTSCENE = 321,
};

#define CG_TR(m_str) String::utf8(m_str)

