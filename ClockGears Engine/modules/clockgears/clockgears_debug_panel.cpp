#include "clockgears_debug_panel.h"
#include "clockgears_map.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_pixel_art.h"
#include "runtime/clockgears_runtime.h"
#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "scene/gui/separator.h"
#include "scene/gui/scroll_container.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"

#define CG_DBG_TR(m_str) String::utf8(m_str)

static Ref<StyleBoxFlat> _debug_window_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.11f, 0.12f, 0.15f, 0.96f));
	style->set_corner_radius_all(6);
	style->set_content_margin_all(10);
	style->set_border_width_all(1);
	style->set_border_color(Color(0.30f, 0.35f, 0.45f, 0.85f));
	style->set_shadow_color(Color(0, 0, 0, 0.45f));
	style->set_shadow_size(8);
	return style;
}

static Ref<StyleBoxFlat> _debug_header_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.08f, 0.09f, 0.12f, 0.95f));
	style->set_corner_radius_all(4);
	style->set_content_margin_individual(8, 4, 8, 4);
	return style;
}

static Ref<StyleBoxFlat> _debug_hud_style() {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_bg_color(Color(0.06f, 0.07f, 0.09f, 0.80f));
	style->set_corner_radius_all(4);
	style->set_content_margin_all(8);
	style->set_border_width_all(1);
	style->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.12f));
	return style;
}

// ============================================================================
// ClockGearsDebugWorldCanvas
// ============================================================================

void ClockGearsDebugWorldCanvas::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_show_events", "show"), &ClockGearsDebugWorldCanvas::set_show_events);
	ClassDB::bind_method(D_METHOD("is_show_events"), &ClockGearsDebugWorldCanvas::is_show_events);
	ClassDB::bind_method(D_METHOD("set_show_collisions", "show"), &ClockGearsDebugWorldCanvas::set_show_collisions);
	ClassDB::bind_method(D_METHOD("is_show_collisions"), &ClockGearsDebugWorldCanvas::is_show_collisions);
	ClassDB::bind_method(D_METHOD("set_show_grid", "show"), &ClockGearsDebugWorldCanvas::set_show_grid);
	ClassDB::bind_method(D_METHOD("is_show_grid"), &ClockGearsDebugWorldCanvas::is_show_grid);
}

ClockGearsDebugWorldCanvas::ClockGearsDebugWorldCanvas() {
	set_z_index(100); // Siempre sobre el mapa y personajes
}

void ClockGearsDebugWorldCanvas::setup(ClockGearsMap *p_map, ClockGearsPixelCamera2D *p_camera) {
	map = p_map;
	camera = p_camera;
	set_process(true);
}

void ClockGearsDebugWorldCanvas::set_show_events(bool p_show) {
	show_events = p_show;
	queue_redraw();
}

void ClockGearsDebugWorldCanvas::set_show_collisions(bool p_show) {
	show_collisions = p_show;
	queue_redraw();
}

void ClockGearsDebugWorldCanvas::set_show_grid(bool p_show) {
	show_grid = p_show;
	queue_redraw();
}

void ClockGearsDebugWorldCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		if (show_events || show_collisions || show_grid) {
			queue_redraw();
		}
	} else if (p_what == NOTIFICATION_DRAW) {
		if (!map) return;

		Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();

		// Rango visible de celdas
		Vector2 cam_pos = camera ? camera->get_position() : (map->get_runtime_player() ? map->get_runtime_player()->get_position() : Vector2(0, 0));
		float zoom = camera ? camera->get_zoom().x : 1.0f;
		if (zoom <= 0.05f) zoom = 1.0f;
		Size2 vp_size = get_viewport() ? get_viewport()->get_visible_rect().size : Size2(960, 540);
		Vector2 half_sz = (vp_size / zoom) * 0.5f;

		int mw = map->get_map_size().x;
		int mh = map->get_map_size().y;
		int min_cx = MAX(0, int(Math::floor((cam_pos.x - half_sz.x - 64.0f) / 32.0f)));
		int max_cx = MIN(mw - 1, int(Math::ceil((cam_pos.x + half_sz.x + 64.0f) / 32.0f)));
		int min_cy = MAX(0, int(Math::floor((cam_pos.y - half_sz.y - 64.0f) / 32.0f)));
		int max_cy = MIN(mh - 1, int(Math::ceil((cam_pos.y + half_sz.y + 64.0f) / 32.0f)));

		// 1. Rejilla de 32x32 (Punto 2)
		if (show_grid && mw > 0 && mh > 0) {
			Color grid_col(1.0f, 1.0f, 1.0f, 0.16f);
			for (int cx = min_cx; cx <= max_cx + 1; cx++) {
				float x = cx * 32.0f;
				draw_line(Vector2(x, min_cy * 32.0f), Vector2(x, (max_cy + 1) * 32.0f), grid_col, 1.0f);
			}
			for (int cy = min_cy; cy <= max_cy + 1; cy++) {
				float y = cy * 32.0f;
				draw_line(Vector2(min_cx * 32.0f, y), Vector2((max_cx + 1) * 32.0f, y), grid_col, 1.0f);
			}
		}

		// 2. Colisiones (Punto 2): Bloqueados (X en rojo), ★ en amarillo, pasables sin marca
		if (show_collisions) {
			for (int cy = min_cy; cy <= max_cy; cy++) {
				for (int cx = min_cx; cx <= max_cx; cx++) {
					Vector2i cell(cx, cy);
					int col = map->get_cell_collision_type(cell);
					Rect2 cell_rect(cx * 32.0f, cy * 32.0f, 32.0f, 32.0f);

					if (col == 1) {
						// Bloqueado (X) en rojo
						draw_rect(cell_rect, Color(0.95f, 0.15f, 0.15f, 0.22f), true);
						draw_rect(cell_rect, Color(0.95f, 0.2f, 0.2f, 0.65f), false, 1.0f);
						draw_line(cell_rect.position + Vector2(6, 6), cell_rect.position + cell_rect.size - Vector2(6, 6), Color(1.0f, 0.25f, 0.25f, 0.95f), 2.0f);
						draw_line(Vector2(cell_rect.position.x + cell_rect.size.x - 6, cell_rect.position.y + 6), Vector2(cell_rect.position.x + 6, cell_rect.position.y + cell_rect.size.y - 6), Color(1.0f, 0.25f, 0.25f, 0.95f), 2.0f);
					} else if (col == 2) {
						// Estrella ★ en amarillo
						draw_rect(cell_rect, Color(1.0f, 0.85f, 0.2f, 0.22f), true);
						draw_rect(cell_rect, Color(1.0f, 0.85f, 0.2f, 0.65f), false, 1.0f);
						if (font.is_valid()) {
							draw_string(font, cell_rect.position + Vector2(10.0f, 22.0f), CG_DBG_TR("★"), HORIZONTAL_ALIGNMENT_CENTER, -1, 16, Color(1.0f, 0.9f, 0.25f, 0.95f));
						}
					}
					// 0 = Pasable: sin marca
				}
			}
		}

		// 3. Eventos (Punto 2): Cuadro con ID y nombre sobre cada evento, y su zona de activación
		if (show_events && map->get_events_container()) {
			Node2D *ev_cont = map->get_events_container();
			for (int i = 0; i < ev_cont->get_child_count(); i++) {
				ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(ev_cont->get_child(i));
				if (!ev) continue;

				Vector2 ev_pos = ev->get_position();

				// Zona de activación (32x32) con color según tipo de disparo
				Rect2 tr_rect(ev_pos + Vector2(2, 2), Vector2(28, 28));
				Color tr_fill(0.2f, 0.8f, 1.0f, 0.30f);
				Color tr_border(0.25f, 0.85f, 1.0f, 0.95f);

				int tr_type = ev->get_trigger_type();
				if (tr_type == 1 || tr_type == 2) {
					// Toque jugador / evento (Naranja)
					tr_fill = Color(1.0f, 0.6f, 0.1f, 0.32f);
					tr_border = Color(1.0f, 0.7f, 0.2f, 0.95f);
				} else if (tr_type == 3 || tr_type == 4) {
					// Autoejecución / Proceso Paralelo (Magenta)
					tr_fill = Color(0.85f, 0.25f, 0.85f, 0.32f);
					tr_border = Color(0.95f, 0.35f, 0.95f, 0.95f);
				}

				draw_rect(tr_rect, tr_fill, true);
				draw_rect(tr_rect, tr_border, false, 2.0f);

				// Cartel con ID y Nombre sobre el evento
				String ev_label = vformat("EV #%d: %s", ev->get_event_id(), ev->get_event_name());
				if (font.is_valid()) {
					int fsz = 9;
					Size2 ssz = font->get_string_size(ev_label, HORIZONTAL_ALIGNMENT_LEFT, -1, fsz);
					Rect2 badge(ev_pos.x + (32.0f - ssz.x - 8.0f) * 0.5f, ev_pos.y - 15.0f, ssz.x + 8.0f, 14.0f);
					draw_rect(badge, Color(0.08f, 0.10f, 0.14f, 0.88f), true);
					draw_rect(badge, tr_border, false, 1.0f);
					draw_string(font, Point2(badge.position.x + 4.0f, badge.position.y + 11.0f), ev_label, HORIZONTAL_ALIGNMENT_LEFT, -1, fsz, Color(1, 1, 1, 0.95f));
				}
			}
		}
	}
}

