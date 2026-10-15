param([string]$ATSDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\American Truck Simulator\bin\win_x64', [switch]$DisableControllerBridge)
$taskExecutable = Join-Path $ATSDirectory 'amtrucks.exe'
if (!(Test-Path -LiteralPath $taskExecutable)) { throw 'ATS executable not found.' }
if (Get-Process amtrucks -ErrorAction SilentlyContinue) { throw 'ATS is already running.' }
if (!(Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Start Steam before using this launcher.' }
$taskStart = New-Object System.Diagnostics.ProcessStartInfo
$taskStart.FileName = $taskExecutable
$taskStart.WorkingDirectory = $ATSDirectory
$taskStart.Arguments = '-experimental_vr -openxr'
$taskStart.UseShellExecute = $false
$taskStart.CreateNoWindow = $true
$taskStart.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Hidden
$taskStart.EnvironmentVariables['SteamAppId'] = '270880'
$taskStart.EnvironmentVariables['SteamGameId'] = '270880'
$taskLayerCandidates = @(
    (Join-Path $ATSDirectory 'plugins/spfPlugins/SPF_VisibleFirstPersonDriverBody'),
    $PSScriptRoot,
    (Join-Path $PSScriptRoot 'controller-layer')
)
$taskLayerDirectory = $taskLayerCandidates | Where-Object {
    (Test-Path -LiteralPath (Join-Path $_ 'spf_controller_xr_layer.json')) -and
    (Test-Path -LiteralPath (Join-Path $_ 'spf_controller_xr_layer.dll'))
} | Select-Object -First 1
if (!$taskLayerDirectory) { throw 'Controller input bridge manifest and DLL were not found.' }
$taskStart.EnvironmentVariables['XR_API_LAYER_PATH'] = [IO.Path]::GetFullPath($taskLayerDirectory)
$taskStart.EnvironmentVariables['XR_ENABLE_API_LAYERS'] = 'XR_APILAYER_SPF_driver_controller_input'
if ($DisableControllerBridge) {
    $taskStart.EnvironmentVariables['SPF_DISABLE_CONTROLLER_HANDS'] = '1'
    $taskStart.EnvironmentVariables.Remove('XR_ENABLE_API_LAYERS')
    $taskStart.EnvironmentVariables.Remove('XR_API_LAYER_PATH')
} else {
    $taskStart.EnvironmentVariables.Remove('SPF_DISABLE_CONTROLLER_HANDS')
}
[System.Diagnostics.Process]::Start($taskStart) | Out-Null
