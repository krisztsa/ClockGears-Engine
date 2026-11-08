#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"

// Códigos de comandos de evento equivalentes a RPG Maker MZ (precreados en el ejecutable)
enum ClockGearsCommandCodes {
	CG_CMD_END = 0,
	// Pestaña 1: Mensajes y Control de Flujo
	CG_CMD_SHOW_TEXT = 101,
	CG_CMD_SHOW_CHOICES = 102,
	CG_CMD_INPUT_NUMBER = 103,
	CG_CMD_SELECT_KEY_ITEM = 104,
	CG_CMD_SHOW_SCROLLING_TEXT = 105,
	CG_CMD_COMMENT = 108,
	CG_CMD_CONDITIONAL_BRANCH = 111,
	CG_CMD_LOOP = 112,
	CG_CMD_BREAK_LOOP = 113,
	CG_CMD_EXIT_EVENT = 115,
	CG_CMD_COMMON_EVENT = 117,
	CG_CMD_LABEL = 118,
	CG_CMD_JUMP_TO_LABEL = 119,
	CG_CMD_CONTROL_SWITCHES = 121,
	CG_CMD_CONTROL_VARIABLES = 122,
	CG_CMD_CONTROL_SELF_SWITCH = 123,
	CG_CMD_CONTROL_TIMER = 124,
	CG_CMD_SCRIPT_FORMULA = 355,
	// Bloques internos de indentación
	CG_CMD_WHEN_CHOICE = 402,
	CG_CMD_WHEN_CANCEL = 403,
	CG_CMD_BRANCH_END_CHOICE = 404,
	CG_CMD_ELSE_BRANCH = 411,
	CG_CMD_END_IF = 412,
	CG_CMD_REPEAT_ABOVE = 413,

	// Pestaña 2: Personaje, Movimiento, Inventario y Seguidores
	CG_CMD_CHANGE_GOLD = 125,
	CG_CMD_CHANGE_ITEMS = 126,
	CG_CMD_CHANGE_WEAPONS = 127,
	CG_CMD_CHANGE_ARMORS = 128,
	CG_CMD_CHANGE_PARTY_MEMBER = 129,
	CG_CMD_TRANSFER_PLAYER = 201,
	CG_CMD_SET_EVENT_LOCATION = 203,
	CG_CMD_SCROLL_MAP = 204,
	CG_CMD_SET_MOVEMENT_ROUTE = 205,
	CG_CMD_CHANGE_TRANSPARENCY = 211,
	CG_CMD_SHOW_ANIMATION = 212,
	CG_CMD_SHOW_BALLOON = 213,
	CG_CMD_CHANGE_FOLLOWERS = 216,
	CG_CMD_GATHER_FOLLOWERS = 217,
	CG_CMD_CHANGE_PARTY_ORDER = 218,
	CG_CMD_TOGGLE_FOLLOWER = 219,
	CG_CMD_CHANGE_HP = 311,
	CG_CMD_CHANGE_MP = 312,
	CG_CMD_CHANGE_STATE = 313,
	CG_CMD_RECOVER_ALL = 314,
	CG_CMD_CHANGE_EXP = 315,
	CG_CMD_CHANGE_LEVEL = 316,
	CG_CMD_CHANGE_PARAMETER = 317,
	CG_CMD_CHANGE_SKILL = 318,
	CG_CMD_CHANGE_EQUIPMENT = 319,
	CG_CMD_CHANGE_NAME = 320,
	CG_CMD_CHANGE_CLASS = 321,
	CG_CMD_CHANGE_ACTOR_GRAPHIC = 322,
	CG_CMD_CHANGE_NICKNAME = 324,

	// Pestaña 3: Escena, Audio y Sistema
	CG_CMD_FADEOUT_SCREEN = 221,
	CG_CMD_FADEIN_SCREEN = 222,
	CG_CMD_TINT_SCREEN = 223,
	CG_CMD_FLASH_SCREEN = 224,
	CG_CMD_SHAKE_SCREEN = 225,
	CG_CMD_CHANGE_CAMERA_ZOOM = 226,
	CG_CMD_WAIT = 230,
	CG_CMD_SHOW_PICTURE = 231,
	CG_CMD_MOVE_PICTURE = 232,
	CG_CMD_ROTATE_PICTURE = 233,
	CG_CMD_TINT_PICTURE = 234,
	CG_CMD_ERASE_PICTURE = 235,
	CG_CMD_SET_WEATHER = 236,
	CG_CMD_PLAY_BGM = 241,
	CG_CMD_FADEOUT_BGM = 242,
	CG_CMD_SAVE_BGM = 243,
	CG_CMD_RESUME_BGM = 244,
	CG_CMD_PLAY_BGS = 245,
	CG_CMD_FADEOUT_BGS = 246,
	CG_CMD_AUDIO_STEMS = 247,
	CG_CMD_PLAY_ME = 249,
	CG_CMD_PLAY_SE = 250,
	CG_CMD_STOP_SE = 251,
	CG_CMD_PLAY_CUTSCENE = 260,
	CG_CMD_CHANGE_TILESET = 282,
	CG_CMD_CHANGE_BATTLE_BG = 283,
	CG_CMD_BATTLE_PROCESSING = 301,
	CG_CMD_SHOP_PROCESSING = 302,
	CG_CMD_NAME_INPUT = 303,
	CG_CMD_OPEN_MENU = 351,
	CG_CMD_OPEN_SAVE = 352,
	CG_CMD_GAME_OVER = 353,
	CG_CMD_RETURN_TO_TITLE = 354,
	CG_CMD_CHANGE_SYSTEM_OPTIONS = 356
};

