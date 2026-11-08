#pragma once

#include "clockgears_command_selector.h"
#include "clockgears_event.h"
#include "clockgears_types.h"
#include "editor/clockgears_picture_picker.h"
#include "editor/clockgears_tile_picker.h"
#include "scene/gui/box_container.h"
#include "scene/gui/center_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "runtime/clockgears_text_parser.h"

class ClockGearsEventEditorDialog : public AcceptDialog {
	GDCLASS(ClockGearsEventEditorDialog, AcceptDialog);

private:
	ClockGearsEvent *target_event = nullptr;

	Label *id_label = nullptr;
	LineEdit *name_edit = nullptr;

	// 1. Page Tabs (Top Bar)
	TabBar *page_tabs = nullptr;
	Button *btn_new_page = nullptr;
	Button *btn_copy_page = nullptr;
	Button *btn_paste_page = nullptr;
	Button *btn_delete_page = nullptr;
	Array working_pages;
	int current_page_index = 0;
	Dictionary clipboard_page;
	bool loading_page_ui = false;

	// 2. Column 1: Conditions & Trigger & Options
	CheckBox *cond_sw1_check = nullptr;
	LineEdit *cond_sw1_name_edit = nullptr;
	CheckBox *cond_sw2_check = nullptr;
	LineEdit *cond_sw2_name_edit = nullptr;
	CheckBox *cond_self_sw_check = nullptr;
	OptionButton *cond_self_sw_opt = nullptr;
	CheckBox *cond_var_check = nullptr;
	LineEdit *cond_var_name_edit = nullptr;
	SpinBox *cond_var_val_spin = nullptr;
	CheckBox *cond_item_check = nullptr;
	LineEdit *cond_item_name_edit = nullptr;
	CheckBox *cond_actor_check = nullptr;
	LineEdit *cond_actor_name_edit = nullptr;

	OptionButton *trigger_opt = nullptr;
	OptionButton *priority_opt = nullptr;
	CheckBox *through_check = nullptr;
	CheckBox *dir_fix_check = nullptr;

	// 3. Column 2: Graphic (SpriteSheet/Atlas Frame) & Autonomous Movement
	TextureRect *graphic_preview = nullptr;
	SpinBox *sprite_hframes_spin = nullptr;
	SpinBox *sprite_vframes_spin = nullptr;
	SpinBox *sprite_frame_spin = nullptr;
	Button *select_graphic_btn = nullptr;
	Button *select_tile_graphic_btn = nullptr;
	Button *clear_graphic_btn = nullptr;
	FileDialog *graphic_file_dialog = nullptr;
	ClockGearsTilePickerDialog *event_tile_picker_dialog = nullptr;
	String current_graphic_path;

	OptionButton *move_type_opt = nullptr;
	OptionButton *speed_opt = nullptr;
	OptionButton *freq_opt = nullptr;

	// 4. Column 3: Commands Tree & Catalog
	Tree *command_tree = nullptr;
	Button *add_cmd_btn = nullptr;
	Button *edit_cmd_btn = nullptr;
	Button *remove_cmd_btn = nullptr;
	Button *clear_cmd_btn = nullptr;
	Button *move_up_cmd_btn = nullptr;
	Button *move_down_cmd_btn = nullptr;
	int editing_command_index = -1;

	// Visual Command Selector Catalog
	ClockGearsCommandSelector *cmd_catalog_dialog = nullptr;

	// Database Picker Dialogs (Selector de Objetos y Actores)
	ConfirmationDialog *dlg_item_picker = nullptr;
	LineEdit *item_picker_search = nullptr;
	TabBar *item_picker_tabs = nullptr;
	ItemList *item_picker_list = nullptr;
	LineEdit *target_item_line_edit = nullptr;

	ConfirmationDialog *dlg_actor_picker = nullptr;
	LineEdit *actor_picker_search = nullptr;
	ItemList *actor_picker_list = nullptr;
	LineEdit *target_actor_line_edit = nullptr;

