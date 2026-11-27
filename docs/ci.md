# Native CI and packaging

The shared immutable workflow pin is unchanged (afeed7458c9fa5da341cc464042fa0f878ecf33c).
Unscene now selects its full matrix and retains the caller verifier, rather than
using the still-experimental Rust replacement. No release job.

| Target | OBS 32.2.0 | OBS 33.0.3 |
| --- | --- | --- |
| Linux x86_64 | Ubuntu 24.04; original flat legacy archive | Ubuntu 26.04; module at unscene/ root |
| Windows x64 | VS 2026; unscene/bin/64bit/unscene.dll | VS 2026; unscene/unscene.dll |
| macOS ARM64 | Native .plugin bundle | Native .plugin bundle |
| macOS x86_64 | Native .plugin bundle | Native .plugin bundle |

The Linux legacy SDK recipe is retained. OBS33 source pins and the native
Windows/macOS SDK bootstrap adapt the already hosted-tested Streaming Media
driver at 875f6b1 (same project owner), without its websocket/runtime
dependencies. Pinned source bytes and upstream dependency manifests are
verified; exact source patches fail if the expected source differs. The
Windows Qt SDK provides minimal rather than offscreen. Tests use isolated
Xvfb for Linux libobs startup. All 20 current tests are required; JUnit rejects
missing, failed, disabled or skipped cases. Source package and its binary
payload manifest are produced only by the Linux OBS32 job.

The original Linux32 archive contains LICENSE and bin/64bit/unscene.so.
New Windows/Linux33 archives use an enclosing unscene folder. macOS bundles
contain Info.plist, executable and LICENSE, with an ad-hoc signature.
No bundled OBS or Qt binaries, PDBs, runtime downloads or automatic releases.
Check the archive SHA256SUMS before extracting. The archive layout is not
a claim that installation/operation has been verified on that platform.

## Evidence

Hosted run 45905573913 built OBS32 and Unscene, ran 17/18 tests, then failed
filter-restore-test because obs_startup could not open an X display.
Reproduced locally with DISPLAY and WAYLAND_DISPLAY unset; fixed by wrapping
that exact test in xvfb-run, without skipping or altering its assertions.
All 18 local Release CTests then passed with no desktop display available.
The new native adapters and OBS33 SDK require hosted qualification. No
Windows/macOS runtime claim, notarization or OBS33 GUI claim is made.

Hosted PR run [45906445233](https://github.com/myrqyry/unscene/actions/runs/45906445233)
passed both Linux legs and uploaded both binary archives and the source
package. All six Windows/macOS legs reached plugin configuration, then
failed because the pinned OBS Qt SDK lacks Qt6TestConfig.cmake. Removed
the unnecessary QtTest dependency, retaining all test assertions: the
row tests send real Qt mouse events and motion tests wait via a precise
timer and event loop. The local configure/build with
CMAKE_DISABLE_FIND_PACKAGE_Qt6Test=TRUE passed all 18 tests; the row and
motion tests also passed on Qt minimal. Native hosted requalification
of this change remains pending.

PR run [45907364027](https://github.com/myrqyry/unscene/actions/runs/45907364027)
passed Linux 32/33, Windows 32, and macOS ARM64 32/33, with five binary
artifacts plus source. Both Intel macOS builds compiled successfully but
portable-source-test segfaulted; Windows 33 timed out downloading the
pinned source. Fixed a dangling QJsonValueRef returned by the nested
transform lookup (compile-time ownership assertion fails before fix),
and expanded coordinate/malformed-input coverage. Linux ASan did not
reproduce the hosted crash, so causality awaits Intel confirmation.
Native downloads now use three bounded attempts for transient network
failures, temporary files and digest verification before publication;
checksum mismatches fail immediately. All 18 local CTests and four
native adapter tests pass. New hosted qualification required.


Hosted main run [45909441018](https://github.com/myrqyry/unscene/actions/runs/45909441018)
completed successfully after the cross-platform fixes. Subsequent license-update
run [45941592511](https://github.com/myrqyry/unscene/actions/runs/45941592511)
passed seven of eight build legs; Linux OBS32 failed at filter-restore-test
with graphics-context teardown warnings and a segmentation fault. This is not
a verified license-related failure. The current scene organization/shortcut
revision adds a nineteenth test and extends existing navigation, row and
item-menu tests; new hosted results are required.
