#include "clockgears_formulas.h"
#include "clockgears_system_catalog.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"

ClockGearsFormulas *ClockGearsFormulas::singleton = nullptr;

void ClockGearsFormulas::set_rng_seed(uint64_t p_seed) {
	rng.seed(p_seed);
}

uint64_t ClockGearsFormulas::get_rng_seed() const {
	return rng.get_seed();
}

double ClockGearsFormulas::rand_float() {
	return rng.randf();
}

int64_t ClockGearsFormulas::rand_range_int(int64_t p_min, int64_t p_max) {
	if (p_min > p_max) {
		SWAP(p_min, p_max);
	}
	if (p_min == p_max) {
		return p_min;
	}
	uint64_t span = uint64_t(p_max - p_min + 1);
	return p_min + int64_t(rng.rand() % span);
}

// ============================================================================
// 9.1 Parámetros
// ============================================================================
int64_t ClockGearsFormulas::calculate_final_parameter(int p_param_id, int64_t p_base_class_lv, int64_t p_equip_sum, int64_t p_add_bonus_sum, const PackedFloat64Array &p_trait_multipliers, int64_t p_max_hp_mp_limit, int64_t p_max_stat_limit) const {
	double base_total = double(p_base_class_lv + p_equip_sum + p_add_bonus_sum);
	double mult = 1.0;
	for (int i = 0; i < p_trait_multipliers.size(); i++) {
		mult *= p_trait_multipliers[i];
	}
	int64_t raw = int64_t(Math::floor(base_total * mult));
	int64_t min_val = (p_param_id == 1) ? 0 : 1; // MMP permite mínimo 0; MHP y demás mínimo 1
	int64_t max_val = (p_param_id == 0 || p_param_id == 1) ? MAX(int64_t(1), p_max_hp_mp_limit) : MAX(int64_t(1), p_max_stat_limit);
	return CLAMP(raw, min_val, max_val);
}

// ============================================================================
// 9.2 Curva de parámetros por nivel (clase)
// ============================================================================
int64_t ClockGearsFormulas::calculate_level_parameter(int p_level, int p_max_level, int64_t p_base_val, int64_t p_max_val, double p_exponent, const PackedInt64Array &p_custom_table) const {
	int lv = CLAMP(p_level, 1, MAX(1, p_max_level));
	if (!p_custom_table.is_empty() && lv - 1 < p_custom_table.size()) {
		return p_custom_table[lv - 1];
	}
	if (p_max_level <= 1 || lv <= 1) {
		return p_base_val;
	}
	double ratio = double(lv - 1) / double(p_max_level - 1);
	double exp_clamped = MAX(0.01, p_exponent);
	double val = double(p_base_val) + double(p_max_val - p_base_val) * Math::pow(ratio, exp_clamped);
	return int64_t(Math::round(val));
}

// ============================================================================
// 9.3 Experiencia (Fórmula RPG Maker MZ)
// ============================================================================
int64_t ClockGearsFormulas::calculate_exp_for_level(int p_level, int p_basis, int p_extra, int p_acc_a, int p_acc_b) const {
	if (p_level <= 1) {
		return 0;
	}
	double lv = double(p_level);
	double basis = double(MAX(1, p_basis));
	double extra = double(MAX(0, p_extra));
	double acc_a = double(MAX(1, p_acc_a));
	double acc_b = double(MAX(1, p_acc_b));

	double pow_term = Math::pow(lv - 1.0, 0.9 + acc_a / 250.0);
	double denom = 6.0 + (lv * lv) / 50.0 / acc_b;
	if (denom <= 0.0) {
		denom = 6.0;
	}
	double exp_val = basis * pow_term * lv * (lv + 1.0) / denom + (lv - 1.0) * extra;
	if (Math::is_nan(exp_val) || Math::is_inf(exp_val) || exp_val < 0.0) {
		return 0;
	}
	return int64_t(Math::floor(exp_val));
}