	// Sub-dialogs for command parameters
	ConfirmationDialog *dlg_msg = nullptr;
	TextEdit *msg_text_edit = nullptr;
	OptionButton *msg_frame_opt = nullptr;
	OptionButton *msg_pos_opt = nullptr;
	LineEdit *msg_face_edit = nullptr;
	Button *msg_face_browse_btn = nullptr;
	FileDialog *msg_face_file_dialog = nullptr;
	Label *msg_lines_label = nullptr;
	PanelContainer *msg_preview_panel = nullptr;
	PanelContainer *msg_preview_name_box = nullptr;
	Label *msg_preview_name_label = nullptr;
	TextureRect *msg_preview_face_rect = nullptr;
	RichTextLabel *msg_preview_label = nullptr;
	void _update_msg_live_preview();
	void _insert_msg_escape_code(const String &p_code);

	ConfirmationDialog *dlg_choices = nullptr;
	LineEdit *choice_1_edit = nullptr;
	LineEdit *choice_2_edit = nullptr;
	LineEdit *choice_3_edit = nullptr;
	LineEdit *choice_4_edit = nullptr;
	LineEdit *choice_5_edit = nullptr;
	LineEdit *choice_6_edit = nullptr;
	OptionButton *choice_cancel_opt = nullptr;

	ConfirmationDialog *dlg_number_input = nullptr;
	SpinBox *num_input_var_spin = nullptr;
	SpinBox *num_input_digits_spin = nullptr;
	void _on_confirm_number_input();

	ConfirmationDialog *dlg_switch = nullptr;
	LineEdit *sw_name_edit = nullptr;
	OptionButton *sw_val_opt = nullptr;

	ConfirmationDialog *dlg_self_switch = nullptr;
	OptionButton *self_sw_letter_opt = nullptr;
	OptionButton *self_sw_val_opt = nullptr;

	ConfirmationDialog *dlg_var = nullptr;
	LineEdit *var_name_edit = nullptr;
	OptionButton *var_op_opt = nullptr;
	SpinBox *var_val_spin = nullptr;

	ConfirmationDialog *dlg_wait = nullptr;
	SpinBox *wait_frames_spin = nullptr;

	ConfirmationDialog *dlg_teleport = nullptr;
	OptionButton *tele_map_opt = nullptr;
	LineEdit *tele_map_edit = nullptr;
	SpinBox *tele_x_spin = nullptr;
	SpinBox *tele_y_spin = nullptr;
	OptionButton *tele_dir_opt = nullptr;

	ConfirmationDialog *dlg_move_route = nullptr;
	OptionButton *mr_target_opt = nullptr;
	OptionButton *mr_action_opt = nullptr;
	CheckBox *mr_repeat_check = nullptr;

	ConfirmationDialog *dlg_item = nullptr;
	LineEdit *item_name_edit = nullptr;
	OptionButton *item_op_opt = nullptr;
	SpinBox *item_qty_spin = nullptr;

	ConfirmationDialog *dlg_gold = nullptr;
	OptionButton *gold_op_opt = nullptr;
	SpinBox *gold_val_spin = nullptr;

	ConfirmationDialog *dlg_hp_mp = nullptr;
	OptionButton *hp_mp_stat_opt = nullptr;
	OptionButton *hp_mp_op_opt = nullptr;
	SpinBox *hp_mp_amount_spin = nullptr;

	ConfirmationDialog *dlg_bgm = nullptr;
	LineEdit *bgm_file_edit = nullptr;
	Button *bgm_file_browse_btn = nullptr;
	FileDialog *bgm_file_dialog = nullptr;
	SpinBox *bgm_volume_spin = nullptr;
	SpinBox *bgm_pitch_spin = nullptr;

	ConfirmationDialog *dlg_se = nullptr;
	LineEdit *se_file_edit = nullptr;
	Button *se_file_browse_btn = nullptr;
	FileDialog *se_file_dialog = nullptr;
	SpinBox *se_volume_spin = nullptr;
	SpinBox *se_pitch_spin = nullptr;

