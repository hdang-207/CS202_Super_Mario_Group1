# 🍄 Super Mario Bros. (1985) - C++17 & SFML 3 Implementation

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![SFML](https://img.shields.io/badge/SFML-3.0-green.svg)](https://www.sfml-dev.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15%2B-orange.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

> A modern C++17 2D Side-scrolling Platformer recreating the iconic **Super Mario Bros. (1985)** game using **SFML 3**, developed for the **Programming Systems (CS202)** course.

---

## 📌 Table of Contents

- [🍄 Super Mario Bros. (1985) - C++17 \& SFML 3 Implementation](#-super-mario-bros-1985---c17--sfml-3-implementation)
  - [📌 Table of Contents](#-table-of-contents)
  - [📖 Overview](#-overview)
  - [⭐ Key Features](#-key-features)
  - [🏗️ Design Patterns \& Architecture](#️-design-patterns--architecture)
  - [🛠️ Tech Stack \& Prerequisites](#️-tech-stack--prerequisites)
    - [Technologies](#technologies)
    - [Prerequisites](#prerequisites)
  - [🚀 Installation \& Building](#-installation--building)
    - [1. Clone the Repository](#1-clone-the-repository)
    - [2. Configure \& Build](#2-configure--build)
    - [3. Run the Game](#3-run-the-game)
  - [🎮 Controls](#-controls)
  - [📁 Project Directory Structure](#-project-directory-structure)
  - [👥 Development Team (CS202 - Group 1)](#-development-team-cs202---group-1)
  - [📜 License](#-license)

---

## 📖 Overview

This project is a complete C++ Object-Oriented Programming (OOP) recreation of classic Super Mario Bros. (1985). Built with **SFML 3** and **CMake**, it showcases clean game architecture, a custom physics engine (Euler integration & AABB collision resolution), a fixed timestep game loop, and software design patterns.

---

## ⭐ Key Features

- **🎮 Multiple Playable Characters (Mario & Luigi)**:
  - **Mario**: Balanced velocity, standard gravity, and classic jump dynamics.
  - **Luigi**: Higher jump force with distinct physics (higher Y-acceleration, lower friction, and longer slide inertia).
- **🗺️ Multi-Level Gameplay & Level Editor**:
  - 12 stages across three world routes, loaded dynamically from text-based grid files.
  - Level Editor allowing custom placement of tiles, blocks, and entities.
- **👾 Enemy AI & Boss Mechanics**:
  - **Goomba**: Patrol AI responding to obstacles and map bounds.
  - **Koopa Troopa**: Retracts into shell on stomp; shell can be launched as a high-speed moving projectile.
  - **Bowser (Level 3 Boss)**: Advanced AI targeting player coordinates, jumping dynamically, and launching fireball attacks.
- **🍄 Power-Ups & Interactive Items**:
  - **Super Mushroom**: Dynamically resizes character sprite and bounding box.
  - **Fire Flower**: Unlocks physics-driven fireball attacks.
  - **Coins & Blocks**: Interactive question blocks, breakable bricks, and score accumulation.
- **💾 Save & Load System**:
  - Persistent game state serialization (Score, Remaining Lives, Current Level, Selected Character).
- **🎵 Dynamic Audio & Graphics**:
  - Background music (BGM) & interactive sound effects (SFX) powered by SFML Audio.
  - High-DPI support for crisp display rendering (including macOS Retina screens).

---

## 🏗️ Design Patterns & Architecture

The application is engineered using **5 core GoF Design Patterns**:

1. **Singleton Pattern (`AssetManager`)**: Centralized resource management & caching (`sf::Texture`, `sf::Font`, `sf::SoundBuffer`) to eliminate memory leaks and redundant disk I/O.
2. **State Pattern (`GameStateManager`)**: Coordinates smooth state transitions (`IntroMenuState`, `CharacterSelectionState`, `PlayState`, `PauseState`, `GameOverState`).
3. **Factory Method Pattern (`EntityFactory`)**: Dynamically parses map grid symbols (e.g., `'G'` $\rightarrow$ Goomba, `'M'` $\rightarrow$ Mushroom) to instantiate game objects without hardcoding.
4. **Command Pattern (`InputHandler`)**: Decouples physical input key presses (`JumpCommand`, `MoveCommand`) from player entities, supporting unified input mapping for both Mario and Luigi.
5. **Observer Pattern (`EventSystem`)**: Loosely coupled event bus notifying HUD updates and SFX playback upon enemy stomps or item collection.

---

## 🛠️ Tech Stack & Prerequisites

### Technologies

- **Language**: C++17
- **Graphics & Audio**: SFML 3.0 (Graphics, Window, System, Audio)
- **Build System**: CMake 3.15+
- **Platform Support**: Windows, macOS, Linux

### Prerequisites

- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+ supporting C++17.
- **CMake**: `3.15` or higher.
- **SFML 3.0**: Installed on system or pointed via `SFML_DIR`.

---

## 🚀 Installation & Building

### 1. Clone the Repository

```bash
git clone https://github.com/hdang-207/CS202_Super_Mario_Group1.git
cd CS202_Super_Mario_Group1
```

### 2. Configure & Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

> **Note for Windows**: CMake automatically copies required SFML DLLs and the `assets/` directory to `build/bin/` after compilation.

### 3. Run the Game

```bash
# Windows
./bin/SuperMarioBros.exe

# macOS / Linux
./bin/SuperMarioBros
```

---

## 🎮 Controls

| Action | Key 1 | Key 2 |
| :--- | :---: | :---: |
| **Move Left** | `A` | `Left Arrow` |
| **Move Right** | `D` | `Right Arrow` |
| **Jump** | `W` / `Space` | `Up Arrow` |
| **Run / Fireball** | `J` / `Shift` | `Z` |
| **Pause Game** | `P` / `Esc` | - |
| **Save Progress** | `S` | - |

---

## 📁 Project Directory Structure

```text
CS202_Super_Mario_Group1/
├── CMakeLists.txt              # CMake build configuration
├── main.cpp                    # Application entry point
├── LICENSE                     # MIT License
├── PROJECT_REPORT.md           # Technical report & class diagrams
├── Super_Mario_Project_Charter.md # Project charter & specification document
├── assets/                     # Game assets (textures, audio, fonts, maps)
│   ├── audio/                  # SFX and background music
│   ├── fonts/                  # UI fonts
│   ├── maps/                   # Text-based level grid files (.txt)
│   └── textures/               # Sprite sheets and backgrounds
├── include/                    # C++ Header files (.hpp)
│   ├── Core/                   # Core definitions & character types
│   ├── Entities/               # Game objects (Player, Enemies, Clouds, Blocks)
│   ├── Input/                  # Command pattern & input mapping
│   ├── Items/                  # Power-up items (Mushroom, Fire Flower, Coin)
│   ├── Physics/                # Collision & movement physics
│   ├── States/                 # Game state machine (Menu, Play, Pause, etc.)
│   └── Systems/                # AssetManager, MapParser, EventSystem
└── src/                        # Implementation files (.cpp)
```

---

## 👥 Development Team (CS202 - Group 1)

| Member | Role | Primary Responsibilities |
| :--- | :--- | :--- |
| **Hồng Đăng** | Project Manager & Core Architect | CMake setup, Game Loop, State Pattern, Map Parser, Save/Load System, Camera |
| **Hải Đăng** | Physics Lead | Euler Physics (Mario/Luigi), AABB Collision Resolution, Command Pattern |
| **Quốc Huy** | AI & Map Lead | Tilemap Parsing, 3 Level Designs, Factory Method, Enemy AI & Bowser Boss |
| **Đại Nghĩa** | UI & Sound Lead | AssetManager (Singleton), Animation System, HUD, Observer Pattern (EventSystem) |

---

## 📜 License

This project is open source under the [MIT License](LICENSE).