// ============================================================================
// 9.9 Tokenizador y Pratt Parser para Fórmulas
// ============================================================================
bool ClockGearsFormulas::_tokenize(const String &p_expr, Vector<Token> &r_tokens, String &r_error, int &r_err_pos) const {
	r_tokens.clear();
	int len = p_expr.length();
	int i = 0;

	while (i < len) {
		char32_t c = p_expr[i];
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			i++;
			continue;
		}

		// Números (enteros o decimales)
		if ((c >= '0' && c <= '9') || (c == '.' && i + 1 < len && p_expr[i + 1] >= '0' && p_expr[i + 1] <= '9')) {
			int start = i;
			bool has_dot = false;
			while (i < len) {
				char32_t nc = p_expr[i];
				if (nc >= '0' && nc <= '9') {
					i++;
				} else if (nc == '.' && !has_dot) {
					has_dot = true;
					i++;
				} else {
					break;
				}
			}
			String num_str = p_expr.substr(start, i - start);
			Token tok;
			tok.type = TOK_NUMBER;
			tok.text = num_str;
			tok.number_val = num_str.to_float();
			tok.pos = start;
			r_tokens.push_back(tok);
			continue;
		}

		// Identificadores (a, b, v, s, max, min, abs, floor, ceil, round, rand, atk, def...)
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
			int start = i;
			while (i < len) {
				char32_t nc = p_expr[i];
				if ((nc >= 'a' && nc <= 'z') || (nc >= 'A' && nc <= 'Z') || (nc >= '0' && nc <= '9') || nc == '_') {
					i++;
				} else {
					break;
				}
			}
			Token tok;
			tok.type = TOK_IDENT;
			tok.text = p_expr.substr(start, i - start).to_lower();
			tok.pos = start;
			r_tokens.push_back(tok);
			continue;
		}

		// Operadores de 2 caracteres
		if (i + 1 < len) {
			char32_t n = p_expr[i + 1];
			if (c == '&' && n == '&') {
				r_tokens.push_back(Token{ TOK_AND, "&&", 0.0, i });
				i += 2;
				continue;
			}
			if (c == '|' && n == '|') {
				r_tokens.push_back(Token{ TOK_OR, "||", 0.0, i });
				i += 2;
				continue;
			}
			if (c == '=' && n == '=') {
				r_tokens.push_back(Token{ TOK_EQ, "==", 0.0, i });
				i += 2;
				continue;
			}
			if (c == '!' && n == '=') {
				r_tokens.push_back(Token{ TOK_NEQ, "!=", 0.0, i });
				i += 2;
				continue;
			}
			if (c == '<' && n == '=') {
				r_tokens.push_back(Token{ TOK_LTE, "<=", 0.0, i });
				i += 2;
				continue;
			}
			if (c == '>' && n == '=') {
				r_tokens.push_back(Token{ TOK_GTE, ">=", 0.0, i });
				i += 2;
				continue;
			}
		}

		// Operadores y símbolos de 1 carácter
		Token tok;
		tok.pos = i;
		tok.text = String::chr(c);
		switch (c) {
			case '.': tok.type = TOK_DOT; break;
			case '(': tok.type = TOK_LPAREN; break;
			case ')': tok.type = TOK_RPAREN; break;
			case '[': tok.type = TOK_LBRACKET; break;
			case ']': tok.type = TOK_RBRACKET; break;
			case ',': tok.type = TOK_COMMA; break;
			case '+': tok.type = TOK_PLUS; break;
			case '-': tok.type = TOK_MINUS; break;
			case '*': tok.type = TOK_STAR; break;
			case '/': tok.type = TOK_SLASH; break;
			case '%': tok.type = TOK_PERCENT; break;
			case '^': tok.type = TOK_CARET; break;
			case '!': tok.type = TOK_BANG; break;
			case '<': tok.type = TOK_LT; break;
			case '>': tok.type = TOK_GT; break;
			case '?': tok.type = TOK_QUESTION; break;
			case ':': tok.type = TOK_COLON; break;
			default:
				r_error = vformat(String::utf8("Carácter inesperado '%s' en posición %d"), String::chr(c), i);
				r_err_pos = i;
				return false;
		}
		r_tokens.push_back(tok);
		i++;
	}

	r_tokens.push_back(Token{ TOK_EOF, "", 0.0, len });
	return true;
}

int ClockGearsFormulas::_get_operator_precedence(TokenType p_type) const {
	switch (p_type) {
		case TOK_QUESTION: return 10;
		case TOK_OR: return 20;
		case TOK_AND: return 30;
		case TOK_EQ:
		case TOK_NEQ: return 40;
		case TOK_LT:
		case TOK_LTE:
		case TOK_GT:
		case TOK_GTE: return 50;
		case TOK_PLUS:
		case TOK_MINUS: return 60;
		case TOK_STAR:
		case TOK_SLASH:
		case TOK_PERCENT: return 70;
		case TOK_CARET: return 80;
		default: return 0;
	}
}

bool ClockGearsFormulas::_is_right_associative(TokenType p_type) const {
	return (p_type == TOK_CARET || p_type == TOK_QUESTION);
}

