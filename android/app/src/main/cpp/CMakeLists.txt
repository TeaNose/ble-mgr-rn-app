cmake_minimum_required(VERSION 3.18.1)
project(NativeRootDetection C)

add_library(native_root_detection SHARED native_root_check.c)
add_library(root_detector SHARED root_detector.c)

find_library(log-lib log)

target_link_libraries(native_root_detection
                      ${log-lib})
target_link_libraries(root_detector
                      ${log-lib})
                
