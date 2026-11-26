---
name: unscene-verify
description: Use when building Unscene, diagnosing a module load failure, or verifying a preview artifact on the supported OBS target.
---

# Verify Unscene

Run from the Unscene repository root.

1. Check the target in `.github/workflows/ci.yml` and `.github/scripts/ci-verify.sh`. Use matching `libobs` and `obs-frontend-api` SDK packages; follow `README.md` → Build and install for the CMake flags. A local build against a newer SDK is not evidence that the module loads on the supported target.
2. Run the README configure, build, and CTest steps. Stage with `cmake --install build --prefix <temporary-directory>` and check that the prefix contains `bin/64bit/unscene.so` and `LICENSE`.
3. For a load or UI claim, use a fresh, isolated OBS process with a disposable profile and the staged module. Check the OBS log for the module path, version or symbol errors, and successful load before debugging dock behavior. Open **Docks → Unscene** and exercise the affected behavior. When verifying a packaged artifact, compare the staged module to the extracted artifact.
4. Report the configure/build/test, staged install, and OBS runtime results separately. Use `docs/usage.md` for the expected dock behavior.
