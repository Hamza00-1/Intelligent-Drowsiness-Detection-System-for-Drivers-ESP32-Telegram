@echo off
title Push to GitHub (Hamza00-1)
echo ====================================================
echo   Pushing Project to GitHub: Hamza00-1
echo ====================================================
echo.

set REPO_NAME=Intelligent-Drowsiness-Detection-System-for-Drivers-ESP32-Telegram

echo Checking Git status...
"C:\Program Files\Git\cmd\git.exe" status
echo.

echo Adding remote origin: https://github.com/Hamza00-1/%REPO_NAME%.git
"C:\Program Files\Git\cmd\git.exe" remote remove origin 2>nul
"C:\Program Files\Git\cmd\git.exe" remote add origin https://github.com/Hamza00-1/%REPO_NAME%.git

echo.
echo Pushing branch 'main' to GitHub...
echo (If prompted, enter your GitHub Personal Access Token or Sign In via browser)
echo.
"C:\Program Files\Git\cmd\git.exe" push -u origin main

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================================
    echo   SUCCESS! Your project is live on GitHub at:
    echo   https://github.com/Hamza00-1/%REPO_NAME%
    echo ====================================================
) else (
    echo.
    echo [NOTE] If push failed:
    echo 1. Ensure you created the repository on https://github.com/new with name:
    echo    %REPO_NAME%
    echo 2. Run this script again.
)

pause
