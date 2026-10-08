cmake_minimum_required(VERSION 3.19)
project(server LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(server
    main.cpp
    tcpserver.h tcpserver.cpp
)

target_link_libraries(server
    PRIVATE
        Qt::Core
        Qt6::Network
)

install(TARGETS server
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
