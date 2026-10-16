#ifndef Payload
  #error Define Payload and OutputRoot when compiling
#endif
#ifndef SetupVersion
  #error Define a unique SetupVersion through build_installer.py
#endif
[Setup]
AppId={{012984B8-7167-4DDC-B6E6-DA662F4924EB}
AppName=SPF Visible First Person Driver Body
AppVersion={#SetupVersion}
VersionInfoVersion={#SetupVersion}
AppPublisher=scegielski
DefaultDirName={code:DefaultPluginDirectory}
UsePreviousAppDir=no
DisableProgramGroupPage=yes
DisableDirPage=yes
UninstallFilesDir={app}\.setup
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputRoot}
OutputBaseFilename=SPF-Visible-First-Person-Driver-Body-Setup-{#SetupVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartApplications=no
UninstallDisplayName=SPF Visible First Person Driver Body
SetupLogging=yes
InfoBeforeFile=INSTALLER-README.txt
[Files]
Source: "Setup-Core.ps1"; DestDir: "{app}\.setup"; Flags: ignoreversion
Source: "{#Payload}\*"; DestDir: "{tmp}\payload"; Flags: ignoreversion recursesubdirs createallsubdirs deleteafterinstall
[Icons]
Name: "{autoprograms}\Uninstall SPF Driver Body"; Filename: "{uninstallexe}"; Comment: "Remove the managed driver-body installation"
Name: "{autodesktop}\American Truck Simulator VR"; Filename: "steam://rungameid/270880"; IconFilename: "{code:GameExe}"
[Code]
var Paths: TInputDirWizardPage; SteamRoot: String;
function Q(S: String): String; begin Result := '"' + S + '"'; end;
function GameExe(Param: String): String; begin Result := Paths.Values[0] + '\bin\win_x64\amtrucks.exe'; end;
function DetectATS(): String;
var Lines: TArrayOfString; I: Integer; S, Candidate: String;
begin
  Result := SteamRoot + '\steamapps\common\American Truck Simulator';
  if FileExists(Result + '\bin\win_x64\amtrucks.exe') then exit;
  if LoadStringsFromFile(SteamRoot + '\steamapps\libraryfolders.vdf', Lines) then
    for I := 0 to GetArrayLength(Lines)-1 do begin
      S := Trim(Lines[I]);
      if Pos('"path"', S) = 1 then begin
        Delete(S, 1, 6); S := Trim(S);
        if (Length(S)>2) and (S[1]='"') then begin
          S := Copy(S,2,Length(S)-2); StringChangeEx(S,'\\','\',True);
          Candidate := S + '\steamapps\common\American Truck Simulator';
          if FileExists(Candidate + '\bin\win_x64\amtrucks.exe') then begin Result := Candidate; exit; end;
        end;
      end;
    end;
end;
function DefaultPluginDirectory(Param: String): String;
begin
  if SteamRoot='' then begin
    if not RegQueryStringValue(HKCU,'Software\Valve\Steam','SteamPath',SteamRoot) then SteamRoot:=ExpandConstant('{commonpf32}\Steam');
    StringChangeEx(SteamRoot,'/','\',True);
  end;
  Result:=DetectATS()+'\bin\win_x64\plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody';
end;
procedure InitializeWizard();
var UserHome: String;
begin
  if not RegQueryStringValue(HKCU,'Software\Valve\Steam','SteamPath',SteamRoot) then SteamRoot := ExpandConstant('{commonpf32}\Steam');
  StringChangeEx(SteamRoot,'/','\',True);
  Paths := CreateInputDirPage(wpSelectDir,'ATS locations','Choose the game and its user-data directory','Close ATS first; Steam can remain open. SPF Framework must already be installed. Existing calibration is preserved. The game directory contains bin; user data contains config.cfg.',False,'');
  Paths.Add('American Truck Simulator game directory:'); Paths.Add('American Truck Simulator user data:');
  Paths.Values[0] := GetPreviousData('ATSRoot',DetectATS());
  UserHome := ExpandConstant('{userdocs}\American Truck Simulator');
  Paths.Values[1] := GetPreviousData('UserData',UserHome);
end;
function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID=Paths.ID then begin
    WizardForm.DirEdit.Text:=Paths.Values[0]+'\bin\win_x64\plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody';
    if not FileExists(GameExe('')) then begin MsgBox('Select the ATS directory containing bin\win_x64\amtrucks.exe.',mbError,MB_OK); Result:=False; exit; end;
    if not FileExists(Paths.Values[0]+'\bin\win_x64\plugins\spf-framework.dll') then begin MsgBox('Install SPF Framework in this ATS directory first.',mbError,MB_OK); Result:=False; exit; end;
    if not FileExists(Paths.Values[1]+'\config.cfg') then begin MsgBox('Choose existing ATS user data containing config.cfg.',mbError,MB_OK); Result:=False; exit; end;
  end;
end;
procedure RegisterPreviousData(PreviousDataKey: Integer);
begin SetPreviousData(PreviousDataKey,'ATSRoot',Paths.Values[0]); SetPreviousData(PreviousDataKey,'UserData',Paths.Values[1]); end;
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result:='';
  WizardForm.DirEdit.Text:=Paths.Values[0]+'\bin\win_x64\plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody';
  if not FileExists(GameExe('')) then Result:='Select a valid ATS game directory.';
  if not FileExists(Paths.Values[1]+'\config.cfg') then Result:='Select existing ATS user data containing config.cfg.';
end;
procedure CurStepChanged(CurStep: TSetupStep);
var Params, Report: String; ExitCode: Integer; Text: AnsiString;
begin
  if CurStep=ssPostInstall then begin
    Report := ExpandConstant('{tmp}\spf-setup-result.txt');
    Params := '-NoProfile -ExecutionPolicy Bypass -File '+Q(ExpandConstant('{app}\.setup\Setup-Core.ps1'))+' -Action Install -ATSRoot '+Q(Paths.Values[0])+' -UserData '+Q(Paths.Values[1])+' -PayloadRoot '+Q(ExpandConstant('{tmp}\payload'))+' -ResultFile '+Q(Report);
    if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),Params,'',SW_HIDE,ewWaitUntilTerminated,ExitCode) then RaiseException('Unable to start installation helper.');
    LoadStringFromFile(Report,Text);
    if ExitCode<>0 then RaiseException(String(Text));
    WizardForm.FinishedLabel.Caption := String(Text);
  end;
end;
function InitializeUninstall(): Boolean;
var Params, Report: String; ExitCode: Integer; Text: AnsiString;
begin
  Result:=False;
  if MsgBox('Remove this plugin? Calibration and later user edits are retained. Replaced files are restored where unchanged. Close ATS first; Steam can remain open.',mbConfirmation,MB_YESNO)<>IDYES then exit;
  Report:=ExpandConstant('{tmp}\spf-setup-result.txt');
  Params:='-NoProfile -ExecutionPolicy Bypass -File '+Q(ExpandConstant('{app}\.setup\Setup-Core.ps1'))+' -Action Uninstall -PluginDirectory '+Q(ExpandConstant('{app}'))+' -ResultFile '+Q(Report);
  if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'),Params,'',SW_HIDE,ewWaitUntilTerminated,ExitCode) then exit;
  if ExitCode<>0 then begin LoadStringFromFile(Report,Text);MsgBox(String(Text),mbError,MB_OK);exit;end;
  Result:=True;
end;
