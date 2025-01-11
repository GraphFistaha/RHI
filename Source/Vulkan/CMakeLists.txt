target_sources (${PROJECT_NAME}
PRIVATE
	VulkanContext.cpp
	VulkanContext.hpp
	GarbageCollector.cpp
	GarbageCollector.hpp
	BuffersAllocator.hpp
	BuffersAllocator.cpp

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
	Graphics/Pipeline.cpp 
	Graphics/Pipeline.hpp
	Graphics/FramebufferAttachment.cpp
	Graphics/FramebufferAttachment.hpp

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

	
	Images/ImageTraits.hpp
	Images/ImageFormatsConversation.cpp
	Images/ImageFormatsConversation.hpp
	Images/ImageGPU.cpp
	Images/ImageGPU.hpp
	Images/ImageView.cpp
	Images/ImageView.hpp

	Resources/BufferGPU.cpp
	Resources/BufferGPU.hpp
	Resources/BufferBase.hpp
	Resources/BufferBase.cpp

	CommandsExecution/CommandBuffer.cpp 
	CommandsExecution/CommandBuffer.hpp 
	CommandsExecution/Submitter.cpp
	CommandsExecution/Submitter.hpp
 )

#---------------- Link third-party libraries ---------------
find_package(Vulkan REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
PRIVATE
	$<$<CONFIG:Debug>:ENABLE_VALIDATION_LAYERS>
)

message(STATUS "Fetch vkBootstrap")
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        c710f245ffd55a49ac698f3dbf41aababfd51f12 # 16.09.2024 update
)
FetchContent_MakeAvailable(vk_bootstrap)

add_subdirectory(ThirdParty/VulkanMemoryAllocator)
#find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
PRIVATE
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)