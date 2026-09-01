# PROJECT CHARTER: SUPER MARIO BROS. (1985) DEVELOPMENT PROJECT

**Course:** Programming Systems (CS202)  
**Development Team:** 4 Members  
**Timeline:** 6 Weeks (Including buffer time)  
**Tech Stack:** C++17, SFML Engine, CMake Build System  

---

## 1. Project Summary & Requirements

### 1.1. Context & Objectives

The project requires a team of 4 software engineering students to collaboratively design, develop, and complete a 2D Side-scrolling Platformer computer game that clones the classic **Super Mario Bros. (1985)** using C++ and the SFML graphics library. The ultimate goal of this project is to optimize the final score based on the official evaluation Rubric (100 core points + 15 bonus points for advanced features).

### 1.2. Evaluation Rubric & Grading Criteria

The game's features and architecture must fully satisfy the following grading components:

* **Functionality (65 Points):**
  * **Player Inputs, Movement, and Collision (20 points):** Keyboard input handling, rigid body kinematics/physics dynamics (running, jumping, acceleration), and accurate 2D axis-aligned bounding box (AABB) collision detection and resolution.
  * **Enemy Behavior (10 points):** Basic patrol Artificial Intelligence (AI) and Finite State Machines (FSM) to handle enemy entity behaviors.
  * **Power-Ups and Items (10 points):** A polymorphic item interaction system that dynamically modifies player attributes, dimensions, and abilities.
  * **3 Level Completion (15 points):** Core game loop design that supports progression through a minimum of 3 independent levels with ascending difficulty driven by configuration files.
  * **Sounds (10 points):** Seamless integration of background music (BGM) tracks and interactive sound effects (SFX).
* **Design & Implementation (35 Points):**
  * **Object-Oriented Design (10 points):** Rigid adherence to the 4 pillars of OOP (Encapsulation, Inheritance, Polymorphism, Abstraction).
  * **5 Design Patterns Check (25 points):** Precise and effective integration of at least 5 software design patterns to achieve high modularity.
* **Advanced Features for Bonus Credits (15 Max Points):**
  * Advanced enemy/Boss AI (5 points).
  * Multiple Player Characters system (5 points).
  * 3D graphics rendering implementation (5 points).

> 🎯 **Team Score Strategy:** The team unifies the objective to target **110/115 points**. We will secure 100% of the core points and claim **10 bonus points from the advanced Boss AI (World 3) and the Multiple Player Characters system (Mario/Luigi Selection)**. The team has decided to **omit the 3D graphics requirement** to minimize architectural risks and ensure maximum technical quality for our grid-based physics and core serialization systems.

---

## 2. Architectural Framework & Design Concepts

To secure maximum points in OOP Design and the 5 Design Patterns, the project's codebase is structured around a robust polymorphic hierarchy and a loosely-coupled system architecture.

### 2.1. Core OOP Hierarchy

All active in-game entities are managed centrally using standard vectors of base abstract class pointers to leverage maximum polymorphism:

* `Character` (Abstract Class): Defines shared attributes like position, velocity, bounding boxes, and pure virtual methods `update()` and `render()`.
  * `Player` (Abstract Class): Extends the base class with input handling via the Command Pattern and virtual locomotion functions.
    * `Mario`: Implements the standard velocity, gravity, and jumping force parameters of the original 1985 game.
    * `Luigi`: Polymorphically overrides locomotion parameters to achieve higher jumping force (higher jumps) but lower maximum running speed and higher sliding inertia due to a reduced ground friction coefficient.
  * `Enemy` (Abstract Class): Encapsulates behavior state machines and the virtual `updateAI()` method.
    * `Goomba`: Standard left/right patrol AI on grid surfaces, changing direction automatically upon colliding with tiles or pipes.
    * `KoopaTroopa`: Logic to retreat into its shell upon the first stomp, transforming into a high-velocity sliding shell weapon that inflicts area-of-effect damage on subsequent impacts.
    * `Bowser (World 3 Boss)`: Advanced AI that dynamically tracks the Player’s X-coordinate to execute leaps and breathes fireballs on periodic intervals.
* `Item` (Abstract Class): Declares the pure virtual method `activate(Player* player)` for entity-player interactions.
  * `Mushroom`: Triggers the player's transformation into Super Mario by modifying their physical collision Bounding Box.
  * `FireFlower`: Transitions the player into Fire Mario state, unlocking the capability to shoot physical bouncing fireballs.
  * `Coin`: Directly increments the player's score via the Head-Up Display (HUD) system and fires an interactive sound effect.

### 2.2. Mapping 5 Design Patterns into Core Systems

