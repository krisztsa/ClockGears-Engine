<USER_REQUEST>
INSTRUCCIONES ANTES DE SEGUIR (leer todo antes de tocar código)

CONTEXTO: te interrumpí mientras hacías la Fase 2. Abajo te paso el ANEXO A.
Reemplaza la sección 15 (fases) del prompt maestro y GANA si algo se contradice.
La Fase 1 del maestro ya está hecha. Lee el anexo completo primero.

PASOS, EN ESTE ORDEN:

1. Si algún archivo quedó a medias por la interrupción (por ejemplo clockgears_ui_widgets.*),
   revísalo y complétalo para que compile.

2. Completa lo que falte de la FASE 1 del anexo, sin rehacer lo que ya funciona:
   - ClockGearsTimeManager (A3.1 y A3.2).
   - Ajustes de pixel art del proyecto (A4.1): tiles 32x32, resolución base 960x540 (16:9),
     escala entera, filtro NEAREST, snap a píxel, cámara pixel-perfect.

3. Revisa todo lo que hiciste de la Fase 2 (tema, iconos, clockgears_ui_widgets) contra la
   sección A4 (pixel art) y corrígelo si no la cumple.

4. Corrige estos bugs de la Base de Datos (ClockGearsDatabaseDialog). Antes de cambiar nada,
   dime la CAUSA RAÍZ con archivo y línea:
   a) No se puede escribir en ningún LineEdit/TextEdit/SpinBox. Revisa editable/disabled cuando
      no hay registro seleccionado, controles superpuestos con mouse_filter = STOP, y el foco
      del teclado. El usuario debe poder pulsar "+", crear un registro y escribir en todos sus campos.
   b) Los botones +, -, Max, Copiar y Pegar deben verse siempre bajo la lista.
   c) El diálogo se corta en pantalla: tamaño inicial 90% de la pantalla, redimensionable,
      con ScrollContainer en las columnas.
   d) El árbol de mapas muestra "001: Pueblo..." sin que el usuario lo haya creado.
      Encuentra de dónde sale (código o archivo del test_project) y elimínalo.
      Las listas deben iniciar vacías (Regla Cero).

5. Reglas para todo lo que sigas haciendo:
   - Código completo, sin TODO ni funciones vacías.
   - Un cambio claro por paso, indicando archivos tocados.
   - Todo debe respetar el pixel art (A4) y la Regla Cero.
   - Si el anexo contradice al maestro, elige el anexo y avísame.

6. Al terminar, NO sigas con la Fase 3. Entrégame:
   - Causa raíz de los bugs del punto 4.
   - Lista de archivos creados o cambiados.
   - Comando de compilación.
   - Checklist de prueba: abrir Base de Datos, pulsar +, escribir un nombre, cerrar, reabrir
     y ver que el nombre sigue ahí; confirmar que no hay datos de prueba.
   Y espera mi OK.

ANEXO A (a continuación):



# ANEXO A — PRIORIDADES DE LA SAGA, NARRATIVA Y PIXEL ART

> Pegar DESPUÉS del prompt maestro. Si algo de este anexo contradice al prompt maestro, **gana este anexo**.

---

## A0. CONTEXTO DE LA SAGA (define las decisiones de diseño)

ClockGears Engine NO es un motor de uso general. Es la herramienta para una saga propia, **todos los juegos en pixel art**:

| Juego | Tipo | Qué exige al motor |
|---|---|---|
| **Juego 1** | RPG narrativo por turnos | Narrativa fuerte + combate por turnos pulido y matemático |
| **Juego 2** | RPG narrativo, mapa compacto pero denso, **sistema de rutinas tipo Majora's Mask**, por turnos | Todo lo del juego 1 + reloj del juego, ciclo de días y NPCs con horarios |
| **Juego 3** (más adelante) | Mundo abierto gigante, combate dinámico | Solo dejar el motor **preparado** (ver A7). NO construir ahora |

**ENFOQUE ACTUAL: Juegos 1 y 2.** No implementes nada exclusivo del juego 3.

### Prioridad de desarrollo (de mayor a menor)
1. **Narrativa:** diálogos, cinemáticas, imágenes y música. Es lo más importante del motor.
2. **Combate por turnos:** pulido, matemático, transparente y fácil de balancear.
3. **Sistemas propios** (tiempo, rutinas, flora/fauna).
4. Todo lo demás.

Cuando haya que decidir entre pulir la narrativa o pulir cualquier otra cosa, **gana la narrativa**. Referencia de tono: juegos narrativos tipo Undertale, donde el combate es entretenido pero la historia es lo central.

