# v0.9.2 local release-candidate checklist

Accepted preview features; release packages must pass compilation and asset checks.

- Added Cell Shaded and expanded grayscale Super Mario Land filters.
- Character Select presents its menu and character preview together on a floating theater panel.
- Added a Speedrunning menu with named splits, configurable HUD position/size/color, progressive split reveal, and portable setup/LiveSplit split-file imports.
- Added stationary Crossed Tree Billboards, enabled by default.
- Improved Big Hands surface climbing, independent hand attachments, and extended grabbing/punching.
- Big Hands lasts 30 seconds by default, with an optional 60-second Special Moves timer.
- Added alternate special power-up music, preserving Sonic Shoes and original cap themes.
- Expanded the in-game tutorial and player guide for current controls and features.
- Corrected the interpolated-camera lighting transform used during turns.

New since v0.9: physical jump/swim gestures, Propeller Mushroom, Power Star,
weighted spawn controls, cap cosmetics, held wall-kick input, and enemy-camera
tracking. Current controls are described in the in-game VR Tutorial and guides.

Release notes are maintained in release_notes.txt. PC and Quest packages must
contain the same shared changes and report v0.9.2. Verify package contents,
ROM-file exclusion and local package hashes. Keep saves, mods, palettes and settings
intact during updates. PC offline speech remains unverified under Wine/Proton.

Publication of v0.9.2 and its matching source was explicitly authorized after
the release review. README identifies the current release. Verify the public
asset hashes and latest-release tags after uploading both platform packages.
