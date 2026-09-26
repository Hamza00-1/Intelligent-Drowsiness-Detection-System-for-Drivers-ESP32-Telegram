@echo off
title Publication vers GitHub (Hamza00-1)
echo ====================================================
echo   Publication du Projet sur GitHub : Hamza00-1
echo ====================================================
echo.

set REPO_NAME=Intelligent-Drowsiness-Detection-System-for-Drivers-ESP32-Telegram

echo Verification du statut Git...
"C:\Program Files\Git\cmd\git.exe" status
echo.

echo Configuration du remote origin : https://github.com/Hamza00-1/%REPO_NAME%.git
"C:\Program Files\Git\cmd\git.exe" remote remove origin 2>nul
"C:\Program Files\Git\cmd\git.exe" remote add origin https://github.com/Hamza00-1/%REPO_NAME%.git

echo.
echo Publication de la branche 'main' vers GitHub...
echo (Si demande, connectez-vous ou entrez votre Personal Access Token)
echo.
"C:\Program Files\Git\cmd\git.exe" push -u origin main

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ====================================================
    echo   SUCCES ! Votre projet est en ligne sur GitHub :
    echo   https://github.com/Hamza00-1/%REPO_NAME%
    echo ====================================================
) else (
    echo.
    echo [REMARQUE] Si la publication echoue :
    echo 1. Assurez-vous d'avoir cree le depot sur https://github.com/new avec le nom :
    echo    %REPO_NAME%
    echo 2. Relancez ce fichier.
)

pause
