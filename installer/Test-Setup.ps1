$ErrorActionPreference='Stop'
$root=Join-Path ([IO.Path]::GetTempPath()) ('spf-installer-tests-'+[guid]::NewGuid().ToString('N'))
$game=Join-Path $root 'game';$data=Join-Path $root 'data';$payload=Join-Path $root 'payload';$result=Join-Path $root 'result.txt'
$bin=Join-Path $game 'bin\win_x64';$plugin=Join-Path $bin 'plugins\spfPlugins\SPF_VisibleFirstPersonDriverBody';$setup=Join-Path $plugin '.setup'
function Text($p,$s){[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($p))|Out-Null;[IO.File]::WriteAllText($p,$s)}
function Read($p){[IO.File]::ReadAllText($p)}
function Assert($v,$m){if(!$v){throw $m}}
function Run($action,$expect=0,$fault=0){
 & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Setup-Core.ps1') -Action $action -ATSRoot $game -UserData $data -PluginDirectory $plugin -PayloadRoot $payload -TestRoot $root -TestFailAfterWrites $fault -ResultFile $result | Out-Null
 Assert ($LASTEXITCODE -eq $expect) ("Unexpected exit for ${action}: "+(Read $result))
}
Text (Join-Path $bin 'amtrucks.exe') 'supported fixture executable';Text (Join-Path $bin 'plugins\spf-framework.dll') 'framework'
Text (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll') 'original plugin'
Text (Join-Path $plugin 'config\pose.txt') 'user calibration'
Text (Join-Path $plugin 'sounds\feedback.wav') 'previous package sound'
Text (Join-Path $bin 'openxr_loader.dll') 'existing loader'
$otherPlugin=Join-Path $bin 'plugins\spfPlugins\SPF_MirrorControls\SPF_MirrorControls.dll';Text $otherPlugin 'do not touch'
$steam=Join-Path $root 'localconfig.vdf';Text $steam 'Steam settings must remain untouched'
Text (Join-Path $data 'config.cfg') "uset o_openxr_external_loader `"0`"`r`nuset unrelated `"keep`"`r`n"
$files=@();foreach($entry in @(@('main.dll','plugin','SPF_VisibleFirstPersonDriverBody.dll','new plugin',$false),@('driver_body.ini','plugin','config/driver_body.ini',"[pose]`npackage pose`n[default]`nnew default`n",$true),@('feedback.wav','plugin','sounds/feedback.wav','package sound',$false),@('body.scs','mod','body.scs','body',$false),@('loader.dll','bin','openxr_loader.dll','loader',$false))){
 Text (Join-Path $payload $entry[0]) $entry[3];$files+=@{source=$entry[0];destination=$entry[1];target=$entry[2];mutable=$entry[4];onlyIfMissing=($entry[0] -eq 'loader.dll');sha256=(Get-FileHash (Join-Path $payload $entry[0])).Hash.ToLowerInvariant()}
}
$package=@{version='fixture';supportedExe=(Get-FileHash (Join-Path $bin 'amtrucks.exe')).Hash.ToLowerInvariant();files=$files}
Text (Join-Path $payload 'package.json') ($package|ConvertTo-Json -Depth 8);Text (Join-Path $payload 'layer-template.json') '{"api_layer":{"library_path":"./bridge.dll"}}'
# A mid-copy failure restores originals without a persistent transaction log.
Run Install 1 1
Assert ((Read (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll')) -eq 'original plugin') 'Failed install did not restore the original'
Assert (!(Test-Path (Join-Path $setup 'install-record.json'))) 'Failed install left an active record'
Run Install
Assert ((Read (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll')) -eq 'new plugin') 'Plugin not installed'
Assert ((Read (Join-Path $plugin 'config\driver_body.ini')).Contains('user calibration')) 'Calibration not migrated'
Assert (!(Test-Path (Join-Path $plugin 'config\pose.txt'))) 'Old TXT remained'
Assert ((Read (Join-Path $plugin 'sounds\feedback.wav')) -eq 'package sound') 'Packaged sound not installed'
Assert (!(Test-Path (Join-Path $plugin 'config\feedback.wav'))) 'Audio unexpectedly created in config'
Assert ((Read (Join-Path $bin 'openxr_loader.dll')) -eq 'existing loader') 'Existing loader changed'
Assert ((Read $otherPlugin) -eq 'do not touch') 'Standalone mirror plugin changed'
Assert ((Read $steam) -eq 'Steam settings must remain untouched') 'Steam configuration changed'
$record=Read (Join-Path $setup 'install-record.json')|ConvertFrom-Json
Assert (@($record.files|Where-Object {$_.path -like '*SPF_MirrorControls*'}).Count -eq 0) 'Standalone mirror plugin recorded'
Assert (@(Get-ChildItem (Join-Path $setup 'backups') -File).Count -eq 2) 'Backups should contain only replaced binary and calibration; never sounds'
Assert (!(Test-Path (Join-Path $plugin 'payload'))) 'Permanent payload folder created'
Assert (@(Get-ChildItem $root -Directory -Filter 'spf-setup-undo-*').Count -eq 0) 'Temporary undo folder retained'
# Re-running setup repairs a managed binary, with no additional backup history.
Text (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll') 'corrupt managed binary';Run Install
Assert ((Read (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll')) -eq 'new plugin') 'Reinstall did not repair'
Assert (@(Get-ChildItem (Join-Path $setup 'backups') -File).Count -eq 2) 'Reinstall grew permanent backup history'
$bundle=Join-Path $plugin 'config\driver_body.ini';Text $bundle ((Read $bundle).Replace('user calibration','edited consolidated calibration'))
Text (Join-Path $plugin 'config\pose.txt') 'stale legacy value';Run Install
Assert ((Read $bundle).Contains('edited consolidated calibration')) 'Stale legacy setting overwrote consolidated value'
Run Uninstall
Assert ((Read (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll')) -eq 'original plugin') 'Original plugin not restored'
Assert ((Read (Join-Path $plugin 'config\pose.txt')) -eq 'user calibration') 'Original text calibration not restored'
Assert ((Read (Join-Path $plugin 'sounds\feedback.wav')) -eq 'package sound') 'Preexisting sound should not be backed up or restored'
Assert ((Read $bundle).Contains('edited consolidated calibration')) 'Consolidated calibration not retained'
Assert (!(Test-Path (Join-Path $setup 'install-record.json'))) 'Completed uninstall retained its record'
Assert ((Read $otherPlugin) -eq 'do not touch') 'Uninstall changed standalone mirror plugin'
Assert ((Read (Join-Path $data 'config.cfg')).Contains('uset o_openxr_external_loader "0"')) 'Cvar not restored'
# Invalid consolidated config and damaged packages fail before installation.
$good=Read $bundle;Text $bundle "[pose]`none`n[pose]`ntwo`n";Run Install 1;Text $bundle $good
Text (Join-Path $payload 'main.dll') 'tampered';Run Install 1
Assert ((Read (Join-Path $plugin 'SPF_VisibleFirstPersonDriverBody.dll')) -eq 'original plugin') 'Rejected package modified files'
# Current layout embeds general settings, migrating only vehicle poses/boxes.
Text (Join-Path $payload 'main.dll') 'new plugin'
$package.files=@($package.files|Where-Object {$_.target -ne 'config/driver_body.ini'})
foreach($item in @(@('mustang_poses.ini',"[ignition_pose]`npackage pose`n"),@('mustang_boxes.ini',"[interaction_boxes]`npackage boxes`n"))){
 Text (Join-Path $payload $item[0]) $item[1]
 $package.files+=@{source=$item[0];destination='plugin';target=('config/'+$item[0]);mutable=$true;onlyIfMissing=$false;sha256=(Get-FileHash (Join-Path $payload $item[0])).Hash.ToLowerInvariant()}
}
Text (Join-Path $payload 'package.json') ($package|ConvertTo-Json -Depth 8)
Text $bundle "[ignition_pose]`nuser pose`n[interaction_boxes]`nuser boxes`n[head_tracking_gain]`n1.0`n"
Run Install
Assert (!(Test-Path $bundle)) 'Retired general INI remains'
$poses=Join-Path $plugin 'config/mustang_poses.ini';$boxes=Join-Path $plugin 'config/mustang_boxes.ini'
Assert ((Read $poses).Contains('user pose')) 'User pose did not migrate'
Assert ((Read $boxes).Contains('user boxes')) 'User boxes did not migrate'
$vehicleProfile=Join-Path $plugin 'config/vehicles/ford_bronco_boxes.ini'
$vehiclePose=Join-Path $plugin 'config/vehicles/ford_bronco_poses.ini';Text $vehiclePose 'live vehicle grip offsets'
Text $vehicleProfile "[vehicle]`nmodel_id vehicle.ford.bronco_24`n[interaction_boxes]`nuser vehicle placement`n"
Text $poses "[ignition_pose]`nnew user pose`n";Run Install
Assert ((Read $vehicleProfile).Contains('user vehicle placement')) 'Reinstall replaced generated per-vehicle box profile'
Assert ((Read $vehiclePose).Contains('live vehicle grip offsets')) 'Reinstall replaced generated per-vehicle pose profile'
Assert ((Read $poses).Contains('new user pose')) 'Reinstall replaced user vehicle poses'
Run Uninstall
Assert ((Read $poses).Contains('new user pose')) 'Uninstall removed user poses'
Assert ((Read $vehicleProfile).Contains('user vehicle placement')) 'Uninstall removed generated per-vehicle box profile'
Assert ((Read $vehiclePose).Contains('live vehicle grip offsets')) 'Uninstall removed generated per-vehicle pose profile'
Write-Output "PASS: compact install/uninstall, replaced-files-only backups, temporary rollback, reinstall repair, settings consolidation and no audio backups, existing-loader preservation, no Steam or mirror-plugin changes. Fixture: $root"
