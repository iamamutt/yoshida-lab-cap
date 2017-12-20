#------------------------------------------------------------------------------
# Joseph M. Burling, 2017
#   jmburling@ucla.edu
#
# -DCMAKE_BUILD_TYPE=Release
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.6.2)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/CompilerOptions.cmake")
project(cogdevcam CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#------------------------------------------------------------------------------
# top level directories
#
#------------------------------------------------------------------------------
set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_NAME})
set(PROJECT_INCLUDE_DIRS ${PROJECT_ROOT_DIR}/include)
set(PROJECT_TEST_FILES ${PROJECT_ROOT_DIR}/tests)
set(MAIN_EXEC_FILE "${PROJECT_ROOT_DIR}/main.cpp")

#------------------------------------------------------------------------------
# System info
#
#------------------------------------------------------------------------------
set(PROJECT_COMPILER ${CMAKE_CXX_COMPILER_ID}) #Clang, GNU, MSVC, AppleClang
set(SYS_NAME ${CMAKE_SYSTEM_NAME})

if (WIN32)
        set(USER_LIB_ROOT_DEFAULT "$ENV{USERPROFILE}")
        set(COMPILER_SUBDIR_DEFAULT ${PROJECT_COMPILER})
else ()
        set(USER_LIB_ROOT_DEFAULT "/usr/local")
        set(COMPILER_SUBDIR_DEFAULT ".")
endif ()
#------------------------------------------------------------------------------
# Cmake variables.
#  -DWITH_OPENCV=TRUE -DWITH_BOOST=TRUE -DWITH_RTAUDIO=TRUE
#------------------------------------------------------------------------------

# -DCUSTOM_LIB_ROOT=""
set(CUSTOM_LIB_ROOT ${USER_LIB_ROOT_DEFAULT}/lib CACHE PATH
    "OPTIONAL. Root path to where custom built libraries are located, e.g. /usr/local or C:/Users/username/libs")

# -DCOMPILER_SUBDIR=""
set(COMPILER_SUBDIR ${COMPILER_SUBDIR_DEFAULT} CACHE STRING
    "OPTIONAL. Name of subfolder that contains compiler specific libraries.")

# -DWITH_OPENCV=TRUE
set(WITH_OPENCV TRUE CACHE BOOL
    "Toggle ON/OFF OpenCV")

# -DOPENCV_INSTALL_DIR=libs/opencv
set(OPENCV_INSTALL_DIR "${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/opencv" CACHE PATH
    "Root path where OpenCV libraries are installed")

# -DWITH_BOOST=TRUE
set(WITH_BOOST TRUE CACHE BOOL
    "Toggle ON/OFF Boost")

# -DBOOST_INSTALL_DIR=libs/boost
set(BOOST_INSTALL_DIR "${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/boost" CACHE PATH
    "Root path where Boost libraries are installed")

# -DWITH_RTAUDIO=TRUE
set(WITH_RTAUDIO TRUE CACHE BOOL
    "Toggle ON/OFF RtAudio")

# -DRTAUDIO_INSTALL_DIR=libs/rtaudio
set(RTAUDIO_INSTALL_DIR "${CUSTOM_LIB_ROOT}/${COMPILER_SUBDIR}/rtaudio" CACHE PATH
    "Root path where compiled RtAudio library is installed")

# -DBUILD_TESTS=TRUE
set(BUILD_TESTS TRUE CACHE BOOL
    "Build small test programs")

# -DBIN_NAME=myProgam
set(BIN_NAME cogdevcam CACHE STRING
    "Name of executable")

#------------------------------------------------------------------------------
# Windows specific stuff
#
#------------------------------------------------------------------------------
if (WIN32)
        set(WIN_SDK_DIR_HINT "C:/Program Files (x86)/Windows Kits")
        if (MSVC)
                set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD ON)
                set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
                set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
                set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /O1 /Ob1 /DNDEBUG")
        endif ()
endif ()

