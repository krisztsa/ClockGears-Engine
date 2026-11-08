#include "clockgears_project_manager.h"
#include "../core/clockgears_pixel_art.h"
#include "../icons/clockgears_icons.h"
#include "clockgears_theme.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

#define CG_PM_TR(m_str) String::utf8(m_str)

// ============================================================================
// Catálogo de las 22 carpetas estándar de un proyecto ClockGears Engine
// (Regla Cero: todas se crean 100% vacías, sin assets de ejemplo)
// ============================================================================
static const ClockGearsProjectStructure::FolderEntry CG_STANDARD_FOLDERS[] = {
	// Audio (5 subcarpetas)
	{ "audio/bgm", "audio/bgm (Música de Fondo)", "Audio", "*.ogg, *.wav, *.mp3", "Música BGM (*.ogg, *.wav, *.mp3)", "Música de fondo en bucle para mapas, combates y escenas narrativas.", false, false },
	{ "audio/bgs", "audio/bgs (Sonido Ambiental)", "Audio", "*.ogg, *.wav, *.mp3", "Ambiente BGS (*.ogg, *.wav, *.mp3)", "Sonidos ambientales continuos (lluvia, viento, río, reloj).", false, false },
	{ "audio/me", "audio/me (Efectos Musicales)", "Audio", "*.ogg, *.wav, *.mp3", "Fanfarrias ME (*.ogg, *.wav, *.mp3)", "Melodías cortas de victoria, posada, objeto clave o derrota.", false, false },
	{ "audio/se", "audio/se (Efectos de Sonido)", "Audio", "*.ogg, *.wav, *.mp3", "Efectos SE (*.ogg, *.wav, *.mp3)", "Efectos de sonido puntuales (cursores, golpes, puertas, pasos).", false, false },
	{ "audio/voice", "audio/voice (Voces / Blips)", "Audio", "*.ogg, *.wav, *.mp3", "Voces y Blips (*.ogg, *.wav, *.mp3)", "Blips de texto por personaje y líneas de voz de diálogo.", false, false },

	// Imágenes Pixel Art (13 subcarpetas)
	{ "img/characters", "img/characters (Personajes 32x32)", "Gráficos", "*.png, *.webp", "Sprites de Personajes (*.png, *.webp)", "Hojas de sprites de personajes, NPCs, eventos y fauna en el mapa (múltiplo de 32 px).", true, true },
	{ "img/portraits", "img/portraits (Retratos Grandes)", "Gráficos", "*.png, *.webp", "Retratos Visual Novel (*.png, *.webp)", "Retratos de cuerpo medio/grande con expresiones para diálogos narrativos.", true, false },
	{ "img/faces", "img/faces (Caras de Diálogo)", "Gráficos", "*.png, *.webp", "Caras de Diálogo (*.png, *.webp)", "Iconos/rostros cuadrados por expresión para cajas de diálogo.", true, false },
	{ "img/battlers", "img/battlers (Battlers de Combate)", "Gráficos", "*.png, *.webp", "Battlers (*.png, *.webp)", "Sprites estáticos o animados de enemigos y actores en batalla.", true, false },
	{ "img/pictures", "img/pictures (Imágenes / Ilustraciones)", "Gráficos", "*.png, *.webp", "Imágenes Libres (*.png, *.webp)", "Imágenes mostradas en pantalla mediante el comando Mostrar Imagen.", true, false },
	{ "img/cutscenes", "img/cutscenes (Láminas de Cinemática)", "Gráficos", "*.png, *.webp", "Gráficos de Cinemática (*.png, *.webp)", "Fondos y viñetas pixel art dedicados al editor de cinemáticas (Timeline).", true, false },
	{ "img/tilesets", "img/tilesets (Hojas de Tileset 32x32)", "Gráficos", "*.png, *.webp", "Hojas de Tileset 32x32 (*.png, *.webp)", "Hojas de tiles y autotiles para mapas (el ancho y alto deben ser múltiplos de 32 px).", true, true },
	{ "img/animations", "img/animations (Animaciones de Combate)", "Gráficos", "*.png, *.webp", "Spritesheets de Animación (*.png, *.webp)", "Hojas de efectos visuales para habilidades, magia y golpes.", true, false },
	{ "img/parallaxes", "img/parallaxes (Fondos Parallax)", "Gráficos", "*.png, *.webp", "Fondos Parallax (*.png, *.webp)", "Capas de fondo o cielo desplazables detrás o delante del mapa.", true, false },
	{ "img/battlebacks", "img/battlebacks (Fondos de Batalla)", "Gráficos", "*.png, *.webp", "Fondos de Batalla (*.png, *.webp)", "Suelos y fondos escénicos para los combates por turnos.", true, false },
	{ "img/titles", "img/titles (Pantallas de Título)", "Gráficos", "*.png, *.webp", "Pantallas de Título (*.png, *.webp)", "Ilustraciones y marcos para la pantalla de inicio del juego.", true, false },
	{ "img/system", "img/system (Sistema e Iconos)", "Gráficos", "*.png, *.webp", "Gráficos de Sistema (*.png, *.webp)", "Windowskin, sets de iconos, globos de emoción y cursores del sistema.", true, false },
	{ "img/ui", "img/ui (Interfaz y HUD)", "Gráficos", "*.png, *.webp", "Gráficos de UI/HUD (*.png, *.webp)", "Marcos de barras de vida, relojes, menús y elementos del Diseñador UI/HUD.", true, false },

	// Otras carpetas raíz del proyecto (4 carpetas)
	{ "fonts", "fonts (Fuentes Pixel Art)", "Datos y Multimedia", "*.ttf, *.otf, *.fnt, *.woff2", "Fuentes (*.ttf, *.otf, *.fnt)", "Fuentes tipográficas bitmap y pixel art sin suavizado borroso.", false, false },
	{ "movies", "movies (Vídeos)", "Datos y Multimedia", "*.ogv, *.webm", "Vídeos (*.ogv, *.webm)", "Secuencias de vídeo pre-renderizadas reproducibles desde eventos.", false, false },
	{ "maps", "maps (Mapas del Juego)", "Datos y Multimedia", "*.tscn, *.scn, *.json", "Escenas de Mapa (*.tscn, *.json)", "Escenas y datos de los mapas 2D creados en el Árbol de Mapas.", false, false },
	{ "clockgears_data", "clockgears_data (Base de Datos JSON)", "Datos y Multimedia", "*.json", "Datos JSON (*.json)", "Archivos JSON de la Base de Datos (Actores, Clases, Objetos, Flora, Fauna, etc.).", false, false }
};

