#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace RHI
{

/// @brief Operation System handle
using ExternalHandle = void *;
/// @brief Internal handle (may be vulkan/DirectX/Metal object)
using InternalObjectHandle = void *;

enum class LogMessageStatus : uint8_t
{
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_DEBUG
};

typedef void (*LoggingFunc)(LogMessageStatus, const std::string &);


/// @brief Generic settings to create a surface
struct SurfaceConfig
{
  /// Generic window Handle (HWND Handle in Windows, X11Window handle in Linux)
  ExternalHandle hWnd;
  /// Additional window handle (HINSTANCE handle in Windows, Display in Linux)
  ExternalHandle hInstance;
};


#define BIT(x) (1 << (x))
/// @brief types of shader which can be attached to pipeline
enum class ShaderType : uint8_t
{
  Vertex = BIT(0),
  TessellationControl = BIT(1),
  TessellationEvaluation = BIT(2),
  Geometry = BIT(3),
  Fragment = BIT(4),
  Compute = BIT(5),
};

/// @brief a way to connect and interpret vertices in VertexData
enum class MeshTopology : uint8_t
{
  Point,        ///< every vertex is point
  Line,         ///< vertices grouped by 2 and draws as line
  LineStrip,    ///< vertices grouped by 2 and draws as line
  Triangle,     ///< vertices grouped by 3 and draws as triangle
  TriangleFan,  ///< vertices grouped by 3 and draws as triangle
  TriangleStrip ///< vertices grouped by 3 and draws as triangle
};

enum class PolygonMode : uint8_t
{
  Fill, ///< Fill polygon by color
  Line, ///< Draw only border of polygon
  Point ///< Draw polygon as point
};

/// @brief A way to define which polygon is front and which is back.
enum class FrontFace : uint8_t
{
  CW, //clockwise
  CCW // counter clockwise
};

/// @brief modes for culling
enum class CullingMode : uint8_t
{
  None,        ///< Culling mode is disabled
  FrontFace,   ///< front faces will be culled
  BackFace,    ///< back faces will be culled
  FrontAndBack ///< front and back polygons will be culled
};

/// @brief binary operation used in blending. It's like operation(source_factor * source_color, dest_factor * dest_color)
enum class BlendOperation : uint8_t
{
  Add,              // src.color + dst.color
  Subtract,         // src.color - dst.color
  ReversedSubtract, //dst.color - src.color
  Min,              // min(src.color, dst.color)
  Max               // max(src.color, dst.color)
};

/// @brief factor, which will be multiplied on color in blending
enum class BlendFactor : uint8_t
{
  Zero, ///<
  One,
  SrcColor,
  OneMinusSrcColor,
  DstColor,
  OneMinusDstColor,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha,
  ConstantColor,
  OneMinusConstantColor,
  ConstantAlpha,
  OneMinusConstantAlpha,
  SrcAlphaSaturate,
  Src1Color,
  OneMinusSrc1Color,
  Src1Alpha,
  OneMinusSrc1Alpha,
};

/// @brief
enum class ShaderImageSlot : uint8_t
{
  Color,
  DepthStencil,
  Input,
  TOTAL
};

/// @brief types of command buffers
enum class CommandBufferType : uint8_t
{
  Executable, ///< executable in GPU
  ThreadLocal ///< filled in separate thread
};

/// @brief enum to define usage of BufferGPU
enum class BufferGPUUsage : uint8_t
{
  VertexBuffer,
  IndexBuffer,
  UniformBuffer,
  StorageBuffer,
  TransformFeedbackBuffer,
  IndirectBuffer
};

/// @brief defines what input attribute is used for. For instance or for each vertex
enum class InputBindingType : uint8_t
{
  VertexData,  ///< input attribute will be associated with vertex
  InstanceData ///< input attribute will be associeated with instance
};

/// @brief types for input attributes
enum class InputAttributeElementType : uint8_t
{
  FLOAT,  ///< Input attribute will be interpreted in shader as float
  DOUBLE, ///< Input attribute will be interpreted in shader as double
  UINT,   ///< Input attribute will be interpreted in shader as unsigned int
  SINT    ///< Input attribute will be interpreted in shader as signed int
};

/// @brief data type for vertex indices
enum class IndexType : uint8_t
{
  UINT8,  ///< indices will be interpreted in driver as uint8_t*
  UINT16, ///< indices will be interpreted in driver as uint16_t*
  UINT32  ///< indices will be interpreted in driver as uint32_t*
};

//----------------- Images ---------------------

/// @brief Defines image's layout in memory. Also defines if image can have mipmaps or not
///        For example image1d is line of pixels, image2d is a rectangle of pixels
enum class ImageType
{
  Image1D, ///< image with height = 1. has only width.Can have one mipmap for a whole image
  Image2D, ///< generic image with width and height. Can have one mipmap for a whole image
  Image3D, ///< layered image2d (with depth). Can have one mipmap for a whole image
  Image1D_Array, ///< array of 1d images, layered in memory like 2d image, but each row of pixels has its own mipmap
  Image2D_Array, ///< the same as image3d, but each layer should have own mipmap
  Cubemap,       ///< it's image2d_array with length = 6
};

/// @brief internal image format.
enum class ImageFormat : uint8_t
{
  // general formats
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // service formats
  DEPTH,
  DEPTH_STENCIL,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

enum class ImageUsage : uint8_t
{
  SHADER_INPUT = BIT(1),
  SHADER_OUTPUT = BIT(2),
  SAMPLER = BIT(3)
};

enum class SamplesCount : uint8_t
{
  One = 1,
  Two = 2,
  Four = 4,
  Eight = 8,
};

/// For Image1D used only 0'th index
/// For Image2D used only 0 and 1 index
/// For Image3D used all 3 indices like width, height and layer
/// For Image1D used 0 and 1 index as width and i-th array element
/// For Image2D used all 3 indices as width, height and i-th array element
/// For Cubemap used all 3 indices as width, height and i-th surface of cube
using ImageExtent = std::array<uint32_t, 3>;

struct ImageRegion
{
  ImageExtent offset;
  ImageExtent extent;
};

struct ImageDescription final
{
  ImageExtent extent;
  uint32_t mipLevels;
  ImageType type;
  ImageFormat format;
  SamplesCount samples;
  ImageUsage usage;
  bool shared;
};

/// @brief Defines in what format image will be uploaded or downloaded
enum class HostImageFormat : uint8_t
{
  R8,
  A8,
  RG8,
  BGR8,
  RGB8,
  RGBA8,
  BGRA8,
  // compressed types
  //BC1,
  //BC5,
  //BC7
};

struct CopyImageArguments
{
  HostImageFormat hostFormat;
  ImageRegion src;
  ImageRegion dst;
};

struct IBufferGPU;
struct IImageGPU;
using SemaphoreHandle = InternalObjectHandle;

struct IInvalidable
{
  virtual ~IInvalidable() = default;
  virtual void SetInvalid() = 0;
  virtual void Invalidate() = 0;
};

struct IUniformDescriptor : public IInvalidable
{
  virtual ~IUniformDescriptor() = default;
  virtual uint32_t GetBinding() const noexcept = 0;
  virtual uint32_t GetArrayIndex() const noexcept = 0;
};

struct ISamplerUniformDescriptor : public IUniformDescriptor
{
  virtual void AssignImage(const IImageGPU & image) = 0;
  virtual bool IsImageAssigned() const noexcept = 0;
};

struct IBufferUniformDescriptor : public IUniformDescriptor
{
  virtual void AssignBuffer(const IBufferGPU & buffer, size_t offset = 0) = 0;
  virtual bool IsBufferAssigned() const noexcept = 0;
};

/// @brief Pipeline is container for rendering state settings (like shaders, input attributes, uniforms, etc).
/// It has two modes: editing and drawing. In editing mode you can change any settings (attach shaders, uniforms, set viewport, etc).
/// After editing you must call Invalidate(), it rebuilds internal objects and applyies new configuration.
/// After invalidate you can bind it to CommandBuffer and draw.
struct IPipeline : public IInvalidable
{
  virtual ~IPipeline() = default;
  // General static settings
  /// @brief attach shader to pipeline
  virtual void AttachShader(ShaderType type, const std::filesystem::path & path) = 0;
  virtual void SetAttachmentUsage(ShaderImageSlot slot, uint32_t binding) = 0;

  virtual void AddInputBinding(uint32_t slot, uint32_t stride, InputBindingType type) = 0;
  virtual void AddInputAttribute(uint32_t binding, uint32_t location, uint32_t offset,
                                 uint32_t elemsCount, InputAttributeElementType elemsType) = 0;
  virtual void DefinePushConstant(uint32_t size, ShaderType shaderStage) = 0;

  virtual IBufferUniformDescriptor * DeclareUniform(uint32_t binding, ShaderType shaderStage) = 0;
  virtual ISamplerUniformDescriptor * DeclareSampler(uint32_t binding, ShaderType shaderStage) = 0;
  virtual void DeclareSamplersArray(uint32_t binding, ShaderType shaderStage, uint32_t size,
                                    ISamplerUniformDescriptor * out_array[]) = 0;

  /// @brief Get subpass index
  virtual uint32_t GetSubpass() const = 0;
};

// IRenderTarget
/// @brief Framebuffer is a set of images to render
struct IRenderTarget
{
  virtual ~IRenderTarget() = default;
  virtual ImageExtent GetExtent() const noexcept = 0;
  virtual void SetClearValue(uint32_t attachmentIndex, float r, float g, float b,
                             float a) noexcept = 0;
  virtual void SetClearValue(uint32_t attachmentIndex, float depth, uint32_t stencil) noexcept = 0;
  virtual IImageGPU * GetImage(uint32_t attachmentIndex) const = 0;
};

struct ISubpass /* : IInvalidable*/
{
  virtual ~ISubpass() = default;
  virtual void BeginPass() = 0;
  virtual void EndPass() = 0;
  virtual IPipeline & GetConfiguration() & noexcept = 0;
  virtual void SetEnabled(bool enabled) noexcept = 0;
  virtual bool IsEnabled() const noexcept = 0;
  virtual bool ShouldBeInvalidated() const noexcept = 0;

  /// @brief draw vertices command (analog glDrawArrays)
  virtual void DrawVertices(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0,
                            uint32_t firstInstance = 0) = 0;
  /// @brief draw vertices with indieces (analog glDrawElements)
  virtual void DrawIndexedVertices(uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                   uint32_t firstInstance = 0) = 0;
  /// @brief Set viewport command
  virtual void SetViewport(float width, float height) = 0;
  /// @brief Set scissor command
  virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
  /// @brief binds buffer as input attribute data
  virtual void BindVertexBuffer(uint32_t binding, const IBufferGPU & buffer,
                                uint32_t offset = 0) = 0;
  /// @brief binds buffer as index buffer
  virtual void BindIndexBuffer(const IBufferGPU & buffer, IndexType type, uint32_t offset = 0) = 0;
  virtual void PushConstant(const void * data, size_t size) = 0;
};

// IFramebuffer
/// @brief Swapchain is a queue of renderTargets. Each renderTarget is a union of renderPass and internalFramebuffer.
struct ISwapchain
{
  virtual ~ISwapchain() = default;
  virtual IRenderTarget * AcquireFrame() = 0;
  virtual void FlushFrame() = 0;
  virtual void SetFramesCount(uint32_t frames_count) noexcept = 0;
  virtual void SetExtent(const ImageExtent & extent) noexcept = 0;
  virtual void SetMultisampling(RHI::SamplesCount samples) noexcept = 0;
  virtual void AddImageAttachment(uint32_t binding, const ImageDescription & args) = 0;
  virtual void ClearImageAttachments() noexcept = 0;
  virtual ISubpass * CreateSubpass() = 0;
};

struct ITransferer
{
  virtual ~ITransferer() = default;
  virtual SemaphoreHandle Flush() = 0;
};

// ------------------- Data ------------------

/// @brief Generic data buffer in GPU. You can map it on CPU memory and change.
/// After mapping changed data can be sent to GPU. Use Flush method to be sure that data is sent
struct IBufferGPU
{
  using UnmapFunc = std::function<void(char *)>;
  using ScopedPointer = std::unique_ptr<char, UnmapFunc>;

  virtual ~IBufferGPU() = default;
  /// @brief uploads data
  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) = 0;
  virtual void UploadAsync(const void * data, size_t size, size_t offset = 0) = 0;
  /// @brief Map buffer into CPU memory.  It will be unmapped in end of scope
  virtual ScopedPointer Map() = 0;
  /// @brief Sends changed buffer after Map to GPU
  virtual void Flush() const noexcept = 0;
  /// @brief checks if buffer is mapped into CPU memory
  virtual bool IsMapped() const noexcept = 0;
  /// @brief Get size of buffer in bytes
  virtual size_t Size() const noexcept = 0;
};


struct IImageGPU
{
  virtual ~IImageGPU() = default;
  virtual void UploadImage(const uint8_t * srcPixelData, const CopyImageArguments & args) = 0;
  //virtual void DownloadImage(void * srcPixelData, CopyImageArguments & args) = 0;
  virtual ImageDescription GetDescription() const noexcept = 0;
  /// @brief Get size of image in bytes
  virtual size_t Size() const = 0;
  //virtual void SetSwizzle() = 0;
};

/// @brief Context is a main container for all objects above. It can creates some user-defined objects like buffers, framebuffers, etc
struct IContext
{
  virtual ~IContext() = default;

  virtual ISwapchain * GetSurfaceSwapchain() = 0;
  virtual std::unique_ptr<ISwapchain> CreateOffscreenSwapchain(uint32_t width, uint32_t height,
                                                               uint32_t frames_count) = 0;
  virtual ITransferer * GetTransferer() const noexcept = 0;
  virtual void ClearResources() = 0;


  /// @brief create offscreen framebuffer
  //virtual std::unique_ptr<IFramebuffer> CreateFramebuffer() const = 0;
  /// @brief creates BufferGPU
  virtual std::unique_ptr<IBufferGPU> AllocBuffer(size_t size, BufferGPUUsage usage,
                                                  bool mapped = false) const = 0;

  virtual std::unique_ptr<IImageGPU> AllocImage(const ImageDescription & args) const = 0;
};

/// @brief Factory-function to create context
std::unique_ptr<IContext> CreateContext(const SurfaceConfig * config = nullptr,
                                        LoggingFunc loggingFunc = nullptr);

namespace details
{

/// @brief changes shader filename path for current API and extension format
/// @param path - shader filename path in GLSL format
/// @param extension - extension that should have result file
/// @return new shader filename adapted for current file format
inline std::filesystem::path ResolveShaderExtension(const std::filesystem::path & path,
                                                    const std::filesystem::path & apiFolder,
                                                    const std::filesystem::path & extension)
{
  std::filesystem::path suffix;
  auto && ext = path.extension();
  if (ext == ".vert")
    suffix = "_vert";
  else if (ext == ".frag")
    suffix = "_frag";
  else if (ext == ".geom")
    suffix = "_geom";
  else if (ext == ".glsl")
  {
    suffix = "";
  }
  else
    assert(false && "Invalid format for shader file");
  std::filesystem::path result = apiFolder / path.parent_path() / path.stem();
  result += suffix;
  result += extension;
  return result;
}
} // namespace details

} // namespace RHI


constexpr inline RHI::ShaderType operator|(RHI::ShaderType t1, RHI::ShaderType t2)
{
  return static_cast<RHI::ShaderType>(static_cast<uint8_t>(t1) | static_cast<uint8_t>(t2));
}

constexpr inline RHI::ShaderType operator&(RHI::ShaderType t1, RHI::ShaderType t2)
{
  return static_cast<RHI::ShaderType>(static_cast<uint8_t>(t1) & static_cast<uint8_t>(t2));
}
