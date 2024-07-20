cmake_minimum_required(VERSION 3.5)

project(MyApp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)  # This enables automatic handling of .ui files
set(CMAKE_AUTORCC ON)  # This enables automatic handling of resource files

find_package(Qt5Widgets REQUIRED)

add_executable(MyApp main.cpp mainwindow.cpp resources.qrc
        AVQtWidgets/qrotarymenuentry.cpp
        AVQtWidgets/qrotarymenuentry.h
        AVQtWidgets/qrotarymenu.cpp
        AVQtWidgets/qrotarymenu.h
        AVQtWidgets/RotaryMenuEntries/qtestmenuentry.cpp
        AVQtWidgets/RotaryMenuEntries/qclockmenuentry.cpp
        AVQtWidgets/RotaryMenuEntries/qclockmenuentry.h
        AVQtWidgets/clock.cpp
        AVQtWidgets/clock.cpp
)

target_link_libraries(MyApp Qt5::Widgets)
