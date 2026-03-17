@echo off
echo ========================================
echo ESP32-CAM Drone Hotspot Build Script
echo ========================================
echo.

echo [1] Cleaning previous build...
idf.py fullclean

echo.
echo [2] Building project...
idf.py build

echo.
echo [3] Build complete! To flash:
echo    - Connect ESP32-CAM via USB
echo    - Press and hold BOOT button while pressing RST button
echo    - Release RST, then release BOOT
echo    - Run: idf.py -p COM# flash monitor
echo.
echo Replace COM# with your actual COM port (e.g., COM3, COM4, etc.)
echo.
echo ========================================
echo Ready to flash!
echo ========================================

pause