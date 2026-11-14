⚙️ ClockGears Engine:

⚠️ AVISO IMPORTANTE SOBRE GRATUIDAD Y ESTAFAS ClockGears Engine es completamente gratuito. Se creó para que cualquiera pueda desarrollar videojuegos RPG sin pagar licencias ni suscripciones. Solo existe una opción voluntaria de donaciones, fuera del programa. Si alguien te cobró por este motor, has sido estafado. La única vía oficial de descarga es este GitHub.

📖 ¿Qué es ClockGears Engine?

ClockGears Engine es un motor de videojuegos 2D para RPG por rejillas, con un editor visual inspirado en RPG Maker MZ. Está construido en C++ modificando directamente el código fuente de Godot Engine, y avanza fase a fase para convertirse en un motor con identidad propia.

Su objetivo es que puedas crear un RPG completo sin programar los sistemas desde cero y sin pagar: mapas, eventos, combates, base de datos, animaciones y pruebas de juego vienen integrados en el mismo programa, con una interfaz pensada para no estorbar.

✅ ¿Ya se puede usar?

Sí. En su estado actual (Alpha 0.0.2), el motor ya permite hacer RPG sencillos de principio a fin: diseñar los mapas, programar la historia con eventos, definir personajes, objetos y enemigos, combatir por turnos, probar el juego con F5 y exportarlo. Al ser una versión Alpha, todavía hay cosas por pulir y funciones por llegar (consulta la hoja de ruta), así que conviene guardar copias de tus proyectos.

✨ Qué incluye
🗺️ Mapas y editor
Flujo de trabajo estilo RPG Maker MZ: árbol de mapas, propiedades de mapa y edición directa sobre cuadrícula.
Paleta de tiles con autotiles: pintado por capas, tilesets importables y creación de mapas eligiendo su tileset.
Pasabilidad de tiles: marca cada tile como transitable, bloqueado o dibujado por encima del jugador.
Pruebas rápidas con F5, con punto de inicio del jugador configurable.
🎬 Eventos
Editor de eventos integrado, sin escribir código: varias páginas por evento, condiciones por interruptores (Switches) y variables, rutas de movimiento, diálogos, elecciones, teletransportes y control de flujo.
Efectos de pantalla: mostrar imágenes, tintes de color y temblor de pantalla.
⚔️ Combate por turnos
Comando de evento "Iniciar Batalla" con tropa, fondo y condiciones de victoria, derrota y huida.
Orden de turnos por AGI, con los comandos Atacar, Habilidad, Objeto, Defender y Huir.
Fórmulas de daño, estados alterados, 10 elementos con debilidades y resistencias, EXP, oro, drops y subidas de nivel.
Panel de depuración (F3) para revisar la batalla en tiempo real.
Editor de tropas con vista previa 16:9, arrastre de enemigos, fondo y suelo ajustables, escala por enemigo y prueba de batalla directamente desde el editor.
🗃️ Base de datos
Gestión centralizada de actores, clases, habilidades, objetos, armas, armaduras, accesorios, enemigos, tropas, estados, animaciones, tilesets, flora, fauna y eventos comunes, además de tipos, vocabulario y ajustes del sistema.
Organizada por capítulos, con ventana redimensionable y sin cortes ni scroll horizontal.
🎞️ Animaciones de sprites
Editor de animaciones por ranuras con nombre: Idle, Ataque, Daño, Muerte y Buff, más lanzamientos libres ligados a un elemento.
Corte de spritesheets en celdas, orden de frames, FPS, bucle, orden aleatorio y botón de prueba.
🎨 Identidad visual propia
Iconos de pixel art dibujados a mano para el editor y la base de datos, escalados en múltiplos enteros y sin filtros.
Interfaz oscura con esquinas redondeadas, pensada para dar aire y mantener cada herramienta legible.
🚀 Rendimiento
Nativo en C++: ligero, rápido y compatible con gráficas integradas y dedicadas mediante OpenGL 3 / Vulkan.
📥 Descarga

