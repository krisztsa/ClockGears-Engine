#pragma once

#include "../clockgears_types.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

// ============================================================================
// ClockGearsProjectStructure
// Gestiona la estructura de 22 carpetas vacías (Regla Cero), el archivo LEEME.txt,
// la generación de project.godot con ajustes Pixel Art (Anexo A4.1), y los
// presets de importación pixel art por carpeta (NEAREST, sin mipmaps, lossless).
// ============================================================================
class ClockGearsProjectStructure {
public:
	struct FolderEntry {
		const char *rel_path;
		const char *display_name;
		const char *category_group;
		const char *filter_extensions;
		const char *filter_description;
		const char *description;
		bool is_pixel_art_image;
		bool require_multiple_of_32;
	};

	static const FolderEntry *get_standard_folders(int &r_count);
	static PackedStringArray get_standard_folder_paths();

	// Crea todas las carpetas vacías, LEEME.txt y project.godot (Anexo A4.1)
	static Error initialize_new_project(const String &p_project_dir, const String &p_project_title, bool p_write_project_godot = true);

	// Devuelve las carpetas que faltan en el proyecto actual (sin modificar nada)
	static PackedStringArray get_missing_folders(const String &p_project_dir = "res://");

	// Crea únicamente las carpetas que falten y LEEME.txt si no existe (sin borrar nada)
	static Error create_missing_folders(const String &p_project_dir = "res://");

	// Genera el archivo LEEME.txt en la raíz del proyecto si no existe
	static Error ensure_readme_file(const String &p_project_dir = "res://", bool p_overwrite = false);

	// Devuelve la ruta globalizada de una subcarpeta del proyecto (creándola si no existe)
	static String resolve_project_folder_global(const String &p_rel_folder);

	// Escribe el archivo .import con preset Pixel Art (NEAREST, sin mipmaps, lossless) para imágenes en img/
	static Error write_pixel_art_import_preset(const String &p_dest_file_path);

	// Valida si una imagen cumple las reglas de la carpeta (ej. múltiplo de 32 px en tilesets y characters)
	static bool validate_folder_asset(const String &p_image_path, const String &p_rel_folder, String &r_warning_message);
};

// ============================================================================
// ClockGearsNewProjectDialog ("Archivo > Nuevo Proyecto...")
// ============================================================================
class ClockGearsNewProjectDialog : public ConfirmationDialog {
	GDCLASS(ClockGearsNewProjectDialog, ConfirmationDialog);

private:
	LineEdit *project_name_edit = nullptr;
	LineEdit *parent_dir_edit = nullptr;
	Button *browse_dir_btn = nullptr;
	Label *resolved_path_label = nullptr;
	Label *status_label = nullptr;
	CheckBox *open_after_create_chk = nullptr;
	FileDialog *dir_dialog = nullptr;

	String _get_target_project_dir() const;
	void _on_fields_changed(const String &p_text = "");
	void _on_browse_pressed();
	void _on_dir_selected(const String &p_dir);
	void _on_confirmed();

protected:
	static void _bind_methods();

public:
	void open_dialog();

	ClockGearsNewProjectDialog();
};

// ============================================================================
// ClockGearsResourceManagerDialog ("Herramientas > Gestor de Recursos...")
// ============================================================================
class ClockGearsResourceManagerDialog : public AcceptDialog {
	GDCLASS(ClockGearsResourceManagerDialog, AcceptDialog);

private:
	ItemList *folder_list = nullptr;
	Label *folder_desc_label = nullptr;
	ItemList *file_list = nullptr;

	Button *btn_import = nullptr;
	Button *btn_export = nullptr;
	Button *btn_delete = nullptr;
	Button *btn_open_explorer = nullptr;
	Button *btn_create_missing = nullptr;

	TextureRect *preview_rect = nullptr;
	Label *file_info_label = nullptr;
	Label *preset_badge_label = nullptr;

	FileDialog *import_file_dialog = nullptr;
	FileDialog *export_file_dialog = nullptr;
	ConfirmationDialog *overwrite_confirm_dialog = nullptr;
	ConfirmationDialog *delete_confirm_dialog = nullptr;
	AcceptDialog *warning_dialog = nullptr;

	int current_folder_idx = 0;
	PackedStringArray pending_import_files;
	int pending_import_index = 0;

	void _refresh_folder_list();
	void _refresh_file_list();
	void _on_folder_selected(int p_idx);
	void _on_file_selected(int p_idx);

	void _on_import_pressed();
	void _on_import_files_selected(const PackedStringArray &p_files);
	void _process_next_pending_import();
	void _perform_copy_import(const String &p_src_file, const String &p_dest_file);
	void _on_overwrite_confirmed();
	void _on_overwrite_canceled();

	void _on_export_pressed();
	void _on_export_file_selected(const String &p_dest_path);
	void _on_delete_pressed();
	void _on_delete_confirmed();
	void _on_open_explorer_pressed();
	void _on_create_missing_pressed();

protected:
	static void _bind_methods();

public:
	void open_manager(const String &p_initial_folder = "");

	ClockGearsResourceManagerDialog();
};
