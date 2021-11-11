#define AppName "fheroes2"
#define AppId "fheroes2"

[Setup]
AppName={#AppName}
AppId={#AppId}
AppVersion={#AppVersion}
AppPublisher="fheroes2 Resurrection Team"
AppPublisherURL="https://github.com/ihhub/fheroes2"
AppUpdatesURL="https://github.com/ihhub/fheroes2/releases"
AppSupportURL="https://discord.gg/xF85vbZ"
LicenseFile=..\..\LICENSE
OutputBaseFilename={#AppName}_windows_{#Platform}_{#DeployConfName}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayIcon={app}\{#AppName}.exe
OutputDir={#BuildDir}
#if Platform == 'x64'
ArchitecturesInstallIn64BitMode=x64
#endif

[Files]
Source: "{#BuildDir}\{#AppName}.exe"; DestDir: "{app}"
Source: "{#BuildDir}\lib*.dll"; DestDir: "{app}"
Source: "{#BuildDir}\SDL*.dll"; DestDir: "{app}"
#if DeployConfName == 'SDL2'
Source: "{#BuildDir}\zlib*.dll"; DestDir: "{app}"
#endif
#if Platform == 'x86'
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvcp140.dll"; DestDir: "{app}"
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vcruntime140.dll"; DestDir: "{app}"
#endif
Source: "..\..\docs\README.txt"; DestDir: "{app}"
Source: "..\demo\download_demo_version.bat"; DestDir: "{app}"
Source: "..\demo\download_demo_version.ps1"; DestDir: "{app}"
Source: "..\homm2\extract_homm2_resources.bat"; DestDir: "{app}"
Source: "..\homm2\extract_homm2_resources.ps1"; DestDir: "{app}"
Source: "..\..\changelog.txt"; DestDir: "{app}"
Source: "..\..\fheroes2.key"; DestDir: "{app}"
Source: "..\..\LICENSE"; DestDir: "{app}"
Source: "..\..\files\lang\*.mo"; DestDir: "{app}\files\lang"
Source: "..\..\files\data\*.h2d"; DestDir: "{app}\files\data"

[Tasks]
Name: desktopicon; Description: "Desktop shortcut"

[Icons]
Name: "{group}\Free Heroes of Might & Magic II"; Filename: "{app}\{#AppName}.exe"; WorkingDir: "{app}"
Name: "{group}\Download demo version files"; Filename: "{app}\download_demo_version.bat"; WorkingDir: "{app}"
Name: "{group}\Extract game resources from the original HoMM2 distribution"; Filename: "{app}\extract_homm2_resources.bat"; WorkingDir: "{app}"
Name: "{group}\Game data files"; Filename: %WINDIR%\explorer.exe; Parameters: """%APPDATA%\{#AppName}"""
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Free Heroes of Might & Magic II"; Filename: "{app}\{#AppName}.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\extract_homm2_resources.bat"; Flags: runascurrentuser; Check: UseResourcesFromOriginalGame
Filename: "{app}\download_demo_version.bat"; Flags: runascurrentuser; Check: UseResourcesFromDemoVersion

[CustomMessages]
ResourcesSettingsPageCaption=Game Resources Settings
ResourcesSettingsPageDescription=Configure the source of the original game's resources
UseResourcesFromOriginalGameRadioButtonCaption=Use resources from the original game
UseResourcesFromOriginalGameLabelCaption=I already have the original Heroes of Might and Magic II installed, use resources from the original game.
UseResourcesFromDemoVersionRadioButtonCaption=Use resources from the demo version
UseResourcesFromDemoVersionLabelCaption=I don't have the original Heroes of Might and Magic II, download the demo version of the game and use resources from it.
DoNothingRadioButtonCaption=Do nothing
DoNothingLabelCaption=I'll figure it out on my own.

[Code]
var
    IsFreshInstallation: Boolean;
    ResourcesSettingsPageID: Integer;
    UseResourcesFromOriginalGameRadioButton: TRadioButton;
    UseResourcesFromDemoVersionRadioButton: TRadioButton;
    DoNothingRadioButton: TRadioButton;

function UseResourcesFromOriginalGame: Boolean;
begin
    Result := IsFreshInstallation and UseResourcesFromOriginalGameRadioButton.Checked;
end;

function UseResourcesFromDemoVersion: Boolean;
begin
    Result := IsFreshInstallation and UseResourcesFromDemoVersionRadioButton.Checked;
end;

procedure CreateResourcesSettingsPage;
var
    VerticalOffset: Integer;
    Page: TWizardPage;
    UseResourcesFromOriginalGameLabel: TLabel;
    UseResourcesFromDemoVersionLabel: TLabel;
    DoNothingLabel: TLabel;
begin
    VerticalOffset := 0;

    Page := CreateCustomPage(wpSelectTasks, ExpandConstant('{cm:ResourcesSettingsPageCaption}'), ExpandConstant('{cm:ResourcesSettingsPageDescription}'));
    with Page do
    begin
        ResourcesSettingsPageID := ID;
    end;

    UseResourcesFromOriginalGameRadioButton := TRadioButton.Create(Page);
    with UseResourcesFromOriginalGameRadioButton do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseResourcesFromOriginalGameRadioButtonCaption}');
        Font.Style := [fsBold];
        Left := ScaleX(0);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(16);
        TabOrder := 1;
        TabStop := True;
        Checked := True;
    end;

    VerticalOffset := VerticalOffset + 24;

    UseResourcesFromOriginalGameLabel := TLabel.Create(Page);
    with UseResourcesFromOriginalGameLabel do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseResourcesFromOriginalGameLabelCaption}');
        Left := ScaleX(16);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(36);
        AutoSize := False;
        WordWrap := True;
    end;

    VerticalOffset := VerticalOffset + 48;

    UseResourcesFromDemoVersionRadioButton := TRadioButton.Create(Page);
    with UseResourcesFromDemoVersionRadioButton do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseResourcesFromDemoVersionRadioButtonCaption}');
        Font.Style := [fsBold];
        Left := ScaleX(0);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(16);
        TabOrder := 2;
    end;

    VerticalOffset := VerticalOffset + 24;

    UseResourcesFromDemoVersionLabel := TLabel.Create(Page);
    with UseResourcesFromDemoVersionLabel do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseResourcesFromDemoVersionLabelCaption}');
        Left := ScaleX(16);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(36);
        AutoSize := False;
        WordWrap := True;
    end;

    VerticalOffset := VerticalOffset + 48;

    DoNothingRadioButton := TRadioButton.Create(Page);
    with DoNothingRadioButton do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:DoNothingRadioButtonCaption}');
        Font.Style := [fsBold];
        Left := ScaleX(0);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(16);
        TabOrder := 3;
    end;

    VerticalOffset := VerticalOffset + 24;

    DoNothingLabel := TLabel.Create(Page);
    with DoNothingLabel do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:DoNothingLabelCaption}');
        Left := ScaleX(16);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(36);
        AutoSize := False;
        WordWrap := True;
    end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
    if PageID = ResourcesSettingsPageID then
        Result := not IsFreshInstallation
    else
        Result := False;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
    { This logic relies on the assumption that the value of UsePreviousAppDir is yes }
    if CurPageID = wpSelectDir then
        IsFreshInstallation := True;
end;

procedure InitializeWizard;
begin
    CreateResourcesSettingsPage;
end;

function InitializeSetup: Boolean;
begin
    IsFreshInstallation := False;
    ResourcesSettingsPageID := -1;

    Result := True;
end;
