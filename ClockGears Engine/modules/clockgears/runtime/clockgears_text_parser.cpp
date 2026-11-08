#include "clockgears_text_parser.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_system_catalog.h"
#include "core/string/string_builder.h"

void ClockGearsTextParser::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("resolve_data_escapes", "text"), &ClockGearsTextParser::resolve_data_escapes);
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("to_bbcode", "text"), &ClockGearsTextParser::to_bbcode);
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("strip_escapes", "text"), &ClockGearsTextParser::strip_escapes);
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("parse_to_dict", "text", "default_speaker"), &ClockGearsTextParser::parse_to_dict, DEFVAL(""));
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("get_palette_color", "idx"), &ClockGearsTextParser::get_palette_color);
	ClassDB::bind_static_method("ClockGearsTextParser", D_METHOD("get_palette_hex", "idx"), &ClockGearsTextParser::get_palette_hex);

	BIND_CONSTANT(TOKEN_CHAR);
	BIND_CONSTANT(TOKEN_PAUSE_SHORT);
	BIND_CONSTANT(TOKEN_PAUSE_LONG);
	BIND_CONSTANT(TOKEN_WAIT_INPUT);
	BIND_CONSTANT(TOKEN_WAIT_FRAMES);
	BIND_CONSTANT(TOKEN_INSTANT_ON);
	BIND_CONSTANT(TOKEN_INSTANT_OFF);
	BIND_CONSTANT(TOKEN_NO_INPUT_WAIT);
	BIND_CONSTANT(TOKEN_SPEED);
	BIND_CONSTANT(TOKEN_FACE_INDEX);
	BIND_CONSTANT(TOKEN_EXPRESSION);
	BIND_CONSTANT(TOKEN_NAME);
	BIND_CONSTANT(TOKEN_PLAY_SE);
	BIND_CONSTANT(TOKEN_GOLD_WINDOW);
	BIND_CONSTANT(TOKEN_SILENCE_BLIPS);
}

Dictionary ClockGearsTextParser::parse_to_dict(const String &p_text, const String &p_default_speaker) {
	ParsedMessage pm = parse_full(p_text, p_default_speaker);
	Dictionary d;
	d["bbcode_text"] = pm.bbcode_text;
	d["plain_text"] = pm.plain_text;
	d["speaker_name"] = pm.speaker_name;
	d["face_index"] = pm.face_index;
	d["face_expression"] = pm.face_expression;
	d["no_input_wait"] = pm.no_input_wait;
	d["show_gold"] = pm.show_gold;
	d["silence_blips"] = pm.silence_blips;
	d["typing_speed"] = pm.typing_speed;

	Array tokens;
	for (int i = 0; i < pm.execution_tokens.size(); i++) {
		Dictionary td;
		td["type"] = int(pm.execution_tokens[i].type);
		td["int_param"] = pm.execution_tokens[i].int_param;
		td["str_param"] = pm.execution_tokens[i].str_param;
		tokens.push_back(td);
	}
	d["execution_tokens"] = tokens;
	return d;
}

