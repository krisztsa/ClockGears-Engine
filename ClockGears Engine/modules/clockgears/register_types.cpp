#include "register_types.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"

// Núcleo Fase 1 + Anexo A (core/)
#include "core/clockgears_data_manager.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_id_registry.h"
#include "core/clockgears_pixel_art.h"
#include "core/clockgears_system_catalog.h"
#include "core/clockgears_time_manager.h"

// Tema, Iconos y Widgets Reutilizables Fase 2 & Fase 3 (icons/ y editor/)
#include "editor/clockgears_project_manager.h"
#include "editor/clockgears_theme.h"
#include "editor/clockgears_tile_picker.h"
#include "editor/clockgears_ui_widgets.h"
#include "icons/clockgears_icons.h"

// Nodos y Runtime
#include "clockgears_action_rpg.h"
#include "clockgears_battler.h"
#include "clockgears_damage_engine.h"
#include "clockgears_event.h"
#include "clockgears_fx_manager.h"
#include "clockgears_map.h"
#include "clockgears_turn_battle.h"
#include "clockgears_animation.h"
#include "clockgears_battle.h"
#include "clockgears_battle_window.h"
#include "runtime/clockgears_runtime.h"
#include "runtime/clockgears_message_window.h"
#include "runtime/clockgears_text_parser.h"
#include "runtime/clockgears_audio_manager.h"
#include "runtime/clockgears_picture_manager.h"
#include "runtime/clockgears_cutscene.h"
#include "clockgears_debug_panel.h"

#ifdef TOOLS_ENABLED
#include "clockgears_automation_panel.h"
#include "clockgears_command_selector.h"
#include "clockgears_database.h"
#include "clockgears_editor_plugin.h"
#include "clockgears_event_editor_dialog.h"
#include "clockgears_map_properties_dialog.h"
#include "clockgears_preferences_dialog.h"
#include "clockgears_simplified_export_dialog.h"
#include "clockgears_top_bar.h"
#include "clockgears_ui_editor.h"
#include "clockgears_workspace.h"
#include "editor/clockgears_picture_picker.h"
#include "clockgears_camera_dialog.h"
#endif

static ClockGearsSystemCatalog *cg_system_catalog_singleton = nullptr;
static ClockGearsIdRegistry *cg_id_registry_singleton = nullptr;
static ClockGearsFormulas *cg_formulas_singleton = nullptr;
static ClockGearsTimeManager *cg_time_manager_singleton = nullptr;
static ClockGearsPixelArt *cg_pixel_art_singleton = nullptr;
static ClockGearsDataManager *cg_data_manager_singleton = nullptr;
static ClockGearsIcons *cg_icons_singleton = nullptr;
static ClockGearsTheme *cg_theme_singleton = nullptr;
static ClockGearsAudioManager *cg_audio_manager_singleton = nullptr;
static ClockGearsAnimationManager *cg_animation_manager_singleton = nullptr;

void initialize_clockgears_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		print_line(vformat(String::utf8("[ClockGears Module] Build: %s %s"), __DATE__, __TIME__));
		// 1. Clases y Singletons del Núcleo (antes de cualquier nodo o UI - Bug #14)
		GDREGISTER_CLASS(ClockGearsSystemCatalog);
		GDREGISTER_CLASS(ClockGearsIdRegistry);
		GDREGISTER_CLASS(ClockGearsFormulas);
		GDREGISTER_CLASS(ClockGearsTimeManager);
		GDREGISTER_CLASS(ClockGearsPixelArt);
		GDREGISTER_CLASS(ClockGearsPixelCamera2D);
		GDREGISTER_CLASS(ClockGearsDataManager);
		GDREGISTER_CLASS(ClockGearsIcons);
		GDREGISTER_CLASS(ClockGearsTheme);
		GDREGISTER_INTERNAL_CLASS(ClockGearsFormulaLineEdit);
		GDREGISTER_INTERNAL_CLASS(ClockGearsSwitchVarPickerDialog);
		GDREGISTER_INTERNAL_CLASS(ClockGearsSwitchVarPickerButton);
		GDREGISTER_INTERNAL_CLASS(ClockGearsRecordPickerButton);
		GDREGISTER_CLASS(ClockGearsTilePickerCanvas);
		GDREGISTER_CLASS(ClockGearsTilePicker);
		GDREGISTER_CLASS(ClockGearsTilePickerDialog);
		GDREGISTER_CLASS(ClockGearsDebugWorldCanvas);
		GDREGISTER_CLASS(ClockGearsDebugOverlay);
