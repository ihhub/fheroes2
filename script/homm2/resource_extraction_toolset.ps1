$ErrorActionPreference = "Stop"

try {
    function Copy-FLACMusic {
        param (
            [string]$ArchiveName,
            [string]$DestPath
        )

        Write-Host -ForegroundColor Green "Extracting FLAC music files, please wait..."

        $musicPath = "$DestPath\music"

        if (-Not (Test-Path -Path $musicPath -PathType Container)) {
            [void](New-Item -Path $musicPath -ItemType "directory")
        }

        while ($true) {
            $randName = [System.IO.Path]::GetRandomFileName()

            if (-Not (Test-Path -Path "$musicPath\$randName")) {
                [void](New-Item -Path "$musicPath\$randName" -ItemType "directory")

                break
            }
        }

        $shell = New-Object -ComObject "Shell.Application"

        $zip = $shell.NameSpace((Resolve-Path $ArchiveName).Path)

        foreach ($item in $zip.Items()) {
            $shell.Namespace((Resolve-Path "$musicPath\$randName").Path).CopyHere($item, 0x14)
        }

        foreach ($item in (Get-ChildItem -Path "$musicPath\$randName" -Filter "*.flac" -Recurse)) {
            $trackNumber = ($item.Name | Select-String -Pattern "[0-9]{2}").Matches[0].Value

            if ($null -Eq $trackNumber) {
                continue
            }

            Copy-Item -Path $item.FullName -Destination "$musicPath\Track$trackNumber.flac"
        }

        Remove-Item -Path "$musicPath\$randName" -Recurse
    }

    Write-Host -ForegroundColor Green "This script will run the extraction toolset to extract and copy additional game resources from the original distribution of Heroes of Might and Magic II`r`n"

    Write-Host "[1/2] determining the destination directory"

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

    Write-Host "[2/2] running the resource extraction toolset"

    [void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")

    $mainForm = New-Object -TypeName "System.Windows.Forms.Form" -Property @{Text = "Resource Extraction Toolset"}

    $flacMusicExtractionGroupBox = New-Object -TypeName "System.Windows.Forms.GroupBox" -Property @{
        Text = "Extract the GOG FLAC music archive"
        Dock = "Fill"
        Padding = 32
        AllowDrop = $true
    }
    $flacMusicExtractionGroupBox.Add_DragEnter({
        $fileDropList = $_.Data.GetFileDropList()

        if ($fileDropList.Count -Ne 1) {
            return
        }

        if ((Test-Path -Path $fileDropList[0] -PathType Leaf) -And ((Get-ChildItem $fileDropList[0]).Extension.ToLower() -Eq ".zip")) {
            $_.Effect = [Windows.Forms.DragDropEffects]::copy
        }
    })
    $flacMusicExtractionGroupBox.add_DragDrop({
        $fileDropList = $_.Data.GetFileDropList()

        if ($fileDropList.Count -Ne 1) {
            return
        }

        if (-Not (Test-Path -Path $fileDropList[0] -PathType Leaf) -Or ((Get-ChildItem $fileDropList[0]).Extension.ToLower() -Ne ".zip")) {
            return
        }

        $mainForm.Close()

        Copy-FLACMusic -ArchiveName $fileDropList[0] -DestPath $destPath
    })
    $mainForm.Controls.Add($flacMusicExtractionGroupBox)

    $flacMusicExtractionBrowseButton = New-Object -TypeName "System.Windows.Forms.Button" -Property @{
        Text = "Choose an archive file..."
        Dock = "Top"
    }
    $flacMusicExtractionBrowseButton.Add_Click({
        $fileDialog = New-Object -TypeName "System.Windows.Forms.OpenFileDialog" -Property @{
            Filter = "ZIP Archives (*.zip)|*.zip"
        }

        [void]$fileDialog.ShowDialog()

        if ($fileDialog.FileName -Ne "") {
            $mainForm.Close()

            Copy-FLACMusic -ArchiveName $fileDialog.FileName -DestPath $destPath
        }
    })
    $flacMusicExtractionGroupBox.Controls.Add($flacMusicExtractionBrowseButton)

    $flacMusicExtractionDragLabel = New-Object -TypeName "System.Windows.Forms.Label" -Property @{
        Text = "Or drag && drop an archive file here"
        Dock = "Bottom"
        TextAlign = "MiddleCenter"
    }
    $flacMusicExtractionGroupBox.Controls.Add($flacMusicExtractionDragLabel)

    [System.Windows.Forms.Application]::Run($mainForm)
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true)
}
