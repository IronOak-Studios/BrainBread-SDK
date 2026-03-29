@echo off
setlocal enabledelayedexpansion

:: increment_build.bat - Auto-increments the build number in build.h
:: Usage: increment_build.bat "path\to\build.h"

set "BUILDFILE=%~1"
if "%BUILDFILE%"=="" (
    echo ERROR: No build.h path specified.
    exit /b 1
)

if not exist "%BUILDFILE%" (
    echo ERROR: File not found: %BUILDFILE%
    exit /b 1
)

:: Read the current BUILD_NR value from build.h
set "CURRENT_NR="
for /f "tokens=3" %%a in ('findstr /C:"#define BUILD_NR" "%BUILDFILE%"') do (
    set "CURRENT_NR=%%a"
)

if "%CURRENT_NR%"=="" (
    echo ERROR: Could not find BUILD_NR in %BUILDFILE%
    exit /b 1
)

:: Increment the build number
set /a NEW_NR=%CURRENT_NR%+1

:: Rewrite build.h with the new values (LF line endings)
powershell -NoProfile -Command "[System.IO.File]::WriteAllText('%BUILDFILE%', \"#define BUILD    `\"Build %NEW_NR%`\"`n#define BUILD_NR %NEW_NR%`n\")"

echo Build number incremented: %CURRENT_NR% -^> %NEW_NR%
endlocal
