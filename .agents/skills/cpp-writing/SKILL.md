---
name: cpp-writing
description: Write or refactor C++20 code in RHI using its naming, design, ownership, and documentation conventions. Use for C++ implementation work
---

# Write C++ in RHI

Read applicable AGENTS.md instructions and nearby declarations, implementations, and tests before editing. Follow the closest maintained code for domain vocabulary and extension points. Written project rules take precedence over older code: inline bodies, missing comments, unusual names, and raw resource management are not instructions to reproduce them. Keep edits focused on requested behavior.

Use C++20. Delegate indentation, braces, spacing, wrapping, and include ordering to the repository's `.clang-format`. Use `.clang-tidy` for configured naming checks. Save authored files as UTF-8 with BOM.

## Naming and source organization

- Use PascalCase for application types, aliases, and operations: `BufferGPU`, `InterfaceUPtr`, `DeclareDescriptorsArray`, `GetSynchronizer`. Use camelCase for locals and parameters: `allowHostAccess`, `shaderStagesMask`, `uniformBinding`.
- Prefix private and protected instance state with `m_`, as in `m_memBlock` and `m_bindings`. Prefer descriptive camelCase constants; follow local prefix conventions for existing static members rather than introducing another scheme. prefix `s_` for static mutable variables.
- Preserve RHI acronyms and existing enum spellings. For new names consult `.clang-tidy`, including scoped-enum rules; do not rename existing APIs to resolve style differences. Keep standard protocol names such as `begin`, `end`, `size`, and trait `type` when interoperability requires them.
- Use `.hpp` headers with `#pragma once` and `.cpp` implementations. Put ordinary non-template definitions in source files. Keep definitions in headers when templates or compile-time evaluation require visibility; avoid explicit `inline` without a concrete reason.
- Use `RHI` for library code, `RHI::utils` for crossplatform helpers, and `RHI::vulkan` and its existing subnamespaces for backend code. Put source-local helper functions in anonymous namespaces; keep externally required definitions and template specializations in their owning namespaces.
- Include declarations actually used rather than relying on transitive includes. Prefer complete includes; use forward declarations when they resolve dependency cycles or materially reduce compilation dependencies.

## If-statements and Loop-statements

- if body of `if` or `while/for` takes one-line - you don't need add `{}`.
- If body of `if` or `while/for` takes multiple lines 0 you should add `{}`
- If one of branches of `if-statement` takes multiline than both branches should have `{}`
- If you want to use `do while` loop, you should add `{}`

## Auto keyword
- Prefer to not use `auto` keyword, but it's allowed in some cases:
  - type is written in right side of expression. f.e. `auto t = static_cast<int>(5)`
  - it's assignment and type is templated and long-to-write
  - it's assignment from some function and return value is reference
  - it's lambda
  - it's tuple, pair, struct unpacking, f.e. `auto [inserted, it] = map.insert(...)`
  - it's range-based loop (but if type is quite simple, prefer explicit type)
- for basic types (`int`, `char`, `uint32_t`, `size_t`, `bool`, etc) do not use `auto`
- if you use `auto` for pointers - write `auto *`
- if you use `auto` for references - write `auto &&`

## Responsibilities and interfaces

Keep each class and function responsible for one coherent operation. Prefer short operations delegating to the component owning the behavior, as `BufferGPU` delegates mapping to `MemoryBlock` and asynchronous uploads to the transferer. Extract helpers around meaningful responsibilities rather than arbitrary line counts.

Prefer composition and existing extension points over new inheritance hierarchies, PIMPL, or CPTR indirection. Preserve inheritance needed for existing interfaces and ownership helpers. Builders separate descriptor accumulation from native creation through operations such as `DeclareDescriptor`, `Reset`, and `Make`; extend that division when editing builders.

Use `explicit` for constructors that should not allow implicit conversion and `override` for interface implementations. If function is `virtual` - keep `virtual` in their overrides. Prefer const locals, const queries, and computations returning values without changing unrelated state. Preserve reference qualifiers when borrowed subobject access must be restricted to a live lvalue owner.

Do not change declarations or behavior in `Source/Public` without explicit approval; comments may be edited. Keep Vulkan types and translation logic inside `Source/Vulkan`. Translate RHI enums and flags deliberately rather than assuming equal numeric values. Avoid editing `Source/Vulkan/ThirdParty` unless requested.

