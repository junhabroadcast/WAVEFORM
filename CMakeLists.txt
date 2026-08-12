cmake_minimum_required(VERSION 3.21)
project(WfmMonitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

set(DECKLINK_SDK_DIR "${CMAKE_SOURCE_DIR}/third_party/decklink" CACHE PATH "DeckLink SDK include root")

add_executable(WfmMonitor
    src/main.cpp
    src/capture/DeckLinkCapture.cpp
    src/capture/DeckLinkCapture.h
    src/capture/FrameQueue.cpp
    src/capture/FrameQueue.h
    src/video/V210Unpack.cpp
    src/video/V210Unpack.h
    src/video/VideoFrame.h
    src/color/ColorMatrix.cpp
    src/color/ColorMatrix.h
    src/display/DisplayModes.h
    src/display/Graticule.cpp
    src/display/Graticule.h
    src/render/WfmGlWidget.cpp
    src/render/WfmGlWidget.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/TileControls.cpp
    src/ui/TileControls.h
    resources/shaders.qrc
)

target_include_directories(WfmMonitor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${DECKLINK_SDK_DIR}
)

target_link_libraries(WfmMonitor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ole32
    oleaut32
    strmiids
)

target_compile_definitions(WfmMonitor PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(WfmMonitor PRIVATE /W4 /permissive-)
endif()

install(TARGETS WfmMonitor RUNTIME DESTINATION bin)
