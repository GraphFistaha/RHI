target_sources (${PROJECT_NAME}
PRIVATE
	VulkanContext.cpp
	VulkanContext.hpp
	GarbageCollector.cpp
	GarbageCollector.hpp
	Memory/MemoryAllocator.hpp
	Memory/MemoryAllocator.cpp
	Memory/MemoryBlock.cpp
	Memory/MemoryBlock.hpp

	Graphics/Swapchain.cpp
	Graphics/Swapchain.hpp
	Graphics/PresentativeSwapchain.cpp
	Graphics/PresentativeSwapchain.hpp
	Graphics/DescriptorsBuffer.cpp
	Graphics/DescriptorsBuffer.hpp
	Graphics/Subpass.cpp
	Graphics/Subpass.hpp
	Graphics/SubpassLayout.cpp
	Graphics/SubpassLayout.hpp
	Graphics/RenderPass.cpp
	Graphics/RenderPass.hpp
	Graphics/RenderTarget.cpp 
	Graphics/RenderTarget.hpp
	Graphics/SubpassConfiguration.cpp
	Graphics/SubpassConfiguration.hpp

	Graphics/Uniforms/BaseUniform.hpp
	Graphics/Uniforms/SamplerUniform.hpp
	Graphics/Uniforms/SamplerUniform.cpp
	Graphics/Uniforms/BufferUniform.cpp
	Graphics/Uniforms/BufferUniform.hpp

	Resources/Transferer.hpp
	Resources/Transferer.cpp

	Utils/CastHelper.hpp
	Utils/DescriptorSetLayoutBuilder.cpp
	Utils/DescriptorSetLayoutBuilder.hpp
	Utils/FramebufferBuilder.cpp
	Utils/FramebufferBuilder.hpp
	Utils/PipelineBuilder.cpp 
	Utils/PipelineBuilder.hpp
	Utils/PipelineLayoutBuilder.cpp
	Utils/PipelineLayoutBuilder.hpp
	Utils/RenderPassBuilder.cpp
	Utils/RenderPassBuilder.hpp
	Utils/ShaderCompiler.hpp

	
	Images/ImageInfo.hpp
	Images/ImageInfo.cpp
	Images/ImageBase.cpp
	Images/ImageBase.hpp
	Images/InternalImageTraits.hpp
	Images/ImageFormatsConversation.cpp
	Images/ImageFormatsConversation.hpp
	Images/ImageGPU.cpp
	Images/ImageGPU.hpp
	Images/ImageView.cpp
	Images/ImageView.hpp
	Images/NonOwningImageGPU.cpp
	Images/NonOwningImageGPU.hpp

	Resources/BufferGPU.cpp
	Resources/BufferGPU.hpp
	

	CommandsExecution/CommandBuffer.cpp 
	CommandsExecution/CommandBuffer.hpp 
	CommandsExecution/Submitter.cpp
	CommandsExecution/Submitter.hpp
	CommandsExecution/AsyncTask.cpp
	CommandsExecution/AsyncTask.hpp
	CommandsExecution/CompositeAsyncTask.cpp
	CommandsExecution/CompositeAsyncTask.hpp
)

#---------------- Link third-party libraries ---------------
find_package(Vulkan REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

add_subdirectory(ThirdParty/VulkanMemoryAllocator)
add_subdirectory(ThirdParty/VkBootstrap)
#find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
PRIVATE
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)