ClockGearsFormulas::ASTNode ClockGearsFormulas::_parse_primary(const Vector<Token> &p_tokens, int &r_idx, String &r_error, int &r_err_pos) const {
	ASTNode node;
	if (r_idx >= p_tokens.size()) {
		r_error = String::utf8("Fin inesperado de la fórmula");
		r_err_pos = 0;
		return node;
	}

	const Token &tok = p_tokens[r_idx];
	node.pos = tok.pos;

	// Operadores unarios (+, -, !)
	if (tok.type == TOK_PLUS || tok.type == TOK_MINUS || tok.type == TOK_BANG) {
		r_idx++;
		node.type = AST_UNARY;
		node.op = tok.type;
		ASTNode operand = _parse_expression(p_tokens, r_idx, 75, r_error, r_err_pos);
		node.children.push_back(operand);
		return node;
	}

	// Paréntesis (expr)
	if (tok.type == TOK_LPAREN) {
		int open_pos = tok.pos;
		r_idx++;
		ASTNode inner = _parse_expression(p_tokens, r_idx, 1, r_error, r_err_pos);
		if (!r_error.is_empty()) {
			return inner;
		}
		if (p_tokens[r_idx].type != TOK_RPAREN) {
			r_error = vformat(String::utf8("Falta ')' para cerrar '(' en posición %d"), open_pos);
			r_err_pos = p_tokens[r_idx].pos;
			return inner;
		}
		r_idx++;
		return inner;
	}

	// Literal numérico
	if (tok.type == TOK_NUMBER) {
		r_idx++;
		node.type = AST_NUMBER;
		node.number_val = tok.number_val;
		return node;
	}

	// Identificador: a.<stat>, b.<stat>, v[n], s[n], o llamada a función f(...)
	if (tok.type == TOK_IDENT) {
		String ident = tok.text;
		int ident_pos = tok.pos;
		r_idx++;

		// Propiedad de combatiente: a.atk, b.def...
		if (p_tokens[r_idx].type == TOK_DOT) {
			r_idx++;
			if (p_tokens[r_idx].type != TOK_IDENT) {
				r_error = vformat(String::utf8("Se esperaba nombre de atributo tras '%s.' en posición %d"), ident, p_tokens[r_idx].pos);
				r_err_pos = p_tokens[r_idx].pos;
				return node;
			}
			String prop = p_tokens[r_idx].text;
			r_idx++;
			if (ident != "a" && ident != "b") {
				r_error = vformat(String::utf8("Objeto desconocido '%s' en posición %d (use 'a' o 'b')"), ident, ident_pos);
				r_err_pos = ident_pos;
				return node;
			}
			node.type = AST_STAT_REF;
			node.target_name = ident;
			node.prop_name = prop;
			return node;
		}

		// Variable o interruptor indexado: v[n], s[n]
		if (p_tokens[r_idx].type == TOK_LBRACKET) {
			if (ident != "v" && ident != "s") {
				r_error = vformat(String::utf8("Solo 'v[n]' o 's[n]' admiten índice en posición %d"), ident_pos);
				r_err_pos = ident_pos;
				return node;
			}
			int bracket_pos = p_tokens[r_idx].pos;
			r_idx++;
			ASTNode idx_expr = _parse_expression(p_tokens, r_idx, 1, r_error, r_err_pos);
			if (!r_error.is_empty()) {
				return node;
			}
			if (p_tokens[r_idx].type != TOK_RBRACKET) {
				r_error = vformat(String::utf8("Falta ']' para cerrar '[' en posición %d"), bracket_pos);
				r_err_pos = p_tokens[r_idx].pos;
				return node;
			}
			r_idx++;
			node.type = AST_VAR_REF;
			node.target_name = ident;
			node.children.push_back(idx_expr);
			return node;
		}

		// Llamada a función: max, min, abs, floor, ceil, round, rand
		if (p_tokens[r_idx].type == TOK_LPAREN) {
			if (ident != "max" && ident != "min" && ident != "abs" && ident != "floor" && ident != "ceil" && ident != "round" && ident != "rand") {
				r_error = vformat(String::utf8("Función no permitida '%s' en posición %d"), ident, ident_pos);
				r_err_pos = ident_pos;
				return node;
			}
			int open_pos = p_tokens[r_idx].pos;
			r_idx++;
			node.type = AST_FUNC_CALL;
			node.target_name = ident;
			if (p_tokens[r_idx].type != TOK_RPAREN) {
				while (true) {
					ASTNode arg = _parse_expression(p_tokens, r_idx, 1, r_error, r_err_pos);
					if (!r_error.is_empty()) {
						return node;
					}
					node.children.push_back(arg);
					if (p_tokens[r_idx].type == TOK_COMMA) {
						r_idx++;
						continue;
					}
					break;
				}
			}
			if (p_tokens[r_idx].type != TOK_RPAREN) {
				r_error = vformat(String::utf8("Falta ')' en llamada a '%s' abierta en posición %d"), ident, open_pos);
				r_err_pos = p_tokens[r_idx].pos;
				return node;
			}
			r_idx++;
			return node;
		}

		r_error = vformat(String::utf8("Identificador suelto '%s' no válido en posición %d"), ident, ident_pos);
		r_err_pos = ident_pos;
		return node;
	}

	r_error = vformat(String::utf8("Token inesperado '%s' en posición %d"), tok.text, tok.pos);
	r_err_pos = tok.pos;
	return node;
}

ClockGearsFormulas::ASTNode ClockGearsFormulas::_parse_expression(const Vector<Token> &p_tokens, int &r_idx, int p_min_prec, String &r_error, int &r_err_pos) const {
	ASTNode lhs = _parse_primary(p_tokens, r_idx, r_error, r_err_pos);
	if (!r_error.is_empty()) {
		return lhs;
	}

	while (r_idx < p_tokens.size()) {
		TokenType op = p_tokens[r_idx].type;
		int prec = _get_operator_precedence(op);
		if (prec == 0 || prec < p_min_prec) {
			break;
		}

		int op_pos = p_tokens[r_idx].pos;
		r_idx++;

		// Operador ternario: cond ? true_expr : false_expr
		if (op == TOK_QUESTION) {
			ASTNode true_branch = _parse_expression(p_tokens, r_idx, 1, r_error, r_err_pos);
			if (!r_error.is_empty()) {
				return lhs;
			}
			if (p_tokens[r_idx].type != TOK_COLON) {
				r_error = vformat(String::utf8("Falta ':' en operador ternario '?' en posición %d"), op_pos);
				r_err_pos = p_tokens[r_idx].pos;
				return lhs;
			}
			r_idx++;
			ASTNode false_branch = _parse_expression(p_tokens, r_idx, prec, r_error, r_err_pos);
			if (!r_error.is_empty()) {
				return lhs;
			}
			ASTNode tern;
			tern.type = AST_TERNARY;
			tern.op = TOK_QUESTION;
			tern.pos = op_pos;
			tern.children.push_back(lhs);
			tern.children.push_back(true_branch);
			tern.children.push_back(false_branch);
			lhs = tern;
			continue;
		}

		int next_min_prec = _is_right_associative(op) ? prec : (prec + 1);
		ASTNode rhs = _parse_expression(p_tokens, r_idx, next_min_prec, r_error, r_err_pos);
		if (!r_error.is_empty()) {
			return lhs;
		}

		ASTNode bin;
		bin.type = AST_BINARY;
		bin.op = op;
		bin.pos = op_pos;
		bin.children.push_back(lhs);
		bin.children.push_back(rhs);
		lhs = bin;
	}

	return lhs;
}

