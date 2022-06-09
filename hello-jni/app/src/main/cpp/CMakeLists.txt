cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/couchbase CouchbaseLiteC)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)

add_library(hello-jni SHARED
            DatabaseManager.cpp
            ReplicatorManager.cpp
            hello-jni.cpp)

# Include libraries needed for hello-jni lib
target_link_libraries(hello-jni
                      android
                      CouchbaseLiteC
                      c++_shared
                      log)
