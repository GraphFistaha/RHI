# CMake generated Testfile for 
# Source directory: C:/Users/rozhkov/Desktop/RHI/Source/Tests
# Build directory: C:/Users/rozhkov/Desktop/RHI/CMakeCache/Source/Tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(RHI_Tests "C:/Users/rozhkov/Desktop/RHI/CMakeCache/Source/Tests/Debug/RHI_Tests.exe")
  set_tests_properties(RHI_Tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;20;add_test;C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(RHI_Tests "C:/Users/rozhkov/Desktop/RHI/CMakeCache/Source/Tests/Release/RHI_Tests.exe")
  set_tests_properties(RHI_Tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;20;add_test;C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(RHI_Tests "C:/Users/rozhkov/Desktop/RHI/CMakeCache/Source/Tests/MinSizeRel/RHI_Tests.exe")
  set_tests_properties(RHI_Tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;20;add_test;C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(RHI_Tests "C:/Users/rozhkov/Desktop/RHI/CMakeCache/Source/Tests/RelWithDebInfo/RHI_Tests.exe")
  set_tests_properties(RHI_Tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;20;add_test;C:/Users/rozhkov/Desktop/RHI/Source/Tests/CMakeLists.txt;0;")
else()
  add_test(RHI_Tests NOT_AVAILABLE)
endif()
