#ifndef CLOCKGEARS_TILE_PICKER_H
#define CLOCKGEARS_TILE_PICKER_H

#include "core/io/image.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_bar.h"
#include "modules/tilemap/tile_set.h"
#include "scene/resources/texture.h"

class ClockGearsTilePicker;

class ClockGearsTilePickerCanvas : public Control {
	GDCLASS(ClockGearsTilePickerCanvas, Control);

	ClockGearsTilePicker *owner_picker = nullptr;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	virtual Size2 get_minimum_size() const override;
	ClockGearsTilePickerCanvas(ClockGearsTilePicker *p_owner = nullptr);
};

// ============================================================================
// ClockGearsTilePicker (Sección 6.1 y Anexo A4.2 / A5)
// Control reutilizable en Editor de Eventos, Flora/Fauna, Tilesets DB y Mapa.
// ============================================================================
class ClockGearsTilePicker : public VBoxContainer {
	GDCLASS(ClockGearsTilePicker, VBoxContainer);

public:
	enum EditMode {
		MODE_SELECT = 0,          // Selección simple, rectangular o Ctrl+clic
		MODE_PASSAGE_BASIC = 1,   // ○ (Pasa) / × (Bloqueado) / ★ (Sobre personaje)
		MODE_PASSAGE_4DIR = 2,    // 4 Direcciones independientes (↓ ← → ↑)
		MODE_TERRAIN_TAG = 3,     // Etiqueta de terreno (0..7)
		MODE_BUSH = 4,            // Arbusto (~)
		MODE_COUNTER = 5,         // Mostrador (♦)
		MODE_LADDER = 6,          // Escalera (H)
		MODE_DAMAGE_FLOOR = 7     // Daño de suelo (▲)
	};

	enum PassageBits {
		PASS_DOWN = 1 << 0,  // 1
		PASS_LEFT = 1 << 1,  // 2
		PASS_RIGHT = 1 << 2, // 4
		PASS_UP = 1 << 3,    // 8
		PASS_ALL = 0x0F,
		PASS_NONE = 0x00
	};

private:
	Ref<TileSet> current_tileset;
	Ref<Texture2D> current_atlas_texture;
	String current_atlas_path;
	int current_source_id = 0;
	int tile_size = 32;
	float zoom_level = 1.0f;
	Vector2 pan_offset = Vector2(0, 0);
	bool show_grid = true;
	bool only_favorites = false;
	bool metadata_editing_enabled = true;
	EditMode current_edit_mode = MODE_SELECT;
	String current_slot_key = "A5";

	// Arrastre para pintar metadatos en múltiples tiles (O/X/★, 4-Dir, etc.)
	bool is_dragging_metadata = false;
	MouseButton drag_metadata_button = MouseButton::LEFT;
	Dictionary drag_target_metadata;
	Vector2i last_drag_metadata_tile = Vector2i(-1, -1);

	// Selección actual
	Vector<Vector2i> selected_tiles;
	Vector2i selection_block_size = Vector2i(1, 1);
	Vector2i hovered_tile = Vector2i(-1, -1);
	bool is_dragging_selection = false;
	Vector2i drag_start_tile = Vector2i(-1, -1);
	bool is_panning = false;
	Vector2 pan_last_mouse = Vector2();

	// Metadatos RPG por coordenada "x,y" -> Dictionary { passage, layer, terrain_tag, bush, counter }
	Dictionary tile_metadata_map;
	// Favoritos persistidos en ProjectSettings ("clockgears/editor/favorite_tiles")
	PackedStringArray favorite_keys;

	// Controles UI superiores
	HBoxContainer *top_row_1 = nullptr;
	OptionButton *atlas_selector = nullptr;
	LineEdit *search_filter_edit = nullptr;
	Button *btn_fav_toggle = nullptr;
	Button *btn_only_favorites = nullptr;
	Button *btn_load_tileset = nullptr;

	HBoxContainer *top_row_2 = nullptr;
	Button *btn_zoom_out = nullptr;
	OptionButton *zoom_selector = nullptr;
	Button *btn_zoom_in = nullptr;
	Button *btn_grid_toggle = nullptr;
	HBoxContainer *mode_buttons_box = nullptr;
	Vector<Button *> mode_buttons;

	// Lienzo central y barras de desplazamiento
	ClockGearsTilePickerCanvas *canvas = nullptr;
	HScrollBar *h_scroll = nullptr;
	VScrollBar *v_scroll = nullptr;
	bool updating_scrollbars = false;

	// Barra de estado inferior (oculta partes que no aplican)
	HBoxContainer *footer_bar = nullptr;
	Label *coords_label = nullptr;
	Label *metadata_label = nullptr;

	FileDialog *file_dialog = nullptr;
	AcceptDialog *error_dialog = nullptr;
	Callable on_metadata_changed_callback;
	Callable on_request_load_callback;
	Callable on_atlas_loaded_callback;

	void _load_favorites_from_settings();
	void _save_favorites_to_settings();
	String _make_fav_key(const Vector2i &p_coords) const;
	bool _is_tile_favorite(const Vector2i &p_coords) const;

	void _sync_metadata_from_tileset();
	void _sync_tile_metadata_to_tileset(const Vector2i &p_coords, const Dictionary &p_meta);

	void _on_atlas_selected(int p_index);
	void _on_atlas_pressed();
	void _on_search_filter_changed(const String &p_text);
	void _on_toggle_favorite_pressed();
	void _on_only_favorites_toggled(bool p_pressed);
	void _on_load_tileset_pressed();
	void _on_file_selected(const String &p_path);
	void _on_zoom_out_pressed();
	void _on_zoom_in_pressed();
	void _on_zoom_option_selected(int p_index);
	void _on_grid_toggled(bool p_pressed);
	void _on_edit_mode_selected(int p_mode);
	void _on_scroll_changed(double p_val);
	void _update_scrollbars();

