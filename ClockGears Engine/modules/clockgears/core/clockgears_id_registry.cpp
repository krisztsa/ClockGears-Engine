#include "clockgears_id_registry.h"

ClockGearsIdRegistry *ClockGearsIdRegistry::singleton = nullptr;

int ClockGearsIdRegistry::allocate_id(const String &p_category) {
	ERR_FAIL_COND_V(p_category.is_empty(), 1);
	CategoryPool &pool = pools[p_category];
	if (pool.next_id < 1) {
		pool.next_id = 1;
	}
	while (pool.active_ids.has(pool.next_id) || pool.retired_ids_until_save.has(pool.next_id)) {
		pool.next_id++;
	}
	int allocated = pool.next_id;
	pool.active_ids.insert(allocated);
	pool.next_id = allocated + 1;
	return allocated;
}

bool ClockGearsIdRegistry::register_id(const String &p_category, int p_id) {
	ERR_FAIL_COND_V(p_category.is_empty(), false);
	ERR_FAIL_COND_V(p_id < 1, false);
	CategoryPool &pool = pools[p_category];
	pool.retired_ids_until_save.erase(p_id);
	pool.active_ids.insert(p_id);
	if (p_id >= pool.next_id) {
		pool.next_id = p_id + 1;
	}
	return true;
}

void ClockGearsIdRegistry::retire_id(const String &p_category, int p_id) {
	ERR_FAIL_COND(p_category.is_empty());
	if (p_id < 1) {
		return;
	}
	CategoryPool &pool = pools[p_category];
	pool.active_ids.erase(p_id);
	pool.retired_ids_until_save.insert(p_id);
}

bool ClockGearsIdRegistry::is_id_active(const String &p_category, int p_id) const {
	const CategoryPool *pool = pools.getptr(p_category);
	if (!pool) {
		return false;
	}
	return pool->active_ids.has(p_id);
}

void ClockGearsIdRegistry::sync_category_from_records(const String &p_category, const Array &p_records) {
	ERR_FAIL_COND(p_category.is_empty());
	CategoryPool &pool = pools[p_category];
	pool.active_ids.clear();
	int max_id = 0;
	for (int i = 0; i < p_records.size(); i++) {
		if (p_records[i].get_type() == Variant::DICTIONARY) {
			Dictionary rec = p_records[i];
			int id = int(rec.get("id", 0));
			if (id >= 1) {
				pool.active_ids.insert(id);
				if (id > max_id) {
					max_id = id;
				}
			}
		}
	}
	// No reutilizar IDs retirados en sesión hasta que se guarde
	for (const int &ret_id : pool.retired_ids_until_save) {
		if (ret_id > max_id) {
			max_id = ret_id;
		}
	}
	pool.next_id = max_id + 1;
}

void ClockGearsIdRegistry::commit_on_save() {
	for (KeyValue<String, CategoryPool> &E : pools) {
		E.value.retired_ids_until_save.clear();
		int max_id = 0;
		for (const int &act_id : E.value.active_ids) {
			if (act_id > max_id) {
				max_id = act_id;
			}
		}
		E.value.next_id = max_id + 1;
	}
}

void ClockGearsIdRegistry::reset_all() {
	pools.clear();
}

void ClockGearsIdRegistry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("allocate_id", "category"), &ClockGearsIdRegistry::allocate_id);
	ClassDB::bind_method(D_METHOD("register_id", "category", "id"), &ClockGearsIdRegistry::register_id);
	ClassDB::bind_method(D_METHOD("retire_id", "category", "id"), &ClockGearsIdRegistry::retire_id);
	ClassDB::bind_method(D_METHOD("is_id_active", "category", "id"), &ClockGearsIdRegistry::is_id_active);
	ClassDB::bind_method(D_METHOD("sync_category_from_records", "category", "records"), &ClockGearsIdRegistry::sync_category_from_records);
	ClassDB::bind_method(D_METHOD("commit_on_save"), &ClockGearsIdRegistry::commit_on_save);
	ClassDB::bind_method(D_METHOD("reset_all"), &ClockGearsIdRegistry::reset_all);
}

ClockGearsIdRegistry::ClockGearsIdRegistry() {
	singleton = this;
}

ClockGearsIdRegistry::~ClockGearsIdRegistry() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
