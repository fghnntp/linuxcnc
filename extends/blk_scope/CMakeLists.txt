cmake_minimum_required(VERSION 3.12)
project(blk_scope VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost（使用传统变量方式）
# find_package(Boost 1.70 REQUIRED COMPONENTS system interprocess)
# if(Boost_FOUND)
#     include_directories(${Boost_INCLUDE_DIRS})
#     message(STATUS "Found Boost: ${Boost_VERSION}")
# else()
#     message(FATAL_ERROR "Could NOT find Boost")
# endif()

# 查找 redis++
find_package(redis++ REQUIRED)

include_directories(${PROJECT_NAME} PUBLIC
    /home/user/srcCode/linuxcnc-master/include
)

add_library(${PROJECT_NAME} SHARED
    blk_scope.cc
    gather_sender.cc)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    -DRTAPI)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    redis++::redis++
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
-Wl,--whole-archive
${CMAKE_CURRENT_SOURCE_DIR}/libs/libServoModel.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/amodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/bmodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/cmodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/xmodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/ymodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/zmodel_win64_win64.a
${CMAKE_CURRENT_SOURCE_DIR}/libs/VirtualMachineToolSamp_win64_win64.a
-Wl,--no-whole-archive
)

# 可选：打印调试信息
message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")