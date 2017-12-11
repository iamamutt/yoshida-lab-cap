:: if defined _success_ (echo true) else (echo false)
@echo off

set __pwd=%~dp0
set __cwd=%cd%
set __search_key=%~1
set __search_directory=0
set __search_root="%~3"
set __search_recurse=0

if /I "%~1"=="" goto __exit_search
if /I "%~2"=="1" (set __search_directory=\nul) else (set __search_directory=)
if /I "%~3"=="" (set __search_root=".")
if /I "%~4"=="0" (set __search_recurse="0") else (set __search_recurse="1")

set "_success_="
cd "%__search_root%"
set __dir_info=
set __last_path=?
set __last_file=?
set __full_paths=.

set /a count=0
setlocal EnableDelayedExpansion
set null_d=\nul
::if %__search_directory%==%null_d% (echo searching for directory...) else (echo searching for file...)
if %__search_recurse%=="1" (echo searching recursive mode...) else (echo searching non-recursive mode...)
for /f "tokens=*" %Q in ('dir /b /s /a:d "%__search_key%"') do (
::for /D /r %%P in (.) do for %%Q in ("%%~fP\%__search_key%") do (
    set __searched_path=%%~fQ
    if not exist !__searched_path!!__search_directory! (
        echo failed: !__searched_path!
    ) else (
        echo [!count!] %%~ftzaQ
        set __last_path=!__searched_path!
        set __last_file=%%~nxQ
        set __full_paths=!__full_paths!,!__last_path!
        set /a count+=1
        if %__search_recurse%=="1" (
            if !count! gtr 0 (
                echo.
                echo breaking at iter !count!
                goto __end_loop
            )
        )
    )
)

:__end_loop

echo.

:__exit_search
cd "%__cwd%"

endlocal & (
    set "count=%count%"
    set "__last_path=%__last_path%"
    set "__last_file=%__last_file%"
)

if %count% equ 1 (
    echo Found one match.
    set "_success_=y"
) else (
    echo Found %count% matches.
)

if defined _success_ (echo Success = true) else (echo Success = false)
echo.
echo __last_path is %__last_path%
echo __last_file is %__last_file%
