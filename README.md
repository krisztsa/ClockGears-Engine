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
*ARCO 1: Cimientos (1–8)

1. Estabilización 0.1: cerrar combate, tropas y animaciones de la 8A · guardado de proyecto robusto · suite de regresión automática · limpieza de logs y del error de debug_settings · lista de bugs conocidos en cero

2. Componentes compartidos: un solo selector de objetos, sprites y audio · layouts con expansión y scroll en todos los paneles · tema centralizado en un solo archivo · tooltips en vez de textos largos · atajos de teclado base

3. Animaciones II: duración por frame y orden aleatorio · sonidos y flashes por fotograma · tabla de triggers (ataque, daño, muerte) en Actores y Enemigos · efectos sobre el objetivo en Habilidades y Objetos · vista previa con Play

4. Eventos II: ramas anidadas con contenido (elecciones, condicionales) · bucles, etiquetas y saltos · copiar y pegar comandos · buscar y reemplazar en eventos · comentarios

5. Eventos III: rutas de movimiento · seguidores del grupo · control de cámara y scroll · tintes, sacudidas y flashes de pantalla · imágenes en pantalla

6. Mapas II: autotiles A1–A4 completos · regiones · capas de eventos y tiles · copiar o exportar mapas · rejillas 16, 32 y 48 con “salir de la casilla”

7. Narrativa II (tu 7B, ajústala): cinemáticas sencillas con eventos · mensajes de mapa y nombres flotantes · diálogos con variables · textos exportables para traducir · retratos animados

8. Primer Engranaje Update: plantilla de proyecto vacía · título, guardar y cargar, game over · menú de opciones in-game · proyecto demo con assets propios · primer instalador de prueba

ARCO 2: RPG completo (9–19)

9. Combate II: elementos y resistencias · críticos, evasión y fallo · objetivos (todos, aleatorio, aliados) · estados avanzados · editor de fórmulas con validación

10. Combate III: tipos de habilidad, costes y recargas · pasivas y rasgos · habilidades encadenadas · invocaciones · contraataque y cobertura

11. Combate IV: IA enemiga por patrones y condiciones · jefes con fases · eventos dentro de la batalla · huida y emboscadas configurables · recompensas especiales

12. Combate V: vista frontal, lateral e isométrica · posiciones de formación · fondos de batalla por mapa o región · cámara de combate · transiciones de entrada y salida

13. Cronómetro Update: sistemas de turno a elegir (clásico, ATB, por ticks) · simulador de batallas con balance automático · curvas de nivel y dificultad · cola de turnos visual · modo auto-batalla

14. Progresión: clases y multiclase · árboles de habilidades · slots de equipo configurables · curvas de EXP por actor · ascensos y reencarnación

15. Inventario y economía: tiendas y su editor · crafteo con recetas · peso y límites · objetos clave · moneda múltiple

16. Sistema de juego: menú principal editable · estado, formación y equipo · guardado en ranuras con captura · opciones de dificultad · pantalla de créditos

17. Misiones y diario: misiones con objetivos · diario y bestiario · enciclopedia de objetos · marcadores en el mapa · recompensas

18. Automatización: presets de movimiento en 2, 4, 6 y 8 direcciones · presets de puertas, cofres y teletransportes · NPC con ruta · plantillas de eventos comunes · auto-conexión sin configurar inputs

19. Crónicas Update: ciclo día y noche · clima por mapa · calendario y tiempo del mundo · horarios de NPC · afinidad con personajes · logros

ARCO 3: Motor visual (20–34)

20. Render base: orden por Y y capas de profundidad · sombras de tiles · luces 2D puntuales · mapas de normales · máscaras de luz

21. Partículas I: editor visual WYSIWYG · curvas y gradientes · presets (fuego, chispas, humo, magia) · vista previa en vivo · importar y exportar

22. Partículas II: comando de evento “Mostrar partículas” · partículas en combate y en habilidades · colisión con tiles · trails y sub-emisores · límites de rendimiento

23. Péndulo Update: biblioteca de efectos prehechos · clima (lluvia, nieve, niebla) · hechizos con partículas y luz · shaders listos · editor de efectos unificado

24. Post-proceso: bloom · viñeta y aberración cromática · grano y pixelado · corrección de color con LUT · pila de efectos por mapa

25. Cámara 2.5D: perspectiva inclinada · rotación por pasos de 90° · sprites billboard · zoom y seguimiento suave · presets de cámara

26. Mapas 2.5D: tiles con altura · escaleras y desniveles · techos que se ocultan · objetos 3D low poly (glTF) · parallax en capas

