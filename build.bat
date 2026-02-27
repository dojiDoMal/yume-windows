@echo off
setlocal enabledelayedexpansion

REM Defaults
set TARGET=native
set CLEAN=0
set RUN=0

REM -------------------------
REM Parse arguments
REM -------------------------
:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="-c" set CLEAN=1
if /i "%~1"=="--clean" set CLEAN=1
if /i "%~1"=="-r" set RUN=1
if /i "%~1"=="--run" set RUN=1
if /i "%~1"=="-t" set TARGET=%~2& shift
if /i "%~1"=="--target" set TARGET=%~2& shift
shift
goto parse_args
:end_parse

REM -------------------------
REM Build dir
REM -------------------------
if /i "%TARGET%"=="web" (
    set BUILD_DIR=build\web
) else (
    set BUILD_DIR=build\pc
)

REM -------------------------
REM Clean
REM -------------------------
if %CLEAN%==1 (
    echo Cleaning %BUILD_DIR%...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM -------------------------
REM Web build
REM -------------------------
if /i "%TARGET%"=="web" (
    echo Building for WebGL with CMake preset...
    call emcmake cmake --preset web
    if errorlevel 1 exit /b 1
    cmake --build "%BUILD_DIR%"
    if errorlevel 1 exit /b 1

    if %RUN%==1 (
        echo Running Web build on http://localhost:8000
        cd "%BUILD_DIR%"
        python -m http.server 8000
    ) else (
        echo Build completed.
        echo To run: python -m http.server 8000 -d %BUILD_DIR%
    )
) else (
    REM -------------------------
    REM Native build
    REM -------------------------
    echo Building for native...

    if not exist "%BUILD_DIR%\CMakeCache.txt" (
        set CC=gcc
        set CXX=g++
        cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=C:/Portable/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -B "%BUILD_DIR%"
        if errorlevel 1 (
            echo Build configuration failed!
            exit /b 1
        )
    )

    cmake --build "%BUILD_DIR%"
    if errorlevel 1 (
        echo Build failed!
        exit /b 1
    )

    if %RUN%==1 (
        echo Running native build...
        "%BUILD_DIR%\main.exe"
    ) else (
        echo Build completed.
    )
)