Color ClockGearsTextParser::get_palette_color(int p_idx) {
	static const Color palette[32] = {
		Color(1.0f, 1.0f, 1.0f, 1.0f),       // 0: Blanco normal
		Color(0.12f, 0.63f, 0.84f, 1.0f),     // 1: Azul celeste
		Color(1.0f, 0.47f, 0.29f, 1.0f),      // 2: Naranja suave
		Color(0.40f, 0.80f, 0.25f, 1.0f),     // 3: Verde
		Color(0.50f, 0.75f, 0.94f, 1.0f),     // 4: Cian
		Color(0.82f, 0.50f, 0.75f, 1.0f),     // 5: Magenta
		Color(1.0f, 1.0f, 0.38f, 1.0f),       // 6: Amarillo
		Color(0.55f, 0.55f, 0.55f, 1.0f),     // 7: Gris medio
		Color(0.75f, 0.75f, 0.75f, 1.0f),     // 8: Gris claro
		Color(0.13f, 0.50f, 0.80f, 1.0f),     // 9: Azul intenso
		Color(1.0f, 0.19f, 0.06f, 1.0f),      // 10: Rojo crítico
		Color(0.13f, 0.75f, 0.25f, 1.0f),     // 11: Verde brillante
		Color(0.19f, 0.69f, 0.88f, 1.0f),     // 12: Celeste
		Color(0.69f, 0.44f, 0.75f, 1.0f),     // 13: Púrpura
		Color(0.91f, 0.75f, 0.25f, 1.0f),     // 14: Oro
		Color(0.10f, 0.10f, 0.10f, 1.0f),     // 15: Oscuro
		Color(0.20f, 0.40f, 0.75f, 1.0f),     // 16: Azul noche
		Color(0.75f, 0.20f, 0.20f, 1.0f),     // 17: Carmesí
		Color(0.25f, 0.60f, 0.25f, 1.0f),     // 18: Bosque
		Color(0.85f, 0.65f, 0.20f, 1.0f),     // 19: Bronce
		Color(0.60f, 0.25f, 0.80f, 1.0f),     // 20: Amatista
		Color(0.25f, 0.75f, 0.75f, 1.0f),     // 21: Turquesa
		Color(0.95f, 0.55f, 0.35f, 1.0f),     // 22: Salmón
		Color(0.95f, 0.85f, 0.40f, 1.0f),     // 23: Vainilla
		Color(0.40f, 0.65f, 0.95f, 1.0f),     // 24: Zafiro
		Color(0.65f, 0.95f, 0.40f, 1.0f),     // 25: Lima
		Color(0.95f, 0.40f, 0.65f, 1.0f),     // 26: Rosa
		Color(0.40f, 0.95f, 0.65f, 1.0f),     // 27: Menta
		Color(0.65f, 0.40f, 0.95f, 1.0f),     // 28: Violeta
		Color(0.90f, 0.90f, 0.90f, 1.0f),     // 29: Platino
		Color(0.35f, 0.35f, 0.35f, 1.0f),     // 30: Carbón
		Color(0.88f, 0.63f, 0.25f, 1.0f),     // 31: ClockGears Gold (#e0a040)
	};
	if (p_idx >= 0 && p_idx < 32) {
		return palette[p_idx];
	}
	return palette[0];
}

String ClockGearsTextParser::get_palette_hex(int p_idx) {
	Color c = get_palette_color(p_idx);
	return c.to_html(false);
}

String ClockGearsTextParser::resolve_data_escapes(const String &p_text) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String text = p_text;

	// Reemplazo iterativo hasta 4 niveles para permitir expansiones anidadas (ej: \V[\V[1]])
	for (int pass = 0; pass < 4; pass++) {
		bool any_replaced = false;
		String res = "";
		int i = 0;
		int len = text.length();

		while (i < len) {
			if (text[i] == '\\' && i + 1 < len) {
				char32_t next = text[i + 1];

				// 1. Doble barra: escapar temporalmente
				if (next == '\\') {
					res += String::chr(1); // placeholder
					i += 2;
					any_replaced = true;
					continue;
				}

				// 2. \V[n] -> Variable
				if (next == 'V' && i + 2 < len && text[i + 2] == '[') {
					int close = text.find_char(']', i + 3);
					if (close != -1) {
						String id_str = text.substr(i + 3, close - (i + 3));
						int var_id = id_str.to_int();
						int val = dm ? dm->get_variable(itos(var_id)) : 0;
						res += itos(val);
						i = close + 1;
						any_replaced = true;
						continue;
					}
				}

				// 3. \N[n] -> Nombre de Actor
				if (next == 'N' && i + 2 < len && text[i + 2] == '[') {
					int close = text.find_char(']', i + 3);
					if (close != -1) {
						String id_str = text.substr(i + 3, close - (i + 3));
						int actor_id = id_str.to_int();
						String actor_name = "";
						if (dm) {
							Array actors = dm->get_category_array(CAT_ACTORS);
							for (int a = 0; a < actors.size(); a++) {
								Dictionary act = actors[a];
								if (int(act.get("id", 0)) == actor_id || (actor_id <= 0 && a == 0)) {
									actor_name = act.get("name", "");
									break;
								}
							}
						}
						if (actor_name.is_empty()) {
							actor_name = vformat("Actor%03d", actor_id);
						}
						res += actor_name;
						i = close + 1;
						any_replaced = true;
						continue;
					}
				}

				// 4. \P[n] -> Miembro del Grupo (1..N)
				if (next == 'P' && i + 2 < len && text[i + 2] == '[') {
					int close = text.find_char(']', i + 3);
					if (close != -1) {
						String id_str = text.substr(i + 3, close - (i + 3));
						int member_idx = id_str.to_int();
						String m_name = "";
						if (dm) {
							Array actors = dm->get_category_array(CAT_ACTORS);
							int idx = MAX(0, member_idx - 1);
							if (idx < actors.size()) {
								Dictionary act = actors[idx];
								m_name = act.get("name", "");
							}
						}
						if (m_name.is_empty()) {
							m_name = vformat("Héroe %d", member_idx);
						}
						res += m_name;
						i = close + 1;
						any_replaced = true;
						continue;
					}
				}

				// 5. \G -> Moneda del Sistema
				if (next == 'G') {
					String currency = "G";
					if (dm) {
						Array sys_arr = dm->get_category_array(CAT_SYSTEM);
						if (!sys_arr.is_empty()) {
							Dictionary sys = sys_arr[0];
							currency = sys.get("currency_name", "G");
						}
					}
					res += currency;
					i += 2;
					any_replaced = true;
					continue;
				}
			}

			res += text[i];
			i++;
		}

		text = res;
		if (!any_replaced) {
			break;
		}
	}

	// Restaurar dobles barras escapadas a barra simple
	text = text.replace(String::chr(1), "\\");
	return text;
}

