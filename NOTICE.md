# Licensing and attribution

Copyright © 2026 myrqyry.

Unscene’s original software in this revision is distributed under the GNU
General Public License, version 3 or (at your option) any later version
(GPL-3.0-or-later). See [LICENSE](LICENSE). Previously granted licenses
for earlier revisions are unaffected. Third-party materials retain their
original terms. The corresponding source is available at
https://github.com/myrqyry/unscene.

Unscene is an independent native plugin for [OBS Studio](https://obsproject.com/),
using libobs, the OBS frontend API and Qt 6. It is not an official OBS Project
product and does not require Aftermarket.

The inspector was inspired by [Source Inspector Dock](https://github.com/prgmitchell/source-inspector-dock).
Portable single-source and filter JSON follows the native OBS serialization
format used by [Source Copy](https://github.com/exeldro/obs-source-copy).
These implementations were developed independently, not bundled from those plugins.
The scene palettes adapt [Material dynamic color principles](https://github.com/material-foundation/material-color-utilities)
using a local OKLab implementation rather than Google's HCT implementation.

The SDK provisioning script is reused from the author's obs-ui-augment
repository; the CI workflow calls the shared obs-plugin-ci repository.

Developed in collaboration with a trusted AI colleague. This voluntary
acknowledgment describes the development process, not a warranty of code quality.
