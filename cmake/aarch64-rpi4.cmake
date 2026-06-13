# Toolchain file for cross-compiling to aarch64 (Raspberry Pi 4)
# Usage:
#   cmake -B build-rpi4 -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-rpi4.cmake [options]

set(CROSS_ROOT /opt/rpi-tools/aarch64-rpi4-linux-gnu)
set(CROSS_TRIPLE aarch64-rpi4-linux-gnu)

set(CMAKE_SYSTEM_NAME    Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-gcc)
set(CMAKE_CXX_COMPILER ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-g++)
set(CMAKE_AR           ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-ar)
set(CMAKE_RANLIB       ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-ranlib)
set(CMAKE_STRIP        ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-strip)

set(CMAKE_SYSROOT ${CROSS_ROOT}/${CROSS_TRIPLE}/sysroot)

# Search for libraries/headers only in the sysroot; never look at the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Direct pkg-config at the sysroot so that find_package(PkgConfig) / pkg_check_modules
# resolves .pc files for the target, not the host.
set(ENV{PKG_CONFIG_DIR}        "")
set(ENV{PKG_CONFIG_LIBDIR}     "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
