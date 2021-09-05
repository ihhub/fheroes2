$ErrorActionPreference = "Stop"

try {
    $successfulExtraction = $false

    Write-Host -ForegroundColor Green "This script will extract animation resources from the original Heroes of Might and Magic II CD image provided by GOG`r`n"

    if ((Test-Path -Path "MUSIC" -PathType Container) -And
        (Test-Path -Path "homm2.ins" -PathType Leaf) -And
        (Test-Path -Path "homm2.gog" -PathType Leaf) -And
        (Test-Path -Path "DOSBOX\DOSBox.exe" -PathType Leaf)) {
        if (-Not (Test-Path -Path "anim" -PathType Container)) {
            [void](New-Item -Path "anim" -ItemType "directory")
        }

        Write-Host -ForegroundColor Green "Running DOSBOX to extract animation resources, please wait..."

        Start-Process -FilePath "DOSBOX\DOSBox.exe" -ArgumentList @("-c", "`"imgmount D homm2.ins -t iso -fs iso`"", `
                                                                    "-c", "`"mount E .`"", `
                                                                    "-c", "`"copy D:\HEROES2\ANIM\*.* E:\ANIM`"", `
                                                                    "-c", "exit") `
                      -Wait

        $successfulExtraction = $true
    } else {
        Write-Host -ForegroundColor Red "FATAL ERROR: Unable to find GOG files"
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    if (-Not $successfulExtraction) {
        Write-Host "Press any key to exit..."

        [void][System.Console]::ReadKey($true);
    }
}
