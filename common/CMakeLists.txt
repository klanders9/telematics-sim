add_library(common
    src/json_utils.cpp
)

target_include_directories(common PUBLIC include)

if(USE_ASYNC_MQTT)
    target_sources(common PRIVATE src/async_mqtt_client.cpp)
    # async_mqtt_iface is the target name from project(async_mqtt_iface)
    target_link_libraries(common PUBLIC async_mqtt_iface Boost::headers)
    target_compile_definitions(common PUBLIC USE_ASYNC_MQTT)
    target_compile_features(common PUBLIC cxx_std_20)
else()
    target_sources(common PRIVATE src/mosquitto_client.cpp)
    target_link_libraries(common PUBLIC ${MOSQUITTO_LIBRARIES})
endif()
