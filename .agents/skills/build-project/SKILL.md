---
name: build-project
description: Build RHI or selected targets from a CMake configuration, run relevant CTest tests, and diagnose build failures. Use configure-cmake when configuration needs to be created or changed.
---

# Build RHI

Run commands from the repository root. Read `CMakePresets.json` and relevant `CMakeLists.txt` files to confirm directories, enabled targets, and test registration. Preserve the requested compiler, configuration, options, and target scope.

## Reuse or configure a build tree

Reuse a compatible generated build tree. Inspect `CMakeCache.txt` for the source directory, compiler, build type, and `RHI_BUILD_TESTS` / `RHI_BUILD_EXAMPLES` options. Do not assume directory names prove compatibility.

When configuration is missing, incompatible, or needs different options, read and follow [configure-cmake](../configure-cmake/SKILL.md). It owns preset selection, compiler environment setup, cache overrides, and Conan integration. A build request includes necessary configuration; configuration-only requests belong to that skill.

Windows builds need the appropriate x64 Visual Studio developer environment, including SDK and linker paths, for both `cl` and `clang-cl`. Keep that environment available in the building process, even when reusing a configured directory.

## Build requested targets

The repository currently has no build presets. Use the actual binary directory with `cmake --build`; standard preset directories are `out/build/<configure-preset>`. Ninja presets select the build type at configuration time, so use the matching tree rather than changing it with `--config`.

For all enabled targets in an existing Windows MSVC Debug tree:

```sh
cmake --build out/build/windows-msvc-debug --parallel
```

For only the library and unit tests:

```sh
cmake --build out/build/windows-msvc-debug --target RHI RHI_Tests --parallel
```

`RHI_Tests` requires `RHI_BUILD_TESTS=ON`. Build an example by its enabled target name, such as `HelloTriangle`, when graphics validation is needed. Check `Examples/CMakeLists.txt`: directories on disk may be commented out and unavailable as targets. Use `cmake --build <binary-dir> --target help` when target availability is unclear. Preserve custom binary directories.

Shader build commands require `glslc` on PATH; the fetched `glslcc` archive does not satisfy them. Shader output goes under `CMAKE_INSTALL_PREFIX`, so example builds may write there without an install command. Check the configured prefix and follow execution permissions for writes outside the workspace. Do not install during an ordinary library build unless requested or needed for runtime validation.

## Validate results

For code changes, build affected targets and run relevant tests. For an ordinary build request, run available unit tests unless the user requests compilation only. Build `RHI_Tests` before CTest when tests are enabled.

For a standard preset directory:

```sh
ctest --preset test-windows-msvc-debug
```

Use `test-<configure-preset>` for the corresponding toolchain. For a custom binary directory, use the actual path:

```sh
ctest --test-dir out/build/custom-debug --output-on-failure --no-tests=error
```

Use CTest's `-R` filter when only selected registered tests are relevant. Tests disabled or no tests discovered means validation was skipped, not passed; report it explicitly.

For graphics changes, run an appropriate enabled example with Vulkan validation layers when the driver and window system are available. Inspect resource paths and runtime library needs before launching: shaders are emitted into the install prefix, while the executable initially lives in the build tree. Use a suitable working directory and, if needed, install built artifacts with `cmake --install <binary-dir>` after checking the destination. Report rendering and validation results separately from compilation and unit tests; report missing runtime prerequisites when execution is unavailable.

## Diagnose and report

Use the first actionable compiler, linker, shader, or test failure to guide the next step. Add `--verbose` to the build command when the failing invocation is needed. If automatic CMake regeneration fails, return to `configure-cmake`; preserve the existing Conan provider. Retry sandbox-related failures through required escalation.

Keep fixes within the authorized task. A build request alone does not authorize changing public APIs, dependencies, or project options to conceal failures. Do not delete build trees or perform clean rebuilds without evidence that stale outputs caused the issue.

Report the configuration and actual binary directory, targets built, test results, graphics checks performed or skipped, and unresolved failures. Keep generated binaries, dependency output, shaders, and caches out of source changes.
