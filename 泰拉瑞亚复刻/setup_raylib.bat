@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "RAYLIB_VER=5.5"
set "RAYLIB_URL=https://github.com/raysan5/raylib/releases/download/%RAYLIB_VER%/raylib-%RAYLIB_VER%_win64_msvc16.zip"
set "ZIP=%TEMP%\raylib-%RAYLIB_VER%.zip"

echo ==========================================================
echo   泰拉瑞亚复刻 - 安装 raylib 依赖
echo ==========================================================
echo.
echo   本工程直接使用 raylib 官方预编译包, 不需要 vcpkg。
echo   下载地址: %RAYLIB_URL%
echo.

rem ---------- 已经装好了就直接走人 ----------
if exist "lib\include\raylib.h" if exist "lib\lib\raylib.lib" (
    echo [OK] 已经检测到 lib\include\raylib.h 与 lib\lib\raylib.lib
    echo      可以直接用 Visual Studio 打开 Project1.sln 编译。
    echo.
    pause
    exit /b 0
)

echo [1/3] 正在下载 (约 3 MB)...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ProgressPreference='SilentlyContinue';" ^
  "try { Invoke-WebRequest -Uri '%RAYLIB_URL%' -OutFile '%ZIP%' -UseBasicParsing; } catch { Write-Host ('下载失败: ' + $_.Exception.Message); exit 1 }"
if errorlevel 1 goto :fail
if not exist "%ZIP%" goto :fail

echo [2/3] 正在解压...
if exist "lib" rmdir /s /q "lib" 2>nul
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Add-Type -AssemblyName System.IO.Compression.FileSystem;" ^
  "$z=[System.IO.Compression.ZipFile]::OpenRead('%ZIP%');" ^
  "$root=(Split-Path -Parent (Split-Path -Parent $z.Entries[0].FullName));" ^
  "foreach($e in $z.Entries){ if($e.Length -eq 0){ continue };" ^
  "  $p=Join-Path '.' $e.FullName.Substring($root.Length);" ^
  "  [System.IO.Directory]::CreateDirectory((Split-Path -Parent $p)) | Out-Null;" ^
  "  [System.IO.Compression.ZipFileExtensions]::ExtractToFile($e,$p,$true) };" ^
  "$z.Dispose()"
if errorlevel 1 goto :fail
del /q "%ZIP%" 2>nul

echo [3/3] 校验...
if not exist "lib\include\raylib.h" goto :fail
if not exist "lib\lib\raylib.lib" goto :fail

echo.
echo ==========================================================
echo   完成!
echo.
echo   - 头文件:  lib\include\raylib.h
echo   - 库文件:  lib\lib\raylib.lib
echo   - 运行库:  lib\lib\raylib.dll  ^(运行时需要, 已在工程里配置好搜索路径^)
echo.
echo   现在用 Visual Studio 打开 Project1.sln, 选 Debug ^| x64 生成运行。
echo ==========================================================
pause
exit /b 0

:fail
echo.
echo [错误] 安装失败。
echo.
echo   如果是下载问题(网络/防火墙), 可以手动下载:
echo     %RAYLIB_URL%
echo   解压后把 include\ 和 lib\ 两个文件夹复制到本项目的 lib\ 目录。
echo   详细说明见 lib\README.md
echo.
pause
exit /b 1
