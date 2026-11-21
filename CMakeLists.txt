cmake_minimum_required(VERSION 3.20)
project(microsip-qt VERSION 3.23.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia)

pkg_check_modules(PJSIP REQUIRED IMPORTED_TARGET libpjproject)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

set(CORE_SOURCES
    linux/src/core/Types.h
    linux/src/core/Settings.h
    linux/src/core/Settings.cpp
    linux/src/core/SingleInstance.h
    linux/src/core/SingleInstance.cpp
    linux/src/core/Database.h
    linux/src/core/Database.cpp
    linux/src/core/SipEngine.h
    linux/src/core/SipEngine.cpp
)

set(UI_SOURCES
    linux/src/ui/Theme.h
    linux/src/ui/MainWindow.h
    linux/src/ui/MainWindow.cpp
    linux/src/ui/DialerWidget.h
    linux/src/ui/DialerWidget.cpp
    linux/src/ui/CallsWidget.h
    linux/src/ui/CallsWidget.cpp
    linux/src/ui/ContactsWidget.h
    linux/src/ui/ContactsWidget.cpp
    linux/src/ui/MessagesWidget.h
    linux/src/ui/MessagesWidget.cpp
    linux/src/ui/IncomingCallDialog.h
    linux/src/ui/IncomingCallDialog.cpp
    linux/src/ui/AccountDialog.h
    linux/src/ui/AccountDialog.cpp
    linux/src/ui/SettingsDialog.h
    linux/src/ui/SettingsDialog.cpp
    linux/src/ui/ShortcutsDialog.h
    linux/src/ui/ShortcutsDialog.cpp
    linux/src/ui/TransferDialog.h
    linux/src/ui/TransferDialog.cpp
)

add_executable(microsip-qt
    linux/src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    linux/resources/resources.qrc
)

target_include_directories(microsip-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/linux/src
)

target_link_libraries(microsip-qt PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    PkgConfig::PJSIP
    PkgConfig::SQLITE3
)

install(TARGETS microsip-qt
    RUNTIME DESTINATION bin
)

install(FILES linux/resources/microsip.desktop
    DESTINATION share/applications
)

install(FILES linux/resources/microsip.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME microsip-qt.svg
)
