#include "clockgears_icons.h"
#include "core/clockgears_data_manager.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"

ClockGearsIcons *ClockGearsIcons::singleton = nullptr;

void ClockGearsIcons::_register_builtin_svgs() {
	svg_sources.clear();

	// Engranaje oficial ClockGears (#e0a040)
	svg_sources["gear"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 24 24\">"
			"<path fill=\"#e0a040\" d=\"M10 2h4l.8 3.2 2 .8 2.6-2 2.8 2.8-2 2.6.8 2L24 12.2v3.6l-3 .8-.8 2 2 2.6-2.8 2.8-2.6-2-2 .8L14 26h-4l-.8-3.2-2-.8-2.6 2-2.8-2.8 2-2.6-.8-2L0 15.8v-3.6l3-.8.8-2-2-2.6L4.6 4l2.6 2 2-.8z\" transform=\"scale(0.85) translate(2,0)\"/>"
			"<circle cx=\"12\" cy=\"12\" r=\"4\" fill=\"#1b1d23\"/>"
			"</svg>";

	// Acciones generales de UI
	svg_sources["add"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#4cc38a\" d=\"M7 2h2v5h5v2H9v5H7V9H2V7h5z\"/>"
			"</svg>";

	svg_sources["remove"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e5534b\" d=\"M3 7h10v2H3z\"/>"
			"</svg>";

	svg_sources["search"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<circle cx=\"6.5\" cy=\"6.5\" r=\"4.5\" fill=\"none\" stroke=\"#9aa0ab\" stroke-width=\"1.8\"/>"
			"<path stroke=\"#9aa0ab\" stroke-width=\"2\" stroke-linecap=\"round\" d=\"M10 10l4 4\"/>"
			"</svg>";

	svg_sources["copy"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<rect x=\"5\" y=\"5\" width=\"9\" height=\"9\" rx=\"1.5\" fill=\"none\" stroke=\"#e6e6e6\" stroke-width=\"1.5\"/>"
			"<path fill=\"none\" stroke=\"#9aa0ab\" stroke-width=\"1.5\" d=\"M3 11V3h8\"/>"
			"</svg>";

	svg_sources["paste"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<rect x=\"3\" y=\"3\" width=\"10\" height=\"11\" rx=\"1.5\" fill=\"none\" stroke=\"#e0a040\" stroke-width=\"1.5\"/>"
			"<rect x=\"6\" y=\"1.5\" width=\"4\" height=\"2.5\" rx=\"1\" fill=\"#e0a040\"/>"
			"</svg>";

	svg_sources["ok"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"none\" stroke=\"#4cc38a\" stroke-width=\"2.2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" d=\"M2.5 8.5l3.5 3.5 7.5-7.5\"/>"
			"</svg>";

	svg_sources["error"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<circle cx=\"8\" cy=\"8\" r=\"6.5\" fill=\"#e5534b\"/>"
			"<path stroke=\"#1b1d23\" stroke-width=\"2\" stroke-linecap=\"round\" d=\"M5.5 5.5l5 5M10.5 5.5l-5 5\"/>"
			"</svg>";

	// Herramientas de mapa
	svg_sources["pencil"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e0a040\" d=\"M2 14l1-3.5L11.5 2 14 4.5 5.5 13z\"/>"
			"</svg>";

	svg_sources["rectangle"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<rect x=\"2.5\" y=\"3.5\" width=\"11\" height=\"9\" fill=\"none\" stroke=\"#e0a040\" stroke-width=\"1.8\"/>"
			"</svg>";

	svg_sources["ellipse"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<ellipse cx=\"8\" cy=\"8\" rx=\"5.5\" ry=\"4.2\" fill=\"none\" stroke=\"#e0a040\" stroke-width=\"1.8\"/>"
			"</svg>";

	svg_sources["fill"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#4da6ff\" d=\"M3 9L8 3l5 6-5 5z\"/>"
			"</svg>";

	svg_sources["shadow"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#9aa0ab\" d=\"M10.5 2a6.2 6.2 0 1 0 3.5 11A5.2 5.2 0 0 1 10.5 2z\"/>"
			"</svg>";

	// Categorías de Comandos de Evento (Sección 11.2: coloreados por categoría)
	// Mensajes -> Azul (#4da6ff)
	svg_sources["cmd_message"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#4da6ff\" d=\"M2 3h12a1 1 0 0 1 1 1v7a1 1 0 0 1-1 1H6l-3 3v-3H2a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1z\"/>"
			"</svg>";

	// Control de Flujo / Progresión -> Ámbar (#e0a040)
	svg_sources["cmd_flow"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e0a040\" d=\"M8 1.5L14.5 8 8 14.5 1.5 8z\"/>"
			"</svg>";

	// Grupo / Inventario / Actor -> Verde (#4cc38a)
	svg_sources["cmd_inventory"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#4cc38a\" d=\"M5 2h6l2 4v7a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V6z\"/>"
			"</svg>";

	// Movimiento / Personaje / Seguidores -> Cian (#45c4d6)
	svg_sources["cmd_movement"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#45c4d6\" d=\"M8 1l3 3H9v3h3V5l3 3-3 3V9H9v3h2l-3 3-3-3h2V9H4v2L1 8l3-3v2h3V4H5z\"/>"
			"</svg>";

	// Escena / Batalla -> Carmesí (#e5534b)
	svg_sources["cmd_scene"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e5534b\" d=\"M2 13L13 2l1 1L3 14zM11 2h3v3zM2 11l3 3H2z\"/>"
			"</svg>";

	// Audio / Sistema / Pantalla -> Gris plata (#9aa0ab)
	svg_sources["cmd_system"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#9aa0ab\" d=\"M3 6h3l4-3v10l-4-3H3z\"/>"
			"<path fill=\"none\" stroke=\"#9aa0ab\" stroke-width=\"1.5\" d=\"M12 5.5a3.5 3.5 0 0 1 0 5\"/>"
			"</svg>";

	// Categorías de la Base de Datos
	svg_sources["db_actors"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<circle cx=\"8\" cy=\"5\" r=\"3\" fill=\"#4da6ff\"/>"
			"<path fill=\"#4da6ff\" d=\"M3 14c0-3 2.5-4.5 5-4.5s5 1.5 5 4.5z\"/>"
			"</svg>";

	svg_sources["db_classes"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e0a040\" d=\"M8 2l5 2.5v4c0 3.2-2.2 5.2-5 6.5-2.8-1.3-5-3.3-5-6.5v-4z\"/>"
			"</svg>";

	svg_sources["db_skills"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#45c4d6\" d=\"M9 1L3 9h4l-1 6 7-8H9z\"/>"
			"</svg>";

	svg_sources["db_items"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#4cc38a\" d=\"M6 2h4v2l3 4v5a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V8l3-4z\"/>"
			"</svg>";

	svg_sources["db_weapons"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e5534b\" d=\"M12.5 1.5l2 2-7.5 7.5-2-2zM3.5 10.5l2 2L3 15l-2-2z\"/>"
			"</svg>";

	svg_sources["db_armors"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#9aa0ab\" d=\"M4 2h8l2 3-2 2v6H4V7L2 5z\"/>"
			"</svg>";

	svg_sources["db_enemies"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#e5534b\" d=\"M3 3l2.5 2h5L13 3l-1 8a4 4 0 0 1-8 0z\"/>"
			"</svg>";

	svg_sources["db_flora"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<path fill=\"#5cd67a\" d=\"M8 2C5 5 4 9 8 14c4-5 3-9 0-12z\"/>"
			"</svg>";

	svg_sources["db_fauna"] =
			"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
			"<circle cx=\"5\" cy=\"5\" r=\"1.6\" fill=\"#e0a040\"/>"
			"<circle cx=\"11\" cy=\"5\" r=\"1.6\" fill=\"#e0a040\"/>"
			"<ellipse cx=\"8\" cy=\"10\" rx=\"3.8\" ry=\"3\" fill=\"#e0a040\"/>"
			"</svg>";
}

Ref<Texture2D> ClockGearsIcons::_get_fallback_icon() const {
	if (fallback_icon.is_valid()) {
		return fallback_icon;
	}
	// Textura de respaldo 16x16 garantizada no nula (sin depender de rasterizador SVG)
	Ref<Image> img = Image::create_empty(16, 16, false, Image::FORMAT_RGBA8);
	const Color bg(0x1b / 255.0f, 0x1d / 255.0f, 0x23 / 255.0f, 1.0f);
	const Color fg(0xe0 / 255.0f, 0xa0 / 255.0f, 0x40 / 255.0f, 1.0f);
	img->fill(bg);
	for (int i = 2; i < 14; i++) {
		img->set_pixel(i, 2, fg);
		img->set_pixel(i, 13, fg);
		img->set_pixel(2, i, fg);
		img->set_pixel(13, i, fg);
		img->set_pixel(i, i, fg);
		img->set_pixel(i, 15 - i, fg);
	}
	fallback_icon = ImageTexture::create_from_image(img);
	return fallback_icon;
}

Ref<Texture2D> ClockGearsIcons::_rasterize_svg(const String &p_svg, float p_scale) const {
	// A4: Escala entera para evitar interpolación borrosa en iconos de UI
	float snapped_scale = MAX(1.0f, Math::round(p_scale));
	Ref<Image> img;
	img.instantiate();
	if (img->load_svg_from_buffer(p_svg.to_utf8_buffer(), snapped_scale) != OK || img->is_empty()) {
		return Ref<Texture2D>();
	}
	return ImageTexture::create_from_image(img);
}

Ref<Texture2D> ClockGearsIcons::get_icon(const String &p_name, float p_scale) const {
	String cache_key = vformat("%s_%.2f", p_name, p_scale);
	if (icon_cache.has(cache_key) && icon_cache[cache_key].is_valid()) {
		return icon_cache[cache_key];
	}
	const String *svg_ptr = svg_sources.getptr(p_name);
	if (!svg_ptr) {
		WARN_PRINT(vformat("ClockGearsIcons: Icono '%s' no registrado; usando icono de respaldo 16x16.", p_name));
		return _get_fallback_icon();
	}
	Ref<Texture2D> tex = _rasterize_svg(*svg_ptr, p_scale);
	if (tex.is_null()) {
		WARN_PRINT(vformat("ClockGearsIcons: Fallo al rasterizar el SVG del icono '%s'; usando icono de respaldo 16x16.", p_name));
		tex = _get_fallback_icon();
	}
	if (tex.is_valid()) {
		icon_cache[cache_key] = tex;
	}
	return tex;
}

Ref<Texture2D> ClockGearsIcons::get_db_category_icon(int p_category, float p_scale) const {
	switch (p_category) {
		case CAT_ACTORS: return get_icon("db_actors", p_scale);
		case CAT_CLASSES: return get_icon("db_classes", p_scale);
		case CAT_SKILLS: return get_icon("db_skills", p_scale);
		case CAT_ITEMS: return get_icon("db_items", p_scale);
		case CAT_WEAPONS: return get_icon("db_weapons", p_scale);
		case CAT_ARMORS:
		case CAT_ACCESSORIES: return get_icon("db_armors", p_scale);
		case CAT_ENEMIES:
		case CAT_TROOPS: return get_icon("db_enemies", p_scale);
		case CAT_FLORA: return get_icon("db_flora", p_scale);
		case CAT_FAUNA: return get_icon("db_fauna", p_scale);
		case CAT_TILESETS: return get_icon("rectangle", p_scale);
		case CAT_COMMON_EVENTS:
		case CAT_SWITCHES_VARS: return get_icon("cmd_flow", p_scale);
		default: return get_icon("gear", p_scale);
	}
}

Ref<Texture2D> ClockGearsIcons::get_command_category_icon(const String &p_category, float p_scale) const {
	String cat = p_category.to_lower();
	if (cat.contains("mensaje")) {
		return get_icon("cmd_message", p_scale);
	}
	if (cat.contains("flujo") || cat.contains("progres")) {
		return get_icon("cmd_flow", p_scale);
	}
	if (cat.contains("inventario") || cat.contains("actor") || cat.contains("grupo")) {
		return get_icon("cmd_inventory", p_scale);
	}
	if (cat.contains("movimiento") || cat.contains("personaje") || cat.contains("seguidor")) {
		return get_icon("cmd_movement", p_scale);
	}
	if (cat.contains("escena") || cat.contains("batalla")) {
		return get_icon("cmd_scene", p_scale);
	}
	return get_icon("cmd_system", p_scale);
}

Color ClockGearsIcons::get_command_category_color(const String &p_category) const {
	String cat = p_category.to_lower();
	if (cat.contains("mensaje")) {
		return Color::html("#4da6ff"); // Azul mensajes
	}
	if (cat.contains("flujo") || cat.contains("progres")) {
		return Color::html("#e0a040"); // Ámbar control/progresión
	}
	if (cat.contains("inventario") || cat.contains("actor") || cat.contains("grupo")) {
		return Color::html("#4cc38a"); // Verde inventario/actor
	}
	if (cat.contains("movimiento") || cat.contains("personaje") || cat.contains("seguidor")) {
		return Color::html("#45c4d6"); // Cian movimiento/seguidores
	}
	if (cat.contains("escena") || cat.contains("batalla")) {
		return Color::html("#e5534b"); // Carmesí escena/batalla
	}
	return Color::html("#9aa0ab"); // Gris/plata audio/sistema
}

void ClockGearsIcons::clear_cache() {
	icon_cache.clear();
}

void ClockGearsIcons::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_icon", "name", "scale"), &ClockGearsIcons::get_icon, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_db_category_icon", "category", "scale"), &ClockGearsIcons::get_db_category_icon, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_command_category_icon", "category", "scale"), &ClockGearsIcons::get_command_category_icon, DEFVAL(1.0f));
	ClassDB::bind_method(D_METHOD("get_command_category_color", "category"), &ClockGearsIcons::get_command_category_color);
	ClassDB::bind_method(D_METHOD("clear_cache"), &ClockGearsIcons::clear_cache);
}

ClockGearsIcons::ClockGearsIcons() {
	singleton = this;
	_register_builtin_svgs();
}

ClockGearsIcons::~ClockGearsIcons() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
