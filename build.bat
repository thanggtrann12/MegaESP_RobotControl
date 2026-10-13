@echo off
chcp 65001 > NUL

echo ===================================================
echo   AUTOMATIC MULTILINGUAL SPHINX BUILD (EN / VI)
echo ===================================================

set "IS_RELEASE=1"
if /I "%~1"=="local" set "IS_RELEASE=0"

if "%IS_RELEASE%"=="1" (
    echo [+] Mode: RELEASE BUILD - Output: docs\release
) else (
    echo [+] Mode: LOCAL BUILD - no release packaging
)

if exist "doc\source\conf.py" (
    set "SOURCE_DIR=doc\source"
    set "OUTPUT_DIR=doc\_build\html"
    set "DOCS_DIR=doc"
) else (
    echo [ERROR] Could not find conf.py!
    pause
    exit /b 1
)

set "RELEASE_DIR=%DOCS_DIR%\release"
set "DOXYGEN_XML=doc\_build\doxygen\xml\index.xml"

where doxygen >NUL 2>&1
if errorlevel 1 (
    echo [WARN] Doxygen not found in PATH. API page will show placeholder.
) else (
    echo [+] Running Doxygen to extract C++ comments...
    doxygen Doxyfile
    if errorlevel 1 (
        echo [WARN] Doxygen reported warnings/errors. API output may be incomplete.
    )
)

if exist "%DOXYGEN_XML%" (
    echo [OK] Doxygen XML found: %DOXYGEN_XML%
) else (
    echo [WARN] Missing Doxygen XML: %DOXYGEN_XML%
)

echo [+] Step 1/4: Extracting translatable strings...
sphinx-build -b gettext "%SOURCE_DIR%" "%OUTPUT_DIR%/gettext"

if errorlevel 1 (
    echo [ERROR] gettext extraction failed!
    pause
    exit /b 1
)

echo [+] Step 2/4: Updating Vietnamese translation files...
sphinx-intl update -p "%OUTPUT_DIR%/gettext" -l vi -d "%DOCS_DIR%\locales"

if errorlevel 1 (
    echo [ERROR] Translation update failed!
    pause
    exit /b 1
)

echo [+] Step 3/4: Auto-translating PO files to Vietnamese...

set "POTRANSLATOR_LOG=%OUTPUT_DIR%\potranslator_error.log"

if exist "%POTRANSLATOR_LOG%" del /q "%POTRANSLATOR_LOG%"

if /I "%POTRANSLATOR_NO_PROXY%"=="1" (
    set "HTTP_PROXY="
    set "HTTPS_PROXY="
    set "ALL_PROXY="
    set "http_proxy="
    set "https_proxy="
    set "all_proxy="
)

potranslator update -p "%OUTPUT_DIR%\gettext" -l vi -d "%DOCS_DIR%\locales" > "%POTRANSLATOR_LOG%" 2>&1

if errorlevel 1 (
    if "%TRANSLATION_STRICT%"=="1" (
        echo [ERROR] potranslator failed.
        echo [ERROR] See: %POTRANSLATOR_LOG%
        pause
        exit /b 1
    ) else (
        echo [WARN] potranslator failed.
        echo [WARN] See: %POTRANSLATOR_LOG%
    )
) else (
    echo [OK] potranslator completed.
)

echo [+] Step 4/4: Building HTML outputs...

echo     - Building English version...
sphinx-build -E -a -b html "%SOURCE_DIR%" "%OUTPUT_DIR%/en" -D language=en

if errorlevel 1 (
    echo [ERROR] English build failed!
    pause
    exit /b 1
)

echo     - Building Vietnamese version...
sphinx-build -E -b html "%SOURCE_DIR%" "%OUTPUT_DIR%/vi" -D language=vi

if errorlevel 1 (
    echo [ERROR] Vietnamese build failed!
    pause
    exit /b 1
)

(
echo ^<html^>^<head^>^<meta http-equiv="refresh" content="0; url=en/index.html"^>^</head^>^</html^>
) > "%OUTPUT_DIR%/index.html"

if "%IS_RELEASE%"=="1" (
    call :release
    if errorlevel 1 (
        echo [ERROR] Release packaging failed!
        pause
        exit /b 1
    )
)

echo.
echo ===================================================
echo   [SUCCESS] AUTO-TRANSLATION ^& BUILD COMPLETE!
echo   - English:    %OUTPUT_DIR%\en\index.html
echo   - Vietnamese: %OUTPUT_DIR%\vi\index.html
if "%IS_RELEASE%"=="1" echo   - Release:    %RELEASE_DIR%\index.html
echo ===================================================
echo.

if "%IS_RELEASE%"=="1" (
    start "" "%RELEASE_DIR%\en\index.html"
) else (
    start "" "%OUTPUT_DIR%\en\index.html"
)

exit /b 0

:release
echo [+] Packaging into release folder...
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"
xcopy /e /i /y "%OUTPUT_DIR%" "%RELEASE_DIR%" > NUL
if errorlevel 1 exit /b 1

echo [OK] Release folder ready at: %RELEASE_DIR%
exit /b 0