	ConfirmationDialog *dlg_flash = nullptr;
	OptionButton *flash_color_opt = nullptr;
	SpinBox *flash_duration_spin = nullptr;

	ConfirmationDialog *dlg_tint = nullptr;
	OptionButton *tint_preset_opt = nullptr;
	SpinBox *tint_duration_spin = nullptr;

	ConfirmationDialog *dlg_fade = nullptr;
	OptionButton *fade_type_opt = nullptr;
	SpinBox *fade_duration_spin = nullptr;

	ConfirmationDialog *dlg_battle = nullptr;
	OptionButton *battle_troop_opt = nullptr;
	LineEdit *battle_troop_edit = nullptr;
	CheckBox *battle_can_escape_check = nullptr;
	CheckBox *battle_can_lose_check = nullptr;
	LineEdit *battle_bgm_edit = nullptr;
	Button *battle_bgm_browse_btn = nullptr;
	FileDialog *battle_bgm_file_dialog = nullptr;

	// Sub-diálogos Fase 7B: Imágenes
	ConfirmationDialog *dlg_show_picture = nullptr;
	SpinBox *pic_show_id_spin = nullptr;
	LineEdit *pic_show_path_edit = nullptr;
	Button *pic_show_browse_btn = nullptr;
	FileDialog *pic_show_file_dialog = nullptr;
	OptionButton *pic_show_origin_opt = nullptr;
	SpinBox *pic_show_x_spin = nullptr;
	SpinBox *pic_show_y_spin = nullptr;
	SpinBox *pic_show_scale_x_spin = nullptr;
	SpinBox *pic_show_scale_y_spin = nullptr;
	SpinBox *pic_show_opacity_spin = nullptr;
	OptionButton *pic_show_blend_opt = nullptr;
	ClockGearsPictureVisualPicker *pic_show_picker = nullptr;
	OptionButton *pic_show_dur_mode_opt = nullptr;
	SpinBox *pic_show_dur_sec_spin = nullptr;
	CheckBox *pic_show_fadeout_check = nullptr;
	CheckBox *pic_show_wait_check = nullptr;
	Button *pic_show_adv_btn = nullptr;
	VBoxContainer *pic_show_adv_box = nullptr;
	void _on_pic_show_adv_toggle();

	ConfirmationDialog *dlg_move_picture = nullptr;
	SpinBox *pic_move_id_spin = nullptr;
	OptionButton *pic_move_origin_opt = nullptr;
	SpinBox *pic_move_x_spin = nullptr;
	SpinBox *pic_move_y_spin = nullptr;
	SpinBox *pic_move_scale_x_spin = nullptr;
	SpinBox *pic_move_scale_y_spin = nullptr;
	SpinBox *pic_move_opacity_spin = nullptr;
	SpinBox *pic_move_duration_spin = nullptr;
	OptionButton *pic_move_easing_opt = nullptr;
	CheckBox *pic_move_wait_check = nullptr;
	ClockGearsPictureVisualPicker *pic_move_picker = nullptr;
	Button *pic_move_adv_btn = nullptr;
	VBoxContainer *pic_move_adv_box = nullptr;
	void _on_pic_move_adv_toggle();

	ConfirmationDialog *dlg_change_camera_zoom = nullptr;
	OptionButton *cam_zoom_opt = nullptr;
	SpinBox *cam_zoom_spin = nullptr;
	SpinBox *cam_zoom_duration_spin = nullptr;
	CheckBox *cam_zoom_wait_check = nullptr;
	void _on_cam_zoom_preset_selected(int p_idx);
	void _on_confirm_change_camera_zoom();

	ConfirmationDialog *dlg_rotate_picture = nullptr;
	SpinBox *pic_rot_id_spin = nullptr;
	SpinBox *pic_rot_speed_spin = nullptr;

	ConfirmationDialog *dlg_tint_picture = nullptr;
	SpinBox *pic_tint_id_spin = nullptr;
	OptionButton *pic_tint_preset_opt = nullptr;
	SpinBox *pic_tint_duration_spin = nullptr;
	CheckBox *pic_tint_wait_check = nullptr;

