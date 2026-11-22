# AI-generated; submitter manual review is required before delivery.
param(
    [string]$BaseUrl = 'http://127.0.0.1:8080',
    [string]$Username = 'admin',
    [string]$Password = '123456',
    [switch]$IncludeP2
)

$ErrorActionPreference = 'Stop'
$apiRoot = $BaseUrl.TrimEnd('/') + '/api'

function Invoke-Api {
    param(
        [Parameter(Mandatory = $true)][string]$Method,
        [Parameter(Mandatory = $true)][string]$Path,
        [object]$Body,
        [string]$Token
    )

    $headers = @{}
    if ($Token) {
        $headers.Authorization = "Bearer $Token"
    }
    $parameters = @{
        Method = $Method
        Uri = $apiRoot + $Path
        Headers = $headers
        ContentType = 'application/json'
        UseBasicParsing = $true
        TimeoutSec = 15
    }
    if ($null -ne $Body) {
        $parameters.Body = $Body | ConvertTo-Json -Depth 8 -Compress
    }

    $response = Invoke-RestMethod @parameters
    if ($null -eq $response.code) {
        throw "$Method $Path did not return the unified response envelope."
    }
    if ([int]$response.code -ne 0) {
        throw "$Method $Path failed with code $($response.code): $($response.message)"
    }
    return $response.data
}

$login = Invoke-Api -Method Post -Path '/auth/login' -Body @{
    username = $Username
    password = $Password
}
if (-not $login.token -or $login.role -ne 'ADMIN') {
    throw 'Login response is missing an administrator token or role.'
}
Write-Host '[PASS] administrator login'

$token = [string]$login.token
$me = Invoke-Api -Method Get -Path '/auth/me' -Token $token
if ($me.username -ne $Username -or $me.role -ne 'ADMIN') {
    throw 'GET /auth/me does not match the authenticated user.'
}
Write-Host '[PASS] current-user session'

$users = @(Invoke-Api -Method Get -Path '/users' -Token $token)
if ($users.Count -lt 1 -or -not ($users | Where-Object username -eq $Username)) {
    throw 'GET /users did not return the administrator account.'
}
Write-Host '[PASS] administrator user list'

if ($IncludeP2) {
    $graph = Invoke-Api -Method Post -Path '/graph/build' -Token $token
    if ($null -eq $graph.nodeCount -or $null -eq $graph.edgeCount) {
        throw 'POST /graph/build returned incomplete metrics.'
    }
    $cycles = Invoke-Api -Method Get -Path '/graph/cycles' -Token $token
    if ($null -eq $cycles.cycles) {
        throw 'GET /graph/cycles returned no cycles field.'
    }
    Write-Host '[PASS] P2 graph endpoints'
}

Invoke-Api -Method Post -Path '/auth/logout' -Token $token | Out-Null
Write-Host '[PASS] logout'
Write-Host 'Frontend 1 live API integration check passed.'
