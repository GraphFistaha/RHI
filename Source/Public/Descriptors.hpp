#pragma once
#include <cstdint>
#include <memory>

namespace RHI
{
struct IBufferGPU;
struct ITexture;

// clang-format off
enum class ShaderSlot : uint8_t
{
  _0 = 0,
  _1, _2, _3, _4, _5,
  _6, _7, _8, _9, _10,
  _11, _12, _13, _14, _15,
  _16, _17, _18, _19, _20,
  _21, _22, _23, _24, _25,
  _26, _27, _28, _29, _30,
  _31, _32,
  Max = 32,
  Invalid = 255,
};
// clang-format on

using ShaderBinding = ShaderSlot;
using ShaderSet = ShaderSlot;

struct LayoutIndex final
{
  ShaderSet set = ShaderSet::_0;
  ShaderBinding binding = ShaderBinding::_0;

  bool operator==(const LayoutIndex & rhs) const noexcept
  {
    return set == rhs.set && binding == rhs.binding;
  }

  inline bool IsValid() const noexcept
  {
    return set != ShaderSet::Invalid && binding != ShaderBinding::Invalid;
  }
};


enum class TextureWrapping : uint8_t
{
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder
};

enum class TextureFilteration : uint8_t
{
  Nearest,
  Linear
};


struct IUniformDescriptor
{
  virtual ~IUniformDescriptor() = default;
  virtual uint32_t GetSet() const noexcept = 0;
  virtual uint32_t GetBinding() const noexcept = 0;
  virtual uint32_t GetArrayIndex() const noexcept = 0;
};

struct ISamplerDescriptor
{
  virtual ~ISamplerDescriptor() = default;
  virtual void SetWrapping(RHI::TextureWrapping uWrap, RHI::TextureWrapping vWrap,
                           RHI::TextureWrapping wWrap) noexcept = 0;
  virtual void SetFilter(RHI::TextureFilteration minFilter,
                         RHI::TextureFilteration magFilter) noexcept = 0;
};

struct ISamplerUniformDescriptor : public IUniformDescriptor,
                                   public ISamplerDescriptor
{
  virtual void AssignImage(ITexture * texture) = 0;
};

struct ISamplerArrayUniformDescriptor : public IUniformDescriptor,
                                        public ISamplerDescriptor
{
  virtual void AssignImage(uint32_t index, ITexture * texture) = 0;
};

struct IBufferUniformDescriptor : public IUniformDescriptor
{
  virtual void AssignBuffer(IBufferGPU * buffer, size_t offset = 0) = 0;
  virtual bool IsBufferAssigned() const noexcept = 0;
};


} // namespace RHI


namespace std
{
template<>
struct hash<RHI::LayoutIndex>
{
  std::size_t operator()(const RHI::LayoutIndex & x) const
  {
    return static_cast<size_t>(static_cast<uint32_t>(x.set) << 8 |
                               static_cast<uint32_t>(x.binding));
  }
};

} // namespace std
