CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

project(HY_LAB_CAM_SYNC)

set(BIN_NAME cogdevcam)
set(CMAKE_BUILD_TYPE Release)
set(SOURCE_FILES main.cpp)

# build boost with b2.exe --toolset=msvc variant=release address-model=64 link=static threading=multi runtime-link=static stage
set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME ON)

add_compile_options(-std=c++11)

# if using windows using visual studio libraries for opencv
# if osx, make sure to install ffmpeg with opencv
find_package(OpenCV REQUIRED)
find_package(Boost 1.55.0 COMPONENTS program_options REQUIRED system REQUIRED filesystem REQUIRED date_time REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include_directories(${OpenCV_INCLUDE_DIRS} $ENV{OpenCV_DIR}/../../../include ${Boost_INCLUDE_DIRS})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
endif()

link_directories(${Boost_LIBRARY_DIRS})

add_executable(${BIN_NAME} ${SOURCE_FILES})
target_link_libraries(${BIN_NAME} ${OpenCV_LIBS} ${Boost_LIBRARIES} ${FFMPEG_LIB})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")

# cmake -G "Visual Studio 12 2013 Win64"