Si solo quieres usar el motor, descarga el ejecutable desde la sección de Releases. No necesitas instalar nada más.

🛠️ Cómo compilar el motor

Compilar el motor es posible, siempre dentro de lo que permite la licencia. Necesitas Python 3.8+, SCons (pip install scons) y un compilador C++ (MSVC o MinGW-w64):

bash
scons platform=windows target=editor arch=x86_64 -j8
📜 Licencia

ClockGears Engine tiene una licencia propia para lo creado por ClockGears Studio, y conserva la licencia MIT del código de Godot Engine.

✅ Permitido, sin pedir permiso:

Usar el editor para crear juegos y distribuirlos, venderlos o publicarlos, incluido el runtime que forma parte de las exportaciones.
Sin regalías, sin pagos y sin suscripciones a ClockGears Studio. Los juegos pertenecen a sus creadores.

⛔ No permitido sin permiso escrito de Kris'ZtS:

Copiar, redistribuir, vender, sublicenciar o publicar el editor ClockGears Engine, su código fuente o versiones modificadas, en todo o en parte de lo creado por ClockGears Studio.

🧩 Código de Godot Engine:

Todo lo que proviene de Godot sigue bajo la licencia MIT, con su aviso de copyright original (© 2014-presente Godot Engine contributors, © 2007-2014 Juan Linietsky, Ariel Manzur). Esto no limita los derechos sobre el código de Godot.

El texto completo está en el archivo de licencia del repositorio, y también dentro del editor, en Ayuda → Acerca de → Licencia. Las licencias de las librerías de terceros aparecen en la pestaña Licencia de Terceros.

© 2026 Kris'ZtS / ClockGears Studio. Todos los derechos reservados sobre las modificaciones y el código original de ClockGears Studio.

🤝 Estado del proyecto y créditos
Creado por Kris'ZtS (ClockGears Studio), con asistencia de Claude, sobre la base de código de Godot Engine.
Hoja de ruta: el desarrollo avanza por fases hacia la versión 1.0, y puede cambiar, ampliarse o reajustarse sobre la marcha.
GitHub oficial: https://github.com/krisztsa/ClockGears-Engine
Canal de YouTube: https://www.youtube.com/@KrisZtS

Creado en 2026.

ClockGears Engine es gratuito y solo tiene una opción voluntaria de donaciones, fuera del programa.
**Leyenda:** ✅ Hecha · 🔧 En curso · ⏳ Planeada · 🚀 **Update** (hito grande con mejoras masivas)

---

## Arco 1: Núcleo del motor (Fases 1–11)

