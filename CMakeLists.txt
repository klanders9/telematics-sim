cmake_minimum_required(VERSION 3.31.6)
project(telematics_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

include_directories(
        ${MOSQUITTO_INCLUDE_DIRS}
        common/include
)

link_directories(${MOSQUITTO_LIBRARY_DIRS})

add_subdirectory(common)
add_subdirectory(sensor_farm)
add_subdirectory(tcu)
add_subdirectory(backend)

