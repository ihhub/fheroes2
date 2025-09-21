###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2023                                             #
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

try {
    function Test-HoMM2DirectoryPath {
        param (
            [string]$Path
        )

        if ((Test-Path -Path "$Path\DATA\HEROES2.AGG" -PathType Leaf) -And
            (Test-Path -Path "$Path\MAPS" -PathType Container)) {
            return $true
        }

        return $false
    }

    Write-Host -ForegroundColor Green "This script will extract and copy game resources from the original distribution of Heroes of Might and Magic II`r`n"

    Write-Host "[1/3] determining the destination directory"

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

    Write-Host "[2/3] determining the HoMM2 directory"

    $homm2Path = $null
    # Some legacy HoMM2 installation use a CD drive
    $homm2CD = $null

    foreach ($key in @("HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_CURRENT_USER\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
                       # Legacy HoMM2 installation
                       "HKEY_LOCAL_MACHINE\SOFTWARE\New World Computing\Heroes of Might and Magic 2",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\New World Computing\Heroes of Might and Magic 2")) {
        if (-Not (Test-Path -Path "Microsoft.PowerShell.Core\Registry::$key" -PathType Container)) {
            continue
        }

        foreach ($subkey in (Get-ChildItem -Path "Microsoft.PowerShell.Core\Registry::$key")) {
            $path = $subkey.GetValue("InstallLocation")

            if ($null -Eq $path) {
                # From HKLM\SOFTWARE\New World Computing\Heroes of Might and Magic 2
            	$path = $subkey.GetValue("AppPath")
            }

            if ($null -Ne $path) {
                $path = $path.TrimEnd("\")

                if (Test-HoMM2DirectoryPath -Path $path) {
                    $homm2Path = $path

                    # From HKLM\SOFTWARE\New World Computing\Heroes of Might and Magic 2
                    $cdDrive = $subkey.GetValue("CDDrive")

                    if ($null -Ne $cdDrive) {
                        # If CDDrive is not a regular disk name (e.g. F:), then it is some path (absolute or relative), we need to try to resolve it
                        if ($cdDrive -notmatch '^[A-Za-z]:$') {
                            $currentPath = (Get-Location).Path

                            # If it is a relative path, then it should be resolved against the HoMM2 installation path, therefore, we will temporarily
                            # change the current directory to the HoMM2 installation directory
                            try {
                                Set-Location $homm2Path

                                $cdDrive = (Resolve-Path $cdDrive).Path

                                # If CDDrive eventually resolves to the HoMM2 installation directory, then ignore it, because there is no need to copy
                                # the same files twice
                                if ((Resolve-Path $homm2Path).Path -Eq $cdDrive) {
                                    throw
                                }
                            } catch {
                                $cdDrive = $null
                            } finally {
                                Set-Location $currentPath
                            }
                        }

                        if ($null -Ne $cdDrive) {
                            $homm2CD = $cdDrive.TrimEnd("\")
                        }
                    }

                    break
                }
            }
        }
    }

    if ($null -Eq $homm2Path) {
        Write-Host -ForegroundColor Yellow "WARNING: Unable to determine the HoMM2 directory"

        $homm2Path = Read-Host -Prompt "Please enter the full path to the HoMM2 directory (e.g. C:\GOG Games\HoMM 2 Gold)"

        if (-Not (Test-HoMM2DirectoryPath -Path $homm2Path)) {
            Write-Host -ForegroundColor Red "FATAL ERROR: Unable to find the HoMM2 directory"

            return
        }
    }

    Write-Host -ForegroundColor Green (-Join("HoMM2 directory: ", (Resolve-Path $homm2Path).Path))

    if ($null -Ne $homm2CD) {
        Write-Host -ForegroundColor Green "HoMM2 CD drive: $homm2CD"
    }

    Write-Host "[3/3] copying game resources"

    $shell = New-Object -ComObject "Shell.Application"

    foreach ($homm2Dir in @($homm2Path, $homm2CD)) {
        if ($null -Eq $homm2Dir) {
            continue
        }

        if (-Not (Test-Path -Path "$homm2Dir\" -PathType Container)) {
            continue
        }

        # fheroes2 can be installed to the HoMM2 directory, there is no need to copy files in this case
        if ((Resolve-Path $homm2Dir).Path -Eq (Resolve-Path $destPath).Path) {
            continue
        }

        foreach ($srcDir in @("HEROES2\ANIM", "ANIM", "DATA", "MAPS", "MUSIC")) {
            if (-Not (Test-Path -Path "$homm2Dir\$srcDir" -PathType Container)) {
                continue
            }

            $destDir = (Split-Path $srcDir -Leaf).ToLower()

            if (-Not (Test-Path -Path "$destPath\$destDir" -PathType Container)) {
                [void](New-Item -Path "$destPath\$destDir" -ItemType "directory")
            }

            $content = $shell.NameSpace((Resolve-Path "$homm2Dir\$srcDir").Path)

            foreach ($item in $content.Items()) {
                $shell.Namespace((Resolve-Path "$destPath\$destDir").Path).CopyHere($item, 0x14)
            }
        }
    }

    # Special case - CD image from GOG
    if (Test-Path -Path "$homm2Path\homm2.gog" -PathType Leaf) {
        foreach ($dosboxExe in @("DOSBox.exe", "DOSBOX\DOSBox.exe")) {
            if (-Not (Test-Path -Path "$homm2Path\$dosboxExe" -PathType Leaf)) {
                continue
            }

            if (-Not (Test-Path -Path "$destPath\anim" -PathType Container)) {
                [void](New-Item -Path "$destPath\anim" -ItemType "directory")
            }

            Write-Host -ForegroundColor Green "Running DOSBOX to extract animation resources, please wait..."

            Start-Process -FilePath "$homm2Path\$dosboxExe" -ArgumentList @("-c", "`"imgmount D '$homm2Path\homm2.gog' -t iso -fs iso`"", `
                                                                            "-c", "`"mount E '$destPath'`"", `
                                                                            "-c", "`"copy D:\HEROES2\ANIM\*.* E:\ANIM`"", `
                                                                            "-c", "exit") `
                          -Wait

            break
        }
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true)
}
