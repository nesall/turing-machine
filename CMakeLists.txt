# =============================================================================
#  1. Project Configuration
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(TuringMachineGUI LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
#  2. Fetch Dependencies
# =============================================================================
include(FetchContent)

# --- Fetch ImGui (GUI Library) ---
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.8
)

# --- Fetch GLFW (Windowing Library) ---
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)

# --- Fetch nlohmann/json (JSON Library) ---
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)

FetchContent_MakeAvailable(imgui glfw nlohmann_json)

# =============================================================================
#  3. Create ImGui Library Target
# =============================================================================
# ImGui doesn't provide a CMake target, so create one
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw)

# =============================================================================
#  4. Executable & Sources
# =============================================================================
add_executable(TuringMachineGUI
  main.cpp
  model/turingmachine.cpp
  model/turingmachine.hpp
  ui/render.cpp
  ui/render.hpp
  ui/manipulators.hpp
  ui/manipulators.cpp
  ui/fa_icons.hpp
  ui/drawobject.hpp
  ui/drawobject.cpp
  app.hpp
)

# =============================================================================
#  5. Includes & Linking
# =============================================================================
target_include_directories(TuringMachineGUI PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(OpenGL REQUIRED)

target_link_libraries(TuringMachineGUI PRIVATE
  imgui
  glfw
  OpenGL::GL
  nlohmann_json::nlohmann_json
)
