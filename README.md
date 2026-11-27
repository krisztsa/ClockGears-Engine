<div align="center">

# ⚙️ ClockGears Engine

**Motor de videojuegos 2D para RPG y ARPG por rejillas — Gratuito, nativo en C++ y sin regalías.**

[![Última versión](https://img.shields.io/github/v/release/krisztsa/ClockGears-Engine?include_prereleases&label=Versión&color=d97706)](https://github.com/krisztsa/ClockGears-Engine/releases)
[![Descargas](https://img.shields.io/github/downloads/krisztsa/ClockGears-Engine/total?label=Descargas&color=b45309)](https://github.com/krisztsa/ClockGears-Engine/releases)
[![Precio](https://img.shields.io/badge/Precio-100%25%20Gratuito-16a34a)](https://github.com/krisztsa/ClockGears-Engine/releases)
[![Base](https://img.shields.io/badge/Base-C%2B%2B%20%2F%20Godot%20Fork-478cbf)](https://godotengine.org)
[![Meta 1.0](https://img.shields.io/badge/Meta%201.0-v1.0--2D--Master%20(Arco%202)-ea580c)](#-la-meta-v10-2d-master)
[![Progreso](https://img.shields.io/badge/Progreso-Fase%209B%20%2F%2055-c2410c)](#-hoja-de-ruta)
[![YouTube](https://img.shields.io/badge/YouTube-@KrisZtS-dc2626?logo=youtube&logoColor=white)](https://www.youtube.com/@KrisZtS)

<br>

[📥 Descargar](#-descarga) • [🎯 Propósito y v1.0](#-por-qué-existe-clockgears-engine) • [✨ Qué incluye](#-qué-incluye) • [🗺️ Hoja de Ruta](#-hoja-de-ruta) • [🛠️ Compilar](#️-cómo-compilar) • [📜 Licencia](#-licencia)

</div>

---

> [!WARNING]
> **AVISO SOBRE GRATUIDAD Y ESTAFAS:** ClockGears Engine es **completamente gratuito**. Se creó para que cualquiera pueda desarrollar videojuegos RPG sin pagar licencias ni suscripciones. Solo existe una opción voluntaria de donaciones fuera del programa. **Si alguien te cobró por este motor, has sido estafado.** La única vía oficial de descarga es este repositorio de GitHub.

## 📖 ¿Qué es ClockGears Engine?

**ClockGears Engine** es un motor de videojuegos 2D especializado en RPG y Action RPG por rejillas con vista cenital (*top-down*). Está construido en **C++** modificando directamente el código fuente de Godot Engine, combinando la inmediatez de un editor visual clásico con el rendimiento nativo de un motor moderno.

Su objetivo es que puedas diseñar y terminar un videojuego completo sin programar sistemas desde cero y sin pagar: mapas, eventos, combates por turnos y en tiempo real, base de datos, animaciones, menús y pruebas instantáneas vienen integrados de serie en un único programa.

* **Estado actual (`Alpha 0.0.3`):** Ya permite crear RPG y ARPG sencillos de principio a fin (mapas, eventos, personajes, combates, pruebas rápidas con `F5` y exportación). Al ser una versión Alpha, conviene guardar copias de seguridad de tus proyectos.

---

## 🎯 ¿Por qué existe ClockGears Engine?

Ante todo, **ClockGears Engine es una herramienta hecha por y para mí (Kris'ZtS)**. Nace de una necesidad real y muy concreta como desarrollador independiente: construir **mi motor definitivo y principal** para dar vida a todos mis futuros videojuegos bajo ClockGears Studio, agilizando al máximo la producción sin depender de licencias abusivas, suscripciones mensuales ni herramientas ajenas que te obligan a trabajar a contracorriente. En lugar de perder meses reprogramando desde cero un sistema de mapas, eventos, inventarios, combates o interfaces cada vez que nace una nueva idea, este proyecto concentra todo ese esfuerzo en una única base hecha a medida de mi propio flujo de trabajo.

Quien haya intentado crear un RPG o ARPG 2D en pixel art sin programar un motor desde cero conoce de sobra el muro con el que siempre se choca. Durante años, el estándar del mercado ha obligado a los creadores a elegir entre dos extremos frustrantes: o pasar meses escribiendo código para mover un personaje por una pantalla, o conformarse con herramientas tradicionales atrapadas en el pasado. Herramientas que, pese a venderse a precios elevados y relanzarse cada pocos años con cambios mínimos bajo un nombre nuevo, siguen ejecutando tus juegos dentro de un navegador web disfrazado que devora memoria RAM, da tirones al cargar mapas medianamente grandes y se ahoga en cuanto intentas añadir algo más ambicioso que un efecto básico. **ClockGears Engine rompe de raíz con esa arquitectura:** al estar construido de forma **nativa en C++** sobre OpenGL 3 y Vulkan, el rendimiento deja de ser un obstáculo; tus proyectos corren ligeros, rápidos y fluidos incluso en gráficas integradas modestas.

Pero el rendimiento era solo la mitad del problema. Lo más agotador de las alternativas clásicas siempre ha sido el famoso **infierno de los complementos externos**: pagar una licencia cara para descubrir que el programa base viene deliberadamente limitado, y verse obligado a rastrear, comprar y apilar decenas de *plugins* de distintos autores —rezando para que el orden de carga no corrompa el proyecto ni genere conflictos invisibles— solo para conseguir mecánicas que hoy deberían existir de fábrica. Si querías un combate en tiempo real decente, necesitabas parches; si querías movimiento fluido en 8 direcciones, animaciones que no estuvieran encorsetadas en el eterno y rígido ciclo de 3 fotogramas, o salirte de una cuadrícula fija e inamovible, tenías que pelear contra el propio motor. En ClockGears Engine todo eso nace **integrado en el núcleo desde el primer día**: combate por turnos y Action RPG en tiempo real conviviendo en el mismo proyecto, rejillas elegibles de `16`, `32` y `48 px` con colocación libre fuera de casilla, corte de *spritesheets* a medida, edición visual de *hitboxes* y *hurtboxes* fotograma a fotograma, búsqueda de rutas A*, sistemas de magia con canalizadores y equipo por capas, todo diseñado bajo un mismo techo para funcionar en armonía sin depender de un solo parche de terceros.

A esto se sumaba la condena estética de los "juegos clónicos": esas ventanas genéricas, los menús idénticos que delatan en dos segundos con qué programa se hizo el juego y las resoluciones anticuadas que emborronan el pixel art al estirarse en monitores actuales. ClockGears Engine ataca ese problema de frente integrando un **editor visual WYSIWYG completo** con capas, carpetas, formas libres, menús radiales, HUDs interactivos y acciones tipo eventos para que cada interfaz tenga personalidad propia sin escribir una línea de código, acompañado de soporte nativo para pantallas panorámicas (`16:9` y `21:9`) y escalado entero que mantiene cada píxel impecablemente nítido.

El motivo por el que este motor es público y **100% gratuito, sin regalías ni letra pequeña**, es tan sencillo como honesto: lo estoy forjando para que sea la herramienta definitiva con la que construir mis propios videojuegos de forma rápida, estable y sin límites artificiales. Si todo ese trabajo también puede servirle a otros desarrolladores independientes para crear el RPG o ARPG que siempre soñaron —sin pagar un centavo, sin pelear con *plugins* incompatibles y con la potencia de un motor nativo en C++—, entonces el esfuerzo habrá valido el doble.

### 🏆 La meta: `v1.0-2D-Master`

La **versión 1.0** de ClockGears Engine no espera al final de las 55 fases: **se alcanza al completar el Arco 2 (Fase 25)** y llevará el nombre de **`v1.0-2D-Master`**.

El propósito de los dos primeros arcos es ambicioso y cristalino: **alcanzar la cúspide absoluta del desarrollo 2D *top-down* en pixel art**. Cuando el Arco 2 concluya, el motor será una herramienta tan completa, estable y autosuficiente de serie que permitirá crear desde una aventura clásica minimalista de exploración pura hasta un RPG moderno de altísimo nivel visual y mecánico.

* **Una obra maestra preservable de por vida:** `v1.0-2D-Master` está diseñada para ser una rama sólida, pulida y lista para producción comercial. A partir del Arco 3 el motor se adentra en terrenos mucho más experimentales y complejos (como el renderizado 2.5D y los escenarios tridimensionales). Asegurar primero la perfección absoluta del 2D garantiza que, pase lo que pase en el futuro con los experimentos más ambiciosos, siempre existirá una versión 1.0 impecable que podrá guardarse y usarse durante años para terminar juegos sin miedo a que nada se rompa.
* **Hoja de ruta viva y flexible (fases adelantables):** Aunque el plan maestro está organizado en 7 arcos, **muchas características planeadas para arcos posteriores pueden adelantarse e integrarse directamente en `v1.0-2D-Master`** si aportan un valor inmediato al desarrollo 2D. Esto significa que, incluso sin dar el salto al 2.5D, tecnologías como los efectos de **post-procesado**, el escalado de código abierto **FSR**, mejoras de audio o pulidos de interfaz pueden llegar mucho antes de lo previsto para hacer que el 2D puro brille con acabados modernos en cualquier pantalla.

---

## 📥 Descarga

No necesitas instalar nada más. Descarga el ejecutable compilado directamente desde la pestaña de **Releases**:

[![Descargar última versión](https://img.shields.io/badge/📥_Descargar_ClockGears_Engine-GitHub_Releases-d97706?style=for-the-badge)](https://github.com/krisztsa/ClockGears-Engine/releases)

---

## ✨ Qué incluye

| Módulo | Funciones principales |
| :--- | :--- |
| 🗺️ **Mapas y editor** | Flujo de trabajo clásico por rejillas (`16`, `32` y `48 px`) con colocación libre, árbol de mapas, capas con autotiles, pasabilidad (`○` / `×` / `★`), punto de inicio y prueba rápida con `F5`. |
| 🎬 **Eventos sin código** | Múltiples páginas por evento, interruptores, variables, rutas de movimiento, diálogos con retratos, elecciones, teletransportes y efectos de pantalla (imágenes, tintes y temblor). |
| ⚔️ **Combate dual nativo** | **Por turnos** (AGI, 10 elementos, estados, EXP/oro/drops, editor de tropas 16:9 y panel `F3`) y **ARPG en tiempo real** (4/8 direcciones, hitboxes por frame, esquiva, bloqueo, IA con A* y equipo por capas), conviviendo en el mismo proyecto. |
| 🗃️ **Base de datos** | Actores, clases, habilidades, objetos, equipo, enemigos, tropas, estados, animaciones, tilesets, flora, fauna y eventos comunes en ventana redimensionable por capítulos. |
| 🎞️ **Animaciones** | Editor por ranuras (`Idle`, `Ataque`, `Daño`, `Muerte`, `Buff` y lanzamientos por elemento) con corte en celdas, FPS, bucle, orden aleatorio y prueba en vivo. |
| 🎨 **Identidad y rendimiento** | Iconos pixel art dibujados a mano con escalado entero, tema oscuro redondeado y ejecución nativa en **C++** compatible con OpenGL 3 / Vulkan. |

---

## 🗺️ Hoja de Ruta

El desarrollo avanza en **55 fases** divididas en dos grandes eras: la **Era 2D Master (Arcos 1–2)** que culmina en **`v1.0-2D-Master`**, y la **Era Post-1.0 (Arcos 3–7)** dedicada a la expansión visual 2.5D, audio avanzado, cinemáticas y ecosistema completo.

> [!NOTE]
> **Hoja de ruta viva:** El orden de las fases es orientativo. Cualquier función de los Arcos 3–7 que mejore el flujo de trabajo 2D (como post-procesado, FSR o mejoras del editor) puede **adelantarse e incluirse antes de lo previsto en `v1.0-2D-Master`**.

**Leyenda:** ✅ Hecha · 🔧 En curso · ⏳ Planeada · 🚀 **Update grande** · 🏆 **Hito Mayor**

| Arco | Fases | Enfoque | Estado |
| :--- | :---: | :--- | :---: |
| **Arco 1** | 1 – 11 | Núcleo del motor, ARPG, Menús WYSIWYG, Magia y Exportación | 🔧 En curso (`9B`) |
| **Arco 2** | 12 – 25 | 🏆 **`v1.0-2D-Master`** — Cúspide del motor 2D (Combate, Misiones, Clima, Ciclo día/noche) | ⏳ Planeado |
| **Arco 3** | 26 – 40 | *Post-1.0:* Motor visual avanzado (Iluminación, Partículas, Post-proceso, Diorama 2.5D, HDR, FSR/DLSS) | ⏳ Planeado |
| **Arco 4** | 41 – 47 | *Post-1.0:* Interfaz avanzada, personalización del editor y limpieza visual total | ⏳ Planeado |
| **Arco 5** | 48 – 50 | *Post-1.0:* Audio avanzado, API de código estable y editor de cinemáticas | ⏳ Planeado |
| **Arco 6** | 51 – 53 | *Post-1.0:* Exportación dedicada a PC, Android, Web, macOS e integraciones | ⏳ Planeado |
| **Arco 7** | 54 – 55 | *Post-1.0:* Calidad, beta y lanzamiento **v2.0 Maestro Relojero** | ⏳ Planeado |

### 🥇 Era 1.0: Camino a `v1.0-2D-Master`

<details>
<summary><b>📂 Ver Arco 1: Núcleo del motor (Fases 1–11) — 🔧 En curso</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **1** | Base y rebranding ✅ | Rebranding completo a ClockGears Engine · créditos de Kris'ZtS · icono propio · compilación aislada en un solo `.exe`. |
| **2** | Interfaz clásica y tema Dark ✅ | Barra superior de accesos rápidos · tema ClockGears Dark · iconos pixel art · selectores y validación en vivo. |
| **3** | Mapas y rejilla ✅ | Rejillas de 16, 32 y 48 · colocación libre · importar imágenes · árbol de mapas · deshacer/rehacer y herramientas de dibujo. |
| **4** | Eventos sin código ✅ | Editor visual de eventos · páginas, condiciones y disparadores · interruptores y variables · pestaña Code opcional. |
| **5** | Base de datos ✅ | Actores, clases, habilidades, objetos, equipo, enemigos, tropas y estados · importar/exportar assets y búsqueda. |
| **6** | Tilesets y colisiones ✅ | 9 ranuras (`A1–A5`, `B–E`) · máscara `○` / `×` / `★` · 4 direcciones, arbusto, mostrador, escalera, daño y terreno 0–7. |
| **7** | Sistema narrativo ✅ | Ventana de mensajes · códigos de escape · retratos con expresiones · historial · modos AUTO y SALTAR · vista previa en vivo. |
| **8** | Combate por turnos, iconos y animaciones ✅ | Combate por turnos con AGI, 10 elementos fijos y panel `F3` · editor de animaciones por ranuras · editor de tropas 16:9 con escala x1–x6 y prueba directa · nueva Base de Datos en dos columnas · iconos pixel art hechos a mano. |
| **9A** | Combate en tiempo real (ARPG) ✅ | Modo Por turnos / ARPG por proyecto o mapa · 4/8 direcciones · animaciones por acción · ratón y teclado · esquiva, bloqueo e invulnerabilidad · editor visual de hitboxes/hurtboxes por frame · IA con búsqueda de ruta A* y zonas de detección · equipo ARPG por capas · herramientas (hacha, pico, pala, azada) · habilidades físicas con recurso propio · depuración `F3`. |
| **9B** | Menús del juego y menú principal 🔧 | Pantalla de título, opciones, guardado/carga, créditos y Game Over personalizables · estilos de ventana (9-patch, bordes, opacidad) · HUD interactivo y menús ilimitados (listas, cuadrículas, carrusel, rueda radial) · editor visual WYSIWYG con capas, carpetas, acciones tipo eventos, zoom manual y plantillas · capas con nombre en mapas · temas Dark/Latón con estilo Neón Cobre · nuevo icono, versión `Alpha-0.0.4`, pantalla de carga y Administrador de proyectos · soporte UTF-8 y pantallas 16:9 / 21:9. |
| **10** | Magic Update ⏳ | Hechizos modulares con 10 elementos y 8 estados · sistema de canalizadores (Grimorios por elemento/estado con desbloqueo por nivel, Báculos universales y objetos canalizadores) · barra rápida 1–9 en ARPG y comando Grimorio en turnos · sistema de prefabs de eventos · generador de estructuras con lógica y UI propia · buscador de eventos por nombre. |
| **11** | Exportación, inputs y Port Nativo a Android ⏳ | Exportación a Windows, Linux, Web y Android (APK/AAB) con presets · **Port Nativo del editor CGE a Android** · reestructuración visual de paneles heredados de Godot · editor de inputs (teclado, ratón, mando y táctil) · apartado Android System para configurar controles táctiles con sprites propios. |

</details>

<details>
<summary><b>📂 Ver Arco 2: RPG completo y lanzamiento 🏆 v1.0-2D-Master (Fases 12–25)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **12** | Animaciones II y cierre de combate | Duración por frame y orden aleatorio · triggers por acción · sonidos/flashes por fotograma · tropas múltiples. |
| **13** | Eventos II | Ramas anidadas · bucles, etiquetas y saltos · copiar/pegar/buscar comandos · comentarios · eventos comunes. |
| **14** | Eventos III | Rutas de movimiento · seguidores del grupo · control de cámara y scroll · tintes, sacudidas y flashes · imágenes en pantalla. |
| **15** | Mapas II | Autotiles A1–A4 completos · regiones · capas de tiles y eventos · copiar/exportar mapas · transiciones. |
| **16** | Combate II | Resistencias/debilidades por elemento y estado · críticos, evasión y fallo · objetivos múltiples · editor de fórmulas. |
| **17** | Combate III | Costes, recargas, pasivas, habilidades encadenadas, invocaciones, contraataque, cobertura e IA por patrones. |
| **18** | Combate IV | Jefes con fases · eventos dentro de batalla · emboscadas · vistas frontal y lateral · fondos por mapa/región. |
| **19** | 🚀 **Cronómetro Update** | Turnos a elegir (clásico, ATB, ticks) · simulador de balance · curvas de dificultad · cola visual · auto-batalla. |
| **20** | Progresión | Clases y multiclase · árboles de habilidades · slots configurables · curvas de EXP · ascensos. |
| **21** | Inventario y economía | Tiendas y editor · crafteo con recetas · peso y límites · objetos clave · monedas múltiples. |
| **22** | Sistema de juego | Menú principal editable · formación y equipo · guardado con captura · dificultad · créditos. |
| **23** | Misiones y diario | Misiones con objetivos · diario y bestiario · enciclopedia · marcadores en mapa · recompensas. |
| **24** | Automatización | Presets de movimiento (2/4/6/8 dir.) · puertas, cofres y teletransportes listos · NPC con ruta · plantillas. |
| **25** | 🏆 **Crónicas Update (`v1.0-2D-Master`)** | Ciclo día/noche · clima por mapa · calendario y tiempo · horarios de NPC · afinidad · logros · **cierre y congelación estable de la versión 1.0 definitiva para 2D**. |

</details>

### 🌌 Era Post-1.0: Expansión Visual 2.5D y Evolución (Fases 26–55)

<details>
<summary><b>📂 Ver Arco 3: Motor visual, Post-proceso, FSR y Diorama 2.5D (Fases 26–40)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **26** | Render base | Orden por Y y capas de profundidad · sombras de tiles · luces 2D puntuales · mapas de normales · máscaras de luz. |
| **27** | Partículas I | Editor visual WYSIWYG · curvas y gradientes · presets (fuego, chispas, humo, magia) · vista previa en vivo. |
| **28** | Partículas II | Comando "Mostrar partículas" · partículas en combate · colisión con tiles · estelas y sub-emisores. |
| **29** | 🚀 **Péndulo Update** | Biblioteca de efectos · lluvia, nieve y niebla · hechizos con partículas y luz · shaders listos · editor unificado. |
| **30** | Post-proceso | Bloom · viñeta · aberración cromática · grano y pixelado · LUTs de color · pila de efectos por mapa (candidata a adelantarse a 1.0). |
| **31** | Cámara 2.5D | Perspectiva inclinada · rotación en pasos de 90° · sprites billboard · zoom y seguimiento suave. |
| **32** | Mapas 2.5D | Tiles con altura · escaleras y desniveles · techos ocultables · objetos 3D low poly (`glTF`) · parallax. |
| **33** | Luz y sombras | Sombras proyectadas · luces dinámicas y emisivos · reflejos en agua · luz coherente día/noche · lightmaps. |
| **34** | Profundidad de campo | DoF y tilt-shift · bokeh · enfoque por eventos · cámara cinematográfica · desenfoque en diálogos. |
| **35** | 🚀 **Diorama Update** | Estilo propio **Diorama 2.5D** (pixel art sobre escenarios 3D) · plantilla, presets y mapa demo. |
| **36** | HDR | Pipeline HDR · tonemapping (ACES, Filmic) · exposición automática · calibración en juego. |
| **37** | FSR | Escalado abierto FSR · resolución interna, calidad y nitidez ajustables (candidata a adelantarse a 1.0). |
| **38** | DLSS y XeSS | Integración NVIDIA DLSS e Intel XeSS con detección de hardware y ajustes en menú. |
| **39** | Rendimiento | Perfilador · culling y batching · atlas de texturas · presets Bajo/Medio/Alto · límite de FPS y VSync. |
| **40** | 🚀 **Cristal de Cuarzo Update** | Panel único de calidad gráfica · benchmark automático · modo foto · captura/grabación · paquete final de shaders. |

</details>

<details>
<summary><b>📂 Ver Arco 4: Interfaz y editor (Fases 41–47)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **41** | HUD I | Editor WYSIWYG · barras de HP/MP · ventanas con anclas · rejillas y vista previa multiresolución. |
| **42** | HUD II | Skins 9-patch · fuentes bitmap · animaciones de UI · cursores, sonidos y estados de botón. |
| **43** | HUD III | Plantillas de título, menú, tienda y batalla · widgets reutilizables · enlaces de datos · modales. |
| **44** | Personalización | 8 colores de acento · temas Retro Pixelart, Dark OLED y Classic JRPG Blue. |
| **45** | Identidad reorganizada | Iconos SVG y tipografía propios · paneles acoplables · búsqueda global · español e inglés. |
| **46** | Accesibilidad y aprendizaje | Tamaño de fuente y modos daltonismo · remapeo · tutorial interactivo · documentación integrada. |
| **47** | 🚀 **Relojería Update** | Limpieza visual casi total del origen Godot · tema final · modo ventana pequeña · revisión general. |

</details>

<details>
<summary><b>📂 Ver Arco 5: Audio, código y cine (Fases 48–50)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **48** | Audio | Editor de BGM, BGS, ME y SE · bucles con crossfade · buses · sonido posicional, reverb y ducking. |
| **49** | Código y datos | Pestaña Code con API estable · hooks y plugins · importar/exportar JSON y CSV · localización. |
| **50** | 🚀 **Telón Update** | Editor de cinemáticas con línea de tiempo (cámara, diálogo, partículas, audio) y exportación a vídeo. |

</details>

<details>
<summary><b>📂 Ver Arco 6: Plataformas (Fases 51–53)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **51** | Exportación PC | Windows y Linux en 1–2 clics · instalador · compresión de assets · licencias incluidas. |
| **52** | Android | SDK y keystore automáticos · controles táctiles · APK y AAB · pruebas en dispositivo. |
| **53** | Web, macOS e integraciones | Build Web y macOS · paquete base para consolas · Steam e itch.io · guardado en la nube. |

</details>

<details>
<summary><b>📂 Ver Arco 7: Cierre Post-1.0 (Fases 54–55)</b></summary>
<br>

| # | Fase | Qué trae |
| :---: | :--- | :--- |
| **54** | Calidad y beta | Pruebas automáticas · informe de regresión · manual completo · sistema de reportes. |
| **55** | 🏆 **Maestro Relojero 2.0 Update** | API congelada · documentación final · proyecto de ejemplo completo · lanzamiento oficial de la era completa. |

</details>

---

## 🛠️ Cómo compilar

<details>
<summary><b>Ver requisitos y comando de compilación (Windows)</b></summary>
<br>

Compilar el motor es posible siempre dentro de los términos de la licencia. Necesitas **Python 3.8+**, **SCons** (`pip install scons`) y un compilador de C++ (**MSVC** o **MinGW-w64**):

```bash
scons platform=windows target=editor arch=x86_64 -j8
```

</details>

---

## 📜 Licencia

ClockGears Engine cuenta con una **licencia propia** para las modificaciones y el código original de **ClockGears Studio**, y conserva la **licencia MIT** del código base de [Godot Engine](https://godotengine.org).

* ✅ **Permitido sin pedir permiso:** Usar el editor para crear, distribuir, publicar o vender tus juegos (incluyendo el runtime exportado). **Sin regalías, sin pagos y sin suscripciones.** Tus juegos te pertenecen.
* ⛔ **No permitido sin permiso escrito de Kris'ZtS:** Copiar, redistribuir, vender, sublicenciar o publicar el editor ClockGears Engine, su código fuente o versiones modificadas, en todo o en parte de lo creado por ClockGears Studio.
* 🧩 **Código de Godot Engine:** Mantiene su licencia MIT y sus avisos de copyright originales (`© 2014-presente Godot Engine contributors`, `© 2007-2014 Juan Linietsky, Ariel Manzur`).

Consulta el texto completo en [`LICENSE-CLOCKGEARS.txt`](./LICENSE-CLOCKGEARS.txt) o desde el editor en **Ayuda → Acerca de → Licencia**.

---

<div align="center">

**Creado por Kris'ZtS · ClockGears Studio (2026)**

[![GitHub](https://img.shields.io/badge/GitHub-ClockGears--Engine-181717?logo=github)](https://github.com/krisztsa/ClockGears-Engine)
[![YouTube](https://img.shields.io/badge/YouTube-Kris'ZtS-dc2626?logo=youtube&logoColor=white)](https://www.youtube.com/@KrisZtS)

</div>
