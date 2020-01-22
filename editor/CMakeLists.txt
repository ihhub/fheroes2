cmake_minimum_required(VERSION 3.1.0)

project(fh2editor)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets Xml REQUIRED)

add_executable(fh2editor
    dialogs.cpp
    engine.cpp
    global.cpp
    mainwindow.cpp
    mapdata.cpp
    mapwindow.cpp
    program.cpp
    resources.qrc
)

target_link_libraries(fh2editor Qt5::Widgets Qt5::Xml)
