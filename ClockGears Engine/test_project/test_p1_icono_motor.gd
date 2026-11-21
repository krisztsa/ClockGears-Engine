extends SceneTree

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _check(cond: bool, label: String) -> void:
	if cond:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: " + label)

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	call_deferred("_run_test")

func _process(_delta: float) -> bool:
	if Time.get_ticks_msec() - start_msec > 55000:
		print("TIMEOUT GUARD TRIGGERED")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count + 1])
		quit(1)
		return true
	return false

func _run_test() -> void:
	var ico_path := "C:/Proyectos/NEW MOTOR/ICONO/ClockGearsEngine_icon.ico"
	var preview_path := "C:/Proyectos/NEW MOTOR/ICONO/ClockGearsEngine_icon_preview.png"
	var orig_path := "C:/Proyectos/NEW MOTOR/ICONO/CGE_NEW_ICONO_.png"
	var linux_png := "C:/Proyectos/NEW MOTOR/ClockGears Engine/misc/logo/icon.png"
	var app_png := "C:/Proyectos/NEW MOTOR/ClockGears Engine/main/app_icon.png"
	var def_svg_path := "C:/Proyectos/NEW MOTOR/ClockGears Engine/editor/icons/DefaultProjectIcon.svg"

	_check(FileAccess.file_exists(ico_path), ".ico nuevo existe")
	_check(FileAccess.file_exists(preview_path), "Hoja de vista previa del .ico existe")
	_check(FileAccess.file_exists("C:/Proyectos/NEW MOTOR/ICONO/ClockGearsEngineIcon_antiguo.ico"), "ICO antiguo renombrado con _antiguo")
	_check(FileAccess.file_exists("C:/Proyectos/NEW MOTOR/ICONO/ClockGearsEngineIcon_antiguo.png"), "PNG antiguo renombrado con _antiguo")

	# Verificar tamaño y forma cuadrada del PNG original
	var orig_img := Image.load_from_file(orig_path)
	_check(orig_img != null and orig_img.get_width() == 32 and orig_img.get_height() == 32, "Original CGE_NEW_ICONO_.png es 32x32 cuadrado")

	# Verificar los 7 tamaños dentro del .ico binario (16, 24, 32, 48, 64, 128, 256)
	var fa := FileAccess.open(ico_path, FileAccess.READ)
	_check(fa != null, "Lectura binaria .ico OK")
	if fa != null:
		var reserved := fa.get_16()
		var itype := fa.get_16()
		var count := fa.get_16()
		_check(reserved == 0 and itype == 1 and count == 7, ".ico tiene exactamente 7 entradas (got %d)" % count)
		var found_sizes: Array[int] = []
		for _i in range(count):
			var bw := fa.get_8()
			var bh := fa.get_8()
			var _pal := fa.get_8()
			var _resv := fa.get_8()
			var _planes := fa.get_16()
			var bpp := fa.get_16()
			var _sz := fa.get_32()
			var _off := fa.get_32()
			var w := 256 if bw == 0 else bw
			var h := 256 if bh == 0 else bh
			_check(w == h and bpp == 32, "Entrada ICO cuadrada 32bpp (%dx%d)" % [w, h])
			found_sizes.append(w)
		for req in [16, 24, 32, 48, 64, 128, 256]:
			_check(found_sizes.has(req), ".ico incluye tamaño %d px" % req)

	# Verificar PNG Linux (256x256), app_icon (128x128) y DefaultProjectIcon.svg
	var limg := Image.load_from_file(linux_png)
	_check(limg != null and limg.get_width() == 256 and limg.get_height() == 256, "Linux PNG es 256x256")
	var aimg := Image.load_from_file(app_png)
	_check(aimg != null and aimg.get_width() == 128 and aimg.get_height() == 128, "app_icon.png es 128x128")
	var svg_txt := FileAccess.get_file_as_string(def_svg_path)
	_check(svg_txt.contains("shape-rendering=\"crispEdges\"") and svg_txt.contains("viewBox=\"0 0 32 32\""), "DefaultProjectIcon.svg es SVG 32x32 crispEdges")
	_check(FileAccess.get_sha256(def_svg_path) != "9fea9f6c539dbb932419a16655e15c2bc628a18dbefb0db96e03aa33f586683d", "DefaultProjectIcon.svg ya NO es el icono antiguo")

	# Verificar proyecto nuevo nace con el icono nuevo vía diálogo real ClockGearsNewProjectDialog
	var tmp_parent := "C:/Proyectos/NEW MOTOR/ClockGears Engine/test_project/_tmp_p1_parent"
	var tmp_proj_name := "NuevoProyectoIcono"
	var tmp_proj_dir := tmp_parent.path_join(tmp_proj_name)
	_rm_recursive(tmp_parent)
	DirAccess.make_dir_recursive_absolute(tmp_parent)

	var npd := ClockGearsNewProjectDialog.new()
	root.add_child(npd)
	var edits := npd.find_children("*", "LineEdit", true, false)
	_check(edits.size() >= 2, "ClockGearsNewProjectDialog tiene campos de nombre y ruta")
	if edits.size() >= 2:
		(edits[0] as LineEdit).text = tmp_proj_name
		(edits[1] as LineEdit).text = tmp_parent
	for chk in npd.find_children("*", "CheckBox", true, false):
		(chk as CheckBox).button_pressed = false
	npd.emit_signal("confirmed")
	await process_frame

	var proj_cfg := FileAccess.get_file_as_string(tmp_proj_dir.path_join("project.godot"))
	var proj_icon := FileAccess.get_file_as_string(tmp_proj_dir.path_join("icon.svg"))
	_check(proj_cfg.contains("config/icon=\"res://icon.svg\""), "Proyecto nuevo apunta a res://icon.svg")
	_check(FileAccess.file_exists(tmp_proj_dir.path_join("icon.svg")) and proj_icon == svg_txt, "Proyecto nuevo nace con el DefaultProjectIcon.svg nuevo")

	# Verificar también plantillas de exportación (godot.ico y godot_console.ico) y que los ejecutables incrustan los 7 tamaños del icono nuevo
	var ico_sha := FileAccess.get_sha256(ico_path)
	_check(FileAccess.get_sha256("C:/Proyectos/NEW MOTOR/ClockGears Engine/platform/windows/godot.ico") == ico_sha, "godot.ico de export template es el nuevo .ico")
	_check(FileAccess.get_sha256("C:/Proyectos/NEW MOTOR/ClockGears Engine/platform/windows/godot_console.ico") == ico_sha, "godot_console.ico de export template es el nuevo .ico")

	# Verificar que un proyecto legado con PNG antiguo conserva su archivo intacto (SHA256 conocido para reemplazo visual en ProjectList)
	var old_png_path := "C:/Proyectos/NEW MOTOR/ICONO/ClockGearsEngineIcon_antiguo.png"
	_check(FileAccess.get_sha256(old_png_path) == "3bbc01c14134041a4184acd8244b432a008ad5966d10af74e47656267c798358", "ClockGearsEngineIcon_antiguo.png conserva SHA256 de detección en ProjectList")

	for exe_path in [
		"C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/clockgears.windows.editor.x86_64.exe",
		"C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/clockgears.windows.editor.x86_64.console.exe",
	]:
		var out_lines := []
		var ec := OS.execute("C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/cg_verify_exe_res.exe", [exe_path], out_lines, true)
		var joined := "\n".join(out_lines)
		_check(ec == 0 and joined.contains("EXE_ICON_COUNT=7"), "Recurso PE GODOT_ICON con 7 tamaños en " + exe_path.get_file())
		for req_s in ["16x16", "24x24", "32x32", "48x48", "64x64", "128x128", "256x256"]:
			_check(joined.contains(req_s), "EXE %s incrusta %s" % [exe_path.get_file(), req_s])

	npd.queue_free()
	await process_frame
	_rm_recursive(tmp_parent)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)

func _rm_recursive(p_dir: String) -> void:
	var da := DirAccess.open(p_dir)
	if da == null:
		return
	da.list_dir_begin()
	var n := da.get_next()
	while n != "":
		if n != "." and n != "..":
			var full := p_dir.path_join(n)
			if da.current_is_dir():
				_rm_recursive(full)
				DirAccess.remove_absolute(full)
			else:
				DirAccess.remove_absolute(full)
		n = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(p_dir)