1. **Singleton Pattern (`AssetManager`, `SoundController`):** Guarantees a single global instance controls the resource buffers (`sf::Texture`, `sf::Font`, `sf::SoundBuffer`), entirely preventing memory leaks or frame rate drops (FPS lag) caused by redundant disk I/O operations.
2. **State Pattern (`GameStateManager`):** Encapsulates and coordinates fluid screen transitions between distinct game states: `IntroMenuState`, `CharacterSelectionState` (Mario/Luigi choice screen), `PlayState`, `PauseState`, and `GameOverState`.
3. **Factory Method Pattern (`EntityFactory`):** Automatically parses string/char matrices from level text files (e.g., 'G' for Goomba, 'M' for Mushroom) to instantiate concrete entities at runtime, eliminating hardcoded subclass allocations.
4. **Command Pattern (`InputHandler`):** Binds keyboard inputs into independent command objects (`JumpCommand`, `MoveCommand`). This decouples key event polling from entity logic, allowing smooth, unified control translation for any active character derived from the `Player` class.
5. **Observer Pattern (`EventSystem`):** Establishes a loosely-coupled communication layer. When Mario stomps an enemy or collects an item, an event notification is broadcasted; the HUD system updates scores, and the Sound system plays SFX independently without direct cross-module compilation dependencies.

### 2.3. File Handling & Serialization

* **Game Progress Save/Load:** Utilizes standard file streams (`std::ofstream` and `std::ifstream`) to serialize and deserialize player metadata (Score, Lives, Current Level, Selected Character) into local plain text `.txt` files upon saving, allowing instant restoration during state loading.
* **Custom Level Editor (Bonus Feature):** Enables the player to place tiles and spawn entities through a graphical user interface (GUI). The system then serializes the 2D grid matrix into a text-based map configuration file, ready to be hot-loaded anytime via the `EntityFactory`.

---

## 3. Role Breakdown & Detailed Roadmap

### 3.1. RACI Matrix Mapping

| Role / Responsibility | Hồng Đăng (PM) | Hải Đăng (Physics Lead) | Quốc Huy (AI & Map Lead) | Đại Nghĩa (UI & Sound) |
| :--- | :---: | :---: | :---: | :---: |
| **Core Architecture & Management** (CMake, Gitflow, Game Loop, State Pattern) | **R / A** | I | I | R |
| **Physics & Kinematics** (Euler Physics, AABB Collision, Command Pattern) | C | **R / A** | C | I |
| **Map & Entity Management** (Map Parsing, 3 Levels, Factory Method, Items) | R | C | **R / A** | I |
| **Artificial Intelligence** (Goomba Patrol AI, Advanced Bowser Boss AI) | I | C | **R / A** | I |
| **UI, Animation & File Systems** (Save/Load Progress, Animations, HUD, Observer) | **R / A** | I | R | **R / A** |

*Note: R (Responsible) - Action Executer; A (Accountable) - Final Reviewer/Sign-off; C (Consulted) - Subject Matter Expert Advisor; I (Informed) - Updated on Progress.*

### 3.2. Weekly Execution & Milestone Plan

#### Week 1: Design Foundations & Base Architecture

* **TV1 (PM):** Initialize CMake configurations, setup Gitflow workflows, and write a production-ready `.gitignore`. Program the core `Game Loop` with Delta Time regulation and implement the `GameStateManager` (**State Pattern**).
* **TV2:** Construct the `InputHandler` (**Command Pattern**) for input polling and decoupling. Draft the base abstract interfaces for the `Player` hierarchy.
* **TV3:** Define the class hierarchy and base attributes for `Character` and `Item` to satisfy strict OOP encapsulation.
* **TV4:** Finalize the centralized resource subsystem `AssetManager` (**Singleton Pattern**) and import raw graphical textures, sprites, and typography fonts into the `assets/` directories.
* **Milestone:** Application window initializes successfully, displaying seamless screen switching between the Main Menu and the Character Selection Screen.

#### Week 2: Kinematics Loco-motion & Level Map Loading

* **TV1 (PM):** Code the matrix text parser module to map text characters into 2D grid coordinates. Build the state transmission bridge to carry selected character data into the gameplay scene.
* **TV2:** Program the explicit Euler physics integration algorithms (handling acceleration, dampening, friction, and gravity coefficients) for the concrete `Mario` and `Luigi` classes.
* **TV3:** Script the baseline horizontal boundary-patrol behavior logic for the entry-level enemy `Goomba`.
* **TV4:** Implement the custom `Animation` class responsible for cutting sprite sheets and syncing playback speeds with the physics data of both character skins.
* **Milestone:** Character choice persists past menus; the chosen hero navigates with accurate, individual physics attributes on top of a static grid canvas.

#### Week 3: Collision Resolution & System Integration (The Core Challenge)

