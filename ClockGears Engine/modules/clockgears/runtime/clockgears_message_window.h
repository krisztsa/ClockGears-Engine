#ifndef CLOCKGEARS_MESSAGE_WINDOW_H
#define CLOCKGEARS_MESSAGE_WINDOW_H

#include "clockgears_text_parser.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/style_box_flat.h"

class ClockGearsMessageWindow : public Control {
	GDCLASS(ClockGearsMessageWindow, Control);

public:
	static ClockGearsMessageWindow *get_singleton() { return singleton; }

	enum WindowPosition {
		POS_BOTTOM = 0,
		POS_MIDDLE = 1,
		POS_TOP = 2,
	};

	enum WindowBackground {
		BG_WINDOW = 0,
		BG_DIM = 1,
		BG_TRANSPARENT = 2,
	};

	enum State {
		STATE_CLOSED = 0,
		STATE_TYPING = 1,
		STATE_WAITING = 2,
		STATE_CHOICES = 3,
		STATE_NUMBER_INPUT = 4,
	};

	struct DialogueHistoryItem {
		String speaker;
		String text;
	};

private:
	static ClockGearsMessageWindow *singleton;

	// Contenedor principal de la ventana
	PanelContainer *main_panel = nullptr;
	Ref<StyleBoxFlat> style_window;
	Ref<StyleBoxFlat> style_dim;
	Ref<StyleBoxFlat> style_transparent;

	// Caja de Nombre (Name Box)
	PanelContainer *name_panel = nullptr;
	Label *name_label = nullptr;

	// Contenedor horizontal: Retrato + Texto
	HBoxContainer *content_hbox = nullptr;
	PanelContainer *face_frame = nullptr;
	TextureRect *face_rect = nullptr;
	VBoxContainer *text_vbox = nullptr;
	RichTextLabel *rich_text_label = nullptr;

	// Indicador de continuar (flecha parpadeante ▼)
	Label *continue_arrow = nullptr;
	float arrow_blink_timer = 0.0f;

	// Ventana de Oro ($)
	PanelContainer *gold_panel = nullptr;
	Label *gold_label = nullptr;

	// Indicadores informativos AUTO / SKIP (no botones, no capturan foco ni ui_accept)
	HBoxContainer *mode_bar = nullptr;
	Label *auto_indicator = nullptr;
	Label *skip_indicator = nullptr;
	void _update_mode_indicators();

	// Ventana de Elecciones (Choices)
	PanelContainer *choices_panel = nullptr;
	VBoxContainer *choices_vbox = nullptr;
	Vector<Button *> choice_buttons;
	int choice_cancel_type = 0; // 0=ninguno/prohibir, 1..N=rama

	// Ventana de Entrada de Número (Number Input)
	PanelContainer *number_panel = nullptr;
	HBoxContainer *number_hbox = nullptr;
	Vector<Label *> digit_labels;
	int number_target_variable = 0;
	int number_digit_count = 3;
	int number_active_digit_idx = 0;
	Vector<int> number_digits;

	// Historial de Diálogos (Backlog)
	PanelContainer *history_panel = nullptr;
	ScrollContainer *history_scroll = nullptr;
	VBoxContainer *history_vbox = nullptr;
	Button *history_close_btn = nullptr;
	Vector<DialogueHistoryItem> dialogue_history;

	// Estado y control de flujo
	State current_state = STATE_CLOSED;
	WindowPosition window_pos = POS_BOTTOM;
	WindowBackground window_bg = BG_WINDOW;

	ClockGearsTextParser::ParsedMessage active_message;
	int current_token_idx = 0;
	int visible_chars_count = 0;
	float type_timer = 0.0f;
	float token_wait_timer = 0.0f;
	float auto_wait_timer = 0.0f;

	bool is_auto_mode = false;
	bool is_skip_mode = false;
	bool instant_text_active = false;
	bool waiting_for_release = false;
	int input_block_frames = 0;
	String current_face_base_path;
	String current_speaker_name;
	String current_expression;
	int current_face_frame = 0;

	void _update_window_position_layout();
	void _update_background_style();
	void _advance_typewriter(float p_delta);
	void _process_current_token();
	void _on_choice_btn_pressed(int p_idx);
	void _update_number_display();
	void _confirm_number_input();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event);

public:
	// API Principal de Diálogo
	void show_message(const String &p_text, const String &p_speaker = "", const String &p_face_path = "", int p_pos = 0, int p_bg = 0);
	void show_choices(const PackedStringArray &p_choices, int p_cancel_type = 0, int p_default_idx = 0);
	void show_number_input(int p_variable_id, int p_digits = 3);
	void advance_or_close();
	void close_window();
	void close_window_with_reason(const String &p_reason);

	// Expresiones y rostros
	void set_face(const String &p_path, int p_frame = 0, const String &p_expression = "");
	String get_current_expression() const { return current_expression; }
	int get_face_frame() const { return current_face_frame; }
	String get_speaker_name() const { return current_speaker_name; }

	// Modos Auto y Saltar
	void set_auto_mode(bool p_enable);
	bool get_auto_mode() const { return is_auto_mode; }
	void set_skip_mode(bool p_enable);
	bool get_skip_mode() const { return is_skip_mode; }

	// Historial (Backlog)
	void open_history();
	void close_history();
	bool is_history_open() const;
	void add_history_entry(const String &p_speaker, const String &p_text);
	int get_history_count() const { return dialogue_history.size(); }
	const Vector<DialogueHistoryItem> &get_history() const { return dialogue_history; }

	// Estado
	State get_state() const { return current_state; }
	bool is_busy() const { return current_state != STATE_CLOSED; }

	ClockGearsMessageWindow();
	~ClockGearsMessageWindow();
};

#endif // CLOCKGEARS_MESSAGE_WINDOW_H
