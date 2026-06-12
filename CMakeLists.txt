cmake_minimum_required(VERSION 3.14)
project(ProjectBackrooms CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch Raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# 2. Fetch ImGui (ImGui does not have a CMakeLists.txt, so we populate it manually)
FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/heads/docking.zip
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# 3. Fetch rlImGui (Bridge between Raylib and ImGui)
FetchContent_Declare(
    rlimhui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
)
FetchContent_GetProperties(rlimhui)
if(NOT rlimhui_POPULATED)
    FetchContent_Populate(rlimhui)
endif()

# 4. Define our executable and include the external source files directly
add_executable(Backrooms 
    src/main.cpp 
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimhui_SOURCE_DIR}/rlImGui.cpp
)

# Tell the compiler where to find the header files (.h files)
target_include_directories(Backrooms PRIVATE
    ${imgui_SOURCE_DIR}
    ${rlimhui_SOURCE_DIR}
)

# Link Raylib to our executable
target_link_libraries(Backrooms PRIVATE raylib)

# Raylib recently renamed IsTextureReady to IsTextureValid. rlImGui expects the new name.
# This define maps it back to the old name so it compiles with Raylib 5.0.
target_compile_definitions(Backrooms PRIVATE IsTextureValid=IsTextureReady)
