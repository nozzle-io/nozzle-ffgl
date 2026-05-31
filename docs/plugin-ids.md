# FFGL plugin IDs

FFGL unique IDs are not display names. Resolume's FFGL SDK examples document a maximum unique ID length of four characters, and `deps/ffgl/build/PluginIds.txt` uses short IDs such as `RS01`, `RE01`, and `RM01`.

Allocated IDs for nozzle-ffgl:

| Display name | FFGL type | Unique ID | Bundle identifier |
|---|---|---:|---|
| NozzleReceive | Source | `NZR1` | `org.nozzle-io.ffgl.receive` |
| NozzleSend | Effect | `NZS1` | `org.nozzle-io.ffgl.send` |

The IDs intentionally do not reuse FFGL SDK example IDs.
