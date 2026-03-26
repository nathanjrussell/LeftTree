cmake_minimum_required(VERSION 3.20)

project(LeftTree
  VERSION 0.1.0
  DESCRIPTION "Binary Tree structure that grows toward the left and recursively returns upward"
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(FetchContent)

FetchContent_Declare(
  CPPTypeConcepts
  GIT_REPOSITORY https://github.com/nathanjrussell/CPP-Type-Concepts.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(CPPTypeConcepts)

option(LEFTTREE_BUILD_TESTS "Build LeftTree unit tests" ON)
option(LEFTTREE_BUILD_EXAMPLES "Build LeftTree examples" ON)

# Header-only template library
add_library(LeftTree INTERFACE)
add_library(LeftTree::LeftTree ALIAS LeftTree)

target_include_directories(LeftTree
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(LeftTree INTERFACE cxx_std_20)

# Concepts are a public dependency for the optional serialization helper header.
# This keeps includes working for downstream consumers.
target_link_libraries(LeftTree INTERFACE CPPTypeConcepts::cpp_type_concepts)

set_target_properties(LeftTree PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME LeftTree
)

# Install headers + library
install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  TARGETS LeftTree
  EXPORT LeftTreeTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Export targets
install(
  EXPORT LeftTreeTargets
  NAMESPACE LeftTree::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LeftTree
)

# Package config files
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/LeftTreeConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/LeftTreeConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LeftTree
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/LeftTreeConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LeftTreeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LeftTreeConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LeftTree
)

# Testing (gtest)
if(LEFTTREE_BUILD_TESTS)
  include(CTest)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(LeftTree_tests
    testing/LeftTree_test.cpp
  )
  target_link_libraries(LeftTree_tests PRIVATE LeftTree::LeftTree GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(LeftTree_tests)
endif()

# Examples
if(LEFTTREE_BUILD_EXAMPLES)
  add_executable(LeftTree_example
    examples/basic.cpp
  )
  target_link_libraries(LeftTree_example PRIVATE LeftTree::LeftTree)
endif()
