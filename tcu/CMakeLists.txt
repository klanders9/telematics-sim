add_executable(tcu
    src/tcu_main.cpp
)

target_link_libraries(tcu
    common
    ${MOSQUITTO_LIBRARIES}
)