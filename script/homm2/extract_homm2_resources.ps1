$ErrorActionPreference = "Stop"

try {
    function Test-HoMM2DirectoryPath {
        param (
            [string]$Path
        )

        if (((Test-Path -Path "$Path\HEROES2.EXE" -PathType Leaf) -Or
             (Test-Path -Path "$Path\HEROES2W.EXE" -PathType Leaf)) -And
            (Test-Path -Path "$Path\DATA" -PathType Container) -And
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
    $homm2DrivePath = $null

    foreach ($key in @("HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_CURRENT_USER\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
                       "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
		       "HKEY_LOCAL_MACHINE\SOFTWARE\New World Computing\Heroes of Might and Magic 2")) {
        if (-Not (Test-Path -Path "Microsoft.PowerShell.Core\Registry::$key" -PathType Container)) {
            continue
        }

        foreach ($subkey in (Get-ChildItem -Path "Microsoft.PowerShell.Core\Registry::$key")) {
            $path = $subkey.GetValue("InstallLocation")
            
	    # Legacy installed path detection
            if ($null -Eq $path) {
            	$path = $subkey.GetValue("AppPath")
            }
	    
            if ($null -Ne $path) {
                $path = $path.TrimEnd("\")
            	# Legacy installed drive path detection
            	$DrivePath = $subkey.GetValue("CDDrive")
				
                if (Test-HoMM2DirectoryPath -Path $path) {
                    $homm2Path = $path
                    $homm2DrivePath = $DrivePath

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

    Write-Host "[3/3] copying game resources"

    if ((Resolve-Path $homm2Path).Path -Eq (Resolve-Path $destPath).Path) {
        Write-Host -ForegroundColor Green "Apparently fheroes2 was installed to the directory of the original game, there is no need to copy anything"
    } else {
        $shell = New-Object -ComObject "Shell.Application"

        foreach ($homm2Dir in @($homm2Path, $homm2DrivePath )) {
		
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
    }

    # Special case - CD image from GOG
    if ((Test-Path -Path "$homm2Path\MUSIC" -PathType Container) -And
        (Test-Path -Path "$homm2Path\homm2.ins" -PathType Leaf) -And
        (Test-Path -Path "$homm2Path\homm2.gog" -PathType Leaf) -And
        (Test-Path -Path "$homm2Path\DOSBOX\DOSBox.exe" -PathType Leaf)) {
        if (-Not (Test-Path -Path "$destPath\anim" -PathType Container)) {
            [void](New-Item -Path "$destPath\anim" -ItemType "directory")
        }

        Write-Host -ForegroundColor Green "Running DOSBOX to extract animation resources, please wait..."

        Start-Process -FilePath "$homm2Path\DOSBOX\DOSBox.exe" -ArgumentList @("-c", "`"imgmount D '$homm2Path\homm2.ins' -t iso -fs iso`"", `
                                                                               "-c", "`"mount E '$destPath'`"", `
                                                                               "-c", "`"copy D:\HEROES2\ANIM\*.* E:\ANIM`"", `
                                                                               "-c", "exit") `
                      -Wait
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true)
}
