[CmdletBinding()]
param(
    [string]$ProjectPath = "",
    [switch]$SkipCodexRegistration
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Join-Path $repoRoot "LanRan"
}

$version = "v0.24.0"
$archiveName = "Monolith-v0.24.0-UE5.8.zip"
$downloadUrl = "https://github.com/tumourlove/monolith/releases/download/$version/$archiveName"
$expectedSha256 = "A12674C3BD35E48C92138531E1A3C5D85D0E630BC017E10466809BF93D5483B1"

$projectDir = (Resolve-Path -LiteralPath $ProjectPath).Path
$projectFile = Join-Path $projectDir "LanRan.uproject"
$pluginDir = Join-Path $projectDir "Plugins\Monolith"
$pluginFile = Join-Path $pluginDir "Monolith.uplugin"
$proxyFile = Join-Path $pluginDir "Binaries\monolith_proxy.exe"

if (-not (Test-Path -LiteralPath $projectFile)) {
    throw "LanRan.uproject was not found under: $projectDir"
}

if (-not (Test-Path -LiteralPath $pluginFile)) {
    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("monolith-mcp-" + [guid]::NewGuid().ToString("N"))
    $archivePath = Join-Path $tempDir $archiveName
    New-Item -ItemType Directory -Path $tempDir | Out-Null

    try {
        Write-Host "Downloading Monolith $version for Unreal Engine 5.8..."
        Invoke-WebRequest -Uri $downloadUrl -OutFile $archivePath

        $actualSha256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
        if ($actualSha256 -ne $expectedSha256) {
            throw "Monolith archive checksum mismatch. Expected $expectedSha256, got $actualSha256."
        }

        $pluginsDir = Split-Path -Parent $pluginDir
        New-Item -ItemType Directory -Path $pluginsDir -Force | Out-Null
        Write-Host "Extracting Monolith to $pluginDir..."
        Expand-Archive -LiteralPath $archivePath -DestinationPath $pluginDir
    }
    finally {
        if (Test-Path -LiteralPath $tempDir) {
            Remove-Item -LiteralPath $tempDir -Recurse -Force
        }
    }
}
else {
    Write-Host "Monolith is already installed at $pluginDir"
}

if (-not (Test-Path -LiteralPath $pluginFile)) {
    throw "Monolith plugin descriptor is missing: $pluginFile"
}
if (-not (Test-Path -LiteralPath $proxyFile)) {
    throw "Monolith MCP proxy is missing: $proxyFile"
}

if (-not $SkipCodexRegistration) {
    if (-not (Get-Command codex -ErrorAction SilentlyContinue)) {
        Write-Warning "Codex CLI was not found. Install it or register the proxy manually."
    }
    else {
        & codex mcp get monolith *> $null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Codex MCP server 'monolith' is already registered."
        }
        else {
            & codex mcp add monolith -- $proxyFile
            if ($LASTEXITCODE -ne 0) {
                throw "Failed to register the Codex MCP server 'monolith'."
            }
        }
    }
}

Write-Host ""
Write-Host "Monolith MCP installation is complete."
Write-Host "Restart Unreal Editor, then open a new Codex session and call monolith_status."
