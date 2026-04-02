# telematics-sim
Sandbox for playing with mqtt and someip 

# Build commands:
### libmosquitto (default):
`cmake .. && make`

### async_mqtt:
`cmake .. -DUSE_ASYNC_MQTT=ON -DCMAKE_CXX_COMPILER=g++-12 && make`
