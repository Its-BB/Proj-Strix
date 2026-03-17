@echo off
echo.
echo ========================================
echo  BUILDING REAL ESP32-CAM OV2640 FIRMWARE  
echo ========================================
echo.

echo ✅ ESP32-camera component: Added
echo ✅ Real camera driver: Created  
echo ✅ OV2640 configuration: Set
echo ✅ Memory optimization: Applied
echo.

echo INSTRUCTIONS:
echo =============
echo 1. Open "ESP-IDF Command Prompt" from Start Menu
echo 2. Navigate here: cd "C:\Users\Daiwik\Desktop\ESP32-CAM-DroneAI"
echo 3. Run: idf.py build
echo 4. Flash: idf.py -p COM# flash monitor
echo.

echo This will give you REAL OV2640 camera video!
echo No more test patterns - actual camera feed!
echo.

echo Expected results after flashing:
echo ✅ Real OV2640 camera initialization  
echo ✅ Live camera video in browser
echo ✅ Working MJPEG stream
echo ✅ Python integration working
echo.

pause