@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "ROOT_DIR=%~dp0"
set "SOURCE_DIR=%ROOT_DIR%SerialPrograms"
set "DEFAULT_PRESET=RelWithDebInfo"
set "PRESET=%DEFAULT_PRESET%"

if /I "%~1"=="-h" goto :usage
if /I "%~1"=="--help" goto :usage
if /I "%~1"=="/?" goto :usage

if not "%~1"=="" (
    set "PRESET=%~1"
    shift
)

set "CMAKE_ARGS="
:collect_args
if "%~1"=="" goto :args_done
set "CMAKE_ARGS=%CMAKE_ARGS% %1"
shift
goto :collect_args
:args_done

if not exist "%SOURCE_DIR%\CMakePresets.json" (
    echo ERROR: Could not find SerialPrograms\CMakePresets.json.
    echo Run this script from the Arduino-Source checkout.
    exit /b 1
)

call :find_cmake
if errorlevel 1 (
    echo ERROR: Native Windows CMake was not found.
    echo Install CMake and add it to PATH.
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo ERROR: Ninja was not found on PATH.
    echo Install Ninja and add it to PATH.
    exit /b 1
)

call :setup_msvc
if errorlevel 1 exit /b 1

pushd "%SOURCE_DIR%" || exit /b 1

echo.
echo === Configuring SerialPrograms (%PRESET%) ===
"%CMAKE_EXE%" --preset "%PRESET%"%CMAKE_ARGS%
if errorlevel 1 (
    echo.
    echo ERROR: CMake configure failed.
    popd
    exit /b 1
)

echo.
echo === Building SerialPrograms (%PRESET%) ===
if defined BUILD_JOBS (
    "%CMAKE_EXE%" --build --preset "%PRESET%" --parallel %BUILD_JOBS%
) else (
    "%CMAKE_EXE%" --build --preset "%PRESET%" --parallel
)
if errorlevel 1 (
    echo.
    echo ERROR: Build failed.
    popd
    exit /b 1
)

popd
echo.
echo Build completed successfully.
echo Output folder: "%ROOT_DIR%build\%PRESET%"
exit /b 0

:find_cmake
rem Prefer the native installer over MSYS2 CMake when both are installed.
if exist "%ProgramFiles%\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
    exit /b 0
)
if exist "%ProgramFiles(x86)%\CMake\bin\cmake.exe" (
    set "CMAKE_EXE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"
    exit /b 0
)

for /f "delims=" %%I in ('where cmake 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
if not defined CMAKE_EXE exit /b 1

echo "%CMAKE_EXE%" | findstr /I /C:"\msys2\" >nul
if not errorlevel 1 (
    set "CMAKE_EXE="
    exit /b 1
)
exit /b 0

:setup_msvc
where cl >nul 2>nul
if not errorlevel 1 exit /b 0

echo MSVC compiler environment not detected. Looking for Visual Studio 2022...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: cl.exe was not found, and vswhere.exe is not installed.
    echo Run this script from an x64 Native Tools Command Prompt for VS 2022.
    exit /b 1
)

set "VS_INSTALL="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%I"
if not defined VS_INSTALL (
    echo ERROR: Visual Studio 2022 C++ tools were not found.
    echo Install the Desktop development with C++ workload.
    exit /b 1
)

set "VSDEVCMD=%VS_INSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo ERROR: Could not find VsDevCmd.bat at "%VSDEVCMD%".
    exit /b 1
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64 -no_logo
where cl >nul 2>nul
if errorlevel 1 (
    echo ERROR: Visual Studio setup completed, but cl.exe is still unavailable.
    exit /b 1
)
exit /b 0

:usage
echo Build SerialPrograms with the repository's CMake presets.
echo.
echo Usage:
echo   Build.bat [preset] [extra CMake configure args]
echo.
echo Examples:
echo   Build.bat
echo   Build.bat Debug
echo   Build.bat Release -DPREFERRED_QT_DIR=C:/Qt -DPREFERRED_QT_VER=6.8.3
echo   set BUILD_JOBS=8 ^& Build.bat
echo.
echo Available presets are defined in SerialPrograms\CMakePresets.json:
echo   Debug, Release, RelWithDebInfo, MinSizeRel, Publish
exit /b 0
