cmake_minimum_required(VERSION 3.22)

project(Enigma VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENIGMA_ENABLE_SANITIZERS "Build with AddressSanitizer and UBSan" OFF)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

function(enigma_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
  endif()
endfunction()

function(enigma_enable_sanitizers target)
  if(ENIGMA_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -fno-omit-frame-pointer
                                             -fsanitize=address,undefined)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

set(ENIGMA_CORE_SOURCES
  src/Charactermap.cpp
  src/Rotor.cpp
  src/Reflector.cpp
  src/Plugboard.cpp
  src/EnigmaMachine.cpp
  src/EnigmaSettings.cpp
)

add_library(EnigmaCore ${ENIGMA_CORE_SOURCES})
target_include_directories(EnigmaCore PUBLIC include)
target_compile_features(EnigmaCore PUBLIC cxx_std_20)
enigma_set_warnings(EnigmaCore)
enigma_enable_sanitizers(EnigmaCore)

option(ENIGMA_BUILD_WORKBENCH "Build Enigma Workbench GUI application" ON)

set(ENIGMA_WORKBENCH_AVAILABLE FALSE)
if(ENIGMA_BUILD_WORKBENCH)
  find_package(Vulkan)
  find_package(glfw3 CONFIG)
  if(NOT TARGET glfw)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
      pkg_check_modules(GLFW IMPORTED_TARGET glfw3)
      if(TARGET PkgConfig::GLFW)
        add_library(glfw ALIAS PkgConfig::GLFW)
      endif()
    endif()
  endif()
  find_package(Freetype)
  find_package(JPEG)

  if(Vulkan_FOUND AND (TARGET glfw OR TARGET glfw3) AND Freetype_FOUND AND JPEG_FOUND)
    set(ENIGMA_WORKBENCH_AVAILABLE TRUE)
  else()
    message(STATUS "Graphics packages not found. Building EnigmaCore only.")
  endif()
endif()

if(ENIGMA_WORKBENCH_AVAILABLE)
  if(TARGET Vulkan::glslc)
    set(GLSLC_EXECUTABLE Vulkan::glslc)
  elseif(Vulkan_GLSLC_EXECUTABLE)
    set(GLSLC_EXECUTABLE "${Vulkan_GLSLC_EXECUTABLE}")
  else()
    find_program(GLSLC_EXECUTABLE glslc REQUIRED)
  endif()

  set(SHADER_SOURCES
    src/workbench/shaders/solid.vert
    src/workbench/shaders/solid.frag
    src/workbench/shaders/sampled.vert
    src/workbench/shaders/sampled.frag
  )

  set(SPV_OUTPUTS "")
  foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPV_FILE}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
      COMMAND ${GLSLC_EXECUTABLE} --target-env=vulkan1.3 ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPV_FILE}
      DEPENDS ${SHADER}
      VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${SPV_FILE})
  endforeach()

  add_custom_target(EnigmaShaders ALL DEPENDS ${SPV_OUTPUTS})

  set(ENIGMA_SOURCES
    src/main.cpp
    src/workbench/Session.cpp
    src/workbench/Layout.cpp
    src/workbench/Input.cpp
    src/workbench/View.cpp
    src/workbench/FontAtlas.cpp
    src/workbench/ImageDecoder.cpp
    src/workbench/DrawCommand.cpp
    src/workbench/VulkanRenderer.cpp
  )

  add_executable(Enigma ${ENIGMA_SOURCES})
  target_include_directories(Enigma PRIVATE src ${FREETYPE_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
  target_link_libraries(Enigma PRIVATE EnigmaCore Vulkan::Vulkan glfw Freetype::Freetype JPEG::JPEG)
  enigma_set_warnings(Enigma)
  enigma_enable_sanitizers(Enigma)
  add_dependencies(Enigma EnigmaShaders)

  set(ENIGMA_ASSETS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/Enigma-Imperial-War-Museum.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/ATTRIBUTION.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/InstrumentSerif-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/InstrumentSerif-OFL.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/Montserrat-Medium.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/Montserrat-OFL.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-Medium.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-OFL.txt"
  )
  add_custom_target(EnigmaAssets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Enigma>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ENIGMA_ASSETS} "$<TARGET_FILE_DIR:Enigma>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Enigma>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SPV_OUTPUTS} "$<TARGET_FILE_DIR:Enigma>/shaders"
    DEPENDS ${ENIGMA_ASSETS} ${SPV_OUTPUTS}
    VERBATIM
  )
  add_dependencies(Enigma EnigmaAssets)
endif()

include(CTest)
if(BUILD_TESTING)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2/CMakeLists.txt")
    set(ENIGMA_CATCH2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2")
    add_subdirectory(libs/Catch2 EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(
      Catch2
      URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
      URL_HASH SHA256=269543a49eb76f40b3f93ff231d4c24c27a7e16c90e47d2e45bcc564de470c6e
    )
    FetchContent_MakeAvailable(Catch2)
    set(ENIGMA_CATCH2_SOURCE_DIR "${catch2_SOURCE_DIR}")
  endif()

  if(ENIGMA_WORKBENCH_AVAILABLE)
    add_executable(EnigmaTests
      test/test_main.cpp
      src/workbench/Session.cpp
      src/workbench/Layout.cpp
      src/workbench/Input.cpp
      src/workbench/View.cpp
      src/workbench/FontAtlas.cpp
      src/workbench/ImageDecoder.cpp
      src/workbench/DrawCommand.cpp
      src/workbench/VulkanRenderer.cpp
    )
    target_include_directories(EnigmaTests PRIVATE src ${FREETYPE_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})
    target_compile_definitions(EnigmaTests PRIVATE CATCH_CONFIG_NO_COUNTER)
    target_link_libraries(EnigmaTests PRIVATE EnigmaCore Catch2::Catch2WithMain Vulkan::Vulkan glfw Freetype::Freetype JPEG::JPEG)
    enigma_set_warnings(EnigmaTests)
    enigma_enable_sanitizers(EnigmaTests)
    add_dependencies(EnigmaTests EnigmaAssets EnigmaShaders)
  else()
    add_executable(EnigmaTests
      test/test_main.cpp
      src/workbench/Session.cpp
    )
    target_include_directories(EnigmaTests PRIVATE src)
    target_compile_definitions(EnigmaTests PRIVATE CATCH_CONFIG_NO_COUNTER ENIGMA_CORE_ONLY=1)
    target_link_libraries(EnigmaTests PRIVATE EnigmaCore Catch2::Catch2WithMain)
    enigma_set_warnings(EnigmaTests)
    enigma_enable_sanitizers(EnigmaTests)
  endif()

  include(${ENIGMA_CATCH2_SOURCE_DIR}/extras/Catch.cmake)
  catch_discover_tests(EnigmaTests)
endif()

include(cmake/Bombe.cmake)

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen and Mermaid diagrams"
    VERBATIM
  )
endif()
