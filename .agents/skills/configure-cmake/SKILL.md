---
name: configure-cmake
description: Configure RHI with CMake presets and Conan 2, select project options, and diagnose configuration failures. Use for configuration rather than building or running tests.
---

# Configure RHI

Run commands from the repository root. Read `CMakePresets.json`, the root and relevant subdirectory `CMakeLists.txt` files, and `conanfile.txt`; they are the source of truth if this skill differs from the repository.

## Select a preset

Preserve the requested compiler, build type, and options. If unspecified, reuse a compatible existing configuration or choose a host-compatible Debug preset and explain the choice. Do not silently substitute a missing compiler.

| Host | Compiler | Debug preset | Release preset |
| --- | --- | --- | --- |
| Windows | MSVC (`cl`) | `windows-msvc-debug` | `windows-msvc-release` |
| Windows | Clang (`clang-cl`) | `windows-clang-debug` | `windows-clang-release` |
| Linux / Darwin | GCC (`gcc`, `g++`) | `unixlike-gcc-debug` | `unixlike-gcc-release` |
| Linux / Darwin | Clang (`clang`, `clang++`) | `unixlike-clang-debug` | `unixlike-clang-release` |

All presets use Ninja. Debug presets select `Debug`; release presets select `RelWithDebInfo`. Hidden `conf-*` presets are inheritance helpers. Run `cmake --list-presets` to see available presets.

Darwin presets do not establish working macOS support: `Source/ShaderCompile.cmake` checks `UNIX` before `APPLE`, selecting the Linux shader-tool archive on Darwin. Report this limitation when relevant; do not expand configuration into an unsolicited build-system fix.

## Prerequisites

Check availability and versions: CMake 3.25 or newer, Conan 2 (the provider requires at least 2.0.5), Ninja, and a C++20 compiler.

Windows presets require an initialized x64 Visual Studio developer environment, including SDK and linker paths. Architecture and toolset use the `external` strategy, so CMake does not initialize it. Use an existing Developer PowerShell / command prompt or import the installed developer environment into the configuring process. `clang-cl` also needs the MSVC toolchain environment.

Configuration may download dependencies, build missing Conan packages, and fetch shader tools. Follow execution permissions for network access and writes outside the workspace. Graphics examples need a Vulkan driver and window system at runtime. Their shader build commands require `glslc` on PATH; the fetched `glslcc` archive does not replace it.

## Configure and choose options

For Windows MSVC Debug:

```sh
cmake --preset windows-msvc-debug
```

For Linux GCC Debug, run from the repository root:

```sh
cmake --preset unixlike-gcc-debug
```

For Linux Clang Debug:

```sh
cmake --preset unixlike-clang-debug
```

For Linux builds with optimizations and debug information, choose either `unixlike-gcc-release` or `unixlike-clang-release`. For example:

```sh
cmake --preset unixlike-gcc-release -DRHI_BUILD_EXAMPLES=OFF -DRHI_BUILD_TESTS=ON
```

On Linux, ensure `gcc` and `g++`, or `clang` and `clang++`, are available on PATH together with CMake, Ninja, and Conan. No Visual Studio developer environment is required. Dependencies built from source may also require distribution development packages; diagnose the actual Conan failure before installing packages.

Preset directories are `out/build/<preset-name>` for binaries and `out/install/<preset-name>` for installation. Use command-line cache overrides for one-off choices rather than editing tracked presets:

```sh
cmake --preset windows-msvc-debug -DRHI_BUILD_EXAMPLES=OFF -DRHI_BUILD_TESTS=ON
```

| Option | Default | Effect |
| --- | --- | --- |
| `RHI_BUILD_EXAMPLES` | `ON` | Adds examples; disable when unnecessary. |
| `RHI_BUILD_TESTS` | `ON` | Enables testing and adds `RHI_Tests`. |
| `RHI_BUILD_SHARED` | `ON` | Explicitly creates a shared library. With `OFF`, the untyped `add_library` follows `BUILD_SHARED_LIBS`; also set `-DBUILD_SHARED_LIBS=OFF` when static linkage must be guaranteed. |
| `RHI_VULKAN_BACKEND` | `ON` | Adds Vulkan. Keep enabled for normal usable configurations; disabling it does not select another backend. |
| `RHI_USE_LOG_OUTPUT` | `ON` | Enables the RHI debug-log compile definition. |

Cache overrides persist. Inspect `CMakeCache.txt` when reusing a directory and explicitly reset earlier choices when needed. Keep different compilers and build types in separate directories. For custom variants, override `-B` and, when needed, `-DCMAKE_INSTALL_PREFIX`; report the actual paths.

## Preserve Conan integration

Presets load `Source/conan.cmake` through `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`. The provider installs dependencies on the first `find_package()` and uses `CMakeDeps` output. Preserve this integration instead of adding a manual Conan install or competing toolchain file for ordinary preset configuration.

Default host profiles are `default;auto-cmake`; the build profile is `default`. The provider detects a default profile when needed; `auto-cmake` derives host settings from CMake. Override `CONAN_HOST_PROFILE` or `CONAN_BUILD_PROFILE` only when the requested toolchain requires it. Quote semicolon-separated values in PowerShell.

Presets pass `--build=missing` and suppress Conan-generated user presets through `CONAN_INSTALL_ARGS`. Preserve these arguments unless different behavior is requested. Disabling tests or examples does not remove their unconditional Conan requirements. The root CMake file also includes the shader-tool fetch unconditionally.

## Verify and troubleshoot

Success requires a zero exit code and completed generation in the intended directory. Confirm compiler, build type, options, and install prefix from output and cache. Report the preset, overrides, directory, and unresolved prerequisites. Configuration alone does not verify compilation, tests, or rendering; use the adjacent `build-project` skill when those actions are requested.

Distinguish compiler-environment issues, missing Ninja/Conan, dependency resolution, network access, shader-tool fetching, and incompatible caches before retrying. Retry sandbox-related network failures through required escalation. Report persistent external failures rather than repeatedly downloading or changing package versions.

Prefer a separate binary directory when changing toolchains. Do not delete build trees to hide failures; refresh caches only when justified and authorized. Keep generated files out of source changes. Configuration alone does not authorize editing public headers, presets, options, or dependency requirements.
