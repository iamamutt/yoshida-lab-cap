::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
echo.
goto start_script
:: May need to make changes to the source files
::   - in opencv/modules/videoio/src/cap_dshow.cpp add before #include <Dshow.h>
::      #define NO_DSHOW_STRSAFE
::
::   - in opencv\modules\python\src2\cv2.cpp add before #include <Python.h>
::      #define _hypot hypot
::
::
:start_script
set __pwd=%~dp0
set __cwd=%cd%
set __compiler=%~1
set __cmake_generator=%~2
set __opencv_source_dir=%~3
set __add_cmake_opts=%~4
set __opencv_install_dir=%~5
set __msvc=%~6
set __opencv_build_dir=%__opencv_source_dir%\build
set _FAILED=0

if "%__opencv_install_dir%"=="" set __opencv_install_dir=%__opencv_source_dir%\install

if "%__cmake_generator%"=="" (
    echo Invalid cmake generator
    set _FAILED=1
    goto end_build_script
)

cd /d "%__opencv_source_dir%"
set __opencv_source_dir=%cd%

mkdir %__opencv_build_dir% > nul
cd /d "%__opencv_build_dir%"
set __opencv_build_dir=%cd%
if exist CMakeCache.txt del CMakeCache.txt

REM -DCUDA_ARCH_BIN=6.2
REM -DCUDA_SDK_ROOT_DIR=/usr/local/cuda
REM -DCMAKE_BUILD_TYPE=Debug;Release
REM -DWITH_OPENEXR=OFF ^
:: OpenCV build options
cmake -G "%__cmake_generator%" ^
-DCMAKE_INSTALL_PREFIX="%__opencv_install_dir%" ^
-DBUILD_DOCS=ON ^
-DBUILD_opencv_python2=OFF ^
-DBUILD_opencv_python3=ON ^
-DENABLE_PYLINT=ON ^
-DBUILD_OPENEXR=OFF ^
-DBUILD_PERF_TESTS=OFF ^
-DBUILD_SHARED_LIBS=OFF ^
-DBUILD_TESTS=OFF ^
-DENABLE_PRECOMPILED_HEADERS=OFF ^
-DINSTALL_TESTS=OFF ^
-DOPENCV_FORCE_PYTHON_LIBS=ON ^
-DWITH_1394=OFF ^
-DWITH_CUBLAS=OFF ^
-DWITH_CUDA=OFF ^
-DWITH_CUFFT=OFF ^
-DWITH_EIGEN=OFF ^
-DWITH_FFMPEG=ON ^
-DWITH_GDAL=OFF ^
-DWITH_GPHOTO2=OFF ^
-DWITH_GSTREAMER=OFF ^
-DWITH_IPP=OFF ^
-DWITH_ITT=OFF ^
-DWITH_LAPACK=OFF ^
-DWITH_MATLAB=OFF ^
-DWITH_NVCUVID=OFF ^
-DWITH_OPENGL=ON ^
-DWITH_OPENMP=OFF ^
-DWITH_PTHREADS_PF=OFF ^
-DWITH_QT=OFF ^
-DWITH_TBB=OFF ^
-DWITH_VFL=OFF ^
-DWITH_VTK=OFF ^
-DWITH_WEBP=OFF ^
%__add_cmake_opts% ^
--build "%__opencv_source_dir%"


::: ------------------------------------------------------------------------------------------------
if %__compiler%==msvc (
    call "%__msvc%" x64
    cd "%__opencv_build_dir%"
    MSBuild.exe OpenCV.sln /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /v:m /m
    MSBuild.exe OpenCV.sln /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false /v:m /m
    goto end_build_script
)

if %__compiler%==gcc (
    mingw32-make -j4
    mingw32-make install
    goto end_build_script
)

:end_build_script

endlocal & (
    set _FAILED=%_FAILED%
    set __build_root=%__opencv_build_dir%
    set __install_root=%__opencv_install_dir%
    cd /d %__cwd%
)

