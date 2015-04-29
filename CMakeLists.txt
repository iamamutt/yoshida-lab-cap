CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

add_compile_options(-std=c++11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -v")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(LINKLIBS)
set(SOURCE_FILES main.cpp)
set(BIN_NAME cogdevcam)
set(INCLUDES include)

project(HY_LAB_CAM_SYNC)

set(Boost_USE_STATIC_LIBS    ON)
set(Boost_USE_MULTITHREADED  ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(OpenCV REQUIRED)
find_package(Boost 1.55.0 COMPONENTS program_options REQUIRED system REQUIRED filesystem REQUIRED date_time REQUIRED)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_library(RTAUDIO rtaudio_static HINTS $ENV{RTAUDIO} lib NO_DEFAULT_PATH)
    find_library(DSOUND dsound lib)
    list(APPEND LINKLIBS ${RTAUDIO} ${DSOUND})
    list(APPEND SOURCE_FILES include/dsound.h)
    include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${INCLUDES})
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(RTAUDIO rtaudio_static HINTS usr/local/lib $ENV{RTAUDIO} lib)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    list(APPEND LINKLIBS ${RTAUDIO} ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
    include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${INCLUDES})
endif()

link_directories(${Boost_LIBRARY_DIRS})
add_executable(${BIN_NAME} ${SOURCE_FILES})
target_link_libraries( ${BIN_NAME} ${OpenCV_LIBS} ${Boost_LIBRARIES} ${LINKLIBS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_BUILD_TYPE Release)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()



# cmake -G "Visual Studio 12 2013 Win64"
