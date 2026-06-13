cmake_minimum_required(VERSION 3.31.6)
project(telematics_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_ASYNC_MQTT "Use async_mqtt backend instead of libmosquitto" OFF)

if(USE_ASYNC_MQTT)
    # GCC 11 has a known ICE (cp/call.c:10200) with C++20 coroutines + complex
    # template instantiations. Require GCC 12+ when building the async_mqtt backend.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12")
        message(FATAL_ERROR
            "USE_ASYNC_MQTT requires GCC 12+ (found ${CMAKE_CXX_COMPILER_VERSION}).\n"
            "Re-run cmake with: -DCMAKE_CXX_COMPILER=g++-12")
    endif()

    include(FetchContent)

    # async_mqtt requires Boost >= 1.82 (header-only for our MQTT usage).
    # The system has no Boost headers, so we fetch them here.
    # OVERRIDE_FIND_PACKAGE makes async_mqtt's find_package(Boost) use this.
    # NOTE: First configure downloads ~150 MB (cached in build/_deps afterward).
    FetchContent_Declare(Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        OVERRIDE_FIND_PACKAGE)
    set(BOOST_INCLUDE_LIBRARIES
        asio beast container container_hash date_time endian
        multi_index numeric preprocessor system)
    set(BOOST_SKIP_INSTALL_RULES ON)
    set(BUILD_TESTING OFF)
    FetchContent_MakeAvailable(Boost)

    # Boost 1.82+ modular cmake splits headers per-library (Boost::asio, etc.).
    # IMPORTED GLOBAL is excluded from install/export sets.
    # async_mqtt links against the legacy Boost::boost monolithic target.
    # The modular cmake splits headers per-library; collect all libs/*/include dirs
    # so that every Boost header is reachable via a single IMPORTED target.
    # Some Boost libs are nested two levels (e.g. libs/numeric/conversion/include)
    file(GLOB _boost_all_includes
        "${boost_SOURCE_DIR}/libs/*/include"
        "${boost_SOURCE_DIR}/libs/*/*/include")
    if(NOT TARGET Boost::boost)
        add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
        set_property(TARGET Boost::boost PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES "${_boost_all_includes}")
    endif()

    # async_mqtt 10.x (header-only; uses Boost.Asio)
    FetchContent_Declare(async_mqtt
        GIT_REPOSITORY https://github.com/redboltz/async_mqtt.git
        GIT_TAG        10.3.0
        GIT_SHALLOW    TRUE)
    set(ASYNC_MQTT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ASYNC_MQTT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(ASYNC_MQTT_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(async_mqtt)

    # async_mqtt requires C++17 minimum; we need C++20 for coroutines
    set(CMAKE_CXX_STANDARD 20)

    include_directories(common/include)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

    include_directories(
        ${MOSQUITTO_INCLUDE_DIRS}
        common/include)

    link_directories(${MOSQUITTO_LIBRARY_DIRS})
endif()

add_subdirectory(common)
add_subdirectory(sensor_farm)
add_subdirectory(tcu)
add_subdirectory(backend)
