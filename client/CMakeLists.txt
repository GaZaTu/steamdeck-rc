cmake_minimum_required(VERSION 3.28.0 FATAL_ERROR)
project(steamdeck-rc-client VERSION 0.0.1 LANGUAGES CXX)

include(FetchContent)

set(PROJECT_ORGANIZATION gazatu.xyz)

add_definitions(
  -DPROJECT_ORGANIZATION="${PROJECT_ORGANIZATION}"
  -DPROJECT_NAME="${PROJECT_NAME}"
  -DPROJECT_VERSION="${PROJECT_VERSION}"
  -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

set(SDL_X11 OFF)
set(SDL_WAYLAND OFF)
set(SDL_VULKAN OFF)
set(SDL_PULSEAUDIO OFF)
set(SDL_PIPEWIRE OFF)
set(SDL_ALSA OFF)
set(SDL_IBUS OFF)
set(SDL_UNIX_CONSOLE_BUILD ON)

FetchContent_Declare(
  SDL
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-3.2.22
)

FetchContent_MakeAvailable(SDL)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/include
)

file(GLOB SRC_FILES
  ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries(${PROJECT_NAME}
  SDL3::SDL3
  mpv
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_custom_target(appimage
  COMMAND make install DESTDIR=${CMAKE_BINARY_DIR}/_AppDir
  COMMAND VERSION=dev linuxdeployqt.AppImage ${CMAKE_BINARY_DIR}/_AppDir/usr/share/applications/${PROJECT_NAME}.desktop -appimage -no-translations
)
