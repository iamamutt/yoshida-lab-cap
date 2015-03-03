CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

project(HY_LAB_CAM_SYNC)

set(BIN_NAME cogdevcam)
set(CMAKE_BUILD_TYPE Release)
set(SOURCE_FILES main.cpp)
set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME OFF)

add_compile_options(-std=c++11)

find_package(OpenCV REQUIRED)
find_package(Boost 1.55.0 COMPONENTS program_options REQUIRED system REQUIRED filesystem REQUIRED date_time REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include_directories($ENV{OpenCV_DIR}/include ${Boost_INCLUDE_DIRS})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
endif()

link_directories(${Boost_LIBRARY_DIRS})

add_executable(${BIN_NAME} ${SOURCE_FILES})
target_link_libraries(${BIN_NAME} ${OpenCV_LIBS} ${Boost_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