const ClockGearsProjectStructure::FolderEntry *ClockGearsProjectStructure::get_standard_folders(int &r_count) {
	r_count = int(sizeof(CG_STANDARD_FOLDERS) / sizeof(CG_STANDARD_FOLDERS[0]));
	return CG_STANDARD_FOLDERS;
}

PackedStringArray ClockGearsProjectStructure::get_standard_folder_paths() {
	PackedStringArray list;
	int count = 0;
	const FolderEntry *entries = get_standard_folders(count);
	for (int i = 0; i < count; i++) {
		list.push_back(String::utf8(entries[i].rel_path));
	}
	return list;
}

static String _resolve_base_dir(const String &p_project_dir) {
	if (p_project_dir.is_empty() || p_project_dir == "res://" || p_project_dir == "res://.") {
		if (ProjectSettings::get_singleton()) {
			return ProjectSettings::get_singleton()->get_resource_path();
		}
	}
	if (p_project_dir.begins_with("res://") && ProjectSettings::get_singleton()) {
		return ProjectSettings::get_singleton()->globalize_path(p_project_dir);
	}
	return p_project_dir;
}

Error ClockGearsProjectStructure::ensure_readme_file(const String &p_project_dir, bool p_overwrite) {
	String base_dir = _resolve_base_dir(p_project_dir);
	if (base_dir.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	String readme_path = base_dir.path_join("LEEME.txt");
	if (!p_overwrite && FileAccess::exists(readme_path)) {
		return OK;
	}

	Error err = OK;
	Ref<FileAccess> fa = FileAccess::open(readme_path, FileAccess::WRITE, &err);
	if (fa.is_null() || err != OK) {
		return err != OK ? err : ERR_CANT_CREATE;
	}

	String content = String::utf8(
			"========================================================================\n"
			"  CLOCKGEARS ENGINE — ESTRUCTURA DE CARPETAS DEL PROYECTO\n"
			"========================================================================\n"
			"Resolución base Pixel Art: 960x540 (16:9) | Escala entera | Filtro NEAREST\n"
			"Tamaño de tile estándar:   32x32 píxeles\n"
			"\n"
			"1. CARPETAS DE AUDIO (audio/)\n"
			"   - audio/bgm       : Música de fondo en bucle (mapas, combates, temas).\n"
			"   - audio/bgs       : Sonidos ambientales continuos (lluvia, viento, reloj).\n"
			"   - audio/me        : Efectos musicales / fanfarrias cortas (victoria, posada).\n"
			"   - audio/se        : Efectos de sonido puntuales (golpes, pasos, interfaz).\n"
			"   - audio/voice     : Sonidos cortos de voz por letra (blips) y doblaje.\n"
			"\n"
			"2. CARPETAS DE IMÁGENES PIXEL ART (img/)\n"
			"   (Todas las imágenes en img/ se importan con filtro NEAREST, sin mipmaps\n"
			"    y con compresión sin pérdida para preservar cada píxel exacto).\n"
			"   - img/characters  : Sprites de personajes, NPCs y eventos (múltiplo de 32 px).\n"
			"   - img/portraits   : Retratos grandes con expresiones para diálogos.\n"
			"   - img/faces       : Caras / avatares para cajas de texto.\n"
			"   - img/battlers    : Sprites de enemigos y personajes para batalla.\n"
			"   - img/pictures    : Ilustraciones e imágenes mostradas por evento.\n"
			"   - img/cutscenes   : Láminas y recursos para el editor de cinemáticas.\n"
			"   - img/tilesets    : Hojas de tiles 32x32 y autotiles (múltiplo de 32 px).\n"
			"   - img/animations  : Hojas de animación para habilidades y efectos.\n"
			"   - img/parallaxes  : Fondos de desplazamiento parallax.\n"
			"   - img/battlebacks : Fondos de escenario para los combates.\n"
			"   - img/titles      : Fondos y logotipos de la pantalla de título.\n"
			"   - img/system      : Windowskin, iconos, cursores y globos de expresión.\n"
			"   - img/ui          : Elementos gráficos del Diseñador UI/HUD.\n"
			"\n"
			"3. OTRAS CARPETAS DEL PROYECTO\n"
			"   - fonts           : Fuentes pixel art (.ttf, .otf, .fnt).\n"
			"   - movies          : Vídeos del juego (.ogv, .webm).\n"
			"   - maps            : Escenas y datos de los mapas creados en el editor.\n"
			"   - clockgears_data : Archivos JSON de la Base de Datos del proyecto.\n"
			"========================================================================\n");

	fa->store_string(content);
	fa->close();
	return OK;
}

Error ClockGearsProjectStructure::initialize_new_project(const String &p_project_dir, const String &p_project_title, bool p_write_project_godot) {
	String base_dir = _resolve_base_dir(p_project_dir);
	if (base_dir.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		return ERR_CANT_CREATE;
	}
	if (!da->dir_exists(base_dir)) {
		Error derr = da->make_dir_recursive(base_dir);
		if (derr != OK) {
			return derr;
		}
	}

	// 1. Crear las 22 carpetas estándar 100% vacías (Regla Cero)
	int folder_count = 0;
	const FolderEntry *entries = get_standard_folders(folder_count);
	for (int i = 0; i < folder_count; i++) {
		String full_sub = base_dir.path_join(String::utf8(entries[i].rel_path));
		if (!da->dir_exists(full_sub)) {
			Error sub_err = da->make_dir_recursive(full_sub);
			if (sub_err != OK) {
				WARN_PRINT(vformat("ClockGearsProjectStructure: No se pudo crear la carpeta '%s'.", full_sub));
			}
		}
	}

	// 2. Generar project.godot con ajustes Pixel Art (Anexo A4.1) y preset de importación lossless/NEAREST
	if (p_write_project_godot) {
		String proj_file = base_dir.path_join("project.godot");
		String safe_title = p_project_title.strip_edges();
		if (safe_title.is_empty()) {
			safe_title = base_dir.get_file();
		}
		safe_title = safe_title.replace("\"", "'");

		Error ferr = OK;
		Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::WRITE, &ferr);
		if (fa.is_null() || ferr != OK) {
			return ferr != OK ? ferr : ERR_CANT_CREATE;
		}

		String godot_cfg = vformat(
				String::utf8("; Engine configuration file.\n"
				"; Generado por ClockGears Engine (Estándar Pixel Art Anexo A4.1)\n"
				"\n"
				"config_version=5\n"
				"\n"
				"[application]\n"
				"\n"
				"config/name=\"%s\"\n"
				"config/features=PackedStringArray(\"4.8\", \"Forward Plus\")\n"
				"config/icon=\"res://icon.svg\"\n"
				"\n"
				"[clockgears]\n"
				"\n"
				"pixel_art/tile_size=32\n"
				"pixel_art/base_width=960\n"
				"pixel_art/base_height=540\n"
				"pixel_art/integer_scaling=true\n"
				"\n"
				"[display]\n"),
				"\n"
				"window/size/viewport_width=960\n"
				"window/size/viewport_height=540\n"
				"window/size/window_width_override=1920\n"
				"window/size/window_height_override=1080\n"
				"window/stretch/mode=\"canvas_items\"\n"
				"window/stretch/aspect=\"keep\"\n"
				"window/stretch/scale_mode=\"integer\"\n"
				"\n"
				"[importer_defaults]\n"
				"\n"
				"texture={\n"
				"\"compress/Channel_pack\": 0,\n"
				"\"compress/hdr_compression\": 1,\n"
				"\"compress/high_quality\": false,\n"
				"\"compress/lossy_quality\": 1.0,\n"
				"\"compress/mode\": 0,\n"
				"\"compress/normal_map\": 0,\n"
				"\"detect_3d/compress_to\": 0,\n"
				"\"mipmaps/generate\": false,\n"
				"\"mipmaps/limit\": -1,\n"
				"\"process/fix_alpha_border\": true,\n"
				"\"process/hdr_as_srgb\": false,\n"
				"\"process/hdr_clamp_exposure\": false,\n"
				"\"process/normal_map_invert_y\": false,\n"
				"\"process/premult_alpha\": false,\n"
				"\"process/size_limit\": 0,\n"
				"\"roughness/mode\": 0,\n"
				"\"roughness/src_normal\": \"\"\n"
				"}\n"
				"\n"
				"[rendering]\n"
				"\n"
				"textures/canvas_textures/default_texture_filter=0\n"
				"2d/snap/snap_2d_transforms_to_pixel=true\n"
				"2d/snap/snap_2d_vertices_to_pixel=true\n",
				safe_title);

		fa->store_string(godot_cfg);
		fa->close();
	}

	// 3. Generar LEEME.txt en la raíz del proyecto (Requisito 7)
	ensure_readme_file(base_dir, false);

	return OK;
}

