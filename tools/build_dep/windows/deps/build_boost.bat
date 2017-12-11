::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
echo.
goto start_script
::: create link to different location but still use buildtools as first path
::: run administrator cmd prompt, cd to the buildtools directory
::: mklink /J  ..\libs c:\libs
:start_script
set __pwd=%~dp0
set __cwd=%cd%
set __compiler=%~1
set __cmake_generator=%~2
set __pkg_source_dir=%~3
set __add_cmake_opts=%~4
set __pkg_install_dir=%~5
set __msvc=%~6
set __pkg_build_dir=%__pkg_source_dir%\build
set _FAILED=0

if "%__pkg_install_dir%"=="" set __pkg_install_dir=%__pkg_source_dir%\install
mkdir "%__pkg_install_dir%" > nul

if "%__cmake_generator%"=="" (
    echo Invalid cmake generator
    set _FAILED=1
    goto __end_build_pkg
)

::: set some variables
cd /d "%__pkg_source_dir%"
set __pkg_source_dir=%cd%
mkdir %__pkg_build_dir% > nul
cd /d "%__pkg_build_dir%"
set __pkg_build_dir=%cd%
cd /d %__cwd%

REM set LIBS_DIR=%__pwd%..\libs
REM set BOOST_DIR=%LIBS_DIR%\boost
REM set BOOST_VER=boost_1_65_1


REM echo Making directories...
REM if not exist %BOOST_DIR%\NUL mkdir %BOOST_DIR%

REM echo Changing directory to project specific libraries directory...
REM echo - %LIBS_DIR%
REM cd %LIBS_DIR%

REM set __boost_src_zip=%LIBS_DIR%\boost_src.7z

REM if exist %__boost_src_zip% goto BOOST_DOWNLOADED
REM echo Downloading Boost... please wait.
REM echo.
REM echo.
REM powershell.exe -nologo -noprofile -command (new-object System.Net.WebClient).DownloadFile('https://dl.bintray.com/boostorg/release/1.65.1/source/%BOOST_VER%.7z',' %__boost_src_zip%')

REM :BOOST_DOWNLOADED

REM if exist %BOOST_DIR%\%BOOST_VER%\bootstrap.bat goto BOOST_UNZIPPED
REM echo Unzipping Boost... please wait.
REM echo.
REM echo.
REM @REM powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%__boost_src_zip%', '%BOOST_DIR%'); }"
REM %__7z_exe% x %__boost_src_zip% -aoa -o%BOOST_DIR%

REM :BOOST_UNZIPPED
REM echo.
REM echo Building boost
REM echo.

REM if %__compiler% == msvc (
REM     call %MSBUILD_ENV% x64
REM )

if %__compiler%==msvc (
    call "%__msvc%" x64
)
cd "%__pkg_source_dir%\tools\build"
call "bootstrap.bat" %__compiler%

::: build the boost copy program to build a new tree with specific modules
b2.exe toolset=%__compiler% ../bcp
set partial_tree=%__pkg_source_dir%\partial_tree
mkdir "%partial_tree%" > nul
cd "%__pkg_source_dir%\dist\bin"
bcp.exe --boost="%__pkg_source_dir%" program_options filesystem date_time build bootstrap.bat bootstrap.sh boostcpp.jam boost-build.jam %partial_tree%

::: build submodules
cd "%partial_tree%"
call "bootstrap.bat" %__compiler%
b2.exe -q --prefix=%__pkg_install_dir% --build-dir="build" --layout="tagged" toolset=%__compiler% headers
b2.exe -q --prefix=%__pkg_install_dir% --build-dir="build" --layout="tagged" toolset=%__compiler% variant=debug,release address-model=64 link=static threading=multi runtime-link=static install

cd /d "%__pkg_install_dir%"
rmdir /s /q "%partial_tree%"

:__end_build_pkg

endlocal & (
    set _FAILED=%_FAILED%
    set __pkg_build_dir=%partial_tree%
    set __pkg_install_dir=%__pkg_install_dir%
    cd /d %__cwd%
)

