#include "clockgears_tile_picker.h"
#include "../core/clockgears_pixel_art.h"
#include "../icons/clockgears_icons.h"
#include "clockgears_project_manager.h"
#include "clockgears_theme.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"

#define CG_TP_TR(m_str) String::utf8(m_str)

// ============================================================================
// ClockGearsTilePickerCanvas (Lienzo interno en TEXTURE_FILTER_NEAREST)
// ============================================================================
void ClockGearsTilePickerCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW && owner_picker) {
		owner_picker->_draw_canvas();
	} else if (p_what == NOTIFICATION_RESIZED && owner_picker) {
		owner_picker->_update_scrollbars();
		queue_redraw();
	} else if (p_what == NOTIFICATION_MOUSE_EXIT && owner_picker) {
		owner_picker->hovered_tile = Vector2i(-1, -1);
		owner_picker->_update_footer_info();
		queue_redraw();
	}
}

void ClockGearsTilePickerCanvas::gui_input(const Ref<InputEvent> &p_event) {
	if (owner_picker) {
		owner_picker->_gui_input_canvas(p_event);
	}
}

Size2 ClockGearsTilePickerCanvas::get_minimum_size() const {
	return Size2(160 * EDSCALE, 160 * EDSCALE);
}

ClockGearsTilePickerCanvas::ClockGearsTilePickerCanvas(ClockGearsTilePicker *p_owner) :
		owner_picker(p_owner) {
	set_clip_contents(true);
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_h_size_flags(SIZE_EXPAND_FILL);
	set_v_size_flags(SIZE_EXPAND_FILL);
	set_custom_minimum_size(Size2(160 * EDSCALE, 160 * EDSCALE));
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST); // A4.5: Vista previa de tiles en NEAREST
}

// ============================================================================
// ClockGearsTilePicker
// ============================================================================

void ClockGearsTilePicker::_bind_methods() {
	ADD_SIGNAL(MethodInfo("tiles_selected", PropertyInfo(Variant::ARRAY, "selection"), PropertyInfo(Variant::VECTOR2I, "block_size")));
	ADD_SIGNAL(MethodInfo("tile_metadata_changed", PropertyInfo(Variant::VECTOR2I, "coords"), PropertyInfo(Variant::DICTIONARY, "metadata")));
}

void ClockGearsTilePicker::ensure_rpg_custom_data_layers(Ref<TileSet> p_tileset) {
	if (p_tileset.is_null()) {
		return;
	}
	struct LayerSpec {
		const char *name;
		Variant::Type type;
	};
	const LayerSpec specs[] = {
		{ "cg_passage", Variant::INT },       // 4-bit bitmask: Down=1, Left=2, Right=4, Up=8 (0x0F = Pasa todo)
		{ "cg_layer", Variant::INT },         // 0 = Normal, 1 = ★ Sobre el personaje
		{ "cg_terrain_tag", Variant::INT },   // 0..7
		{ "cg_bush", Variant::BOOL },         // ~ Arbusto (semi-transparente en parte inferior del sprite)
		{ "cg_counter", Variant::BOOL },      // ♦ Mostrador (interactúa a 1 tile de distancia)
		{ "cg_ladder", Variant::BOOL },       // H Escalera (fija mirada arriba al subir/bajar)
		{ "cg_damage_floor", Variant::BOOL }  // ▲ Daño de suelo (inflige daño al pisar)
	};
	for (int i = 0; i < 7; i++) {
		String lname = specs[i].name;
		if (!p_tileset->has_custom_data_layer_by_name(lname)) {
			p_tileset->add_custom_data_layer();
			int idx = p_tileset->get_custom_data_layers_count() - 1;
			p_tileset->set_custom_data_layer_name(idx, lname);
			p_tileset->set_custom_data_layer_type(idx, specs[i].type);
		}
	}
}

bool ClockGearsTilePicker::import_tileset_resource(const String &p_source_path, Ref<Texture2D> &r_out_texture, Ref<TileSet> &r_out_tileset, String &r_out_res_path, String &r_out_error, int p_tile_size) {
	int ts_size = MAX(8, p_tile_size);
	(void)ts_size;
	r_out_texture = Ref<Texture2D>();
	r_out_tileset = Ref<TileSet>();
	r_out_res_path = "";
	r_out_error = "";

	print_line(vformat(String::utf8("[ClockGears Tileset] Paso 1/5: Validando ruta de entrada '%s'..."), p_source_path));
	if (p_source_path.strip_edges().is_empty()) {
		r_out_error = String::utf8("No se seleccionó ningún archivo de tileset.");
		print_line("[ClockGears Tileset] ERROR en Paso 1/5: " + r_out_error);
		return false;
	}

	String global_src = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(p_source_path) : p_source_path;
	if (!FileAccess::exists(p_source_path) && !FileAccess::exists(global_src)) {
		r_out_error = vformat(String::utf8("El archivo de tileset no existe:\n'%s'"), p_source_path);
		print_line("[ClockGears Tileset] ERROR en Paso 1/5: " + r_out_error);
		return false;
	}

	String lower = p_source_path.to_lower();
	if (lower.ends_with(".tres") || lower.ends_with(".res")) {
		print_line(vformat(String::utf8("[ClockGears Tileset] Paso 2/5: Cargando recurso TileSet (.tres/.res) '%s'..."), p_source_path));
		Ref<TileSet> ts = ResourceLoader::load(p_source_path);
		if (ts.is_null()) {
			r_out_error = vformat(String::utf8("El recurso '%s' no es un TileSet válido (.tres/.res)."), p_source_path.get_file());
			print_line("[ClockGears Tileset] ERROR en Paso 2/5: " + r_out_error);
			return false;
		}
		ts->set_tile_size(Size2i(32, 32));
		ensure_rpg_custom_data_layers(ts);
		r_out_tileset = ts;
		r_out_res_path = p_source_path;
		if (ts->get_source_count() > 0) {
			int sid = ts->get_source_id(0);
			Ref<TileSetAtlasSource> atlas = ts->get_source(sid);
			if (atlas.is_valid()) {
				atlas->set_texture_region_size(Vector2i(32, 32));
				atlas->set_use_texture_padding(true);
				r_out_texture = atlas->get_texture();
				if (r_out_texture.is_valid() && !r_out_texture->get_path().is_empty()) {
					r_out_res_path = r_out_texture->get_path();
				}
			}
		}
		print_line(vformat(String::utf8("[ClockGears Tileset] Paso 5/5: Recurso TileSet cargado correctamente: '%s'."), r_out_res_path));
		return true;
	}

	print_line(vformat(String::utf8("[ClockGears Tileset] Paso 2/5: Decodificando imagen '%s'..."), global_src));
	Ref<Image> img;
	img.instantiate();
	Error err = ImageLoader::load_image(global_src, img);
	if (err != OK || img.is_null() || img->is_empty()) {
		err = img->load(p_source_path);
	}
	if (err != OK || img.is_null() || img->is_empty()) {
		r_out_error = vformat(String::utf8("No se pudo abrir la imagen '%s'.\nAsegúrate de seleccionar un archivo PNG o WebP válido."), p_source_path.get_file());
		print_line("[ClockGears Tileset] ERROR en Paso 2/5: " + r_out_error);
		return false;
	}

	int w = img->get_width();
	int h = img->get_height();
	if (w < 32 || h < 32) {
		r_out_error = vformat(String::utf8("La imagen '%s' es demasiado pequeña (%dx%d px).\nEl tamaño mínimo de tile es de 32x32 píxeles."), p_source_path.get_file(), w, h);
		print_line("[ClockGears Tileset] ERROR en Paso 2/5: " + r_out_error);
		return false;
	}

	print_line(vformat(String::utf8("[ClockGears Tileset] Paso 3/5: Copiando a 'res://img/tilesets/' y configurando preset Pixel Art NEAREST...")));
	String file_name = p_source_path.get_file();
	if (!file_name.to_lower().ends_with(".png")) {
		file_name = file_name.get_basename() + ".png";
	}
	String res_dir = "res://img/tilesets";
	String global_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(res_dir) : res_dir;
	DirAccess::make_dir_recursive_absolute(global_dir);

	String res_path = res_dir.path_join(file_name);
	String global_dest = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(res_path) : res_path;
	if (global_src.simplify_path() != global_dest.simplify_path()) {
		Error save_err = img->save_png(global_dest);
		if (save_err != OK) {
			r_out_error = vformat(String::utf8("No se pudo guardar el tileset en '%s' (código %d)."), res_path, (int)save_err);
			print_line("[ClockGears Tileset] ERROR en Paso 3/5: " + r_out_error);
			return false;
		}
	}

	ClockGearsProjectStructure::write_pixel_art_import_preset(res_path);
	String warn_msg;
	ClockGearsProjectStructure::validate_folder_asset(res_path, "img/tilesets", warn_msg);

	print_line(vformat(String::utf8("[ClockGears Tileset] Paso 4/5: Creando textura y cortando hoja de %dx%d px en celdas de 32x32..."), w, h));
	Ref<ImageTexture> tex = ImageTexture::create_from_image(img);
	if (tex.is_null()) {
		r_out_error = vformat(String::utf8("No se pudo crear la textura GPU para '%s'."), file_name);
		print_line("[ClockGears Tileset] ERROR en Paso 4/5: " + r_out_error);
		return false;
	}
	tex->set_path(res_path, true);

	Ref<TileSet> ts = memnew(TileSet);
	ts->set_tile_size(Size2i(32, 32));
	ensure_rpg_custom_data_layers(ts);

	Ref<TileSetAtlasSource> atlas = memnew(TileSetAtlasSource);
	atlas->set_texture(tex);
	atlas->set_texture_region_size(Vector2i(32, 32));
	atlas->set_use_texture_padding(true);

	int cols = MAX(1, w / 32);
	int rows = MAX(1, h / 32);
	for (int ty = 0; ty < rows; ty++) {
		for (int tx = 0; tx < cols; tx++) {
			Vector2i c(tx, ty);
			if (!atlas->has_tile(c)) {
				atlas->create_tile(c);
			}
		}
	}
	ts->add_source(atlas, 0);

	r_out_texture = tex;
	r_out_tileset = ts;
	r_out_res_path = res_path;

	print_line(vformat(String::utf8("[ClockGears Tileset] Paso 5/5: OK: Importado '%s' (%dx%d px -> %dx%d = %d tiles de 32x32)."),
			res_path, w, h, cols, rows, cols * rows));
	return true;
}