PackedStringArray ClockGearsProjectStructure::get_missing_folders(const String &p_project_dir) {
	PackedStringArray missing;
	String base_dir = _resolve_base_dir(p_project_dir);
	if (base_dir.is_empty()) {
		return missing;
	}
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		return missing;
	}
	int count = 0;
	const FolderEntry *entries = get_standard_folders(count);
	for (int i = 0; i < count; i++) {
		String rel = String::utf8(entries[i].rel_path);
		if (!da->dir_exists(base_dir.path_join(rel))) {
			missing.push_back(rel);
		}
	}
	return missing;
}

Error ClockGearsProjectStructure::create_missing_folders(const String &p_project_dir) {
	String base_dir = _resolve_base_dir(p_project_dir);
	if (base_dir.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		return ERR_CANT_CREATE;
	}
	int count = 0;
	const FolderEntry *entries = get_standard_folders(count);
	for (int i = 0; i < count; i++) {
		String full_sub = base_dir.path_join(String::utf8(entries[i].rel_path));
		if (!da->dir_exists(full_sub)) {
			da->make_dir_recursive(full_sub);
		}
	}
	ensure_readme_file(base_dir, false);
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan();
	}
	return OK;
}

String ClockGearsProjectStructure::resolve_project_folder_global(const String &p_rel_folder) {
	String base_dir = _resolve_base_dir("res://");
	if (base_dir.is_empty()) {
		return "";
	}
	String clean_rel = p_rel_folder.trim_prefix("res://").trim_suffix("/");
	String target = clean_rel.is_empty() ? base_dir : base_dir.path_join(clean_rel);
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_valid() && !da->dir_exists(target)) {
		da->make_dir_recursive(target);
	}
	return target;
}

Error ClockGearsProjectStructure::write_pixel_art_import_preset(const String &p_dest_file_path) {
	String ext = p_dest_file_path.get_extension().to_lower();
	if (ext != "png" && ext != "webp" && ext != "jpg" && ext != "jpeg") {
		return OK;
	}
	String import_path = p_dest_file_path + ".import";
	// Si ya existe un .import creado por Godot, actualizamos sus claves de compresión y mipmaps;
	// si aún no existe, escribimos la sección [params] para que el importador de Godot use lossless + sin mipmaps.
	String res_source = p_dest_file_path;
	if (ProjectSettings::get_singleton() && !p_dest_file_path.begins_with("res://")) {
		res_source = ProjectSettings::get_singleton()->localize_path(p_dest_file_path);
	}

	String existing_remap;
	if (FileAccess::exists(import_path)) {
		String old_txt = FileAccess::get_file_as_string(import_path);
		int params_pos = old_txt.find("[params]");
		if (params_pos > 0) {
			existing_remap = old_txt.substr(0, params_pos);
		}
	}

	if (existing_remap.is_empty()) {
		existing_remap = vformat(
				"[remap]\n"
				"\n"
				"importer=\"texture\"\n"
				"type=\"CompressedTexture2D\"\n"
				"\n"
				"[deps]\n"
				"\n"
				"source_file=\"%s\"\n"
				"\n",
				res_source);
	}

	String params_section =
			"[params]\n"
			"\n"
			"compress/mode=0\n"
			"compress/high_quality=false\n"
			"compress/lossy_quality=1.0\n"
			"compress/hdr_compression=1\n"
			"compress/normal_map=0\n"
			"compress/channel_pack=0\n"
			"mipmaps/generate=false\n"
			"mipmaps/limit=-1\n"
			"roughness/mode=0\n"
			"roughness/src_normal=\"\"\n"
			"process/fix_alpha_border=true\n"
			"process/premult_alpha=false\n"
			"process/normal_map_invert_y=false\n"
			"process/hdr_as_srgb=false\n"
			"process/hdr_clamp_exposure=false\n"
			"process/size_limit=0\n"
			"detect_3d/compress_to=0\n";

	Error err = OK;
	Ref<FileAccess> fa = FileAccess::open(import_path, FileAccess::WRITE, &err);
	if (fa.is_null() || err != OK) {
		return err != OK ? err : ERR_CANT_CREATE;
	}
	fa->store_string(existing_remap + params_section);
	fa->close();
	return OK;
}

