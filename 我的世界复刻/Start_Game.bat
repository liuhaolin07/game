@echo off
chcp 65001 >nul
title Minecraft.js
cd /d "%~dp0"

echo.
echo   正在启动本地服务器...
echo.

where node >nul 2>nul
if errorlevel 1 (
  echo   [X] 没找到 Node.js。
  echo.
  echo   请先安装 Node.js ^(https://nodejs.org/^) ，装完再双击本文件。
  echo.
  pause
  exit /b 1
)

start "" http://localhost:8080
node server.mjs 8080

echo.
echo   服务器已停止。
pause
