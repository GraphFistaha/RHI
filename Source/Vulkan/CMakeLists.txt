add_subdirectory (ThirdParty/VkBootstrap)

target_sources (${PROJECT_NAME}
PRIVATE
	"VulkanContext.cpp"
	"VulkanContext.hpp"
	"GarbageCollector.cpp"
	"GarbageCollector.hpp"
	"Device.cpp"
	"Device.hpp"
	"Surface.cpp"
	"Surface.hpp"

	"Memory/MemoryAllocator.hpp"
	"Memory/MemoryAllocator.cpp"
	"Memory/MemoryBlock.cpp"
	"Memory/MemoryBlock.hpp"
	
	"RenderPass/Framebuffer.cpp"
	"RenderPass/Framebuffer.hpp"
	"RenderPass/Subpass.cpp"
	"RenderPass/Subpass.hpp"
	"RenderPass/SubpassLayout.cpp"
	"RenderPass/SubpassLayout.hpp"
	"RenderPass/RenderPass.cpp"
	"RenderPass/RenderPass.hpp"
	"RenderPass/RenderTarget.cpp"
	"RenderPass/RenderTarget.hpp"
	"RenderPass/SubpassConfiguration.cpp"
	"RenderPass/SubpassConfiguration.hpp"

	"Descriptors/DescriptorBufferLayout.cpp"
	"Descriptors/DescriptorBufferLayout.hpp"
	"Descriptors/DescriptorsBuffer.cpp"
	"Descriptors/DescriptorsBuffer.hpp"
	"Descriptors/BaseUniform.hpp"
	"Descriptors/SamplerUniform.hpp"
	"Descriptors/SamplerUniform.cpp"
	"Descriptors/SamplerArrayUniform.hpp"
	"Descriptors/SamplerArrayUniform.cpp"
	"Descriptors/BufferUniform.cpp"
	"Descriptors/BufferUniform.hpp"
	
	
	"Utils/CastHelper.hpp"
	"Utils/DescriptorSetLayoutBuilder.cpp"
	"Utils/DescriptorSetLayoutBuilder.hpp"
	"Utils/FramebufferBuilder.cpp"
	"Utils/FramebufferBuilder.hpp"
	"Utils/PipelineBuilder.cpp" 
	"Utils/PipelineBuilder.hpp"
	"Utils/PipelineLayoutBuilder.cpp"
	"Utils/PipelineLayoutBuilder.hpp"
	"Utils/RenderPassBuilder.cpp"
	"Utils/RenderPassBuilder.hpp"
	"Utils/SamplerBuilder.cpp"
	"Utils/SamplerBuilder.hpp"
	"Utils/FenceBuilder.hpp"
	"Utils/FenceBuilder.cpp"
	"Utils/SemaphoreBuilder.hpp"
	"Utils/SemaphoreBuilder.cpp"
	"Utils/EventBuilder.hpp"
	"Utils/EventBuilder.cpp"
	
	"ImageUtils/InternalImageTraits.hpp"
	"ImageUtils/ImageFormatsConversation.cpp"
	"ImageUtils/ImageFormatsConversation.hpp"
	"ImageUtils/ImageUtils.hpp"
	"ImageUtils/ImageUtils.cpp"

	"Attachments/GenericAttachment.cpp"
	"Attachments/GenericAttachment.hpp"
	"Attachments/Attachment.hpp"
	"Attachments/SurfacedAttachment.cpp"
	"Attachments/SurfacedAttachment.hpp"
	
	"Resources/TextureInterface.hpp"
	"Resources/Texture.cpp"
	"Resources/Texture.hpp"
	"Resources/BufferGPU.cpp"
	"Resources/BufferGPU.hpp"
	"Resources/BufferInterface.hpp"
	"Resources/Synchronizer.cpp"
	"Resources/Synchronizer.hpp"

	"TransferPass/Transferer.hpp"
	"TransferPass/Transferer.cpp"
	"TransferPass/TransferTask.hpp"
	"TransferPass/TransferTask.cpp"
	"TransferPass/TransferAlgorithm.hpp"
	"TransferPass/Blit.cpp"
	"TransferPass/Buffer.cpp"
	"TransferPass/Image.cpp"
	"TransferPass/Mipmaps.cpp"
	
	"CommandsExecution/InternalAwaitable.hpp"
	"CommandsExecution/CommandBuffer.cpp" 
	"CommandsExecution/CommandBuffer.hpp" 
	"CommandsExecution/Submitter.cpp"
	"CommandsExecution/Submitter.hpp"
	"CommandsExecution/SubmitTask.cpp"
	"CommandsExecution/SubmitTask.hpp"
	"CommandsExecution/DoubleBufferedSubmitter.hpp"
	"CommandsExecution/DoubleBufferedSubmitter.cpp"
)

#---------------- Link third-party libraries ---------------
#find_package(Vulkan REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(VulkanLoader REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

target_link_libraries(${PROJECT_NAME}
PRIVATE
	#Vulkan::Vulkan
	vulkan-headers::vulkan-headers
	Vulkan::Loader
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${PROJECT_NAME}
PRIVATE
 "./"
)