bool ClockGearsProjectStructure::validate_folder_asset(const String &p_image_path, const String &p_rel_folder, String &r_warning_message) {
	r_warning_message = "";
	String ext = p_image_path.get_extension().to_lower();
	if (ext != "png" && ext != "webp" && ext != "jpg" && ext != "jpeg") {
		return true;
	}

	String clean_folder = p_rel_folder.trim_prefix("res://").trim_suffix("/");
	bool needs_mult_32 = (clean_folder == "img/tilesets" || clean_folder == "img/characters");
	if (!needs_mult_32) {
		return true;
	}

	Ref<Image> img = Image::load_from_file(p_image_path);
	if (img.is_null() || img->is_empty()) {
		return true;
	}

	int w = img->get_width();
	int h = img->get_height();
	int tile_sz = 32;
	if (ClockGearsPixelArt::get_singleton()) {
		tile_sz = MAX(1, ClockGearsPixelArt::get_singleton()->get_tile_size());
	}

	if ((w % tile_sz) != 0 || (h % tile_sz) != 0) {
		r_warning_message = vformat(
				CG_PM_TR("Aviso Pixel Art (32x32):\nEl archivo '%s' mide %dx%d px, que NO es múltiplo de %d px (carpeta '%s').\n\nSe recomienda usar dimensiones múltiplos exactos de %d px para evitar recortes en rejilla."),
				p_image_path.get_file(), w, h, tile_sz, clean_folder, tile_sz);
		WARN_PRINT(r_warning_message);
		return false;
	}
	return true;
}

// ============================================================================
// ClockGearsNewProjectDialog ("Archivo > Nuevo Proyecto...")
// ============================================================================
void ClockGearsNewProjectDialog::_bind_methods() {}

String ClockGearsNewProjectDialog::_get_target_project_dir() const {
	String parent = parent_dir_edit ? parent_dir_edit->get_text().strip_edges() : "";
	String name = project_name_edit ? project_name_edit->get_text().strip_edges() : "";
	if (parent.is_empty() || name.is_empty()) {
		return "";
	}
	String safe_folder = OS::get_singleton()->get_safe_dir_name(name);
	if (safe_folder.is_empty()) {
		safe_folder = "NuevoProyectoRPG";
	}
	return parent.path_join(safe_folder);
}

void ClockGearsNewProjectDialog::_on_fields_changed(const String &p_text) {
	String target = _get_target_project_dir();
	if (resolved_path_label) {
		resolved_path_label->set_text(target.is_empty() ? CG_PM_TR("(Selecciona nombre y carpeta destino)") : target);
	}
	if (!status_label) {
		return;
	}
	if (target.is_empty()) {
		status_label->set_text(CG_PM_TR("Introduce un nombre de proyecto y una carpeta válida."));
		status_label->add_theme_color_override("font_color", Color(0.95f, 0.55f, 0.45f));
		get_ok_button()->set_disabled(true);
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_valid() && da->file_exists(target.path_join("project.godot"))) {
		status_label->set_text(CG_PM_TR("Ya existe un project.godot en esa ruta. Elige otro nombre."));
		status_label->add_theme_color_override("font_color", Color(0.95f, 0.45f, 0.45f));
		get_ok_button()->set_disabled(true);
		return;
	}

	status_label->set_text(CG_PM_TR("Listo: se crearán las 22 carpetas vacías (Regla Cero), LEEME.txt y project.godot (960x540, 32x32, NEAREST)."));
	status_label->add_theme_color_override("font_color", Color(0.45f, 0.90f, 0.55f));
	get_ok_button()->set_disabled(false);
}

void ClockGearsNewProjectDialog::_on_browse_pressed() {
	if (!dir_dialog) {
		return;
	}
	String cur = parent_dir_edit ? parent_dir_edit->get_text().strip_edges() : "";
	if (!cur.is_empty()) {
		dir_dialog->set_current_dir(cur);
	}
	dir_dialog->popup_centered(Size2(700, 480) * EDSCALE);
}

void ClockGearsNewProjectDialog::_on_dir_selected(const String &p_dir) {
	if (parent_dir_edit) {
		parent_dir_edit->set_text(p_dir);
	}
	_on_fields_changed();
}

void ClockGearsNewProjectDialog::_on_confirmed() {
	String target = _get_target_project_dir();
	String title = project_name_edit ? project_name_edit->get_text().strip_edges() : "";
	if (target.is_empty() || title.is_empty()) {
		return;
	}

	Error err = ClockGearsProjectStructure::initialize_new_project(target, title, true);
	if (err != OK) {
		ERR_PRINT(vformat(String::utf8("ClockGearsNewProjectDialog: Error al crear el proyecto en '%s' (código %d)."), target, int(err)));
		return;
	}

	if (open_after_create_chk && open_after_create_chk->is_pressed()) {
		List<String> args;
		args.push_back("--path");
		args.push_back(target);
		args.push_back("--editor");
		OS::get_singleton()->create_instance(args);
	}
}

