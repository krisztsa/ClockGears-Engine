extends SceneTree

var pass_count := 0
var fail_count := 0
var _start_msec := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _wait_frames(n: int) -> void:
	for i in range(n):
		if Time.get_ticks_msec() - _start_msec > 55000:
			printerr("INTERNAL TIMEOUT 55s")
			_finish(1)
			return
		await process_frame

func _finish(forced_code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	if forced_code >= 0:
		quit(forced_code)
	else:
		quit(0 if fail_count == 0 else 1)

func _collect_buttons(n: Node, out: Array) -> void:
	if n == null:
		return
	if n is Button:
		out.append(n)
	for ch in n.get_children():
		_collect_buttons(ch, out)

func _initialize() -> void:
	_start_msec = Time.get_ticks_msec()
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s")
		_finish(1)
	)
	call_deferred("_run_test")

func _run_test() -> void:
	var ci = Engine.get_singleton("ClockGearsIcons")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(ci != null and dm != null, "Singletons ClockGearsIcons y DataManager disponibles")
	if ci == null or dm == null:
		_finish(1)
		return
	dm.load_all()

	# 1. Todos los 15 PNGs nuevos incrustados, tamaño 32x32 y escalado entero exacto
	var new_pngs := [
		"accesorios", "actores_npc_npc_arpg", "armaduras_armadurasarpg",
		"clases", "copiar", "cortar", "enemigos_enemigosarpg", "escudosarpg",
		"eventoscomunes_interruptores", "fauna", "flora", "pegar",
		"pincel_de_sombra", "tiles_icon", "tropas"
	]
	for p in new_pngs:
		var has_p: bool = ci.has_png_icon(p)
		var sz_i: Vector2i = ci.get_png_icon_size(p)
		var tex1: Texture2D = ci.get_icon(p, 1.0)
		var tex2: Texture2D = ci.get_icon(p, 2.0)
		_check(has_p and sz_i == Vector2i(32, 32) and tex1 != null and tex1.get_size() == Vector2(32, 32) and tex2 != null and tex2.get_size() == Vector2(64, 64),
			"PNG '%s' incrustado 32x32 y escala entera 2x -> 64x64" % p)

	# 2. Botones de categorias de la Base de Datos tienen icono, filtro NEAREST y sin tinte
	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	db.size = Vector2i(1366, 768)
	root.add_child(db)
	db.open_database()
	await _wait_frames(3)

	var all_btns := []
	_collect_buttons(db, all_btns)
	var target_cat_names := [
		"Actores", "Clases", "Armaduras", "Accesorios", "Enemigos", "Tropas",
		"NPC", "Fauna", "Flora", "Tilesets", "Eventos Comunes", "Interruptores",
		"NPC ARPG", "Enemigos ARPG", "Armaduras ARPG", "Escudos ARPG", "Accesorios ARPG"
	]
	var found_cats := {}
	for b in all_btns:
		var btn: Button = b
		if btn.text in target_cat_names:
			found_cats[btn.text] = btn

	for cname in target_cat_names:
		var cb: Button = found_cats.get(cname, null) as Button
		var ic_ok := cb != null and cb.icon != null and cb.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST
		var no_tint := cb != null and cb.get_theme_color("icon_normal_color") == Color(1, 1, 1, 1)
		_check(ic_ok and no_tint, "Categoría BD '%s' tiene icono propio NEAREST sin tinte" % cname)

	# 3. Botones de la barra superior (Cortar, Copiar, Pegar, Pincel de Sombra y demás)
	var top_bar := ClockGearsTopBar.new()
	root.add_child(top_bar)
	await _wait_frames(2)

	var audit: Array = top_bar.get_toolbar_icons_audit()
	_check(audit.size() >= 18, "ClockGearsTopBar tiene todos sus botones (%d)" % audit.size())
	var checked_tips := {}
	for entry in audit:
		if typeof(entry) != TYPE_DICTIONARY:
			continue
		var tip := str(entry.get("tooltip", ""))
		var pn := str(entry.get("png_name", ""))
		var has_ic := bool(entry.get("has_icon", false))
		var flt := int(entry.get("filter", -1))
		if "Cortar" in tip or "Copiar" in tip or "Pegar" in tip or "Pincel de Sombra" in tip:
			checked_tips[pn] = has_ic and flt == CanvasItem.TEXTURE_FILTER_NEAREST
		_check(has_ic, "TopBar botón '%s' (%s) tiene icono cargado" % [tip, pn])

	for need_pn in ["cortar", "copiar", "pegar", "pincel_de_sombra"]:
		_check(bool(checked_tips.get(need_pn, false)), "TopBar PNG nuevo '%s' activo con filtro NEAREST" % need_pn)

	top_bar.queue_free()
	db.queue_free()
	await _wait_frames(1)
	_finish()
