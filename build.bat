@echo off
chcp 65001 > NUL

echo ===================================================
echo   AUTOMATIC MULTILINGUAL SPHINX BUILD (EN / VI)
echo ===================================================

:: Optional strict mode for translation step:
:: set POTRANSLATOR_REQUIRED=1 to stop build when potranslator fails.
:: set POTRANSLATOR_NO_PROXY=1 to run potranslator without HTTP(S)_PROXY variables.
if /I "%POTRANSLATOR_REQUIRED%"=="1" (
    set "TRANSLATION_STRICT=1"
) else (
    set "TRANSLATION_STRICT=0"
)

:: 1. Xác định thư mục nguồn
if exist "docs\source\conf.py" (
    set "SOURCE_DIR=docs\source"
    set "OUTPUT_DIR=docs\build\html"
    set "DOCS_DIR=docs"
) else if exist "docs\conf.py" (
    set "SOURCE_DIR=docs"
    set "OUTPUT_DIR=docs\_build\html"
    set "DOCS_DIR=docs"
) else (
    echo [ERROR] Could not find conf.py!
    pause
    exit /b 1
)

set "DOXYGEN_XML=docs\_build\doxygen\xml\index.xml"
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
:: 2. Trích xuất văn bản & cập nhật file translation
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

:: 3. Auto-translate PO files
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



:: 4. Build ra 2 giao diện HTML
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

:: Tạo index.html điều hướng mặc định
(
echo ^<html^>^<head^>^<meta http-equiv="refresh" content="0; url=en/index.html"^>^</head^>^</html^>
) > "%OUTPUT_DIR%/index.html"

echo.
echo ===================================================
echo   [SUCCESS] AUTO-TRANSLATION ^& BUILD COMPLETE!
echo   - English:    %OUTPUT_DIR%\en\index.html
echo   - Vietnamese: %OUTPUT_DIR%\vi\index.html
echo ===================================================
echo.

start "" "%OUTPUT_DIR%\en\index.html"
exit /b 0