const ClockGearsFormulas::CachedFormula &ClockGearsFormulas::_get_or_compile(const String &p_formula) const {
	if (ast_cache.has(p_formula)) {
		return ast_cache[p_formula];
	}

	CachedFormula compiled;
	String trimmed = p_formula.strip_edges();
	if (trimmed.is_empty()) {
		compiled.valid = false;
		compiled.error_message = String::utf8("La fórmula está vacía");
		compiled.error_pos = 0;
		ast_cache[p_formula] = compiled;
		return ast_cache[p_formula];
	}

	Vector<Token> tokens;
	String err;
	int err_pos = -1;
	if (!_tokenize(p_formula, tokens, err, err_pos)) {
		compiled.valid = false;
		compiled.error_message = err;
		compiled.error_pos = err_pos;
		ast_cache[p_formula] = compiled;
		return ast_cache[p_formula];
	}

	int idx = 0;
	ASTNode root = _parse_expression(tokens, idx, 1, err, err_pos);
	if (err.is_empty() && tokens[idx].type != TOK_EOF) {
		err = vformat(String::utf8("Token extra inesperado '%s' en posición %d"), tokens[idx].text, tokens[idx].pos);
		err_pos = tokens[idx].pos;
	}

	if (!err.is_empty()) {
		compiled.valid = false;
		compiled.error_message = err;
		compiled.error_pos = err_pos;
	} else {
		compiled.valid = true;
		compiled.root = root;
	}

	ast_cache[p_formula] = compiled;
	return ast_cache[p_formula];
}

double ClockGearsFormulas::_resolve_battler_stat(const Dictionary &p_battler, const String &p_prop) const {
	if (p_battler.has(p_prop)) {
		return double(p_battler[p_prop]);
	}
	// Alias compatibles con RPG Maker MZ y ClockGears Battler
	if (p_prop == "mat" && p_battler.has("matk")) {
		return double(p_battler["matk"]);
	}
	if (p_prop == "matk" && p_battler.has("mat")) {
		return double(p_battler["mat"]);
	}
	if (p_prop == "mdf" && p_battler.has("mdef")) {
		return double(p_battler["mdef"]);
	}
	if (p_prop == "mdef" && p_battler.has("mdf")) {
		return double(p_battler["mdf"]);
	}
	if (p_prop == "mhp" && p_battler.has("max_hp")) {
		return double(p_battler["max_hp"]);
	}
	if (p_prop == "mmp" && p_battler.has("max_mp")) {
		return double(p_battler["max_mp"]);
	}
	return 0.0;
}

double ClockGearsFormulas::_eval_ast(const ASTNode &p_node, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_variables, const Dictionary &p_switches) const {
	switch (p_node.type) {
		case AST_NUMBER:
			return p_node.number_val;

		case AST_STAT_REF: {
			const Dictionary &target = (p_node.target_name == "a") ? p_attacker : p_defender;
			return _resolve_battler_stat(target, p_node.prop_name);
		}

		case AST_VAR_REF: {
			if (p_node.children.is_empty()) {
				return 0.0;
			}
			int idx = int(Math::floor(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches)));
			const Dictionary &dict = (p_node.target_name == "v") ? p_variables : p_switches;
			if (dict.has(idx)) {
				return double(dict[idx]);
			}
			String key_str = itos(idx);
			if (dict.has(key_str)) {
				return double(dict[key_str]);
			}
			return 0.0;
		}

		case AST_UNARY: {
			if (p_node.children.is_empty()) {
				return 0.0;
			}
			double v = _eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches);
			if (p_node.op == TOK_MINUS) {
				return -v;
			}
			if (p_node.op == TOK_BANG) {
				return (Math::is_zero_approx(v)) ? 1.0 : 0.0;
			}
			return v;
		}

		case AST_BINARY: {
			if (p_node.children.size() < 2) {
				return 0.0;
			}
			double l = _eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches);
			if (p_node.op == TOK_AND) {
				if (Math::is_zero_approx(l)) {
					return 0.0;
				}
				double r = _eval_ast(p_node.children[1], p_attacker, p_defender, p_variables, p_switches);
				return (!Math::is_zero_approx(r)) ? 1.0 : 0.0;
			}
			if (p_node.op == TOK_OR) {
				if (!Math::is_zero_approx(l)) {
					return 1.0;
				}
				double r = _eval_ast(p_node.children[1], p_attacker, p_defender, p_variables, p_switches);
				return (!Math::is_zero_approx(r)) ? 1.0 : 0.0;
			}

			double r = _eval_ast(p_node.children[1], p_attacker, p_defender, p_variables, p_switches);
			switch (p_node.op) {
				case TOK_PLUS: return l + r;
				case TOK_MINUS: return l - r;
				case TOK_STAR: return l * r;
				case TOK_SLASH: {
					if (Math::is_zero_approx(r)) {
						if (!suppress_div_zero_warning) {
							WARN_PRINT(String::utf8("ClockGearsFormulas: División entre cero detectada; devolviendo 0."));
						}
						return 0.0;
					}
					return l / r;
				}
				case TOK_PERCENT: {
					if (Math::is_zero_approx(r)) {
						if (!suppress_div_zero_warning) {
							WARN_PRINT(String::utf8("ClockGearsFormulas: Módulo entre cero detectada; devolviendo 0."));
						}
						return 0.0;
					}
					return Math::fmod(l, r);
				}
				case TOK_CARET: return Math::pow(l, r);
				case TOK_LT: return (l < r) ? 1.0 : 0.0;
				case TOK_LTE: return (l <= r) ? 1.0 : 0.0;
				case TOK_GT: return (l > r) ? 1.0 : 0.0;
				case TOK_GTE: return (l >= r) ? 1.0 : 0.0;
				case TOK_EQ: return Math::is_equal_approx(l, r) ? 1.0 : 0.0;
				case TOK_NEQ: return (!Math::is_equal_approx(l, r)) ? 1.0 : 0.0;
				default: return 0.0;
			}
		}

		case AST_TERNARY: {
			if (p_node.children.size() < 3) {
				return 0.0;
			}
			double cond = _eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches);
			if (!Math::is_zero_approx(cond)) {
				return _eval_ast(p_node.children[1], p_attacker, p_defender, p_variables, p_switches);
			} else {
				return _eval_ast(p_node.children[2], p_attacker, p_defender, p_variables, p_switches);
			}
		}

		case AST_FUNC_CALL: {
			const String &fn = p_node.target_name;
			int argc = p_node.children.size();
			if (fn == "max") {
				if (argc == 0) return 0.0;
				double best = _eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches);
				for (int i = 1; i < argc; i++) {
					double v = _eval_ast(p_node.children[i], p_attacker, p_defender, p_variables, p_switches);
					if (v > best) best = v;
				}
				return best;
			}
			if (fn == "min") {
				if (argc == 0) return 0.0;
				double best = _eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches);
				for (int i = 1; i < argc; i++) {
					double v = _eval_ast(p_node.children[i], p_attacker, p_defender, p_variables, p_switches);
					if (v < best) best = v;
				}
				return best;
			}
			if (fn == "abs" && argc >= 1) {
				return Math::abs(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches));
			}
			if (fn == "floor" && argc >= 1) {
				return Math::floor(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches));
			}
			if (fn == "ceil" && argc >= 1) {
				return Math::ceil(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches));
			}
			if (fn == "round" && argc >= 1) {
				return Math::round(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches));
			}
			if (fn == "rand") {
				ClockGearsFormulas *mutable_self = const_cast<ClockGearsFormulas *>(this);
				if (argc == 0) {
					return mutable_self->rand_float();
				} else if (argc == 1) {
					int64_t max_v = int64_t(Math::floor(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches)));
					return double(mutable_self->rand_range_int(0, MAX(int64_t(0), max_v)));
				} else {
					int64_t min_v = int64_t(Math::floor(_eval_ast(p_node.children[0], p_attacker, p_defender, p_variables, p_switches)));
					int64_t max_v = int64_t(Math::floor(_eval_ast(p_node.children[1], p_attacker, p_defender, p_variables, p_switches)));
					return double(mutable_self->rand_range_int(min_v, max_v));
				}
			}
			return 0.0;
		}
	}
	return 0.0;
}

