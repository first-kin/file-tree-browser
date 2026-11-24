@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul 2>&1
title FolioTree Startup

set ROOT=%~dp0
echo.
echo  ========================================
echo   FolioTree - One-Click Startup
echo  ========================================
echo.

rem === 1. Kill leftovers ===
echo  [1/4] Cleaning old processes...
taskkill /F /IM dir_tree_server.exe 2>nul
taskkill /F /FI "WINDOWTITLE eq FolioTree-Frontend" 2>nul
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":5173.*LISTENING"') do (
    echo    Port 5173 occupied by PID=%%a, releasing
    taskkill /F /PID %%a 2>nul
)
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":8080.*LISTENING"') do (
    echo    Port 8080 occupied by PID=%%a, releasing
    taskkill /F /PID %%a 2>nul
)
timeout /t 1 /nobreak >nul

rem === 2. Start backend ===
echo  [2/4] Starting backend Drogon (port 8080)...
cd /d "%ROOT%build"
start "FolioTree-Backend" ".\Release\dir_tree_server.exe"
cd /d "%ROOT%"

set BACKEND_READY=0
for /l %%i in (1,1,10) do (
    if !BACKEND_READY! equ 0 (
        timeout /t 1 /nobreak >nul
        curl -s -o nul http://127.0.0.1:8080/api/auth/me 2>nul
        if !errorlevel! equ 0 set BACKEND_READY=1
    )
)
if !BACKEND_READY! equ 0 (
    echo    [FAIL] Backend did not start. Check build\Release\dir_tree_server.exe
    pause
    exit /b 1
)
echo    [OK] Backend running at http://127.0.0.1:8080

rem === 3. Start frontend ===
echo  [3/4] Starting frontend Vite (port 5173)...
cd /d "%ROOT%web"

if not exist "node_modules" (
    echo    node_modules missing, running npm install...
    call npm.cmd install --no-audit --no-fund
    if errorlevel 1 (
        echo    [FAIL] npm install failed
        pause
        exit /b 1
    )
)

start "FolioTree-Frontend" cmd /k "npm.cmd run dev"
cd /d "%ROOT%"

set FRONTEND_READY=0
for /l %%i in (1,1,20) do (
    if !FRONTEND_READY! equ 0 (
        timeout /t 1 /nobreak >nul
        curl -s -o nul http://localhost:5173 2>nul
        if !errorlevel! equ 0 set FRONTEND_READY=1
    )
)
if !FRONTEND_READY! equ 0 (
    echo    [FAIL] Frontend did not start
    pause
    exit /b 1
)
echo    [OK] Frontend running at http://localhost:5173

rem === 4. Open browser ===
echo  [4/4] Opening browser...
start "" "http://localhost:5173"

echo.
echo  ========================================
echo   All services started!
echo.
echo   Frontend : http://localhost:5173
echo   Backend  : http://127.0.0.1:8080
echo.
echo   Close this window to shut down all
echo  ========================================
echo.
pause
endlocal
