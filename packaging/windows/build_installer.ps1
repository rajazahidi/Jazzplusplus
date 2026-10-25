<#
.SYNOPSIS
    Builds the official Inno Setup installer for Jazz++ on Windows.
.PARAMETER PackageDir
    Directory containing the assembled Windows package (with jazz.exe, dlls, conf, etc.)
.PARAMETER Version
    The release version string (defaults to "6.0.0")
.PARAMETER OutputDir
    Output directory for the setup executable (defaults to "dist")
#>
param(
    [string]$PackageDir = "Jazz++-Windows-x64",
    [string]$Version = "6.0.0",
    [string]$OutputDir = "dist"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($PackageDir)) {
    $PackageDir = "Jazz++-Windows-x64"
}
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = "6.0.0"
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = "dist"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = (Resolve-Path "$scriptDir\..\..").Path

# Locate ISCC.exe (Inno Setup Compiler)
$isccPath = $null

$cmd = Get-Command "iscc" -ErrorAction SilentlyContinue
if (-not $cmd) {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
}
if ($cmd) {
    $isccPath = $cmd.Source
}

if (-not $isccPath) {
    $isccCandidates = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
    )
    foreach ($candidate in $isccCandidates) {
        if (Test-Path $candidate) {
            $isccPath = $candidate
            break
        }
    }
}

if (-not $isccPath) {
    Write-Error "Inno Setup Compiler (ISCC.exe) not found in PATH or standard install directories."
    exit 1
}

Write-Host "Using Inno Setup Compiler: $isccPath"

# Resolve package directory
if (-not (Test-Path $PackageDir)) {
    if (Test-Path "$rootDir\$PackageDir") {
        $PackageDir = "$rootDir\$PackageDir"
    } else {
        Write-Error "Package directory '$PackageDir' does not exist."
        exit 1
    }
}
$resolvedPkgDir = (Resolve-Path $PackageDir).Path

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$resolvedOutDir = (Resolve-Path $OutputDir).Path
$issFile = (Resolve-Path "$scriptDir\jazz_setup.iss").Path

Write-Host "Building Windows Installer for Jazz++ v$Version..."
Write-Host "Source directory: $resolvedPkgDir"
Write-Host "Output directory: $resolvedOutDir"
Write-Host "Script file:      $issFile"

# Invoke Inno Setup Compiler
& $isccPath "/DAppVersion=$Version" "/DSourceDir=$resolvedPkgDir" "/DOutputDir=$resolvedOutDir" $issFile

if ($LASTEXITCODE -eq 0) {
    $setupExe = Get-ChildItem -Path $resolvedOutDir -Filter "Jazz++-Windows-x64-Setup-v*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($setupExe) {
        Write-Host "Successfully generated installer: $($setupExe.FullName)" -ForegroundColor Green
    } else {
        Write-Error "Inno Setup succeeded but output installer file was not found in $resolvedOutDir."
        exit 1
    }
} else {
    Write-Error "Inno Setup compilation failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}
