::: ------------------------------------------------------------------------------------------------
::: Joseph M. Burling <josephburling@gmail.com> 2017
::: ------------------------------------------------------------------------------------------------
@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd "%~dp0"
set _input=%~1
set _args=%~2
goto _batch_start
:: %~I         - expands %I removing any surrounding quotes (")
:: %~fI        - expands %I to a fully qualified path name
:: %~dI        - expands %I to a drive letter only
:: %~pI        - expands %I to a path only
:: %~nI        - expands %I to a file name only
:: %~xI        - expands %I to a file extension only
:: %~sI        - expanded path contains short names only
:: %~aI        - expands %I to file attributes of file
:: %~tI        - expands %I to date/time of file
:: %~zI        - expands %I to size of file
:: %~$PATH:I   - searches the directories listed in the PATH
::                environment variable and expands %I to the
::                fully qualified name of the first one found.
::                If the environment variable name is not
::                defined or the file is not found by the
::                search, then this modifier expands to the
::                empty string

:_batch_start
if "%_args%"=="" (set _args=nx)
set _fileparts=nul
for %%i in (%_input%) do (
    call :filePartsProcess %%~%_args%%i
)
goto __return


:filePartsProcess:
    setlocal EnableExtensions EnableDelayedExpansion
    set part=%~1
    endlocal & set _fileparts=%part%
    exit /b

:__return

if "%~3"=="" (
    echo _fileparts=%_fileparts%
    goto :eof
) else (
    echo file part is %_fileparts%
    endlocal & (
        set _fileparts=%_fileparts%
        set %~3=%_fileparts%
        )
)
