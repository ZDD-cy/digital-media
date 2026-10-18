param(
    [string]$EngineRoot = 'D:\language\UE_5.7\UE_5.8',
    [Parameter(Mandatory=$true)][string]$PackageDirectory
)
$ErrorActionPreference = 'Stop'
$uatPath = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$pluginPath = Join-Path $PSScriptRoot 'IndigoInteraction.uplugin'
if (-not (Test-Path -LiteralPath $uatPath)) { throw "RunUAT not found: $uatPath" }
$packagePath = [System.IO.Path]::GetFullPath($PackageDirectory)
$sourcePath = [System.IO.Path]::GetFullPath($PSScriptRoot)
if ($packagePath.Equals($sourcePath, [StringComparison]::OrdinalIgnoreCase) -or
    $packagePath.StartsWith($sourcePath.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'PackageDirectory must be outside the source plugin directory.'
}
if (Test-Path -LiteralPath $packagePath) { throw 'Choose a new output directory; existing directories are not overwritten.' }
& $uatPath BuildPlugin "-Plugin=$pluginPath" "-Package=$packagePath" -TargetPlatforms=Win64
if ($LASTEXITCODE -ne 0) { throw "BuildPlugin failed with exit code $LASTEXITCODE" }
Write-Output "Compiled plugin: $packagePath"
