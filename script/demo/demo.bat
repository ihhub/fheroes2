@echo off

if not exist "demo" mkdir "demo"

echo downloading demo version ...
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip', 'demo\demo.zip')"
echo done

where 7z.exe >nul 2>nul
IF NOT ERRORLEVEL 0 (
    @echo 7z.exe not found in path. Please unzip files manually.
    exit 1
)

cd demo
7z x demo.zip > nul

xcopy /Y /s "DATA" "..\..\.."
xcopy /Y /s "MAPS" "..\..\.."
