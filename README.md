![OnionVoxelTitle](assets/textures/OnionVoxelTitle.png)

# OnionVoxel

**OnionVoxel** is a multiplayer voxel-based sandbox game written in modern **C++**, basically a *Minecraft* clone.
The project focuses on building a lightweight, custom game engine from the ground up with a strong emphasis on graphics programming and engine architecture.

---

## 🎯 Project Goals

* Learn how to design and implement a game engine from scratch
* Deepen knowledge of modern C++ and real-time graphics
* Understand OpenGL rendering pipelines and GPU concepts
* Explore multiplayer architecture fundamentals
* Experiment custom User Interface (UI) and input systems
* Have fun experimenting and iterating

---

## 🛠 Tech Stack

Core technologies powering the engine:

* **C++** – Main language (modern standard)
* **GitHub** – Version control and project management
* **CMake** – Build system
* **OpenGL** (via glad) – Rendering API

---

## 📦 Dependencies

### Internal Modules

Custom utilities:

* `onion::datetime`
* `onion::event`
* `onion::logger`
* `onion::timer`
* `onion::threadpool`
* `onion::threadsafequeue`


These modules provide foundational services such as logging, timing, event handling, ...

### External Libraries

* **GLFW** – Window creation and input handling
* **GLM** – Mathematics library (vectors, matrices, transformations)
* **STB Image** – Texture loading
* **ENet** – Networking library for multiplayer features
* **Dear ImGui** – Immediate mode GUI for debugging and tools
* **Cereal** – Serialization library for saving/loading game data and network messages
* **FastNoiseLite** – Procedural noise generation for terrain and world features
* **Miniz** - Compression library for reading textures from zip resource packs
* **Nlohmann JSON** - JSON library for configuration files and data serialization

---

## 🚀 Features

* **UI**: Custom UI with multiple menus and controls.
* **Singleplayer**: Local world generation and gameplay.
* **Multiplayer**: Basic client-server architecture for online play.
* **Player Movements and Actions**: Walk, Run, Jump, Fly, Break Blocks, Place Cobblestone, Collision, FreeCam
* **Resource Packs**: Support for loading textures from *Minecraft* .zip resource packs.
* **Skin Rendering**: Render the official Minecraft's player Skin depending on PlayerName
* **World Generation**: Chunk-based procedural terrain generation using noise functions.
* **World Saving/Loading**: Serialization of world data for persistence (Chunk-based).
* **Options Menu**: Configurable settings for graphics, controls, and gameplay. Options are saved.


* **UI**: Available menus include:
  * Main Menu
  * Singleplayer Menu (World selection, World Creation, World Deletion, Filter)
  * Multiplayer Menu (Server selection, Register server, Delete registered server, Direct Connection)
  * Options Menu (FOV, Video Settings, Controls, ResourcePacks)
	* Video Settings : Max Framerate, VSync, Render Distance
	* Resource Packs : Filter, ResourcePack selection, Open Pack Folder)
	* Controls (Mouse Settings, Key Binds)
	  * Mouse Settings : Sensitivity, Scroll Sensitivity
	  * Key Binds : Bind every Action to any Key.
  * Pause Menu (Back to game, Options, Save and quit to title)

* **World Generation**: Multiple world generation options available
  * Superflat: Flat world with trees and grass.
  * Classic No Biomes: Montains, Forest and Sea, but no ther biomes.
  * Classic: Biomes : Ocean, Desert, Snow, Plains, Mountains
  * BiomeVisualizer: Superflat version of "Classic"

---

## 🔨 Building

* WIP

---

## 📄 License

Well, since it use the official Minecraft's textures, that seems illegal.
Please don't tell Mojang about this project...

No commercial intent of course.
That thing is not even playable yet, nobody would be interested in it watsoever.

---