#ifdef TOOLS_ENABLED
		GDREGISTER_CLASS(ClockGearsTilesetCollisionEditor);
		GDREGISTER_CLASS(ClockGearsDatabaseDialog);
		GDREGISTER_CLASS(ClockGearsTilePalette);
		GDREGISTER_CLASS(ClockGearsMapPropertiesDialog);
		GDREGISTER_CLASS(ClockGearsMapPanel);
		GDREGISTER_CLASS(ClockGearsCommandSelector);
		GDREGISTER_CLASS(ClockGearsEventEditorDialog);
		GDREGISTER_CLASS(ClockGearsPicturePreviewCanvas);
		GDREGISTER_CLASS(ClockGearsPictureVisualPicker);
		GDREGISTER_CLASS(ClockGearsCameraPreviewCanvas);
		GDREGISTER_CLASS(ClockGearsCameraDialog);
#endif

		cg_system_catalog_singleton = memnew(ClockGearsSystemCatalog);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsSystemCatalog", ClockGearsSystemCatalog::get_singleton()));

		cg_id_registry_singleton = memnew(ClockGearsIdRegistry);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsIdRegistry", ClockGearsIdRegistry::get_singleton()));

		cg_formulas_singleton = memnew(ClockGearsFormulas);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsFormulas", ClockGearsFormulas::get_singleton()));

		cg_time_manager_singleton = memnew(ClockGearsTimeManager);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsTimeManager", ClockGearsTimeManager::get_singleton()));

		cg_pixel_art_singleton = memnew(ClockGearsPixelArt);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsPixelArt", ClockGearsPixelArt::get_singleton()));

		cg_data_manager_singleton = memnew(ClockGearsDataManager);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsDataManager", ClockGearsDataManager::get_singleton()));

		cg_icons_singleton = memnew(ClockGearsIcons);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsIcons", ClockGearsIcons::get_singleton()));

		cg_theme_singleton = memnew(ClockGearsTheme);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsTheme", ClockGearsTheme::get_singleton()));

		cg_audio_manager_singleton = memnew(ClockGearsAudioManager);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsAudioManager", ClockGearsAudioManager::get_singleton()));

		cg_animation_manager_singleton = memnew(ClockGearsAnimationManager);
		Engine::get_singleton()->add_singleton(Engine::Singleton("ClockGearsAnimationManager", ClockGearsAnimationManager::get_singleton()));

#ifdef TOOLS_ENABLED
		// Ejecutar auto-tests numéricos del motor matemático y parser de fórmulas (Sección 9)
		bool tests_ok = cg_formulas_singleton->run_self_tests();
		ERR_FAIL_COND_MSG(!tests_ok, String::utf8("ClockGearsFormulas: Fallaron los auto-tests internos del motor matemático."));