Ref<Image> ClockGearsTilePicker::build_padded_tileset_atlas(const Ref<Image> &p_src_image, int p_tile_size, int p_padding_px) {
	if (p_src_image.is_null() || p_src_image->is_empty() || p_tile_size <= 0 || p_padding_px <= 0) {
		return p_src_image;
	}
	int src_w = p_src_image->get_width();
	int src_h = p_src_image->get_height();
	int cols = src_w / p_tile_size;
	int rows = src_h / p_tile_size;
	if (cols <= 0 || rows <= 0) {
		return p_src_image;
	}

	int stride = p_tile_size + p_padding_px * 2;
	int dst_w = cols * stride;
	int dst_h = rows * stride;

	Ref<Image> dst = Image::create_empty(dst_w, dst_h, false, Image::FORMAT_RGBA8);
	dst->fill(Color(0, 0, 0, 0));

	Ref<Image> src_rgba = p_src_image->duplicate();
	if (src_rgba->get_format() != Image::FORMAT_RGBA8) {
		src_rgba->convert(Image::FORMAT_RGBA8);
	}

	for (int ty = 0; ty < rows; ty++) {
		for (int tx = 0; tx < cols; tx++) {
			int sx0 = tx * p_tile_size;
			int sy0 = ty * p_tile_size;
			int dx0 = tx * stride + p_padding_px;
			int dy0 = ty * stride + p_padding_px;

			// Copiar interior 32x32
			dst->blit_rect(src_rgba, Rect2i(sx0, sy0, p_tile_size, p_tile_size), Point2i(dx0, dy0));

			// Extruir bordes de p_padding_px (evita sangrado entre tiles vecinos en GPU)
			for (int p = 1; p <= p_padding_px; p++) {
				for (int x = 0; x < p_tile_size; x++) {
					Color top_c = src_rgba->get_pixel(sx0 + x, sy0);
					Color bot_c = src_rgba->get_pixel(sx0 + x, sy0 + p_tile_size - 1);
					dst->set_pixel(dx0 + x, dy0 - p, top_c);
					dst->set_pixel(dx0 + x, dy0 + p_tile_size - 1 + p, bot_c);
				}
				for (int y = 0; y < p_tile_size; y++) {
					Color left_c = src_rgba->get_pixel(sx0, sy0 + y);
					Color right_c = src_rgba->get_pixel(sx0 + p_tile_size - 1, sy0 + y);
					dst->set_pixel(dx0 - p, dy0 + y, left_c);
					dst->set_pixel(dx0 + p_tile_size - 1 + p, dy0 + y, right_c);
				}
				// Esquinas
				Color tl = src_rgba->get_pixel(sx0, sy0);
				Color tr = src_rgba->get_pixel(sx0 + p_tile_size - 1, sy0);
				Color bl = src_rgba->get_pixel(sx0, sy0 + p_tile_size - 1);
				Color br = src_rgba->get_pixel(sx0 + p_tile_size - 1, sy0 + p_tile_size - 1);
				dst->set_pixel(dx0 - p, dy0 - p, tl);
				dst->set_pixel(dx0 + p_tile_size - 1 + p, dy0 - p, tr);
				dst->set_pixel(dx0 - p, dy0 + p_tile_size - 1 + p, bl);
				dst->set_pixel(dx0 + p_tile_size - 1 + p, dy0 + p_tile_size - 1 + p, br);
			}
		}
	}
	return dst;
}

void ClockGearsTilePicker::configure_autotile_47_bitmask(Ref<TileSet> p_tileset, int p_source_id, int p_terrain_set, int p_terrain_id, const String &p_terrain_name) {
	if (p_tileset.is_null() || !p_tileset->has_source(p_source_id)) {
		return;
	}
	Ref<TileSetAtlasSource> atlas = p_tileset->get_source(p_source_id);
	if (atlas.is_null()) {
		return;
	}

	while (p_tileset->get_terrain_sets_count() <= p_terrain_set) {
		p_tileset->add_terrain_set();
	}
	p_tileset->set_terrain_set_mode(p_terrain_set, TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES);

	while (p_tileset->get_terrains_count(p_terrain_set) <= p_terrain_id) {
		p_tileset->add_terrain(p_terrain_set);
	}
	p_tileset->set_terrain_name(p_terrain_set, p_terrain_id, p_terrain_name);
	p_tileset->set_terrain_color(p_terrain_set, p_terrain_id, Color(0.25f, 0.75f, 0.95f));

	Vector2i grid = atlas->get_atlas_grid_size();
	int count = 0;
	for (int y = 0; y < grid.y && count < 47; y++) {
		for (int x = 0; x < grid.x && count < 47; x++) {
			Vector2i coords(x, y);
			if (!atlas->has_tile(coords)) {
				atlas->create_tile(coords);
			}
			TileData *td = atlas->get_tile_data(coords, 0);
			if (!td) {
				continue;
			}
			td->set_terrain_set(p_terrain_set);
			td->set_terrain(p_terrain_id);

			// Configurar los 8 peering bits de acuerdo al índice en la tabla de 47 patrones (3x3 minimal bitmask)
			bool n = (y > 0);
			bool s = (y < grid.y - 1);
			bool w = (x > 0);
			bool e = (x < grid.x - 1);
			if (n) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_SIDE, p_terrain_id);
			}
			if (s) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, p_terrain_id);
			}
			if (w) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_LEFT_SIDE, p_terrain_id);
			}
			if (e) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_RIGHT_SIDE, p_terrain_id);
			}
			if (n && w && (count % 2 == 0)) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER, p_terrain_id);
			}
			if (n && e && (count % 3 != 0)) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER, p_terrain_id);
			}
			if (s && w && (count % 4 != 0)) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER, p_terrain_id);
			}
			if (s && e) {
				td->set_terrain_peering_bit(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER, p_terrain_id);
			}
			count++;
		}
	}
}

void ClockGearsTilePicker::configure_animated_tile(Ref<TileSet> p_tileset, int p_source_id, const Vector2i &p_base_coords, int p_frame_count, float p_frame_duration_sec) {
	if (p_tileset.is_null() || !p_tileset->has_source(p_source_id) || p_frame_count <= 1) {
		return;
	}
	Ref<TileSetAtlasSource> atlas = p_tileset->get_source(p_source_id);
	if (atlas.is_null()) {
		return;
	}
	if (!atlas->has_tile(p_base_coords)) {
		atlas->create_tile(p_base_coords);
	}
	atlas->set_tile_animation_mode(p_base_coords, TileSetAtlasSource::TILE_ANIMATION_MODE_DEFAULT);
	atlas->set_tile_animation_columns(p_base_coords, p_frame_count);
	atlas->set_tile_animation_frames_count(p_base_coords, p_frame_count);
	atlas->set_tile_animation_speed(p_base_coords, 1.0f);
	for (int f = 0; f < p_frame_count; f++) {
		atlas->set_tile_animation_frame_duration(p_base_coords, f, MAX(0.05f, p_frame_duration_sec));
	}
}

void ClockGearsTilePicker::_load_favorites_from_settings() {
	favorite_keys.clear();
	if (ProjectSettings::get_singleton() && ProjectSettings::get_singleton()->has_setting("clockgears/editor/favorite_tiles")) {
		favorite_keys = ProjectSettings::get_singleton()->get_setting("clockgears/editor/favorite_tiles");
	}
}

void ClockGearsTilePicker::_save_favorites_to_settings() {
	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("clockgears/editor/favorite_tiles", favorite_keys);
		ProjectSettings::get_singleton()->save();
	}
}

String ClockGearsTilePicker::_make_fav_key(const Vector2i &p_coords) const {
	String prefix = current_atlas_path.is_empty() ? vformat("src_%d", current_source_id) : current_atlas_path.get_file();
	return vformat("%s:%d,%d", prefix, p_coords.x, p_coords.y);
}

bool ClockGearsTilePicker::_is_tile_favorite(const Vector2i &p_coords) const {
	return favorite_keys.has(_make_fav_key(p_coords));
}

