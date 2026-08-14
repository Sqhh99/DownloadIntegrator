# Repository Guidelines

Human-facing contribution rules live in [CONTRIBUTING.md](CONTRIBUTING.md). Keep this file limited to the project map and agent-facing conventions.

## Project Structure & Module Organization

FLiNG Downloader is a Windows-focused C++17 and Qt 6 application. Core business logic and the QML bridge live in `src/`; public headers are under `src/include/`. The interface is organized in `qml/`, with reusable controls in `qml/components/`, page views in `qml/pages/`, and theme definitions in `qml/themes/`. Runtime icons, translations, the SQLite translation database, and the ONNX model belong in `resources/`. Tests are split into `tests/unit/`, `tests/integration/`, and `tests/performance/`, with shared helpers in `tests/fixtures/`. Treat `third_party/` as vendored or generated dependency content and avoid editing it unless updating that dependency intentionally.

## Build, Test, and Development Commands

Development requires Visual Studio 2022, CMake, Ninja, Qt 6, and vcpkg. Set `VCPKG_ROOT` and, when Qt is not at the default path, `CMAKE_PREFIX_PATH`.

- `build.cmd` or `build.cmd release`: configure and build Release into `build/ninja-release/`.
- `build.cmd debug`: build Debug into `build/ninja-debug/`.
- `build.cmd run debug`: build and launch the Debug application.
- `build.cmd tests`: enable, build, and run all GoogleTest targets through CTest.
- `build.cmd benchmark --filter CoverExtractor/.*`: run matching Google Benchmark cases.
- `build.cmd i18n check`: verify generated Qt translation files are current.

## Coding Style & Naming Conventions

Match existing four-space indentation and C++17 idioms. Use `PascalCase` for classes and QML component filenames, `camelCase` for functions and local variables, and the `m_` prefix for private members. Keep headers in `src/include/` paired with implementations in `src/`. Follow the existing brace style, Qt signal/slot patterns, and `QStringLiteral` usage. No repository-wide formatter is configured, so keep formatting consistent with adjacent code and avoid unrelated cleanup.

## Testing Guidelines

Use GoogleTest fixtures and descriptive `TEST_F` names in `PascalCase`, for example `DownloadFileRenamesDetectedExecutableFormat`. Put isolated behavior in `tests/unit/` and network/database workflows in `tests/integration/`; use test hooks and fixtures instead of live services. There is no numeric coverage gate, but every bug fix or behavior change should include focused regression coverage. Run `build.cmd tests` before submitting.

## Commit & Pull Request Guidelines

Follow the established Conventional Commit style: `feat: add ...`, `fix(db): restore ...`, or `chore: ...`. Keep subjects concise and imperative. Pull requests should explain the user-visible change, identify affected modules, link relevant issues, and list verification performed. Include before/after screenshots for QML or visual changes and call out resource, database-schema, model, or translation updates explicitly.
