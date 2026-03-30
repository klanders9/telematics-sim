add_executable(sensor_farm_sim
    src/sensor_farm_main.cpp
)

target_link_libraries(sensor_farm_sim
    common
    ${MOSQUITTO_LIBRARIES}
)
