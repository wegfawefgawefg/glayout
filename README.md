# glayout

`glayout` is a small C++20 layout authoring library being split out from Gubsy's
runtime UI layout tool.

The target shape is:

- `glayout_core`: layout data, matching, and Lisp-style file parsing/writing.
- `glayout_editor`: runtime selection, drag, resize, undo/redo, and save requests.
- `glayout_imgui`: optional Dear ImGui panels for users that already have ImGui.

The current repo is an initial scaffold. The first real milestone is documented
in [docs/spec.md](docs/spec.md).

`glayout_core` uses the sibling `gsexp` library for S-expression parsing. By
default CMake looks for `../gsexp`, or you can provide an existing
`gsexp::gsexp` target before adding `glayout`.

Optional Dear ImGui helpers build when `GLAYOUT_WITH_IMGUI=ON`. Consumers can
provide an ImGui CMake target through `GLAYOUT_IMGUI_TARGET`, `imgui`, or
`ImGui::ImGui`. For simple source-tree integration, callers can also pass
`GLAYOUT_IMGUI_SOURCE_DIR=/path/to/imgui`.

The default `dev` preset enables ImGui for the SDL demo and fetches Dear ImGui
into the build directory if no ImGui source/target is provided. This is
demo-only; no ImGui sources are committed into `glayout`.

## Build

```sh
./scripts/build.sh
```

The build script also runs the core test executable through CTest.

## Run Demo

```sh
./scripts/run.sh
```

## Run SDL Demo

```sh
./scripts/run_sdl_demo.sh
```

In VS Code, F5 launches the SDL demo through `.vscode/launch.json`. With the
default `dev` preset, this is the SDL3 + ImGui demo path. The checked-in VS Code
debugger config uses `cppdbg`/`gdb`, so it is Linux-oriented; on macOS/Windows,
use your platform's debugger config or run `./scripts/run_sdl_demo.sh` from a
shell after installing SDL3.

SDL demo controls are documented in
[examples/sdl_demo/README.md](examples/sdl_demo/README.md).
Press `E` in the demo to toggle edit mode; the ImGui panels appear in edit mode.
