@echo off

set fh2Path=
set dev=

if not exist "fheroes2.exe" (
    Rem this is not fheroes2 related directory. Let's see if it's for developer
    if exist "..\..\src" (
        set fh2Path=..\..\
        set dev=y
    )
)


echo [1/3] downloading demo version

powershell -Command "(New-Object Net.WebClient).DownloadFile('https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip', '%fh2Path%demo.zip')"

echo [2/3] unpacking archive

if not exist "%fh2Path%demo" mkdir %fh2Path%demo
powershell -Command "$shell = new-object -com shell.application; $zip = $shell.NameSpace((Resolve-Path '%fh2Path%demo.zip').Path); foreach($item in $zip.items()) { $shell.Namespace((Resolve-Path '%fh2Path%demo').Path).copyhere($item, 0x14) }"

echo [3/3] copying files

if not exist "%fh2Path%data" mkdir "%fh2Path%data"
if not exist "%fh2Path%maps" mkdir "%fh2Path%maps"
xcopy /Y /s "%fh2Path%demo\DATA" "%fh2Path%data" >nul
xcopy /Y /s "%fh2Path%demo\MAPS" "%fh2Path%maps" >nul

if defined dev (
    powershell -Command "(New-Object Net.WebClient).DownloadFile('https://wikidll.com/download/25503/wing32.zip', '%fh2Path%wing32.zip')"
    powershell -Command "$shell = new-object -com shell.application; $zip = $shell.NameSpace((Resolve-Path '%fh2Path%wing32.zip').Path); foreach($item in $zip.items()) { $shell.Namespace((Resolve-Path '%fh2Path%demo').Path).copyhere($item, 0x14) }"
    del %fh2Path%wing32.zip
) else (
    rmdir /s /q %fh2Path%demo
)
del %fh2Path%demo.zip

if not "%APPVEYOR_REPO_PROVIDER%" == "gitHub" (
    echo Press any key to exit...
    pause >nul
)
