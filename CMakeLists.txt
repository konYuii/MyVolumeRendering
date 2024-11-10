cmake_minimum_required(VERSION 3.0.0)
project(project VERSION 0.1.0)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
add_executable(project 
src/main.cpp 
imgui/imgui.cpp
imgui/imgui_widgets.cpp
imgui/imgui_tables.cpp
imgui/imgui_impl_opengl3.cpp
imgui/imgui_impl_glfw.cpp
imgui/imgui_draw.cpp
imgui/imgui_demo.cpp
)
target_link_libraries(project  GL glfw GLEW)
 