Dictionary ClockGearsFormulas::validate_formula(const String &p_formula) const {
	const CachedFormula &cf = _get_or_compile(p_formula);
	Dictionary res;
	res["valid"] = cf.valid;
	res["error"] = cf.error_message;
	res["error_pos"] = cf.error_pos;
	return res;
}

double ClockGearsFormulas::evaluate_formula(const String &p_formula, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_variables, const Dictionary &p_switches) const {
	const CachedFormula &cf = _get_or_compile(p_formula);
	if (!cf.valid) {
		ERR_PRINT(vformat(String::utf8("ClockGearsFormulas: Fórmula inválida '%s' -> %s (pos %d)"), p_formula, cf.error_message, cf.error_pos));
		return 0.0;
	}
	double val = _eval_ast(cf.root, p_attacker, p_defender, p_variables, p_switches);
	if (Math::is_nan(val) || Math::is_inf(val)) {
		return 0.0;
	}
	return val;
}

void ClockGearsFormulas::clear_formula_cache() {
	ast_cache.clear();
}

// ============================================================================
// 9.4 Pipeline Completo de Daño
// ============================================================================
int64_t ClockGearsFormulas::calculate_damage(const String &p_formula, const Dictionary &p_attacker, const Dictionary &p_defender, const Dictionary &p_context, const Dictionary &p_variables, const Dictionary &p_switches) {
	String expr = p_formula.strip_edges().is_empty() ? "max(a.atk * 4 - b.def * 2, 0)" : p_formula;
	double dmg = evaluate_formula(expr, p_attacker, p_defender, p_variables, p_switches);
	if (Math::is_nan(dmg) || Math::is_inf(dmg)) {
		return 0;
	}

	// 1. Multiplicador de elemento (máximo entre elementos aplicables)
	double element_rate = double(p_context.get("element_rate", 1.0));
	if (p_context.has("element_rates")) {
		Array rates = p_context["element_rates"];
		if (!rates.is_empty()) {
			element_rate = double(rates[0]);
			for (int i = 1; i < rates.size(); i++) {
				double r = double(rates[i]);
				if (r > element_rate) {
					element_rate = r;
				}
			}
		}
	}
	dmg *= element_rate;

	// 2. Multiplicador de crítico (x3 por defecto, modulado por 1 - CEV si se especifica)
	bool is_critical = bool(p_context.get("is_critical", false));
	if (is_critical) {
		double crit_mult = double(p_context.get("critical_multiplier", 3.0));
		double cev = CLAMP(double(p_context.get("target_cev", 0.0)), 0.0, 1.0);
		if (bool(p_context.get("apply_cev_to_crit_damage", false))) {
			crit_mult *= (1.0 - cev);
		}
		dmg *= crit_mult;
	}

	// 3. Guardia (÷2, ajustado por GRD)
	bool is_guard = bool(p_context.get("is_guard", false));
	if (is_guard) {
		double grd = MAX(0.01, double(p_context.get("grd", 1.0)));
		dmg = dmg / (2.0 * grd);
	}

	// 4. PDR / MDR (Daño físico o mágico recibido)
	int hit_type = int(p_context.get("hit_type", 1)); // 0=Seguro, 1=Físico, 2=Mágico
	if (hit_type == 1) {
		double pdr = double(p_context.get("pdr", 1.0));
		dmg *= pdr;
	} else if (hit_type == 2) {
		double mdr = double(p_context.get("mdr", 1.0));
		dmg *= mdr;
	}

	// 5. FDR (Daño de suelo, si aplica)
	if (bool(p_context.get("is_floor_damage", false))) {
		double fdr = double(p_context.get("fdr", 1.0));
		dmg *= fdr;
	}

	// 6. Varianza: dmg = dmg + dmg * rand(-var, +var) / 100
	int variance = CLAMP(int(p_context.get("variance", 0)), 0, 100);
	if (variance > 0) {
		int64_t delta_pct;
		if (p_context.has("forced_variance_roll")) {
			delta_pct = CLAMP(int64_t(p_context["forced_variance_roll"]), int64_t(-variance), int64_t(variance));
		} else {
			delta_pct = rand_range_int(-variance, variance);
		}
		dmg = dmg + dmg * (double(delta_pct) / 100.0);
	}

	if (Math::is_nan(dmg) || Math::is_inf(dmg)) {
		return 0;
	}
	return int64_t(Math::round(dmg));
}

