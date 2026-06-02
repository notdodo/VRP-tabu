cmake_minimum_required(VERSION 3.20)

project(VRP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(VRP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(VRP_FETCH_NLOHMANN_JSON "Fetch nlohmann/json automatically if not installed" ON)
option(VRP_ENABLE_LTO "Enable interprocedural optimization/link-time optimization" OFF)
option(VRP_NATIVE_OPTIMIZATIONS "Enable native CPU tuning for local benchmarks" OFF)
option(VRP_ENABLE_PROFILING "Build with profiler-friendly symbols and frame pointers" OFF)

set(NLOHMANN_JSON_VERSION 3.12.0)

set(VRP_SOURCES
    main.cpp
    actor/Controller.cpp
    actor/Customer.cpp
    actor/Route.cpp
    actor/TabuList.cpp
    actor/TabuSearch.cpp
    lib/Graph.cpp
    lib/OptimalMove.cpp
    lib/Utils.cpp
    lib/VRP.cpp
)

add_executable(VRP ${VRP_SOURCES})

target_include_directories(VRP
    PRIVATE
        actor
        lib
)

find_package(Threads REQUIRED)
target_link_libraries(VRP PRIVATE Threads::Threads)

find_package(nlohmann_json ${NLOHMANN_JSON_VERSION} CONFIG QUIET)

if(NOT nlohmann_json_FOUND AND VRP_FETCH_NLOHMANN_JSON)
    include(FetchContent)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.tar.xz
        URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    message(FATAL_ERROR "nlohmann/json not found. Install nlohmann-json or enable VRP_FETCH_NLOHMANN_JSON.")
endif()

target_link_libraries(VRP PRIVATE nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(VRP PRIVATE /W4 /permissive- $<$<CONFIG:Release>:/O2>)
    if(VRP_ENABLE_PROFILING)
        target_compile_options(VRP PRIVATE /Zi)
        target_link_options(VRP PRIVATE /DEBUG)
    endif()
    if(VRP_WARNINGS_AS_ERRORS)
        target_compile_options(VRP PRIVATE /WX)
    endif()
else()
    target_compile_options(VRP PRIVATE -Wall -Wextra -Wpedantic $<$<CONFIG:Release>:-O3>)
    if(VRP_NATIVE_OPTIMIZATIONS)
        target_compile_options(VRP PRIVATE -march=native)
    endif()
    if(VRP_ENABLE_PROFILING)
        target_compile_options(VRP PRIVATE -g -fno-omit-frame-pointer)
    endif()
    if(VRP_WARNINGS_AS_ERRORS)
        target_compile_options(VRP PRIVATE -Werror)
    endif()
endif()

if(VRP_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(TARGET VRP PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(WARNING "LTO requested but not supported: ${ipo_error}")
    endif()
endif()

install(TARGETS VRP RUNTIME DESTINATION bin)
