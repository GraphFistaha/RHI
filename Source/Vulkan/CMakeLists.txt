add_subdirectory (ThirdParty/VkBootstrap)

set(this_target "RHI_vulkan_backend")
set(backend_target ${this_target} PARENT_SCOPE)
add_library(${this_target} OBJECT)
set_target_properties(${this_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources (${this_target}
PUBLIC
	"VulkanContext.hpp"
	"GarbageCollector.hpp"
	"Device.hpp"
	"Surface.hpp"

	"Memory/MemoryAllocator.hpp"
	"Memory/MemoryBlock.hpp"
	"Memory/TextureInterface.hpp"
	"Memory/Texture.hpp"
	"Memory/BufferGPU.hpp"
	"Memory/BufferInterface.hpp"
	"Memory/Synchronizer.hpp"
	"Memory/LegacySynchronization.hpp"
	"Memory/ResourceUser.hpp"

	"RenderPass/Framebuffer.hpp"
	"RenderPass/RenderTarget.hpp"
	"RenderPass/SubpassGraph.hpp"
	"RenderPass/SubpassSynchronization.hpp"
	"RenderPass/AttachmentDescriptionBuilder.hpp"
	"RenderPass/RenderPass.hpp"

	"Pipeline/DescriptorBufferLayout.hpp"
	"Pipeline/DescriptorsBuffer.hpp"
	"Pipeline/BaseDescriptor.hpp"
	"Pipeline/SamplerUniform.hpp"
	"Pipeline/SamplerArrayUniform.hpp"
	"Pipeline/BufferUniform.hpp"
	"Pipeline/InputAttachmentUniform.hpp"
	"Pipeline/UpdateDescriptorTask.hpp"
	"Pipeline/Pipeline.hpp"
	"Pipeline/PipelineAttachmentsUsage.hpp"

	"Utils/CastHelper.hpp"
	"Utils/DescriptorSetLayoutBuilder.hpp"
	"Utils/FramebufferBuilder.hpp"
	"Utils/PipelineBuilder.hpp"
	"Utils/PipelineLayoutBuilder.hpp"
	"Utils/SamplerBuilder.hpp"
	"Utils/FenceBuilder.hpp"
	"Utils/SemaphoreBuilder.hpp"
	"Utils/EventBuilder.hpp"

	"ImageUtils/InternalImageTraits.hpp"
	"ImageUtils/ImageFormatsConversation.hpp"
	"ImageUtils/ImageUtils.hpp"

	"Attachments/GenericAttachment.hpp"
	"Attachments/Attachment.hpp"
	"Attachments/SurfacedAttachment.hpp"

	"TransferPass/Transferer.hpp"
	"TransferPass/TransferTask.hpp"
	"TransferPass/TransferAlgorithm.hpp"

	"CommandsExecution/InternalAwaitable.hpp"
	"CommandsExecution/CommandBuffer.hpp"
	"CommandsExecution/Submitter.hpp"
	"CommandsExecution/SubmitTask.hpp"
	"CommandsExecution/DoubleBufferedSubmitter.hpp"

PRIVATE
	"VulkanContext.cpp"
	"GarbageCollector.cpp"
	"Device.cpp"
	"Surface.cpp"

	"Memory/MemoryAllocator.cpp"
	"Memory/MemoryBlock.cpp"
	"Memory/Texture.cpp"
	"Memory/BufferGPU.cpp"
	"Memory/Synchronizer.cpp"
	"Memory/LegacySynchronization.cpp"

	"RenderPass/Framebuffer.cpp"
	"RenderPass/RenderTarget.cpp"
	"RenderPass/SubpassGraph.cpp"
	"RenderPass/SubpassSynchronization.cpp"
	"RenderPass/AttachmentDescriptionBuilder.cpp"
	"RenderPass/RenderPass.cpp"
	"RenderPass/RenderPass.hpp"
	"RenderPass/SubpassInfo.hpp"
	"RenderPass/SubpassInfo.cpp"

	"Pipeline/DescriptorBufferLayout.cpp"
	"Pipeline/DescriptorsBuffer.cpp"
	"Pipeline/SamplerUniform.cpp"
	"Pipeline/SamplerArrayUniform.cpp"
	"Pipeline/BufferUniform.cpp"
	"Pipeline/InputAttachmentUniform.cpp"
	"Pipeline/Pipeline.cpp"
	"Pipeline/PipelineAttachmentsUsage.cpp"

	"Utils/DescriptorSetLayoutBuilder.cpp"
	"Utils/FramebufferBuilder.cpp"
	"Utils/PipelineBuilder.cpp"
	"Utils/PipelineLayoutBuilder.cpp"
	"Utils/SamplerBuilder.cpp"
	"Utils/FenceBuilder.cpp"
	"Utils/SemaphoreBuilder.cpp"
	"Utils/EventBuilder.cpp"

	"ImageUtils/ImageFormatsConversation.cpp"
	"ImageUtils/ImageUtils.cpp"

	"Attachments/GenericAttachment.cpp"
	"Attachments/SurfacedAttachment.cpp"

	"TransferPass/Transferer.cpp"
	"TransferPass/TransferTask.cpp"
	"TransferPass/Blit.cpp"
	"TransferPass/Buffer.cpp"
	"TransferPass/Image.cpp"
	"TransferPass/Mipmaps.cpp"

	"CommandsExecution/CommandBuffer.cpp"
	"CommandsExecution/Submitter.cpp"
	"CommandsExecution/SubmitTask.cpp"
	"CommandsExecution/DoubleBufferedSubmitter.cpp"
)

#---------------- Link third-party libraries ---------------
#find_package(Vulkan REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(VulkanLoader REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_compile_definitions(${this_target} 
PUBLIC
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)


target_link_libraries(${this_target}
PUBLIC
	#Vulkan::Vulkan
    vulkan-headers::vulkan-headers
    Vulkan::Loader
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${this_target} 
PUBLIC 
	"./"
)

if (RHI_BUILD_TESTS)
    find_package(Catch2 REQUIRED)
    add_executable(RHI_Vulkan_Tests
        "Utils/DescriptorSetLayoutBuilder_test.cpp"
        "Memory/LegacySynchronization_test.cpp"
    )
    target_link_libraries(RHI_Vulkan_Tests 
	PRIVATE Catch2::Catch2WithMain ${this_target} RHI_Interface
	)
    target_include_directories(RHI_Vulkan_Tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
    target_compile_definitions(RHI_Vulkan_Tests PRIVATE RHI_BUILD)
    add_test(NAME RHI_Vulkan_Tests COMMAND $<TARGET_FILE:RHI_Vulkan_Tests>)
endif()