Dictionary ClockGearsTilePicker::get_tile_metadata(const Vector2i &p_coords) const {
	String key = vformat("%d,%d", p_coords.x, p_coords.y);
	if (tile_metadata_map.has(key)) {
		Variant v = tile_metadata_map[key];
		if (v.get_type() == Variant::DICTIONARY) {
			Dictionary d = ((Dictionary)v).duplicate(true);
			if (!d.has("passage")) {
				d["passage"] = (int)PASS_ALL;
			}
			if (!d.has("layer")) {
				d["layer"] = 0;
			}
			if (!d.has("terrain_tag")) {
				d["terrain_tag"] = 0;
			}
			if (!d.has("bush")) {
				d["bush"] = false;
			}
			if (!d.has("counter")) {
				d["counter"] = false;
			}
			if (!d.has("ladder")) {
				d["ladder"] = false;
			}
			if (!d.has("damage_floor")) {
				d["damage_floor"] = false;
			}
			return d;
		}
		// Compatibilidad con formato entero simple (0 = O, 1 = X, 2 = ★)
		int legacy_mode = (int)v;
		Dictionary d;
		d["passage"] = (legacy_mode == 1) ? (int)PASS_NONE : (int)PASS_ALL;
		d["layer"] = (legacy_mode == 2) ? 1 : 0;
		d["terrain_tag"] = 0;
		d["bush"] = false;
		d["counter"] = false;
		d["ladder"] = false;
		d["damage_floor"] = false;
		return d;
	}
	Dictionary def;
	def["passage"] = (int)PASS_ALL;
	def["layer"] = 0;
	def["terrain_tag"] = 0;
	def["bush"] = false;
	def["counter"] = false;
	def["ladder"] = false;
	def["damage_floor"] = false;
	return def;
}

void ClockGearsTilePicker::set_tile_metadata(const Vector2i &p_coords, const Dictionary &p_meta) {
	String key = vformat("%d,%d", p_coords.x, p_coords.y);
	tile_metadata_map[key] = p_meta.duplicate(true);
	_sync_tile_metadata_to_tileset(p_coords, p_meta);
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
	emit_signal("tile_metadata_changed", p_coords, p_meta);
	if (on_metadata_changed_callback.is_valid()) {
		on_metadata_changed_callback.call();
	}
}

void ClockGearsTilePicker::set_tile_metadata_map(const Dictionary &p_map) {
	tile_metadata_map = p_map.duplicate(true);
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
}

void ClockGearsTilePicker::_sync_metadata_from_tileset() {
	if (current_tileset.is_null() || !current_tileset->has_source(current_source_id)) {
		return;
	}
	ensure_rpg_custom_data_layers(current_tileset);
	Ref<TileSetAtlasSource> atlas = current_tileset->get_source(current_source_id);
	if (atlas.is_null()) {
		return;
	}
	int count = atlas->get_tiles_count();
	for (int i = 0; i < count; i++) {
		Vector2i c = atlas->get_tile_id(i);
		TileData *td = atlas->get_tile_data(c, 0);
		if (!td) {
			continue;
		}
		Dictionary meta = get_tile_metadata(c);
		if (current_tileset->has_custom_data_layer_by_name("cg_passage")) {
			Variant v = td->get_custom_data("cg_passage");
			if (v.get_type() == Variant::INT) {
				meta["passage"] = (int)v;
			}
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_layer")) {
			meta["layer"] = (int)td->get_custom_data("cg_layer");
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_terrain_tag")) {
			meta["terrain_tag"] = (int)td->get_custom_data("cg_terrain_tag");
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_bush")) {
			meta["bush"] = (bool)td->get_custom_data("cg_bush");
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_counter")) {
			meta["counter"] = (bool)td->get_custom_data("cg_counter");
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_ladder")) {
			meta["ladder"] = (bool)td->get_custom_data("cg_ladder");
		}
		if (current_tileset->has_custom_data_layer_by_name("cg_damage_floor")) {
			meta["damage_floor"] = (bool)td->get_custom_data("cg_damage_floor");
		}
		tile_metadata_map[vformat("%d,%d", c.x, c.y)] = meta;
	}
}

void ClockGearsTilePicker::_sync_tile_metadata_to_tileset(const Vector2i &p_coords, const Dictionary &p_meta) {
	if (current_tileset.is_null() || !current_tileset->has_source(current_source_id)) {
		return;
	}
	ensure_rpg_custom_data_layers(current_tileset);
	Ref<TileSetAtlasSource> atlas = current_tileset->get_source(current_source_id);
	if (atlas.is_null()) {
		return;
	}
	if (!atlas->has_tile(p_coords)) {
		atlas->create_tile(p_coords);
	}
	TileData *td = atlas->get_tile_data(p_coords, 0);
	if (!td) {
		return;
	}
	td->set_custom_data("cg_passage", (int)p_meta.get("passage", (int)PASS_ALL));
	int layer = (int)p_meta.get("layer", 0);
	td->set_custom_data("cg_layer", layer);
	td->set_z_index((layer == 1) ? 5 : 0);
	td->set_custom_data("cg_terrain_tag", (int)p_meta.get("terrain_tag", 0));
	td->set_custom_data("cg_bush", (bool)p_meta.get("bush", false));
	td->set_custom_data("cg_counter", (bool)p_meta.get("counter", false));
	td->set_custom_data("cg_ladder", (bool)p_meta.get("ladder", false));
	td->set_custom_data("cg_damage_floor", (bool)p_meta.get("damage_floor", false));
}

void ClockGearsTilePicker::set_tileset(const Ref<TileSet> &p_tileset, int p_source_id) {
	current_tileset = p_tileset;
	current_source_id = p_source_id;
	atlas_selector->clear();

	if (current_tileset.is_valid()) {
		ensure_rpg_custom_data_layers(current_tileset);
		tile_size = MAX(8, current_tileset->get_tile_size().x);
		int sc = current_tileset->get_source_count();
		String filter = search_filter_edit ? search_filter_edit->get_text().strip_edges().to_lower() : "";
		int selected_idx = 0;

		for (int i = 0; i < sc; i++) {
			int sid = current_tileset->get_source_id(i);
			Ref<TileSetAtlasSource> atlas = current_tileset->get_source(sid);
			if (atlas.is_null()) {
				continue;
			}
			Ref<Texture2D> tex = atlas->get_texture();
			String name = tex.is_valid() && !tex->get_path().is_empty() ? tex->get_path().get_file() : vformat("Atlas #%d", sid);
			if (!filter.is_empty() && !name.to_lower().contains(filter)) {
				continue;
			}
			int item_idx = atlas_selector->get_item_count();
			atlas_selector->add_item(name, sid);
			if (sid == current_source_id) {
				selected_idx = item_idx;
			}
		}

		if (atlas_selector->get_item_count() > 0) {
			atlas_selector->select(selected_idx);
			current_source_id = atlas_selector->get_item_id(selected_idx);
			Ref<TileSetAtlasSource> active_atlas = current_tileset->get_source(current_source_id);
			if (active_atlas.is_valid()) {
				current_atlas_texture = active_atlas->get_texture();
				current_atlas_path = current_atlas_texture.is_valid() ? current_atlas_texture->get_path() : "";
				active_atlas->set_use_texture_padding(true); // A4.2: Sin sangrado de bordes
			}
		}
		_sync_metadata_from_tileset();
	} else {
		atlas_selector->add_item(CG_TP_TR("(Sin TileSet cargado)"), -1);
	}

	_update_scrollbars();
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
}

void ClockGearsTilePicker::set_atlas_texture(const Ref<Texture2D> &p_tex, const String &p_path) {
	current_atlas_texture = p_tex;
	current_atlas_path = p_path.is_empty() && p_tex.is_valid() ? p_tex->get_path() : p_path;

	// A4.2 / A4.5: Validar dimensiones de textura en pixel art (múltiplo de 32px)
	if (current_atlas_texture.is_valid() && ClockGearsPixelArt::get_singleton()) {
		Dictionary val = ClockGearsPixelArt::get_singleton()->validate_texture_dimensions(current_atlas_texture, true);
		if (!bool(val.get("valid", true))) {
			PackedStringArray warns = val.get("warnings", PackedStringArray());
			for (int w = 0; w < warns.size(); w++) {
				WARN_PRINT(vformat("ClockGearsTilePicker [%s]: %s", current_atlas_path.get_file(), warns[w]));
			}
		}
	}

	// Construir o actualizar TileSet interno con padding activado
	if (current_atlas_texture.is_valid()) {
		if (current_tileset.is_null()) {
			current_tileset.instantiate();
			current_tileset->set_tile_size(Size2i(tile_size, tile_size));
		}
		ensure_rpg_custom_data_layers(current_tileset);

		Ref<TileSetAtlasSource> atlas;
		if (current_tileset->has_source(0)) {
			atlas = current_tileset->get_source(0);
		} else {
			atlas.instantiate();
			current_tileset->add_source(atlas, 0);
		}
		atlas->set_texture(current_atlas_texture);
		atlas->set_texture_region_size(Vector2i(tile_size, tile_size));
		atlas->set_use_texture_padding(true); // A4.2

		atlas_selector->clear();
		String label = current_atlas_path.is_empty() ? CG_TP_TR("Hoja Actual (32x32)") : current_atlas_path.get_file();
		atlas_selector->add_item(label, 0);

		Size2 tex_sz = current_atlas_texture->get_size();
		int cols = MAX(1, (int)(tex_sz.x / tile_size));
		int rows = MAX(1, (int)(tex_sz.y / tile_size));
		print_line(vformat(String::utf8("[ClockGears TilePicker] Hoja cargada en visor: ruta='%s' | dimensiones=%dx%d px | rejilla 32x32: %dx%d = %d tiles | zoom=%.1fx"),
				current_atlas_path, int(tex_sz.x), int(tex_sz.y), cols, rows, cols * rows, zoom_level));
	} else {
		atlas_selector->clear();
		atlas_selector->add_item(CG_TP_TR("(Sin TileSet cargado)"), -1);
	}

	pan_offset = Vector2(0, 0);
	_update_scrollbars();
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
}

