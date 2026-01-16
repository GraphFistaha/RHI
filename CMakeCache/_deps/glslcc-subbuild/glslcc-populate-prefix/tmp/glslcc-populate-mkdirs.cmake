# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-src")
  file(MAKE_DIRECTORY "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-build"
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix"
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/tmp"
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/src/glslcc-populate-stamp"
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/src"
  "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/src/glslcc-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/src/glslcc-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/rozhkov/Desktop/RHI/CMakeCache/_deps/glslcc-subbuild/glslcc-populate-prefix/src/glslcc-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