// ============================================================================
// 9.5 Probabilidades
// ============================================================================
double ClockGearsFormulas::calculate_hit_chance(int p_skill_success_pct, double p_attacker_hit, double p_target_eva, int p_hit_type) const {
	double base_rate = CLAMP(double(p_skill_success_pct) / 100.0, 0.0, 1.0);
	if (p_hit_type == 0) {
		// Golpe seguro: ignora HIT y EVA/MEV
		return base_rate;
	}
	double hit = base_rate * MAX(0.0, p_attacker_hit);
	double eva = CLAMP(p_target_eva, 0.0, 1.0);
	return CLAMP(hit * (1.0 - eva), 0.0, 1.0);
}

double ClockGearsFormulas::calculate_critical_chance(double p_attacker_cri, double p_target_cev) const {
	return CLAMP(p_attacker_cri * (1.0 - CLAMP(p_target_cev, 0.0, 1.0)), 0.0, 1.0);
}

bool ClockGearsFormulas::roll_drop(double p_probability, double p_drop_rate_mod) {
	double final_prob = CLAMP(p_probability * p_drop_rate_mod, 0.0, 1.0);
	return rand_float() < final_prob;
}

double ClockGearsFormulas::calculate_escape_ratio(double p_party_avg_agi, double p_enemy_avg_agi, int p_failed_attempts) const {
	double denom = MAX(1.0, p_enemy_avg_agi);
	double base_ratio = 0.5 * (MAX(1.0, p_party_avg_agi) / denom);
	double bonus = 0.10 * double(MAX(0, p_failed_attempts));
	return CLAMP(base_ratio + bonus, 0.0, 1.0);
}

// ============================================================================
// 9.6 Orden de turno
// ============================================================================
int64_t ClockGearsFormulas::calculate_action_speed(int64_t p_agi, int64_t p_skill_speed, int64_t p_item_speed) {
	int64_t agi = MAX(int64_t(0), p_agi);
	int64_t rand_bonus = rand_range_int(0, 5 + agi / 4);
	return agi + rand_bonus + p_skill_speed + p_item_speed;
}

