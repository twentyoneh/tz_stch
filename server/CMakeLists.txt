cmake_minimum_required(VERSION 3.19)
project(server LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Sql)

qt_standard_project_setup()

qt_add_executable(server
    main.cpp
    clientsession.h clientsession.cpp
    tcpserver.h tcpserver.cpp
    databaseconfig.h
    databaseconnection.h databaseconnection.cpp
    envloader.h envloader.cpp
    user.h
    userrepository.h userrepository.cpp
    requestworker.h requestworker.cpp
)

target_link_libraries(server
    PRIVATE
        Qt::Core
        Qt6::Network
        Qt6::Sql
)

install(TARGETS server
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
