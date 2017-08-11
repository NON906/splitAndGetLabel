cmake_minimum_required(VERSION 3.5.1)

include_directories(../julius/libsent/include)
include_directories(../julius/libjulius/include)

link_directories(../julius/libsent)
link_directories(../julius/libjulius)

add_executable(splitAndGetLabel main.cpp execJulius.cpp recordAndSplit.cpp)

target_link_libraries(splitAndGetLabel julius sent pthread dl z asound pulse-simple pulse)
