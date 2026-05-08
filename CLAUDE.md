# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
# Desktop debug build (Qt 6.11, MinGW 64-bit)
cmake -B build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug -DCMAKE_BUILD_TYPE=Debug \
  -G "MinGW Makefiles" .
cmake --build build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug

# Android debug build
cmake -B build/Qt_6_11_0_for_Android_arm64_v8a-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=<path-to-android-toolchain> \
  -DANDROID_ABI=arm64-v8a .
cmake --build build/Qt_6_11_0_for_Android_arm64_v8a-Debug
```

Build artifacts live in `build/`. There are no tests.

## Architecture

**SuperLottoNumberGenerator** — a Qt 6 Widgets app for generating random lottery numbers, targeting both desktop and Android.

### Layers

```
main.cpp          →  QApplication + MainWindow (showMaximized)
LottoEngine       →  Pure number-generation logic (QObject, Q_INVOKABLE)
MainWindow        →  All UI construction, event handling, and persistence
```

### Core domain: [lottoengine.h](lottoengine.h)

`LottoResult` is a plain struct holding two sorted vectors: `front` (5 numbers from 1–35) and `back` (2 numbers from 1–12). It's registered as a Qt metatype via `Q_DECLARE_METATYPE`.

`LottoEngine` generates numbers using `std::shuffle` on `std::iota`-filled pools, driven by `QRandomGenerator::global()`. No `rand()` or manual seeding. Its methods are `Q_INVOKABLE` (hinting at potential future QML use, though the UI is currently Widgets-only).

### UI: [mainwindow.cpp](mainwindow.cpp)

The UI is built **entirely programmatically** in `MainWindow::init()`. The `.ui` file is a minimal skeleton (central widget, menu bar, status bar) — do not design UI in Qt Designer for this project. The layout:
- Vertical column layout with a time label at top, then 5 horizontal rows of number labels, then lock/generate buttons at the bottom.
- Front-area numbers (1–35) are red; back-area numbers (1–12) are blue.
- Screen width < 800px triggers mobile scaling (label size = 8% of screen width, min 24px).

### Persistence

Numbers are saved/restored via `QSettings` in INI format (file: `SuperLottoNumberGenerator.ini`). Save happens when the lock button is clicked. Restore is async (`QTimer::singleShot(0, ...)`) to avoid blocking the Android startup UI. The locked state stores: front/back numbers for all 5 groups + the generation timestamp.

### Android support

The `android/` directory contains the standard Qt Android deployment files: `AndroidManifest.xml`, `build.gradle`, Gradle wrapper, and launcher icons in `res/mipmap-*`. The CMake build uses `QT_ANDROID_PACKAGE_SOURCE_DIR` to point here.

## Code style

- C++17 (required in CMake)
- Qt 6 primary, Qt 5 fallback in CMake
- `CMAKE_AUTOMOC` / `AUTOUIC` / `AUTORCC` enabled — MOC runs automatically, no manual generated-source management
- Chinese comments throughout the source code
