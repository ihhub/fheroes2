$ErrorActionPreference = "Stop"

try {
    function Test-HoMM2DirectoryPath {
        param (
            [string]$Path
        )

        if ((Test-Path -Path "$Path\HEROES2.EXE" -PathType Leaf) -And
            (Test-Path -Path "$Path\DATA" -PathType Container) -And
            (Test-Path -Path "$Path\MAPS" -PathType Container)) {
            return $true
        }

        return $false
    }

    Write-Host -ForegroundColor Green "This script will extract and copy assets from the original Heroes of Might and Magic II distribution`r`n"

    Write-Host "[1/3] determining destination directory"

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
            Write-Host -ForegroundColor Red "FATAL ERROR: Unable to determine destination directory"

            return
        }

        $destPath = "$Env:APPDATA\fheroes2"

        if (-Not (Test-Path -Path $destPath -PathType Container)) {
            [void](New-Item -Path $destPath -ItemType "directory")
        }
    }

    Write-Host -ForegroundColor Green (-Join("Destination directory: ", (Resolve-Path $destPath).Path))

    Write-Host "[2/3] determining HoMM2 directory"

    $homm2Path = $null

    foreach ($key in @("HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_CURRENT_USER\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall")) {
        if (-Not (Test-Path -Path "Microsoft.PowerShell.Core\Registry::$key" -PathType Container)) {
            continue
        }

        foreach ($subkey in (Get-ChildItem -Path "Microsoft.PowerShell.Core\Registry::$key")) {
            if ($subkey.GetValue("Publisher") -Eq "GOG.com") {
                $path = $subkey.GetValue("InstallLocation").TrimEnd("\")

                if (Test-HoMM2DirectoryPath -Path $path) {
                    $homm2Path = $path

                    break
                }
            }
        }
    }

    if ($null -Eq $homm2Path) {
        Write-Host -ForegroundColor Yellow "WARNING: Unable to determine HoMM2 directory"

        $homm2Path = Read-Host -Prompt "Please enter the full path to the HoMM2 directory (e.g. C:\GOG Games\HoMM 2 Gold)"

        if (-Not (Test-HoMM2DirectoryPath -Path $homm2Path)) {
            Write-Host -ForegroundColor Red "FATAL ERROR: Unable to find HoMM2 directory"

            return
        }
    }

    Write-Host -ForegroundColor Green (-Join("HoMM2 directory: ", (Resolve-Path $homm2Path).Path))

    Write-Host "[3/3] copying assets"

    $shell = New-Object -ComObject "Shell.Application"

    foreach ($dir in @("ANIM", "DATA", "MAPS", "MUSIC")) {
        if (-Not (Test-Path -Path "$homm2Path\$dir" -PathType Container)) {
            continue
        }

        if (-Not (Test-Path -Path "$destPath\$dir" -PathType Container)) {
            [void](New-Item -Path "$destPath\$dir" -ItemType "directory")
        }

        $content = $shell.NameSpace((Resolve-Path "$homm2Path\$dir").Path)

        foreach ($item in $content.items()) {
            $shell.Namespace((Resolve-Path "$destPath\$dir").Path).CopyHere($item, 0x14)
        }
    }

    if ((Test-Path -Path "$homm2Path\homm2.ins" -PathType Leaf) -And
        (Test-Path -Path "$homm2Path\homm2.gog" -PathType Leaf) -And
        (Test-Path -Path "$homm2Path\DOSBOX\DOSBox.exe" -PathType Leaf)) {
        Write-Host -ForegroundColor Green "Running DOSBOX to extract animation assets, please wait..."

        & "$homm2Path\DOSBOX\DOSBox.exe" -c "imgmount D '$homm2Path\homm2.ins' -t iso -fs iso" `
                                         -c "mount E '$destPath'" `
                                         -c "mkdir E:\ANIM" `
                                         -c "copy D:\HEROES2\ANIM\*.* E:\ANIM" `
                                         -c "exit"
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true);
}
