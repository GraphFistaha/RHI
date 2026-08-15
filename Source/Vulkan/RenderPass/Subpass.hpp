#pragma once
#include <atomic>
#include <mutex>

#include <CommandsExecution/CommandBuffer.hpp>
#include <Descriptors/DescriptorBufferLayout.hpp>
#include <Descriptors/DescriptorsBuffer.hpp>
#include <Private/OwnedBy.hpp>
#include <RenderPass/PipelineProcess.hpp>
#include <RenderPass/SubpassLayout.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct Context;
struct SubpassConfiguration;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
struct Subpass : public OwnedBy<Context>,
                 public OwnedBy<SubpassConfiguration>,
                 public ICommandWriter
{
  using UsedAttachments = std::unordered_map<uint32_t, RHI::ShaderAttachmentSlot>;
  explicit Subpass(Context & ctx, SubpassConfiguration & owner, uint32_t subpassIndex,
                   uint32_t familyIndex);
  virtual ~Subpass() override;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);
  MAKE_ALIAS_FOR_GET_OWNER(SubpassConfiguration, GetPipeline);

public: //ICommandWriter
  virtual void RecordCommands(details::CommandBuffer & commands) override;

public: // IResourceUser
  void CollectResources(std::vector<ResourcePtr> & resources) const;
  void SynchroniseResources(details::CommandBuffer & commands) const;

public:
  const SubpassLayout & GetLayout() const & noexcept;
  SubpassLayout & GetLayout() & noexcept;
  const DescriptorBufferLayout & GetDescriptorsLayout() const & noexcept;
  DescriptorBufferLayout & GetDescriptorsLayout() & noexcept;
  DescriptorBuffer & GetDescriptorBuffer() & noexcept;


  void SetRenderProcess(PipelineProcessPtr process);
  void SetDirtyCommands() noexcept;

  void SetInvalid();
  void Invalidate();

  void OnDescriptorChanged(UpdateDescriptorTask task) noexcept
  {
    m_execDescriptorBuffer.UpdateDescriptor(task);
    m_writeDescriptorBuffer.UpdateDescriptor(task);
    SetDirtyCommands();
  }

private:
  std::shared_ptr<PipelineProcess> m_process;
  std::atomic_bool m_enabled = true;
  std::atomic_bool m_dirtyCommands;

  details::CommandBuffer m_execBuffer;
  details::CommandBuffer m_writeBuffer;
  DescriptorBufferLayout m_descriptorsLayout;
  DescriptorBuffer m_execDescriptorBuffer;
  DescriptorBuffer m_writeDescriptorBuffer;

  SubpassLayout m_layout{VK_PIPELINE_BIND_POINT_GRAPHICS};
};
} // namespace RHI::vulkan