| # | Fase | Qué trae |
|---|------|----------|
| 1 | Base y rebranding ✅ | Rebranding completo a ClockGears Engine · créditos de Kris'ZtS en "Acerca de" · icono propio del ejecutable · compilación aislada en un solo exe · banderas de compilación reducidas |
| 2 | Interfaz MZ y tema Dark ✅ | Barra superior de accesos rápidos · tema ClockGears Dark · iconos SVG pixel art · selectores (pickers) · validación en vivo |
| 3 | Mapas y rejilla ✅ | Rejillas de 16, 32 y 48 · "Salir de la casilla" (colocación libre) · importar imágenes al mapa · árbol de mapas · deshacer y rehacer · herramientas de dibujo |
| 4 | Eventos sin código ✅ | Editor de eventos estilo MZ · páginas, condiciones y disparadores · lista de comandos editable · interruptores y variables · pestaña Code opcional |
| 5 | Base de datos ✅ | Actores, clases, habilidades, objetos, armas, armaduras y accesorios · enemigos, tropas y estados · importar y exportar assets · selectores con pestañas y búsqueda |
| 6 | Tilesets y colisiones ✅ | 9 ranuras (A1–A5, B, C, D, E) · máscara ○ pasable, × bloqueado, ★ pasa por detrás · 4 direcciones, arbusto, mostrador, escalera, daño y terreno 0–7 · selector de tileset por mapa · paleta por pestañas |
| 7 | Sistema narrativo ✅ | Ventana de mensajes · códigos de escape · retratos con expresiones · caja de nombre · opciones y entrada de número · historial · modos AUTO y SALTAR · vista previa en vivo |
| 8 | Combate por turnos, iconos y animaciones ✅ | **Combate:** comando "Iniciar Batalla" · orden de turnos por AGI · Atacar, Habilidad, Objeto, Defender y Huir · fórmulas de daño, estados, EXP, oro y drops · subidas de nivel y resumen de recompensas · panel de depuración F3<br>**Enemigos:** nivel, parámetros, drops, debilidades y resistencias por tasa elemental con 10 elementos fijos (agua, fuego, hielo, electricidad, viento, luz, oscuridad, astral, tierra y naturaleza) · rasgos y patrones de acción<br>**Animaciones:** editor de sprites con ranuras fijas (Idle, Ataque, Daño, Muerte y Buff) y lanzamientos libres por elemento · corte en celdas, orden de frames, FPS, bucle, orden aleatorio y prueba · migración automática de datos antiguos<br>**Tropas:** vista previa de colocación 16:9 con arrastre y rejilla de 32 px · fondo y suelo por tropa con ventana "Ajustar" (Cubrir, Ajustar, Estirar y Mosaico, escala entera para pixel art) · escala por enemigo de x1 a x6 con menú contextual · Alinear, franja del panel de combate y prueba de batalla desde el editor<br>**Base de Datos nueva:** capítulos en pestañas y dos columnas, sin cortes ni scroll horizontal · ventana al 90% de la pantalla, redimensionable y que recuerda su tamaño · OK y Aplicar · icono de cada elemento con su propio gráfico · selector de tiles que guarda la celda<br>**Identidad visual:** iconos de pixel art dibujados a mano para el editor y la Base de Datos, escalados en múltiplos enteros según la pantalla · tema con botones y pestañas redondeados · todo pensado para alejar al motor de sus cimientos y darle una identidad propia |
| 9A | Combate en tiempo real (ARPG) 🔧 | Movimiento en 4 u 8 direcciones a elegir · animaciones por ranuras con nombre (caminar, correr, atacar por dirección) · editor de hitboxes y hurtboxes por frame con maniquí de referencia · combate en tiempo real estilo Zelda · herramientas (hacha, pico, pala, azada) · habilidades físicas con recurso propio |
| 9B | 🚀 **Magic Update** ⏳ | Base de datos por capítulos (Magia, Equipo, Objetos…) · hechizos con 10 elementos y 8 estados · grimorios por tipo con hechizos por nivel · báculos y objetos canalizadores · barra de acceso rápido 1–9 · reglas de equipo configurables y modo "Lv 100" en F3 |
| 10 | Menús del juego y menú principal ⏳ | Menús estándar del juego (objetos, equipo, estado, guardar) · flujo de menú principal (título, nuevo juego, cargar, opciones, salir) |
| 11 | Exportación y empaquetado de la primera etapa ⏳ | Exportación simplificada en 1 o 2 clics · empaquetado final de la primera etapa · licencias incluidas en el paquete |

---

## Arco 2: RPG completo (Fases 12–25)

