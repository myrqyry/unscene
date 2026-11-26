#!/usr/bin/env bash
# Shared OBS CI caller; native runners retain their own SDK adapters.
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
[[ "${TARGET:?}" =~ ^(linux-x86_64|windows-x64|macos-arm64|macos-x86_64)$ && "${OBS_VERSION:?}" =~ ^(32\.2\.0|33\.0\.3)$ && "${MODULE:?}" == unscene && "${PUBLIC_NAME:?}" == unscene ]]
if [[ "$TARGET" != linux-x86_64 ]]; then
 python "$root/.github/scripts/test-verify-native.py"
 exec python "$root/.github/scripts/verify-native.py" "$RUNNER_TEMP/plugin-ci" --obs-version "$OBS_VERSION" --minimum-tests "$MINIMUM_TESTS"
fi
[[ "${ARCHIVE:?}" == "unscene-linux-x86_64-obs$OBS_VERSION-test.tar.gz" ]]
[[ "${MINIMUM_TESTS:?}" =~ ^[1-9][0-9]*$ ]]
python3 "$root/.github/scripts/test-verify-native.py"
work="${RUNNER_TEMP:?}/plugin-ci"
srcwork="$RUNNER_TEMP/plugin-ci-source"
mkdir -p "$work" "$srcwork"
sdk="${UNSCENE_OBS_SDK:-$RUNNER_TEMP/obs-sdk/sdk}"
if [[ -z "${UNSCENE_OBS_SDK:-}" ]]; then
 bash "$root/.github/scripts/prepare-obs32-sdk.sh" "$RUNNER_TEMP/obs-sdk" "$OBS_VERSION"
 test "$(cat "$sdk/OBS_SDK_VERSION")" = "$OBS_VERSION"
fi
test -s "$sdk/lib/cmake/libobs/libobsConfig.cmake"
test -s "$sdk/lib/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake"
layout=legacy
if [[ "$OBS_VERSION" == 33.0.3 ]]; then layout=modern; fi
cmake -S "$root" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DUNSCENE_PACKAGE_LAYOUT="$layout" \
 -Dlibobs_DIR="$sdk/lib/cmake/libobs" -Dobs-frontend-api_DIR="$sdk/lib/cmake/obs-frontend-api"
cmake --build "$work/build" --parallel 2
ctest --test-dir "$work/build" --show-only=json-v1 > "$work/tests.json"
python3 - "$work/tests.json" "$MINIMUM_TESTS" <<'PY'
import json,sys
count=len(json.load(open(sys.argv[1]))["tests"])
assert count>=int(sys.argv[2]),count
print(f"Registered {count} tests")
PY
LD_LIBRARY_PATH="$sdk/lib:${LD_LIBRARY_PATH:-}" ctest --test-dir "$work/build" --output-on-failure --no-tests=error --output-junit "$work/ctest.xml"
python3 - "$work/ctest.xml" "$MINIMUM_TESTS" <<'XML'
import sys, xml.etree.ElementTree as ET
cases = ET.parse(sys.argv[1]).findall(".//testcase")
assert len(cases) >= int(sys.argv[2]) and not any(
    c.get("status") in ("notrun", "disabled") or any(c.find(x) is not None for x in ("skipped","failure","error")) for c in cases)
XML
stage="$work/stage"
test ! -e "$stage"
cmake --install "$work/build" --prefix "$stage"
module_rel=bin/64bit/unscene.so
package="$stage"
if [[ "$layout" == modern ]]; then
 package="$stage/unscene"
 module_rel=unscene.so
fi
# Stage modern packages under their OBS plugin-manager directory.
if [[ "$layout" == modern ]]; then
 mkdir "$package"
 mv "$stage/unscene.so" "$stage/LICENSE" "$package/"
fi
test -s "$package/$module_rel"
cmp "$root/LICENSE" "$package/LICENSE"
test "$(find "$stage" -type f | wc -l)" -eq 2
test -z "$(find "$stage" -type l -print -quit)"
LD_LIBRARY_PATH="$sdk/lib:${LD_LIBRARY_PATH:-}" ldd "$package/$module_rel" > "$work/ldd.txt"
! grep -q 'not found' "$work/ldd.txt"
nm -D --defined-only "$package/$module_rel" > "$work/symbols.txt"
for symbol in obs_module_load obs_module_unload obs_module_ver; do
 grep -Eq " [TW] $symbol$" "$work/symbols.txt"
done
if [[ "$layout" == legacy ]]; then
 (cd "$stage"; sha256sum LICENSE "$module_rel" > "$srcwork/PAYLOAD-SHA256SUMS"; tar -czf "$work/$ARCHIVE" LICENSE "$module_rel")
else
 (cd "$stage"; tar -czf "$work/$ARCHIVE" unscene/LICENSE unscene/unscene.so)
fi
(cd "$work"; sha256sum "$ARCHIVE" > SHA256SUMS; sha256sum -c SHA256SUMS; tar -tzf "$ARCHIVE")
# Only the legacy Linux host emits the shared source artifact.
if [[ "$OBS_VERSION" != 32.2.0 ]]; then exit 0; fi
# Source corresponds to the checked-out commit, never untracked local files.
git -C "$root" diff --quiet HEAD -- .
git -C "$root" archive --format=tar --prefix=unscene/ HEAD | gzip -n > "$srcwork/unscene-source.tar.gz"
(cd "$srcwork"; sha256sum unscene-source.tar.gz > SHA256SUMS; sha256sum -c SHA256SUMS)
