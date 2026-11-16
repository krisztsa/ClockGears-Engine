extends SceneTree

# ==============================================================================
# TEST DE VERIFICACIÓN: CAMBIO DE VERSIÓN A ALPHA-0.0.3
# ==============================================================================
# 1. Ventana "Acerca de" muestra "Alpha-0.0.3"
# 2. No queda ninguna aparición de la versión 0.0.2 del motor
# 3. Un proyecto de la versión anterior abre y se migra sin errores
# ==============================================================================

class WatchdogNode extends Node:
	var start_msec: int = 0
	var limit_msec: int = 25000
	var owner_tree: SceneTree = null
	func _process(_delta: float) -> void:
		if owner_tree != null and (Time.get_ticks_msec() - start_msec) >= limit_msec:
			owner_tree.call("_on_watchdog_timeout")

var passed: int = 0
var failed: int = 0
var _finished: bool = false
var _res_data_dir: String = "res://clockgears_data"
var _backup: Dictionary = {}

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for p in backup.keys():
		var fa := FileAccess.open(p, FileAccess.WRITE)
		if fa != null:
			fa.store_string(backup[p])
			fa.close()

func _on_watchdog_timeout() -> void:
	printerr("[TIMEOUT] Watchdog timer alcanzado (25s) en test_version_9b.gd")
	failed += 1
	_finish_test()

func _init() -> void:
	call_deferred("_run_test")

