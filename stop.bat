@echo off
chcp 65001 >nul 2>&1
title FolioTree — 一键关闭

echo.
echo  正在关闭 FolioTree 服务…
echo.

rem 后端
taskkill /F /IM dir_tree_server.exe 2>nul && echo  ✓ 后端已关闭 || echo  - 后端未运行

rem 前端（Vite 是 node 进程，关闭对应窗口）
taskkill /F /FI "WINDOWTITLE eq FolioTree-Frontend" 2>nul && echo  ✓ 前端窗口已关闭 || echo  - 前端窗口未运行

rem 兜底：释放端口
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":5173.*LISTENING"') do (
    echo    释放端口 5173 (PID=%%a)
    taskkill /F /PID %%a 2>nul
)
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":8080.*LISTENING"') do (
    echo    释放端口 8080 (PID=%%a)
    taskkill /F /PID %%a 2>nul
)

echo.
echo  ✓ 全部已关闭
echo.
pause
