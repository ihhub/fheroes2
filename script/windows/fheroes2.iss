#define AppName "fheroes2"
#define AppId "fheroes2"
#define AppVersion "0.9.3"

[Setup]
AppName={#AppName}
AppId={#AppId}
AppVersion={#AppVersion}
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

[Run]
Filename: "{app}\demo_windows.bat"; Description: "Download demo version files"; Flags: postinstall runascurrentuser
