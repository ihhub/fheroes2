@echo off

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip','demo.zip')"

for /f usebackq %%g in (
`powershell -Command $PSVersionTable.PSVersion.Major`) do (
if %%g lss 4 (
powershell -Command "(New-Object Net.WebClient).DownloadFile('https://www.willus.com/archive/zip64/unzip.exe','unzip.exe')"
unzip -o -qq -L demo.zip data/* maps/*
del unzip.exe
) else (
powershell -command "Expand-Archive -Force 'demo.zip'"
xcopy /y /s "demo\DATA" "data\" >nul
xcopy /y /s "demo\MAPS" "maps\" >nul
rd /S /Q demo
)
)

del demo.zip

pause