// ============================================================================
// ClockGearsDebugOverlay
// ============================================================================

void ClockGearsDebugOverlay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("toggle_debug_panel"), &ClockGearsDebugOverlay::toggle_debug_panel);
	ClassDB::bind_method(D_METHOD("is_panel_open"), &ClockGearsDebugOverlay::is_panel_open);
	ClassDB::bind_method(D_METHOD("set_show_events", "show"), &ClockGearsDebugOverlay::set_show_events);
	ClassDB::bind_method(D_METHOD("is_show_events"), &ClockGearsDebugOverlay::is_show_events);
	ClassDB::bind_method(D_METHOD("set_show_collisions", "show"), &ClockGearsDebugOverlay::set_show_collisions);
	ClassDB::bind_method(D_METHOD("is_show_collisions"), &ClockGearsDebugOverlay::is_show_collisions);
	ClassDB::bind_method(D_METHOD("set_show_grid", "show"), &ClockGearsDebugOverlay::set_show_grid);
	ClassDB::bind_method(D_METHOD("is_show_grid"), &ClockGearsDebugOverlay::is_show_grid);
	ClassDB::bind_method(D_METHOD("set_show_fps", "show"), &ClockGearsDebugOverlay::set_show_fps);
	ClassDB::bind_method(D_METHOD("is_show_fps"), &ClockGearsDebugOverlay::is_show_fps);
	ClassDB::bind_method(D_METHOD("set_show_player", "show"), &ClockGearsDebugOverlay::set_show_player);
	ClassDB::bind_method(D_METHOD("is_show_player"), &ClockGearsDebugOverlay::is_show_player);
	ClassDB::bind_method(D_METHOD("set_show_camera", "show"), &ClockGearsDebugOverlay::set_show_camera);
	ClassDB::bind_method(D_METHOD("is_show_camera"), &ClockGearsDebugOverlay::is_show_camera);
	ClassDB::bind_method(D_METHOD("setup_overlay", "map", "player", "camera"), &ClockGearsDebugOverlay::setup_overlay, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_world_canvas"), &ClockGearsDebugOverlay::get_world_canvas);
}

bool ClockGearsDebugOverlay::is_playtest_mode() {
	if (OS::get_singleton()->has_feature("template")) {
		return false;
	}
	if (OS::get_singleton()->has_feature("editor")) {
		return true;
	}
	List<String> uargs = OS::get_singleton()->get_cmdline_user_args();
	for (const String &a : uargs) {
		if (a == "--clockgears-playtest") return true;
	}
	List<String> margs = OS::get_singleton()->get_cmdline_args();
	for (const String &a : margs) {
		if (a == "--clockgears-playtest" || a == "--debug" || a == "-d") return true;
	}
	if (OS::get_singleton()->has_feature("debug")) {
		return true;
	}
	return false;
}

static Vector<String> s_battle_log_history;
static ClockGearsDebugOverlay *s_overlay_singleton = nullptr;

