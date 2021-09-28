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
Source: "..\..\docs\README.txt"; DestDir: "{app}"; Flags: isreadme
Source: "..\demo\download_demo_version.bat"; DestDir: "{app}"
Source: "..\demo\download_demo_version.ps1"; DestDir: "{app}"
Source: "..\homm2\extract_homm2_dos_anim.bat"; DestDir: "{app}"
Source: "..\homm2\extract_homm2_dos_anim.ps1"; DestDir: "{app}"
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
Filename: "{app}\download_demo_version.bat"; Description: "Download demo version files"; Flags: postinstall runascurrentuser; Check: IsNotInstalledToTheDirectoryOfTheOriginalGame
Filename: "{app}\extract_homm2_dos_anim.bat"; Description: "Extract video files from the original HoMM2 DOS-based distribution"; Flags: postinstall runascurrentuser; Check: IsInstalledToTheDirectoryWithAnimationResources

[Code]
function IsNotInstalledToTheDirectoryOfTheOriginalGame: Boolean;
begin
    result := not FileExists(ExpandConstant('{app}\HEROES2.EXE')) or
              not DirExists(ExpandConstant('{app}\DATA')) or
              not DirExists(ExpandConstant('{app}\MAPS'));
end;

function IsInstalledToTheDirectoryWithAnimationResources: Boolean;
begin
    result := DirExists(ExpandConstant('{app}\MUSIC')) and
              FileExists(ExpandConstant('{app}\homm2.ins')) and
              FileExists(ExpandConstant('{app}\homm2.gog')) and
              FileExists(ExpandConstant('{app}\DOSBOX\DOSBox.exe'));
end;
