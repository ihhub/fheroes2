###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2024                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

$ErrorActionPreference = "Stop"

$h2DemoURL = "https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
$h2DemoSHA256 = "12048C8B03875C81E69534A3813AAF6340975E77B762DC1B79A4FF5514240E3C"

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
                    Write-Host -ForegroundColor Yellow "WARNING: Neither the Get-FileHash cmdlet nor certutil.exe is supported on this system, the hash of the downloaded file cannot be verified"

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

    # Reserve space for the Invoke-WebRequest progress indicator
    for ($i = 0; $i -Le 7; $i++) {
        Write-Host ""
    }

    Write-Host -ForegroundColor Green (-Join("This script will download the demo version of the original Heroes of Might and Magic II`r`n", `
                                             "It may take a few minutes, please wait...`r`n"))

    Write-Host "[1/4] determining the destination directory"

    $destPath = $null

    if (Test-Path -Path "fheroes2.exe" -PathType Leaf) {
        $destPath = "."
    } elseif (Test-Path -Path "..\..\src" -PathType Container) {
        # Special hack for developers running this script from the source tree
        $destPath = "..\.."
    }

    try {
        if ($null -Eq $destPath) {
            throw
        }

        while ($true) {
            $randName = [System.IO.Path]::GetRandomFileName()

            if (-Not (Test-Path -Path "$destPath\$randName")) {
                [void](New-Item -Path "$destPath\$randName" -ItemType "directory")
                Remove-Item -Path "$destPath\$randName"

                break
            }
        }
    } catch {
        if ($null -Eq $Env:APPDATA) {
            Write-Host -ForegroundColor Red "FATAL ERROR: Unable to determine the destination directory"

            return
        }

        $destPath = "$Env:APPDATA\fheroes2"

        if (-Not (Test-Path -Path $destPath -PathType Container)) {
            [void](New-Item -Path $destPath -ItemType "directory")
        }
    }

    Write-Host -ForegroundColor Green (-Join("Destination directory: ", (Resolve-Path $destPath).Path))

    Write-Host "[2/4] downloading the demo version"

    $demoPath = "$destPath\demo"

    if (-Not (Test-Path -Path $demoPath -PathType Container)) {
        [void](New-Item -Path $demoPath -ItemType "directory")
    }

    Get-FileViaHTTP -URL $h2DemoURL -FilePath "$demoPath\h2demo.zip"

    $result = Get-SHA256HashForFile -Path "$demoPath\h2demo.zip"

    if (-Not ($result -Is [Boolean]) -And ($result -Ne $h2DemoSHA256)) {
        Write-Host -ForegroundColor Red (-Join("FATAL ERROR: Invalid hash for HoMM2 demo archive`r`n", `
                                               "Expected:`t$h2DemoSHA256`r`n", `
                                               "Got:`t`t$result`r`n", `
                                               "Installation aborted"))

        return
    }

    Write-Host "[3/4] unpacking archives"

    $shell = New-Object -ComObject "Shell.Application"

    $zip = $shell.NameSpace((Resolve-Path "$demoPath\h2demo.zip").Path)

    foreach ($item in $zip.Items()) {
        $shell.Namespace((Resolve-Path $demoPath).Path).CopyHere($item, 0x14)
    }

    Write-Host "[4/4] copying files"

    $dataPath = "$destPath\data"
    $mapsPath = "$destPath\maps"

    if (-Not (Test-Path -Path $dataPath -PathType Container)) {
        [void](New-Item -Path $dataPath -ItemType "directory")
    }
    if (-Not (Test-Path -Path $mapsPath -PathType Container)) {
        [void](New-Item -Path $mapsPath -ItemType "directory")
    }

    $data = $shell.NameSpace((Resolve-Path "$demoPath\DATA").Path)
    $maps = $shell.NameSpace((Resolve-Path "$demoPath\MAPS").Path)

    foreach ($item in $data.Items()) {
        $shell.Namespace((Resolve-Path $dataPath).Path).CopyHere($item, 0x14)
    }
    foreach ($item in $maps.Items()) {
        $shell.Namespace((Resolve-Path $mapsPath).Path).CopyHere($item, 0x14)
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true)
}
