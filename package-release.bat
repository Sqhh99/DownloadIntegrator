@echo off
setlocal enabledelayedexpansion

echo ===== DownloadIntegrator Packaging Tool =====

rem Define paths
set QT_DEPLOY_PATH=D:\Qt\6.6.0\msvc2019_64\bin\windeployqt6.exe
set EXE_PATH=build\Desktop_Qt_6_6_0_MSVC2019_64bit-Release\DownloadIntegrator.exe
set OUTPUT_DIR=release-package

rem Check if executable exists
if not exist "%EXE_PATH%" (
    echo Error: Could not find executable at %EXE_PATH%
    echo Please build the project in Release mode first.
    exit /b 1
)

rem Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

rem Copy executable to output directory
echo Copying executable to package directory...
copy "%EXE_PATH%" "%OUTPUT_DIR%\"

rem Run windeployqt with optimizations
echo Running windeployqt with optimizations...
"%QT_DEPLOY_PATH%" --release --no-translations --no-system-d3d-compiler --no-opengl-sw --no-virtualkeyboard --no-compiler-runtime "%OUTPUT_DIR%\DownloadIntegrator.exe"

rem Copy required application files
echo Copying additional application files...
if exist "translations" xcopy /E /I /Y "translations" "%OUTPUT_DIR%\translations"
if exist "modify.txt" copy "modify.txt" "%OUTPUT_DIR%\"
if exist "README.md" copy "README.md" "%OUTPUT_DIR%\"
if exist "LICENSE" copy "LICENSE" "%OUTPUT_DIR%\"

echo.
echo Packaging complete! Files are in the "%OUTPUT_DIR%" directory.
echo.

pause 