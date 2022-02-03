@echo off
setlocal EnableDelayedExpansion

set LOCAL_PATH=%~dp0
set "FILE_N=-[%~n0]:"

set PLATFORM=x64
set BUILD_TYPE=Release

set CLEAN_BUILD=false
set BUILD_PATH=%LOCAL_PATH%..

:arg-parse
if not "%1"=="" (
    if "%1"=="--build-type" (
        set BUILD_TYPE=%2
        shift
    )

    if "%1"=="--build-path" (
        set BUILD_PATH=%2
        shift
    )

    if "%1"=="--platform" (
        set PLATFORM=%2
        shift
    )

    if "%1"=="--clean" (
        set CLEAN_BUILD=true
    )

    if "%1"=="--help" (
        goto help
    )

    if "%1"=="-h" (
        goto help
    )

    shift
    goto arg-parse
)

REM
REM Check if arguments are valid
REM
if %BUILD_TYPE% neq Release if %BUILD_TYPE% neq Debug (
    goto unknown_build_type
)

REM
REM Check if platform is valid
REM
if %PLATFORM% neq x64 if %PLATFORM% neq x86 (
    goto unknown_platform
)

REM
REM Create build directories
REM
set BIN_PATH=%BUILD_PATH%\build\bin
set OBJ_PATH=%BUILD_PATH%\build\obj

REM
REM Delete the build directory
REM
if %CLEAN_BUILD% == true if exist "%BUILD_PATH%\build" (
    echo Remove build directory: "%BUILD_PATH%\build"
    rmdir /s /q "%BUILD_PATH%\build"
    goto good_exit
)

REM
REM Set compiler flags in function of the build type
REM
set CompilerFlags= /nologo /MP /W4 /Oi /GR /wd4200 /Fo"%OBJ_PATH%\\" /Fe"%BIN_PATH%\\ls"
set LinkerFlags= /NOLOGO /SUBSYSTEM:CONSOLE /INCREMENTAL:NO

if %BUILD_TYPE% == Debug (
    set CompilerFlags= /Od /MTd /Z7 /GS /Gs /RTC1 !CompilerFlags! /Fd"%BIN_PATH%\\"
) else (
    set CompilerFlags= /WX /O2 /MT !CompilerFlags!
)

REM
REM Generate the build path and compile the project
REM
if not exist "%BIN_PATH%" MKDIR "%BIN_PATH%"
if not exist "%OBJ_PATH%" MKDIR "%OBJ_PATH%"

set LIST=
for %%x in ("%LOCAL_PATH%"..\source\*.c) do set LIST=!LIST! "%%x"

if %PLATFORM% == x64 (CALL "%LOCAL_PATH%\cl.bat" amd64) else (CALL "%LOCAL_PATH%\cl.bat" amd64_x86)
cl %LIST% %CompilerFlags% /link %LinkerFlags%

if %ERRORLEVEL% neq 0 (goto bad_exit)
echo Binary output (%BUILD_TYPE%): "%BIN_PATH%\ls"

goto good_exit
REM ============================================================================
REM -- Messages and Errors -----------------------------------------------------
REM ============================================================================

:help
    echo build.bat [--build-type=Release^|Debug] [--platform=x64^|x86] [--build-path=<output directory>]
    echo By default: --build-type=Release --platform=x64 --build-path=<project root>
    echo    --build-type    type of build, release or debug
    echo    --build-path    directory where binaries are generated
    echo    --platform      x64 for 64 bits or x86 for 32 bits
    echo    --clean         remove the previous build data
    echo    --help, -h      show help
    goto good_exit

:unknown_build_type
    echo.
    echo %FILE_N% [ERROR] Unknown build type
    echo %FILE_N% [INFO ] Allowed values are "Release" or "Debug"
    goto bad_exit

:unknown_platform
    echo.
    echo %FILE_N% [ERROR] Unknown platform
    echo %FILE_N% [INFO ] Allowed values are "x64" or "x86"
    goto bad_exit

:good_exit
    endlocal
    exit /b 0

:bad_exit
    endlocal
    exit /b %ERRORLEVEL%
