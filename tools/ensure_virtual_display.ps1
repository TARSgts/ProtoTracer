param(
    [switch]$Silent
)

function Write-Info {
    param([string]$Message)
    if (-not $Silent.IsPresent) {
        Write-Host $Message
    }
}

function Test-Admin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    $args = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", "`"$PSCommandPath`"",
        "-Silent"
    )

    try {
        $proc = Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList ($args -join " ") -Wait -PassThru
        exit $proc.ExitCode
    } catch {
        Write-Error "Elevation was canceled. Virtual display setup was not completed."
        exit 1
    }
}

$ErrorActionPreference = "Stop"

function Get-VddDevices {
    $allDisplay = Get-PnpDevice -Class Display -ErrorAction SilentlyContinue
    $vdd = @()

    foreach ($dev in $allDisplay) {
        try {
            $prop = Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName "DEVPKEY_Device_HardwareIds" -ErrorAction Stop
            $ids = @($prop.Data)
            if ($ids -contains "Root\\MttVDD" -or $ids -contains "Root\MttVDD") {
                $vdd += $dev
            }
        } catch {
            continue
        }
    }

    return $vdd
}

function Get-DisplayIndexFromInstanceId {
    param([string]$InstanceId)
    if ($InstanceId -match "ROOT\\DISPLAY\\(\d+)$") {
        return [int]$Matches[1]
    }
    return -1
}

try {
    Write-Info "Checking Virtual Display Driver package..."

    $pkgRoot = Join-Path $env:LOCALAPPDATA "Microsoft\\WinGet\\Packages"
    $pkg = Get-ChildItem -Path $pkgRoot -Directory -Filter "VirtualDrivers.Virtual-Display-Driver_*" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $pkg) {
        Write-Info "Installing Virtual Display Driver package via winget..."
        winget install --id VirtualDrivers.Virtual-Display-Driver --accept-source-agreements --accept-package-agreements --disable-interactivity --silent | Out-Null

        $pkg = Get-ChildItem -Path $pkgRoot -Directory -Filter "VirtualDrivers.Virtual-Display-Driver_*" -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
    }

    if (-not $pkg) {
        throw "Virtual Display Driver package folder was not found."
    }

    $devcon = Join-Path $pkg.FullName "Dependencies\\devcon.exe"
    if (-not (Test-Path $devcon)) {
        throw "devcon.exe was not found in package dependencies."
    }

    $inf = Join-Path $pkg.FullName "SignedDrivers\\x86\\VDD\\MttVDD.inf"
    if (-not (Test-Path $inf)) {
        $inf = Get-ChildItem -Path $pkg.FullName -Recurse -Filter "MttVDD.inf" -ErrorAction SilentlyContinue |
            Select-Object -First 1 -ExpandProperty FullName
    }
    if (-not $inf) {
        throw "MttVDD.inf was not found in the package."
    }

    $settingsSource = Join-Path $pkg.FullName "Dependencies\\vdd_settings.xml"
    $settingsDir = "C:\\VirtualDisplayDriver"
    $settingsDest = Join-Path $settingsDir "vdd_settings.xml"

    if (-not (Test-Path $settingsDir)) {
        New-Item -ItemType Directory -Path $settingsDir -Force | Out-Null
    }
    if ((Test-Path $settingsSource) -and (-not (Test-Path $settingsDest))) {
        Copy-Item -Path $settingsSource -Destination $settingsDest -Force
    }

    Write-Info "Ensuring Root\\MttVDD is installed and enabled..."
    $vddDevices = Get-VddDevices

    if (-not $vddDevices -or $vddDevices.Count -eq 0) {
        Write-Info "No Root\\MttVDD device found. Installing one instance..."
        & $devcon install $inf "Root\\MttVDD" | Out-Null
        Start-Sleep -Seconds 1
        pnputil /scan-devices | Out-Null
        Start-Sleep -Seconds 1
        $vddDevices = Get-VddDevices
    }

    if (-not $vddDevices -or $vddDevices.Count -eq 0) {
        throw "Root\\MttVDD device could not be created."
    }

    $sorted = $vddDevices | Sort-Object { Get-DisplayIndexFromInstanceId $_.InstanceId } -Descending
    $keep = $sorted | Select-Object -First 1
    $remove = $sorted | Select-Object -Skip 1

    foreach ($dev in $remove) {
        Write-Info "Removing duplicate VDD instance: $($dev.InstanceId)"
        & $devcon remove "@$($dev.InstanceId)" | Out-Null
    }

    if ($remove.Count -gt 0) {
        Start-Sleep -Seconds 1
        pnputil /scan-devices | Out-Null
        Start-Sleep -Seconds 1
        $keep = (Get-VddDevices | Sort-Object { Get-DisplayIndexFromInstanceId $_.InstanceId } -Descending | Select-Object -First 1)
    }

    if (-not $keep) {
        throw "No usable Root\\MttVDD device remained after cleanup."
    }

    & $devcon update $inf "@$($keep.InstanceId)" | Out-Null
    & $devcon enable "@$($keep.InstanceId)" | Out-Null
    & $devcon restart "@$($keep.InstanceId)" | Out-Null

    Write-Info "Virtual display driver is ready."
    exit 0
} catch {
    Write-Error $_
    exit 1
}
