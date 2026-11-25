# Unscene

**Scenes and sources, unfolded.**

A standalone, native Qt dock for OBS that combines Scenes and Sources. Scene rows expand independently; the hover arrow activates a scene (selects preview in Studio Mode). Source rows expose visibility, lock and properties on hover, with a second row for filters, ordering and audio mute where applicable. Groups can expand. It uses the OBS theme's palette and standard icons.

Snapshots are cached by source UUID, captured only for visible rows whose source is already showing, with at most one attempt per 400 ms timer cycle. No forced source activation; unavailable snapshots retain icons. Refresh snapshots manually via the toolbar. Snapshots are currently session-only and do not automatically track changes to content.

## Custom appearance
Right-click any scene or source row, or use its Appearance action. Choose automatic artwork, one of 13 built-in line icons, or import a local image/icon file. Choose a row color independently; Clear restores the theme color and Reset restores both defaults. Confirm with OK.

Appearance follows the OBS source UUID, so a reused source has consistent artwork and color across scenes. Imported artwork is stored as a bounded PNG in Unscene's own configuration, not as an external file dependency. Custom artwork takes precedence over automatic snapshots. These settings do not change the stream or the native OBS source configuration.

![Custom scene icon and color, shared source artwork](docs/screenshots/custom-appearance.png)

## Build (Linux / Qt 6 / OBS 32.2 tested)
```sh
cmake -S . -B build -Dlibobs_DIR=/usr/local/lib/cmake/libobs -Dobs-frontend-api_DIR=/usr/local/lib/cmake/obs-frontend-api
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.config/obs-studio/plugins/unscene"
```
Restart OBS and enable the Unscene dock from Docks. Existing docks are unaffected.

First prototype screenshot (before the rename):

![Expanded source with cached thumbnails](docs/screenshots/expanded.png)

## Current limitations

First working slice, not a full native dock replacement. No drag/drop, add/remove, search, persistence, nested scene traversal, polished custom icons, automatic snapshot invalidation, or CI yet. Group traversal and Studio Mode activation are implemented but have not been runtime-validated. GPU readback is synchronous and not yet profiled. Inactive sources without cached snapshots remain placeholders.
