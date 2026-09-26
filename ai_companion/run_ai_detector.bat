@echo off
title AI Driver Drowsiness & Yawn Detector
echo ====================================================
echo   AI Driver Drowsiness & Fatigue Detector
echo ====================================================
echo.
echo Installing / checking Python dependencies...
pip install -r requirements.txt
echo.
echo Launching AI Computer Vision Webcam Monitor...
python drowsiness_ai_detector.py
pause