void ClockGearsNewProjectDialog::open_dialog() {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	if (parent_dir_edit && parent_dir_edit->get_text().is_empty()) {
		String docs = OS::get_singleton()->get_system_dir(OS::SYSTEM_DIR_DOCUMENTS);
		parent_dir_edit->set_text(docs);
	}
	if (project_name_edit && project_name_edit->get_text().is_empty()) {
		project_name_edit->set_text("Nuevo Proyecto RPG");
	}
	_on_fields_changed();
	popup_centered(Size2(640, 420) * EDSCALE);
	if (project_name_edit) {
		project_name_edit->grab_focus();
		project_name_edit->select_all();
	}
}

ClockGearsNewProjectDialog::ClockGearsNewProjectDialog() {
	set_title(CG_PM_TR("Nuevo Proyecto — ClockGears Engine"));
	set_ok_button_text(CG_PM_TR("Crear Proyecto"));
	set_cancel_button_text(CG_PM_TR("Cancelar"));

	VBoxContainer *vb = memnew(VBoxContainer);
	vb->add_theme_constant_override("separation", 10);
	add_child(vb);

	Label *intro = memnew(Label(CG_PM_TR("Crea un proyecto nuevo con la estructura estándar de carpetas de ClockGears Engine (estilo RPG Maker MZ) y configuración Pixel Art (960x540, escala entera, filtro NEAREST, tiles 32x32).")));
	intro->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(intro);
	vb->add_child(memnew(HSeparator));

	HBoxContainer *name_row = memnew(HBoxContainer);
	name_row->add_theme_constant_override("separation", 8);
	vb->add_child(name_row);
	Label *name_lbl = memnew(Label(CG_PM_TR("Nombre del Proyecto:")));
	name_lbl->set_custom_minimum_size(Size2(160 * EDSCALE, 0));
	name_row->add_child(name_lbl);
	project_name_edit = memnew(LineEdit);
	project_name_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	project_name_edit->connect("text_changed", callable_mp(this, &ClockGearsNewProjectDialog::_on_fields_changed));
	name_row->add_child(project_name_edit);

	HBoxContainer *dir_row = memnew(HBoxContainer);
	dir_row->add_theme_constant_override("separation", 8);
	vb->add_child(dir_row);
	Label *dir_lbl = memnew(Label(CG_PM_TR("Ubicación Padre:")));
	dir_lbl->set_custom_minimum_size(Size2(160 * EDSCALE, 0));
	dir_row->add_child(dir_lbl);
	parent_dir_edit = memnew(LineEdit);
	parent_dir_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	parent_dir_edit->connect("text_changed", callable_mp(this, &ClockGearsNewProjectDialog::_on_fields_changed));
	dir_row->add_child(parent_dir_edit);
	browse_dir_btn = memnew(Button(CG_PM_TR("Examinar...")));
	browse_dir_btn->connect("pressed", callable_mp(this, &ClockGearsNewProjectDialog::_on_browse_pressed));
	dir_row->add_child(browse_dir_btn);

	HBoxContainer *res_row = memnew(HBoxContainer);
	res_row->add_theme_constant_override("separation", 8);
	vb->add_child(res_row);
	Label *res_lbl = memnew(Label(CG_PM_TR("Ruta Final:")));
	res_lbl->set_custom_minimum_size(Size2(160 * EDSCALE, 0));
	res_row->add_child(res_lbl);
	resolved_path_label = memnew(Label);
	resolved_path_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	resolved_path_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	res_row->add_child(resolved_path_label);

	PanelContainer *folders_box = memnew(PanelContainer);
	Ref<StyleBoxFlat> fstyle = memnew(StyleBoxFlat);
	fstyle->set_anti_aliased(false);
	fstyle->set_bg_color(Color(0.09f, 0.10f, 0.12f, 1.0f));
	fstyle->set_border_width_all(1);
	fstyle->set_border_color(Color(0.24f, 0.27f, 0.33f, 1.0f));
	fstyle->set_content_margin_all(8);
	folders_box->add_theme_style_override("panel", fstyle);
	vb->add_child(folders_box);

	Label *folders_info = memnew(Label(CG_PM_TR(
			"Carpetas vacías que se generarán automáticamente (Regla Cero):\n"
			"• audio/bgm, audio/bgs, audio/me, audio/se, audio/voice\n"
			"• img/characters, img/portraits, img/faces, img/battlers, img/pictures, img/cutscenes,\n"
			"  img/tilesets, img/animations, img/parallaxes, img/battlebacks, img/titles, img/system, img/ui\n"
			"• fonts, movies, maps, clockgears_data + archivo LEEME.txt")));
	folders_info->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	folders_box->add_child(folders_info);

	open_after_create_chk = memnew(CheckBox(CG_PM_TR("Abrir el nuevo proyecto en una nueva ventana al crearlo")));
	open_after_create_chk->set_pressed(false);
	vb->add_child(open_after_create_chk);

	status_label = memnew(Label);
	status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(status_label);

	dir_dialog = memnew(FileDialog);
	dir_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	dir_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	dir_dialog->set_title(CG_PM_TR("Seleccionar Carpeta Padre para el Proyecto"));
	dir_dialog->connect("dir_selected", callable_mp(this, &ClockGearsNewProjectDialog::_on_dir_selected));
	add_child(dir_dialog);

	connect("confirmed", callable_mp(this, &ClockGearsNewProjectDialog::_on_confirmed));
}

// ============================================================================
// ClockGearsResourceManagerDialog ("Herramientas > Gestor de Recursos...")
// ============================================================================
void ClockGearsResourceManagerDialog::_bind_methods() {}

void ClockGearsResourceManagerDialog::_refresh_folder_list() {
	if (!folder_list) {
		return;
	}
	int prev_sel = current_folder_idx;
	folder_list->clear();

	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	String base_dir = _resolve_base_dir("res://");

	for (int i = 0; i < count; i++) {
		String rel = String::utf8(entries[i].rel_path);
		String full = base_dir.path_join(rel);
		int file_count = 0;
		Ref<DirAccess> da = DirAccess::open(full);
		if (da.is_valid()) {
			da->list_dir_begin();
			String fn = da->get_next();
			while (!fn.is_empty()) {
				if (!da->current_is_dir() && !fn.begins_with(".") && !fn.ends_with(".import") && !fn.ends_with(".uid")) {
					file_count++;
				}
				fn = da->get_next();
			}
			da->list_dir_end();
		}
		folder_list->add_item(vformat("%s (%d)", rel, file_count));
		folder_list->set_item_tooltip(i, String::utf8(entries[i].description));
	}

	if (prev_sel >= 0 && prev_sel < folder_list->get_item_count()) {
		folder_list->select(prev_sel);
	}
}

