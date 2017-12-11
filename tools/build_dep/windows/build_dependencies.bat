::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@echo off
cls
set pwd=%~dp0
set cwd=%cd%
set compiler=%~1

::: ------------------------------------------------------------------------------------------------
::: Toggle variables on/off and set paths
::: ------------------------------------------------------------------------------------------------

::: global toggles
::: --------------------------
set _RTAUDIO=0
set _OPENCV=1
set _BOOST=0

::: build environment options
::: --------------------------

::: msvc, mingw, cygwin
if "%compiler%"=="" set compiler=gcc

::: root of where to install libraries
set root_install_dir=%USERPROFILE%\libs

::: other paths depending on the build chain
set __msvc_toolchain=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat
set __mingw_toolchain=C:\MinGW\mingw64
set __cygwin_toolchain=C:\cygwin64

::: path to 7zip to unzip downloaded files
set __7z_exe=C:\Program Files\7-Zip\7z.exe

::: OpenCV
::: --------------------------
::: path where opencv source code exists or where to download
set opencv_src_root=opencv
set opencv_url=https://github.com/opencv/opencv/archive/3.3.1.zip

::: Boost
::: --------------------------
set boost_src_root=boost
set boost_url=https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.7z

::: RtAudio
::: --------------------------
set rtaudio_src_root=rtaudio
set rtaudio_url=http://www.music.mcgill.ca/~gary/rtaudio/release/rtaudio-5.0.0.tar.gz

::: ------------------------------------------------------------------------------------------------
::: Start build
::: ------------------------------------------------------------------------------------------------
set tools_dir="%pwd%bat_tools"
set pkg_scripts_dir="%pwd%deps"

::: msvc "Visual Studio 15 2017 Win64"
call "%tools_dir%\buildvars.bat" %compiler% %cmake_generator%

set root_install_dir=%root_install_dir:"=%
set toolchain_install_dir=%root_install_dir%\%compiler%

set tools_dir=%tools_dir:"=%
set _FAILED=0
set _add_opts=
echo Batch tools at %tools_dir%
echo.

set opencv_src_root=%toolchain_install_dir%\%opencv_src_root%
set rtaudio_src_root=%toolchain_install_dir%\%rtaudio_src_root%
set boost_src_root=%toolchain_install_dir%\%boost_src_root%

::: check if root exists, make if not
call "%tools_dir%\checkdir.bat" "%toolchain_install_dir%" 1

::: check before downloading anything
if %compiler%==msvc (
    if not exist "%__msvc_toolchain%" (
        echo Specified msvc but not location of visual studio sdk env variables batch file!
        goto __end_build_script
    )
    set _add_opts=-DCMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD=ON %_add_opts%
)

:_build_start
echo Build starting...

::: -------------------------------------------------------------------------------------------------
::: OpenCV
::: -------------------------------------------------------------------------------------------------
echo.
if %_OPENCV%==1 (echo Building OpenCV...) else (goto __end_opencv)
cd /d "%cwd%"
set _FAILED=0
echo.

::: path to saved zip file
set opencv_zipname=opencv_src.zip

::: download source code if not already downloaded
call "%tools_dir%\downloadpkg.bat" "%opencv_url%" "%root_install_dir%" "%opencv_zipname%"
if %_FAILED%==1 goto __end_opencv

::: unzip if not yet unzipped and return updated source directory
call "%tools_dir%\unzipper.bat" "%root_install_dir%\%opencv_zipname%" "%root_install_dir%" opencv-* CMakeLists.txt "%__7z_exe%"
if %_FAILED%==1 goto __end_opencv

::: use zip_subfolder from uzipper.bat as updated source target, install to opencv_src_root
call "%pkg_scripts_dir%\build_opencv.bat" %compiler% %cmake_generator% "%zip_subfolder%" "%_add_opts%" "%opencv_src_root%" "%__msvc_toolchain%"

cd /d "%cwd%"
:__end_opencv

::: -------------------------------------------------------------------------------------------------
::: RtAudio
::: -------------------------------------------------------------------------------------------------
echo.
if %_RTAUDIO%==1 (echo Building RtAudio...) else (goto __end_rtaudio)
cd /d "%cwd%"
set _FAILED=0
echo.

::: path to saved zip file
set rtaudio_zipname=rtaudio_src.tar.gz

::: download source code if not already downloaded
call "%tools_dir%\downloadpkg.bat" "%rtaudio_url%" "%root_install_dir%" "%rtaudio_zipname%"
if %_FAILED%==1 goto __end_rtaudio

::: unzip if not yet unzipped and return updated source directory
call "%tools_dir%\unzipper.bat" "%root_install_dir%\%rtaudio_zipname%" "%root_install_dir%" rtaudio-* CMakeLists.txt "%__7z_exe%"
if %_FAILED%==1 goto __end_rtaudio

::: use zip_subfolder from uzipper.bat as updated source target, install to rtaudio_src_root
call "%pkg_scripts_dir%\build_rtaudio.bat" %compiler% %cmake_generator% "%zip_subfolder%" "%_add_opts%" "%rtaudio_src_root%" "%__msvc_toolchain%"

:__end_rtaudio

::: -------------------------------------------------------------------------------------------------
::: Boost
::: -------------------------------------------------------------------------------------------------
echo.
if %_BOOST%==1 (echo Building Boost...) else (goto __end_boost)
cd /d "%cwd%"
set _FAILED=0
echo.

::: path to saved zip file
set boost_zipname=boost_src.7z
call "%tools_dir%\downloadpkg.bat" "%boost_url%" "%root_install_dir%" "%boost_zipname%"
if %_FAILED%==1 goto __end_boost
call "%tools_dir%\unzipper.bat" "%root_install_dir%\%boost_zipname%" "%root_install_dir%" boost_1_6* bootstrap.bat "%__7z_exe%"
if %_FAILED%==1 goto __end_boost
call "%pkg_scripts_dir%\build_boost.bat" %compiler% %cmake_generator% "%zip_subfolder%" "" "%boost_src_root%" "%__msvc_toolchain%"

:__end_boost
:__end_build_script

cd /d "%cwd%"
echo.
if _FAILED==1 (echo Build did not complete!)
ping -n 2 127.0.0.1 > nul