// Catálogo de tipos de Rasgos (Traits) - Sección 4.2
enum ClockGearsTraitCode {
	CG_TRAIT_ELEMENT_RATE = 11,
	CG_TRAIT_DEBUFF_RATE = 12,
	CG_TRAIT_STATE_RATE = 13,
	CG_TRAIT_STATE_RESIST = 14,
	CG_TRAIT_PARAM = 21,
	CG_TRAIT_XPARAM = 22,
	CG_TRAIT_SPARAM = 23,
	CG_TRAIT_ATTACK_ELEMENT = 31,
	CG_TRAIT_ATTACK_STATE = 32,
	CG_TRAIT_ATTACK_SPEED = 33,
	CG_TRAIT_ATTACK_TIMES = 34,
	CG_TRAIT_STYPE_ADD = 41,
	CG_TRAIT_STYPE_SEAL = 42,
	CG_TRAIT_SKILL_ADD = 43,
	CG_TRAIT_SKILL_SEAL = 44,
	CG_TRAIT_EQUIP_WTYPE = 51,
	CG_TRAIT_EQUIP_ATYPE = 52,
	CG_TRAIT_EQUIP_LOCK = 53,
	CG_TRAIT_EQUIP_SEAL = 54,
	CG_TRAIT_SLOT_TYPE = 55,
	CG_TRAIT_ACTION_PLUS = 61,
	CG_TRAIT_SPECIAL_FLAG = 62,
	CG_TRAIT_PARTY_ABILITY = 64
};

// Catálogo de tipos de Efectos de Objeto/Habilidad - Sección 4.3
enum ClockGearsEffectCode {
	CG_EFFECT_RECOVER_HP = 11,
	CG_EFFECT_RECOVER_MP = 12,
	CG_EFFECT_GAIN_TP = 13,
	CG_EFFECT_ADD_STATE = 21,
	CG_EFFECT_REMOVE_STATE = 22,
	CG_EFFECT_ADD_BUFF = 31,
	CG_EFFECT_ADD_DEBUFF = 32,
	CG_EFFECT_REMOVE_BUFF = 33,
	CG_EFFECT_REMOVE_DEBUFF = 34,
	CG_EFFECT_SPECIAL_ESCAPE = 41,
	CG_EFFECT_GROW_PARAM = 42,
	CG_EFFECT_LEARN_SKILL = 43,
	CG_EFFECT_COMMON_EVENT = 44
};

// Condiciones de IA de Enemigos - Sección 5.3 / 9.7
enum ClockGearsAIConditionType {
	CG_AI_COND_ALWAYS = 0,
	CG_AI_COND_TURN = 1,
	CG_AI_COND_HP_RANGE = 2,
	CG_AI_COND_MP_RANGE = 3,
	CG_AI_COND_STATE = 4,
	CG_AI_COND_PARTY_LEVEL = 5,
	CG_AI_COND_SWITCH = 6
};

class ClockGearsSystemCatalog : public Object {
	GDCLASS(ClockGearsSystemCatalog, Object);

private:
	static ClockGearsSystemCatalog *singleton;

	PackedStringArray elements;
	PackedStringArray weapon_types;
	PackedStringArray armor_types;
	PackedStringArray equip_slots;
	PackedStringArray scopes;
	PackedStringArray occasions;
	PackedStringArray triggers;
	PackedStringArray move_types;
	PackedStringArray base_param_names;
	PackedStringArray ex_param_names;
	PackedStringArray sp_param_names;
	Array trait_definitions;
	Array effect_definitions;
	Array ai_condition_definitions;
	Array command_definitions;
	Array text_escape_definitions;

	void _init_system_defaults();

protected:
	static void _bind_methods();

public:
	static ClockGearsSystemCatalog *get_singleton() { return singleton; }

	void reset_to_defaults();

	PackedStringArray get_elements() const { return elements; }
	void set_elements(const PackedStringArray &p_elements) { elements = p_elements; }

	PackedStringArray get_weapon_types() const { return weapon_types; }
	void set_weapon_types(const PackedStringArray &p_types) { weapon_types = p_types; }

	PackedStringArray get_armor_types() const { return armor_types; }
	void set_armor_types(const PackedStringArray &p_types) { armor_types = p_types; }

	PackedStringArray get_equip_slots() const { return equip_slots; }
	void set_equip_slots(const PackedStringArray &p_slots) { equip_slots = p_slots; }

	PackedStringArray get_scopes() const { return scopes; }
	PackedStringArray get_occasions() const { return occasions; }
	PackedStringArray get_triggers() const { return triggers; }
	PackedStringArray get_move_types() const { return move_types; }
	PackedStringArray get_base_param_names() const { return base_param_names; }
	PackedStringArray get_ex_param_names() const { return ex_param_names; }
	PackedStringArray get_sp_param_names() const { return sp_param_names; }

	Array get_trait_definitions() const { return trait_definitions; }
	Array get_effect_definitions() const { return effect_definitions; }
	Array get_ai_condition_definitions() const { return ai_condition_definitions; }
	Array get_command_definitions() const { return command_definitions; }
	Array get_text_escape_definitions() const { return text_escape_definitions; }

	Dictionary to_dict() const;
	void from_dict(const Dictionary &p_dict);

	ClockGearsSystemCatalog();
	~ClockGearsSystemCatalog();
};
