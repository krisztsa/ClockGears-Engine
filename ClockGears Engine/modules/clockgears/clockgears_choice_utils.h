#pragma once

#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/object/class_db.h"
#include "modules/clockgears/core/clockgears_system_catalog.h"
#include "modules/clockgears/clockgears_types.h"

class ClockGearsChoiceUtils {
public:
	// Identificación de comandos de bloque de elecciones
	static bool is_choices_start(const Dictionary &p_cmd);
	static bool is_when_choice(const Dictionary &p_cmd);
	static bool is_when_cancel(const Dictionary &p_cmd);
	static bool is_branch_end(const Dictionary &p_cmd);

	// Navegación de bloques
	static int find_block_end(const Array &p_cmds, int p_start_idx);
	static int find_block_start(const Array &p_cmds, int p_end_or_inside_idx);
	static int get_command_depth(const Array &p_cmds, int p_idx);

	// Contenido y preservación
	static bool block_has_content(const Array &p_cmds, int p_start_idx);
	static HashMap<int, Array> extract_branches(const Array &p_cmds, int p_start_idx);
	static Array rebuild_choice_block(const Dictionary &p_choice_cmd, const HashMap<int, Array> &p_branches);

	// Movimiento (Subir / Bajar)
	static bool can_move_up(const Array &p_cmds, int p_idx);
	static int move_up(Array &r_cmds, int p_idx);

	static bool can_move_down(const Array &p_cmds, int p_idx);
	static int move_down(Array &r_cmds, int p_idx);

	// Eliminación
	static void delete_choice_block(Array &r_cmds, int p_start_idx);
};
