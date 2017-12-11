::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@setlocal enabledelayedexpansion
@echo off
goto start_script
::: call unzipper.bat RtAudio.tar.gz RtAudio rtaudio-* CmakeList.txt C:\Program Files\7-Zip\7z.exe
::: dir /b /s /a:d "%var%"
:start_script
echo.
echo Unzipper script...
echo.
set __pwd=%~dp0
set __cwd=%cd%

::: location to zip file
set __zip=%~1

::: path where to extract
set __extract_subdir=%~2

::: search extracted for some file at this location
set __search_subdir=%~3

::: search for this file at __search_subdir
set __search_file=%~4

::: path to 7zip exe
set __7z_exe=%~5

::: output of where found src code is containing __search_file
set zip_subfolder=%__search_subdir%

::: error code
set _FAILED=0

::: try unzip counter
set /a counter=0

::: ------------------------------------------------------------------------------------------------
if "%~5"=="" (set __7z_exe=C:\Program Files\7-Zip\7z.exe)
if not exist "%__7z_exe%" (
    echo program 7zip not found at location: %__7z_exe%
    set _FAILED=1
    goto __end_unzipper
)

if not exist "%__zip%" (
    echo zip file %__zip% not found
    set _FAILED=1
    goto __end_unzipper
)

call "%__pwd%fileparts.bat" "%__zip%" dp __fpath > nul
echo - zip file parent at %__fpath%

call "%__pwd%fileparts.bat" "%__zip%" x __ext > nul

set __extract_path=%__extract_subdir%
if not "%__extract_subdir%"=="" goto _skip_find_stem

::: find file name part only from zip file
call "%__pwd%fileparts.bat" "%__zip%" n __extract_subdir > nul
if "%__ext%"==".gz" (
    call "%__pwd%fileparts.bat" "%__extract_subdir%" n __extract_subdir > nul
)
set __extract_path=%__fpath%%__extract_subdir%

::: ------------------------------------------------------------------------------------------------
:_skip_find_stem
echo - zip extention is %__ext%
echo - extraction folder is %__extract_path%

call "%__pwd%checkdir.bat" "%__extract_path%" 0 __extract_subdir_exists > nul
if %__extract_subdir_exists%==1 (
    echo - zip file seems to be unzipped already at %__extract_path%
    goto __no_unzip
)

::: ------------------------------------------------------------------------------------------------
:__do_unzip
cd %__cwd%
echo - unzipping %__zip% to %__extract_path%
"%__7z_exe%" x "%__zip%" -aoa -o"%__extract_path%"
if not "%__ext%"==".gz" goto __no_unzip

::: unzip .tar
call "%__pwd%fileparts.bat" "%__zip%" n __extract_short > nul
call "%__pwd%fileparts.bat" "%__extract_short%" n __extract_short > nul
set __tarfile=%__extract_path%\%__extract_short%.tar
echo - 2nd pass for %__tarfile%
"%__7z_exe%" x "%__tarfile%" -aoa -o"%__extract_path%"
del "%__tarfile%"

::: ------------------------------------------------------------------------------------------------
:__no_unzip
cd /d %__extract_path%

::: Check for subfolder and set to variable if found
if "%__search_subdir%"=="" goto __end_unzipper

set __search_cmd="dir /b /a:d "%__search_subdir%""
for /f "tokens=*" %%i in ('%__search_cmd%') do (
    call :searchSubroute %%~f%i
)
goto __check_file_in_sub

:searchSubroute:
    setlocal EnableExtensions EnableDelayedExpansion
    set part=%~1
    endlocal & set zip_subfolder=%part%
    exit /b

:__check_file_in_sub

echo - searching subfolder %zip_subfolder% for file %__search_file%
if not exist "%zip_subfolder%\%__search_file%" (
    set /a counter+=1
    if %counter% lss 2 goto __do_unzip
    echo - FAILED to find file %__search_file%!
    set _FAILED=1
) else (
    echo - file %__search_file% was found.
    set _FAILED=0
)

::: ------------------------------------------------------------------------------------------------
:__end_unzipper

if "%~3"=="" (
    goto :eof
)

endlocal & (
    set zip_subfolder=%zip_subfolder%
    set _FAILED=%_FAILED%
    cd /d %__cwd%
)
