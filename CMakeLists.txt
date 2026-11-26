cmake_minimum_required(VERSION 3.20)

project(Enigma VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENIGMA_ENABLE_SANITIZERS "Build with AddressSanitizer and UBSan" OFF)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# CMake 4 removed compatibility modes requested by Raylib 5.0 and Catch2 3.5.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/raylib/CMakeLists.txt")
  add_subdirectory(libs/raylib EXCLUDE_FROM_ALL)
else()
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
  )
  FetchContent_MakeAvailable(raylib)
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

add_library(EnigmaCore
  src/Charactermap.cpp
  src/Rotor.cpp
  src/Reflector.cpp
  src/Plugboard.cpp
  src/EnigmaMachine.cpp
)
target_include_directories(EnigmaCore PUBLIC include)
target_compile_features(EnigmaCore PUBLIC cxx_std_20)
enigma_set_warnings(EnigmaCore)
enigma_enable_sanitizers(EnigmaCore)

add_executable(Enigma
  src/main.cpp
  src/workbench/Session.cpp
  src/workbench/Layout.cpp
  src/workbench/Input.cpp
  src/workbench/View.cpp
)
target_link_libraries(Enigma PRIVATE EnigmaCore raylib)
enigma_set_warnings(Enigma)
enigma_enable_sanitizers(Enigma)

set(ENIGMA_FONT_ASSETS
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/Montserrat-Medium.ttf"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/Montserrat-OFL.txt"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-Medium.ttf"
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/JetBrainsMono-OFL.txt"
)
add_custom_command(TARGET Enigma POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Enigma>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${ENIGMA_FONT_ASSETS} "$<TARGET_FILE_DIR:Enigma>/assets"
  VERBATIM
)

include(CTest)
if(BUILD_TESTING)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2/CMakeLists.txt")
    set(ENIGMA_CATCH2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2")
    add_subdirectory(libs/Catch2 EXCLUDE_FROM_ALL)
  else()
    FetchContent_Declare(
      Catch2
      URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
    )
    FetchContent_MakeAvailable(Catch2)
    set(ENIGMA_CATCH2_SOURCE_DIR "${catch2_SOURCE_DIR}")
  endif()

  add_executable(EnigmaTests test/test_main.cpp src/workbench/Session.cpp)
  target_include_directories(EnigmaTests PRIVATE src)
  target_link_libraries(EnigmaTests PRIVATE EnigmaCore Catch2::Catch2WithMain)
  enigma_set_warnings(EnigmaTests)
  enigma_enable_sanitizers(EnigmaTests)

  include(${ENIGMA_CATCH2_SOURCE_DIR}/extras/Catch.cmake)
  catch_discover_tests(EnigmaTests)
endif()
