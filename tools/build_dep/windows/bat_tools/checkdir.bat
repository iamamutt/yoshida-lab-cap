::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
goto __start_checkdir
::echo. 2>"%_tempfile%"
:::<nul (set/p z=) > %_tempfile%

:__start_checkdir
set __pwd=%~dp0
set __cwd=%cd%
set __checked_directory=%~1
set __make_dir=%~2
set _tempfile="%__checked_directory%\__temp_file"
set _checkdir=0
echo.
echo Checking directory %__checked_directory%

::: make temporary file to check if file exists
copy nul %_tempfile% > nul 2>&1

::: __temp_file created in directory to check
if exist %_tempfile% goto _del_temp_file

::: __temp_file not created
if "%~2%"=="" (
    echo - directory %__checked_directory% doesn't exist. Skipping mkdir
    goto _end_make_dir
)
if "%~2%"=="0" (
    echo - directory %__checked_directory% doesn't exist. Skipping mkdir
    goto _end_make_dir
)

::: make directory which didn't exist in the first place
mkdir %__checked_directory%
echo - directory %__checked_directory% doesn't exist. Calling mkdir
set _checkdir=1
goto _end_make_dir

:_del_temp_file
echo - directory %__checked_directory% exists.
set _checkdir=1
del %_tempfile%

:_end_make_dir
endlocal & (
    set _checkdir=%_checkdir%
    if "%~3"=="" goto :eof
    set %~3=%_checkdir%
)
