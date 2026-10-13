param([string]$ATSDirectory = 'C:\Program Files (x86)\Steam\steamapps\common\American Truck Simulator\bin\win_x64')
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
$taskStart.EnvironmentVariables['XR_ENABLE_API_LAYERS'] = 'XR_APILAYER_SPF_driver_controller_input'
[System.Diagnostics.Process]::Start($taskStart) | Out-Null
