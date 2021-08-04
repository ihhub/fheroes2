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

[Components]
Name: "lang"; Description: "Localizations"; Types: full
Name: "lang\pt"; Description: "Brazilian Portuguese"; Types: full
Name: "lang\cs"; Description: "Czech"; Types: full
Name: "lang\nl"; Description: "Dutch"; Types: full
Name: "lang\fr"; Description: "French"; Types: full
Name: "lang\hu"; Description: "Hungarian"; Types: full
Name: "lang\lt"; Description: "Lithuanian"; Types: full
Name: "lang\pl"; Description: "Polish"; Types: full
Name: "lang\ru"; Description: "Russian"; Types: full
Name: "lang\es"; Description: "Spanish"; Types: full
Name: "lang\sv"; Description: "Swedish"; Types: full
Name: "lang\tr"; Description: "Turkish"; Types: full

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
Source: "..\demo\demo_windows.bat"; DestDir: "{app}"
Source: "..\demo\demo_windows.ps1"; DestDir: "{app}"
Source: "..\..\changelog.txt"; DestDir: "{app}"
Source: "..\..\fheroes2.key"; DestDir: "{app}"
Source: "..\..\LICENSE"; DestDir: "{app}"
Source: "..\..\files\lang\cs.mo"; DestDir: "{app}\files\lang"; Components: "lang\cs"
Source: "..\..\files\lang\es.mo"; DestDir: "{app}\files\lang"; Components: "lang\es"
Source: "..\..\files\lang\fr.mo"; DestDir: "{app}\files\lang"; Components: "lang\fr"
Source: "..\..\files\lang\hu.mo"; DestDir: "{app}\files\lang"; Components: "lang\hu"
Source: "..\..\files\lang\lt.mo"; DestDir: "{app}\files\lang"; Components: "lang\lt"
Source: "..\..\files\lang\nl.mo"; DestDir: "{app}\files\lang"; Components: "lang\nl"
Source: "..\..\files\lang\pl.mo"; DestDir: "{app}\files\lang"; Components: "lang\pl"
Source: "..\..\files\lang\pt.mo"; DestDir: "{app}\files\lang"; Components: "lang\pt"
Source: "..\..\files\lang\ru.mo"; DestDir: "{app}\files\lang"; Components: "lang\ru"
Source: "..\..\files\lang\sv.mo"; DestDir: "{app}\files\lang"; Components: "lang\sv"
Source: "..\..\files\lang\tr.mo"; DestDir: "{app}\files\lang"; Components: "lang\tr"

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