#------------------------------------------------------------------------------
# Boost Modules
#   Some modules need to be built and are not header only. See README.md
#------------------------------------------------------------------------------
if (WITH_BOOST)
        set(Boost_NO_BOOST_CMAKE ON)
        set(Boost_USE_STATIC_LIBS ON)
        set(Boost_USE_STATIC_RUNTIME ON)
        set(Boost_DETAILED_FAILURE_MSG ON)
        set(Boost_USE_MULTITHREADED ON)
        set(Boost_ADDITIONAL_VERSIONS 106000 106500 106501)
        set(BOOST_ROOT ${BOOST_INSTALL_DIR}) # also set $ENV{BOOST_ROOT}
        add_definitions(-DBOOST_ALL_NO_LIB)
        if (WIN32 OR MINGW)
                set(Boost_NO_SYSTEM_PATHS ON)
                file(GLOB BOOST_LIBRARY_SEARCH "${BOOST_INSTALL_DIR}/lib")
                file(GLOB BOOST_INCLUDE_SEARCH "${BOOST_INSTALL_DIR}/include/boost*")
                set(BOOST_LIBRARYDIR "${BOOST_LIBRARY_SEARCH}")
                set(BOOST_INCLUDEDIR "${BOOST_INCLUDE_SEARCH}")
        endif ()
        find_package(Boost 1.60 REQUIRED COMPONENTS program_options filesystem system date_time)
        if (NOT Boost_FOUND)
                message(FATAL_ERROR "Boost not found!")
        endif ()

        list(APPEND PROJECT_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
endif ()

#------------------------------------------------------------------------------
# OpenCV Library
#   Set environment variable OpenCV_DIR to directory where opencv was installed
#   Look for files OpenCVConfig.cmake and cmake_install.cmake in staticlibs
#------------------------------------------------------------------------------
if (WITH_OPENCV)
        find_package(OpenCV REQUIRED HINTS ${OPENCV_INSTALL_DIR})
        if (NOT OPENCV_CORE_FOUND)
                message(FATAL_ERROR "OpenCV Libraries not found!")
        endif ()

        list(APPEND PROJECT_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS})
endif ()

