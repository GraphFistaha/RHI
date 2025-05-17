add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)
add_subdirectory(ThirdParty/test_utils)

add_custom_target("ExampleShaders")

add_subdirectory(HelloWindow)
add_subdirectory(HelloTriangle)
add_subdirectory(Uniforms)
add_subdirectory(ParallelPass)
add_subdirectory(Texturing)
add_subdirectory(DownloadImages)
add_subdirectory(TextureArray)
#add_subdirectory(OffscreenQuad)
add_subdirectory(DepthStencilTest)


# hard examples
add_subdirectory(Cube)