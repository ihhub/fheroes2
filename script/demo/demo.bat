@echo off

if not exist "demo" mkdir "demo"

echo downloading demo version [1/3]
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip', 'demo\demo.zip')"

echo unpacking archive [2/3]

set sevenZipPath=

where 7z.exe >nul 2>nul
if %errorlevel% == 0 (
    set sevenZipPath=7z.exe
) else (
    if exist "%ProgramFiles%\7-Zip\7z.exe" (
        set sevenZipPath=%ProgramFiles%\7-Zip\7z.exe
    )
)

if not "%sevenZipPath%" == "" (
    cd demo
    "%sevenZipPath%" x demo.zip -aoa > nul

    echo copying files [3/3]
    
    if not exist "..\..\..\data" mkdir "..\..\..\data"
    if not exist "..\..\..\maps" mkdir "..\..\..\maps"
    xcopy /Y /s "DATA" "..\..\..\data"
    xcopy /Y /s "MAPS" "..\..\..\maps"

    del demo.zip

    cd ..
) else (
    echo 7z.exe is not found in path.
    echo Please unzip demo.zip file manually and copy DATA and MAPS folders into folder with the game.
    echo Alternatively, re-run script after installing 7-zip archiver at your local system's folder as- 'C:\Program Files\7-Zip\7z.exe
)

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
