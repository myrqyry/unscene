#!/usr/bin/env bash
# Shared OBS CI caller; only the qualified Linux target is enabled.
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
[[ "${TARGET:?}" == linux-x86_64 && "${OBS_VERSION:?}" == 32.2.0 && "${MODULE:?}" == unscene && "${PUBLIC_NAME:?}" == unscene ]]
[[ "${ARCHIVE:?}" == unscene-linux-x86_64-obs32.2.0-test.tar.gz ]]
[[ "${MINIMUM_TESTS:?}" =~ ^[1-9][0-9]*$ ]]
work="${RUNNER_TEMP:?}/plugin-ci"
srcwork="$RUNNER_TEMP/plugin-ci-source"
mkdir -p "$work" "$srcwork"
sdk="${UNSCENE_OBS_SDK:-$RUNNER_TEMP/obs-sdk/sdk}"
if [[ -z "${UNSCENE_OBS_SDK:-}" ]]; then
 bash "$root/.github/scripts/prepare-obs32-sdk.sh" "$RUNNER_TEMP/obs-sdk"
 test "$(cat "$sdk/OBS_SDK_VERSION")" = "$OBS_VERSION"
fi
test -s "$sdk/lib/cmake/libobs/libobsConfig.cmake"
test -s "$sdk/lib/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake"
cmake -S "$root" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
 -Dlibobs_DIR="$sdk/lib/cmake/libobs" -Dobs-frontend-api_DIR="$sdk/lib/cmake/obs-frontend-api"
cmake --build "$work/build" --parallel 2
ctest --test-dir "$work/build" --show-only=json-v1 > "$work/tests.json"
python3 - "$work/tests.json" "$MINIMUM_TESTS" <<'PY'
import json,sys
count=len(json.load(open(sys.argv[1]))["tests"])
assert count>=int(sys.argv[2]),count
print(f"Registered {count} tests")
PY
ctest --test-dir "$work/build" --output-on-failure
stage="$work/stage"
test ! -e "$stage"
cmake --install "$work/build" --prefix "$stage"
test -s "$stage/bin/64bit/unscene.so"
cmp "$root/LICENSE" "$stage/LICENSE"
test "$(find "$stage" -type f | wc -l)" -eq 2
test -z "$(find "$stage" -type l -print -quit)"
LD_LIBRARY_PATH="$sdk/lib:${LD_LIBRARY_PATH:-}" ldd "$stage/bin/64bit/unscene.so" > "$work/ldd.txt"
! grep -q 'not found' "$work/ldd.txt"
nm -D --defined-only "$stage/bin/64bit/unscene.so" > "$work/symbols.txt"
for symbol in obs_module_load obs_module_unload obs_module_ver; do
 grep -Eq " [TW] $symbol$" "$work/symbols.txt"
done
(cd "$stage"; sha256sum LICENSE bin/64bit/unscene.so > "$srcwork/PAYLOAD-SHA256SUMS"; tar -czf "$work/$ARCHIVE" LICENSE bin/64bit/unscene.so)
(cd "$work"; sha256sum "$ARCHIVE" > SHA256SUMS; sha256sum -c SHA256SUMS; tar -tzf "$ARCHIVE")
# Source corresponds to the checked-out commit, never untracked local files.
git -C "$root" diff --quiet HEAD -- .
git -C "$root" archive --format=tar --prefix=unscene/ HEAD | gzip -n > "$srcwork/unscene-source.tar.gz"
(cd "$srcwork"; sha256sum unscene-source.tar.gz > SHA256SUMS; sha256sum -c SHA256SUMS)
