#ifndef CLOCKGEARS_TEXT_PARSER_H
#define CLOCKGEARS_TEXT_PARSER_H

#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/variant/variant.h"
#include "core/templates/vector.h"
#include "core/math/color.h"

class ClockGearsTextParser : public RefCounted {
	GDCLASS(ClockGearsTextParser, RefCounted);

public:
	enum TokenType {
		TOKEN_CHAR,          // Carácter visible
		TOKEN_PAUSE_SHORT,   // \. (15 frames / 0.25s)
		TOKEN_PAUSE_LONG,    // \| (60 frames / 1.0s)
		TOKEN_WAIT_INPUT,    // \! (Pausa esperando tecla)
		TOKEN_WAIT_FRAMES,   // \W[n]
		TOKEN_INSTANT_ON,    // \>
		TOKEN_INSTANT_OFF,   // \<
		TOKEN_NO_INPUT_WAIT, // \^ (No esperar al final)
		TOKEN_SPEED,         // \S[n] (Cambiar velocidad)
		TOKEN_FACE_INDEX,    // \FACE[n]
		TOKEN_EXPRESSION,    // \E[exp]
		TOKEN_NAME,          // \NM[nombre]
		TOKEN_PLAY_SE,       // \SE[nombre]
		TOKEN_GOLD_WINDOW,   // \$
		TOKEN_SILENCE_BLIPS, // \V0
	};

	struct Token {
		TokenType type = TOKEN_CHAR;
		char32_t character = 0;
		int int_param = 0;
		String str_param;
	};

	struct ParsedMessage {
		String bbcode_text;
		String plain_text;
		String speaker_name;
		int face_index = -1;
		String face_expression;
		bool no_input_wait = false;
		bool show_gold = false;
		bool silence_blips = false;
		int typing_speed = 35; // Caracteres por segundo por defecto
		Vector<Token> execution_tokens;
	};

	static String resolve_data_escapes(const String &p_text);
	static String to_bbcode(const String &p_text);
	static String strip_escapes(const String &p_text);
	static ParsedMessage parse_full(const String &p_text, const String &p_default_speaker = "");
	static Dictionary parse_to_dict(const String &p_text, const String &p_default_speaker = "");

	static Color get_palette_color(int p_idx);
	static String get_palette_hex(int p_idx);

protected:
	static void _bind_methods();
};

#endif // CLOCKGEARS_TEXT_PARSER_H
