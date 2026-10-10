cmake_minimum_required(VERSION 3.20)
project(KdeShortcutPopup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Network)
find_package(LayerShellQt REQUIRED)

add_executable(kde-shortcut-popup
    src/main.cpp
    src/shortcut_entry.h
    src/shortcut_manager.h
    src/shortcut_manager.cpp
    src/shortcut_delegate.h
    src/shortcut_delegate.cpp
    src/popup_window.h
    src/popup_window.cpp
)

target_link_libraries(kde-shortcut-popup PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    Qt6::Network
    LayerShellQt::Interface
)

install(TARGETS kde-shortcut-popup DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES kde-shortcut-popup.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