Array ClockGearsFormulas::sort_turn_order(const Array &p_combatants) {
	struct TurnEntry {
		int original_index;
		int64_t speed;
		Variant data;
	};
	Vector<TurnEntry> entries;
	entries.resize(p_combatants.size());
	for (int i = 0; i < p_combatants.size(); i++) {
		TurnEntry e;
		e.original_index = i;
		e.data = p_combatants[i];
		e.speed = 0;
		if (p_combatants[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = p_combatants[i];
			if (d.has("action_speed")) {
				e.speed = int64_t(d["action_speed"]);
			} else {
				int64_t agi = int64_t(d.get("agi", 1));
				int64_t sk_spd = int64_t(d.get("skill_speed", 0));
				int64_t it_spd = int64_t(d.get("item_speed", 0));
				e.speed = calculate_action_speed(agi, sk_spd, it_spd);
				d["action_speed"] = e.speed;
				e.data = d;
			}
		}
		entries.write[i] = e;
	}

	// Ordenamiento por inserción estable (descendente por velocidad, desempate por original_index)
	for (int i = 1; i < entries.size(); i++) {
		TurnEntry key = entries[i];
		int j = i - 1;
		while (j >= 0 && (entries[j].speed < key.speed || (entries[j].speed == key.speed && entries[j].original_index > key.original_index))) {
			entries.write[j + 1] = entries[j];
			j--;
		}
		entries.write[j + 1] = key;
	}

	Array sorted;
	for (int i = 0; i < entries.size(); i++) {
		sorted.push_back(entries[i].data);
	}
	return sorted;
}

// ============================================================================
// 9.7 IA de enemigos
// ============================================================================
bool ClockGearsFormulas::evaluate_ai_condition(int p_cond_type, double p_param1, double p_param2, const Dictionary &p_battle_state) const {
	switch (p_cond_type) {
		case CG_AI_COND_ALWAYS:
			return true;
		case CG_AI_COND_TURN: {
			int turn = int(p_battle_state.get("turn", 0));
			int a = int(p_param1);
			int b = int(p_param2);
			if (b == 0) {
				return turn == a;
			}
			return (turn >= a) && ((turn - a) % b == 0);
		}
		case CG_AI_COND_HP_RANGE: {
			double hp_pct = double(p_battle_state.get("hp_pct", 100.0));
			return hp_pct >= p_param1 && hp_pct <= p_param2;
		}
		case CG_AI_COND_MP_RANGE: {
			double mp_pct = double(p_battle_state.get("mp_pct", 100.0));
			return mp_pct >= p_param1 && mp_pct <= p_param2;
		}
		case CG_AI_COND_STATE: {
			int state_id = int(p_param1);
			Array active_states = p_battle_state.get("states", Array());
			return active_states.has(state_id);
		}
		case CG_AI_COND_PARTY_LEVEL: {
			int party_lv = int(p_battle_state.get("party_highest_level", 1));
			return party_lv >= int(p_param1);
		}
		case CG_AI_COND_SWITCH: {
			int sw_id = int(p_param1);
			Dictionary switches = p_battle_state.get("switches", Dictionary());
			if (switches.has(sw_id)) {
				return bool(switches[sw_id]);
			}
			return bool(switches.get(itos(sw_id), false));
		}
		default:
			return true;
	}
}

Dictionary ClockGearsFormulas::select_enemy_action(const Array &p_patterns, const Dictionary &p_battle_state) {
	Vector<Dictionary> valid_patterns;
	int max_rating = -1;

	for (int i = 0; i < p_patterns.size(); i++) {
		if (p_patterns[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary pat = p_patterns[i];
		int cond_type = int(pat.get("condition_type", CG_AI_COND_ALWAYS));
		double p1 = double(pat.get("param1", 0.0));
		double p2 = double(pat.get("param2", 100.0));
		if (evaluate_ai_condition(cond_type, p1, p2, p_battle_state)) {
			int rating = CLAMP(int(pat.get("rating", 5)), 1, 10);
			valid_patterns.push_back(pat);
			if (rating > max_rating) {
				max_rating = rating;
			}
		}
	}

	// Si ninguna condición es válida, acción por defecto: atacar (skill_id = 1)
	if (valid_patterns.is_empty()) {
		Dictionary fallback;
		fallback["skill_id"] = 1;
		fallback["rating"] = 5;
		fallback["is_default_attack"] = true;
		return fallback;
	}

	// Filtrar candidatos con rating >= max_rating - 3 y asignar peso = MAX(1, rating - (max_rating - 3))
	int threshold = max_rating - 3;
	Vector<Dictionary> weighted_candidates;
	Vector<int> weights;
	int total_weight = 0;

	for (int i = 0; i < valid_patterns.size(); i++) {
		int rating = CLAMP(int(valid_patterns[i].get("rating", 5)), 1, 10);
		if (rating >= threshold) {
			int weight = MAX(1, rating - threshold);
			weighted_candidates.push_back(valid_patterns[i]);
			weights.push_back(weight);
			total_weight += weight;
		}
	}

	if (weighted_candidates.is_empty() || total_weight <= 0) {
		return valid_patterns[0];
	}

	int64_t roll = rand_range_int(0, total_weight - 1);
	int accum = 0;
	for (int i = 0; i < weighted_candidates.size(); i++) {
		accum += weights[i];
		if (roll < accum) {
			return weighted_candidates[i];
		}
	}
	return weighted_candidates[weighted_candidates.size() - 1];
}

// ============================================================================
// 9.8 Movimiento y tiempo
// ============================================================================
double ClockGearsFormulas::calculate_distance_per_frame(int p_speed) const {
	int spd = CLAMP(p_speed, 1, 6);
	return Math::pow(2.0, double(spd)) / 256.0;
}

int ClockGearsFormulas::calculate_wait_frames_for_frequency(int p_frequency) const {
	int freq = CLAMP(p_frequency, 1, 5);
	return (5 - freq) * 30;
}

// ============================================================================
// 9.10 Estados y buffs
// ============================================================================
double ClockGearsFormulas::calculate_buff_multiplier(int p_buff_level) const {
	int lv = CLAMP(p_buff_level, -2, 2);
	return 1.0 + 0.25 * double(lv);
}

// ============================================================================
// Auto-Tests Internos (Sección 9)
// ============================================================================
bool ClockGearsFormulas::run_self_tests() {
	set_rng_seed(123456789ULL);

	// Test 1: Parámetros finales (9.1)
	PackedFloat64Array mults;
	mults.push_back(1.1);
	mults.push_back(1.2);
	int64_t final_atk = calculate_final_parameter(2, 100, 20, 5, mults); // floor(125 * 1.32) = 165
	ERR_FAIL_COND_V_MSG(final_atk != 165, false, vformat(String::utf8("SelfTest 1 falló: esperado 165, obtenido %d"), final_atk));

	// Test 2: Curva de parámetros por nivel (9.2)
	int64_t lv50_stat = calculate_level_parameter(50, 99, 10, 108, 1.0); // 10 + 98 * (49/98) = 59
	ERR_FAIL_COND_V_MSG(lv50_stat != 59, false, vformat(String::utf8("SelfTest 2 falló: esperado 59, obtenido %d"), lv50_stat));

	// Test 3: Curva de EXP RPG Maker MZ (9.3)
	int64_t exp_lv1 = calculate_exp_for_level(1);
	int64_t exp_lv2 = calculate_exp_for_level(2, 30, 20, 30, 30);
	ERR_FAIL_COND_V_MSG(exp_lv1 != 0, false, String::utf8("SelfTest 3a falló: EXP nivel 1 debe ser 0"));
	ERR_FAIL_COND_V_MSG(exp_lv2 <= 0, false, String::utf8("SelfTest 3b falló: EXP nivel 2 debe ser > 0"));

	// Test 4: Parser de fórmulas + precedencia + ternario + división entre cero (9.4 & 9.9)
	Dictionary a;
	a["atk"] = 50;
	a["mat"] = 40;
	Dictionary b;
	b["def"] = 30;
	b["mdf"] = 20;
	Dictionary vars;
	vars[1] = 10;

	double dmg_basic = evaluate_formula("max(a.atk * 4 - b.def * 2, 0)", a, b, vars);
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(dmg_basic, 140.0), false, vformat(String::utf8("SelfTest 4a falló: esperado 140, obtenido %f"), dmg_basic));

	double dmg_complex = evaluate_formula("(a.atk > b.def ? a.atk * 2 : b.def) + v[1] * 3", a, b, vars);
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(dmg_complex, 130.0), false, vformat(String::utf8("SelfTest 4b falló: esperado 130, obtenido %f"), dmg_complex));

	suppress_div_zero_warning = true;
	double div_zero = evaluate_formula("a.atk / (b.def - 30)", a, b, vars);
	suppress_div_zero_warning = false;
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(div_zero, 0.0), false, String::utf8("SelfTest 4c falló: división entre 0 debe devolver 0"));

	Dictionary bad_check = validate_formula("a.atk * (4 + ");
	ERR_FAIL_COND_V_MSG(bool(bad_check["valid"]) != false, false, String::utf8("SelfTest 4d falló: fórmula incompleta debe ser inválida"));

	// Test 5: Pipeline completo de daño con elemento x2, crítico x3, guardia ÷2 y varianza 0 (9.4)
	Dictionary ctx;
	ctx["element_rate"] = 2.0;
	ctx["is_critical"] = true;
	ctx["critical_multiplier"] = 3.0;
	ctx["is_guard"] = true;
	ctx["grd"] = 1.0;
	ctx["variance"] = 0;
	int64_t total_dmg = calculate_damage("max(a.atk * 4 - b.def * 2, 0)", a, b, ctx, vars);
	// 140 * 2.0 (elem) * 3.0 (crit) / 2.0 (guard) = 420
	ERR_FAIL_COND_V_MSG(total_dmg != 420, false, vformat(String::utf8("SelfTest 5 falló: esperado 420, obtenido %d"), total_dmg));

	// Test 6: Movimiento y Buffs (9.8 & 9.10)
	double dist_spd4 = calculate_distance_per_frame(4); // 16 / 256 = 0.0625
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(dist_spd4, 0.0625), false, String::utf8("SelfTest 6a falló"));
	int wait_f3 = calculate_wait_frames_for_frequency(3); // (5-3)*30 = 60
	ERR_FAIL_COND_V_MSG(wait_f3 != 60, false, String::utf8("SelfTest 6b falló"));
	double buff_lv2 = calculate_buff_multiplier(2); // 1.5
	ERR_FAIL_COND_V_MSG(!Math::is_equal_approx(buff_lv2, 1.5), false, String::utf8("SelfTest 6c falló"));

	return true;
}

