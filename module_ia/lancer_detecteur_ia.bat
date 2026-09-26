@echo off
title Detecteur IA de Somnolence et Fatigue au Volant
echo ====================================================
echo   Detecteur IA de Somnolence et Baillements
echo ====================================================
echo.
echo Verification et installation des dependances Python...
pip install -r requirements.txt
echo.
echo Lancement du moniteur de surveillance par webcam...
python detecteur_somnolence_ia.py
pause
