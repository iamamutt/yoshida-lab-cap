::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
set __pwd=%~dp0
set __compiler=%~1
set __cmake_generator=%~2
echo.
echo Build environment vars script...
echo.
if /I "%1"=="msvc" goto set_msvc
if /I "%1"=="m" goto set_msvc
goto not_msvc
:set_msvc
set __compiler=msvc
set __cmake_generator="Visual Studio 15 2017 Win64"
goto __end_build_vars
:not_msvc

if /I "%1"=="mingw" goto set_mingw
if /I "%1"=="g" goto set_mingw
if /I "%1"=="gcc" goto set_mingw
goto not_mingw
:set_mingw
set __compiler=gcc
set __cmake_generator="MinGW Makefiles"
goto __end_build_vars
:not_mingw

if /I "%1"=="cygwin" goto set_cygwin
if /I "%1"=="c" goto set_cygwin
goto not_cygwin
:set_cygwin
set __compiler=gcc
set __cmake_generator="Unix Makefiles"
goto __end_build_vars
:not_cygwin

:: if reached here then compiler not found
echo "compiler of type '%__compiler%' is invalid!"

:__end_build_vars

endlocal & (
    set compiler=%__compiler%
    if not "%~2"=="" set %~2=%__cmake_generator%
    set cmake_generator=%__cmake_generator%
    cd /d %__cwd%
)
echo.
