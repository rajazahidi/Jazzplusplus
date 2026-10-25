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

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = (Resolve-Path "$scriptDir\..\..").Path

# Locate ISCC.exe (Inno Setup Compiler)
$isccCandidates = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
)

$isccPath = $null
foreach ($candidate in $isccCandidates) {
    if (Test-Path $candidate) {
        $isccPath = $candidate
        break
    }
}

if (-not $isccPath) {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        $isccPath = $cmd.Source
    }
}

if (-not $isccPath) {
    Write-Error "Inno Setup Compiler (ISCC.exe) not found. Please install Inno Setup 6."
    exit 1
}

Write-Host "Using Inno Setup Compiler: $isccPath"

# Resolve full paths
$resolvedPkgDir = (Resolve-Path $PackageDir -ErrorAction SilentlyContinue)
if (-not $resolvedPkgDir) {
    if (Test-Path "$rootDir\$PackageDir") {
        $resolvedPkgDir = "$rootDir\$PackageDir"
    } else {
        Write-Error "Package directory '$PackageDir' does not exist."
        exit 1
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$resolvedOutDir = (Resolve-Path $OutputDir).Path
$issFile = "$scriptDir\jazz_setup.iss"

Write-Host "Building Windows Installer for Jazz++ v$Version..."
Write-Host "Source directory: $resolvedPkgDir"
Write-Host "Output directory: $resolvedOutDir"

& $isccPath "/DAppVersion=$Version" "/DSourceDir=$resolvedPkgDir" "/DOutputDir=$resolvedOutDir" $issFile

if ($LASTEXITCODE -eq 0) {
    $setupExe = Get-ChildItem -Path $resolvedOutDir -Filter "Jazz++-Windows-x64-Setup-v*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    Write-Host "Successfully generated installer: $($setupExe.FullName)" -ForegroundColor Green
} else {
    Write-Error "Inno Setup compilation failed with exit code $LASTEXITCODE."
    exit $LASTEXITCODE
}