27. Luz y sombras: sombras proyectadas de sprites · luces dinámicas y emisivos · reflejos en agua · día y noche con luz coherente · lightmaps horneados opcionales

28. Profundidad de campo: DoF y tilt-shift · bokeh · enfoque controlado por eventos · cámara cinematográfica · desenfoque de fondo en diálogos

29. Diorama Update: estilo propio Diorama 2.5D (pixel art sobre escenarios 3D) · plantilla lista · presets de cámara, luz y DoF · partículas integradas · mapa demo

30. HDR: pipeline HDR · tonemapping (ACES, Filmic) · exposición automática · salida a pantallas HDR · calibración in-game

31. FSR: integrar FSR (la base ya trae alguna versión) · escalado por resolución interna · selector de calidad · nitidez ajustable · fallback sin soporte

32. DLSS y XeSS: DLSS vía SDK de NVIDIA · XeSS de Intel · detección de hardware · desactivación automática si no hay soporte · ajustes en el menú

33. Rendimiento: perfilador integrado · culling y batching · atlas de texturas · presets Bajo, Medio y Alto · límites de FPS y VSync

34. Cristal de Cuarzo Update: panel único de calidad gráfica · benchmark automático · modo foto · captura y grabación · paquete final de shaders

ARCO 4: Interfaz y editor (35–43)

35. HUD I: editor WYSIWYG con arrastrar y soltar · barras de vida y MP · ventanas con anclas · alineación y rejillas · vista previa en varias resoluciones

36. HUD II: skins de ventana 9-patch · fuentes bitmap · animaciones de UI · cursores y sonidos de menú · estados (normal, hover, presionado)

37. HUD III: plantillas de título, menú, tienda y batalla · widgets reutilizables · enlaces a datos del juego · pantallas modales · pruebas de resolución

38. Personalización del editor: paleta de 8 acentos · tema Retro Pixelart · Dark OLED · Classic JRPG Blue · guardado de preferencias

39. Identidad visual: iconos SVG propios · tipografía propia · logo y splash discreto · iconos de archivo y de ejecutable · coherencia de marca

40. Editor reorganizado: barra superior estilo MZ · paneles acoplables y recordados · búsqueda global · historial de deshacer visible · interfaz en español e inglés

41. Accesibilidad: tamaño de fuente · modos para daltonismo · remapeo de controles · soporte de mando · subtítulos y avisos visuales

42. Aprendizaje integrado: tutorial interactivo de primer uso · documentación dentro del editor · ejemplos abribles · tooltips con enlace al manual · plantillas con comentarios

43. Relojería Update: limpieza visual casi total del origen Godot · nombres internos y menús propios · tema final · modo ventana pequeña · revisión de cada pantalla

ARCO 5: Audio, datos y código (44–48)

44. Audio I: editor de BGM, BGS, ME y SE · bucles con puntos · crossfade · buses y volúmenes por categoría · previsualización

45. Audio II: sonido posicional · reverb por mapa · música adaptativa · ducking en diálogos · importar y exportar audios

46. Código opcional: pestaña Code con API estable · hooks y plugins · documentación de la API · ejemplos · aislamiento de errores de scripts

47. Datos y localización: importar y exportar JSON y CSV · traducción de textos · versionado del proyecto · migraciones automáticas · copias de seguridad

48. Telón Update: editor de cinemáticas con línea de tiempo · pistas de cámara, diálogo, partículas y audio · cortes y transiciones · reproducción dentro del juego · exportar a vídeo

ARCO 6: Plataformas (49–52)

49. Exportación PC: Windows y Linux en 1 clic · instalador · ejecutable con tu icono · compresión de assets · avisos de licencias incluidos

50. Android: SDK y keystore automáticos · controles táctiles · pantalla adaptable · APK y AAB · pruebas en dispositivo

51. Web, macOS y consolas: build web · build macOS · paquete base para consolas (sin SDK, que exige acuerdos) · guía de portado · validación de límites de memoria

52. Integraciones: logros y nube de Steam · itch.io · guardado en la nube · actualizador de proyecto · telemetría opcional de errores

ARCO 7: Cierre (53–55)

53. Calidad: pruebas automáticas por pantalla · proyectos de prueba · pruebas de rendimiento · fuzzing de datos · informe de regresión

54. Beta pública: manual completo · plantillas y tutoriales en vídeo · canal de comunidad · sistema de reportes con captura · ciclo de hotfix

55. Maestro Relojero 1.0 Update: congelar la API · documentación final · proyecto de ejemplo completo · web y página de descarga · lanzamiento.*
