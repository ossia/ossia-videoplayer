cmake_minimum_required(VERSION 3.12)
project(ossia-videoplayer VERSION 1.0 LANGUAGES CXX)

find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(ossia REQUIRED)

add_executable(ossia-videoplayer)

target_sources(ossia-videoplayer PRIVATE
    src/main.cpp
    src/Viewport.h
    src/Viewport.cpp
)

target_compile_features(ossia-videoplayer PRIVATE 
    cxx_std_17
)

target_link_libraries(ossia-videoplayer PRIVATE
    Qt5::Gui
    mpv
    ossia::ossia
    GL
)
