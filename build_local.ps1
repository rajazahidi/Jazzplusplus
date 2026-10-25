<#
.SYNOPSIS
    Compiles, links, and tests Jazz++ locally on Windows using MSVC, Ninja, and wxWidgets.
.PARAMETER Config
    Build configuration: "Release" (default) or "Debug".
.PARAMETER RunTests
    Run core unit tests (jazz_tests.exe) after compilation. Defaults to $true.
.PARAMETER BuildDir
    Output build directory. Defaults to "build".
.PARAMETER Clean
    Remove existing build directory before configuring.
.EXAMPLE
    .\build_local.ps1
    .\build_local.ps1 -Config Debug -RunTests
    .\build_local.ps1 -Clean -Config Release
#>
param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$RunTests = $true,
    [string]$BuildDir = "build",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "         Jazz++ Local Windows Build Pipeline              " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Configuration : $Config" -ForegroundColor White
Write-Host " Build Directory: $BuildDir" -ForegroundColor White

$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

if ($Clean -and (Test-Path "$rootDir\$BuildDir")) {
    Write-Host "Cleaning build directory $BuildDir..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$rootDir\$BuildDir"
}

# 1. Locate MSVC
$msvcDir = $null
$msvcBase = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
if (Test-Path $msvcBase) {
    $latestMsvc = Get-ChildItem $msvcBase -Directory | Sort-Object Name -Descending | Select-Object -First 1
    if ($latestMsvc) {
        $msvcDir = $latestMsvc.FullName
    }
}
if (-not $msvcDir) {
    throw "MSVC compiler directory not found under $msvcBase."
}
Write-Host " MSVC Tools    : $msvcDir" -ForegroundColor Green

# 2. Locate Windows SDK
$sdkDir = $null
$sdkVersion = $null
$sdkCandidates = @(
    "C:\winsdk\Windows Kits\10",
    "C:\Program Files (x86)\Windows Kits\10"
)
foreach ($cand in $sdkCandidates) {
    if (Test-Path "$cand\Include") {
        $sdkVerDir = Get-ChildItem "$cand\Include" -Directory | Where-Object { $_.Name -like "10.*" } | Sort-Object Name -Descending | Select-Object -First 1
        if ($sdkVerDir) {
            $sdkDir = $cand
            $sdkVersion = $sdkVerDir.Name
            break
        }
    }
}
if (-not $sdkDir) {
    throw "Windows 10/11 SDK not found in candidate paths: $($sdkCandidates -join ', ')."
}
Write-Host " Windows SDK   : $sdkDir (version $sdkVersion)" -ForegroundColor Green

# 3. Locate wxWidgets
$wxDir = $env:wxWidgets_ROOT_DIR
if (-not $wxDir -or -not (Test-Path $wxDir)) {
    if (Test-Path "C:\wxWidgets") {
        $wxDir = "C:\wxWidgets"
    }
}
if (-not $wxDir -or -not (Test-Path $wxDir)) {
    throw "wxWidgets directory not found. Please set `$env:wxWidgets_ROOT_DIR or place wxWidgets at C:\wxWidgets."
}
$wxLibDir = "$wxDir\lib\vc14x_x64_dll"
if (-not (Test-Path $wxLibDir)) {
    throw "wxWidgets MSVC x64 DLL directory not found: $wxLibDir"
}
Write-Host " wxWidgets     : $wxDir" -ForegroundColor Green

# 4. Locate Ninja
$ninjaCmd = Get-Command "ninja" -ErrorAction SilentlyContinue
$ninjaDir = $null
if ($ninjaCmd) {
    $ninjaDir = Split-Path -Parent $ninjaCmd.Source
} else {
    $ninjaSearch = Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "ninja.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ninjaSearch) {
        $ninjaDir = $ninjaSearch.DirectoryName
    }
}
if (-not $ninjaDir) {
    throw "Ninja build system executable not found. Install via: winget install Ninja-build.Ninja --scope user"
}
Write-Host " Ninja Build   : $ninjaDir" -ForegroundColor Green

# 5. Export environment variables
$env:PATH = "$msvcDir\bin\Hostx64\x64;$sdkDir\bin\$sdkVersion\x64;$ninjaDir;$env:PATH"
$env:INCLUDE = "$msvcDir\include;$sdkDir\Include\$sdkVersion\ucrt;$sdkDir\Include\$sdkVersion\shared;$sdkDir\Include\$sdkVersion\um;$sdkDir\Include\$sdkVersion\winrt"
$env:LIB = "$msvcDir\lib\x64;$sdkDir\Lib\$sdkVersion\ucrt\x64;$sdkDir\Lib\$sdkVersion\um\x64"

# 6. Configure CMake
$wxConfig = if ($Config -eq "Debug") { "mswud" } else { "mswu" }
Write-Host "`n[1/3] Configuring CMake project ($Config)..." -ForegroundColor Cyan
cmake -B "$rootDir\$BuildDir" -G Ninja `
    -DCMAKE_BUILD_TYPE="$Config" `
    -DwxWidgets_ROOT_DIR="$wxDir" `
    -DwxWidgets_LIB_DIR="$wxLibDir" `
    -DwxWidgets_CONFIGURATION="$wxConfig"

# 7. Compile project
Write-Host "`n[2/3] Compiling and linking all files..." -ForegroundColor Cyan
cmake --build "$rootDir\$BuildDir" --parallel

# 8. Deploy runtime DLLs
Write-Host "`n[3/3] Deploying runtime DLLs to $BuildDir..." -ForegroundColor Cyan
$dllPattern = if ($Config -eq "Debug") { "wx*334ud_*.dll" } else { "wx*334u_*.dll" }
Get-ChildItem -Path $wxLibDir -Filter $dllPattern | Copy-Item -Destination "$rootDir\$BuildDir" -Force
if ($Config -eq "Debug") {
    $ucrtDebugDll = "$sdkDir\bin\$sdkVersion\x64\ucrt\ucrtbased.dll"
    if (Test-Path $ucrtDebugDll) {
        Copy-Item -Path $ucrtDebugDll -Destination "$rootDir\$BuildDir" -Force
    }
}

Write-Host "`nCompilation completed successfully!" -ForegroundColor Green
Write-Host "Executable binary: $rootDir\$BuildDir\jazz.exe" -ForegroundColor White

# 9. Run tests if requested
if ($RunTests) {
    Write-Host "`nExecuting core unit tests..." -ForegroundColor Cyan
    & "$rootDir\$BuildDir\jazz_tests.exe"
}