	ConfirmationDialog *dlg_erase_picture = nullptr;
	SpinBox *pic_erase_id_spin = nullptr;

	// Sub-diálogos Fase 7B: Pantalla
	ConfirmationDialog *dlg_shake = nullptr;
	SpinBox *shake_power_spin = nullptr;
	SpinBox *shake_speed_spin = nullptr;
	SpinBox *shake_duration_spin = nullptr;
	CheckBox *shake_wait_check = nullptr;

	ConfirmationDialog *dlg_weather = nullptr;
	OptionButton *weather_type_opt = nullptr;
	SpinBox *weather_power_spin = nullptr;
	SpinBox *weather_duration_spin = nullptr;

	// Sub-diálogos Fase 7B: Audio
	ConfirmationDialog *dlg_bgs = nullptr;
	LineEdit *bgs_file_edit = nullptr;
	Button *bgs_file_browse_btn = nullptr;
	FileDialog *bgs_file_dialog = nullptr;
	SpinBox *bgs_volume_spin = nullptr;
	SpinBox *bgs_pitch_spin = nullptr;

	ConfirmationDialog *dlg_me = nullptr;
	LineEdit *me_file_edit = nullptr;
	Button *me_file_browse_btn = nullptr;
	FileDialog *me_file_dialog = nullptr;
	SpinBox *me_volume_spin = nullptr;
	SpinBox *me_pitch_spin = nullptr;

	ConfirmationDialog *dlg_stop_fade_bgm = nullptr;
	OptionButton *bgm_stop_action_opt = nullptr;
	SpinBox *bgm_stop_duration_spin = nullptr;

	ConfirmationDialog *dlg_restore_bgm = nullptr;
	SpinBox *bgm_restore_duration_spin = nullptr;

	ConfirmationDialog *dlg_stems = nullptr;
	SpinBox *stem_id_spin = nullptr;
	LineEdit *stem_file_edit = nullptr;
	Button *stem_file_browse_btn = nullptr;
	FileDialog *stem_file_dialog = nullptr;
	SpinBox *stem_volume_spin = nullptr;
	SpinBox *stem_fade_spin = nullptr;

	// Sub-diálogos Fase 7B: Cinemática
	ConfirmationDialog *dlg_play_cutscene = nullptr;
	LineEdit *cutscene_file_edit = nullptr;
	Button *cutscene_file_browse_btn = nullptr;
	FileDialog *cutscene_file_dialog = nullptr;
	CheckBox *cutscene_wait_check = nullptr;

	ConfirmationDialog *dlg_create_cutscene = nullptr;
	LineEdit *cutscene_create_name_edit = nullptr;
	SpinBox *cutscene_create_dur_spin = nullptr;
	CheckBox *cutscene_create_letterbox_check = nullptr;
	CheckBox *cutscene_create_block_check = nullptr;
	Tree *cutscene_kf_tree = nullptr;
	SpinBox *kf_time_spin = nullptr;
	OptionButton *kf_track_opt = nullptr;
	LineEdit *kf_value_edit = nullptr;
	Button *kf_add_btn = nullptr;
	Button *kf_remove_btn = nullptr;
	Array cutscene_temp_keyframes;

	Dictionary _create_empty_page() const;
	void _save_current_page_from_ui();
	void _load_page_to_ui(int p_page_idx);
	void _rebuild_page_tabs();

	void _on_page_tab_changed(int p_tab);
	void _on_new_page_pressed();
	void _on_copy_page_pressed();
	void _on_paste_page_pressed();
	void _on_delete_page_pressed();

	void _append_command_to_active_page(const Dictionary &p_cmd);
	Array _get_active_page_commands() const;
	void _set_active_page_commands(const Array &p_cmds);

	void _setup_command_subdialogs();
	void _refresh_command_list();
	void _on_add_command_pressed();
	void _on_edit_command_pressed();
	void _on_remove_command_pressed();
	void _on_clear_commands_pressed();
	void _on_move_command_up();
	void _on_move_command_down();
	void _on_command_id_selected(int p_cmd_id);

