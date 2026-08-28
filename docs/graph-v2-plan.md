# GLayout graph evolution plan

## Goal

Preserve GLayout as a small, renderer/input/widget-independent library while
adding a compiled hierarchical geometry graph suitable for GView and for
projects that only want stronger layout calculation.

The full ecosystem contract lives in the neighboring GView repository at
`docs/MASTER_PLAN.md`. This document records GLayout's narrower responsibility.

## Milestone status

The graph target is implemented and independently tested. It provides dense
compiled hierarchy, all listed container kinds, sizing and intrinsic
measurement, min/max/aspect constraints, named anchors with cycle diagnostics,
safe-area resolution, clip propagation, content extents, responsive graph-store
selection, clean-frame caching, S-expression persistence, structural edits,
nested template repetition, and snapshot undo/redo. GView supplies the live
composed editor and uses these renderer-free operations rather than duplicating
them.

Dirty invalidation currently resolves the compact graph as one deterministic
domain after a geometry change; clean frames do no layout work. Trial-sized
dirty resolves are far below the one-millisecond target. A dependency-indexed
partial resolver should be added only when a larger real UI demonstrates that
whole-graph dirty resolution is material, not as speculative complexity.

The renderer-free graph-canvas editing domain is now implemented and composed
by GView on the real native canvas. It supplies selection and multi-selection,
center movement, eight resize handles, grid and true-sibling snapping, guides,
nudge, transaction boundaries, anchored edits, constraint-aware flow resize and
reorder, and selection bounds. Focus and presentation remain outside GLayout.
The composed workflow is ready for user review; its acceptance contract lives
in `../../gview/docs/AUTHORING_PRESENTATION_PLAN.md`.

## Existing contract

Current GLayout stores flat normalized rectangles, selects resolution/form-factor
variants, persists S-expressions, and provides renderer-free plus optional ImGui
editing. Existing callers and files must continue to work during migration.

The existing flat model becomes a compatibility frontend and an absolute-layout
leaf representation. It is not silently deleted.

## New graph target

Add an independent target initially:

```text
glayout::core
  existing flat model, persistence, lookup, and editor

glayout::graph
  nested nodes, constraints, containers, measurement, and compilation
```

The graph supports:

- Stable authored UUID plus readable alias.
- Dense compiled runtime index.
- Parent/child hierarchy and reusable templates.
- Absolute, row, column, grid, stack, and overlay containers.
- Fixed, proportional, intrinsic, fill, min/max, and aspect sizing.
- Parent, sibling, safe-area, and named-node anchors.
- Padding, gaps, alignment, and distribution.
- Repeated geometry templates driven by a count.
- Scroll viewport/content extent geometry without owning scroll state.
- Rectangular clip propagation and mask bounds/identity.
- Resolution, aspect, form-factor, and DPI variants.
- Dirty-subtree recomputation and deterministic diagnostics.

## Non-responsibilities

GLayout does not own:

- Rendering or renderer command types.
- Fonts, text shaping, images, or assets.
- Buttons, controls, or widget state.
- Pointer/controller input or hit testing policy.
- Focus/navigation graphs.
- Scroll offset, inertia, or virtualized item materialization.
- Application state, events, actions, or model binding.
- ImGui in required targets.

GLayout requests intrinsic sizes from a caller callback and returns geometry.
The callback identity and measured dimensions are opaque to GLayout.

## Persistence

Extend the existing S-expression family rather than replacing it with a
different mandatory format. Parsing must preserve source diagnostics and reject
invalid cycles or references explicitly.

Authored files should remain readable. A compiled binary cache may be added for
shipping/load performance, but the S-expression remains canonical source.

## Editor evolution

Renderer-free editor operations should cover:

- Selection and multi-selection.
- Move, resize, nudge, snapping, and numeric edits.
- Create, duplicate, delete, and reparent.
- Container conversion and constraint editing.
- Layout variant creation and copying.
- Transactional undo/redo.
- Atomic save and restore.

Optional ImGui UI remains an adapter over those operations. GView may compose
the editor with presentation/focus tools, but GLayout does not depend on GView.

The graph-canvas pass preserves the useful behavior already present in
`include/glayout/editor.hpp`, `src/editor.cpp`, `src/editor_overlay.cpp`, the SDL
demo, and Gubsy's `src/layout_editor`: center/edge/corner manipulation,
multi-select, grid and sibling snapping, nudging, copy/paste, undo/redo, numeric
editing, and persistence. Preserve the workflow rather than blindly retaining
weak implementation details.

Direct edits map back to the authored layout kind. Absolute nodes may edit
rectangles, while stack/grid/row/column nodes edit tracks, order, sizing, gaps,
or alignment; anchored nodes edit anchor offsets; responsive edits target the
explicit active variant. The editor must never silently flatten constraints to
make dragging easy.

GLayout supplies renderer-free manipulation and snapping. GView supplies
semantic widget-part selection and focus authoring. Gubsy hosts input, native
overlays, display simulation, and persistence integration.

## Performance

Measure flat lookup, graph compile, first resolve, stable resolve, value-neutral
queries, dirty-leaf changes, dirty-container changes, and memory separately.

Targets:

- No work for a clean graph query beyond returning cached results.
- No allocation during clean or ordinary dirty resolution after capacity is set.
- Normal dirty layout well below 1 ms for trial-sized graphs.
- Runtime structures use dense arrays and explicit indices.

## Code quality

- Roughly 300-500 lines maximum per source file.
- Cohesive domains with mostly flat organization.
- Terse what-is comments above paragraph blocks.
- Plain structs and explicit functions over deep class hierarchies.
- Existing tests remain; new behavior receives focused unit tests.
