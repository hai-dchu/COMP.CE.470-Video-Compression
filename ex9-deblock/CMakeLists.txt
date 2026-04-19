
cmake_minimum_required(VERSION 3.10)
# Set the project name
project(Videocourse_deblock VERSION 1.0 LANGUAGES CXX)
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(Videocourse_deblock ex_9.cpp)

# Copy output executables back to the main directory
add_custom_command(TARGET Videocourse_deblock POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Videocourse_deblock> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:Videocourse_deblock>
)
