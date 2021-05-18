@echo off

if not exist "demo" mkdir "demo"

echo [1/3] downloading demo version
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip', 'demo\demo.zip')"

echo [2/3] unpacking archive
powershell -command "Expand-Archive -Force 'demo\demo.zip' 'demo'"

echo [3/3] copying files
if not exist "data" mkdir "data"
if not exist "maps" mkdir "maps"
xcopy /y /s "demo\DATA" "data" >nul
xcopy /y /s "demo\MAPS" "maps" >nul

rd /S /Q demo

echo Press any key to exit...
pause >nul
