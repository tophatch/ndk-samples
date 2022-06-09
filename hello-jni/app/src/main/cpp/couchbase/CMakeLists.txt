cmake_minimum_required(VERSION 3.10.2)

add_library(CouchbaseLiteC SHARED IMPORTED GLOBAL)

set_target_properties(CouchbaseLiteC PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/${ANDROID_ABI}/libcblite.so)

set_property(TARGET CouchbaseLiteC APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include)