| # | Fase | Qué trae |
|---|------|----------|
| 12 | Animaciones II y cierre de combate | Duración por frame y orden aleatorio · triggers de ataque, daño y muerte en Actores y Enemigos · sonidos y flashes por fotograma · tropas con varios enemigos y posiciones · regresión completa del combate |
| 13 | Eventos II | Ramas anidadas (elecciones y condiciones) · bucles, etiquetas y saltos · copiar, pegar y buscar comandos · comentarios · eventos comunes |
| 14 | Eventos III | Rutas de movimiento · seguidores del grupo · control de cámara y scroll · tintes, sacudidas y flashes · imágenes en pantalla |
| 15 | Mapas II | Autotiles A1–A4 completos · regiones · capas de tiles y eventos · copiar y exportar mapas · transiciones y teletransportes |
| 16 | Combate II | Resistencias y debilidades por elemento y estado · críticos, evasión y fallo · objetivos múltiples y aleatorios · estados avanzados · editor de fórmulas con validación |
| 17 | Combate III | Costes, recargas y tipos de habilidad · pasivas y rasgos · habilidades encadenadas · invocaciones · contraataque y cobertura · IA enemiga por patrones y condiciones |
| 18 | Combate IV | Jefes con fases · eventos dentro de la batalla · emboscadas y huida configurables · vistas frontal y lateral · fondos por mapa o región · recompensas especiales |
| 19 | 🚀 **Cronómetro Update** | Sistemas de turno a elegir (clásico, ATB, ticks) · simulador de batallas con balance automático · curvas de nivel y dificultad · cola de turnos visual · auto-batalla |
| 20 | Progresión | Clases y multiclase · árboles de habilidades · slots de equipo configurables · curvas de EXP por actor · ascensos |
| 21 | Inventario y economía | Tiendas y su editor · crafteo con recetas · peso y límites · objetos clave · monedas múltiples |
| 22 | Sistema de juego | Menú principal editable · estado, formación y equipo · guardado en ranuras con captura · opciones de dificultad · créditos |
| 23 | Misiones y diario | Misiones con objetivos · diario y bestiario · enciclopedia de objetos · marcadores en el mapa · recompensas |
| 24 | Automatización | Presets de movimiento en 2, 4, 6 y 8 direcciones · puertas, cofres y teletransportes listos · NPC con ruta · plantillas de eventos comunes · auto-conexión sin configurar inputs |
| 25 | 🚀 **Crónicas Update** | Ciclo de día y noche · clima por mapa · calendario y tiempo del mundo · horarios de NPC · afinidad con personajes · logros |

---

## Arco 3: Motor visual (Fases 26–40)

| # | Fase | Qué trae |
|---|------|----------|
| 26 | Render base | Orden por Y y capas de profundidad · sombras de tiles · luces 2D puntuales · mapas de normales · máscaras de luz |
| 27 | Partículas I | Editor visual WYSIWYG · curvas y gradientes · presets (fuego, chispas, humo, magia) · vista previa en vivo · importar y exportar |
| 28 | Partículas II | Comando de evento "Mostrar partículas" · partículas en combate y habilidades · colisión con tiles · estelas y sub-emisores · límites de rendimiento |
| 29 | 🚀 **Péndulo Update** | Biblioteca de efectos prehechos · lluvia, nieve y niebla · hechizos con partículas y luz · shaders listos · editor de efectos unificado |
| 30 | Post-proceso | Bloom · viñeta y aberración cromática · grano y pixelado · corrección de color con LUT · pila de efectos por mapa |
| 31 | Cámara 2.5D | Perspectiva inclinada · rotación por pasos de 90° · sprites billboard · zoom y seguimiento suave · presets de cámara |
| 32 | Mapas 2.5D | Tiles con altura · escaleras y desniveles · techos que se ocultan · objetos 3D low poly (glTF) · parallax en capas |
| 33 | Luz y sombras | Sombras proyectadas de sprites · luces dinámicas y emisivos · reflejos en agua · día y noche con luz coherente · lightmaps horneados opcionales |
| 34 | Profundidad de campo | DoF y tilt-shift · bokeh · enfoque controlado por eventos · cámara cinematográfica · desenfoque de fondo en diálogos |
| 35 | 🚀 **Diorama Update** | Estilo propio **Diorama 2.5D** (pixel art sobre escenarios 3D) · plantilla lista · presets de cámara, luz y DoF · partículas integradas · mapa demo |
| 36 | HDR | Pipeline HDR · tonemapping (ACES, Filmic) · exposición automática · salida a pantallas HDR · calibración dentro del juego |
| 37 | FSR | Escalado FSR · resolución interna ajustable · selector de calidad · nitidez ajustable · alternativa sin soporte |
| 38 | DLSS y XeSS | DLSS mediante el SDK de NVIDIA · XeSS de Intel · detección de hardware · desactivación automática si no hay soporte · ajustes en el menú |
| 39 | Rendimiento | Perfilador integrado · culling y batching · atlas de texturas · presets Bajo, Medio y Alto · límite de FPS y VSync |
| 40 | 🚀 **Cristal de Cuarzo Update** | Panel único de calidad gráfica · benchmark automático · modo foto · captura y grabación · paquete final de shaders |

