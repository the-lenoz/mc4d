# mc4d

`mc4d` is a small Minecraft-like 4D voxel renderer written in C++ and OpenGL. The project renders hypercubes, projects them into 3D, and then draws the result to the screen with a regular 3D camera.

This fork is focused on turning the original viewer into an explorable first-person prototype. It now has FPV camera control, walking physics, jumping, sprinting, player collisions, and chunk loading around the player.

![mc4d](screenshots/mc4d.gif)

## Current State

Implemented:

* First-person view with mouse look.
* Walking mode with gravity, jumping, sprinting, damping, and ground checks.
* Free-fly mode for debugging and easier exploration.
* Player collision against solid 4D blocks using a player-sized AABB.
* Axis-by-axis collision resolution to prevent clipping through blocks.
* Surface spawn search so the player starts above terrain.
* Dynamic square-world chunks generated and loaded around the player.
* Procedural 4D terrain with grass, stone, sand, water, wood, leaves, trees, and simple wood tesseract structures.
* Optional 4D slice mode.
* Original round-world preview mode and hypercube wireframe overlay.
* Skybox, water pass, block shading, and instanced hypercube rendering.

Not implemented yet:

* Block placing and breaking.
* Inventory or survival mechanics.
* Collisions in round-world mode. Player collision is currently used for the square world.
* Legacy scene autorotation controls. The toggle flags exist in code, but the rotation update is not wired into the current movement loop.
* Full gameplay loop.

## Controls

### Basic

| Key | Action |
| --- | --- |
| `Esc` | Exit |
| Mouse | Look around |
| `Tab` | Toggle free-fly mode |
| Double `Space` | Toggle free-fly mode |
| `0` | Reset legacy 4D scene rotations and slice angle |

### Walking Mode

Walking mode is the default mode.

| Key | Action |
| --- | --- |
| `W` / `S` | Move forward / backward |
| `A` / `D` | Strafe left / right |
| `Q` / `E` | Move through the W axis |
| `Space` | Jump |
| `Left Ctrl` | Sprint |

### Free-Fly Mode

| Key | Action |
| --- | --- |
| `W` / `S` | Fly forward / backward |
| `A` / `D` | Fly left / right |
| `Space` | Fly up |
| `Left Shift` | Fly down |
| `Q` / `E` | Fly through the W axis |
| `Left Ctrl` | Increase movement speed |

### 4D View

| Key | Action |
| --- | --- |
| `R` / `F` | Look/rotate through the W direction, or rotate the slice in slice mode |
| `G` | Reset W-look or slice rotation |
| `M` | Toggle 4D slice mode |
| `[` / `]` | Decrease / increase field of view |

### Display Toggles

| Key | Action |
| --- | --- |
| `;` | Toggle square world / round world |
| `,` | Toggle example hypercube wireframe |
| `.` | Toggle block rendering |
| `'` | Toggle water visibility |

## Building

Install the required native libraries:

* GLEW
* GLFW 3
* GLM
* libpng
* `xxd`, used by the Makefile to embed GLSL shader files into generated C headers

Build:

```sh
make
```

Run:

```sh
make run
```

Clean generated objects, dependency files, shader headers, and the executable:

```sh
make clean
```

The executable is written to `./mc4d`.

## Code Layout

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Application setup, input, camera, walking/free-fly movement, collisions during movement, rendering loop |
| `src/world.cpp`, `src/world.h` | Square-world terrain generation, chunk cache, async chunk loading, collision sampling, spawn search |
| `src/roundworld.cpp`, `src/roundworld.h` | Original finite round-world generator |
| `src/tesseract.cpp`, `src/tesseract.h` | Tesseract vertex and wireframe geometry |
| `src/project.cpp`, `src/project.h` | 4D-to-3D projection math |
| `src/*glsl` | OpenGL shaders |
| `src/blocktype.h` | GPU upload and binding for per-block-type instance locations |
| `src/skybox.h`, `src/readpng.*` | Skybox loading and PNG support |

## How Movement Works

The player position is stored as a 4D eye position: `x`, `y`, `z`, and `w`. Walking mode keeps horizontal velocity on `x`, `z`, and `w`, applies gravity on `y`, and performs movement in small substeps. Each substep is tested axis by axis against `World::collidesWithPlayer()`.

The collision shape is a player-sized 4D AABB around the eye position. Solid blocks are sampled from the procedural world through `World::worldSample()`, so collisions work against terrain, trees, and generated wood structures without needing a separate physics mesh.

Free-fly mode uses the same camera basis and keeps square-world collision checks, but it ignores gravity and allows vertical movement with `Space` / `Left Shift`.

## Credits

The project is based on the original `mc4d` renderer by Michael Layzell. The code still uses the original 4D projection approach, simplex noise terrain generation, OpenGL rendering pipeline, framebuffer-based water pass, and skybox resources described in the upstream project.

Useful references used by the original project:

* Steve Hollasch's 4D projection math: <http://steve.hollasch.net/thesis/index.html>
* 4D simplex noise paper: <http://webstaff.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf>
* GLFW documentation: <https://www.glfw.org/docs/latest/>
* OpenGL Wiki framebuffer examples: <https://www.opengl.org/wiki/Framebuffer_Object_Examples>
