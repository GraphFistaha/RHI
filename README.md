# RHI
Graphic library provides generic independent API for rendering.
Provided backends:
- Vulkan (in progress)
- DirectX (in plan)
- Metal (in plan)

My goal - is use modern low-level graphic API, but create a really simple API saving all features like multithreading  
For learning this API, you'd better to watch on examples. Below is a list of examples.  

Each example is a little test application which is tests some functional of API like uniforms, rendering geometry, multithreading and so on.  
Watch examples in order of this list, 
- HelloWindow - start example which should only open window and clear it with changing color
- HelloTriangle - basic graphic application - opens the window, clear color and draws a colored triangle
- Uniforms - you'd learn how to setup uniform variables (except Sampler2D)
- ParallelPass - very important example because it shows how to build an application to provide multithreaded rendering.
- Textures - a basic example that shows how to work with Sampler2D uniform
- DownloadImages - shows how to download image from GPU.
- TexturesArray - shows how to use arrays of uniforms: f.e uniform sampler2D var[5]. For buffered uniforms is similar.
- LayeredTexture - shows how to implement texture array properly (like Sampler2DArray). Also shows how to generate mipmaps.
- MultiWindow - example shows how to render into two windows. It's optional example

## How to integrate and build
This project uses CMake as build system, so you can just download this project into folder and include it in your CMakeLists via `add_subdirectory`
You can link it to another project via `target_link_libraries(${target_name} PRIVATE RHI)`
Also you can define some options:
- RHI_VULKAN_BACKEND - RHI will be compiled with Vulkan backend.
- RHI_DIRECTX_BACKEND - RHI will be compiled with DirectX backend.
- RHI_METAL_BACKEND -RHI will be compiled with Metal backend
- RHI_BUILD_EXAMPLES - example applications from Example folder will be compiled
### Dependencies
The library provides independent API, so you don't have to link any other libraries. Backends have their own dependencies which are intergrated with FetchContent

## How to use
There is only one header file you need to include in code for using - RHI.hpp. It contains all interface declaration you can use to write code.


