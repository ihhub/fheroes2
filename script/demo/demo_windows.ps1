$ErrorActionPreference = "Stop"

$h2DemoURL = "https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
$h2DemoSHA256 = "12048C8B03875C81E69534A3813AAF6340975E77B762DC1B79A4FF5514240E3C"

$wing32URL = "https://wikidll.com/download/25503/wing32.zip"
$wing32SHA256 = "0CD89F09C66F53F30782858DF5453F6AC4C8A6D482F558E4FDF24C26E0A05A49"

try {
    $shell = New-Object -com "shell.application"
    $webClient = New-Object Net.WebClient

    $fheroes2Path = ""

    if (-Not (Test-Path -Path "fheroes2.exe" -PathType Leaf) -And (Test-Path -Path "..\..\src" -PathType Container)) {
        $fheroes2Path = "..\..\"
    }

    if (-Not (Test-Path -Path "demo" -PathType Container)) {
        [void](New-Item -Path "demo" -ItemType "directory")
    }

    Write-Host "[1/4] downloading demo version"

    $webClient.DownloadFile($h2DemoURL, "demo\demo.zip")

    try {
        $hash = (Get-FileHash -Path "demo\demo.zip" -Algorithm SHA256).Hash

        if ($hash -Ne $h2DemoSHA256) {
            Write-Host -ForegroundColor Red (-Join("FATAL ERROR: Invalid hash for HoMM2 demo archive`r`n", `
                                                   "Expected:`t$h2DemoSHA256`r`n", `
                                                   "Got:`t`t$hash`r`n", `
                                                   "Installation aborted"))

            return
        }
    } catch [System.Management.Automation.CommandNotFoundException] {
        if ($_.Exception.CommandName -Eq "Get-FileHash") {
            Write-Host -ForegroundColor Yellow "WARNING: Get-FileHash cmdlet is not supported on this system, hash of HoMM2 demo archive cannot be verified"
        } else {
            throw
        }
    }

    Write-Host "[2/4] downloading wing32.dll library"

    $webClient.DownloadFile($wing32URL, "demo\wing32.zip")

    try {
        $hash = (Get-FileHash -Path "demo\wing32.zip" -Algorithm SHA256).Hash

        if ($hash -Ne $wing32SHA256) {
            Write-Host -ForegroundColor Red (-Join("FATAL ERROR: Invalid hash for wing32.dll archive`r`n", `
                                                   "Expected:`t$wing32SHA256`r`n", `
                                                   "Got:`t`t$hash`r`n", `
                                                   "Installation aborted"))

            return
        }
    } catch [System.Management.Automation.CommandNotFoundException] {
        if ($_.Exception.CommandName -Eq "Get-FileHash") {
            Write-Host -ForegroundColor Yellow "WARNING: Get-FileHash cmdlet is not supported on this system, hash of wing32.dll archive cannot be verified"
        } else {
            throw
        }
    }

    Write-Host "[3/4] unpacking archives"

    $zip = $shell.NameSpace((Resolve-Path "demo\demo.zip").Path)

    foreach ($item in $zip.items()) {
        $shell.Namespace((Resolve-Path "demo").Path).CopyHere($item, 0x14)
    }

    $zip = $shell.NameSpace((Resolve-Path "demo\wing32.zip").Path)

    foreach ($item in $zip.items()) {
        $shell.Namespace((Resolve-Path "demo").Path).CopyHere($item, 0x14)
    }

    Write-Host "[4/4] copying files"

    $dataPath = (-Join($fheroes2Path, "data"))
    $mapsPath = (-Join($fheroes2Path, "maps"))

    if (-Not (Test-Path -Path $dataPath -PathType Container)) {
        [void](New-Item -Path $dataPath -ItemType "directory")
    }
    if (-Not (Test-Path -Path $mapsPath -PathType Container)) {
        [void](New-Item -Path $mapsPath -ItemType "directory")
    }

    $data = $shell.NameSpace((Resolve-Path "demo\DATA").Path)
    $maps = $shell.NameSpace((Resolve-Path "demo\MAPS").Path)

    foreach ($item in $data.items()) {
        $shell.Namespace((Resolve-Path $dataPath).Path).CopyHere($item, 0x14)
    }
    foreach ($item in $maps.items()) {
        $shell.Namespace((Resolve-Path $mapsPath).Path).CopyHere($item, 0x14)
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", $_))
} finally {
    if ($Env:APPVEYOR_REPO_PROVIDER -Ne "gitHub") {
        Write-Host "Press any key to exit..."

        [void][System.Console]::ReadKey($true);
    }
}
