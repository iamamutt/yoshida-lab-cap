CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

add_compile_options(-std=c++11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -v")
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(LINKLIBS)
set(SOURCE_FILES main.cpp include/RtAudio.h)
set(BIN_NAME cogdevcam)

project(HY_LAB_CAM_SYNC)

find_package(OpenCV REQUIRED)
find_package(Boost 1.55.0 COMPONENTS program_options REQUIRED system REQUIRED filesystem REQUIRED date_time REQUIRED)

set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME ON)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    list(APPEND LINKLIBS lib/rtaudio_static lib/dsound)
    list(APPEND SOURCE_FILES include/dsound.h)
    include_directories(${OpenCV_INCLUDE_DIRS} $ENV{OpenCV_DIR}/../../../include ${Boost_INCLUDE_DIRS})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(RTAUDIO rtaudio_static usr/local/lib $ENV{RTAUDIO})
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    list(APPEND LINKLIBS ${RTAUDIO} ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
    include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
endif()

link_directories(${Boost_LIBRARY_DIRS})
add_executable(${BIN_NAME} ${SOURCE_FILES})
target_link_libraries( ${BIN_NAME} ${OpenCV_LIBS} ${Boost_LIBRARIES} ${LINKLIBS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

# cmake -G "Visual Studio 12 2013 Win64"