	void _update_footer_info();
	void _emit_selection_changed();

	friend class ClockGearsTilePickerCanvas;
	void _draw_canvas();
	void _gui_input_canvas(const Ref<InputEvent> &p_event);
	Vector2i _screen_to_tile(const Vector2 &p_local_pos, bool *r_inside = nullptr) const;
	void _handle_metadata_click(const Vector2i &p_tile, const Vector2 &p_local_pos_in_tile, MouseButton p_button);
	void _apply_metadata_to_tile(const Vector2i &p_tile, const Dictionary &p_meta);

protected:
	static void _bind_methods();

public:
	// Capas de datos personalizadas precreadas (Regla de Base Precreada 1.2 y Sección 6.2)
	static void ensure_rpg_custom_data_layers(Ref<TileSet> p_tileset);

	// Importador compartido de hojas PNG/WebP y recursos .tres (corte en 32x32, copia a res://img/tilesets y logs paso a paso)
	static bool import_tileset_resource(const String &p_source_path, Ref<Texture2D> &r_out_texture, Ref<TileSet> &r_out_tileset, String &r_out_res_path, String &r_out_error, int p_tile_size = 32);

	// A4.2: Extrusión de bordes de 1px alrededor de cada tile 32x32 para evitar sangrado GPU
	static Ref<Image> build_padded_tileset_atlas(const Ref<Image> &p_src_image, int p_tile_size = 32, int p_padding_px = 1);

	// Sección 6.2: Autotiles de 47 patrones de borde (estilo RPG Maker A1-A5 con TerrainSet)
	static void configure_autotile_47_bitmask(Ref<TileSet> p_tileset, int p_source_id, int p_terrain_set = 0, int p_terrain_id = 0, const String &p_terrain_name = "Autotile");

	// Sección 6.2: Tiles animados por frames (agua, cascadas, fuego) sin interpolación borrosa
	static void configure_animated_tile(Ref<TileSet> p_tileset, int p_source_id, const Vector2i &p_base_coords, int p_frame_count, float p_frame_duration_sec = 0.2f);

	void set_tileset(const Ref<TileSet> &p_tileset, int p_source_id = 0);
	Ref<TileSet> get_tileset() const { return current_tileset; }

	void set_atlas_texture(const Ref<Texture2D> &p_tex, const String &p_path = "");
	Ref<Texture2D> get_atlas_texture() const { return current_atlas_texture; }
	String get_atlas_path() const { return current_atlas_path; }

	void set_tile_size(int p_size);
	int get_tile_size() const { return tile_size; }

	void set_zoom_level(float p_zoom);
	float get_zoom_level() const { return zoom_level; }

	void set_metadata_editing_enabled(bool p_enabled);
	bool is_metadata_editing_enabled() const { return metadata_editing_enabled; }

	void set_edit_mode(EditMode p_mode);
	EditMode get_edit_mode() const { return current_edit_mode; }

	void set_slot_key(const String &p_key) { current_slot_key = p_key; }
	String get_slot_key() const { return current_slot_key; }
	Vector2i get_autotile_block_size() const {
		if (current_slot_key == "A1" || current_slot_key == "A2") {
			return Vector2i(2, 3); // 64x96 px (RPG Maker MZ)
		} else if (current_slot_key == "A3" || current_slot_key == "A4") {
			return Vector2i(2, 2); // 64x64 px (RPG Maker MZ)
		}
		return Vector2i(1, 1);
	}
	bool is_autotile_slot() const {
		return current_slot_key == "A1" || current_slot_key == "A2" || current_slot_key == "A3" || current_slot_key == "A4";
	}

	void set_tile_metadata_map(const Dictionary &p_map);
	Dictionary get_tile_metadata_map() const { return tile_metadata_map; }
	Dictionary get_tile_metadata(const Vector2i &p_coords) const;
	void set_tile_metadata(const Vector2i &p_coords, const Dictionary &p_meta);

	void set_on_metadata_changed(const Callable &p_cb) { on_metadata_changed_callback = p_cb; }
	void set_on_request_load(const Callable &p_cb) { on_request_load_callback = p_cb; }
	void set_on_atlas_loaded(const Callable &p_cb) { on_atlas_loaded_callback = p_cb; }
	void set_load_button_visible(bool p_visible);

	// Devuelve Array de Dictionaries: { "source_id": int, "atlas_coords": Vector2i, "alternative": int }
	Array get_selected_tiles_array() const;
	Vector<Vector2i> get_selected_coords() const { return selected_tiles; }
	Vector2i get_selection_block_size() const { return selection_block_size; }
	Vector2i get_primary_selected_tile() const { return selected_tiles.is_empty() ? Vector2i(0, 0) : selected_tiles[0]; }

	ClockGearsTilePicker();
};

// ============================================================================
// ClockGearsTilePickerDialog (ConfirmationDialog modal reutilizable)
// ============================================================================
class ClockGearsTilePickerDialog : public ConfirmationDialog {
	GDCLASS(ClockGearsTilePickerDialog, ConfirmationDialog);

private:
	ClockGearsTilePicker *picker = nullptr;
	Callable on_confirmed_callback;

	void _on_dialog_confirmed();

protected:
	static void _bind_methods();

public:
	ClockGearsTilePicker *get_picker() const { return picker; }
	void open_picker(const Ref<Texture2D> &p_tex, const Callable &p_on_confirmed = Callable());

	ClockGearsTilePickerDialog();
};

#endif // CLOCKGEARS_TILE_PICKER_H
