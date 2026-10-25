; Script generated for Jazz++ MIDI & Audio Workstation
; Inno Setup 6 Script

#ifndef AppVersion
  #define AppVersion "6.0.0"
#endif

#ifndef SourceDir
  #define SourceDir "..\..\Jazz++-Windows-x64"
#endif

#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#define MyAppName "Jazz++"
#define MyAppPublisher "Raja Zahidi"
#define MyAppURL "https://rajazahidi.github.io/Jazzplusplus/"
#define MyAppExeName "jazz.exe"

[Setup]
AppId={{8B19C903-8824-4B3B-B053-C8AE3429E256}}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppVerName={#MyAppName} v{#AppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\..\COPYING
OutputDir={#OutputDir}
OutputBaseFilename=Jazz++-Windows-x64-Setup-v{#AppVersion}
SetupIconFile=..\..\resources\icons\jazz.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
ChangesAssociations=yes
DisableProgramGroupPage=auto

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "fileassoc_jazz"; Description: "Associate .jazz project files with Jazz++"; GroupDescription: "File associations:"
Name: "fileassoc_midi"; Description: "Associate .mid / .midi files with Jazz++"; GroupDescription: "File associations:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\jazz.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#SourceDir}\conf\*"; DestDir: "{app}\conf"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\jazz.cfg"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\song\*"; DestDir: "{app}\song"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\HelpFiles\*"; DestDir: "{app}\HelpFiles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\COPYING"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\ChangeLog"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\resources\icons\jazz.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\jazz.ico"
Name: "{group}\Jazz++ User Guide"; Filename: "{app}\HelpFiles\index.html"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\jazz.ico"; Tasks: desktopicon

[Registry]
; .jazz file association
Root: HKA; Subkey: "Software\Classes\.jazz"; ValueType: string; ValueName: ""; ValueData: "JazzPlusPlus.Project"; Flags: uninsdeletevalue; Tasks: fileassoc_jazz
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Project"; ValueType: string; ValueName: ""; ValueData: "Jazz++ Project File"; Flags: uninsdeletekey; Tasks: fileassoc_jazz
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\jazz.ico,0"; Tasks: fileassoc_jazz
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_jazz

; .mid / .midi file association (optional)
Root: HKA; Subkey: "Software\Classes\.mid\OpenWithProgids"; ValueType: string; ValueName: "JazzPlusPlus.Midi"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc_midi
Root: HKA; Subkey: "Software\Classes\.midi\OpenWithProgids"; ValueType: string; ValueName: "JazzPlusPlus.Midi"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassoc_midi
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Midi"; ValueType: string; ValueName: ""; ValueData: "MIDI Sequence File"; Flags: uninsdeletekey; Tasks: fileassoc_midi
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Midi\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\jazz.ico,0"; Tasks: fileassoc_midi
Root: HKA; Subkey: "Software\Classes\JazzPlusPlus.Midi\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc_midi

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