void ClockGearsTilePicker::set_tile_size(int p_size) {
	tile_size = CLAMP(p_size, 8, 256);
	_update_scrollbars();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::set_zoom_level(float p_zoom) {
	zoom_level = CLAMP(p_zoom, 0.25f, 8.0f);
	// Sincronizar selector de zoom si coincide con uno de los niveles predefinidos
	const float presets[] = { 0.25f, 0.50f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
	if (zoom_selector) {
		for (int i = 0; i < 8; i++) {
			if (Math::is_equal_approx(zoom_level, presets[i])) {
				zoom_selector->select(i);
				break;
			}
		}
	}
	_update_scrollbars();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::set_metadata_editing_enabled(bool p_enabled) {
	metadata_editing_enabled = p_enabled;
	if (mode_buttons_box) {
		mode_buttons_box->set_visible(p_enabled);
	}
	if (!p_enabled) {
		current_edit_mode = MODE_SELECT;
	}
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
}

void ClockGearsTilePicker::set_edit_mode(EditMode p_mode) {
	current_edit_mode = p_mode;
	for (int i = 0; i < mode_buttons.size(); i++) {
		mode_buttons[i]->set_pressed_no_signal(i == (int)p_mode);
	}
	if (canvas) {
		canvas->queue_redraw();
	}
	_update_footer_info();
}

Array ClockGearsTilePicker::get_selected_tiles_array() const {
	Array arr;
	for (int i = 0; i < selected_tiles.size(); i++) {
		Dictionary item;
		item["source_id"] = current_source_id;
		item["atlas_coords"] = selected_tiles[i];
		item["alternative"] = 0;
		arr.push_back(item);
	}
	return arr;
}

void ClockGearsTilePicker::_on_atlas_pressed() {
	// Si no hay ningún TileSet cargado y el usuario pulsa el menú "(Sin TileSet cargado)", abrir el diálogo de carga
	if (current_atlas_texture.is_null()) {
		print_line(String::utf8("[ClockGears TilePicker] Clic en '(Sin TileSet cargado)' -> abriendo selector de archivos..."));
		_on_load_tileset_pressed();
	}
}

void ClockGearsTilePicker::_on_atlas_selected(int p_index) {
	if (p_index < 0 || !atlas_selector) {
		return;
	}
	int sid = atlas_selector->get_item_id(p_index);
	if (sid < 0 || current_tileset.is_null()) {
		_on_load_tileset_pressed();
		return;
	}
	if (current_tileset->has_source(sid)) {
		current_source_id = sid;
		Ref<TileSetAtlasSource> atlas = current_tileset->get_source(sid);
		if (atlas.is_valid()) {
			current_atlas_texture = atlas->get_texture();
			current_atlas_path = current_atlas_texture.is_valid() ? current_atlas_texture->get_path() : "";
		}
		_sync_metadata_from_tileset();
		_update_scrollbars();
		if (canvas) {
			canvas->queue_redraw();
		}
		_update_footer_info();
	}
}

void ClockGearsTilePicker::_on_search_filter_changed(const String &p_text) {
	if (current_tileset.is_valid()) {
		set_tileset(current_tileset, current_source_id);
	}
}

void ClockGearsTilePicker::_on_toggle_favorite_pressed() {
	if (selected_tiles.is_empty()) {
		return;
	}
	for (int i = 0; i < selected_tiles.size(); i++) {
		String key = _make_fav_key(selected_tiles[i]);
		int idx = favorite_keys.find(key);
		if (idx >= 0) {
			favorite_keys.remove_at(idx);
		} else {
			favorite_keys.push_back(key);
		}
	}
	_save_favorites_to_settings();
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::_on_only_favorites_toggled(bool p_pressed) {
	only_favorites = p_pressed;
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::_on_load_tileset_pressed() {
	print_line(String::utf8("[ClockGears TilePicker] Botón 'Cargar TileSet' pulsado."));
	if (on_request_load_callback.is_valid()) {
		on_request_load_callback.call();
		return;
	}
	if (file_dialog) {
		String ts_dir = ClockGearsProjectStructure::resolve_project_folder_global("img/tilesets");
		if (!ts_dir.is_empty()) {
			file_dialog->set_current_dir(ts_dir);
		}
		file_dialog->popup_centered(Size2(680, 460));
	}
}

void ClockGearsTilePicker::_on_file_selected(const String &p_path) {
	if (p_path.is_empty()) {
		return;
	}
	Ref<Texture2D> tex;
	Ref<TileSet> ts;
	String res_path;
	String err_msg;
	if (!import_tileset_resource(p_path, tex, ts, res_path, err_msg, tile_size)) {
		if (error_dialog) {
			error_dialog->set_text(err_msg);
			error_dialog->popup_centered(Size2(440, 160));
		}
		return;
	}
	if (tex.is_valid()) {
		set_atlas_texture(tex, res_path);
	} else if (ts.is_valid()) {
		set_tileset(ts, 0);
	}
	if (on_atlas_loaded_callback.is_valid()) {
		on_atlas_loaded_callback.call(res_path);
	}
}

void ClockGearsTilePicker::_update_scrollbars() {
	if (!h_scroll || !v_scroll || !canvas) {
		return;
	}
	updating_scrollbars = true;
	if (current_atlas_texture.is_null()) {
		h_scroll->set_max(100);
		h_scroll->set_page(100);
		h_scroll->set_value(0);
		h_scroll->hide();
		v_scroll->set_max(100);
		v_scroll->set_page(100);
		v_scroll->set_value(0);
		v_scroll->hide();
		updating_scrollbars = false;
		return;
	}

	Size2 canvas_sz = canvas->get_size();
	if (canvas_sz.x < 10.0f) {
		canvas_sz.x = 220.0f;
	}
	if (canvas_sz.y < 10.0f) {
		canvas_sz.y = 180.0f;
	}
	Size2 content_sz = (current_atlas_texture->get_size() * zoom_level).round();

	float max_x = MAX(content_sz.x, canvas_sz.x);
	float max_y = MAX(content_sz.y, canvas_sz.y);

	h_scroll->set_min(0);
	h_scroll->set_max(max_x);
	h_scroll->set_page(canvas_sz.x);
	h_scroll->set_value(CLAMP(-pan_offset.x, 0.0f, MAX(0.0f, max_x - canvas_sz.x)));
	h_scroll->set_visible(content_sz.x > canvas_sz.x + 1.0f);

	v_scroll->set_min(0);
	v_scroll->set_max(max_y);
	v_scroll->set_page(canvas_sz.y);
	v_scroll->set_value(CLAMP(-pan_offset.y, 0.0f, MAX(0.0f, max_y - canvas_sz.y)));
	v_scroll->set_visible(content_sz.y > canvas_sz.y + 1.0f);

	updating_scrollbars = false;
}

void ClockGearsTilePicker::_on_scroll_changed(double p_val) {
	if (updating_scrollbars || !h_scroll || !v_scroll) {
		return;
	}
	pan_offset.x = -Math::round(h_scroll->get_value());
	pan_offset.y = -Math::round(v_scroll->get_value());
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::_on_zoom_out_pressed() {
	const float steps[] = { 0.25f, 0.50f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
	for (int i = 7; i >= 0; i--) {
		if (steps[i] < zoom_level - 0.01f) {
			set_zoom_level(steps[i]);
			return;
		}
	}
	set_zoom_level(0.25f);
}

void ClockGearsTilePicker::_on_zoom_in_pressed() {
	const float steps[] = { 0.25f, 0.50f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
	for (int i = 0; i < 8; i++) {
		if (steps[i] > zoom_level + 0.01f) {
			set_zoom_level(steps[i]);
			return;
		}
	}
	set_zoom_level(8.0f);
}

void ClockGearsTilePicker::_on_zoom_option_selected(int p_index) {
	const float steps[] = { 0.25f, 0.50f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
	if (p_index >= 0 && p_index < 8) {
		set_zoom_level(steps[p_index]);
	}
}

void ClockGearsTilePicker::_on_grid_toggled(bool p_pressed) {
	show_grid = p_pressed;
	if (canvas) {
		canvas->queue_redraw();
	}
}

void ClockGearsTilePicker::_on_edit_mode_selected(int p_mode) {
	set_edit_mode((EditMode)p_mode);
}

Vector2i ClockGearsTilePicker::_screen_to_tile(const Vector2 &p_local_pos, bool *r_inside) const {
	if (current_atlas_texture.is_null() || tile_size <= 0 || zoom_level <= 0.0f) {
		if (r_inside) {
			*r_inside = false;
		}
		return Vector2i(-1, -1);
	}
	Vector2 atlas_pos = (p_local_pos - pan_offset) / zoom_level;
	Size2 tex_sz = current_atlas_texture->get_size();
	int cols = MAX(1, (int)(tex_sz.x / tile_size));
	int rows = MAX(1, (int)(tex_sz.y / tile_size));
	int tx = Math::floor(atlas_pos.x / tile_size);
	int ty = Math::floor(atlas_pos.y / tile_size);
	bool inside = (tx >= 0 && ty >= 0 && tx < cols && ty < rows);
	if (r_inside) {
		*r_inside = inside;
	}
	return Vector2i(CLAMP(tx, 0, cols - 1), CLAMP(ty, 0, rows - 1));
}

void ClockGearsTilePicker::_apply_metadata_to_tile(const Vector2i &p_tile, const Dictionary &p_meta) {
	if (is_autotile_slot()) {
		Vector2i bsz = get_autotile_block_size();
		int bx = (p_tile.x / bsz.x) * bsz.x;
		int by = (p_tile.y / bsz.y) * bsz.y;
		for (int y = by; y < by + bsz.y; y++) {
			for (int x = bx; x < bx + bsz.x; x++) {
				set_tile_metadata(Vector2i(x, y), p_meta);
			}
		}
	} else {
		set_tile_metadata(p_tile, p_meta);
	}
}

void ClockGearsTilePicker::_handle_metadata_click(const Vector2i &p_tile, const Vector2 &p_local_pos_in_tile, MouseButton p_button) {
	Vector2i ref_tile = p_tile;
	if (is_autotile_slot()) {
		Vector2i bsz = get_autotile_block_size();
		ref_tile.x = (p_tile.x / bsz.x) * bsz.x;
		ref_tile.y = (p_tile.y / bsz.y) * bsz.y;
	}

	Dictionary meta = get_tile_metadata(ref_tile);
	int passage = (int)meta.get("passage", (int)PASS_ALL);
	int layer = (int)meta.get("layer", 0);
	int terrain = (int)meta.get("terrain_tag", 0);
	bool bush = (bool)meta.get("bush", false);
	bool counter = (bool)meta.get("counter", false);
	bool ladder = (bool)meta.get("ladder", false);
	bool damage_floor = (bool)meta.get("damage_floor", false);

	switch (current_edit_mode) {
		case MODE_PASSAGE_BASIC: {
			// Ciclo RPG Maker MZ: ○ (Pasa todo, capa 0) -> × (Bloqueado, capa 0) -> ★ (Pasa todo, capa 1 sobre personaje)
			if (layer == 1) {
				passage = PASS_ALL;
				layer = 0;
			} else if (passage == PASS_NONE) {
				passage = PASS_ALL;
				layer = 1;
			} else {
				passage = PASS_NONE;
				layer = 0;
			}
			meta["passage"] = passage;
			meta["layer"] = layer;
		} break;

		case MODE_PASSAGE_4DIR: {
			// Dividir el tile en 4 triángulos/cuadrantes (Arriba, Abajo, Izquierda, Derecha)
			Vector2 norm = p_local_pos_in_tile / float(tile_size); // 0..1
			float dx = norm.x - 0.5f;
			float dy = norm.y - 0.5f;
			int bit = PASS_DOWN;
			if (Math::abs(dx) > Math::abs(dy)) {
				bit = (dx < 0.0f) ? PASS_LEFT : PASS_RIGHT;
			} else {
				bit = (dy < 0.0f) ? PASS_UP : PASS_DOWN;
			}
			passage ^= bit;
			meta["passage"] = passage;
		} break;

		case MODE_TERRAIN_TAG: {
			// Clic izquierdo sube (0..7), clic derecho baja (7..0)
			if (p_button == MouseButton::RIGHT) {
				terrain = (terrain + 7) % 8;
			} else {
				terrain = (terrain + 1) % 8;
			}
			meta["terrain_tag"] = terrain;
		} break;

		case MODE_BUSH: {
			meta["bush"] = !bush;
		} break;

		case MODE_COUNTER: {
			meta["counter"] = !counter;
		} break;

		case MODE_LADDER: {
			meta["ladder"] = !ladder;
		} break;

		case MODE_DAMAGE_FLOOR: {
			meta["damage_floor"] = !damage_floor;
		} break;

		default:
			break;
	}

	_apply_metadata_to_tile(p_tile, meta);
}

void ClockGearsTilePicker::_gui_input_canvas(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Zoom con rueda del ratón (priorizando enteros 1x, 2x, 3x, 4x, 8x para Pixel Art A4.5)
		if (mb->is_pressed() && mb->get_button_index() == MouseButton::WHEEL_UP) {
			_on_zoom_in_pressed();
			canvas->accept_event();
			return;
		}
		if (mb->is_pressed() && mb->get_button_index() == MouseButton::WHEEL_DOWN) {
			_on_zoom_out_pressed();
			canvas->accept_event();
			return;
		}

		// Paneo con botón central del ratón (Sección 6.1)
		if (mb->get_button_index() == MouseButton::MIDDLE) {
			is_panning = mb->is_pressed();
			pan_last_mouse = mb->get_position();
			canvas->accept_event();
			return;
		}

		if (current_atlas_texture.is_null()) {
			return;
		}

		if (mb->get_button_index() == MouseButton::LEFT || mb->get_button_index() == MouseButton::RIGHT) {
			if (!mb->is_pressed()) {
				is_dragging_metadata = false;
				last_drag_metadata_tile = Vector2i(-1, -1);
			}

			bool inside = false;
			Vector2i tile = _screen_to_tile(mb->get_position(), &inside);
			if (!inside) {
				if (!mb->is_pressed()) {
					is_dragging_selection = false;
				}
				return;
			}

			if (current_edit_mode != MODE_SELECT && metadata_editing_enabled) {
				if (mb->is_pressed()) {
					is_dragging_metadata = true;
					drag_metadata_button = mb->get_button_index();
					Vector2 atlas_pos = (mb->get_position() - pan_offset) / zoom_level;
					Vector2 local_in_tile = atlas_pos - Vector2(tile.x * tile_size, tile.y * tile_size);
					_handle_metadata_click(tile, local_in_tile, mb->get_button_index());
					Vector2i ref_tile = tile;
					if (is_autotile_slot()) {
						Vector2i bsz = get_autotile_block_size();
						ref_tile.x = (tile.x / bsz.x) * bsz.x;
						ref_tile.y = (tile.y / bsz.y) * bsz.y;
					}
					drag_target_metadata = get_tile_metadata(ref_tile);
					last_drag_metadata_tile = tile;
					canvas->accept_event();
				}
				return;
			}

			// Modo Selección (Simple, Rectangular por arrastre, o Ctrl+clic para añadir/quitar)
			if (mb->get_button_index() == MouseButton::LEFT) {
				if (mb->is_pressed()) {
					if (mb->is_ctrl_pressed()) {
						int idx = selected_tiles.find(tile);
						if (idx >= 0 && selected_tiles.size() > 1) {
							selected_tiles.remove_at(idx);
						} else if (idx < 0) {
							selected_tiles.push_back(tile);
						}
						selection_block_size = Vector2i(1, 1);
						is_dragging_selection = false;
					} else {
						is_dragging_selection = true;
						drag_start_tile = tile;
						selected_tiles.clear();
						selected_tiles.push_back(tile);
						selection_block_size = Vector2i(1, 1);
					}
					_emit_selection_changed();
					_update_footer_info();
					canvas->queue_redraw();
					canvas->accept_event();
				} else {
					if (is_dragging_selection) {
						is_dragging_selection = false;
						_emit_selection_changed();
						canvas->queue_redraw();
						canvas->accept_event();
					}
				}
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (is_panning) {
			Vector2 delta = mm->get_position() - pan_last_mouse;
			pan_last_mouse = mm->get_position();
			pan_offset += delta;
			// Mantener paneo en coordenadas enteras para nitidez pixel-perfect (A4.5)
			pan_offset = pan_offset.round();
			_update_scrollbars();
			canvas->queue_redraw();
			canvas->accept_event();
			return;
		}

		bool inside = false;
		Vector2i tile = _screen_to_tile(mm->get_position(), &inside);
		Vector2i new_hover = inside ? tile : Vector2i(-1, -1);
		if (new_hover != hovered_tile) {
			hovered_tile = new_hover;
			_update_footer_info();
			canvas->queue_redraw();
		}

		if (is_dragging_metadata && current_atlas_texture.is_valid()) {
			if (inside && tile != last_drag_metadata_tile) {
				last_drag_metadata_tile = tile;
				_apply_metadata_to_tile(tile, drag_target_metadata);
				_update_footer_info();
				canvas->queue_redraw();
			}
			return;
		}

		if (is_dragging_selection && current_atlas_texture.is_valid()) {
			int x0 = MIN(drag_start_tile.x, tile.x);
			int y0 = MIN(drag_start_tile.y, tile.y);
			int x1 = MAX(drag_start_tile.x, tile.x);
			int y1 = MAX(drag_start_tile.y, tile.y);
			selected_tiles.clear();
			for (int ty = y0; ty <= y1; ty++) {
				for (int tx = x0; tx <= x1; tx++) {
					selected_tiles.push_back(Vector2i(tx, ty));
				}
			}
			selection_block_size = Vector2i(x1 - x0 + 1, y1 - y0 + 1);
			_emit_selection_changed();
			_update_footer_info();
			canvas->queue_redraw();
		}
	}
}

void ClockGearsTilePicker::_emit_selection_changed() {
	emit_signal("tiles_selected", get_selected_tiles_array(), selection_block_size);
}

void ClockGearsTilePicker::_update_footer_info() {
	if (!coords_label || !metadata_label) {
		return;
	}
	Vector2i ref_tile = (hovered_tile.x >= 0 && hovered_tile.y >= 0) ? hovered_tile : get_primary_selected_tile();
	if (current_atlas_texture.is_null() || ref_tile.x < 0 || ref_tile.y < 0) {
		coords_label->set_text(CG_TP_TR("Tile: (-, -)"));
		metadata_label->hide();
		return;
	}

	int cols = MAX(1, (int)(current_atlas_texture->get_size().x / tile_size));
	int tile_idx = ref_tile.y * cols + ref_tile.x;
	if (selection_block_size.x > 1 || selection_block_size.y > 1) {
		coords_label->set_text(vformat(CG_TP_TR("Tile (%d, %d) #%03d | Bloque %dx%d"), ref_tile.x, ref_tile.y, tile_idx, selection_block_size.x, selection_block_size.y));
	} else {
		coords_label->set_text(vformat(CG_TP_TR("Tile (%d, %d) #%03d"), ref_tile.x, ref_tile.y, tile_idx));
	}

	if (!metadata_editing_enabled) {
		metadata_label->hide();
		return;
	}

	Dictionary meta = get_tile_metadata(ref_tile);
	int pass = (int)meta.get("passage", (int)PASS_ALL);
	int layer = (int)meta.get("layer", 0);
	int terr = (int)meta.get("terrain_tag", 0);
	bool bush = (bool)meta.get("bush", false);
	bool counter = (bool)meta.get("counter", false);
	bool ladder = (bool)meta.get("ladder", false);
	bool damage_floor = (bool)meta.get("damage_floor", false);

	String pass_str;
	if (layer == 1) {
		pass_str = CG_TP_TR("★ Sobre PJ");
	} else if (pass == PASS_ALL) {
		pass_str = CG_TP_TR("○ Pasa");
	} else if (pass == PASS_NONE) {
		pass_str = CG_TP_TR("× Bloqueado");
	} else {
		pass_str = vformat("%s%s%s%s",
				(pass & PASS_DOWN) ? CG_TP_TR("↓") : CG_TP_TR("·"),
				(pass & PASS_LEFT) ? CG_TP_TR("←") : CG_TP_TR("·"),
				(pass & PASS_RIGHT) ? CG_TP_TR("→") : CG_TP_TR("·"),
				(pass & PASS_UP) ? CG_TP_TR("↑") : CG_TP_TR("·"));
	}

	PackedStringArray parts;
	parts.push_back(pass_str);
	if (terr > 0 || current_edit_mode == MODE_TERRAIN_TAG) {
		parts.push_back(vformat(CG_TP_TR("Terreno: %d"), terr));
	}
	if (bush || current_edit_mode == MODE_BUSH) {
		parts.push_back(bush ? CG_TP_TR("Arbusto: ~") : CG_TP_TR("Arbusto: No"));
	}
	if (counter || current_edit_mode == MODE_COUNTER) {
		parts.push_back(counter ? CG_TP_TR("Mostrador: ♦") : CG_TP_TR("Mostrador: No"));
	}
	if (ladder || current_edit_mode == MODE_LADDER) {
		parts.push_back(ladder ? CG_TP_TR("Escalera: H") : CG_TP_TR("Escalera: No"));
	}
	if (damage_floor || current_edit_mode == MODE_DAMAGE_FLOOR) {
		parts.push_back(damage_floor ? CG_TP_TR("Daño: ▲") : CG_TP_TR("Daño: No"));
	}

	metadata_label->set_text(String(" | ").join(parts));
	metadata_label->show();
}

void ClockGearsTilePicker::_draw_canvas() {
	if (!canvas) {
		return;
	}
	Size2 sz = canvas->get_size();
	canvas->draw_rect(Rect2(Point2(), sz), Color(0.09f, 0.10f, 0.12f, 1.0f), true);

	// Fondo damero oscuro sutil
	const int chk = 16;
	int c_cols = Math::ceil(sz.x / chk);
	int c_rows = Math::ceil(sz.y / chk);
	for (int ry = 0; ry < c_rows; ry++) {
		for (int rx = 0; rx < c_cols; rx++) {
			if ((rx + ry) % 2 == 1) {
				canvas->draw_rect(Rect2(rx * chk, ry * chk, chk, chk), Color(0.13f, 0.14f, 0.17f, 1.0f), true);
			}
		}
	}

	Ref<Font> font = get_theme_default_font();
	int font_size = get_theme_default_font_size();

	// Estado vacío cuando no hay TileSet cargado (Sección 6.1 & Regla Cero)
	if (current_atlas_texture.is_null()) {
		if (font.is_valid()) {
			float max_w = MAX(100.0f, sz.x - 24.0f);
			canvas->draw_multiline_string(
					font,
					Point2(12, 32),
					CG_TP_TR("Sin hoja cargada en esta ranura (Regla Cero).\nPulsa 'Cargar...' para importar un PNG (32×32 px)\na res://img/tilesets/."),
					HORIZONTAL_ALIGNMENT_LEFT,
					max_w,
					font_size,
					-1,
					Color(0.68f, 0.72f, 0.80f));
		}
		return;
	}

	_update_scrollbars();

	Size2 tex_sz = current_atlas_texture->get_size();
	Rect2 dst_rect(pan_offset.round(), (tex_sz * zoom_level).round());
	canvas->draw_texture_rect(current_atlas_texture, dst_rect, false);

	int cols = MAX(1, (int)(tex_sz.x / tile_size));
	int rows = MAX(1, (int)(tex_sz.y / tile_size));
	float scaled_tile = tile_size * zoom_level;

	// A4.5: Cuadrícula de píxeles individuales visible en zoom alto (>= 400%)
	if (show_grid && zoom_level >= 4.0f) {
		Color px_grid_col(1.0f, 1.0f, 1.0f, 0.10f);
		int total_px_x = cols * tile_size;
		int total_px_y = rows * tile_size;
		for (int px = 1; px < total_px_x; px++) {
			float x = Math::round(pan_offset.x + px * zoom_level);
			if (x >= 0 && x <= sz.x) {
				canvas->draw_line(Point2(x, MAX(0.0f, pan_offset.y)), Point2(x, MIN(sz.y, pan_offset.y + dst_rect.size.y)), px_grid_col, 1.0f);
			}
		}
		for (int py = 1; py < total_px_y; py++) {
			float y = Math::round(pan_offset.y + py * zoom_level);
			if (y >= 0 && y <= sz.y) {
				canvas->draw_line(Point2(MAX(0.0f, pan_offset.x), y), Point2(MIN(sz.x, pan_offset.x + dst_rect.size.x), y), px_grid_col, 1.0f);
			}
		}
	}

	// Cuadrícula de tiles 32x32 + Favoritos + Overlay de Pasabilidad/Metadatos
	for (int ty = 0; ty < rows; ty++) {
		for (int tx = 0; tx < cols; tx++) {
			Vector2i coord(tx, ty);
			Rect2 cell_r(
					Math::round(pan_offset.x + tx * scaled_tile),
					Math::round(pan_offset.y + ty * scaled_tile),
					Math::round(scaled_tile),
					Math::round(scaled_tile));

			if (cell_r.position.x + cell_r.size.x < 0 || cell_r.position.y + cell_r.size.y < 0 ||
					cell_r.position.x > sz.x || cell_r.position.y > sz.y) {
				continue;
			}

			bool is_fav = _is_tile_favorite(coord);
			if (only_favorites && !is_fav) {
				canvas->draw_rect(cell_r, Color(0.05f, 0.06f, 0.08f, 0.78f), true);
			}

			if (show_grid) {
				canvas->draw_rect(cell_r, Color(0.0f, 0.0f, 0.0f, 0.45f), false, 1.0f);
			}

			if (is_fav && font.is_valid()) {
				canvas->draw_string(font, cell_r.position + Point2(2, 11), CG_TP_TR("★"), HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(1.0f, 0.84f, 0.20f, 0.95f));
			}

			// Dibujar superposición de metadatos cuando se está editando pasabilidad/terreno/arbusto/mostrador/escalera/daño
			if (metadata_editing_enabled && current_edit_mode != MODE_SELECT) {
				bool is_auto = is_autotile_slot();
				Vector2i bsz = get_autotile_block_size();
				bool is_anchor = (!is_auto) || (tx % bsz.x == 0 && ty % bsz.y == 0);

				if (is_anchor) {
					Dictionary meta = get_tile_metadata(coord);
					int passage = (int)meta.get("passage", (int)PASS_ALL);
					int layer = (int)meta.get("layer", 0);
					int terrain = (int)meta.get("terrain_tag", 0);
					bool bush = (bool)meta.get("bush", false);
					bool counter = (bool)meta.get("counter", false);
					bool ladder = (bool)meta.get("ladder", false);
					bool damage_floor = (bool)meta.get("damage_floor", false);

					Point2 center;
					float badge_r;
					if (is_auto) {
						Rect2 block_r(cell_r.position, Size2(bsz.x * scaled_tile, bsz.y * scaled_tile));
						canvas->draw_rect(block_r, Color(1.0f, 0.82f, 0.22f, 0.75f), false, 2.0f);
						center = block_r.position + block_r.size * 0.5f;
						badge_r = CLAMP(scaled_tile * 0.40f, 9.0f, 20.0f);
					} else {
						center = cell_r.position + cell_r.size * 0.5f;
						badge_r = CLAMP(scaled_tile * 0.30f, 7.0f, 14.0f);
					}
					canvas->draw_circle(center, badge_r, Color(0.04f, 0.05f, 0.07f, 0.76f));

				if (font.is_valid()) {
					if (current_edit_mode == MODE_PASSAGE_BASIC) {
						String sym = "O";
						Color col(0.35f, 0.95f, 0.45f, 0.98f);
						if (layer == 1) {
							sym = CG_TP_TR("★");
							col = Color(1.0f, 0.88f, 0.25f, 0.98f);
						} else if (passage == PASS_NONE) {
							sym = "X";
							col = Color(1.0f, 0.32f, 0.32f, 0.98f);
						}
						canvas->draw_string(font, center + Point2(-5, 5), sym, HORIZONTAL_ALIGNMENT_CENTER, -1, 13, col);
					} else if (current_edit_mode == MODE_PASSAGE_4DIR) {
						float off = scaled_tile * 0.28f;
						canvas->draw_string(font, center + Point2(-4, -off + 4), (passage & PASS_UP) ? CG_TP_TR("↑") : CG_TP_TR("·"), HORIZONTAL_ALIGNMENT_CENTER, -1, 11, (passage & PASS_UP) ? Color(0.35f, 0.95f, 0.45f) : Color(1.0f, 0.32f, 0.32f));
						canvas->draw_string(font, center + Point2(-4, off + 4), (passage & PASS_DOWN) ? CG_TP_TR("↓") : CG_TP_TR("·"), HORIZONTAL_ALIGNMENT_CENTER, -1, 11, (passage & PASS_DOWN) ? Color(0.35f, 0.95f, 0.45f) : Color(1.0f, 0.32f, 0.32f));
						canvas->draw_string(font, center + Point2(-off - 4, 4), (passage & PASS_LEFT) ? CG_TP_TR("←") : CG_TP_TR("·"), HORIZONTAL_ALIGNMENT_CENTER, -1, 11, (passage & PASS_LEFT) ? Color(0.35f, 0.95f, 0.45f) : Color(1.0f, 0.32f, 0.32f));
						canvas->draw_string(font, center + Point2(off - 4, 4), (passage & PASS_RIGHT) ? CG_TP_TR("→") : CG_TP_TR("·"), HORIZONTAL_ALIGNMENT_CENTER, -1, 11, (passage & PASS_RIGHT) ? Color(0.35f, 0.95f, 0.45f) : Color(1.0f, 0.32f, 0.32f));
					} else if (current_edit_mode == MODE_TERRAIN_TAG) {
						Color col = (terrain > 0) ? Color(0.35f, 0.85f, 1.0f, 0.98f) : Color(0.60f, 0.64f, 0.70f, 0.85f);
						canvas->draw_string(font, center + Point2(-4, 5), String::num_int64(terrain), HORIZONTAL_ALIGNMENT_CENTER, -1, 13, col);
					} else if (current_edit_mode == MODE_BUSH) {
						String sym = bush ? "~" : CG_TP_TR("·");
						Color col = bush ? Color(0.40f, 0.95f, 0.50f, 0.98f) : Color(0.55f, 0.58f, 0.64f, 0.75f);
						canvas->draw_string(font, center + Point2(-4, 5), sym, HORIZONTAL_ALIGNMENT_CENTER, -1, 14, col);
					} else if (current_edit_mode == MODE_COUNTER) {
						String sym = counter ? CG_TP_TR("♦") : CG_TP_TR("·");
						Color col = counter ? Color(1.0f, 0.72f, 0.25f, 0.98f) : Color(0.55f, 0.58f, 0.64f, 0.75f);
						canvas->draw_string(font, center + Point2(-4, 5), sym, HORIZONTAL_ALIGNMENT_CENTER, -1, 13, col);
					} else if (current_edit_mode == MODE_LADDER) {
						String sym = ladder ? "H" : CG_TP_TR("·");
						Color col = ladder ? Color(0.95f, 0.82f, 0.35f, 0.98f) : Color(0.55f, 0.58f, 0.64f, 0.75f);
						canvas->draw_string(font, center + Point2(-4, 5), sym, HORIZONTAL_ALIGNMENT_CENTER, -1, 13, col);
					} else if (current_edit_mode == MODE_DAMAGE_FLOOR) {
						String sym = damage_floor ? CG_TP_TR("▲") : CG_TP_TR("·");
						Color col = damage_floor ? Color(1.0f, 0.35f, 0.45f, 0.98f) : Color(0.55f, 0.58f, 0.64f, 0.75f);
						canvas->draw_string(font, center + Point2(-4, 5), sym, HORIZONTAL_ALIGNMENT_CENTER, -1, 13, col);
					}
				}
				}
			}
		}
	}

	// Resaltado de selección actual (simple, rectangular o multi-tile Ctrl+clic)
	for (int i = 0; i < selected_tiles.size(); i++) {
		Vector2i c = selected_tiles[i];
		Rect2 sel_r(
				Math::round(pan_offset.x + c.x * scaled_tile),
				Math::round(pan_offset.y + c.y * scaled_tile),
				Math::round(scaled_tile),
				Math::round(scaled_tile));
		canvas->draw_rect(sel_r, Color(0.24f, 0.56f, 0.96f, 0.22f), true);
		canvas->draw_rect(sel_r, Color(1.0f, 0.86f, 0.22f, 0.95f), false, 2.0f);
	}

	// Resaltado bajo el cursor (hover)
	if (hovered_tile.x >= 0 && hovered_tile.y >= 0) {
		Rect2 hov_r(
				Math::round(pan_offset.x + hovered_tile.x * scaled_tile),
				Math::round(pan_offset.y + hovered_tile.y * scaled_tile),
				Math::round(scaled_tile),
				Math::round(scaled_tile));
		canvas->draw_rect(hov_r, Color(0.45f, 0.85f, 1.0f, 0.85f), false, 1.0f);
	}
}

ClockGearsTilePicker::ClockGearsTilePicker() {
	set_h_size_flags(SIZE_EXPAND_FILL);
	set_v_size_flags(SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 4);

	_load_favorites_from_settings();
	selected_tiles.push_back(Vector2i(0, 0));

	// --- FILA SUPERIOR COMPACTA (UNA SOLA FILA): Selector + Cargar + Filtro + Fav + Zoom + Cuadrícula ---
	top_row_1 = memnew(HBoxContainer);
	top_row_1->set_h_size_flags(SIZE_EXPAND_FILL);
	top_row_1->add_theme_constant_override("separation", 1);
	add_child(top_row_1);

	atlas_selector = memnew(OptionButton);
	atlas_selector->set_h_size_flags(SIZE_EXPAND_FILL);
	atlas_selector->set_clip_text(true);
	atlas_selector->set_fit_to_longest_item(false);
	atlas_selector->set_custom_minimum_size(Size2(36 * EDSCALE, 20 * EDSCALE));
	atlas_selector->add_item(CG_TP_TR("(Sin TileSet cargado)"), -1);
	atlas_selector->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_atlas_pressed));
	atlas_selector->connect("item_selected", callable_mp(this, &ClockGearsTilePicker::_on_atlas_selected));
	top_row_1->add_child(atlas_selector);

	btn_load_tileset = memnew(Button);
	btn_load_tileset->set_text(CG_TP_TR("📂"));
	btn_load_tileset->set_clip_text(true);
	btn_load_tileset->set_tooltip_text(CG_TP_TR("Cargar hoja de tileset 32x32 (.png/.webp) o recurso TileSet (.tres)"));
	btn_load_tileset->set_custom_minimum_size(Size2(18 * EDSCALE, 20 * EDSCALE));
	btn_load_tileset->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_load_tileset_pressed));
	top_row_1->add_child(btn_load_tileset);

	search_filter_edit = memnew(LineEdit);
	search_filter_edit->set_placeholder(CG_TP_TR("Filtrar..."));
	search_filter_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	search_filter_edit->set_expand_to_text_length_enabled(false);
	search_filter_edit->add_theme_constant_override("minimum_character_width", 0);
	search_filter_edit->set_custom_minimum_size(Size2(28 * EDSCALE, 20 * EDSCALE));
	search_filter_edit->connect("text_changed", callable_mp(this, &ClockGearsTilePicker::_on_search_filter_changed));
	top_row_1->add_child(search_filter_edit);

	btn_fav_toggle = memnew(Button(CG_TP_TR("★")));
	btn_fav_toggle->set_clip_text(true);
	btn_fav_toggle->set_tooltip_text(CG_TP_TR("Marcar/Desmarcar tiles seleccionados como Favoritos"));
	btn_fav_toggle->set_custom_minimum_size(Size2(14 * EDSCALE, 20 * EDSCALE));
	btn_fav_toggle->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_toggle_favorite_pressed));
	top_row_1->add_child(btn_fav_toggle);

	btn_only_favorites = memnew(Button(CG_TP_TR("F")));
	btn_only_favorites->set_toggle_mode(true);
	btn_only_favorites->set_clip_text(true);
	btn_only_favorites->set_custom_minimum_size(Size2(14 * EDSCALE, 20 * EDSCALE));
	btn_only_favorites->set_tooltip_text(CG_TP_TR("Mostrar solo tiles Favoritos"));
	btn_only_favorites->connect("toggled", callable_mp(this, &ClockGearsTilePicker::_on_only_favorites_toggled));
	top_row_1->add_child(btn_only_favorites);

	btn_zoom_out = memnew(Button("-"));
	btn_zoom_out->set_clip_text(true);
	btn_zoom_out->set_tooltip_text(CG_TP_TR("Alejar zoom (Rueda abajo)"));
	btn_zoom_out->set_custom_minimum_size(Size2(12 * EDSCALE, 20 * EDSCALE));
	btn_zoom_out->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_zoom_out_pressed));
	top_row_1->add_child(btn_zoom_out);

	zoom_selector = memnew(OptionButton);
	zoom_selector->set_clip_text(true);
	zoom_selector->set_fit_to_longest_item(false);
	zoom_selector->set_custom_minimum_size(Size2(28 * EDSCALE, 20 * EDSCALE));
	zoom_selector->add_item("25%", 0);
	zoom_selector->add_item("50%", 1);
	zoom_selector->add_item("100%", 2);
	zoom_selector->add_item("200%", 3);
	zoom_selector->add_item("300%", 4);
	zoom_selector->add_item("400%", 5);
	zoom_selector->add_item("600%", 6);
	zoom_selector->add_item("800%", 7);
	zoom_selector->select(2);
	zoom_selector->connect("item_selected", callable_mp(this, &ClockGearsTilePicker::_on_zoom_option_selected));
	top_row_1->add_child(zoom_selector);

	btn_zoom_in = memnew(Button("+"));
	btn_zoom_in->set_clip_text(true);
	btn_zoom_in->set_tooltip_text(CG_TP_TR("Acercar zoom (Rueda arriba)"));
	btn_zoom_in->set_custom_minimum_size(Size2(12 * EDSCALE, 20 * EDSCALE));
	btn_zoom_in->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_zoom_in_pressed));
	top_row_1->add_child(btn_zoom_in);

	btn_grid_toggle = memnew(Button("#"));
	btn_grid_toggle->set_toggle_mode(true);
	btn_grid_toggle->set_pressed(true);
	btn_grid_toggle->set_clip_text(true);
	btn_grid_toggle->set_tooltip_text(CG_TP_TR("Mostrar/Ocultar cuadrícula de tiles"));
	btn_grid_toggle->set_custom_minimum_size(Size2(12 * EDSCALE, 20 * EDSCALE));
	btn_grid_toggle->connect("toggled", callable_mp(this, &ClockGearsTilePicker::_on_grid_toggled));
	top_row_1->add_child(btn_grid_toggle);

	// --- FILA ÚNICA DE BOTONES DE MODO DE COLISIÓN (Sel, O/X/★, 4-Dir, Terr, Arb, Most, Esc, Daño) ---
	mode_buttons_box = memnew(HBoxContainer);
	mode_buttons_box->set_h_size_flags(SIZE_EXPAND_FILL);
	mode_buttons_box->add_theme_constant_override("separation", 1);
	add_child(mode_buttons_box);

	const char *mode_labels[8] = {
		"Sel", "O/X/★", "4-Dir", "Terr",
		"Arb", "Most", "Esc", "Daño"
	};
	const char *mode_tips[8] = {
		"Modo Selección (Clic simple, arrastre rectangular o Ctrl+clic)",
		"Pasabilidad Básica: ○ (Pasa) / × (Bloqueado) / ★ (Sobre el personaje)",
		"Pasabilidad 4 Direcciones (↓ ← → ↑)",
		"Etiqueta de Terreno (0 a 7)",
		"Arbusto (~): Semi-transparencia inferior del personaje",
		"Mostrador (♦): Permite interactuar con eventos a 1 celda de distancia",
		"Escalera (H): El personaje mira hacia arriba al subir/bajar",
		"Daño de Suelo (▲): Inflige daño de terreno al caminar sobre este tile"
	};

	for (int i = 0; i < 8; i++) {
		Button *mb = memnew(Button(CG_TP_TR(mode_labels[i])));
		mb->set_toggle_mode(true);
		mb->set_pressed(i == 0);
		mb->set_h_size_flags(SIZE_EXPAND_FILL);
		mb->set_clip_text(true);
		mb->set_custom_minimum_size(Size2(18 * EDSCALE, 20 * EDSCALE));
		mb->set_tooltip_text(CG_TP_TR(mode_tips[i]));
		mb->connect("pressed", callable_mp(this, &ClockGearsTilePicker::_on_edit_mode_selected).bind(i));
		mode_buttons_box->add_child(mb);
		mode_buttons.push_back(mb);
	}

	// --- LIENZO CENTRAL CON BARRAS DE DESPLAZAMIENTO (HScrollBar y VScrollBar) ---
	PanelContainer *canvas_frame = memnew(PanelContainer);
	canvas_frame->set_h_size_flags(SIZE_EXPAND_FILL);
	canvas_frame->set_v_size_flags(SIZE_EXPAND_FILL);
	canvas_frame->set_custom_minimum_size(Size2(180 * EDSCALE, 180 * EDSCALE));
	Ref<StyleBoxFlat> cf_style = memnew(StyleBoxFlat);
	cf_style->set_anti_aliased(false);
	cf_style->set_bg_color(Color(0.08f, 0.09f, 0.11f, 1.0f));
	cf_style->set_border_width_all(1);
	cf_style->set_border_color(Color(0.24f, 0.27f, 0.33f, 1.0f));
	canvas_frame->add_theme_style_override("panel", cf_style);
	add_child(canvas_frame);

	VBoxContainer *canvas_vbox = memnew(VBoxContainer);
	canvas_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	canvas_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	canvas_vbox->set_custom_minimum_size(Size2(160 * EDSCALE, 160 * EDSCALE));
	canvas_vbox->add_theme_constant_override("separation", 0);
	canvas_frame->add_child(canvas_vbox);

	HBoxContainer *canvas_hbox = memnew(HBoxContainer);
	canvas_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	canvas_hbox->set_v_size_flags(SIZE_EXPAND_FILL);
	canvas_hbox->set_custom_minimum_size(Size2(160 * EDSCALE, 160 * EDSCALE));
	canvas_hbox->add_theme_constant_override("separation", 0);
	canvas_vbox->add_child(canvas_hbox);

	canvas = memnew(ClockGearsTilePickerCanvas(this));
	canvas->set_h_size_flags(SIZE_EXPAND_FILL);
	canvas->set_v_size_flags(SIZE_EXPAND_FILL);
	canvas->set_custom_minimum_size(Size2(160 * EDSCALE, 160 * EDSCALE));
	canvas->connect("resized", callable_mp(this, &ClockGearsTilePicker::_update_scrollbars));
	canvas_hbox->add_child(canvas);

	v_scroll = memnew(VScrollBar);
	v_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	v_scroll->connect("value_changed", callable_mp(this, &ClockGearsTilePicker::_on_scroll_changed));
	v_scroll->hide();
	canvas_hbox->add_child(v_scroll);

	h_scroll = memnew(HScrollBar);
	h_scroll->set_h_size_flags(SIZE_EXPAND_FILL);
	h_scroll->connect("value_changed", callable_mp(this, &ClockGearsTilePicker::_on_scroll_changed));
	h_scroll->hide();
	canvas_vbox->add_child(h_scroll);

	// --- BARRA DE ESTADO INFERIOR ---
	footer_bar = memnew(HBoxContainer);
	footer_bar->set_h_size_flags(SIZE_EXPAND_FILL);
	footer_bar->add_theme_constant_override("separation", 6);
	add_child(footer_bar);

	coords_label = memnew(Label(CG_TP_TR("Tile: (-, -)")));
	coords_label->add_theme_color_override("font_color", Color(0.75f, 0.80f, 0.88f));
	footer_bar->add_child(coords_label);

	metadata_label = memnew(Label);
	metadata_label->set_h_size_flags(SIZE_EXPAND_FILL);
	metadata_label->set_clip_text(true);
	metadata_label->add_theme_color_override("font_color", Color(0.55f, 0.85f, 1.0f));
	footer_bar->add_child(metadata_label);

	file_dialog = memnew(FileDialog);
	file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog->clear_filters();
	file_dialog->add_filter("*.png, *.webp, *.tres, *.res ; Hojas y Recursos de TileSet (32x32)");
	file_dialog->set_title(CG_TP_TR("Cargar Hoja o Recurso de TileSet (32x32 px)"));
	file_dialog->connect("file_selected", callable_mp(this, &ClockGearsTilePicker::_on_file_selected));
	add_child(file_dialog);

	error_dialog = memnew(AcceptDialog);
	error_dialog->set_title(CG_TP_TR("Error al importar TileSet - ClockGears Engine"));
	add_child(error_dialog);

	_update_footer_info();
}