void ClockGearsResourceManagerDialog::_refresh_file_list() {
	if (!file_list) {
		return;
	}
	file_list->clear();

	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx < 0 || current_folder_idx >= count) {
		return;
	}

	const ClockGearsProjectStructure::FolderEntry &entry = entries[current_folder_idx];
	if (folder_desc_label) {
		folder_desc_label->set_text(vformat(String::utf8("res://%s — %s"), String::utf8(entry.rel_path), String::utf8(entry.description)));
	}
	if (preset_badge_label) {
		if (entry.is_pixel_art_image) {
			String extra = entry.require_multiple_of_32 ? CG_PM_TR(" | Validación: Múltiplo de 32 px") : String();
			preset_badge_label->set_text(CG_PM_TR("Preset Pixel Art activo: Filtro NEAREST | Mipmaps OFF | Compresión Lossless") + extra);
			preset_badge_label->add_theme_color_override("font_color", Color(0.50f, 0.88f, 0.62f));
		} else {
			preset_badge_label->set_text(vformat(CG_PM_TR("Formatos admitidos: %s"), String::utf8(entry.filter_extensions)));
			preset_badge_label->add_theme_color_override("font_color", Color(0.68f, 0.74f, 0.84f));
		}
	}

	String folder_global = ClockGearsProjectStructure::resolve_project_folder_global(String::utf8(entry.rel_path));
	Ref<DirAccess> da = DirAccess::open(folder_global);
	if (da.is_valid()) {
		PackedStringArray files;
		da->list_dir_begin();
		String fn = da->get_next();
		while (!fn.is_empty()) {
			if (!da->current_is_dir() && !fn.begins_with(".") && !fn.ends_with(".import") && !fn.ends_with(".uid")) {
				files.push_back(fn);
			}
			fn = da->get_next();
		}
		da->list_dir_end();
		files.sort();
		for (int i = 0; i < files.size(); i++) {
			file_list->add_item(files[i]);
			file_list->set_item_metadata(i, folder_global.path_join(files[i]));
		}
	}

	if (file_list->get_item_count() > 0) {
		file_list->select(0);
		_on_file_selected(0);
	} else {
		_on_file_selected(-1);
	}
}

void ClockGearsResourceManagerDialog::_on_folder_selected(int p_idx) {
	current_folder_idx = p_idx;
	_refresh_file_list();
}

void ClockGearsResourceManagerDialog::_on_file_selected(int p_idx) {
	bool valid = (file_list && p_idx >= 0 && p_idx < file_list->get_item_count());
	if (btn_export) {
		btn_export->set_disabled(!valid);
	}
	if (btn_delete) {
		btn_delete->set_disabled(!valid);
	}
	if (!valid) {
		if (preview_rect) {
			preview_rect->set_texture(Ref<Texture2D>());
		}
		if (file_info_label) {
			file_info_label->set_text(CG_PM_TR("(Carpeta vacía o sin archivo seleccionado)"));
		}
		return;
	}

	String full_path = file_list->get_item_metadata(p_idx);
	String ext = full_path.get_extension().to_lower();
	if (ext == "png" || ext == "webp" || ext == "jpg" || ext == "jpeg") {
		Ref<Image> img = Image::load_from_file(full_path);
		if (img.is_valid() && !img->is_empty()) {
			if (preview_rect) {
				preview_rect->set_texture(ImageTexture::create_from_image(img));
			}
			int w = img->get_width();
			int h = img->get_height();
			bool mult32 = ((w % 32) == 0 && (h % 32) == 0);
			if (file_info_label) {
				file_info_label->set_text(vformat(
						CG_PM_TR("%s\nDimensiones: %d x %d px (%s)\nFiltro: NEAREST (Pixel Art)"),
						full_path.get_file(), w, h,
						mult32 ? CG_PM_TR("Múltiplo de 32 px OK") : CG_PM_TR("No es múltiplo de 32 px")));
			}
			return;
		}
	}

	if (preview_rect) {
		preview_rect->set_texture(Ref<Texture2D>());
	}
	uint64_t fsize = 0;
	Ref<FileAccess> fa = FileAccess::open(full_path, FileAccess::READ);
	if (fa.is_valid()) {
		fsize = fa->get_length();
	}
	if (file_info_label) {
		file_info_label->set_text(vformat(CG_PM_TR("%s\nTamaño: %.1f KB"), full_path.get_file(), double(fsize) / 1024.0));
	}
}

void ClockGearsResourceManagerDialog::_on_import_pressed() {
	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx < 0 || current_folder_idx >= count || !import_file_dialog) {
		return;
	}
	const ClockGearsProjectStructure::FolderEntry &entry = entries[current_folder_idx];
	import_file_dialog->clear_filters();
	import_file_dialog->add_filter(vformat("%s ; %s", String::utf8(entry.filter_extensions), String::utf8(entry.filter_description)));
	import_file_dialog->set_title(vformat(CG_PM_TR("Importar a res://%s (Copiar al Proyecto)"), String::utf8(entry.rel_path)));
	import_file_dialog->popup_centered(Size2(740, 480) * EDSCALE);
}

void ClockGearsResourceManagerDialog::_on_import_files_selected(const PackedStringArray &p_files) {
	pending_import_files = p_files;
	pending_import_index = 0;
	_process_next_pending_import();
}

void ClockGearsResourceManagerDialog::_process_next_pending_import() {
	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx < 0 || current_folder_idx >= count) {
		return;
	}
	String rel_folder = String::utf8(entries[current_folder_idx].rel_path);
	String dest_dir = ClockGearsProjectStructure::resolve_project_folder_global(rel_folder);

	while (pending_import_index < pending_import_files.size()) {
		String src_file = pending_import_files[pending_import_index];
		String dest_file = dest_dir.path_join(src_file.get_file());

		// Si el archivo ya existe en la carpeta destino (y no es exactamente la misma ruta origen), avisar antes de sobrescribir
		if (FileAccess::exists(dest_file) && src_file.simplify_path() != dest_file.simplify_path()) {
			if (overwrite_confirm_dialog) {
				overwrite_confirm_dialog->set_text(vformat(
						CG_PM_TR("El archivo '%s' ya existe en 'res://%s'.\n¿Deseas reemplazarlo con la nueva copia?"),
						src_file.get_file(), rel_folder));
				overwrite_confirm_dialog->popup_centered();
			}
			return;
		}

		_perform_copy_import(src_file, dest_file);
		pending_import_index++;
	}

	_refresh_folder_list();
	_refresh_file_list();
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan();
	}
}

