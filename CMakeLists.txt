cmake_minimum_required(VERSION 3.12)

set(ProjectName gtc)
project(${ProjectName})

set(SOURCES "main.c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu99 -lform -lmenu -lncurses -O2 -o gtc")
add_executable(${ProjectName} ${SOURCES})
