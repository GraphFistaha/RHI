#pragma once
#include "BufferGPU.hpp"

namespace RHI::vulkan
{

struct Transferer;

struct ImageGPU : public IImageGPU,
                  private BufferBase
{
  explicit ImageGPU(const Context & ctx, BuffersAllocator & allocator, Transferer & transferer,
                    const ImageCreateArguments & args);
  virtual ~ImageGPU() override;

  virtual void UploadSync(const void * data, size_t size, size_t offset = 0) override;
  virtual void UploadAsync(const void * data, size_t size, size_t offset = 0) override;
  virtual ScopedPointer Map() override { return BufferBase::Map(); }
  virtual void Flush() const noexcept override { BufferBase::Flush(); }
  virtual bool IsMapped() const noexcept override { return BufferBase::IsMapped(); }
  virtual size_t Size() const noexcept override { return BufferBase::Size(); }

  virtual ImageType GetImageType() const noexcept override;
  virtual ImageFormat GetImageFormat() const noexcept override;

  virtual void Invalidate() noexcept override;

public:
  VkImage GetHandle() const noexcept;

private:
  const Context & m_owner;
  vk::Image m_image;
  ImageCreateArguments m_args;
};

struct ImageGPU_View final : public IImageGPU_View
{
  explicit ImageGPU_View(const Context & ctx);
  virtual ~ImageGPU_View() override;
  ImageGPU_View(ImageGPU_View && rhs) noexcept;
  ImageGPU_View & operator=(ImageGPU_View && rhs) noexcept;

  virtual void AssignImage(const IImageGPU & image) override;
  virtual bool IsImageAssigned() const noexcept override;

  virtual InternalObjectHandle GetHandle() const noexcept override;

private:
  const Context & m_context;
  vk::ImageView m_view = VK_NULL_HANDLE;

private:
  ImageGPU_View(const ImageGPU_View &) = delete;
  ImageGPU_View & operator=(const ImageGPU_View &) = delete;
};

struct ImageGPU_Sampler final : public IImageGPU_Sampler
{
  explicit ImageGPU_Sampler(const Context & ctx);
  ~ImageGPU_Sampler();

  virtual void Invalidate() override;
  virtual InternalObjectHandle GetHandle() const noexcept override;

  virtual IImageGPU_View & GetImageView() & noexcept override { return m_view; }
  virtual const IImageGPU_View & GetImageView() const & noexcept { return m_view; };

private:
  const Context & m_context;
  ImageGPU_View m_view{m_context};
  vk::Sampler m_sampler = VK_NULL_HANDLE;
  bool m_invalidSampler = true;
};

} // namespace RHI::vulkan
