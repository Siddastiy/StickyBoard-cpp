cmake_minimum_required(VERSION 3.21)
project(StickBoard)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLAD CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ImGui CONFIG REQUIRED)

set(DEPS glfw glad::glad OpenGL::GL imgui::imgui)

add_executable(StickBoard WIN32 main.cpp)
target_link_libraries(StickBoard PRIVATE ${DEPS})
target_include_directories(StickBoard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(TARGET StickBoard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/fonts
        $<TARGET_FILE_DIR:StickBoard>/fonts
)