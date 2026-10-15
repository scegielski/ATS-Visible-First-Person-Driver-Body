param(
 [ValidateSet('Install','Uninstall')][string]$Action='Install',
 [string]$ATSRoot='', [string]$UserData='', [string]$PluginDirectory='',
 [string]$PayloadRoot='', [string]$TestRoot='',
 [int]$TestFailAfterWrites=0,
 [string]$ResultFile=(Join-Path $env:TEMP 'spf-driver-setup-result.txt')
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2
$utf8=[Text.UTF8Encoding]::new($false)
function Write-Text($Path,$Text) { [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path)) | Out-Null; [IO.File]::WriteAllText($Path,$Text,$utf8) }
function Full($Path) { [IO.Path]::GetFullPath($Path).TrimEnd('\') }
function Hash($Path) { if(Test-Path -LiteralPath $Path -PathType Leaf){(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}else{''} }
function Child($Root,$Relative) {
    $r=Full $Root; $p=Full (Join-Path $r $Relative)
    if(!$p.StartsWith($r+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Package path escapes its destination.'};$p
}
function Save-Json($Path,$Value) { Write-Text ($Path+'.tmp') ($Value | ConvertTo-Json -Depth 30);Move-Item -LiteralPath ($Path+'.tmp') -Destination $Path -Force }
function Read-Json($Path) { Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json }
function Set-Cvar($Text,$Name,$Value) {
    $pattern='(?m)^uset '+[regex]::Escape($Name)+' "[^"\r\n]*"\r?$'
    $line='uset '+$Name+' "'+$Value+'"'
    if([regex]::IsMatch($Text,$pattern)){[regex]::Replace($Text,$pattern,[Text.RegularExpressions.MatchEvaluator]{param($m)$line})}else{$Text.TrimEnd()+"`r`n"+$line+"`r`n"}
}
function Read-Cvar($Text,$Name) { $m=[regex]::Match($Text,'(?m)^uset '+[regex]::Escape($Name)+' "([^"\r\n]*)"');if($m.Success){$m.Groups[1].Value}else{$null} }
function Read-IniSections($Text) {
    if($Text.Length -gt 262144 -or $Text.Contains([string][char]0)){throw 'Invalid driver settings bundle.'}
    $sections=[ordered]@{};$current=''
    foreach($line in ($Text -split '\r?\n')){
        $trim=$line.Trim()
        if($trim.StartsWith('[')){
            if($trim -notmatch '^\[([a-z0-9_]{1,80})\]$' -or $sections.Contains($Matches[1])){throw 'Malformed or duplicate settings section.'}
            $current=$Matches[1];$sections[$current]=''
        }elseif($current){$sections[$current]+=$line+"`n"}
        elseif($trim -and !$trim.StartsWith('#') -and !$trim.StartsWith(';')){throw 'Settings content appears before a section.'}
    }
    if(!$sections.Count){throw 'Driver settings bundle is empty.'};return $sections
}
function Write-IniSections($Sections) {
    $text="# Live driver settings and poses. Preserve section names.`n"
    foreach($key in $Sections.Keys){$text+="`n[${key}]`n"+$Sections[$key].TrimEnd("`r","`n")+"`n"};$text
}
function Registry-Read {
 if($TestRoot){if(Test-Path -LiteralPath $mockRegistry){Read-Json $mockRegistry}else{[pscustomobject]@{exists=$false;value=0}};return}
 $key=Get-Item -LiteralPath $regKey -ErrorAction SilentlyContinue;$value=if($key){$key.GetValue($manifestPath,$null)}else{$null}
 if($null -ne $value -and $key.GetValueKind($manifestPath) -ne [Microsoft.Win32.RegistryValueKind]::DWord){throw 'Existing layer registration is not a DWORD.'}
 [pscustomobject]@{exists=($null -ne $value);value=$value}
}
function Registry-Write($Value) {
 if($TestRoot){Save-Json $mockRegistry $Value;return}
 if($Value.exists){New-Item -Path $regKey -Force|Out-Null;New-ItemProperty -LiteralPath $regKey -Name $manifestPath -Value ([int]$Value.value) -PropertyType DWord -Force|Out-Null}
 else{Remove-ItemProperty -LiteralPath $regKey -Name $manifestPath -ErrorAction SilentlyContinue}
}
function Remember($Path,$Mutable=$false) {
 $entry=$script:record.files|Where-Object {$_.path -eq $Path}
 if(!$entry){$entry=[pscustomobject]@{path=$Path;existed=(Test-Path -LiteralPath $Path -PathType Leaf);backup='';installedHash=(Hash $Path);mutable=$Mutable};$script:record.files+=,$entry};$entry
}
function Undo-Snapshot($Path) {
 if($script:undo|Where-Object {$_.path -eq $Path}){return}
 $exists=Test-Path -LiteralPath $Path -PathType Leaf;$copy=Join-Path $undoDir ([string]$script:undo.Count)
 if($exists){Copy-Item -LiteralPath $Path -Destination $copy}
 $script:undo+=,[pscustomobject]@{path=$Path;existed=$exists;copy=$copy}
}
function Backup-Replaced($Entry) {
 if([IO.Path]::GetExtension($Entry.path) -ieq ".wav"){return}
 if($Entry.existed -and !$Entry.backup -and (Test-Path -LiteralPath $Entry.path -PathType Leaf)){
  [IO.Directory]::CreateDirectory($backupDir)|Out-Null
  $copy=Join-Path $backupDir ([guid]::NewGuid().ToString('N')+'-'+[IO.Path]::GetFileName($Entry.path))
  Copy-Item -LiteralPath $Entry.path -Destination $copy;$Entry.backup=$copy;$script:newBackups+=,$copy
 }
}
function Changed {
 $script:writes++
 if($TestFailAfterWrites -gt 0 -and $script:writes -ge $TestFailAfterWrites){throw 'Injected test write failure.'}
}
function Put-Text($Path,$Text,$Mutable=$false) {
 $entry=Remember $Path $Mutable;$current=if(Test-Path -LiteralPath $Path -PathType Leaf){[IO.File]::ReadAllText($Path)}else{$null}
 if($null -ne $current -and $current -ceq $Text){return}
 Undo-Snapshot $Path;Backup-Replaced $entry;Write-Text $Path $Text;$entry.installedHash=Hash $Path;Changed
}
function Put-File($Source,$Path,$Mutable=$false,$OnlyIfMissing=$false) {
 if(($Mutable -or $OnlyIfMissing) -and (Test-Path -LiteralPath $Path)){return}
 $entry=Remember $Path $Mutable;$sourceHash=Hash $Source
 if((Hash $Path) -eq $sourceHash){return}
 Undo-Snapshot $Path;Backup-Replaced $entry;[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))|Out-Null;Copy-Item -LiteralPath $Source -Destination $Path -Force;$entry.installedHash=$sourceHash;Changed
}
function Remove-Migrated($Path) {
 $entry=Remember $Path;Undo-Snapshot $Path;Backup-Replaced $entry;Remove-Item -LiteralPath $Path;$entry.installedHash='';Changed
}
$undo=@();$newBackups=@();$writes=0;$undoDir=''
try {
 if(!$TestRoot -and $TestFailAfterWrites){throw 'Fault injection is restricted to an isolated test fixture.'}
 if(!$TestRoot -and (Get-Process amtrucks -ErrorAction SilentlyContinue)){throw 'Close ATS before setup or uninstall. Steam can remain open.'}
 if($Action -eq 'Install'){
  $ATSRoot=Full $ATSRoot;$UserData=Full $UserData;$PayloadRoot=Full $PayloadRoot;$bin=Join-Path $ATSRoot 'bin\win_x64';$PluginDirectory=Join-Path $bin 'plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody'
 }else{$PluginDirectory=Full $PluginDirectory}
 $setupDir=Join-Path $PluginDirectory '.setup';$recordFile=Join-Path $setupDir 'install-record.json';$backupDir=Join-Path $setupDir 'backups';$manifestPath=Join-Path $PluginDirectory 'spf_controller_xr_layer.json';$regKey='HKLM:\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit'
 if($TestRoot){$TestRoot=Full $TestRoot;foreach($p in @($PluginDirectory)+$(if($Action -eq 'Install'){@($ATSRoot,$UserData,$PayloadRoot)}else{@()})){if(!(Full $p).StartsWith($TestRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Test target escapes TestRoot.'}}}
 $mockRegistry=if($TestRoot){Join-Path $TestRoot 'registry.json'}else{''}
 if($Action -eq 'Uninstall'){
  if(!(Test-Path -LiteralPath $recordFile)){Write-Text $ResultFile 'No managed installation remains.';exit 0}
  $record=Read-Json $recordFile
  if($record.schema -ne 2 -or (Full (Join-Path $record.ats 'bin\win_x64\plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody')) -ne (Full $PluginDirectory)){throw 'Uninstall record does not match this plugin directory.'}
  $allowedMod=(Full (Join-Path $record.userData 'mod'))+'\';$allowedPlugin=(Full $PluginDirectory)+'\';$allowedLoader=Full (Join-Path $record.ats 'bin\win_x64\openxr_loader.dll')
  foreach($entry in $record.files){
   $path=Full $entry.path
   if(!$path.StartsWith($allowedPlugin,[StringComparison]::OrdinalIgnoreCase) -and !$path.StartsWith($allowedMod,[StringComparison]::OrdinalIgnoreCase) -and $path -ne $allowedLoader){throw 'Uninstall record contains an unexpected destination.'}
   if($entry.backup -and !(Full $entry.backup).StartsWith((Full $backupDir)+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Uninstall backup path is outside the plugin setup directory.'}
  }
  foreach($entry in $record.files){
   if($entry.mutable){continue}
   if((Hash $entry.path) -ne $entry.installedHash){continue}
   if($entry.existed){if($entry.backup -and (Test-Path -LiteralPath $entry.backup)){Copy-Item -LiteralPath $entry.backup -Destination $entry.path -Force;Remove-Item -LiteralPath $entry.backup}}
   elseif(Test-Path -LiteralPath $entry.path -PathType Leaf){Remove-Item -LiteralPath $entry.path}
  }
  $config=Join-Path $record.userData 'config.cfg';if(Test-Path -LiteralPath $config){$text=[IO.File]::ReadAllText($config);if((Read-Cvar $text 'o_openxr_external_loader') -eq '1'){
   if($null -eq $record.cvarOriginal){$text=[regex]::Replace($text,'(?m)^uset o_openxr_external_loader "1"\r?\n?','')}else{$text=Set-Cvar $text 'o_openxr_external_loader' $record.cvarOriginal};Write-Text $config $text
  }}
  $registry=Registry-Read;if($registry.exists -and $registry.value -eq 0){Registry-Write $record.registry}
  # Retain only backups that could not be restored because a user changed files.
  $remaining=@($record.files|Where-Object {$_.backup -and (Test-Path -LiteralPath $_.backup)})
  if($remaining.Count){$record.files=$remaining;Save-Json $recordFile $record}else{Remove-Item -LiteralPath $recordFile; if(Test-Path -LiteralPath $backupDir){Remove-Item -LiteralPath $backupDir}}
  Write-Text $ResultFile 'Uninstalled. Calibration and later user edits were preserved; unchanged replaced files were restored.';exit 0
 }
 if(!(Test-Path -LiteralPath (Join-Path $bin 'amtrucks.exe'))){throw 'Select the ATS game directory containing bin\win_x64\amtrucks.exe.'}
 if(!(Test-Path -LiteralPath (Join-Path $bin 'plugins\spf-framework.dll'))){throw 'SPF Framework must be installed first.'}
 $package=Read-Json (Join-Path $PayloadRoot 'package.json');if((Hash (Join-Path $bin 'amtrucks.exe')) -ne $package.supportedExe){throw 'This ATS executable is not the tested supported build.'}
 foreach($f in $package.files){if((Hash (Child $PayloadRoot $f.source)) -ne $f.sha256){throw ('Package integrity check failed: '+$f.source)}}
 $config=Join-Path $UserData 'config.cfg';if(!(Test-Path -LiteralPath $config)){throw 'ATS user-data config.cfg not found.'};$configBefore=[IO.File]::ReadAllText($config);$registryBefore=Registry-Read
 $record=if(Test-Path -LiteralPath $recordFile){Read-Json $recordFile}else{[pscustomobject]@{schema=2;ats=$ATSRoot;userData=$UserData;registry=$registryBefore;cvarOriginal=(Read-Cvar $configBefore 'o_openxr_external_loader');files=@()}}
 if($record.schema -ne 2 -or $record.ats -ne $ATSRoot -or $record.userData -ne $UserData){throw 'Uninstall record belongs to another installation.'}
 $bundlePath=Join-Path $PluginDirectory 'config\driver_body.ini';$bundleBefore=if(Test-Path -LiteralPath $bundlePath){[IO.File]::ReadAllText($bundlePath)}else{''};$bundleEntry=$package.files|Where-Object {$_.target -eq 'config/driver_body.ini'};$sections=$null;$legacyFiles=@()
 if($bundleEntry){
  $sections=Read-IniSections ([IO.File]::ReadAllText((Child $PayloadRoot $bundleEntry.source)))
  foreach($key in @($sections.Keys)){$legacy=Join-Path $PluginDirectory ('config\'+$key+'.txt');if(Test-Path -LiteralPath $legacy){$sections[$key]=[IO.File]::ReadAllText($legacy);$legacyFiles+=,$legacy}}
  if($bundleBefore){$existing=Read-IniSections $bundleBefore;foreach($key in $existing.Keys){$sections[$key]=$existing[$key]}}
 }
 if(!$bundleEntry -and $bundleBefore){$sections=Read-IniSections $bundleBefore}
 $undoDir=Join-Path $(if($TestRoot){$TestRoot}else{[IO.Path]::GetTempPath()}) ('spf-setup-undo-'+[guid]::NewGuid().ToString('N'));[IO.Directory]::CreateDirectory($undoDir)|Out-Null
 try {
  $mustangPath=Join-Path $PluginDirectory 'config\mustang_boxes.ini';$mustangExisted=Test-Path -LiteralPath $mustangPath
  $posesPath=Join-Path $PluginDirectory 'config\mustang_poses.ini';$posesExisted=Test-Path -LiteralPath $posesPath
  foreach($f in $package.files){
   if($f.target -eq 'config/driver_body.ini'){continue}
   $destinationRoot=switch($f.destination){'plugin'{$PluginDirectory};'bin'{$bin};'mod'{Join-Path $UserData 'mod'};default{throw 'Unknown package destination.'}}
   $source=Child $PayloadRoot $f.source
   Put-File $source (Child $destinationRoot $f.target) ([bool]$f.mutable) ([bool]$f.onlyIfMissing)
  }
  if($sections){Put-Text $bundlePath (Write-IniSections $sections) $true;foreach($legacy in $legacyFiles){Remove-Migrated $legacy}}
  # Migrate only existing user placements; clean installs use the captured Mustang defaults.
  if(!$mustangExisted -and $bundleBefore -and ($package.files|Where-Object {$_.target -eq 'config/mustang_boxes.ini'})){
   $priorSections=Read-IniSections $bundleBefore
   if($priorSections.Contains('interaction_boxes')){Put-Text $mustangPath (Write-IniSections ([ordered]@{interaction_boxes=$priorSections['interaction_boxes']})) $true}
  }
  if(Test-Path -LiteralPath $mustangPath){
   foreach($obsolete in @('interaction_boxes','horn_box_offset','horn_box_scale','ignition_box_offset')){if($sections -and $sections.Contains($obsolete)){$sections.Remove($obsolete)}}
   if($sections){Put-Text $bundlePath (Write-IniSections $sections) $true}
  }
  if(Test-Path -LiteralPath $posesPath){
   $poseKeys=@('controller_left_fingers','controller_right_fingers','gesture_pose','right_gesture_pose','ignition_pose','signal_left_pose','signal_right_pose')
   $poseSections=Read-IniSections ([IO.File]::ReadAllText($posesPath))
   foreach($poseKey in $poseKeys){
    if($sections -and $sections.Contains($poseKey)){if(!$posesExisted){$poseSections[$poseKey]=$sections[$poseKey]};$sections.Remove($poseKey)}
   }
   if(!$posesExisted){Put-Text $posesPath (Write-IniSections $poseSections) $true}
   if($sections){Put-Text $bundlePath (Write-IniSections $sections) $true}
  }
  if(!$bundleEntry -and (Test-Path -LiteralPath $bundlePath)){Undo-Snapshot $bundlePath;Remove-Item -LiteralPath $bundlePath}
  $manifest=Read-Json (Join-Path $PayloadRoot 'layer-template.json');$manifest.api_layer.library_path=Join-Path $PluginDirectory 'spf_controller_xr_layer.dll';Put-Text $manifestPath ($manifest|ConvertTo-Json -Depth 8)
  $configAfter=Set-Cvar $configBefore 'o_openxr_external_loader' '1';if($configAfter -cne $configBefore){Undo-Snapshot $config;Write-Text $config $configAfter}
  Registry-Write ([pscustomobject]@{exists=$true;value=0});Save-Json $recordFile $record
 }catch{
  for($i=$undo.Count-1;$i -ge 0;$i--){$entry=$undo[$i];if($entry.existed){Copy-Item -LiteralPath $entry.copy -Destination $entry.path -Force}else{Remove-Item -LiteralPath $entry.path -ErrorAction SilentlyContinue}}
  Registry-Write $registryBefore;foreach($copy in $newBackups){Remove-Item -LiteralPath $copy -ErrorAction SilentlyContinue};throw
 }
 Write-Text $ResultFile 'Installed. Your VR launch options were left unchanged. Enable the body mod in a new ATS profile and launch normally through Steam.';exit 0
}catch{Write-Text $ResultFile ('Setup failed: '+$_.Exception.Message);[Console]::Error.WriteLine($_.Exception.Message);exit 1}
finally{if($undoDir -and (Test-Path -LiteralPath $undoDir)){foreach($file in Get-ChildItem -LiteralPath $undoDir -File){Remove-Item -LiteralPath $file.FullName};Remove-Item -LiteralPath $undoDir}}
