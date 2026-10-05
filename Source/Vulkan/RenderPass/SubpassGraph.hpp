#pragma once
#include <vector>

#include <Memory/Synchronizer.hpp>
#include <Private/OwnedBy.hpp>
#include <Private/Table2D.hpp>
#include <RHI.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct IInternalAttachment;
struct Context;
struct Pipeline;
} // namespace RHI::vulkan

namespace RHI::vulkan
{
enum class SubpassIndex : int32_t
{
  initialRenderPass = -1,
  finalRenderPass = std::numeric_limits<int32_t>::max(),
  Subpass0 = 0,
  Subpass1 = 1,
  Subpass2,
  Subpass3,
  Subpass4,
  Subpass5,
  //...
};

struct SubpassGraph final : public OwnedBy<Context>
{
  explicit SubpassGraph(Context & ctx, VkPipelineBindPoint bindPoint);
  virtual ~SubpassGraph() override = default;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  ShaderSlot AddAttachment(const IInternalAttachment & attachment, const ResourceState & initState,
                           const ResourceState & finalState, VkAttachmentLoadOp loadOp,
                           VkAttachmentStoreOp storeOp);

  /// @brief add subpass description to graph (the same as to add vertex to graph)
  /// @param subpass - description of subpass
  /// @return - index of subpass
  SubpassIndex AddSubpass(const Pipeline & pipeline);

  /// @brief you must add dependency if subpass should wait for some actions in previos subpass
  void AddDependency(SubpassIndex waitFor, SubpassIndex waitingSubpass, ShaderSlot attachment);

  /// @brief you must add external dependency when attachment is changed outside of RenderPass.
  /// For example: when you call vkAcquireNextImageKHR you should synchronize render pass with that external operation
  /// You do it with external dependency
  /// @param externalState - the state of attachment after external operation completed
  /// @param subpass - index of subpass that should wait for the external operation
  /// @param attachmentIdx - attachment index has been changed
  void AddExternalDependency(const ResourceState & externalState, SubpassIndex subpass,
                             uint32_t attachmentIdx);

  /// @brief You must add self-dependency when subpass must have PipelineBarriers
  /// @param subpass - index of subpass which has Pipeline barriers
  void AddSelfDependency(SubpassIndex subpass);

  /// @brief compile graph and make VkRenderPass
  /// @param device 
  /// @return true if renderPass created successfully
  bool Compile(const VkDevice & device);

  void SynchronizeAttachmentsDuringRenderPass(SubpassIndex subpassIndex,
                                              std::span<IInternalAttachment *> attachments) const;


private:
  VkPipelineBindPoint m_bindPoint;
  /// cached info about attachments
  std::vector<VkAttachmentDescription> m_attachments;
  /// cached attachments objects
  std::vector<const IInternalAttachment *> m_attachmentsObj; // TODO: shared_ptr
  /// description of subpass. A vertex of the graph
  std::vector<VkSubpassDescription> m_subpassDescriptions;
  /// transfers from one subpass to another. The edge of vertex
  std::vector<VkSubpassDependency> m_dependenciesGraph;
  /// a table with subpassCount rows and attachmentsCount columns
  /// it describes barrier, the attachment should sync into, to enter in subpassIdx
  RHI::utils::Table2D<ResourceState> m_attachmentsUsageTable;

  std::vector<ResourceState> m_prevState; ///< prev state of each attachment

private:
  size_t GetBarrierRowIndex(SubpassIndex idx) const noexcept;
  void BuildDependencyGraph(std::span<SubpassIndex> selfDependencies);
};
} // namespace RHI::vulkan
