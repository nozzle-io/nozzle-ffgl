# Third-party notices

This repository builds against two submodules:

- `nozzle` — MIT License, nozzle contributors.
- `deps/ffgl` — Resolume FFGL SDK pinned to `v2.2` (`fda8d4a5904eaf09dd97ac0f95c246bd7f9f9a46`).

## FFGL SDK

The pinned `resolume/ffgl@v2.2` tree does not contain a root `LICENSE.md`, but the FFGL SDK headers include the FreeFrame / FFGL BSD-style license text and GitHub's license API for the current upstream default branch reports `BSD-3-Clause`.

The release zips ship only the built nozzle FFGL plugin binaries and project documentation. They do not ship the FFGL examples, Xcode/Visual Studio projects, `CustomThumbnail` assets, or other unused SDK sample/vendor files.

## glsdk / glload

The FFGL SDK uses `source/lib/glsdk_0_5_2/glload` on Windows. Its generated headers state:

> Copyright (C) 2011-2013 by Jason L. McKesson. This file is licensed by the MIT License.

The Windows binaries may include compiled glload code through `FFGLSDK.cpp`.
