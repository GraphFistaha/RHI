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
	
	"Graphics/Framebuffer.cpp"
	"Graphics/Framebuffer.hpp"
	"Graphics/Subpass.cpp"
	"Graphics/Subpass.hpp"
	"Graphics/SubpassLayout.cpp"
	"Graphics/SubpassLayout.hpp"
	"Graphics/RenderPass.cpp"
	"Graphics/RenderPass.hpp"
	"Graphics/RenderTarget.cpp"
	"Graphics/RenderTarget.hpp"
	"Graphics/SubpassConfiguration.cpp"
	"Graphics/SubpassConfiguration.hpp"

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
	"Utils/ShaderCompiler.hpp"
	"Utils/FenceBuilder.hpp"
	"Utils/FenceBuilder.cpp"
	"Utils/SemaphoreBuilder.hpp"
	"Utils/SemaphoreBuilder.cpp"
	
	
	"Images/ImageLayoutTransferer.cpp"
	"Images/ImageLayoutTransferer.hpp"
	"Images/InternalImageTraits.hpp"
	"Images/ImageFormatsConversation.cpp"
	"Images/ImageFormatsConversation.hpp"
	"Images/TextureInterface.hpp"
	"Images/ImageUtils.hpp"
	"Images/ImageUtils.cpp"
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