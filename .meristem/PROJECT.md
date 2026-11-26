# Unscene

Standalone native OBS combined scene/source dock. Independent expand, activate and source selection; hover actions and second-row tools; cached thumbnails, no forced activation. Theme-aware and no dependency on Aftermarket.

## Purpose

Unscene is a native OBS Studio dock that brings the entire scene collection into one expandable, theme-aware workspace. It organizes and edits sources without constantly switching scenes. No Aftermarket dependency.

## Stable invariants

- OBS owns the scene collection. Unscene does not maintain a second scene model on disk; editing actions mutate OBS state directly and can affect the live output.
- Expanding a scene or selecting a source in the dock does not activate a scene. Explicit scene activation selects the program scene in normal mode or the preview scene in Studio Mode.
- Automatic snapshots are disabled by default. Opt-in idle capture is gated while OBS reports streaming, recording, replay buffer, or virtual camera active.
- Sources are identified by UUID; item-targeted edits also identify the owning scene or group and the scene item ID. Duplicate paste and portable source import intentionally create independent OBS sources.

## Architecture and ownership boundaries

- **C++/CMake native plugin** with a verified local OBS 32.2.0 Linux x86_64 baseline; expanded CI targets OBS 32.2.0/33.0.3 on Linux, Windows and macOS (qualification pending). Qt 6 widgets. libobs + libobs-frontend-api.
- **Plugin**: `unscene.so`. CMake target: `unscene`.
- **Core modules**: scene hierarchy, scene/source selection, thumbnail cache, navigation/search, appearance/theme, source management (add/remove/copy/rename/reorder), drag scroll, motion system, inspector (transform/filters/usage), portable transfers (filter stack / single-source JSON), native OBS context menus.
- **CI**: The [pinned shared workflow](../.github/workflows/ci.yml) uses the [repository's CI driver](../.github/scripts/ci-verify.sh) for the full eight-job matrix. The caller owns native SDK preparation and exact package layouts; the Linux OBS32 job emits the source archive. The workflow sets the test floor. See docs/ci.md for qualification.
- **Testing**: [CMake](../CMakeLists.txt) registers the CTest suite. Use isolated OBS profiles for runtime verification.

## Known-good workflows

| Area | Authority |
|---|---|
| Build, test, install | [README](../README.md#build-and-install) and [CMake](../CMakeLists.txt) |
| Project identity, architecture, invariants | `.meristem/PROJECT.md` (this file) |
| Dated work and verification history | [`ACTIVE_WORK.md`](../ACTIVE_WORK.md); use Git for current branch and working-tree state |
| Agent rules | `AGENTS.md` (Meristem block) |
| Project decisions | `.meristem/DECISIONS.md` |
| Reusable lessons | `.meristem/LEARNINGS.md` |
| Known failures | `.meristem/ERRORS.md` |
| Requested capabilities | `.meristem/FEATURE_REQUESTS.md` |

## Verification expectations

- **C++**: Follow the README's configure, build, and CTest commands; report the result for the current tree.
- **Runtime**: Use a disposable OBS 32.2.0 profile to check Docks → Unscene and exercise affected behavior. Check collection edits in OBS collection data and dock preferences in Unscene's module configuration.
- **Install**: Use a temporary install prefix for verification. Preview packaging and publication are separate.
- **Snapshot policy**: Verify the default and output gates against [Graphics isolation](../docs/usage.md#graphics-isolation).

## Durable structural uncertainty

- Live OBS runtime testing is done in disposable profiles; stream performance benchmark and multi-thread graphics contention not independently measured.
- Native OBS Undo covers a growing subset of operations (filter add/remove/toggle/reorder/rename, item reorder, visibility, lock, scale, blending, transform reset); direct scene/item add/removal still lacks native undo.
- Native OBS context menus delegate to OBS's Qt slots when safe; fallback to Unscene's own menus for inactive/unresolvable targets; exact native menu mapping is runtime-discovered.
- Portable JSON supports individual sources, single filters, and filter stacks; it is separate from the collection-scoped reference/duplicate clipboard. See the [usage guide](../docs/usage.md) for transfer limits.
- Theme-based tonal surfaces use local OKLab roles and preserve native selection colors. Historical captures cover light and dark OBS themes; they do not establish that switching themes live was exercised.
- Qt widget animation runs on GUI thread (asynchronous); frame-time impact during streaming not independently benchmarked.
- ReorderMotion caps at ≤96 visible rows and ≤32 moving widgets; large-batch restructures settle directly.