String ClockGearsTextParser::to_bbcode(const String &p_text) {
	String text = resolve_data_escapes(p_text);
	String res = "";
	int i = 0;
	int len = text.length();

	bool bold_open = false;
	bool italic_open = false;
	bool shake_open = false;
	bool wave_open = false;
	bool rainbow_open = false;
	int color_stack = 0;
	int font_size_offset = 0;
	const int BASE_FONT_SIZE = 16;

	while (i < len) {
		if (text[i] == '\\' && i + 1 < len) {
			char32_t next = text[i + 1];

			// 1. Color: \C[n]
			if (next == 'C' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) {
					int col_idx = text.substr(i + 3, close - (i + 3)).to_int();
					if (col_idx == 0) {
						while (color_stack > 0) {
							res += "[/color]";
							color_stack--;
						}
					} else {
						String hex = get_palette_hex(col_idx);
						res += vformat("[color=#%s]", hex);
						color_stack++;
					}
					i = close + 1;
					continue;
				}
			}

			// 2. Negrita: \B
			if (next == 'B' && (i + 2 >= len || text[i + 2] != '[')) {
				if (!bold_open) {
					res += "[b]";
					bold_open = true;
				} else {
					res += "[/b]";
					bold_open = false;
				}
				i += 2;
				continue;
			}

			// 3. Cursiva: \IT
			if (next == 'I' && i + 2 < len && text[i + 2] == 'T') {
				if (!italic_open) {
					res += "[i]";
					italic_open = true;
				} else {
					res += "[/i]";
					italic_open = false;
				}
				i += 3;
				continue;
			}

			// 4. Tamaño de fuente: \{ (aumentar) y \} (reducir)
			if (next == '{') {
				font_size_offset += 4;
				res += vformat("[font_size=%d]", BASE_FONT_SIZE + font_size_offset);
				i += 2;
				continue;
			}
			if (next == '}') {
				font_size_offset = MAX(-8, font_size_offset - 4);
				res += vformat("[font_size=%d]", BASE_FONT_SIZE + font_size_offset);
				i += 2;
				continue;
			}

			// 5. Tamaño de fuente absoluto: \FS[n]
			if (next == 'F' && i + 2 < len && text[i + 2] == 'S' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) {
					int sz = text.substr(i + 4, close - (i + 4)).to_int();
					if (sz > 0) {
						res += vformat("[font_size=%d]", sz);
					}
					i = close + 1;
					continue;
				}
			}

			// 6. Efecto de temblor: \SH
			if (next == 'S' && i + 2 < len && text[i + 2] == 'H') {
				if (!shake_open) {
					res += "[shake rate=20.0 level=5 connected=1]";
					shake_open = true;
				} else {
					res += "[/shake]";
					shake_open = false;
				}
				i += 3;
				continue;
			}

			// 7. Efecto de onda: \WV
			if (next == 'W' && i + 2 < len && text[i + 2] == 'V') {
				if (!wave_open) {
					res += "[wave amp=40.0 freq=3.0 connected=1]";
					wave_open = true;
				} else {
					res += "[/wave]";
					wave_open = false;
				}
				i += 3;
				continue;
			}

			// 8. Efecto arcoíris: \RB
			if (next == 'R' && i + 2 < len && text[i + 2] == 'B') {
				if (!rainbow_open) {
					res += "[rainbow freq=0.5 sat=0.8 val=0.9]";
					rainbow_open = true;
				} else {
					res += "[/rainbow]";
					rainbow_open = false;
				}
				i += 3;
				continue;
			}

			// 9. Icono: \I[n]
			if (next == 'I' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) {
					int ico_id = text.substr(i + 3, close - (i + 3)).to_int();
					res += vformat("[color=#e0a040]◆[%d][/color]", ico_id);
					i = close + 1;
					continue;
				}
			}

			// 10. Ignorar / ocultar de la salida visible los códigos de control que ejecuta el motor
			if (next == 'N' && i + 2 < len && text[i + 2] == 'M' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'F' && i + 2 < len && text[i + 2] == 'A' && i + 3 < len && text[i + 3] == 'C' && i + 4 < len && text[i + 4] == 'E' && i + 5 < len && text[i + 5] == '[') {
				int close = text.find_char(']', i + 6);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'E' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'S' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'W' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'S' && i + 2 < len && text[i + 2] == 'E' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == '.' || next == '|' || next == '!' || next == '>' || next == '<' || next == '^' || next == '$') {
				i += 2;
				continue;
			}
			if (next == 'V' && i + 2 < len && text[i + 2] == '0') {
				i += 3;
				continue;
			}
			if (next == '\\') {
				res += "\\";
				i += 2;
				continue;
			}
		}

		res += text[i];
		i++;
	}

	// Cerrar etiquetas abiertas si quedaron pendientes
	while (color_stack > 0) { res += "[/color]"; color_stack--; }
	if (bold_open) res += "[/b]";
	if (italic_open) res += "[/i]";
	if (shake_open) res += "[/shake]";
	if (wave_open) res += "[/wave]";
	if (rainbow_open) res += "[/rainbow]";

	return res;
}

