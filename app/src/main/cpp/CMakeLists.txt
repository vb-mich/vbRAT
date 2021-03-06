#vbRAT CMakeList
cmake_minimum_required(VERSION 3.4.1)

include_directories(openssl/include)
include_directories(.)
include_directories(tiny_websockets_lib/include/)

file(GLOB_RECURSE tinyws_SOURCES
        "tiny_websockets_lib/src/*.cpp"
        )

file(GLOB_RECURSE tinyws_HEADERS
        "tiny_websockets_lib/include/*.h"
        "tiny_websockets_lib/include/*.hpp"
        )
file(GLOB_RECURSE json_SOURCES
        "json/*.cpp"
        )
file(GLOB_RECURSE json_HEADERS
        "json/*.h"
        )
add_library( # Sets the name of the library.
            native-lib
            SHARED

            ${tinyws_HEADERS} ${tinyws_SOURCES}
            ${json_HEADERS} ${json_SOURCES}
            vbConnection.cpp
            vbConnection.h
            vbTTY.cpp
            vbTTY.h
            vbDeviceProperties.cpp
            vbDeviceProperties.h
            openpty.cpp
            openpty.h
            common.h
            vbRAT.cpp)

add_definitions(-D__linux__)
add_definitions(-Werror=return-type) #do not remove. I lost 3 hours trying to figure out a SIGILL due to a non returning fuction
find_library(
              log-lib
              log )

add_library( libssl STATIC IMPORTED )
set_target_properties( libssl PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/openssl/${ANDROID_ABI}/libssl.a)
add_library( libcrypto STATIC IMPORTED )
set_target_properties( libcrypto PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/openssl/${ANDROID_ABI}/libcrypto.a)

target_link_libraries( # Specifies the target library.
        native-lib
        libssl
        libcrypto
        ${log-lib} )
##            && ./configure --quiet CPPFLAGS=-DPIC CFLAGS=-fPIC CXXFLAGS=-fPIC LDFLAGS=-fPIC "${ANDROID_BUILD_OPTS[@]}" --disable-soname-versions \
#        && ./configure --quiet CPPFLAGS=-DPIC CFLAGS=-fPIC CXXFLAGS=-fPIC LDFLAGS=-fPIC "${ANDROID_BUILD_OPTS[@]}" ${CURVE} --without-docs \