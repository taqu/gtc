cmake_minimum_required(VERSION 3.6)

set(ProjectName gtc)
project(${ProjectName})

#set(CMAKE_C_COMPIER clang)
#set(CMAKE_CXX_COMPILER clang++)

include_directories(AFTER, "${CMAKE_CURRENT_SOURCE_DIR}/include")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")


set(SOURCES "gtc.cpp;NIC.cpp;QDisc.cpp;FlIntSliderInput.cpp;FlFloatSliderInput.cpp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -o gtc")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
add_executable(${ProjectName} ${SOURCES})

target_link_libraries(${ProjectName} fltk X11 dl)