void ClockGearsResourceManagerDialog::_perform_copy_import(const String &p_src_file, const String &p_dest_file) {
	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx < 0 || current_folder_idx >= count) {
		return;
	}
	const ClockGearsProjectStructure::FolderEntry &entry = entries[current_folder_idx];
	String rel_folder = String::utf8(entry.rel_path);

	if (p_src_file.simplify_path() != p_dest_file.simplify_path()) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			Error err = da->copy(p_src_file, p_dest_file);
			if (err != OK) {
				ERR_PRINT(vformat("ClockGearsResourceManager: Error al copiar '%s' a '%s'.", p_src_file, p_dest_file));
				return;
			}
		}
	}

	// Requisito 5: Aplicar preset de importación Pixel Art (NEAREST, sin mipmaps, lossless) en img/
	if (entry.is_pixel_art_image) {
		ClockGearsProjectStructure::write_pixel_art_import_preset(p_dest_file);
	}

	// Requisito 5: Avisar si un tileset o personaje no es múltiplo de 32 px
	String warn_msg;
	if (!ClockGearsProjectStructure::validate_folder_asset(p_dest_file, rel_folder, warn_msg)) {
		if (warning_dialog) {
			warning_dialog->set_text(warn_msg);
			warning_dialog->popup_centered();
		}
	}
}

void ClockGearsResourceManagerDialog::_on_overwrite_confirmed() {
	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx >= 0 && current_folder_idx < count && pending_import_index < pending_import_files.size()) {
		String rel_folder = String::utf8(entries[current_folder_idx].rel_path);
		String dest_dir = ClockGearsProjectStructure::resolve_project_folder_global(rel_folder);
		String src_file = pending_import_files[pending_import_index];
		String dest_file = dest_dir.path_join(src_file.get_file());
		_perform_copy_import(src_file, dest_file);
	}
	pending_import_index++;
	_process_next_pending_import();
}

void ClockGearsResourceManagerDialog::_on_overwrite_canceled() {
	pending_import_index++;
	_process_next_pending_import();
}

void ClockGearsResourceManagerDialog::_on_export_pressed() {
	if (!file_list || !export_file_dialog) {
		return;
	}
	Vector<int> sel = file_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	String src_path = file_list->get_item_metadata(sel[0]);
	export_file_dialog->set_current_file(src_path.get_file());
	export_file_dialog->popup_centered(Size2(700, 460) * EDSCALE);
}

void ClockGearsResourceManagerDialog::_on_export_file_selected(const String &p_dest_path) {
	if (!file_list) {
		return;
	}
	Vector<int> sel = file_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	String src_path = file_list->get_item_metadata(sel[0]);
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_valid()) {
		da->copy(src_path, p_dest_path);
	}
}

void ClockGearsResourceManagerDialog::_on_delete_pressed() {
	if (!file_list || !delete_confirm_dialog) {
		return;
	}
	Vector<int> sel = file_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	String src_path = file_list->get_item_metadata(sel[0]);
	delete_confirm_dialog->set_text(vformat(CG_PM_TR("¿Eliminar el archivo '%s' del proyecto?"), src_path.get_file()));
	delete_confirm_dialog->popup_centered();
}

void ClockGearsResourceManagerDialog::_on_delete_confirmed() {
	if (!file_list) {
		return;
	}
	Vector<int> sel = file_list->get_selected_items();
	if (sel.is_empty()) {
		return;
	}
	String src_path = file_list->get_item_metadata(sel[0]);
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_valid()) {
		da->remove(src_path);
		if (da->file_exists(src_path + ".import")) {
			da->remove(src_path + ".import");
		}
	}
	_refresh_folder_list();
	_refresh_file_list();
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan();
	}
}

void ClockGearsResourceManagerDialog::_on_open_explorer_pressed() {
	int count = 0;
	const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
	if (current_folder_idx < 0 || current_folder_idx >= count) {
		return;
	}
	String folder_global = ClockGearsProjectStructure::resolve_project_folder_global(String::utf8(entries[current_folder_idx].rel_path));
	OS::get_singleton()->shell_show_in_file_manager(folder_global, true);
}

void ClockGearsResourceManagerDialog::_on_create_missing_pressed() {
	ClockGearsProjectStructure::create_missing_folders("res://");
	_refresh_folder_list();
	_refresh_file_list();
}

void ClockGearsResourceManagerDialog::open_manager(const String &p_initial_folder) {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	ClockGearsProjectStructure::create_missing_folders("res://");

	if (!p_initial_folder.is_empty()) {
		String clean = p_initial_folder.trim_prefix("res://").trim_suffix("/");
		int count = 0;
		const ClockGearsProjectStructure::FolderEntry *entries = ClockGearsProjectStructure::get_standard_folders(count);
		for (int i = 0; i < count; i++) {
			if (clean == String::utf8(entries[i].rel_path)) {
				current_folder_idx = i;
				break;
			}
		}
	}

	_refresh_folder_list();
	_refresh_file_list();

	Size2i avail(1280, 720);
	Window *pw = get_parent_visible_window();
	if (pw) {
		avail = pw->get_size();
	}
	popup_centered(Size2i(int(avail.x * 0.82f), int(avail.y * 0.82f)).max(Size2i(780, 480)));
}

