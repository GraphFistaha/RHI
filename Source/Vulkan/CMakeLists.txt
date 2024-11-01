target_sources (${PROJECT_NAME}
PRIVATE
	"VulkanContext.cpp"
	"Swapchain.cpp"
	"Framebuffer.cpp" 
	"Pipeline.cpp" 
	"Utils/PipelineBuilder.cpp" 
	"Utils/FramebufferBuilder.cpp"
	"CommandBuffer.cpp" 
	"BufferGPU.cpp"
	"DescriptorsBuffer.cpp"
	 "ImageGPU.cpp"
	 
PRIVATE
	"VulkanContext.hpp"
	"Swapchain.hpp"
	"Framebuffer.hpp"
	"Pipeline.hpp"
	"Utils/Builders.hpp"
	"CommandBuffer.hpp"
	"BufferGPU.hpp"
	"DescriptorsBuffer.hpp"
	"ImageGPU.hpp"

	"GraphicsExecutor.hpp"
 "GraphicsExecutor.cpp")

#---------------- Link third-party libraries ---------------
find_package(Vulkan REQUIRED)

message(STATUS "Fetch vkBootstrap")
FetchContent_Declare(
    vk_bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
    GIT_TAG        c710f245ffd55a49ac698f3dbf41aababfd51f12 # 16.09.2024 update
)
FetchContent_MakeAvailable(vk_bootstrap)

message(STATUS "Fetch VulkanMemoryAllocator")
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        1c35ba99ce775f8342d87a83a3f0f696f99c2a39 # 01.10.2024
	GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)
#find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
PRIVATE
	Vulkan::Vulkan
	vk-bootstrap::vk-bootstrap
	GPUOpen::VulkanMemoryAllocator
)