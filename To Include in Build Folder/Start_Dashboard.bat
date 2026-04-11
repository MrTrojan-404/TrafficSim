@echo off
title UrbanFlow Command Center Launcher

echo Booting Telemetry Server...
:: The 'start /min' command opens the Python server in a hidden, minimized window
start /min cmd /c "python dashboard.py"

:: Wait 2 seconds to ensure the server is fully online before the game boots
timeout /t 2 /nobreak > NUL

echo Launching Unreal Engine Simulation...
:: Replace "UrbanFlow.exe" with the actual name of your packaged game!
start "" "UrbanFlow.exe"

exit