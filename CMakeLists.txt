cmake_minimum_required(VERSION 3.14)

project(server LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

set(LIBRARY_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/lib/httplib.h
    ${CMAKE_CURRENT_LIST_DIR}/lib/handlers.h
    ${CMAKE_CURRENT_LIST_DIR}/lib/handlers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/server.h
    ${CMAKE_CURRENT_LIST_DIR}/lib/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/dbsingleton.h
    ${CMAKE_CURRENT_LIST_DIR}/lib/dbsingleton.cpp
)

add_subdirectory(tests)

add_executable(server
    main.cpp
    ${LIBRARY_SOURCES}
)
target_link_libraries(server Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Sql)

install(TARGETS server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