void ClockGearsFormulas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rng_seed", "seed"), &ClockGearsFormulas::set_rng_seed);
	ClassDB::bind_method(D_METHOD("get_rng_seed"), &ClockGearsFormulas::get_rng_seed);
	ClassDB::bind_method(D_METHOD("calculate_final_parameter", "param_id", "base_class_lv", "equip_sum", "add_bonus_sum", "trait_multipliers", "max_hp_mp_limit", "max_stat_limit"), &ClockGearsFormulas::calculate_final_parameter, DEFVAL(999999), DEFVAL(9999));
	ClassDB::bind_method(D_METHOD("calculate_level_parameter", "level", "max_level", "base_val", "max_val", "exponent", "custom_table"), &ClockGearsFormulas::calculate_level_parameter, DEFVAL(1.0), DEFVAL(PackedInt64Array()));
	ClassDB::bind_method(D_METHOD("calculate_exp_for_level", "level", "basis", "extra", "acc_a", "acc_b"), &ClockGearsFormulas::calculate_exp_for_level, DEFVAL(30), DEFVAL(20), DEFVAL(30), DEFVAL(30));
	ClassDB::bind_method(D_METHOD("validate_formula", "formula"), &ClockGearsFormulas::validate_formula);
	ClassDB::bind_method(D_METHOD("evaluate_formula", "formula", "attacker", "defender", "variables", "switches"), &ClockGearsFormulas::evaluate_formula, DEFVAL(Dictionary()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("clear_formula_cache"), &ClockGearsFormulas::clear_formula_cache);
	ClassDB::bind_method(D_METHOD("calculate_damage", "formula", "attacker", "defender", "context", "variables", "switches"), &ClockGearsFormulas::calculate_damage, DEFVAL(Dictionary()), DEFVAL(Dictionary()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("calculate_hit_chance", "skill_success_pct", "attacker_hit", "target_eva", "hit_type"), &ClockGearsFormulas::calculate_hit_chance);
	ClassDB::bind_method(D_METHOD("calculate_critical_chance", "attacker_cri", "target_cev"), &ClockGearsFormulas::calculate_critical_chance);
	ClassDB::bind_method(D_METHOD("roll_drop", "probability", "drop_rate_mod"), &ClockGearsFormulas::roll_drop, DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("calculate_escape_ratio", "party_avg_agi", "enemy_avg_agi", "failed_attempts"), &ClockGearsFormulas::calculate_escape_ratio, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("calculate_action_speed", "agi", "skill_speed", "item_speed"), &ClockGearsFormulas::calculate_action_speed, DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("sort_turn_order", "combatants"), &ClockGearsFormulas::sort_turn_order);
	ClassDB::bind_method(D_METHOD("evaluate_ai_condition", "cond_type", "param1", "param2", "battle_state"), &ClockGearsFormulas::evaluate_ai_condition);
	ClassDB::bind_method(D_METHOD("select_enemy_action", "patterns", "battle_state"), &ClockGearsFormulas::select_enemy_action);
	ClassDB::bind_method(D_METHOD("calculate_distance_per_frame", "speed"), &ClockGearsFormulas::calculate_distance_per_frame);
	ClassDB::bind_method(D_METHOD("calculate_wait_frames_for_frequency", "frequency"), &ClockGearsFormulas::calculate_wait_frames_for_frequency);
	ClassDB::bind_method(D_METHOD("calculate_buff_multiplier", "buff_level"), &ClockGearsFormulas::calculate_buff_multiplier);
	ClassDB::bind_method(D_METHOD("run_self_tests"), &ClockGearsFormulas::run_self_tests);
}

ClockGearsFormulas::ClockGearsFormulas() {
	singleton = this;
	rng.randomize();
}

ClockGearsFormulas::~ClockGearsFormulas() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
