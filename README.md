# ⚙️ ClockGears Engine

![Estado](https://img.shields.io/badge/estado-En%20Desarrollo-brightgreen)
![Base](https://img.shields.io/badge/base-Godot%20Engine%20(C%2B%2B%20Fork)-478cbf)
![Estilo](https://img.shields.io/badge/diseño-RPG%20Maker%20MZ-purple)
![Plataforma](https://img.shields.io/badge/plataforma-Windows%20%7C%20Linux-blue)
[![Licencia](https://img.shields.io/badge/Licencia-ClockGears%20%2B%20MIT%20(Godot)-lightgrey)](LICENSE-CLOCKGEARS.txt)
![Versión](https://img.shields.io/badge/versión-Alpha--0.0.2-orange)

> **ClockGears Engine es gratuito en GitHub. Cualquier venta o redistribución no autorizada no es oficial.**


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

```bash
scons platform=windows target=editor arch=x86_64 -j8
```

Si solo quieres usar el motor listo para abrir y crear tu juego, descarga el ejecutable desde la sección de [**Releases**](https://github.com/krisztsa/ClockGears-Engine/releases).

---

## 📌 Versión actual

**Alpha-0.0.2**

---

## 🌐 Redes

* **GitHub:** https://github.com/krisztsa/ClockGears-Engine
* **YouTube:** https://www.youtube.com/@KrisZtS

---

## 🤝 Créditos

* **Kris'ZtS · ClockGears Studio**, sobre la base de código de Godot Engine.
* Licencia: ver [LICENSE-CLOCKGEARS.txt](LICENSE-CLOCKGEARS.txt) (ClockGears Studio) y la licencia MIT de Godot Engine.

**Creado en 2026.**

> **ClockGears Engine es gratuito en GitHub. Cualquier venta o redistribución no autorizada no es oficial.**