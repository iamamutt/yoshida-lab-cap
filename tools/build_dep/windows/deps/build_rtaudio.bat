::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
set __pwd=%~dp0
set __cwd=%cd%
set __compiler=%~1
set __cmake_generator=%~2
set __rtaudio_src=%~3
set __add_cmake_opts=%~4
set __rtaudio_install=%~5
set __msvc=%~6
set __rtaudio_build=%__rtaudio_src%\build
set __install_root=
set __build_root=
set _FAILED=0

if "%__rtaudio_install%"=="" set __rtaudio_install=%__rtaudio_src%\install
cd /d "%__rtaudio_src%"
copy rtaudio.pc.in rtaudio.pc

::: ------------------------------------------------------------------------------------------------

mkdir %__rtaudio_build% > nul
cd /d "%__rtaudio_build%"
set __rtaudio_build=%cd%

::: ------------------------------------------------------------------------------------------------
if exist CMakeCache.txt (
    del CMakeCache.txt
)

cmake -G "%__cmake_generator%" ^
-DCMAKE_INSTALL_PREFIX="%__rtaudio_install%" ^
-DCMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD=ON ^
-DCMAKE_BUILD_TYPE=Debug;Release ^
-DAUDIO_WINDOWS_DS=ON ^
%__add_cmake_opts% ^
--build "%__rtaudio_src%"

::: ------------------------------------------------------------------------------------------------
if %__compiler%==msvc (
    call "%__msvc%" x64
    cd "%__rtaudio_build%"
    MSBuild.exe RtAudio.sln /p:Configuration=Debug /p:Platform=x64 /p:BuildProjectReferences=false /v:m /m
    MSBuild.exe RtAudio.sln /p:Configuration=Release /p:Platform=x64 /p:BuildProjectReferences=false /v:m /m

    if exist Release\rtaudio_static.lib (
        copy /b Release\rtaudio_static.lib "%__rtaudio_install%\lib\rtaudio_static.lib" /y /v
    )

    robocopy Debug %__rtaudio_install%\debug /E /COPYALL

    goto end_build_script
)

if %__compiler%==gcc (
    mingw32-make -j4
    mingw32-make install
    if exist librtaudio_static.a (
        copy /b librtaudio_static.a "%__rtaudio_install%\lib\librtaudio_static.lib" /y /v
    )
    goto end_build_script
)

cd /d "%__rtaudio_install%"
rmdir /s /q "%__rtaudio_src%"

::: ------------------------------------------------------------------------------------------------
:end_build_script

endlocal & (
    set _FAILED=%_FAILED%
    set __build_root=%__rtaudio_build%
    set __install_root=%__rtaudio_install%
    cd /d %__cwd%
)
