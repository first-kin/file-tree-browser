# AI-generated; submitter manual review is required before delivery.
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$SkipFrontendBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$webRoot = Join-Path $repoRoot 'web'
$serverRoot = Join-Path $repoRoot 'server'
$serverCandidates = @(
    (Join-Path $repoRoot "build\$Configuration\dir_tree_server.exe"),
    (Join-Path $repoRoot 'build\dir_tree_server.exe')
)
$serverExe = $serverCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$indexFile = Join-Path $webRoot 'dist\index.html'

if (-not $SkipFrontendBuild) {
    Push-Location $webRoot
    try {
        npm ci
        npm run build
        npm run verify:dist
    }
    finally {
        Pop-Location
    }
}

if (-not (Test-Path -LiteralPath $indexFile)) {
    throw "Missing frontend production file: $indexFile"
}

if (-not $serverExe) {
    throw "Missing server executable. Checked: $($serverCandidates -join ', '). Build the CMake $Configuration target first."
}

Start-Process -FilePath $serverExe -WorkingDirectory $serverRoot -WindowStyle Hidden
Write-Host 'Server started: http://127.0.0.1:8080'
Write-Host 'Run scripts\smoke-check.ps1 to verify the same-origin deployment.'