* **TV1 (PM):** Construct the File I/O architecture skeleton, drafting file models for session state serialization (Score, Lives, Character State).
* **TV2:** **Focus entirely on solving the biggest technical bottleneck:** Engineer the 2D **AABB Collision Detection and Resolution** algorithm to calculate penetration vectors, ensuring characters stand firmly on ground blocks without snapping or clipping through geometry.
* **TV3:** Realize the `EntityFactory` (**Factory Method Pattern**) and connect it with TV1’s map parser to stream dynamic entity spawning.
* **TV4:** Deploy the central internal event dispatcher `EventSystem` (**Observer Pattern**).
* **Milestone:** The physics engine reaches stability. Entities stand on solid grounds; upward head collisions with Question Blocks change the block state and spawn items.

#### Week 4: Core Gameplay Loop & Camera Scrolling

* **TV1 (PM):** Develop the camera translation tracking engine (Horizontal Camera Scrolling) to slide the viewport viewport view bounding rect relative to the active player's X-coordinate.
* **TV2:** Write combat collision resolution paths (Mario jumping on enemy bounds triggers enemy crush/death logic; enemy side-impact flags player damage or state shrinkage).
* **TV3:** Realize polymorphic behaviors for item interactions (Super Mushrooms scale bounding box scales up; Fire Flowers swap state variables to enable projectile firing).
* **TV4:** Layout and render HUD display vectors (rendering lives, current score digits, countdown timers) and hook them to the `EventSystem` for thread-safe UI updates.
* **Milestone:** A polished, fully playable World 1-1 Beta build is achieved with absolute win/loss state conditions.

#### Week 5: Multi-Level Scaling, Data Persistence & Boss AI

* **TV1 (PM):** Scale text assets to generate configuration files for **World 1, World 2, and World 3** with steep difficulty arcs; finalize text stream serialization for the Save/Load feature.
* **TV2:** Optimize locomotion routines and help TV3 format vectors for the level designer layout.
* **TV3:** Architect multi-phase behavior code for the Bowser Boss AI in World 3 (dynamic physics tracking, random fireball projectiles - **5 Bonus Points**) and finish the Custom Level Editor UI.
* **TV4:** Map out and wire up the complete SFX directory and ambient soundtrack loops to corresponding game engine event hooks.
* **Milestone:** Feature freeze. 100% of required specifications are integrated. The game streams smoothly across 3 consecutive levels with stable data saving/loading.

#### Week 6: Performance Optimization, Documentation & Release

* **All Members:** Execute continuous regression playtesting to identify edge-case bugs. Clean up memory allocations, swapping remaining raw pointers for smart pointers (`std::unique_ptr` or `std::shared_ptr`) to eliminate memory leaks and guarantee a rock-solid 60 FPS output.
* **TV1 + TV2:** Model the comprehensive project **Class Diagram** illustrating clear inheritance trees and structural patterns.
* **TV3 + TV4:** Model runtime **Sequence Diagrams** showcasing design pattern orchestration and capture a high-definition video demonstration.
* **Milestone:** Ship the software package. Codebase, documentation dossiers, and compiled builds are archived and ready for presentation.

---

## 4. Risk Management & Team Cooperation Policy

1. **Strict Gitflow Enforcement:** Direct commits to the long-lived `main` or `develop` branches are strictly prohibited. Every individual workflow feature must isolate itself on localized shorts branches (`feature/physics`, `feature/tilemap`, `feature/ui-audio`) branching out from `develop`. Code merging is exclusively permitted via a Pull Request (PR) after getting code-review sign-off from the Project Manager (TV1).
2. **Explicit Design Pattern Annotation:** Since architecture carries heavy grading weight, engineers must prominently label applied patterns using standardized header source comments at the top of the files (Example: `// DESIGN PATTERN: FACTORY METHOD APPLIED FOR DYNAMIC ENTITY SPAWNING`).
3. **Decoupled Architecture for Persistence:** Avoid mixing file system operations (`ifstream/ofstream`) inside entity updates or rendering pipelines. All level parsing and progress tracking serialization must live inside localized helper utility modules to guarantee proper Encapsulation.
4. **Safe Pointer Management:** Because the engine coordinates a massive pointer stack of polymorphic `Character*` and `Item*` references, memory deallocation when objects are flag-deleted must be handled defensively. The team highly encourages utilizing `std::unique_ptr` structures wherever applicable to avoid dangling references or catastrophic runtime application crashes.

---

## 5. Conclusion

This Project Charter serves as the definitive engineering compass and structural commitment for the entire team across our 6-week schedule for Super Mario Bros. (1985). Every development resource is expected to maintain absolute alignment with these timeline constraints, clean-code rules, and role delineations to deliver an outstanding software product that achieves top marks.
