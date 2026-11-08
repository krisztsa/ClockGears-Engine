#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

// ============================================================================
// ClockGearsTimeManager — Reloj del Juego y Reinicio de Ciclo (Anexo A3.1 y A3.2)
// Singleton de núcleo para RPGs con rutinas y ciclos de días (estilo Majora's Mask)
// ============================================================================

enum ClockGearsTimeAdvanceMode {
	TIME_ADVANCE_REALTIME = 0,
	TIME_ADVANCE_BY_ACTIONS = 1,
	TIME_ADVANCE_MANUAL = 2,
};

class ClockGearsTimeManager : public Object {
	GDCLASS(ClockGearsTimeManager, Object);

private:
	static ClockGearsTimeManager *singleton;

	// --- A3.1 Configuración del Reloj por Proyecto ---
	int minutes_per_hour = 60;
	int hours_per_day = 24;
	int days_per_cycle = 3;
	int cycle_start_hour = 6; // Hora inicial del Día 1 al comenzar o reiniciar el ciclo
	bool cycle_mode_enabled = false;
	PackedStringArray day_names; // Vacío por defecto (Regla Cero)
	Array day_periods; // Periodos definibles: [{name, start_hour, end_hour}]

	// --- A3.1 Estado del Tiempo en Partida ---
	int64_t total_game_minutes = 0; // Minutos transcurridos desde las 00:00 del Día 1 del ciclo actual
	double fractional_minutes_accum = 0.0;
	double minutes_per_real_second = 1.0; // Factor: 1 seg real = X minutos de juego
	int current_cycle_count = 1;

	int advance_mode = TIME_ADVANCE_REALTIME;
	bool paused = false;

	// Reglas de pausa contextual (A3.1)
	bool pause_in_dialogue = true;
	bool pause_in_menu = true;
	bool pause_in_cutscene = true;
	bool pause_in_battle = true;

	// Flags de contexto activo en runtime
	bool active_in_dialogue = false;
	bool active_in_menu = false;
	bool active_in_cutscene = false;
	bool active_in_battle = false;

	// --- A3.2 Reglas de Persistencia de Ciclo ---
	// Por defecto TODO se reinicia salvo lo marcado explícitamente como true en persistent_rules.
	// Estructura: { "switches": { "1": true }, "variables": {}, "items": {}, "equipment": {},
	//               "skills": {}, "events_state": {}, "npc_progress": {},
	//               "flags": { "gold": false, "party_level_exp": false } }
	Dictionary persistent_rules;
	Dictionary cycle_initial_snapshot;

	// Configuración de transición y reposición al reiniciar el ciclo (A3.2)
	int cycle_reset_common_event_id = 0;
	String cycle_spawn_map;
	int cycle_spawn_x = 0;
	int cycle_spawn_y = 0;

	void _init_default_periods();
	void _init_default_persistence_rules();
	void _apply_minutes_delta(int64_t p_delta_minutes);

protected:
	static void _bind_methods();

public:
	static ClockGearsTimeManager *get_singleton() { return singleton; }

	void init_empty();

	// --- Configuración de calendario (A3.1) ---
	void set_minutes_per_hour(int p_minutes);
	int get_minutes_per_hour() const { return minutes_per_hour; }

	void set_hours_per_day(int p_hours);
	int get_hours_per_day() const { return hours_per_day; }

	void set_days_per_cycle(int p_days);
	int get_days_per_cycle() const { return days_per_cycle; }

	void set_cycle_start_hour(int p_hour);
	int get_cycle_start_hour() const { return cycle_start_hour; }

	void set_cycle_mode_enabled(bool p_enabled);
	bool is_cycle_mode_enabled() const { return cycle_mode_enabled; }

	void set_day_names(const PackedStringArray &p_names);
	PackedStringArray get_day_names() const { return day_names; }
	String get_day_name(int p_day_1based) const;

	void set_day_periods(const Array &p_periods);
	Array get_day_periods() const { return day_periods; }
	String get_current_period_name() const;

	// --- Avance y control del reloj (A3.1) ---
	void set_advance_mode(int p_mode);
	int get_advance_mode() const { return advance_mode; }

	void set_time_scale(double p_minutes_per_real_sec);
	double get_time_scale() const { return minutes_per_real_second; }

	void set_paused(bool p_paused);
	bool is_paused() const { return paused; }
	bool is_effectively_paused() const;

	void set_pause_in_dialogue(bool p_pause) { pause_in_dialogue = p_pause; }
	bool get_pause_in_dialogue() const { return pause_in_dialogue; }
	void set_pause_in_menu(bool p_pause) { pause_in_menu = p_pause; }
	bool get_pause_in_menu() const { return pause_in_menu; }
	void set_pause_in_cutscene(bool p_pause) { pause_in_cutscene = p_pause; }
	bool get_pause_in_cutscene() const { return pause_in_cutscene; }
	void set_pause_in_battle(bool p_pause) { pause_in_battle = p_pause; }
	bool get_pause_in_battle() const { return pause_in_battle; }

	void set_context_state(bool p_dialogue, bool p_menu, bool p_cutscene, bool p_battle);

	void tick(double p_delta_seconds);

	// --- Comandos de evento de tiempo (A3.1) ---
	void set_total_minutes(int64_t p_total_minutes);
	int64_t get_total_minutes() const { return total_game_minutes; }

	void set_game_time(int p_day_1based, int p_hour, int p_minute);
	void advance_minutes(int64_t p_minutes);
	void skip_to_hour(int p_target_hour, int p_target_minute = 0);

	int get_current_minute() const;
	int get_current_hour() const;
	int get_current_day() const; // 1-based (Día 1 .. days_per_cycle)
	int get_current_cycle_count() const { return current_cycle_count; }
	String get_formatted_time(bool p_include_day = true) const;

	// --- Condiciones de evento (A3.1) ---
	bool is_time_between(int p_start_hour, int p_start_min, int p_end_hour, int p_end_min) const;
	bool is_on_day(int p_day_1based) const;
	bool is_in_period(const String &p_period_name) const;

	// --- A3.2 Reglas de Persistencia y Reinicio de Ciclo ---
	void set_persistence_rule(const String &p_category, const String &p_key, bool p_persistent);
	bool is_persistent(const String &p_category, const String &p_key) const;

	void set_global_persistence_flag(const String &p_flag_name, bool p_persistent);
	bool get_global_persistence_flag(const String &p_flag_name) const;

	void set_persistence_rules(const Dictionary &p_rules);
	Dictionary get_persistence_rules() const { return persistent_rules.duplicate(true); }

	Dictionary get_persistence_summary() const;

	void set_cycle_reset_transition(int p_common_event_id, const String &p_spawn_map, int p_spawn_x, int p_spawn_y);
	int get_cycle_reset_common_event_id() const { return cycle_reset_common_event_id; }
	String get_cycle_spawn_map() const { return cycle_spawn_map; }
	int get_cycle_spawn_x() const { return cycle_spawn_x; }
	int get_cycle_spawn_y() const { return cycle_spawn_y; }

	void capture_cycle_initial_snapshot(const Dictionary &p_runtime_state);
	Dictionary get_cycle_initial_snapshot() const { return cycle_initial_snapshot.duplicate(true); }

	Dictionary apply_cycle_reset_filter(const Dictionary &p_current_runtime_state) const;
	Dictionary reset_cycle(const Dictionary &p_current_runtime_state = Dictionary());

	// --- Serialización (Proyecto y Guardado de Partida) ---
	Dictionary to_dict() const;
	void from_dict(const Dictionary &p_dict);

	ClockGearsTimeManager();
	~ClockGearsTimeManager();
};