---

## A1. SISTEMA NARRATIVO (reemplaza y amplía la sección 8 del prompt maestro)

### A1.1 Diálogos avanzados
Todo lo de la sección 8, **más**:
- **Retratos con expresiones:** cada actor/NPC tiene un conjunto de expresiones con nombre (`neutral`, `feliz`, `triste`, `enojado`, `sorprendido`…) definidas por el usuario (lista vacía por defecto). Comando de texto: `\E[expresión]` cambia la expresión a mitad de frase.
- **Retratos grandes** (estilo visual novel) además de caras pequeñas: posición izquierda/centro/derecha, entrada/salida con fundido o deslizamiento, resaltado del que habla (los demás se oscurecen).
- **Voz de texto (blips):** sonido corto por letra, configurable por personaje (tono, volumen, variación). Silenciable con `\V0`.
- **Estilos de ventana por personaje o por escena:** color, borde, fuente, fondo. Cambiables con comando.
- **Texto con efectos** (de la sección 8) usable también en ventanas libres y sobre imágenes.
- **Historial de diálogos (backlog):** tecla para releer lo último dicho (mínimo 50 mensajes, guardado en la partida).
- **Avance automático y salto:** modo Auto (espera según longitud del texto) y modo Saltar (solo textos ya leídos, configurable).
- **Diálogos con tiempo:** opción de elección con límite de tiempo (opcional por comando).
- **Ramas y consecuencias:** cada opción puede escribir variables/banderas; el editor muestra un **panel de banderas narrativas** (lista de interruptores y variables marcadas como "narrativas", con nota de dónde se usan).

### A1.2 Editor de cinemáticas (Timeline)
Clase `ClockGearsCutsceneEditor`:
- Línea de tiempo con **pistas**: Cámara, Personajes/Eventos, Imágenes, Texto, Audio, Pantalla (efectos), Espera.
- Cada pista tiene **claves** (keyframes) con tiempo, acción y **curva de interpolación** (lineal, ease-in, ease-out, ease-in-out, rebote, salto).
- Se puede **previsualizar** dentro del editor con play/pausa/avance por fotograma.
- Se ejecuta en runtime como un comando de evento ("Reproducir Cinemática") y se puede **saltar** con una tecla (configurable por cinemática) dejando el estado final correcto (banderas aplicadas, posiciones finales).
- Una cinemática es un recurso reutilizable (`CGCutscene`) que se puede llamar desde varios eventos. Lista vacía por defecto (Regla Cero).
- Opciones: barras de cine (letterbox) animadas, bloqueo de control del jugador, restauración automática de cámara y seguidores al terminar.

### A1.3 Imágenes y pantalla
- **Imágenes (mínimo 100 simultáneas):** mostrar, mover, rotar, escalar, opacidad, tono, modo de mezcla (normal, aditivo, multiplicar, pantalla), capa (detrás/delante del mapa, de la UI), anclaje (esquina, centro).
- **Pixel art:** las imágenes se escalan SOLO por enteros por defecto; escala fraccionaria exige activar una casilla con aviso.
- **Efectos de pantalla:** tono, flash, temblor, viñeta, desenfoque pixelado (pixelate), ondulación, escala de grises, inversión, fundido a color/imagen, **transiciones personalizadas** (disolver por máscara en escala de grises, cortina, persiana, mosaico).
- **Cámara:** seguir, fijar, desplazar con curva, zoom entero (1x, 2x, 3x), temblor, ir a punto de interés.
- **Efectos de partículas simples** (lluvia, nieve, polvo, luciérnagas, cenizas) con parámetros editables y paleta limitada.
- **Capas de parallax y niebla** por mapa.

### A1.4 Música y audio (`ClockGearsAudioManager`)
Singleton en C++ sobre el sistema de audio de Godot:
- **BGM con fundido cruzado** (crossfade) entre pistas, duración configurable.
- **Puntos de bucle** (inicio y fin) por pista, con bucle sin cortes.
- **Música por capas (stems):** una pista base + capas que se activan/desactivan por comando o bandera (ej. se agrega percusión en tensión). Todas sincronizadas.
- **BGS** (ambiente) y **ME** (jingles) con regla: al terminar un ME, la BGM vuelve sola.
- **Guardar y reanudar** la música actual (para batallas y cinemáticas).
- **Música por mapa, por hora del día (juego 2) y por situación**, sin cortar si la pista es la misma.
- **Duck automático:** baja la música mientras suena una voz o un efecto importante.
- Buses separados: Música, Efectos, Ambiente, Voces, UI. Controles de volumen en opciones.
- Formatos: OGG, WAV, MP3. Sin dependencias externas.
- Todo con **IDs de audio** gestionados por el DataManager (lista vacía por defecto; el usuario importa sus pistas).

