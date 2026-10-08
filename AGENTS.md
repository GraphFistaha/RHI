# Project instructions

## Purpose and scope

RHI is a Render Hardware Interface written in C++20, built with CMake and Conan 2. Vulkan is the currently implemented backend. The public interface must remain suitable for future Metal and DirectX 12 backends.

Keep changes focused on the requested task. Follow the surrounding code and preserve existing public behavior unless the task requires an API change. Do not implement speculative backends or perform unrelated architectural rewrites.

## Repository layout

- `Source/Public/`: public RHI headers, interfaces, descriptors, and image types. `RHI.hpp` is the main entry point for consumers.
- `Source/Private/`: shared implementation details and utilities. Keep these independent of any particular graphics backend.
- `Source/Vulkan/`: Vulkan implementation, including memory, command submission, synchronization, pipelines, render passes, and transfers.
- `Source/Vulkan/ThirdParty/`: vendored dependencies. Avoid editing them unless the task explicitly concerns them.
- `Source/Tests/`: Catch2 unit tests, built as `RHI_Tests` and registered with CTest.
- `Examples/`: rendering examples and common window/test helpers. Check `Examples/CMakeLists.txt` to see which examples are enabled; most are currently commented out.
- `conanfile.txt`: dependency requirements and Conan generators.
- `CMakePresets.json`: configure and test presets for MSVC, Clang, and GCC.

## Backend boundaries

- Express public behavior using RHI types and concepts. Keep Vulkan headers, handles, constants, and implementation classes inside the Vulkan backend.
- Preserve the existing native handle interoperability points, but do not make ordinary public operations depend on native API objects.
- Translate public descriptors and enums into native API values within the backend. Do not assume that RHI enum values equal Vulkan constants.
- Define resource usage, synchronization, and lifetime contracts in terms that another backend can implement. Avoid exposing Vulkan layouts, queue families, descriptor mechanics, or render pass requirements as mandatory public concepts.
- When a feature depends on hardware or backend capabilities, make the requirement and unsupported behavior explicit. Do not silently promise support across all future backends.
- Future backends should have their own implementation directories and build options. Add their dependencies only when implementing them, scoped to the appropriate backend and platform.

## C++ conventions

- Use C++20 and the standard library where appropriate; do not require a newer language standard.
- Follow local formatting and naming. Existing code commonly uses `.hpp`/`.cpp`, `#pragma once`, braces on separate lines, the `RHI` namespace, and `RHI::vulkan` for backend classes.
- Use the repository's `.clang-format` and `.clang-tidy` configuration when formatting or running static analysis. Avoid reformatting unrelated code.
- Keep headers self-contained and minimize implementation details in public headers.
- Use RAII for native resources and mappings. Make ownership and non-owning references clear; follow the existing ownership helpers and pointer conventions.
- Prefer unique ownership when sharing is unnecessary. Preserve shared ownership where existing asynchronous operations or public contracts require it.
- Resource-owning types must not accidentally copy native handles. Implement move operations and destruction consistently with existing types.
- Use `override` for overrides and `noexcept` only where the operation really cannot throw. Avoid unchecked casts and unexplained magic constants.
- Document public contracts, including sizes, offsets, units, ownership, lifetime, thread safety, and asynchronous completion where relevant.
- Preserve public export annotations and shared/static library support when changing declarations.

## GPU correctness

- Keep resources alive until submitted GPU work that references them has completed. Follow the existing awaitable and garbage collection mechanisms.
- Check execution ordering, memory visibility, image layouts, and queue ownership when changing command recording or submission.
- Treat host synchronization separately from GPU synchronization. Do not assume command recording, descriptor updates, or resource access is thread-safe without an explicit contract.
- Validate transfer ranges, image subresources, formats, alignment, and host memory flush/invalidate requirements as applicable.
- Avoid adding device-wide idle waits to routine rendering paths. Use the narrowest synchronization that satisfies the operation's contract.
- Preserve Vulkan validation layer support and investigate new validation messages caused by a change.

## Build and dependencies

Use CMake 3.25 or newer, Conan 2, Ninja, and a compiler supporting C++20. Windows presets require the appropriate compiler environment to be initialized; MSVC presets use `cl`, and Windows Clang presets use `clang-cl`.

The presets load `Source/conan.cmake` through `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`. Configuration can install dependencies and build missing Conan packages. Use this existing integration instead of adding a separate dependency manager or a competing Conan workflow.

For a Windows MSVC Debug build, run from the repository root:

```sh
cmake --preset windows-msvc-debug
cmake --build out/build/windows-msvc-debug --parallel
ctest --preset test-windows-msvc-debug
```

Use the corresponding configure preset, build directory, and `test-<configure-preset>` on other toolchains. The repository currently has no build presets; build with the binary directory rather than `cmake --build --preset`.

Existing options are `RHI_VULKAN_BACKEND`, `RHI_BUILD_SHARED`, `RHI_BUILD_TESTS`, `RHI_BUILD_EXAMPLES`, and `RHI_USE_LOG_OUTPUT`. Vulkan is currently the only implemented backend, so leave it enabled for normal builds. Disable examples when a library-only build is appropriate:

```sh
cmake --preset windows-msvc-debug -DRHI_BUILD_EXAMPLES=OFF
```

- Register new source files in the relevant `target_sources` list.
- Prefer target-scoped CMake properties, include directories, compile definitions, and link dependencies.
- Manage package requirements in `conanfile.txt`; avoid hardcoded local SDK or package paths.
- Keep generated binaries, Conan output, shader output, and build caches out of source changes.

## Validation and completion

- For code changes, build the affected targets and run relevant CTest tests. Add focused tests for new behavior or regressions when practical.
- Unit tests alone do not verify rendering or GPU synchronization. For graphics changes, run an appropriate enabled example with validation layers when the environment supports it.
- Examples may require a window system, a working Vulkan driver, and shader compilation tools. Report missing prerequisites or checks that could not be run.
- For public API changes, update affected examples and documentation and check consumer compilation where practical.
- Documentation-only changes do not require a full GPU build.
- Before finishing, review the diff for unrelated changes. State what changed, what was verified, and any remaining limitations. Do not claim a build or runtime check passed unless it was actually run.
