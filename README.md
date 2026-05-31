# nozzle-ffgl

First-party FFGL plugins for publishing and receiving [nozzle](https://github.com/nozzle-io/nozzle) GPU texture streams in FFGL hosts such as Resolume.

## Plugins

| Plugin | FFGL type | Purpose | Default sender |
|---|---|---|---|
| `NozzleReceive` | Source | Receives a named nozzle source and renders it as an FFGL source. | `nozzle` |
| `NozzleSend` | Effect | Publishes the incoming FFGL texture as a nozzle source and renders the input through unchanged. | `nozzle_ffgl` |

FFGL unique IDs are documented in [`docs/plugin-ids.md`](docs/plugin-ids.md): `NZR1` and `NZS1`.

## Platform status

| Platform | Status | Output | Notes |
|---|---|---|---|
| macOS | CI build/package | Universal `.bundle` | Uses CGL/IOSurface path through nozzle OpenGL interop. |
| Windows | CI build/package | x64 `.dll` | Functional CPU-copy bridge around the current nozzle OpenGL/D3D11 path. Not zero-copy. |
| Linux | N/A | N/A | Out of scope for the Resolume-first FFGL pass. |

Do not read CI status as real host validation. CI proves build shape, package contents, binary architecture, and exported FFGL entry points. Resolume loading/rendering is manual smoke evidence and is tracked separately.

## Install

Download the `latest` release zip or a versioned release zip and copy both plugin files into Resolume's Extra Effects folder.

macOS:

```text
~/Documents/Resolume/Extra Effects/NozzleReceive.bundle
~/Documents/Resolume/Extra Effects/NozzleSend.bundle
```

Windows:

```text
%USERPROFILE%\Documents\Resolume\Extra Effects\NozzleReceive.dll
%USERPROFILE%\Documents\Resolume\Extra Effects\NozzleSend.dll
```

Restart the host after copying plugins.

## Parameters

### NozzleReceive

- `Sender`: nozzle sender name, default `nozzle`.
- `TimeoutMs`: frame acquisition timeout, clamped to 0..1000 ms.
- `Width` / `Height`: disconnected fallback size. The first-pass fallback is black output.

### NozzleSend

- `Sender`: nozzle sender name, default `nozzle_ffgl`.
- `AppName`: nozzle application name metadata, default `FFGL`.
- `Publish`: enables or disables publishing. Rendering pass-through remains active either way.

## Format and performance scope

First pass is deliberately narrow:

- Texture format claim: `rgba8_unorm` only.
- macOS: GPU-side transfer through CGL/IOSurface in nozzle.
- Windows: CPU-copy bridge via the current nozzle OpenGL/D3D11 implementation. This is functional but not a low-latency zero-copy path.
- 16F/32F and other host texture formats are not claimed.

## Orientation / smoke testing

FFGL v2.2 supports top-left texture orientation negotiation. The plugins opt into that SDK path and pass the negotiated origin into nozzle OpenGL interop.

Manual host smoke is still required before marking real support as tested:

1. Resolume loads `NozzleReceive` and `NozzleSend`.
2. `NozzleSend -> nozzle-viewer` shows a moving test pattern.
3. known nozzle sender -> `NozzleReceive` renders in Resolume.
4. no vertical flip, no R/B swap, alpha behavior documented.
5. non-square resolution works.
6. disconnect/reconnect does not leak or leave stale frames.

Until that evidence exists, status should remain CI-only.

## Build

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

macOS universal build:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config RelWithDebInfo
```

Windows x64 build:

```powershell
cmake -S . -B build -A x64
cmake --build build --config RelWithDebInfo
```

## Release artifacts

Latest release assets from `main`:

```text
nozzle-ffgl-latest-<short_sha>-macos.zip
nozzle-ffgl-latest-<short_sha>-windows-x64.zip
```

Versioned tag assets:

```text
nozzle-ffgl-<tag>-macos.zip
nozzle-ffgl-<tag>-windows-x64.zip
```

The release zips contain only the two plugin binaries plus `README.md`, `LICENSE`, and `THIRD-PARTY-NOTICES.md`.
