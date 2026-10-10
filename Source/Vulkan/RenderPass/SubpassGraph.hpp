#pragma once
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include <Memory/Synchronizer.hpp>
#include <Private/OwnedBy.hpp>
#include <vulkan/vulkan.h>

namespace RHI::vulkan
{
struct IInternalAttachment;
struct Context;
struct Pipeline;

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
};

/// Builds and owns one render pass. Not thread-safe; the context must outlive this object.
/// Successful compilation freezes the graph. Rebuilding requires a new graph.
struct SubpassGraph final : public OwnedBy<Context>
{
  explicit SubpassGraph(Context & ctx);
  ~SubpassGraph() override;
  SubpassGraph(const SubpassGraph &) = delete;
  SubpassGraph & operator=(const SubpassGraph &) = delete;
  SubpassGraph(SubpassGraph &&) = delete;
  SubpassGraph & operator=(SubpassGraph &&) = delete;
  MAKE_ALIAS_FOR_GET_OWNER(Context, GetContext);

public:
  /// Returns an index in this graph's attachment array (not a shader location).
  /// Register attachments before subpasses that reference them. Depth/stencil share load/store ops.
  /// Boundary states track usage; initial/final layouts are supplied to Vulkan.
  uint32_t AddAttachment(const IInternalAttachment & attachment, const ResourceState & initState,
                         const ResourceState & finalState, VkAttachmentLoadOp loadOp,
                         VkAttachmentStoreOp storeOp);

  /// Pipeline attachment arrays must remain alive and unchanged until Compile finishes.
  /// References use graph attachment indices. Subpasses execute in insertion order.
  SubpassIndex AddSubpass(const Pipeline & pipeline);

  /// Connects distinct forward-ordered subpasses using the attachment. No automatic dependencies.
  void AddDependency(SubpassIndex source, SubpassIndex destination, uint32_t attachment);

  /// External scopes describe earlier/later accesses. Layouts must match the attachment boundary,
  /// except an UNDEFINED initial layout discards contents regardless of the prior external layout.
  /// These do not perform semaphore waits or queue ownership transfers.
  void AddIncomingDependency(const ResourceState & externalState, SubpassIndex destination,
                             uint32_t attachment);
  void AddOutgoingDependency(SubpassIndex source, const ResourceState & externalState,
                             uint32_t attachment);

  /// Enables barriers within these scopes; does not perform layout transitions.
  /// Framebuffer-space scopes use BY_REGION; other scopes use a global dependency.
  void AddSelfDependency(SubpassIndex subpass, const ResourceState & sourceState,
                         const ResourceState & destinationState);

  /// Uses the context device and vkCreateRenderPass. Vulkan failures return false and are logged.
  /// Failed creation permits retry. Invalid descriptions and modification/recompile after success throw.
  /// Core synchronization2 masks are converted to legacy equivalents; unsupported masks throw.
  bool Compile();
  VkRenderPass GetHandle() const noexcept { return m_renderPass; }

  /// Updates CPU tracking only; records no barriers. Call at the corresponding execution boundary.
  /// Attachments must be alive, in graph index order, and have exactly the registered count.
  /// Boundary sentinels select initial/final states; unused subpass attachments retain tracked state.
  void SynchronizeAttachmentsDuringRenderPass(SubpassIndex subpassIndex,
                                              std::span<IInternalAttachment *> attachments) const;

private:
  /// Bind point shared by all subpasses in this graphics render pass.
  static constexpr VkPipelineBindPoint m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  /// Owned handle; null until compilation succeeds. Destruction is deferred through the context.
  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  /// Native attachment descriptions in graph attachment index order.
  std::vector<VkAttachmentDescription> m_attachments;
  /// Boundary tracking states, indexed identically to m_attachments.
  std::vector<ResourceState> m_initialStates;
  std::vector<ResourceState> m_finalStates;
  /// Insertion-ordered subpasses; attachment reference arrays are borrowed from pipelines.
  std::vector<VkSubpassDescription> m_subpassDescriptions;
  /// Explicit internal, external, and self dependencies passed to render pass creation.
  std::vector<VkSubpassDependency> m_dependenciesGraph;
  /// States indexed by subpass then attachment; UNDEFINED layout marks unused attachments.
  std::vector<std::vector<ResourceState>> m_attachmentsUsage;

private:
  void RequireMutable() const;
  size_t GetSubpassIndex(SubpassIndex index) const;
  const ResourceState & GetAttachmentState(SubpassIndex subpass, uint32_t attachment) const &;
  void AppendDependency(uint32_t source, uint32_t destination, const ResourceState & sourceState,
                        const ResourceState & destinationState, bool byRegion);
};
} // namespace RHI::vulkan
