CMAKE_MINIMUM_REQUIRED( VERSION 3.0 )

set( PROJECT_NAME yoshida-lab-cap )
set( BIN_NAME cdlcap )
set( CMAKE_BUILD_TYPE Release )
set( HEADER_FILES rec.hpp axis.hpp)
set( SOURCE_FILES main.cpp rec.cpp axis.cpp)

project( ${PROJECT_NAME} )

find_package( OpenCV REQUIRED )

add_executable( ${BIN_NAME} ${SOURCE_FILES} ${HEADER_FILES} )

# Windows doesn't like OpenCV_Includes for some reason
include_directories( ${OpenCV_DIR}/../../../include )

target_link_libraries( ${BIN_NAME} ${OpenCV_LIBS} )

set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin )
