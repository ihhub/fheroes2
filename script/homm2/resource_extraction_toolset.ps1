$ErrorActionPreference = "Stop"

try {
    function Test-DragDropFile {
        param (
            [string[]]$FileDropList,
            [string]$AllowedExtension
        )

        if ($FileDropList.Count -Ne 1) {
            return $false
        }
        if (-Not (Test-Path -Path $FileDropList[0] -PathType Leaf)) {
            return $false
        }

        $fileExtension = (Get-ChildItem $FileDropList[0]).Extension

        if ($null -Eq $fileExtension) {
            return $false
        }

        $fileExtension = $fileExtension.ToLower()

        if ($fileExtension -Ne $AllowedExtension) {
            return $false
        }

        return $true
    }

    function Copy-GOGMusic {
        param (
            [string]$ArchiveName,
            [string]$DestPath
        )

        $gogMusicPath = "$DestPath\music.gog"

        if (-Not (Test-Path -Path $gogMusicPath -PathType Container)) {
            [void](New-Item -Path $gogMusicPath -ItemType "directory")
        }
        if (-Not (Test-Path -Path "$gogMusicPath\pol" -PathType Container)) {
            [void](New-Item -Path "$gogMusicPath\pol" -ItemType "directory")
        }
        if (-Not (Test-Path -Path "$gogMusicPath\sw" -PathType Container)) {
            [void](New-Item -Path "$gogMusicPath\sw" -ItemType "directory")
        }

        Write-Host -ForegroundColor Green (-Join("GOG OST directory: ", (Resolve-Path $gogMusicPath).Path))

        $tempPath = $null

        while ($true) {
            $randName = [System.IO.Path]::GetRandomFileName()
            $tempPath = "$DestPath\$randName"

            if (-Not (Test-Path -Path $tempPath)) {
                [void](New-Item -Path $tempPath -ItemType "directory")

                break
            }
        }

        Write-Host -ForegroundColor Green "Extracting GOG OST files, please wait..."

        $shell = New-Object -ComObject "Shell.Application"

        $zip = $shell.NameSpace((Resolve-Path $ArchiveName).Path)

        foreach ($item in $zip.Items()) {
            $shell.Namespace((Resolve-Path $tempPath).Path).CopyHere($item, 0x14)
        }

        # Additional PoL soundtrack id -> SW soundtrack id
        $polSWMap = @{"44" = "05"; "45" = "06"; "46" = "07"; "47" = "08"; "48" = "09"; "49" = "10"}

        foreach ($item in (Get-ChildItem -Path $tempPath -Recurse)) {
            $fileExtension = (Get-ChildItem $item.FullName).Extension

            if ($null -Eq $fileExtension) {
                continue
            }

            $fileExtension = $fileExtension.ToLower()

            if (($fileExtension -Ne ".mp3") -And ($fileExtension -Ne ".flac")) {
                continue
            }
            if ($item.Name -notmatch "[0-9]{2}") {
                continue
            }

            $trackNumber = ($item.Name | Select-String -Pattern "[0-9]{2}").Matches[0].Value

            # Castle soundtracks from the Succession Wars
            if ($polSWMap.Values -contains $trackNumber) {
                Copy-Item -Path $item.FullName -Destination "$gogMusicPath\sw\Track$trackNumber$fileExtension"
            # Castle soundtracks from the Price of Loyalty expansion
            } elseif ($polSWMap.Keys -contains $trackNumber) {
                $trackNumber = $polSWMap[$trackNumber]

                Copy-Item -Path $item.FullName -Destination "$gogMusicPath\pol\Track$trackNumber$fileExtension"
            }

            Copy-Item -Path $item.FullName -Destination "$gogMusicPath\Track$trackNumber$fileExtension"
        }

        Remove-Item -Path $tempPath -Recurse
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

    $commandToRun = $null

    [void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")

    $mainForm = New-Object -TypeName "System.Windows.Forms.Form" -Property @{Text = "Resource Extraction Toolset"}

    $gogOSTExtractionGroupBox = New-Object -TypeName "System.Windows.Forms.GroupBox" -Property @{
        Text = "Extract a GOG OST archive"
        Dock = "Fill"
        Padding = 32
        AllowDrop = $true
    }
    $gogOSTExtractionGroupBox.Add_DragEnter({
        if (Test-DragDropFile -FileDropList $_.Data.GetFileDropList() -AllowedExtension ".zip") {
            $_.Effect = [Windows.Forms.DragDropEffects]::Copy
        } else {
            $_.Effect = [Windows.Forms.DragDropEffects]::None
        }
    })
    $gogOSTExtractionGroupBox.add_DragDrop({
        $fileDropList = $_.Data.GetFileDropList()

        if (-Not (Test-DragDropFile -FileDropList $fileDropList -AllowedExtension ".zip")) {
            return
        }

        $global:commandToRun = {Copy-GOGMusic -ArchiveName $fileDropList[0] -DestPath $destPath}.GetNewClosure()

        $mainForm.Close()
    })
    $mainForm.Controls.Add($gogOSTExtractionGroupBox)

    $gogOSTExtractionBrowseButton = New-Object -TypeName "System.Windows.Forms.Button" -Property @{
        Text = "Choose an OST archive file..."
        Dock = "Top"
    }
    $gogOSTExtractionBrowseButton.Add_Click({
        $fileDialog = New-Object -TypeName "System.Windows.Forms.OpenFileDialog" -Property @{
            Filter = "ZIP Archives (*.zip)|*.zip"
        }

        [void]$fileDialog.ShowDialog()

        if ($fileDialog.FileName -Eq "") {
            return
        }

        $global:commandToRun = {Copy-GOGMusic -ArchiveName $fileDialog.FileName -DestPath $destPath}.GetNewClosure()

        $mainForm.Close()
    })
    $gogOSTExtractionGroupBox.Controls.Add($gogOSTExtractionBrowseButton)

    $gogOSTExtractionDragLabel = New-Object -TypeName "System.Windows.Forms.Label" -Property @{
        Text = "Or drag && drop an OST archive file here"
        Dock = "Bottom"
        TextAlign = "MiddleCenter"
    }
    $gogOSTExtractionGroupBox.Controls.Add($gogOSTExtractionDragLabel)

    [System.Windows.Forms.Application]::Run($mainForm)

    if ($null -Ne $commandToRun) {
        & $commandToRun
    }
} catch {
    Write-Host -ForegroundColor Red (-Join("FATAL ERROR: ", ($_ | Out-String)))
} finally {
    Write-Host "Press any key to exit..."

    [void][System.Console]::ReadKey($true)
}
