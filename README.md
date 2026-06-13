# telematics-sim
Sandbox for playing with mqtt and someip 

# Build commands:
### libmosquitto (default):
```bash
cmake -B build && cmake --build build
```

### async_mqtt:
```bash
cmake -B build -DUSE_ASYNC_MQTT=ON -DCMAKE_CXX_COMPILER=g++-12 && cmake --build build
```

### Cross-compile for Raspberry Pi 4 (aarch64):
```bash
cmake -B build-rpi4 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-rpi4.cmake \
  -DUSE_ASYNC_MQTT=ON
cmake --build build-rpi4
```
Requires the cross-toolchain at `/opt/rpi-tools/aarch64-rpi4-linux-gnu`. `USE_ASYNC_MQTT=ON` is required since mosquitto dev libs are not in the sysroot.
