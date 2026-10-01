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
) 
if /i "%TARGET%"=="native" (
    set BUILD_DIR=build\pc
)
if /i "%TARGET%"=="switch" (
    set BUILD_DIR=build\switch
)

REM -------------------------
REM Clean
REM -------------------------
if %CLEAN%==1 (
    echo Cleaning %BUILD_DIR%...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

    REM Compiled scenes live in the project root, not in %BUILD_DIR%, so a build
    REM dir wipe won't touch them. Remove stale .scnb here: their binary layout
    REM depends on MAX_WORLD_OBJECTS, so an outdated .scnb makes the engine fail
    REM to read the scene. They are regenerated from the .scn files by the build.
    echo Cleaning compiled scenes ^(*.scnb^)...
    del /q "*.scnb" 2>nul
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
) 

REM -------------------------
REM Desktop build
REM -------------------------
if /i "%TARGET%"=="native" (
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
        REM The sandbox example is built via yume_add_project, which drops the
        REM executable and its runtime assets in the project's build subdir. The
        REM exe's working dir must be that folder so relative asset paths
        REM (scene.scnb, shaders, cube.obj, ...) resolve.
        pushd "%BUILD_DIR%\examples\sandbox"
        sandbox.exe
        popd
    ) else (
        echo Build completed.
    )
)

REM -------------------------
REM Switch build
REM -------------------------
if /i "%TARGET%"=="switch" goto build_switch
goto after_switch

:build_switch
echo Building for switch...

REM The Switch toolchain (aarch64-none-elf-gcc, elf2nro, make, ...) lives in the
REM devkitPro MSYS2 environment, not in the Windows PATH. So the build runs
REM inside a devkitPro bash login shell. DEVKITPRO_BIN points at that MSYS2's
REM usr\bin (where bash.exe / cygpath.exe live).
if not defined DEVKITPRO_BIN (
    echo DEVKITPRO_BIN is not set. Point it at devkitPro's msys2\usr\bin ^(e.g. c:\devkitPro\msys2\usr\bin^).
    exit /b 1
)

REM Host shader tools (Windows .exe's) that generate the .nxs GLSL. They run on
REM the host during the build, not on the Switch. Override by setting HOST_DXC /
REM HOST_SPIRV_CROSS before running build.bat.
if not defined HOST_DXC set "HOST_DXC=C:\Portable\dxc\bin\x64\dxc.exe"
if not defined HOST_SPIRV_CROSS set "HOST_SPIRV_CROSS=C:\Portable\spirv-cross\bin\spirv-cross.exe"

REM The devkitPro toolchain (dkp-initialize-path.cmake) refuses to run unless it
REM is driven by the CMake installed inside MSYS2. The Windows CMake on PATH
REM (C:\Portable\...) trips that check, so invoke the MSYS2 cmake by absolute
REM path instead of relying on PATH order (the login .bashrc reorders it).
set "MSYS2_CMAKE_WIN=%DEVKITPRO_BIN%\cmake.exe"
if not exist "%MSYS2_CMAKE_WIN%" (
    echo MSYS2 cmake not found at "%MSYS2_CMAKE_WIN%". Install it with: pacman -S cmake
    exit /b 1
)

REM Pass everything to bash via the environment. The actual build logic lives
REM in build_switch.sh so the bash invocation carries no shell operators (; &&)
REM or nested quotes for cmd.exe to mangle -- cmd does NOT treat single quotes
REM as a string delimiter, so an inline one-liner would get chopped at && / ;.
set "HOST_DXC_WIN=%HOST_DXC%"
set "HOST_SPIRV_CROSS_WIN=%HOST_SPIRV_CROSS%"
set "PROJECT_DIR_WIN=%CD%"

"%DEVKITPRO_BIN%\bash.exe" -l "%CD%\build_switch.sh"
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)
echo Build completed.

:after_switch