String ClockGearsTextParser::strip_escapes(const String &p_text) {
	String text = resolve_data_escapes(p_text);
	String res = "";
	int i = 0;
	int len = text.length();

	while (i < len) {
		if (text[i] == '\\' && i + 1 < len) {
			char32_t next = text[i + 1];

			if (next == '\\') {
				res += "\\";
				i += 2;
				continue;
			}

			// Escapes con corchetes [ ... ]
			if (next == 'C' || next == 'I' || next == 'E' || next == 'S' || next == 'W') {
				if (i + 2 < len && text[i + 2] == '[') {
					int close = text.find_char(']', i + 3);
					if (close != -1) { i = close + 1; continue; }
				}
			}
			if (next == 'N' && i + 2 < len && text[i + 2] == 'M' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'F' && i + 2 < len && text[i + 2] == 'S' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'F' && i + 2 < len && text[i + 2] == 'A' && i + 3 < len && text[i + 3] == 'C' && i + 4 < len && text[i + 4] == 'E' && i + 5 < len && text[i + 5] == '[') {
				int close = text.find_char(']', i + 6);
				if (close != -1) { i = close + 1; continue; }
			}
			if (next == 'S' && i + 2 < len && text[i + 2] == 'E' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) { i = close + 1; continue; }
			}

			// Escapes cortos de 2 o 3 caracteres
			if (next == 'B' || next == '{' || next == '}' || next == '.' || next == '|' || next == '!' || next == '>' || next == '<' || next == '^' || next == '$') {
				i += 2;
				continue;
			}
			if (next == 'I' && i + 2 < len && text[i + 2] == 'T') { i += 3; continue; }
			if (next == 'S' && i + 2 < len && text[i + 2] == 'H') { i += 3; continue; }
			if (next == 'W' && i + 2 < len && text[i + 2] == 'V') { i += 3; continue; }
			if (next == 'R' && i + 2 < len && text[i + 2] == 'B') { i += 3; continue; }
			if (next == 'V' && i + 2 < len && text[i + 2] == '0') { i += 3; continue; }
		}

		res += text[i];
		i++;
	}

	return res;
}

