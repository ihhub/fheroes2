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
Source: "..\..\doc\README.txt"; DestDir: "{app}"; Flags: isreadme
Source: "..\demo\demo_windows.bat"; DestDir: "{app}"
Source: "..\demo\demo_windows.ps1"; DestDir: "{app}"
Source: "..\..\changelog.txt"; DestDir: "{app}"
Source: "..\..\fheroes2.key"; DestDir: "{app}"
Source: "..\..\LICENSE"; DestDir: "{app}"

[Tasks]
Name: desktopicon; Description: "Desktop shortcut"

[Icons]
Name: "{group}\Free Heroes of Might & Magic II"; Filename: "{app}\{#AppName}.exe"; WorkingDir: "{app}"
Name: "{group}\Download demo version files"; Filename: "{app}\demo_windows.bat"; WorkingDir: "{app}"
Name: "{group}\Game data files"; Filename: %WINDIR%\explorer.exe; Parameters: """%APPDATA%\{#AppName}"""
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Free Heroes of Might & Magic II"; Filename: "{app}\{#AppName}.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\demo_windows.bat"; Description: "Download demo version files"; Flags: postinstall runascurrentuser
