extends SceneTree

var pass_count := 0
var fail_count := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _init() -> void:
	print("=== TEST PUNTO 1: TOKENS DEL TEMA NEÓN COBRE ===")
	var th = Engine.get_singleton("ClockGearsTheme")
	check(th != null, "Singleton ClockGearsTheme disponible")

	for tid in ["dark", "brass"]:
		th.set_active_theme_id(tid)
		ClockGearsTheme.set_glow_effects_level(1) # Suave (por defecto)
		var tk: Dictionary = th.get_theme_tokens_dict(tid)
		var theme_res: Theme = th.get_theme_by_id(tid)

		var s_base: Color = tk.get("surface_base", Color())
		var s_panel: Color = tk.get("surface_panel", Color())
		var s_elev: Color = tk.get("surface_elevated", Color())
		var s_sunk: Color = tk.get("surface_sunken", Color())

		check(s_sunk.get_luminance() < s_base.get_luminance() and s_base.get_luminance() < s_panel.get_luminance() and s_panel.get_luminance() < s_elev.get_luminance(),
			"%s: jerarquía de superficies (sunken < base < panel < elevated)" % tid)

		var b_cop: Color = tk.get("border_copper", Color())
		var b_bri: Color = tk.get("border_copper_bright", Color())
		var b_sub: Color = tk.get("border_subtle", Color())
		check(b_bri.get_luminance() > b_cop.get_luminance(), "%s: border_copper_bright (~15%% más claro) > border_copper" % tid)
		check(b_sub.a > 0.10 and b_sub.a < 0.35, "%s: border_subtle con baja opacidad (a=%.2f)" % [tid, b_sub.a])

		var acc_neon: Color = tk.get("accent_neon", Color())
		check(acc_neon.get_luminance() > b_cop.get_luminance(), "%s: accent_neon más claro y saturado que el borde cobrizo" % tid)

		var halo_c: Color = tk.get("halo_color", Color())
		var halo_r := int(tk.get("halo_radius", 0))
		var trans_ms := int(tk.get("transition_ms", 0))
		check(halo_c.a >= 0.25 and halo_c.a <= 0.40, "%s: halo_color con opacidad entre 25%% y 40%% (a=%.2f)" % [tid, halo_c.a])
		check(halo_r >= 6 and halo_r <= 10, "%s: halo_radius entre 6 y 10 px (%d px)" % [tid, halo_r])
		check(trans_ms >= 120 and trans_ms <= 150, "%s: transition_ms entre 120 y 150 ms (%d ms)" % [tid, trans_ms])

		# Comprobar que están registrados dentro del recurso Theme ("ClockGears")
		check(theme_res.has_color("surface_base", "ClockGears") and theme_res.has_color("surface_panel", "ClockGears") and theme_res.has_color("surface_elevated", "ClockGears") and theme_res.has_color("surface_sunken", "ClockGears"),
			"%s: tokens de superficie registrados en Theme" % tid)
		check(theme_res.has_color("border_copper", "ClockGears") and theme_res.has_color("border_copper_bright", "ClockGears") and theme_res.has_color("border_subtle", "ClockGears") and theme_res.has_color("accent_neon", "ClockGears") and theme_res.has_color("halo_color", "ClockGears"),
			"%s: tokens de borde, acento neón y halo registrados en Theme" % tid)
		check(theme_res.has_constant("halo_radius", "ClockGears") and theme_res.has_constant("transition_ms", "ClockGears") and theme_res.has_constant("corner_radius", "ClockGears"),
			"%s: constantes halo_radius, transition_ms y corner_radius registradas en Theme" % tid)

	# Comprobar que el tema Oscuro y el tema Latón tienen sus propios colores diferenciados
	var tk_dark: Dictionary = th.get_theme_tokens_dict("dark")
	var tk_brass: Dictionary = th.get_theme_tokens_dict("brass")
	check(Color(tk_dark["surface_panel"]) != Color(tk_brass["surface_panel"]) and Color(tk_dark["accent_neon"]) != Color(tk_brass["accent_neon"]),
		"El tema Latón usa sus colores propios diferenciados del tema Oscuro")

	th.set_active_theme_id("dark")
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
