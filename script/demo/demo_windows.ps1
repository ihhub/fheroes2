$ErrorActionPreference = "Stop"

$h2DemoURL = "https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
$h2DemoSHA256 = "12048C8B03875C81E69534A3813AAF6340975E77B762DC1B79A4FF5514240E3C"

$wing32URL = "https://wikidll.com/download/25503/wing32.zip"
$wing32SHA256 = "0CD89F09C66F53F30782858DF5453F6AC4C8A6D482F558E4FDF24C26E0A05A49"

try {
    function Get-FileViaHTTP {
        param (
            [string]$URL,
            [string]$FilePath
        )

        try {
            Invoke-WebRequest -Uri $URL -OutFile $FilePath
        } catch [System.Management.Automation.CommandNotFoundException] {
            if ($_.Exception.CommandName -Eq "Invoke-WebRequest") {
                $webClient = New-Object System.Net.WebClient

                $webClient.DownloadFile($URL, $FilePath)
            } else {
                throw
            }
        }
    }

    function Get-SHA256HashForFile {
        param (
            [string]$Path
        )

        try {
            return (Get-FileHash -Path $Path -Algorithm SHA256).Hash
        } catch [System.Management.Automation.CommandNotFoundException] {
            if ($_.Exception.CommandName -Eq "Get-FileHash") {
                try {
                    $output = certutil.exe -hashfile $Path sha256 2>&1

                    return ($output | Select-String -Pattern "^[0-9A-Fa-f]{64}$").Line.ToUpper()
                } catch {
                    Write-Host -ForegroundColor Yellow "WARNING: Neither Get-FileHash cmdlet nor certutil.exe are supported on this system, hash of downloaded file cannot be verified"

                    if ($null -Ne $output) {
                        Write-Host -ForegroundColor Yellow (-Join("certutil.exe output: ", ($output | Out-String)))
                    }

                    return $false
                }
            } else {
                throw
            }
        }
    }

    $fheroes2Path = ""

    if (-Not (Test-Path -Path "fheroes2.exe" -PathType Leaf) -And (Test-Path -Path "..\..\src" -PathType Container)) {
        $fheroes2Path = "..\..\"
    }

    if (-Not (Test-Path -Path "demo" -PathType Container)) {
        [void](New-Item -Path "demo" -ItemType "directory")
    }

    # Reserve space for the Invoke-WebRequest progress indicator
    for ($i = 0; $i -Le 7; $i++) {
        Write-Host ""
    }

    Write-Host -ForegroundColor Green (-Join("This script will download the demo version of the original Heroes of Might and Magic II`r`n", `
                                             "It may take a few minutes, please wait...`r`n"))

    Write-Host "[1/4] downloading demo version"

    Get-FileViaHTTP -URL $h2DemoURL -FilePath "demo\h2demo.zip"

    $result = Get-SHA256HashForFile -Path "demo\h2demo.zip"

    if (-Not ($result -Is [Boolean]) -And ($result -Ne $h2DemoSHA256)) {
        Write-Host -ForegroundColor Red (-Join("FATAL ERROR: Invalid hash for HoMM2 demo archive`r`n", `
                                               "Expected:`t$h2DemoSHA256`r`n", `
                                               "Got:`t`t$result`r`n", `
                                               "Installation aborted"))

        return
    }

    Write-Host "[2/4] downloading wing32.dll library"

    Get-FileViaHTTP -URL $wing32URL -FilePath "demo\wing32.zip"

    $result = Get-SHA256HashForFile -Path "demo\wing32.zip"

    if (-Not ($result -Is [Boolean]) -And ($result -Ne $wing32SHA256)) {
        Write-Host -ForegroundColor Red (-Join("FATAL ERROR: Invalid hash for wing32.dll archive`r`n", `
                                               "Expected:`t$wing32SHA256`r`n", `
                                               "Got:`t`t$result`r`n", `
                                               "Installation aborted"))

        return
    }

    Write-Host "[3/4] unpacking archives"

    $shell = New-Object -ComObject "Shell.Application"

    $zip = $shell.NameSpace((Resolve-Path "demo\h2demo.zip").Path)

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
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true);
}
