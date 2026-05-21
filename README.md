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

In VS Code, F5 launches the SDL demo through `.vscode/launch.json`.
