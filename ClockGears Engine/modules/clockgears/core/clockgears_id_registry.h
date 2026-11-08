#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

class ClockGearsIdRegistry : public Object {
	GDCLASS(ClockGearsIdRegistry, Object);

private:
	struct CategoryPool {
		int next_id = 1;
		HashSet<int> active_ids;
		HashSet<int> retired_ids_until_save;
	};

	static ClockGearsIdRegistry *singleton;
	HashMap<String, CategoryPool> pools;

protected:
	static void _bind_methods();

public:
	static ClockGearsIdRegistry *get_singleton() { return singleton; }

	// Asigna un nuevo ID entero estable (>= 1) que no esté activo ni retirado en la sesión actual
	int allocate_id(const String &p_category);

	// Registra un ID existente como activo (por ejemplo, al cargar o deshacer un borrado)
	bool register_id(const String &p_category, int p_id);

	// Retira un ID al eliminarse un registro; bloquea su reutilización hasta que se guarde el proyecto
	void retire_id(const String &p_category, int p_id);

	// Comprueba si un ID está actualmente activo en la categoría
	bool is_id_active(const String &p_category, int p_id) const;

	// Sincroniza el pool de una categoría tras cargar registros desde disco
	void sync_category_from_records(const String &p_category, const Array &p_records);

	// Libera la reserva de IDs retirados tras confirmar el guardado en disco
	void commit_on_save();

	// Reinicia todos los contadores (al cerrar o crear proyecto nuevo)
	void reset_all();

	ClockGearsIdRegistry();
	~ClockGearsIdRegistry();
};
