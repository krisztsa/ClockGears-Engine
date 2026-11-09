# ⚙️ ClockGears Engine

![Estado](https://img.shields.io/badge/estado-En%20Desarrollo-brightgreen)
![Base](https://img.shields.io/badge/base-Godot%20Engine%20(C%2B%2B%20Fork)-478cbf)
![Estilo](https://img.shields.io/badge/diseño-RPG%20Maker%20MZ-purple)
![Plataforma](https://img.shields.io/badge/plataforma-Windows%20%7C%20Linux-blue)
![Licencia](https://img.shields.io/badge/licencia-MIT-lightgrey)

> ⚠️ **AVISO IMPORTANTE SOBRE GRATUIDAD Y ESTAFAS**
> **ClockGears Engine NO es de paga en GitHub. Es un motor completamente gratuito creado para facilitar el desarrollo de videojuegos RPG sin tener que pagar licencias, y únicamente cuenta con una opción voluntaria de donaciones fuera del programa. Si alguien te cobró por este motor, has sido estafado. La única vía oficial es nuestro GitHub.**

---

## 📖 ¿Qué es ClockGears Engine?

**ClockGears Engine** es un motor de videojuegos inspirado en **RPG Maker MZ** y construido modificando directamente el código fuente en C++ de **Godot Engine**.

Su objetivo principal es **facilitar la creación de videojuegos RPG sin tener que pagar**, integrando de serie todas las herramientas que necesitas para armar tu juego sin programar sistemas desde cero.

## Créditos y licencia
Creado por **Kris'ZtS**. Basado en [Godot Engine](https://godotengine.org), distribuido bajo licencia MIT. Conserva el aviso de copyright original de Godot en el repositorio.


---

## ✨ Qué incluye

* **Interfaz y Flujo de Trabajo estilo RPG Maker MZ:** Árbol de mapas, paleta de tiles con soporte de autotiles, propiedades de mapa y modos de edición directa en cuadrícula.
* **Editor de Eventos Integrado:** Creación visual de eventos con múltiples páginas, condiciones por interruptores (*Switches*) y variables, rutas de movimiento, diálogos, elecciones, teletransportes y control de flujo sin tocar código.
* **Sistema de Combates por Turnos:** Motor de batalla por turnos listo para usar con configuración de fórmulas, habilidades, estados alterados, elementos y grupos de enemigos (*Troops*).
* **Base de Datos RPG Completa:** Gestión centralizada de personajes, clases, habilidades, objetos, armas, armaduras, enemigos, estados, animaciones, tilesets y eventos comunes.
* **Rendimiento Nativo en C++:** Ligero, rápido y compatible con gráficas integradas y dedicadas mediante OpenGL 3 / Vulkan.

---

## 🛠️ Cómo compilar el motor

Si deseas compilar el código fuente por tu cuenta necesitas **Python 3.8+**, **SCons** (`pip install scons`) y un compilador C++ (MSVC o MinGW-w64):

```ash
scons platform=windows target=editor arch=x86_64 -j8
```

Si solo quieres usar el motor listo para abrir y crear tu juego, descarga el ejecutable desde la sección de [**Releases**](https://github.com/krisztsa/ClockGears-Engine/releases).

---

## 🤝 Estado del Proyecto y Créditos

* **Creado por Kris´ZtS** (ClockGears Studio) usando asistencia de **Claude** sobre la base de código de Godot Engine.
* **GitHub oficial:** https://github.com/krisztsa/ClockGears-Engine
* **Canal de YouTube:** https://youtube.com/@kriszts?si=la18fuis0frX-LGA

**Creado en 2026.**

> **ClockGears Engine no es de paga en GitHub y solo tiene una opción de donaciones fuera del programa.**



---
# 🗺 Hoja de ruta de ClockGears Engine hacia la 1.0

**ClockGears Engine** es un motor de RPG 2D por rejillas con editor visual al estilo RPG Maker MZ, hecho en C++ por **Kris'ZtS**. Esta es la ruta de 55 fases hasta la versión 1.0.

> ⚠️ Es una hoja de ruta, no una promesa. El orden y el contenido pueden cambiar según la dificultad y el costo de cada fase.

**Leyenda:** ✅ Hecha · 🔧 En curso · ⏳ Planeada · 🚀 **Update** (hito grande con mejoras masivas)

---

## Arco 1: Núcleo del motor (Fases 1–11)

| # | Fase | Qué trae |
|---|------|----------|
| 1 | Base y rebranding ✅ | Rebranding completo a ClockGears Engine · créditos de Kris'ZtS en "Acerca de" · icono propio del ejecutable · compilación aislada en un solo exe · banderas de compilación reducidas |
| 2 | Interfaz MZ y tema Dark 🔧 | Barra superior de accesos rápidos · tema ClockGears Dark · iconos SVG pixel art · selectores (pickers) · validación en vivo |
| 3 | Mapas y rejilla ✅ | Rejillas de 16, 32 y 48 · "Salir de la casilla" (colocación libre) · importar imágenes al mapa · árbol de mapas · deshacer y rehacer · herramientas de dibujo |
| 4 | Eventos sin código ✅ | Editor de eventos estilo MZ · páginas, condiciones y disparadores · lista de comandos editable · interruptores y variables · pestaña Code opcional |
| 5 | Base de datos ✅ | Actores, clases, habilidades, objetos, armas, armaduras y accesorios · enemigos, tropas y estados · importar y exportar assets · selectores con pestañas y búsqueda |
| 6 | Tilesets y colisiones ✅ | 9 ranuras (A1–A5, B, C, D, E) · máscara ○ pasable, × bloqueado, ★ pasa por detrás · 4 direcciones, arbusto, mostrador, escalera, daño y terreno 0–7 · selector de tileset por mapa · paleta por pestañas |
| 7 | Sistema narrativo (7A ✅ · 7B ⏳) | 7A: ventana de mensajes, códigos de escape, retratos con expresiones, caja de nombre, opciones y entrada de número, historial, modos AUTO y SALTAR, vista previa en vivo · 7B: por completar |
| 8 | Combate y animaciones 🔧 | Comando "Iniciar Batalla" · orden de turnos por AGI · Atacar, Habilidad, Objeto, Defender y Huir · fórmulas, estados, EXP, oro y drops · panel de depuración F3 · editor de animaciones de sprites |
| 9 | Por completar ⏳ | Sistema de Combate en Tiempo Real (Action RPG) |
| 10 | Por completar ⏳ | Menús Estándar del Juego y Flujo de Menú Principa |
| 11 | Por completar ⏳ | Exportación Simplificada y Empaquetado Final de la primera etapa |

---

## Arco 2: RPG completo (Fases 12–25)

| # | Fase | Qué trae |
|---|------|----------|
| 12 | Animaciones II y cierre de combate | Duración por frame y orden aleatorio · triggers de ataque, daño y muerte en Actores y Enemigos · sonidos y flashes por fotograma · tropas con varios enemigos y posiciones · regresión completa del combate |
| 13 | Eventos II | Ramas anidadas (elecciones y condiciones) · bucles, etiquetas y saltos · copiar, pegar y buscar comandos · comentarios · eventos comunes |
| 14 | Eventos III | Rutas de movimiento · seguidores del grupo · control de cámara y scroll · tintes, sacudidas y flashes · imágenes en pantalla |
| 15 | Mapas II | Autotiles A1–A4 completos · regiones · capas de tiles y eventos · copiar y exportar mapas · transiciones y teletransportes |
| 16 | Combate II | Elementos y resistencias · críticos, evasión y fallo · objetivos múltiples y aleatorios · estados avanzados · editor de fórmulas con validación |
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

