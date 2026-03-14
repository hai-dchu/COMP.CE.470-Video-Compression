
cmake_minimum_required(VERSION 3.10)
# Set the project name
project(Videocourse_cabac VERSION 1.0 LANGUAGES CXX)
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(Videocourse_cabac ex_6.cpp)
add_executable(helper helper.cpp)

# Copy output executables back to the main directory
add_custom_command(TARGET Videocourse_cabac POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Videocourse_cabac> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:Videocourse_cabac>
)
add_custom_command(TARGET helper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:helper> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:helper>
)