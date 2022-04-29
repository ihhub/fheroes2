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

        Write-Host -ForegroundColor Green "Extracting GOG OST files, please wait..."

        $musicPath = "$DestPath\music"
        $tempPath = $null

        if (-Not (Test-Path -Path $musicPath -PathType Container)) {
            [void](New-Item -Path $musicPath -ItemType "directory")
        }

        while ($true) {
            $randName = [System.IO.Path]::GetRandomFileName()
            $tempPath = "$DestPath\$randName"

            if (-Not (Test-Path -Path $tempPath)) {
                [void](New-Item -Path $tempPath -ItemType "directory")

                break
            }
        }

        $shell = New-Object -ComObject "Shell.Application"

        $zip = $shell.NameSpace((Resolve-Path $ArchiveName).Path)

        foreach ($item in $zip.Items()) {
            $shell.Namespace((Resolve-Path $tempPath).Path).CopyHere($item, 0x14)
        }

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

            Copy-Item -Path $item.FullName -Destination "$musicPath\Track$trackNumber$fileExtension"
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
        if (-Not (Test-DragDropFile -FileDropList $_.Data.GetFileDropList() -AllowedExtension ".zip")) {
            return
        }

        $_.Effect = [Windows.Forms.DragDropEffects]::Copy
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
