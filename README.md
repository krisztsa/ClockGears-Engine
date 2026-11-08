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