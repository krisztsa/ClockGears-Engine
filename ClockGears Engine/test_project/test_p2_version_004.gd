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

func _bytes_contain_ascii(p_haystack: PackedByteArray, p_needle: String) -> bool:
	var n := p_needle.to_utf8_buffer()
	var nlen := n.size()
	var hlen := p_haystack.size()
	if nlen == 0 or hlen < nlen:
		return false
	var first := n[0]
	var i := 0
	while i <= hlen - nlen:
		var found_pos := p_haystack.find(first, i)
		if found_pos < 0 or found_pos > hlen - nlen:
			return false
		var match_all := true
		for j in range(1, nlen):
			if p_haystack[found_pos + j] != n[j]:
				match_all = false
				break
		if match_all:
			return true
		i = found_pos + 1
	return false

func _run_test() -> void:
	# 1. Runtime desde la única fuente de verdad (core/clockgears_version.h)
	_check(ClockGearsNewProjectDialog.get_engine_version() == "Alpha-0.0.4", "Versión oficial en runtime es Alpha-0.0.4")
	_check(ClockGearsNewProjectDialog.get_splash_version() == "v0.0.4 ALPHA", "Versión de pantalla de carga en runtime es v0.0.4 ALPHA")

	# 2. Única fuente de verdad y ausencia de Alpha-0.0.3 hardcodeada
	var ver_h := FileAccess.get_file_as_string("C:/Proyectos/NEW MOTOR/ClockGears Engine/core/clockgears_version.h")
	_check(ver_h.contains('#define CLOCKGEARS_VERSION "Alpha-0.0.4"'), "core/clockgears_version.h define Alpha-0.0.4")
	_check(ver_h.contains('#define CLOCKGEARS_SPLASH_VERSION "v0.0.4 ALPHA"'), "core/clockgears_version.h define v0.0.4 ALPHA")
	_check(ver_h.contains('#define CLOCKGEARS_FILE_VERSION_STR "0.0.4.0"'), "core/clockgears_version.h define 0.0.4.0")

	var ev_h := FileAccess.get_file_as_string("C:/Proyectos/NEW MOTOR/ClockGears Engine/editor/gui/editor_version_button.h")
	_check(ev_h.contains('#include "core/clockgears_version.h"') and not ev_h.contains("Alpha-0.0."), "editor_version_button.h importa core/clockgears_version.h sin versión duplicada")

	var ev_cpp := FileAccess.get_file_as_string("C:/Proyectos/NEW MOTOR/ClockGears Engine/editor/gui/editor_version_button.cpp")
	_check(ev_cpp.contains("CLOCKGEARS_VERSION") and not ev_cpp.contains("Alpha-0.0."), "editor_version_button.cpp usa CLOCKGEARS_VERSION (Acerca de y ProjectManager)")

	for rc_name in ["godot_res.rc", "godot_res_wrap.rc", "godot_res_template.rc", "godot_res_wrap_template.rc"]:
		var rc_txt := FileAccess.get_file_as_string("C:/Proyectos/NEW MOTOR/ClockGears Engine/platform/windows/" + rc_name)
		_check(rc_txt.contains('#include "core/clockgears_version.h"'), rc_name + " incluye core/clockgears_version.h")

	# 3. Propiedades PE reales de los binarios Windows (.exe y .console.exe)
	for exe_path: String in [
		"C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/clockgears.windows.editor.x86_64.exe",
		"C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/clockgears.windows.editor.x86_64.console.exe",
	]:
		var out_lines := []
		var ec := OS.execute("C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/cg_verify_exe_res.exe", [exe_path], out_lines, true)
		var joined := "\n".join(out_lines)
		var fname: String = exe_path.get_file()
		_check(ec == 0, "cg_verify_exe_res OK en " + fname)
		_check(joined.contains("FILEVERSION=0.0.4.0"), fname + " FILEVERSION=0.0.4.0")
		_check(joined.contains("PRODUCTVERSION=0.0.4.0"), fname + " PRODUCTVERSION=0.0.4.0")
		_check(joined.contains("ProductName=ClockGears Engine"), fname + " ProductName=ClockGears Engine")
		_check(joined.contains("CompanyName=ClockGears Studio"), fname + " CompanyName=ClockGears Studio")
		_check(joined.contains("FileDescription=ClockGears Engine Editor"), fname + " FileDescription=ClockGears Engine Editor")
		_check(joined.contains("LegalCopyright_U0=U+00A9") and joined.contains("2026 Kris'ZtS / ClockGears Studio"), fname + " LegalCopyright UTF-16LE exacto (© 2026 Kris'ZtS / ClockGears Studio)")
		_check(joined.contains("FileVersion=0.0.4.0"), fname + " FileVersion=0.0.4.0")
		_check(joined.contains("ProductVersion=0.0.4.0"), fname + " ProductVersion=0.0.4.0")

	# 4. Verificar binario compilado contiene Alpha-0.0.4 y NO Alpha-0.0.3
	var exe_bytes := FileAccess.get_file_as_bytes("C:/Proyectos/NEW MOTOR/ClockGears Engine/bin/clockgears.windows.editor.x86_64.exe")
	_check(_bytes_contain_ascii(exe_bytes, "Alpha-0.0.4"), "Binario compilado contiene Alpha-0.0.4")
	_check(not _bytes_contain_ascii(exe_bytes, "Alpha-0.0.3"), "Binario compilado ya NO contiene Alpha-0.0.3")

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