#endif

		// 2. Clases de Escena y Runtime
		GDREGISTER_CLASS(ClockGearsMap);
		GDREGISTER_CLASS(ClockGearsEvent);
		GDREGISTER_CLASS(ClockGearsPlayer);
		GDREGISTER_CLASS(ClockGearsFollower);
		GDREGISTER_CLASS(ClockGearsFollowerManager);
		GDREGISTER_CLASS(ClockGearsEventInterpreter);
		GDREGISTER_CLASS(ClockGearsBattler);
		GDREGISTER_CLASS(ClockGearsDamageEngine);
		GDREGISTER_CLASS(ClockGearsFXInstance);
		GDREGISTER_CLASS(ClockGearsFXManager);
		GDREGISTER_CLASS(ClockGearsHurtbox2D);
		GDREGISTER_CLASS(ClockGearsHitbox2D);
		GDREGISTER_CLASS(ClockGearsTurnBattle);
		GDREGISTER_CLASS(ClockGearsTextParser);
		GDREGISTER_CLASS(ClockGearsMessageWindow);
		GDREGISTER_CLASS(ClockGearsAudioManager);
		GDREGISTER_CLASS(ClockGearsPictureManager);
		GDREGISTER_CLASS(ClockGearsCutscene);
		GDREGISTER_CLASS(ClockGearsCutscenePlayer);
		GDREGISTER_CLASS(ClockGearsAnimationTriggerTable);
		GDREGISTER_CLASS(ClockGearsAnimationManager);
		GDREGISTER_CLASS(ClockGearsBattleUnit);
		GDREGISTER_CLASS(ClockGearsBattleController);
		GDREGISTER_CLASS(ClockGearsBattleScene);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_INTERNAL_CLASS(ClockGearsTopBar);
		GDREGISTER_INTERNAL_CLASS(ClockGearsSimplifiedExportDialog);
		GDREGISTER_INTERNAL_CLASS(ClockGearsAutomationPanel);
		GDREGISTER_INTERNAL_CLASS(ClockGearsUIEditorPanel);
		GDREGISTER_INTERNAL_CLASS(ClockGearsPreferencesDialog);
		GDREGISTER_INTERNAL_CLASS(ClockGearsNewProjectDialog);
		GDREGISTER_INTERNAL_CLASS(ClockGearsResourceManagerDialog);
		GDREGISTER_INTERNAL_CLASS(ClockGearsEditorPlugin);

		EditorPlugins::add_by_type<ClockGearsEditorPlugin>();
	}
#endif
}

void uninitialize_clockgears_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (Engine::get_singleton()) {
		if (cg_theme_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsTheme");
		}
		if (cg_audio_manager_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsAudioManager");
		}
		if (cg_animation_manager_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsAnimationManager");
		}
		if (cg_icons_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsIcons");
		}
		if (cg_data_manager_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsDataManager");
		}
		if (cg_pixel_art_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsPixelArt");
		}
		if (cg_time_manager_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsTimeManager");
		}
		if (cg_formulas_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsFormulas");
		}
		if (cg_id_registry_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsIdRegistry");
		}
		if (cg_system_catalog_singleton) {
			Engine::get_singleton()->remove_singleton("ClockGearsSystemCatalog");
		}
	}

	if (cg_theme_singleton) {
		memdelete(cg_theme_singleton);
		cg_theme_singleton = nullptr;
	}
	if (ClockGearsAudioManager::get_singleton()) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am->get_parent()) {
			am->get_parent()->remove_child(am);
		}
		memdelete(am);
	}
	cg_audio_manager_singleton = nullptr;
	if (cg_animation_manager_singleton) {
		memdelete(cg_animation_manager_singleton);
		cg_animation_manager_singleton = nullptr;
	}
	if (cg_icons_singleton) {
		memdelete(cg_icons_singleton);
		cg_icons_singleton = nullptr;
	}
	if (cg_data_manager_singleton) {
		memdelete(cg_data_manager_singleton);
		cg_data_manager_singleton = nullptr;
	}
	if (cg_pixel_art_singleton) {
		memdelete(cg_pixel_art_singleton);
		cg_pixel_art_singleton = nullptr;
	}
	if (cg_time_manager_singleton) {
		memdelete(cg_time_manager_singleton);
		cg_time_manager_singleton = nullptr;
	}
	if (cg_formulas_singleton) {
		memdelete(cg_formulas_singleton);
		cg_formulas_singleton = nullptr;
	}
	if (cg_id_registry_singleton) {
		memdelete(cg_id_registry_singleton);
		cg_id_registry_singleton = nullptr;
	}
	if (cg_system_catalog_singleton) {
		memdelete(cg_system_catalog_singleton);
		cg_system_catalog_singleton = nullptr;
	}
}
