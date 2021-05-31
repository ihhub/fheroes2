#define AppName "fheroes2"
#define AppId "fheroes2"
#define AppVersion "0.9.3"

[Setup]
AppName={#AppName}
AppId={#AppId}
AppVersion={#AppVersion}
OutputBaseFilename={#AppName}-{#Platform}-{#DeployConfName}
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
Source: "{#BuildDir}\zlib*.dll"; DestDir: "{app}"
#if Platform == 'x86'
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvcp140.dll"; DestDir: "{app}"
Source: "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vcruntime140.dll"; DestDir: "{app}"
#endif
Source: "doc\README.txt"; DestDir: "{app}"; Flags: isreadme
Source: "script\demo\demo_windows.bat"; DestDir: "{app}"
Source: "script\demo\demo_windows.ps1"; DestDir: "{app}"
Source: "changelog.txt"; DestDir: "{app}"
Source: "fheroes2.key"; DestDir: "{app}"
Source: "LICENSE"; DestDir: "{app}"

[Run]
Filename: "{app}\demo_windows.bat"; Description: "Load demo files"; Flags: postinstall runascurrentuser unchecked
