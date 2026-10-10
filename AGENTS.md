# Project instructions

## General

RHI is a Render Hardware Interface written in C++20, built with CMake and Conan 2. Vulkan is the currently implemented backend. The public interface must remain suitable for future Metal and DirectX 12 backends.

Keep changes focused on the requested task. Follow the surrounding code and preserve existing public behavior unless the task requires an API change. Do not implement speculative backends or perform unrelated architectural rewrites.

## Repository layout

- `Source/Public/`: public RHI headers, interfaces, descriptors, and image types. `RHI.hpp` is the main entry point for consumers.
- `Source/Private/`: private crossplatform utilities and types, which makes internal code simplier and clean
- `Source/Vulkan/`: Vulkan implementation, including memory, command submission, synchronization, pipelines, render passes, and transfers.
- `Source/Vulkan/ThirdParty/`: vendored dependencies. Avoid editing them unless the task explicitly concerns them.
- `Source/Tests/`: Catch2 unit tests, built as `RHI_Tests` and registered with CTest.
- `Examples/`: rendering examples. Each example is a test to check API features.
- `conanfile.txt`: dependency requirements and Conan generators.
- `CMakePresets.json`: configure and test presets for MSVC, Clang, and GCC.
- All files should be in UTF-8-with-BOM encoding

Important: Do not change API in Source/Public. You can only edit comments. If you really need it - say it first and wait for approve.

## Backend boundaries

- Express public behavior using RHI types and concepts. Keep Vulkan headers, handles, constants, and implementation classes inside the Vulkan backend.
- Preserve the existing native handle interoperability points, but do not make ordinary public operations depend on native API objects.
- Translate public descriptors and enums into native API values within the backend. Do not assume that RHI enum values equal Vulkan constants.
- Define resource usage, synchronization, and lifetime contracts in terms that another backend can implement. Avoid exposing Vulkan layouts, queue families, descriptor mechanics, or render pass requirements as mandatory public concepts.
- When a feature depends on hardware or backend capabilities, make the requirement and unsupported behavior explicit. Do not silently promise support across all future backends.
- Future backends should have their own implementation directories and build options. Add their dependencies only when implementing them, scoped to the appropriate backend and platform.

## AI and tasks completion

- Each request should start from plan mode. The first step os each task is discussion about what should be done and why. 
- If task is big, try to break it on some smaller tasks. Think that one task - one commit, and one commit is a logically atomic action. In plan you can list that this task should be done with this commit (and put commit's message)
- After the plan is committed, you can implement it. Try to use several agents to make it. Each agent should take its role (architect, developer, reviewer)
- After the code is written, you should check it with independent agent - reviewer. There are some reviewer types: code-style reviewer, vulkan-code reviewer, performance-reviewer, C++-correctness reviewer. Check code for each of these side.

## Programming philosophy

- SOLID is important thing. First and second principles (Single-responsibility and OpenClose) are most important things.
- API must stay simple and understandable. Under API I mean any public interface (class-interface, library interface and so on).
- Code-clean and readablility is more important than performance. Less code is better.
- Comment shouldn't repeat code, but should explain what code does. Any function/class must have a comment to explain what it used for. What does it responsible for.
- No magic numbers. If you want to use constants - make a constant with comment why do you need it.
- No long functions. If function is long - break it. Remember about SOLID.
- const variable is better than mutable.
- Clean function is better than state-changer.
- Aggregation is better than inheritance.


## C++ conventions

- Use C++20 and the standard library where appropriate; do not require a newer language standard.
- headers in `.hpp` files. Each header must contain `#pragma once`.
- source files has `.cpp` extension. It's allowed to have several source files for one header file, in cases when it's logically approved.
- main namespace `RHI`. vulkan backend uses `RHI::vulkan`. If you declare function in source file - put it in anonimous namespace.
- Use the repository's `.clang-format` and `.clang-tidy` configuration when formatting or running static analysis. Avoid reformatting unrelated code.
- Use RAII for native resources and mappings. Make ownership and non-owning references clear; follow the existing ownership helpers and pointer conventions.
- Prefer unique ownership to sharing. Preserve shared ownership where existing asynchronous operations or public contracts require it.
- Minimize dynamic memory usage.
- Code must be exception safety. If function can't throw exception then mark it `noexcept`. If function can throw exception, try to use strong exception-safety (commit or rollback semantics).
- avoid inline functions
- avoid forward-declaration, but it's possible if it solves compilation and reduces compilation-time.
- avoid PIMPL and CPTR idioms. If some cases it's possible, but it should be justified
- write comments in doxygen-style. Use @brief style.
- all comments should be in english language

## GPU correctness

- Keep resources alive until submitted GPU work that references them has completed. Follow the existing awaitable and garbage collection mechanisms.
- Check execution ordering, memory visibility, image layouts, and queue ownership when changing command recording or submission.
- Treat host synchronization separately from GPU synchronization. Do not assume command recording, descriptor updates, or resource access is thread-safe without an explicit contract.
- Validate transfer ranges, image subresources, formats, alignment, and host memory flush/invalidate requirements as applicable.
- Avoid adding device-wide idle waits to routine rendering paths. Use the narrowest synchronization that satisfies the operation's contract.
- Preserve Vulkan validation layer support and investigate new validation messages caused by a change.

## Validation and completion

- For code changes, build the affected targets and run relevant CTest tests. Add focused tests for new behavior or regressions when practical.
- Unit tests alone do not verify rendering or GPU synchronization. For graphics changes, run an appropriate enabled example with validation layers when the environment supports it.
- Examples may require a window system, a working Vulkan driver, and shader compilation tools. Report missing prerequisites or checks that could not be run.
- Documentation-only changes do not require a full GPU build.
