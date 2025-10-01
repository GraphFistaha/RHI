add_subdirectory(ThirdParty/VulkanMemoryAllocator)
add_subdirectory(ThirdParty/VkBootstrap)

target_sources (${PROJECT_NAME}
PRIVATE
	"VulkanContext.cpp"
	"VulkanContext.hpp"
	"GarbageCollector.cpp"
	"GarbageCollector.hpp"
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
	
	
	"ImageUtils/ImageLayoutTransferer.cpp"
	"ImageUtils/ImageLayoutTransferer.hpp"
	"ImageUtils/InternalImageTraits.hpp"
	"ImageUtils/ImageFormatsConversation.cpp"
	"ImageUtils/ImageFormatsConversation.hpp"
	"ImageUtils/TextureInterface.hpp"
	"ImageUtils/ImageUtils.hpp"
	"ImageUtils/ImageUtils.cpp"

	"Attachments/GenericAttachment.cpp"
	"Attachments/GenericAttachment.hpp"
	"Attachments/Attachment.hpp"
	"Attachments/SurfacedAttachment.cpp"
	"Attachments/SurfacedAttachment.hpp"
	
	"Resources/Texture.cpp"
	"Resources/Texture.hpp"
	"Resources/BufferGPU.cpp"
	"Resources/BufferGPU.hpp"
	"Resources/Transferer.hpp"
	"Resources/Transferer.cpp"
	
	
	"CommandsExecution/CommandBuffer.cpp" 
	"CommandsExecution/CommandBuffer.hpp" 
	"CommandsExecution/Submitter.cpp"
	"CommandsExecution/Submitter.hpp"
	"CommandsExecution/AsyncTask.cpp"
	"CommandsExecution/AsyncTask.hpp"
	"CommandsExecution/CompositeAsyncTask.cpp"
	"CommandsExecution/CompositeAsyncTask.hpp"
)

#---------------- Link third-party libraries ---------------
find_package(Vulkan REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

#find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
PRIVATE
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)

target_include_directories(${PROJECT_NAME}
PRIVATE
 "./"
)