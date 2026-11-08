#pragma once

#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

class ClockGearsFormulas : public Object {
	GDCLASS(ClockGearsFormulas, Object);

public:
	enum TokenType {
		TOK_EOF = 0,
		TOK_NUMBER,
		TOK_IDENT,
		TOK_DOT,
		TOK_LPAREN,
		TOK_RPAREN,
		TOK_LBRACKET,
		TOK_RBRACKET,
		TOK_COMMA,
		TOK_PLUS,
		TOK_MINUS,
		TOK_STAR,
		TOK_SLASH,
		TOK_PERCENT,
		TOK_CARET,
		TOK_BANG,
		TOK_LT,
		TOK_LTE,
		TOK_GT,
		TOK_GTE,
		TOK_EQ,
		TOK_NEQ,
		TOK_AND,
		TOK_OR,
		TOK_QUESTION,
		TOK_COLON
	};

	struct Token {
		TokenType type = TOK_EOF;
		String text;
		double number_val = 0.0;
		int pos = 0;
	};

	enum ASTNodeType {
		AST_NUMBER = 0,
		AST_STAT_REF, // a.atk, b.def, etc.
		AST_VAR_REF, // v[n] o s[n]
		AST_UNARY,
		AST_BINARY,
		AST_TERNARY,
		AST_FUNC_CALL
	};

	struct ASTNode {
		ASTNodeType type = AST_NUMBER;
		double number_val = 0.0;
		String target_name; // "a", "b", "v", "s" o nombre de función
		String prop_name; // "atk", "def", "mhp", etc.
		TokenType op = TOK_EOF;
		int pos = 0;
		Vector<ASTNode> children;
	};

	struct CachedFormula {
		bool valid = false;
		String error_message;
		int error_pos = -1;
		ASTNode root;
	};

private:
	static ClockGearsFormulas *singleton;
	mutable HashMap<String, CachedFormula> ast_cache;
	mutable RandomPCG rng;
	mutable bool suppress_div_zero_warning = false;

	bool _tokenize(const String &p_expr, Vector<Token> &r_tokens, String &r_error, int &r_err_pos) const;
	ASTNode _parse_expression(const Vector<Token> &p_tokens, int &r_idx, int p_min_prec, String &r_error, int &r_err_pos) const;
	ASTNode _parse_primary(const Vector<Token> &p_tokens, int &r_idx, String &r_error, int &r_err_pos) const;
	int _get_operator_precedence(TokenType p_type) const;
	bool _is_right_associative(TokenType p_type) const;

	const CachedFormula &_get_or_compile(const String &p_formula) const;
	double _eval_ast(const ASTNode &p_node, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_variables, const Dictionary &p_switches) const;
	double _resolve_battler_stat(const Dictionary &p_battler, const String &p_prop) const;

protected:
	static void _bind_methods();

public:
	static ClockGearsFormulas *get_singleton() { return singleton; }

	// Semilla RNG reproducible (Sección 9.5)
	void set_rng_seed(uint64_t p_seed);
	uint64_t get_rng_seed() const;
	double rand_float();
	int64_t rand_range_int(int64_t p_min, int64_t p_max);

	// 9.1 Cálculo de Parámetros Finales
	int64_t calculate_final_parameter(int p_param_id, int64_t p_base_class_lv, int64_t p_equip_sum, int64_t p_add_bonus_sum, const PackedFloat64Array &p_trait_multipliers, int64_t p_max_hp_mp_limit = 999999, int64_t p_max_stat_limit = 9999) const;

	// 9.2 Curva de Parámetros por Nivel
	int64_t calculate_level_parameter(int p_level, int p_max_level, int64_t p_base_val, int64_t p_max_val, double p_exponent = 1.0, const PackedInt64Array &p_custom_table = PackedInt64Array()) const;

	// 9.3 Curva de Experiencia (Fórmula exacta RPG Maker MZ)
	int64_t calculate_exp_for_level(int p_level, int p_basis = 30, int p_extra = 20, int p_acc_a = 30, int p_acc_b = 30) const;

	// 9.4 & 9.9 Validación y Evaluación de Fórmulas
	Dictionary validate_formula(const String &p_formula) const;
	double evaluate_formula(const String &p_formula, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_variables = Dictionary(), const Dictionary &p_switches = Dictionary()) const;
	void clear_formula_cache();

	// 9.4 Pipeline Completo de Daño
	int64_t calculate_damage(const String &p_formula, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_context = Dictionary(), const Dictionary &p_variables = Dictionary(), const Dictionary &p_switches = Dictionary());

	// 9.5 Probabilidades
	double calculate_hit_chance(int p_skill_success_pct, double p_attacker_hit, double p_target_eva, int p_hit_type) const;
	double calculate_critical_chance(double p_attacker_cri, double p_target_cev) const;
	bool roll_drop(double p_probability, double p_drop_rate_mod = 1.0);
	double calculate_escape_ratio(double p_party_avg_agi, double p_enemy_avg_agi, int p_failed_attempts = 0) const;

	// 9.6 Orden de Turno
	int64_t calculate_action_speed(int64_t p_agi, int64_t p_skill_speed = 0, int64_t p_item_speed = 0);
	Array sort_turn_order(const Array &p_combatants);

	// 9.7 IA de Enemigos
	bool evaluate_ai_condition(int p_cond_type, double p_param1, double p_param2, const Dictionary &p_battle_state) const;
	Dictionary select_enemy_action(const Array &p_patterns, const Dictionary &p_battle_state);

	// 9.8 Movimiento y Tiempo (60 Hz fijo)
	double calculate_distance_per_frame(int p_speed) const;
	int calculate_wait_frames_for_frequency(int p_frequency) const;

	// 9.10 Estados y Buffs
	double calculate_buff_multiplier(int p_buff_level) const;

	// Auto-Tests de Verificación Numérica (Sección 9)
	bool run_self_tests();

	ClockGearsFormulas();
	~ClockGearsFormulas();
};
