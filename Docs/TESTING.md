# Vulkan unit tests

Place backend unit tests beside the implementation, for example
`Utils/DescriptorSetLayoutBuilder.hpp`, `DescriptorSetLayoutBuilder.cpp`, and
`DescriptorSetLayoutBuilder_test.cpp`. Add each test source explicitly to the
`RHI_Vulkan_Tests` source list in this directory's `CMakeLists.txt`.

`RHI_BUILD_TESTS=ON` enables the Catch2 executable and its CTest registration.
With tests disabled, backend sources compile directly into RHI. With tests enabled,
the private RHI_Vulkan object target shares compiled code with the test executable
and the library, so private backend
symbols do not need to be exported from the library. Test sources and Catch2
are excluded from the production library.

Build and run from an initialized compiler environment:

```sh
cmake --build out/build/windows-msvc-debug --target RHI RHI_Tests RHI_Vulkan_Tests --parallel
ctest --test-dir out/build/windows-msvc-debug --output-on-failure --no-tests=error
```

The initial builder tests run without creating a Vulkan instance or device.
They cover declaration ordering, array binding progression, and reset/reuse.
Device creation and GPU execution need separate integration tests or examples.