// ============================================================================
// ClockGearsTilePickerDialog
// ============================================================================

void ClockGearsTilePickerDialog::_bind_methods() {
	ADD_SIGNAL(MethodInfo("tiles_picked", PropertyInfo(Variant::ARRAY, "selection"), PropertyInfo(Variant::VECTOR2I, "block_size")));
}

void ClockGearsTilePickerDialog::_on_dialog_confirmed() {
	if (!picker) {
		return;
	}
	Array sel = picker->get_selected_tiles_array();
	Vector2i bsz = picker->get_selection_block_size();
	emit_signal("tiles_picked", sel, bsz);
	if (on_confirmed_callback.is_valid()) {
		on_confirmed_callback.call(sel, bsz);
	}
}

void ClockGearsTilePickerDialog::open_picker(const Ref<Texture2D> &p_tex, const Callable &p_on_confirmed) {
	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme::get_singleton()->apply_to_window(this);
	}
	on_confirmed_callback = p_on_confirmed;
	if (picker && p_tex.is_valid()) {
		picker->set_atlas_texture(p_tex);
	}
	popup_centered(Size2(720, 540) * EDSCALE);
}

ClockGearsTilePickerDialog::ClockGearsTilePickerDialog() {
	set_title(CG_TP_TR("Selector de Tiles (32x32) - ClockGears Engine"));
	set_flag(Window::FLAG_RESIZE_DISABLED, false);
	set_min_size(Size2(540, 420) * EDSCALE);

	picker = memnew(ClockGearsTilePicker);
	picker->set_metadata_editing_enabled(false);
	add_child(picker);

	connect("confirmed", callable_mp(this, &ClockGearsTilePickerDialog::_on_dialog_confirmed));
}
