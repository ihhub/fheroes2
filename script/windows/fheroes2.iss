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
Filename: "{app}\extract_homm2_resources.bat"; Flags: runascurrentuser; Check: UseAssetsFromOriginalGame
Filename: "{app}\download_demo_version.bat"; Flags: runascurrentuser; Check: UseAssetsFromDemoVersion

[CustomMessages]
AssetSettingsPageCaption=Game Asset Settings
AssetSettingsPageDescription=Configure the source of the original game's assets
UseAssetsFromOriginalGameRadioButtonCaption=Use assets from the original game
UseAssetsFromOriginalGameLabelCaption=I have the original Heroes of Might and Magic II installed, use assets from the original game.
UseAssetsFromDemoVersionRadioButtonCaption=Use assets from the demo version
UseAssetsFromDemoVersionLabelCaption=I don't have the original Heroes of Might and Magic II, download the demo version of the game and use assets from it.
DoNothingRadioButtonCaption=Do nothing
DoNothingLabelCaption=Do nothing, I'll figure it out on my own.

[Code]
var
    UseAssetsFromOriginalGameRadioButton: TRadioButton;
    UseAssetsFromDemoVersionRadioButton: TRadioButton;
    DoNothingRadioButton: TRadioButton;

function UseAssetsFromOriginalGame: Boolean;
begin
    Result := UseAssetsFromOriginalGameRadioButton.Checked;
end;

function UseAssetsFromDemoVersion: Boolean;
begin
    Result := UseAssetsFromDemoVersionRadioButton.Checked;
end;

procedure CreateAssetSettingsPage(PreviousPageId: Integer);
var
    VerticalOffset: Integer;
    Page: TWizardPage;
    UseAssetsFromOriginalGameLabel: TLabel;
    UseAssetsFromDemoVersionLabel: TLabel;
    DoNothingLabel: TLabel;
begin
    VerticalOffset := 0;

    Page := CreateCustomPage(PreviousPageId, ExpandConstant('{cm:AssetSettingsPageCaption}'), ExpandConstant('{cm:AssetSettingsPageDescription}'));

    UseAssetsFromOriginalGameRadioButton := TRadioButton.Create(Page);
    with UseAssetsFromOriginalGameRadioButton do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseAssetsFromOriginalGameRadioButtonCaption}');
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

    UseAssetsFromOriginalGameLabel := TLabel.Create(Page);
    with UseAssetsFromOriginalGameLabel do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseAssetsFromOriginalGameLabelCaption}');
        Left := ScaleX(16);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(36);
        AutoSize := False;
        WordWrap := True;
    end;

    VerticalOffset := VerticalOffset + 48;

    UseAssetsFromDemoVersionRadioButton := TRadioButton.Create(Page);
    with UseAssetsFromDemoVersionRadioButton do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseAssetsFromDemoVersionRadioButtonCaption}');
        Font.Style := [fsBold];
        Left := ScaleX(0);
        Top := ScaleY(VerticalOffset);
        Width := Page.SurfaceWidth - Left * 2;
        Height := ScaleY(16);
        TabOrder := 2;
    end;

    VerticalOffset := VerticalOffset + 24;

    UseAssetsFromDemoVersionLabel := TLabel.Create(Page);
    with UseAssetsFromDemoVersionLabel do
    begin
        Parent := Page.Surface;
        Caption := ExpandConstant('{cm:UseAssetsFromDemoVersionLabelCaption}');
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

procedure InitializeWizard();
begin
    CreateAssetSettingsPage(wpSelectTasks);
end;