## Structures
- Use `struct` keyword when all it's members are public by default and can't have private methods, members
- Struct can't have methods, but can have operators like `operator bool`, `operator==`, etc
- Struct folows Zero rule, never implements its own constructors/destructors/copy operators/move operators, etc
- struct is always `final`

## Classes
- use `class` keyword for complex objects with internal statement
- members are listed in bottom of the class body
- members has prefix `m_`
- members are always `protected` or `private`
- Try to group class methods by it's interfaces or similar in meaning
```
  class Foo : public Interface1, public Interface2
  {
    public: // constructors and destructors
      Foo() = default;
      virtual ~Foo() override = default;

    public: // Interface1 implementation
      virtual void Method1() override;
      virtual void Method2() override;
    public: // Interface2 implementation
      virtual void Method3() override;
      virtual void Method4() override;
    public: // API for feature1
      void Feature1_Method1();
      void Feature1_Method2();
    public: // API for feature2
      void MyApi();
      void AnotherMethod();

    private:
      int m_member;
      std::vector<int> m_data;
  };
```
- Avoid `inline` methods, `static` methods, `friend` declaration. `inline` allowed for template-classses
- it's allowed to make a forward-declaration for some types used in class. If method uses type by ref/pointer - prefered forward-declaration. if class has some data in smart-pointer, it's allowed to used forwward-declaration
- prefer to make destructor `default`.

## Ownership, lifetime, and failure

- Prefer values and unique ownership. Preserve shared ownership where asynchronous contracts require it. Treat pointers and references as borrows unless the established contract says otherwise; document required lifetimes.
- `OwnedBy<T>` stores a non-owning owner pointer and does not extend its lifetime. Reuse existing owner access conventions such as `GetContext` when integrating with these classes.
- Use RAII for native resources and mappings. Preserve existing non-copyable resource conventions, including `RESTRICTED_COPY` where used. Moves must transfer responsibility for every owned resource and leave both objects valid for destruction. Inspect cleanup behavior rather than assuming a swap always suffices.
- Keep GPU resources alive until submitted work completes through existing awaitable and garbage collector mechanisms. C++ destruction does not establish GPU completion.
- Mark operations `noexcept` only when their implementations and callees cannot throw. Allocation, container growth, validation exceptions, and native creation wrappers may throw. Match declarations and definitions; keep cleanup non-throwing.
- Validate before changing state. Acquire resources into temporary owners and commit after required steps succeed; constructor failure must release earlier acquisitions. Preserve error contracts and identify the failed operation in diagnostics.
- Prefer existing containers and helpers over new allocation machinery. Name constants carrying domain meaning and explain their purpose. Check ranges and narrowing at native boundaries; casts do not validate values.

For command, transfer, or synchronization changes, check execution order, visibility, image layouts, queue ownership, and host synchronization separately. Validate relevant ranges, subresources, alignment, and flush/invalidate requirements. Use the narrowest required synchronization rather than routine device-wide waits.

## Comments and verification

Write English Doxygen comments with `@brief` for added classes and functions. For members use just `/// <comment>`. Argements described with `@param`. Explain responsibility and observable behavior, including non-obvious ownership, lifetime, failure, and concurrency requirements. Implementation comments explain constraints or choices rather than narrating assignments. Document templates at their declarations; avoid repeating descriptions at every definition.

Build affected targets and run relevant CTest tests for implementation changes, following [build-project](../build-project/SKILL.md). Follow neighboring Catch2 cases and test behavior, including rejection without consuming state where that is the contract. Add focused regression coverage when practical. Graphics changes also require an appropriate enabled example with Vulkan validation when supported; report unavailable checks. Documentation-only changes do not require a GPU build.

## Representative code

Paths are relative to the repository root. Read the examples relevant to the edit; they illustrate patterns rather than guaranteed correctness:

- `Source/Vulkan/Memory/BufferGPU.hpp` and `.cpp`: interface delegation, owner access, move operations, and deferred cleanup.
- `Source/Private/OwnedBy.hpp`: borrowed owner access and reference-qualified queries.
- `Source/Private/ObjectsTable.hpp`: constrained templates, unique ownership, aliases, and container protocol naming exceptions.
- `Source/Vulkan/Utils/DescriptorSetLayoutBuilder.cpp`: builder responsibilities and validation before mutation.
- `Source/Vulkan/Utils/DescriptorSetLayoutBuilder_test.cpp`: focused Catch2 cases for ordering, arrays, and reset.
