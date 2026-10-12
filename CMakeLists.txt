cmake_minimum_required(VERSION 3.25)
project(user_directory LANGUAGES CXX)

option(START_DATABASE "Start PostgreSQL with Docker Compose before building" ON)

if(START_DATABASE)
    function(find_docker_cli)
        # On macOS, find_program otherwise prefers the Docker.app bundle.
        set(CMAKE_FIND_APPBUNDLE NEVER)
        find_program(DOCKER_EXECUTABLE NAMES docker REQUIRED)
    endfunction()
    find_docker_cli()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.env")
        message(FATAL_ERROR "Create .env from .env.example before building")
    endif()

    add_custom_target(database_up ALL
        COMMAND "${DOCKER_EXECUTABLE}" compose
            --project-directory "${CMAKE_CURRENT_SOURCE_DIR}"
            --env-file "${CMAKE_CURRENT_SOURCE_DIR}/.env"
            --file "${CMAKE_CURRENT_SOURCE_DIR}/compose.yaml"
            up --detach --wait --wait-timeout 90 db
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Starting PostgreSQL and waiting for its health check"
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(database_stop
        COMMAND "${DOCKER_EXECUTABLE}" compose
            --project-directory "${CMAKE_CURRENT_SOURCE_DIR}"
            --env-file "${CMAKE_CURRENT_SOURCE_DIR}/.env"
            --file "${CMAKE_CURRENT_SOURCE_DIR}/compose.yaml"
            stop db
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Stopping PostgreSQL without removing its data volume"
        USES_TERMINAL
        VERBATIM
    )
endif()

add_subdirectory(server)
add_subdirectory(client)

if(START_DATABASE)
    foreach(application IN ITEMS server client)
        add_dependencies(${application} database_up)
        # Qt code generation must also wait for PostgreSQL in parallel builds.
        set_property(TARGET ${application} APPEND PROPERTY
            AUTOGEN_TARGET_DEPENDS database_up)
    endforeach()
endif()
