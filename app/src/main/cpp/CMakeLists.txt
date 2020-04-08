# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

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

add_library( # Sets the name of the library.
            native-lib
            SHARED

        ${tinyws_HEADERS} ${tinyws_SOURCES}
            handlepty.cpp
            handlepty.h
            openpty.cpp
            openpty.h
            common.h
            native-lib.cpp )
add_definitions(-D__linux__)
find_library( # Sets the name of the path variable.
              log-lib
              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

#add_library( libcurl STATIC IMPORTED )
#set_target_properties( libcurl PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/curl/${ANDROID_ABI}/libcurl.a)

add_library( libssl STATIC IMPORTED )
set_target_properties( libssl PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/openssl/${ANDROID_ABI}/libssl.a)
add_library( libcrypto STATIC IMPORTED )
set_target_properties( libcrypto PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/openssl/${ANDROID_ABI}/libcrypto.a)

target_link_libraries( # Specifies the target library.
        native-lib
        libssl
        libcrypto
        ${log-lib} )