ClockGearsResourceManagerDialog::ClockGearsResourceManagerDialog() {
	set_title(CG_PM_TR("Gestor de Recursos — ClockGears Engine"));
	set_ok_button_text(CG_PM_TR("Cerrar"));
	set_wrap_controls(false);
	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_min_size(Size2(760, 460) * EDSCALE);

	VBoxContainer *root_vb = memnew(VBoxContainer);
	root_vb->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	root_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	root_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	root_vb->add_theme_constant_override("separation", 8);
	add_child(root_vb);

	HBoxContainer *top_row = memnew(HBoxContainer);
	top_row->add_theme_constant_override("separation", 8);
	root_vb->add_child(top_row);

	folder_desc_label = memnew(Label(CG_PM_TR("Selecciona una carpeta del proyecto para gestionar sus recursos.")));
	folder_desc_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	folder_desc_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	top_row->add_child(folder_desc_label);

	btn_create_missing = memnew(Button(CG_PM_TR("Verificar / Crear Carpetas Estándar")));
	btn_create_missing->connect("pressed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_create_missing_pressed));
	top_row->add_child(btn_create_missing);

	preset_badge_label = memnew(Label);
	preset_badge_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	root_vb->add_child(preset_badge_label);
	root_vb->add_child(memnew(HSeparator));

	HBoxContainer *cols_hb = memnew(HBoxContainer);
	cols_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	cols_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	cols_hb->add_theme_constant_override("separation", 12);
	root_vb->add_child(cols_hb);

	// Columna 1: Carpetas del proyecto
	VBoxContainer *col1 = memnew(VBoxContainer);
	col1->set_custom_minimum_size(Size2(250 * EDSCALE, 0));
	col1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col1->add_theme_constant_override("separation", 6);
	cols_hb->add_child(col1);

	col1->add_child(memnew(Label(CG_PM_TR("Carpetas del Proyecto (res://):"))));
	folder_list = memnew(ItemList);
	folder_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	folder_list->connect("item_selected", callable_mp(this, &ClockGearsResourceManagerDialog::_on_folder_selected));
	col1->add_child(folder_list);

	// Columna 2: Archivos en la carpeta seleccionada + Botones de acción
	VBoxContainer *col2 = memnew(VBoxContainer);
	col2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	col2->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col2->add_theme_constant_override("separation", 6);
	cols_hb->add_child(col2);

	col2->add_child(memnew(Label(CG_PM_TR("Archivos en la Carpeta:"))));
	file_list = memnew(ItemList);
	file_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	file_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	file_list->connect("item_selected", callable_mp(this, &ClockGearsResourceManagerDialog::_on_file_selected));
	col2->add_child(file_list);

	HBoxContainer *act_row = memnew(HBoxContainer);
	act_row->add_theme_constant_override("separation", 6);
	col2->add_child(act_row);

	btn_import = memnew(Button(CG_PM_TR("Importar (Copiar al Proyecto)...")));
	btn_import->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	btn_import->connect("pressed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_import_pressed));
	act_row->add_child(btn_import);

	btn_export = memnew(Button(CG_PM_TR("Exportar...")));
	btn_export->connect("pressed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_export_pressed));
	act_row->add_child(btn_export);

	btn_delete = memnew(Button(CG_PM_TR("Eliminar")));
	btn_delete->connect("pressed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_delete_pressed));
	act_row->add_child(btn_delete);

	btn_open_explorer = memnew(Button(CG_PM_TR("Abrir Carpeta")));
	btn_open_explorer->connect("pressed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_open_explorer_pressed));
	act_row->add_child(btn_open_explorer);

	// Columna 3: Vista Previa Pixel Art (NEAREST) y Diagnóstico 32x32
	VBoxContainer *col3 = memnew(VBoxContainer);
	col3->set_custom_minimum_size(Size2(250 * EDSCALE, 0));
	col3->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	col3->add_theme_constant_override("separation", 8);
	cols_hb->add_child(col3);

	col3->add_child(memnew(Label(CG_PM_TR("Vista Previa Pixel Art (NEAREST):"))));

	PanelContainer *prev_frame = memnew(PanelContainer);
	prev_frame->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	Ref<StyleBoxFlat> pstyle = memnew(StyleBoxFlat);
	pstyle->set_anti_aliased(false);
	pstyle->set_bg_color(Color(0.08f, 0.09f, 0.11f, 1.0f));
	pstyle->set_border_width_all(1);
	pstyle->set_border_color(Color(0.28f, 0.31f, 0.37f, 1.0f));
	pstyle->set_content_margin_all(8);
	prev_frame->add_theme_style_override("panel", pstyle);
	col3->add_child(prev_frame);

	preview_rect = memnew(TextureRect);
	preview_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	preview_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	preview_rect->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	prev_frame->add_child(preview_rect);

	file_info_label = memnew(Label);
	file_info_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	col3->add_child(file_info_label);

	// Diálogos auxiliares
	import_file_dialog = memnew(FileDialog);
	import_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILES);
	import_file_dialog->connect("files_selected", callable_mp(this, &ClockGearsResourceManagerDialog::_on_import_files_selected));
	add_child(import_file_dialog);

	export_file_dialog = memnew(FileDialog);
	export_file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	export_file_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
	export_file_dialog->set_title(CG_PM_TR("Exportar Copia de Recurso"));
	export_file_dialog->connect("file_selected", callable_mp(this, &ClockGearsResourceManagerDialog::_on_export_file_selected));
	add_child(export_file_dialog);

	overwrite_confirm_dialog = memnew(ConfirmationDialog);
	overwrite_confirm_dialog->set_title(CG_PM_TR("El Archivo Ya Existe"));
	overwrite_confirm_dialog->set_ok_button_text(CG_PM_TR("Reemplazar"));
	overwrite_confirm_dialog->set_cancel_button_text(CG_PM_TR("Omitir"));
	overwrite_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_overwrite_confirmed));
	overwrite_confirm_dialog->connect("canceled", callable_mp(this, &ClockGearsResourceManagerDialog::_on_overwrite_canceled));
	add_child(overwrite_confirm_dialog);

	delete_confirm_dialog = memnew(ConfirmationDialog);
	delete_confirm_dialog->set_title(CG_PM_TR("Confirmar Eliminación"));
	delete_confirm_dialog->set_ok_button_text(CG_PM_TR("Eliminar"));
	delete_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsResourceManagerDialog::_on_delete_confirmed));
	add_child(delete_confirm_dialog);

	warning_dialog = memnew(AcceptDialog);
	warning_dialog->set_title(CG_PM_TR("Aviso de Dimensiones Pixel Art (32x32)"));
	add_child(warning_dialog);
}
