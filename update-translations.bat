@echo off
echo Updating translation files...

:: Update paths to your Qt installation if needed
set QT_DIR=d:\Qt\6.6.0\msvc2019_64\bin

:: Extract translatable strings from source code
echo Extracting strings from source code...
"%QT_DIR%\lupdate.exe" -recursive src -ts translations/downloadintegrator_en_US.ts translations/downloadintegrator_ja_JP.ts

echo.
echo Now edit the translation files with Qt Linguist:
echo %QT_DIR%\linguist.exe translations/downloadintegrator_en_US.ts
echo %QT_DIR%\linguist.exe translations/downloadintegrator_ja_JP.ts
echo.

:: Wait for user to confirm they've edited translations
set /p confirm=Have you finished editing translations? (y/n): 

if /i "%confirm%"=="y" (
    :: Compile translation files to binary format
    echo Compiling translation files...
    "%QT_DIR%\lrelease.exe" translations/downloadintegrator_en_US.ts translations/downloadintegrator_ja_JP.ts
    
    :: Copy .qm files to resources directory
    echo Copying translation files to resources directory...
    copy translations\*.qm resources\translations\
    
    echo Done! Remember to rebuild your application.
) else (
    echo Update canceled. No files were compiled or copied.
)

pause 