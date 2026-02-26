cmake_minimum_required(VERSION 3.10)
project(RetroReplayV2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Add GLFW
add_subdirectory(third_party/glfw)

# 2. Setup Dear ImGui
set(IMGUI_DIR third_party/imgui)
add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(ImGui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
# FIX: Tell the ImGui backend where GLFW is located
target_link_libraries(ImGui PRIVATE glfw) 

# 3. Find OpenGL
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------
# NEW: NETWORK LIBRARIES
# ---------------------------------------------------------
# Enable the FetchContent module
include(FetchContent)

# Download nlohmann/json (For parsing the API responses)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Download CPR - C++ Requests (For sending web requests)
FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE) # Forces CPR to build its own curl
FetchContent_MakeAvailable(cpr)
# ---------------------------------------------------------

# 4. Your Executable
add_executable(RetroReplayV2 src/main.cpp)

# 5. Bulletproof the include paths for the main executable
target_include_directories(RetroReplayV2 PRIVATE 
    third_party/stb
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    third_party/glfw/include
)

# 6. Link everything together (NEW LIBRARIES ADDED HERE)
target_link_libraries(RetroReplayV2 PRIVATE 
    ImGui 
    glfw 
    OpenGL::GL 
    cpr::cpr 
    nlohmann_json::nlohmann_json
)

# 7. Automatically copy the assets folder to the build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 8. Copy the secrets file to the build directory so the .exe can find it
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/secrets.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})