ClockGearsTextParser::ParsedMessage ClockGearsTextParser::parse_full(const String &p_text, const String &p_default_speaker) {
	ParsedMessage msg;
	msg.speaker_name = p_default_speaker;
	msg.bbcode_text = to_bbcode(p_text);
	msg.plain_text = strip_escapes(p_text);

	String text = resolve_data_escapes(p_text);
	int i = 0;
	int len = text.length();

	while (i < len) {
		if (text[i] == '\\' && i + 1 < len) {
			char32_t next = text[i + 1];

			if (next == 'N' && i + 2 < len && text[i + 2] == 'M' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) {
					msg.speaker_name = text.substr(i + 4, close - (i + 4));
					Token tok;
					tok.type = TOKEN_NAME;
					tok.str_param = msg.speaker_name;
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == 'E' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) {
					msg.face_expression = text.substr(i + 3, close - (i + 3));
					Token tok;
					tok.type = TOKEN_EXPRESSION;
					tok.str_param = msg.face_expression;
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == 'F' && i + 2 < len && text[i + 2] == 'A' && i + 3 < len && text[i + 3] == 'C' && i + 4 < len && text[i + 4] == 'E' && i + 5 < len && text[i + 5] == '[') {
				int close = text.find_char(']', i + 6);
				if (close != -1) {
					msg.face_index = text.substr(i + 6, close - (i + 6)).to_int();
					Token tok;
					tok.type = TOKEN_FACE_INDEX;
					tok.int_param = msg.face_index;
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == 'S' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) {
					msg.typing_speed = CLAMP(text.substr(i + 3, close - (i + 3)).to_int(), 1, 200);
					Token tok;
					tok.type = TOKEN_SPEED;
					tok.int_param = msg.typing_speed;
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == 'W' && i + 2 < len && text[i + 2] == '[') {
				int close = text.find_char(']', i + 3);
				if (close != -1) {
					Token tok;
					tok.type = TOKEN_WAIT_FRAMES;
					tok.int_param = text.substr(i + 3, close - (i + 3)).to_int();
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == 'S' && i + 2 < len && text[i + 2] == 'E' && i + 3 < len && text[i + 3] == '[') {
				int close = text.find_char(']', i + 4);
				if (close != -1) {
					Token tok;
					tok.type = TOKEN_PLAY_SE;
					tok.str_param = text.substr(i + 4, close - (i + 4));
					msg.execution_tokens.push_back(tok);
					i = close + 1;
					continue;
				}
			}

			if (next == '.') {
				Token tok; tok.type = TOKEN_PAUSE_SHORT; tok.int_param = 15;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '|') {
				Token tok; tok.type = TOKEN_PAUSE_LONG; tok.int_param = 60;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '!') {
				Token tok; tok.type = TOKEN_WAIT_INPUT;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '>') {
				Token tok; tok.type = TOKEN_INSTANT_ON;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '<') {
				Token tok; tok.type = TOKEN_INSTANT_OFF;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '^') {
				msg.no_input_wait = true;
				Token tok; tok.type = TOKEN_NO_INPUT_WAIT;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == '$') {
				msg.show_gold = true;
				Token tok; tok.type = TOKEN_GOLD_WINDOW;
				msg.execution_tokens.push_back(tok);
				i += 2;
				continue;
			}

			if (next == 'V' && i + 2 < len && text[i + 2] == '0') {
				msg.silence_blips = true;
				Token tok; tok.type = TOKEN_SILENCE_BLIPS;
				msg.execution_tokens.push_back(tok);
				i += 3;
				continue;
			}
		}

		Token tok;
		tok.type = TOKEN_CHAR;
		tok.character = text[i];
		msg.execution_tokens.push_back(tok);
		i++;
	}

	return msg;
}
