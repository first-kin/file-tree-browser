# AI-generated; submitter manual review is required before delivery.
param(
    [string]$BaseUrl = 'http://127.0.0.1:8080'
)

$ErrorActionPreference = 'Stop'
$routes = @('/', '/login', '/register', '/tree', '/search', '/stats', '/users', '/merkle', '/graph')
$failures = @()

foreach ($route in $routes) {
    $uri = $BaseUrl.TrimEnd('/') + $route
    try {
        $response = Invoke-WebRequest -Uri $uri -UseBasicParsing -TimeoutSec 10
        $isSpaPage = $response.StatusCode -eq 200 -and $response.Content -match '<div id="app"></div>'
        if (-not $isSpaPage) {
            $failures += "$route did not return the frontend entry page"
        }
        else {
            Write-Host "[PASS] $route"
        }
    }
    catch {
        $failures += "$route request failed: $($_.Exception.Message)"
    }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    throw 'Same-origin deployment smoke check failed.'
}

Write-Host 'Same-origin deployment smoke check passed.'