### A1.5 Escenas y progreso de historia
- **Galería/registro** (opcional): escenas vistas, finales, imágenes y música desbloqueadas.
- **Guardado en cualquier momento** con captura de pantalla (miniatura en pixel art, sin filtro) y datos del capítulo.
- **Capítulos:** marcador de capítulo en la partida y en el menú.
- **Idioma:** todo texto narrativo en tablas de traducción exportables (CSV), con el español como idioma de origen.

---

## A2. COMBATE POR TURNOS: PULIDO, MATEMÁTICO Y TRANSPARENTE

Base: secciones 9 y 5.3 del prompt maestro. Añadir:

### A2.1 Estructura
- Orden de turnos **visible** (línea de turnos con iconos), calculado con la fórmula 9.6, actualizable en vivo cuando cambian estados o velocidad.
- Modos configurables por proyecto: por rondas clásicas (todos eligen y luego actúan) o por orden de iniciativa.
- Acciones: Atacar, Habilidad, Objeto, Defender, Huir. Los comandos extra se añaden desde la DB, sin código.
- Tipos de objetivo: uno, todos, aleatorio N, fila, uno propio, todos propios, aliado caído.
- Formación, filas (delantera/trasera) con modificadores configurables.

### A2.2 Matemática transparente
- **Vista previa de daño** (opcional, activable): rango estimado antes de confirmar.
- **Registro de combate** (log) con desglose de cada golpe: base, elemento, crítico, guardia, varianza, resultado.
- **Panel de depuración** (solo editor, F11): muestra parámetros finales, traits activos, estados, valores de RNG y semilla.
- **Semilla de combate** fija opcional para reproducir un combate exacto.
- Todas las fórmulas editables por habilidad/objeto con validación (sección 9.9) y variables `a`, `b`, `v[n]`, `s[n]`.

### A2.3 Simulador de balance (editor)
Herramienta `ClockGearsBattleSimulator`:
- Elegir un grupo (de la DB) contra una tropa y ejecutar **N combates automáticos (ej. 1000)** con IA en ambos bandos.
- Reporta: % de victorias, turnos promedio, daño medio por habilidad, HP restante medio, habilidades más/menos usadas.
- Ejecuta en un hilo aparte con barra de progreso y botón de cancelar; sin congelar el editor.
- Exporta resultados a CSV.
- Comparación rápida: cambiar una variable y volver a simular.

### A2.4 Presentación (pixel art)
- Battlers estáticos o animados, con **animaciones por frames** (golpe, daño, muerte, entrada).
- **Animaciones de habilidades** hechas con sprites por frames + editor de animación con línea de tiempo, sonido y flash por frame. Sin interpolaciones borrosas.
- Números de daño en fuente pixelada, con efectos (rebote, color por tipo: daño, curación, crítico, fallo).
- Sacudida de pantalla y destello controlables en opciones (accesibilidad).
- Velocidad de combate ajustable (1x, 2x, 3x) y opción de saltar animaciones.
- **Eventos de batalla:** páginas por tropa con condiciones (turno, HP del enemigo, interruptor) que pueden mostrar texto, cambiar música o lanzar cinemáticas en pleno combate (clave para jefes narrativos).
- Final de combate: EXP, oro y botín con pantalla de resultados.

---

## A3. SISTEMA DE TIEMPO Y RUTINAS (Juego 2 — implementar en el NÚCLEO, Fase 1)

Clase `ClockGearsTimeManager` (singleton, guardada en la partida).