ClockGearsDebugOverlay::ClockGearsDebugOverlay() {
	s_overlay_singleton = this;
	set_layer(120); // Encima de todo
	set_process(true);
	set_process_unhandled_input(true);

	// Lienzo en mundo
	world_canvas = memnew(ClockGearsDebugWorldCanvas);
	add_child(world_canvas);

	// 1. HUD de información en pantalla (esquina superior izquierda)
	hud_card = memnew(PanelContainer);
	hud_card->add_theme_style_override("panel", _debug_hud_style());
	hud_card->set_position(Vector2(14, 14));
	hud_card->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	add_child(hud_card);

	VBoxContainer *hud_vb = memnew(VBoxContainer);
	hud_vb->add_theme_constant_override("separation", 2);
	hud_card->add_child(hud_vb);

	lbl_fps = memnew(Label("FPS: 60 (16.6 ms)"));
	lbl_fps->add_theme_color_override("font_color", Color(0.35f, 0.95f, 0.55f));
	hud_vb->add_child(lbl_fps);

	lbl_player = memnew(Label("Jugador: (0, 0) | Dir: Abajo"));
	lbl_player->add_theme_color_override("font_color", Color(0.40f, 0.85f, 1.0f));
	hud_vb->add_child(lbl_player);

	lbl_camera = memnew(Label("Cámara: Zoom 2.0x"));
	lbl_camera->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.40f));
	hud_vb->add_child(lbl_camera);

	hud_card->hide();

	// 2. Ventana flotante de depuración (F3)
	floating_window = memnew(PanelContainer);
	floating_window->add_theme_style_override("panel", _debug_window_style());
	floating_window->set_custom_minimum_size(Size2(360, 420));
	floating_window->set_position(Vector2(20, 60));
	add_child(floating_window);

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_vb->add_theme_constant_override("separation", 8);
	floating_window->add_child(main_vb);

	// Barra de título arrastrable
	PanelContainer *title_bar = memnew(PanelContainer);
	title_bar->add_theme_style_override("panel", _debug_header_style());
	title_bar->connect("gui_input", callable_mp(this, &ClockGearsDebugOverlay::_on_title_gui_input));
	main_vb->add_child(title_bar);

	HBoxContainer *title_hb = memnew(HBoxContainer);
	title_hb->add_theme_constant_override("separation", 8);
	title_bar->add_child(title_hb);

	Label *title_lbl = memnew(Label(CG_DBG_TR("⚙ Panel de Depuración (F3)")));
	title_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	title_lbl->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.35f));
	title_hb->add_child(title_lbl);

	Button *btn_close = memnew(Button("✕"));
	btn_close->set_custom_minimum_size(Size2(24, 24));
	btn_close->connect("pressed", callable_mp(this, &ClockGearsDebugOverlay::toggle_debug_panel));
	title_hb->add_child(btn_close);

	// Pestañas
	TabContainer *tabs = memnew(TabContainer);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->add_child(tabs);

	// Pestaña 1: Visualización
	VBoxContainer *tab_vis = memnew(VBoxContainer);
	tab_vis->set_name(CG_DBG_TR("Visualización"));
	tab_vis->add_theme_constant_override("separation", 6);
	tabs->add_child(tab_vis);

	chk_events = memnew(CheckBox(CG_DBG_TR("Eventos (ID, Nombre y Zona)")));
	chk_events->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_events);

	chk_collisions = memnew(CheckBox(CG_DBG_TR("Colisiones (X en rojo, ★ en amarillo)")));
	chk_collisions->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_collisions);

	chk_grid = memnew(CheckBox(CG_DBG_TR("Rejilla de 32x32")));
	chk_grid->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_grid);

	tab_vis->add_child(memnew(HSeparator));

	chk_fps = memnew(CheckBox(CG_DBG_TR("FPS y tiempo de frame")));
	chk_fps->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_fps);

	chk_player = memnew(CheckBox(CG_DBG_TR("Datos del jugador (Celda, Dirección, Mapa)")));
	chk_player->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_player);

	chk_camera = memnew(CheckBox(CG_DBG_TR("Cámara (Zoom y Rectángulo visible)")));
	chk_camera->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_chk_toggled));
	tab_vis->add_child(chk_camera);

	// Pestaña 2: Variables e Interruptores (Punto 3 - Preparado)
	VBoxContainer *tab_vars = memnew(VBoxContainer);
	tab_vars->set_name(CG_DBG_TR("Variables"));
	tab_vars->add_theme_constant_override("separation", 6);
	tabs->add_child(tab_vars);

	Label *lbl_vars_desc = memnew(Label(CG_DBG_TR("Inspección y edición en vivo de Interruptores y Variables:")));
	lbl_vars_desc->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
	tab_vars->add_child(lbl_vars_desc);

	vars_search = memnew(LineEdit);
	vars_search->set_placeholder(CG_DBG_TR("Buscar interruptor o variable..."));
	tab_vars->add_child(vars_search);

	vars_tree = memnew(Tree);
	vars_tree->set_columns(3);
	vars_tree->set_column_title(0, "ID");
	vars_tree->set_column_title(1, CG_DBG_TR("Nombre"));
	vars_tree->set_column_title(2, CG_DBG_TR("Valor"));
	vars_tree->set_column_titles_visible(true);
	vars_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tab_vars->add_child(vars_tree);

	Label *lbl_vars_hint = memnew(Label(CG_DBG_TR("💡 Espacio preparado: se conectará a la edición interactiva completa.")));
	lbl_vars_hint->add_theme_color_override("font_color", Color(0.7f, 0.85f, 1.0f));
	tab_vars->add_child(lbl_vars_hint);

	// Pestaña 3: Herramientas (Punto 3 - Preparado)
	VBoxContainer *tab_tools = memnew(VBoxContainer);
	tab_tools->set_name(CG_DBG_TR("Herramientas"));
	tab_tools->add_theme_constant_override("separation", 8);
	tabs->add_child(tab_tools);

	Label *lbl_tools_desc = memnew(Label(CG_DBG_TR("Herramientas rápidas de depuración:")));
	lbl_tools_desc->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
	tab_tools->add_child(lbl_tools_desc);

	chk_noclip = memnew(CheckBox(CG_DBG_TR("Modo Fantasma (Atravesar paredes / NoClip)")));
	chk_noclip->connect("toggled", callable_mp(this, &ClockGearsDebugOverlay::_on_noclip_toggled));
	tab_tools->add_child(chk_noclip);

	tab_tools->add_child(memnew(HSeparator));

	Button *btn_teleport = memnew(Button(CG_DBG_TR("Teletransportarse a mapa...")));
	btn_teleport->set_disabled(true);
	btn_teleport->set_tooltip_text(CG_DBG_TR("Preparado: selector de mapa y coordenadas"));
	tab_tools->add_child(btn_teleport);

	Button *btn_give_items = memnew(Button(CG_DBG_TR("Dar objetos / Oro...")));
	btn_give_items->set_disabled(true);
	btn_give_items->set_tooltip_text(CG_DBG_TR("Preparado: requiere sistema de inventario"));
	tab_tools->add_child(btn_give_items);

	Button *btn_heal = memnew(Button(CG_DBG_TR("Recuperar Grupo (HP/MP 100%)")));
	btn_heal->set_disabled(true);
	btn_heal->set_tooltip_text(CG_DBG_TR("Preparado: requiere sistema de combate/menú"));
	tab_tools->add_child(btn_heal);

	// Pestaña 4: Log de Combate (Fase 8A)
	VBoxContainer *tab_battle = memnew(VBoxContainer);
	tab_battle->set_name(CG_DBG_TR("Combate"));
	tab_battle->add_theme_constant_override("separation", 6);
	tabs->add_child(tab_battle);

	Label *lbl_battle_desc = memnew(Label(CG_DBG_TR("Registro de golpes y fórmulas de combate:")));
	lbl_battle_desc->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
	tab_battle->add_child(lbl_battle_desc);

	battle_log_rtl = memnew(RichTextLabel);
	battle_log_rtl->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	battle_log_rtl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	battle_log_rtl->set_scroll_follow(true);
	tab_battle->add_child(battle_log_rtl);

	for (int i = 0; i < s_battle_log_history.size(); i++) {
		battle_log_rtl->append_text(s_battle_log_history[i] + "\n");
	}

	// Inicia oculta
	floating_window->hide();

	_load_settings();
	_populate_vars_tree();
}