#------------------------------------------------------------------------------
# RTAudio
#
#------------------------------------------------------------------------------
if (WITH_RTAUDIO)
        set(RtAudio_EXTERN_LIST)

        if (WIN32)
                # RtAudio library
                find_library(RtAudio_STATIC_LIBRARIES NAMES librtaudio_static
                             HINTS "$ENV{RTAUDIO_DIR}" "${RTAUDIO_INSTALL_DIR}" PATH_SUFFIXES lib)

                # Compiled RtAudio with DirectSound support needs shared Windows dsound library
                find_library(DSound_SHARED_LIB dsound HINTS ${WIN_SDK_DIR_HINT})

                # RtAudio and header
                find_path(RtAudio_INCLUDE_DIRS RtAudio.h HINTS
                          $ENV{RTAUDIO_DIR} ${RTAUDIO_INSTALL_DIR}
                          PATH_SUFFIXES include)

                if (NOT RtAudio_STATIC_LIBRARIES)
                        message(FATAL_ERROR "RtAudio compiled library --with-ds not found!")
                endif ()

                if (NOT RtAudio_INCLUDE_DIRS)
                        message(FATAL_ERROR "RtAudio.h source file not found!")
                endif ()

                if (NOT DSound_SHARED_LIB)
                        message(FATAL_ERROR "dsound library not found!")
                endif ()

                # Link static RtAudio lib
                add_library(RtAudio_STATIC_LIBRARIES STATIC IMPORTED GLOBAL)
                set_target_properties(RtAudio_STATIC_LIBRARIES PROPERTIES
                                      IMPORTED_LOCATION "${RtAudio_STATIC_LIB}"
                                      IMPORTED_IMPLIB "${RtAudio_STATIC_LIB}"
                                      INSTALL_RPATH "${RtAudio_STATIC_LIB}"
                                      INSTALL_RPATH_USE_LINK_PATH TRUE)

                # Save RtAudio header file and include directory
                list(APPEND PROJECT_INCLUDE_DIRS ${RtAudio_INCLUDE_DIRS})

                # include path and library for dsound
                list(APPEND RtAudio_EXTERN_LIST ${DSound_SHARED_LIB})
                file(GLOB_RECURSE DSound_INCLUDE_FILE LIST_DIRECTORIES false "${WIN_SDK_DIR_HINT}/**/*/dsound.h")
                get_filename_component(DSound_INCLUDE_DIRS DSound_INCLUDE_FILE DIRECTORY)
                find_path(DSound_INCLUDE_DIRS NAMES DSound_INCLUDE_FILE)
                list(APPEND PROJECT_INCLUDE_DIRS ${DSound_INCLUDE_DIRS})
        endif ()

        if (UNIX AND NOT APPLE) # ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux"
                find_package(PkgConfig REQUIRED)
                pkg_check_modules(RtAudio REQUIRED rtaudio)

                if (NOT RtAudio_FOUND)
                        find_library(RtAudio_STATIC_LIBRARIES NAMES librtaudio.a
                                     HINTS /usr/local "$ENV{RTAUDIO_DIR}" "${RTAUDIO_INSTALL_DIR}" PATH_SUFFIXES lib)

                        find_path(RtAudio_INCLUDE_DIRS RtAudio.h HINTS /usr/local/include
                                  $ENV{RTAUDIO_DIR} ${RTAUDIO_INSTALL_DIR}
                                  PATH_SUFFIXES rtaudio)
                endif ()

                if (NOT RtAudio_STATIC_LIBRARIES)
                        message(FATAL_ERROR "RtAudio library not found!")
                endif ()

                if (NOT RtAudio_INCLUDE_DIRS)
                        message(FATAL_ERROR "RtAudio header not found!")
                endif ()

                list(APPEND PROJECT_INCLUDE_DIRS ${RtAudio_INCLUDE_DIRS})
        endif ()

        if (APPLE)
                # brew install rt-audio
                find_library(RtAudio_STATIC_LIBRARIES NAMES rtaudio
                             HINTS usr/local/lib "$ENV{RTAUDIO_DIR}" "${RTAUDIO_INSTALL_DIR}")
                if (RtAudio_STATIC_LIBRARIES)
                        if (APPLE)
                                find_library(COREAUDIO_LIB CoreAudio)
                                find_library(COREFOUNDATION_LIB CoreFoundation)
                                list(APPEND RtAudio_EXTERN_LIST ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
                        else ()
                                message(FATAL_ERROR "RtAudio compiled library --with-core not found!")
                        endif ()
                endif ()
        endif ()
endif ()

#------------------------------------------------------------------------------
# Finish up
#   To build VS solution. cd to new directory vsbuild, then:
#     cmake .. -G "Visual Studio 15 2017 Win64"
#------------------------------------------------------------------------------
include_directories(${PROJECT_INCLUDE_DIRS})
foreach (COLLECTED_SRC_DIR ${PROJECT_INCLUDE_DIRS})
        message("\t- ${COLLECTED_SRC_DIR}")
endforeach (COLLECTED_SRC_DIR)

set(EXEC_OUTPUT_NAMES ${BIN_NAME})

if (BUILD_TESTS)
        list(APPEND EXEC_OUTPUT_NAMES test_audio)
        add_executable(test_audio "${PROJECT_TEST_FILES}/test_audio.cpp")
        target_link_libraries(test_audio ${RtAudio_STATIC_LIBRARIES} ${RtAudio_EXTERN_LIST} ${Boost_LIBRARIES})

        list(APPEND EXEC_OUTPUT_NAMES test_video)
        add_executable(test_video "${PROJECT_TEST_FILES}/test_video.cpp")
        target_link_libraries(test_video ${OpenCV_LIBS})
endif ()

add_executable(${BIN_NAME} ${MAIN_EXEC_FILE})

if (WITH_BOOST)
        target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
else ()
        unset(BOOST_INSTALL_DIR CACHE)
        unset(Boost_LIBRARIES CACHE)
endif ()

if (WITH_OPENCV)
        target_link_libraries(${BIN_NAME} ${OpenCV_LIBS})
else ()
        unset(OPENCV_INSTALL_DIR CACHE)
        unset(OpenCV_LIBS CACHE)
endif ()

if (WITH_RTAUDIO)
        target_link_libraries(${BIN_NAME} ${RtAudio_STATIC_LIBRARIES} ${RtAudio_EXTERN_LIST})
else ()
        unset(RTAUDIO_INSTALL_DIR CACHE)
        unset(RtAudio_STATIC_LIBRARIES CACHE)
        unset(RtAudio_EXTERN_LIST CACHE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/build/install.d")
else ()
        set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/build/install")
endif ()

install(TARGETS ${EXEC_OUTPUT_NAMES}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
