cmake_minimum_required(VERSION 3.20)

project(VRP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VRP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(VRP_FETCH_RAPIDJSON "Fetch RapidJSON automatically if not installed" ON)

set(VRP_SOURCES
    main.cpp
    actor/Controller.cpp
    actor/Customer.cpp
    actor/Route.cpp
    actor/TabuList.cpp
    actor/TabuSearch.cpp
    lib/Edge.cpp
    lib/Graph.cpp
    lib/OptimalMove.cpp
    lib/Utils.cpp
    lib/VRP.cpp
    lib/Vertex.cpp
)

add_executable(VRP ${VRP_SOURCES})

target_include_directories(VRP
    PRIVATE
        actor
        lib
)

find_package(Threads REQUIRED)
target_link_libraries(VRP PRIVATE Threads::Threads)

find_path(RAPIDJSON_INCLUDE_DIR rapidjson/document.h)

if(NOT RAPIDJSON_INCLUDE_DIR AND VRP_FETCH_RAPIDJSON)
    include(FetchContent)
    FetchContent_Declare(
        rapidjson
        # Latest upstream commit on master as of 2026-05-31.
        URL https://github.com/Tencent/rapidjson/archive/24b5e7a8b27f42fa16b96fc70aade9106cf7102f.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_GetProperties(rapidjson)
    if(NOT rapidjson_POPULATED)
        if(POLICY CMP0169)
            cmake_policy(PUSH)
            cmake_policy(SET CMP0169 OLD)
            FetchContent_Populate(rapidjson)
            cmake_policy(POP)
        else()
            FetchContent_Populate(rapidjson)
        endif()
    endif()
    set(RAPIDJSON_INCLUDE_DIR "${rapidjson_SOURCE_DIR}/include")
endif()

if(NOT RAPIDJSON_INCLUDE_DIR)
    message(FATAL_ERROR "RapidJSON headers not found. Install rapidjson-dev or enable VRP_FETCH_RAPIDJSON.")
endif()

target_include_directories(VRP SYSTEM PRIVATE "${RAPIDJSON_INCLUDE_DIR}")

if(MSVC)
    target_compile_options(VRP PRIVATE /W4 /permissive-)
    if(VRP_WARNINGS_AS_ERRORS)
        target_compile_options(VRP PRIVATE /WX)
    endif()
else()
    target_compile_options(VRP PRIVATE -Wall -Wextra -Wpedantic)
    if(VRP_WARNINGS_AS_ERRORS)
        target_compile_options(VRP PRIVATE -Werror)
    endif()
endif()

install(TARGETS VRP RUNTIME DESTINATION bin)
