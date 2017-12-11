::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@echo off
set __pwd=%~dp0
set __cwd=%cd%
set __src_file_url=%~1
set __src_download_dir=%~2
set __src_file_name=%~3
set _FAILED=0

echo.
echo File download script...
echo.

if "%~2"=="" (
    echo No download directory specified.
    set __src_download_dir=__tempdir
)

:: make download location if not existing
set dir_exists=0
call "%__pwd%checkdir.bat" %__src_download_dir% 1 dir_exists > nul

if %dir_exists%==0 (
    echo Directory %__src_download_dir% could not be made.
    set _FAILED=1
    goto __end_download_pkg
)

cd /d %__src_download_dir%

if "%~1"=="" (
    echo No URL specified.
    set _FAILED=1
    goto __end_download_pkg
)

if "%~3"=="" (
    set __src_file_name="_temp_dl__"
)

if exist %__src_file_name% (
    echo - %__src_file_name% already exists. Skipping download.
    goto __end_download_pkg
)

echo - downloading %__src_file_url% to %__src_download_dir%\%__src_file_name%
echo - please wait...
echo - ...
echo - ..

powershell.exe -nologo -noprofile -command (new-object System.Net.WebClient).DownloadFile('%__src_file_url%','%__src_file_name%')

if not exist %__src_file_name% (
    set _FAILED=1
    echo - download %__src_file_name% failed!
)


:__end_download_pkg
cd /d "%__cwd%"
echo.