	// Database Pickers
	void _setup_database_picker_dialogs();
	void _open_item_picker(LineEdit *p_target);
	void _refresh_item_picker_list();
	void _on_item_picker_tab_changed(int p_tab);
	void _on_item_picker_search_changed(const String &p_text);
	void _on_item_picker_item_activated(int p_idx);
	void _on_item_picker_confirmed();

	void _open_actor_picker(LineEdit *p_target);
	void _refresh_actor_picker_list();
	void _on_actor_picker_search_changed(const String &p_text);
	void _on_actor_picker_item_activated(int p_idx);
	void _on_actor_picker_confirmed();

	// Confirmation handlers for sub-dialogs
	void _on_confirm_msg();
	void _on_confirm_choices();
	void _on_confirm_switch();
	void _on_confirm_self_switch();
	void _on_confirm_var();
	void _on_confirm_wait();
	void _on_confirm_teleport();
	void _populate_teleport_maps();
	void _on_tele_map_opt_selected(int p_idx);
	void _on_confirm_move_route();
	void _on_confirm_item();
	void _on_confirm_gold();
	void _on_confirm_hp_mp();
	void _on_confirm_bgm();
	void _on_confirm_se();
	void _on_confirm_flash();
	void _on_confirm_tint();
	void _on_confirm_fade();
	void _on_confirm_battle();
	void _on_battle_bgm_browse_pressed();
	void _on_battle_bgm_selected(const String &p_path);
	void _on_browse_face_pressed();
	void _on_face_file_selected(const String &p_path);
	void _on_browse_bgm_pressed();
	void _on_bgm_file_selected(const String &p_path);
	void _on_browse_se_pressed();
	void _on_se_file_selected(const String &p_path);

	// Handlers Fase 7B
	void _on_confirm_show_picture();
	void _on_confirm_move_picture();
	void _on_confirm_rotate_picture();
	void _on_confirm_tint_picture();
	void _on_confirm_erase_picture();
	void _on_confirm_shake();
	void _on_confirm_weather();
	void _on_confirm_bgs();
	void _on_confirm_me();
	void _on_confirm_stop_fade_bgm();
	void _on_confirm_restore_bgm();
	void _on_confirm_stems();
	void _on_confirm_play_cutscene();
	void _on_confirm_create_cutscene();
	void _on_cutscene_add_kf();
	void _on_cutscene_remove_kf();
	void _refresh_cutscene_kf_tree();
	void _on_browse_pic_pressed();
	void _on_pic_file_selected(const String &p_path);
	void _on_pic_show_dur_mode_selected(int p_idx);
	void _on_pic_show_origin_selected(int p_idx);
	void _on_pic_move_origin_selected(int p_idx);
	void _on_pic_show_path_changed(const String &p_text);
	void _on_browse_bgs_pressed();
	void _on_bgs_file_selected(const String &p_path);
	void _on_browse_me_pressed();
	void _on_me_file_selected(const String &p_path);
	void _on_browse_stem_pressed();
	void _on_stem_file_selected(const String &p_path);
	void _on_browse_cutscene_pressed();
	void _on_cutscene_file_selected(const String &p_path);

	void _on_dialog_confirmed();
	void _on_dialog_canceled();
	void _on_graphic_preview_gui_input(const Ref<InputEvent> &p_event);
	void _on_sprite_frame_changed(double p_val);
	void _on_select_graphic_pressed();
	void _on_select_tile_graphic_pressed();
	void _on_event_tile_picked(const Array &p_sel, const Vector2i &p_block);
	void _on_clear_graphic_pressed();
	void _on_graphic_file_selected(const String &p_path);
	void _update_graphic_preview(const String &p_path);

protected:
	static void _bind_methods();

public:
	void edit_event(ClockGearsEvent *p_event);

	ClockGearsEventEditorDialog();
};

using ClockGearsEventEditor = ClockGearsEventEditorDialog;
