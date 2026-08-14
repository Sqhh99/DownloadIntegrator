# Third-party notices

FLiNG Downloader is licensed under the GNU Affero General Public License v3.0.
The binary releases also include or link the following third-party components.
This file is a pointer to those licenses, not a substitute for the upstream texts.

## Runtime (shipped next to the executable)

| Component | License | Source |
|---|---|---|
| Qt 6 (Core, Gui, Network, Qml, Quick, QuickControls2, …) | LGPL-3.0 | https://www.qt.io/licensing |
| ONNX Runtime 1.27.0 | MIT | https://github.com/microsoft/onnxruntime |
| Cover detection model (`models/game-cover-v2.onnx`) | project asset | shipped under this repository's AGPL-3.0 |

Qt is dynamically linked via `windeployqt`. The corresponding LGPL object is the Qt DLLs and QML modules in the release package.

## Vendored source

| Component | License | Location |
|---|---|---|
| YOLOs-CPP | MIT (upstream) | `third_party/YOLOs-CPP/` — see that directory's README |

Upstream: https://github.com/Geekgineer/YOLOs-CPP

## Built via vcpkg (see `vcpkg.json`)

| Port | Typical license |
|---|---|
| nlohmann-json | MIT |
| SQLiteCpp | MIT |
| sqlite3 | blessing / public domain |
| opencv4 (selected codecs) | Apache-2.0 |
| pugixml | MIT |
| libjpeg-turbo | BSD-like / IJG / zlib |
| libpng | libpng |
| libwebp | BSD-3-Clause |
| zlib | zlib |
| gtest | BSD-3-Clause (tests only) |
| benchmark | Apache-2.0 (benchmarks only) |

Exact texts travel with each vcpkg port under `share/*/copyright` after install.

## MSVC runtime

Windows packages may include Microsoft Visual C++ runtime DLLs (`vcruntime*.dll`, `msvcp*.dll`) redistributed under Microsoft's Visual C++ Redistributable terms.
