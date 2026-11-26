#!/usr/bin/env bash
# Reused from obs-ui-augment at 4530772; pinned OBS 32.2.0 SDK recipe.
set -euo pipefail
work="$(realpath -m "$1")"
version="${2:-32.2.0}"
case "$version" in
  32.2.0) sha=c333e4a7d5c4a94c7bb4833f046368e0ac5e981fb3964ca4981b23be1dfbda4a; extra=() ;;
  33.0.3) sha=b0eff217a972e1dd9951ce24d66595c723dde847461b16de07eb33d75ec34659; extra=(-DENABLE_SCRIPTING=OFF -DENABLE_BROWSER=OFF) ;;
  *) echo "Unsupported OBS SDK: $version" >&2; exit 1 ;;
esac
sdk="$work/sdk"
archive="$work/obs-studio-$version.tar.gz"
mkdir -p "$work"
curl -fsSL --retry 5 "https://github.com/obsproject/obs-studio/archive/refs/tags/$version.tar.gz" -o "$archive"
printf '%s  %s\n' "$sha" "$archive" | sha256sum -c -
tar -xzf "$archive" -C "$work"
cmake -S "$work/obs-studio-$version" -B "$work/obs-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$sdk" \
  -DOBS_CMAKE_VERSION:STRING=3.0.0 -DOBS_VERSION_OVERRIDE:STRING="$version" \
  -DENABLE_PLUGINS=OFF -DENABLE_FRONTEND=OFF "${extra[@]}"
cmake --build "$work/obs-build" --target obs-frontend-api libobs-opengl --parallel 4
cmake --install "$work/obs-build" --component Development --config Release --prefix "$sdk"
cmake --install "$work/obs-build" --component Runtime --config Release --prefix "$sdk"
test -s "$sdk/lib/cmake/libobs/libobsConfig.cmake"
test -s "$sdk/lib/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake"
printf '%s\n' "$version" > "$sdk/OBS_SDK_VERSION"