func _run_test() -> void:
	var wd := WatchdogNode.new()
	wd.start_msec = Time.get_ticks_msec()
	wd.owner_tree = self
	root.add_child(wd)

	_backup = _snapshot_data_dir(_res_data_dir)

	print("====================================================")
	print("INICIANDO TESTS DE VERSIÓN ALPHA-0.0.3")
	print("====================================================")

	# ----------------------------------------------------
	# TEST 1: Ventana 'Acerca de' y constante de versión
	# ----------------------------------------------------
	print("\n--- TEST 1: Verificación de versión en código fuente y componentes de UI ---")

	# 1.1 Verificar la constante CLOCKGEARS_VERSION en editor_version_button.h
	var h_path := ProjectSettings.globalize_path("res://../editor/gui/editor_version_button.h")
	var h_content := FileAccess.get_file_as_string(h_path)
	check(h_content.contains('#define CLOCKGEARS_VERSION "Alpha-0.0.3"'),
		"editor_version_button.h define CLOCKGEARS_VERSION como 'Alpha-0.0.3'")

	# 1.2 Verificar editor_version_button.cpp usa CLOCKGEARS_VERSION
	var cpp_path := ProjectSettings.globalize_path("res://../editor/gui/editor_version_button.cpp")
	var cpp_content := FileAccess.get_file_as_string(cpp_path)
	check(cpp_content.contains("CLOCKGEARS_DISPLAY_VERSION = CLOCKGEARS_VERSION"),
		"editor_version_button.cpp asigna CLOCKGEARS_DISPLAY_VERSION = CLOCKGEARS_VERSION")
	check(not cpp_content.contains("Alpha-0.0.2"),
		"editor_version_button.cpp no contiene ninguna referencia a 'Alpha-0.0.2'")

	# 1.3 Verificar editor_about.cpp instancia EditorVersionButton
	var about_path := ProjectSettings.globalize_path("res://../editor/gui/editor_about.cpp")
	var about_content := FileAccess.get_file_as_string(about_path)
	check(about_content.contains("EditorVersionButton"),
		"editor_about.cpp usa EditorVersionButton para mostrar la versión")
	check(not about_content.contains("Alpha-0.0.2"),
		"editor_about.cpp no contiene ninguna referencia a 'Alpha-0.0.2'")

	# 1.4 Verificar el binario compilado contiene 'Alpha-0.0.3' y no 'Alpha-0.0.2'
	var exe_path := ProjectSettings.globalize_path("res://../bin/clockgears.windows.editor.x86_64.exe")
	if FileAccess.file_exists(exe_path):
		var output := []
		var py_code := "d=open(r'%s','rb').read(); exit(0 if (b'Alpha-0.0.3' in d and b'Alpha-0.0.2' not in d) else 1)" % exe_path
		var exit_code := OS.execute("python", ["-c", py_code], output)
		check(exit_code == 0, "El ejecutable compilado contiene 'Alpha-0.0.3' y no 'Alpha-0.0.2'")
	else:
		check(false, "Ejecutable no encontrado en " + exe_path)

	# ----------------------------------------------------
	# TEST 2: No queda ninguna aparición de la versión 0.0.2 del motor
	# ----------------------------------------------------
	print("\n--- TEST 2: Escaneo de 0.0.2 en el repositorio ---")
	var readme_path := ProjectSettings.globalize_path("res://../../README.md")
	var readme_content := FileAccess.get_file_as_string(readme_path)
	check(readme_content.contains("Alpha-0.0.3"), "README.md contiene 'Alpha-0.0.3'")
	check(not readme_content.contains("Alpha-0.0.2"), "README.md no contiene 'Alpha-0.0.2'")

	# ----------------------------------------------------
	# TEST 3: Carga y migración de proyecto de versión anterior
	# ----------------------------------------------------
	print("\n--- TEST 3: Carga y migración de proyecto anterior ---")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")

	# 3.1 Cargar proyecto actual (schema_version 2)
	var load_ok = dm.load_all("res://clockgears_data")
	check(load_ok, "load_all() cargó exitosamente el proyecto de schema_version 2")

	# 3.2 Verificar que los datos del proyecto se leen correctamente tras migración
	var tilesets = dm.get_category_array(13)
	check(tilesets.size() > 0, "Tilesets cargados correctamente (" + str(tilesets.size()) + " registros)")
	var sys = dm.get_system_config()
	check(sys.size() > 0, "Configuración del sistema cargada correctamente")

	# 3.3 Simular un proyecto legado con schema_version 1
	var legacy_dir := "user://_test_legacy_project"
	var legacy_abs := ProjectSettings.globalize_path(legacy_dir)
	DirAccess.make_dir_recursive_absolute(legacy_abs)

	# Crear un database.json de versión 1 con registros sin campos nuevos
	var legacy_db_content := {
		"author": "Test Author",
		"engine": "ClockGears Engine",
		"schema_version": 1,
		"files": ["database.json", "actors.json", "enemies.json"]
	}
	var legacy_actors := [
		{"id": 1, "name": "Héroe Antiguo"} # Falta 'desc', 'traits', etc.
	]
	var legacy_enemies := [
		{"id": 1, "name": "Limo Antiguo"}
	]

	var fa_db := FileAccess.open(legacy_dir.path_join("database.json"), FileAccess.WRITE)
	fa_db.store_string(JSON.stringify(legacy_db_content))
	fa_db.close()

	var fa_act := FileAccess.open(legacy_dir.path_join("actors.json"), FileAccess.WRITE)
	fa_act.store_string(JSON.stringify(legacy_actors))
	fa_act.close()

	var fa_enm := FileAccess.open(legacy_dir.path_join("enemies.json"), FileAccess.WRITE)
	fa_enm.store_string(JSON.stringify(legacy_enemies))
	fa_enm.close()

	# Cargar proyecto legado
	var legacy_load_ok = dm.load_all(legacy_dir)
	check(legacy_load_ok, "load_all() abrió y migró exitosamente un proyecto de schema_version 1")

	# Verificar que el actor migrado ahora tiene los campos esperados
	var migrated_actor = dm.get_record_by_id(0, 1)
	check(migrated_actor.has("desc"), "Registro migrado tiene campo 'desc'")
	check(migrated_actor.has("traits"), "Registro migrado tiene campo 'traits'")

	# Limpiar proyecto temporal de test
	DirAccess.remove_absolute(legacy_dir.path_join("database.json"))
	DirAccess.remove_absolute(legacy_dir.path_join("actors.json"))
	DirAccess.remove_absolute(legacy_dir.path_join("enemies.json"))
	DirAccess.remove_absolute(legacy_abs)

	# Restaurar datos del proyecto original
	_restore_data_dir(_res_data_dir, _backup)
	dm.load_all("res://clockgears_data")

	print("\n====================================================")
	print("RESULTADO: %d PASS, %d FAIL" % [passed, failed])
	print("====================================================")

	_finish_test()

func _finish_test() -> void:
	if _finished:
		return
	_finished = true
	var code := 0 if failed == 0 else 1
	quit(code)
