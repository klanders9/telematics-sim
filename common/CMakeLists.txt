add_library(common
    src/json_utils.cpp
    src/mqtt_client.cpp
)

target_include_directories(common PUBLIC
        include
)