### A3.1 Reloj del juego
- Tiempo en **minutos de juego** (`int64_t`), con factor configurable (ej. 1 segundo real = X minutos de juego).
- Definición por proyecto: minutos por hora, horas por día, días por ciclo, nombres de los días (lista del usuario, vacía por defecto).
- **Modo ciclo** (estilo Majora's Mask): el juego tiene un ciclo de N días. Al llegar al final (o por comando) ocurre el **reinicio del ciclo**.
- Modos de avance: tiempo real, pausado en diálogos/menús/cinemáticas (configurable), avance por acciones, o manual por comando.
- Comandos de evento: Fijar hora, Avanzar tiempo, Pausar/Reanudar reloj, Cambiar factor, Reiniciar ciclo, Saltar a una hora.
- Condiciones de evento nuevas: hora entre X e Y, día N, periodo del día (amanecer, mañana, tarde, noche, madrugada, definibles).
- Señales C++: `hour_changed`, `day_changed`, `cycle_reset`.

### A3.2 Reinicio de ciclo (regla de persistencia)
Cada dato se clasifica en el editor como **Persistente** o **Se reinicia**:
- Interruptores, variables, objetos, equipo, dinero, nivel, habilidades, posiciones, estado de eventos, progreso de NPCs.
- Por defecto: todo **se reinicia** salvo lo marcado persistente.
- Panel "Reglas de ciclo" con lista filtrable y casillas. Ver una **vista de resumen** de qué se conserva.
- Al reiniciar: pantalla de transición configurable (evento común), carga del estado inicial del ciclo y reposición del jugador.

### A3.3 Rutinas de NPCs (`ClockGearsRoutineEditor`)
- Un NPC (evento) tiene una **tabla de rutina**: filas con intervalo de tiempo (día + hora inicio → fin), mapa, posición, acción (parado, caminar a, patrullar, página de evento a activar, animación) y condiciones.
- **Editor visual:** vista de horario tipo calendario (días x horas) por NPC, con bloques arrastrables y colores por ubicación.
- **Vista global:** ver todos los NPCs en una hora concreta, y un **simulador de tiempo** que avanza el reloj y muestra dónde está cada uno.
- Movimiento entre ubicaciones: el NPC **camina de verdad** hacia su destino con pathfinding (A*) y llega a tiempo; si el jugador no está en el mapa, se calcula su posición teóricamente (sin simular cada paso).
- Estados especiales: duerme, trabaja, desaparece, solo existe en ciertos días.
- Las rutinas pueden **cambiar según banderas** (ej. si el jugador ayudó a alguien, el NPC cambia su horario).
- Linter de rutinas: avisa de huecos de horario, NPCs en dos lugares a la vez, destinos bloqueados o inalcanzables.
- Al reiniciar el ciclo vuelven a su rutina inicial salvo marcas persistentes.

### A3.4 Pathfinding
- A* sobre la rejilla de pasabilidad del mapa, con costos por terreno, evitando eventos sólidos, caché y límite de nodos por frame.
- Conexiones entre mapas (puertas/salidas) como grafo para rutas entre mapas.

### A3.5 Eventos y mundo según la hora
- Páginas de evento con condición de horario.
- Cambios de iluminación y paleta por hora (tono de pantalla con curva, luces puntuales en pixel art sin sombras suaves).
- Música y ambiente por hora.

---

## A4. REGLAS DE PIXEL ART (obligatorias en todo el motor)

### A4.1 Renderizado
- Resolución base **16:9**, valor por defecto **960x540** (30 tiles de ancho y 16,875 de alto con tiles de 32 px; la cámara maneja la fila parcial). Escala por enteros: 2x a 1080p y 4x a 4K. En pantallas que no den un entero exacto (ej. 1440p) usar el entero menor con barras negras. Configurable por proyecto; alternativas en la lista: 640x360 (3x a 1080p, 2x a 720p, 4x a 1440p, 6x a 4K), 512x288 (16x9 tiles exactos).
- Modo de ventana: `canvas_items` + aspecto `keep`, **escalado entero** (`integer scaling`) con barras negras si hace falta.
- Filtro de textura por defecto `NEAREST` en el proyecto, importadores y la UI del juego.
- `snap_2d_transforms_to_pixel` y `snap_2d_vertices_to_pixel` activados.
- **Cámara pixel-perfect:** la posición de la cámara se redondea al píxel; seguimiento suave sin temblor de subpíxel.
- Movimiento de sprites a píxeles enteros en el render (guardar posición float, dibujar redondeada).
- Sin rotación ni escala fraccionaria de sprites por defecto (opción explícita con aviso).

### A4.2 Tiles y atlas
- Tamaño de tile por defecto **32x32** (configurable por proyecto) y verificación al importar: avisar si un tileset o sprite no es múltiplo de 32.
- **Sin sangrado de bordes:** padding/extrusión de 1 px en atlas generados, y avisos si el atlas no es múltiplo del tile.
- Importación de sprites sin compresión con pérdida, sin mipmaps, sin filtro.
- Tilesets animados (agua, fuego) por frames con temporizador común.

### A4.3 Texto y UI
- Fuentes **bitmap o vectoriales con hinting desactivado y antialiasing apagado**, alineadas a píxel, en tamaños múltiplos de su diseño.
- La UI del juego usa escala entera y `NinePatch` pixelados.
- Cajas de diálogo en 9-slice sin suavizado.

### A4.4 Paleta y efectos
- Efectos de pantalla (viñeta, tono, niebla) con **cuantización opcional a paleta** para mantener la estética.
- Opción de **shader de paleta/dither** (opcional por mapa).
- Sin sombras ni luces suaves que rompan el pixel art; luces como máscaras pixeladas con dither.

### A4.5 Herramientas del editor para pixel art
- Vistas previas siempre en `NEAREST` y con zoom entero.
- Rejilla de píxeles visible a zoom alto.
- Aviso al cargar imágenes con tamaño impar, canal alfa semitransparente o escalado no entero.

---

## A5. CAMBIOS AL ORDEN DE FASES (reemplaza la sección 15 del prompt maestro)

1. **Núcleo:** módulo, catálogo, IDs, fórmulas + parser + tests, DataManager, **TimeManager (A3.1–A3.2)**, ajustes de pixel art del proyecto (A4.1).
2. **Tema y UI base** (pixel art en la UI del juego y tema del editor).
3. **TilePicker + herramientas de mapa + pasabilidad + reglas de tiles pixel art (A4.2).**
4. **Base de datos:** esqueleto, Objetos, Equipo, Enemigos, luego Actores/Clases/Habilidades/Estados/Tropas.
5. **Editor de Eventos + CommandSelector + persistencia.**
6. **Runtime base:** jugador, cámara pixel-perfect, intérprete de eventos, seguidores.
7. **SISTEMA NARRATIVO COMPLETO:** ventana de mensajes, retratos, códigos de texto, historial, imágenes y pantalla, **AudioManager**, Cinemáticas.
8. **Combate por turnos** + simulador de balance + eventos de batalla.
9. **Tiempo y rutinas (A3.3–A3.5)** + pathfinding.
10. **Flora y fauna** (versión sencilla primero; sin spawn en chunks todavía).
11. **Guardado de partida, exportación, pulido de UI, revisión de la lista de bugs.**

Entrega por fases igual que en el prompt maestro (archivos, comando de compilación, checklist de prueba).

---

## A6. CRITERIOS DE ACEPTACIÓN ADICIONALES

- Se puede crear una **escena narrativa completa** sin escribir código: cinemática con cámara, retratos, texto con códigos de escape, música con crossfade y capas, imágenes, y elecciones que cambian banderas.
- Esa escena se puede **saltar** y deja el mundo en el estado correcto.
- Una **batalla** de 3 contra 3 muestra orden de turnos, log con desglose del daño y se puede simular 1000 veces desde el editor.
- Un NPC con rutina de **3 días** camina entre 2 mapas según la hora, y al reiniciar el ciclo vuelve a su estado inicial salvo lo marcado persistente.
- Todo se ve **nítido** a 1x, 2x, 3x y en pantalla completa 1080p/1440p, sin líneas entre tiles ni temblor de cámara.
- Todas las listas siguen **vacías** al abrir un proyecto nuevo.

---

## A7. PREPARACIÓN PARA EL JUEGO 3 (solo diseño, NO implementar ahora)

Para no bloquear el futuro, respeta estas reglas de arquitectura (sin escribir funcionalidad del juego 3):
- El **movimiento** del jugador va detrás de una interfaz (`ClockGearsMovementMode`): ahora solo existe el modo "rejilla" (y libre simple); más adelante se podrá añadir movimiento libre completo.
- El **combate** va detrás de una interfaz (`ClockGearsBattleMode`): ahora solo "por turnos"; más adelante se podrá añadir "dinámico".
- Los **mapas** se acceden mediante un `ClockGearsMapProvider` (hoy carga un mapa entero; mañana podrá cargar por trozos).
- El **TimeManager, la DB y el sistema narrativo** no deben asumir rejilla ni turnos.
- Nada de singletons con estado oculto: todo el estado de partida pasa por el sistema de guardado.

---

## A8. REGLAS PARA PROMPTS FUTUROS DE ARREGLOS Y MEJORAS

- Toda mejora o arreglo debe respetar **A4 (pixel art)** sin excepción.
- Si un cambio puede afectar a **la narrativa** (diálogos, cinemáticas, audio, imágenes), tiene prioridad y se prueba primero.
- Un cambio por pedido; indicar archivos tocados y cómo probarlo.
- Si se detecta una contradicción con el prompt maestro, avisar y **elegir lo que diga este anexo**.

**Empieza ahora por la FASE 1 de la sección A5**, sin hacer preguntas.
</USER_REQUEST>
<ADDITIONAL_METADATA>
The current local time is: 2026-11-06T13:51:51-06:00.
</ADDITIONAL_METADATA>