ClockGearsDebugOverlay::~ClockGearsDebugOverlay() {
	if (s_overlay_singleton == this) {
		s_overlay_singleton = nullptr;
	}
}

void ClockGearsDebugOverlay::log_battle(const String &p_entry) {
	s_battle_log_history.push_back(p_entry);
	if (s_overlay_singleton && s_overlay_singleton->battle_log_rtl) {
		s_overlay_singleton->battle_log_rtl->append_text(p_entry + "\n");
	}
}

void ClockGearsDebugOverlay::setup_overlay(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsPixelCamera2D *p_camera) {
	map = p_map;
	player = p_player;
	camera = p_camera;
	if (world_canvas) {
		world_canvas->setup(p_map, p_camera);
	}
	_on_chk_toggled(false);
}

void ClockGearsDebugOverlay::toggle_debug_panel() {
	if (!is_playtest_mode()) {
		return;
	}
	if (floating_window) {
		bool was_vis = floating_window->is_visible();
		floating_window->set_visible(!was_vis);
		if (!was_vis) {
			_populate_vars_tree();
		}
	}
}

bool ClockGearsDebugOverlay::is_panel_open() const {
	return floating_window && floating_window->is_visible();
}

void ClockGearsDebugOverlay::set_show_events(bool p_show) {
	if (chk_events) chk_events->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_events() const {
	return chk_events && chk_events->is_pressed();
}

void ClockGearsDebugOverlay::set_show_collisions(bool p_show) {
	if (chk_collisions) chk_collisions->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_collisions() const {
	return chk_collisions && chk_collisions->is_pressed();
}

void ClockGearsDebugOverlay::set_show_grid(bool p_show) {
	if (chk_grid) chk_grid->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_grid() const {
	return chk_grid && chk_grid->is_pressed();
}

void ClockGearsDebugOverlay::set_show_fps(bool p_show) {
	if (chk_fps) chk_fps->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_fps() const {
	return chk_fps && chk_fps->is_pressed();
}

void ClockGearsDebugOverlay::set_show_player(bool p_show) {
	if (chk_player) chk_player->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_player() const {
	return chk_player && chk_player->is_pressed();
}

void ClockGearsDebugOverlay::set_show_camera(bool p_show) {
	if (chk_camera) chk_camera->set_pressed(p_show);
}
bool ClockGearsDebugOverlay::is_show_camera() const {
	return chk_camera && chk_camera->is_pressed();
}

void ClockGearsDebugOverlay::_on_chk_toggled(bool) {
	if (world_canvas) {
		world_canvas->set_show_events(chk_events && chk_events->is_pressed());
		world_canvas->set_show_collisions(chk_collisions && chk_collisions->is_pressed());
		world_canvas->set_show_grid(chk_grid && chk_grid->is_pressed());
	}
	_save_settings();
	_update_hud();
}

void ClockGearsDebugOverlay::_on_noclip_toggled(bool p_val) {
	if (player) {
		player->set_through_enabled(p_val);
	}
	_save_settings();
}

void ClockGearsDebugOverlay::_on_title_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::LEFT) {
		is_dragging = mb->is_pressed();
		drag_offset = mb->get_position();
	}
	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && is_dragging && floating_window) {
		floating_window->set_position(floating_window->get_position() + mm->get_position() - drag_offset);
	}
}

void ClockGearsDebugOverlay::_save_settings() {
	if (is_loading_settings) {
		return;
	}
	Error err;
	Ref<FileAccess> f = FileAccess::open("user://clockgears_debug_settings.json", FileAccess::WRITE, &err);
	if (err == OK && f.is_valid()) {
		Dictionary d;
		d["show_events"] = chk_events ? chk_events->is_pressed() : false;
		d["show_collisions"] = chk_collisions ? chk_collisions->is_pressed() : false;
		d["show_grid"] = chk_grid ? chk_grid->is_pressed() : false;
		d["show_fps"] = chk_fps ? chk_fps->is_pressed() : false;
		d["show_player"] = chk_player ? chk_player->is_pressed() : false;
		d["show_camera"] = chk_camera ? chk_camera->is_pressed() : false;
		if (chk_noclip) d["noclip"] = chk_noclip->is_pressed();
		f->store_string(JSON::stringify(d));
		f->flush();
		f->close();
		f.unref();
	}
}

void ClockGearsDebugOverlay::_load_settings() {
	if (FileAccess::exists("user://clockgears_debug_settings.json")) {
		Error err;
		Ref<FileAccess> f = FileAccess::open("user://clockgears_debug_settings.json", FileAccess::READ, &err);
		if (err == OK && f.is_valid()) {
			String text = f->get_as_text();
			f->close();
			f.unref();

			Variant v = JSON::parse_string(text);
			if (v.get_type() == Variant::DICTIONARY) {
				Dictionary d = v;
				is_loading_settings = true;
				if (chk_events) chk_events->set_pressed(bool(d.get("show_events", false)));
				if (chk_collisions) chk_collisions->set_pressed(bool(d.get("show_collisions", false)));
				if (chk_grid) chk_grid->set_pressed(bool(d.get("show_grid", false)));
				if (chk_fps) chk_fps->set_pressed(bool(d.get("show_fps", false)));
				if (chk_player) chk_player->set_pressed(bool(d.get("show_player", false)));
				if (chk_camera) chk_camera->set_pressed(bool(d.get("show_camera", false)));
				if (chk_noclip) chk_noclip->set_pressed(bool(d.get("noclip", false)));
				is_loading_settings = false;
			}
		}
	}
}

void ClockGearsDebugOverlay::_populate_vars_tree() {
	if (!vars_tree) return;
	vars_tree->clear();
	TreeItem *root = vars_tree->create_item();

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;

	// Interruptores
	Dictionary sw_names = dm->get_switches_names();
	TreeItem *sw_root = vars_tree->create_item(root);
	sw_root->set_text(0, CG_DBG_TR("Interruptores"));
	sw_root->set_selectable(0, false);

	for (int i = 1; i <= 20; i++) {
		String k = itos(i);
		String sname = sw_names.get(k, vformat("Interruptor %04d", i));
		bool sval = dm->get_switch(k);

		TreeItem *item = vars_tree->create_item(sw_root);
		item->set_text(0, vformat("%04d", i));
		item->set_text(1, sname);
		item->set_text(2, sval ? "ON" : "OFF");
		item->set_custom_color(2, sval ? Color(0.3f, 0.9f, 0.4f) : Color(0.6f, 0.6f, 0.6f));
	}

	// Variables
	Dictionary var_names = dm->get_variables_names();
	TreeItem *var_root = vars_tree->create_item(root);
	var_root->set_text(0, CG_DBG_TR("Variables"));
	var_root->set_selectable(0, false);

	for (int i = 1; i <= 20; i++) {
		String k = itos(i);
		String vname = var_names.get(k, vformat("Variable %04d", i));
		int vval = dm->get_variable(k);

		TreeItem *item = vars_tree->create_item(var_root);
		item->set_text(0, vformat("%04d", i));
		item->set_text(1, vname);
		item->set_text(2, itos(vval));
		item->set_custom_color(2, Color(0.4f, 0.8f, 1.0f));
	}
}

void ClockGearsDebugOverlay::_update_hud() {
	bool show_fps = chk_fps && chk_fps->is_pressed();
	bool show_player = chk_player && chk_player->is_pressed();
	bool show_camera = chk_camera && chk_camera->is_pressed();

	bool any_hud = show_fps || show_player || show_camera;
	if (hud_card) {
		hud_card->set_visible(any_hud);
	}
	if (!any_hud) return;

	// FPS
	if (lbl_fps) {
		lbl_fps->set_visible(show_fps);
		if (show_fps) {
			int fps = Engine::get_singleton()->get_frames_per_second();
			float frame_time = (fps > 0) ? (1000.0f / (float)fps) : 0.0f;
			lbl_fps->set_text(vformat("FPS: %d (%.1f ms)", fps, frame_time));
		}
	}

	// Jugador
	if (lbl_player) {
		lbl_player->set_visible(show_player);
		if (show_player) {
			Vector2i cell = player ? player->get_grid_cell() : Vector2i(0, 0);
			int dir = player ? player->get_direction() : 2;
			const char *dir_name = (dir == 2) ? "Abajo" : ((dir == 4) ? "Izquierda" : ((dir == 6) ? "Derecha" : "Arriba"));
			String map_name = map ? map->get_map_name() : "Mapa";
			lbl_player->set_text(vformat("Jugador: (%d, %d) | Dir: %s | Mapa: %s", cell.x, cell.y, dir_name, map_name));
		}
	}

	// Cámara
	if (lbl_camera) {
		lbl_camera->set_visible(show_camera);
		if (show_camera) {
			float z = camera ? camera->get_zoom().x : 1.0f;
			Vector2 cpos = camera ? camera->get_position() : Vector2(0, 0);
			Size2 vsz = get_viewport_size();
			if (vsz.x <= 0 || vsz.y <= 0) vsz = Size2(960, 540);
			Rect2 vis_rect(cpos - (vsz / z) * 0.5f, vsz / z);
			lbl_camera->set_text(vformat("Cámara: Zoom %.1fx | Rect: (%.0f, %.0f, %.0f, %.0f)", z, vis_rect.position.x, vis_rect.position.y, vis_rect.size.x, vis_rect.size.y));
		}
	}
}

void ClockGearsDebugOverlay::_notification(int p_what) {
	if (p_what == NOTIFICATION_PROCESS) {
		_update_hud();
	}
}

void ClockGearsDebugOverlay::unhandled_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::F3) {
			toggle_debug_panel();
			Viewport *active_vp = Node::get_viewport();
			if (active_vp) {
				active_vp->set_input_as_handled();
			}
		}
	}
}