---

## Arco 4: Interfaz y editor (Fases 41–47)

| # | Fase | Qué trae |
|---|------|----------|
| 41 | HUD I | Editor WYSIWYG con arrastrar y soltar · barras de vida y MP · ventanas con anclas · rejillas y alineación · vista previa en varias resoluciones |
| 42 | HUD II | Skins de ventana 9-patch · fuentes bitmap · animaciones de UI · cursores y sonidos de menú · estados (normal, hover, presionado) |
| 43 | HUD III | Plantillas de título, menú, tienda y batalla · widgets reutilizables · enlaces a datos del juego · pantallas modales · pruebas de resolución |
| 44 | Personalización del editor | Paleta de 8 colores de acento · tema Retro Pixelart · tema Dark OLED · tema Classic JRPG Blue · preferencias guardadas |
| 45 | Identidad y editor reorganizado | Iconos SVG y tipografía propios · logo, splash discreto e icono de ejecutable · paneles acoplables · búsqueda global · interfaz en español e inglés |
| 46 | Accesibilidad y aprendizaje | Tamaño de fuente y modos para daltonismo · remapeo de controles y mando · tutorial interactivo · documentación dentro del editor · plantillas comentadas |
| 47 | 🚀 **Relojería Update** | Limpieza visual casi total del origen Godot · nombres internos y menús propios · tema final · modo ventana pequeña · revisión de cada pantalla |

---

## Arco 5: Audio, código y cine (Fases 48–50)

| # | Fase | Qué trae |
|---|------|----------|
| 48 | Audio | Editor de BGM, BGS, ME y SE · bucles con puntos y crossfade · buses y volúmenes por categoría · sonido posicional y reverb por mapa · música adaptativa y ducking en diálogos |
| 49 | Código y datos | Pestaña Code con API estable · hooks y plugins · importar y exportar JSON y CSV · localización de textos · migraciones y copias de seguridad |
| 50 | 🚀 **Telón Update** | Editor de cinemáticas con línea de tiempo · pistas de cámara, diálogo, partículas y audio · cortes y transiciones · reproducción dentro del juego · exportar a vídeo |

---

## Arco 6: Plataformas (Fases 51–53)

| # | Fase | Qué trae |
|---|------|----------|
| 51 | Exportación PC | Windows y Linux en 1 o 2 clics · instalador · ejecutable con icono propio · compresión de assets · licencias incluidas en el paquete |
| 52 | Android | SDK y keystore automáticos · controles táctiles · pantalla adaptable · APK y AAB · pruebas en dispositivo |
| 53 | Web, macOS e integraciones | Build web · build macOS · paquete base para consolas · Steam e itch.io · guardado en la nube |

---

## Arco 7: Cierre (Fases 54–55)

| # | Fase | Qué trae |
|---|------|----------|
| 54 | Calidad y beta | Pruebas automáticas por pantalla · proyectos de prueba y de rendimiento · informe de regresión · manual completo · sistema de reportes con captura |
| 55 | 🚀 **Maestro Relojero 1.0 Update** | API congelada · documentación final · proyecto de ejemplo completo · web y página de descarga · lanzamiento |

---

## Notas

- **Estilo Diorama 2.5D:** es una técnica visual propia (pixel art sobre escenarios 3D). No se usan nombres, assets ni iconografía de otros juegos.
- **Licencia:** las modificaciones y el código original son de Kris'ZtS · ClockGears Studio (ver `LICENSE-CLOCKGEARS.txt`). El código de [Godot Engine](https://godotengine.org) que contiene se mantiene bajo su licencia MIT, con sus avisos originales conservados. Los juegos creados con el motor son de sus creadores, sin regalías ni suscripciones.
