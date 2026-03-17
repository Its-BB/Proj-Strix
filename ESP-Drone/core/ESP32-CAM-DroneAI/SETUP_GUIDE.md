# ESP32-CAM WiFi Connection & Video Streaming Setup

## 🎯 Overview

This guide will help you:
1. **Configure ESP32-CAM** to connect to your WiFi network
2. **Build and flash** the firmware
3. **Stream video** from ESP32-CAM to your computer using Python

---

## 📋 Prerequisites

- **ESP32-CAM board** with USB-to-Serial adapter
- **ESP-IDF** installed and configured
- **Python 3.7+** with required packages
- **WiFi credentials**: 
  - SSID: `GNXS-5G-854460`
  - Password: `9496795613`

---

## 🔧 Step 1: Build and Flash Firmware

### 1.1 Open ESP-IDF Command Prompt

### 1.2 Navigate to project directory
```bash
cd "c:\Users\Daiwik\Desktop\My Things\Strix project\ESP32-CAM-DroneAI"
```

### 1.3 Build the project
```bash
idf.py build
```

### 1.4 Flash to ESP32-CAM
```bash
idf.py -p COM# flash monitor
```
Replace `COM#` with your serial port (e.g., `COM3`, `COM4`)

### 1.5 Monitor output
You should see logs like:
```
✅ WiFi Connected!
   🌐 IP Address: 192.168.x.x
   🔗 Web Interface: http://192.168.x.x
   📺 Camera Stream: http://192.168.x.x/stream
```

**Note the IP address** - you'll need it for the Python script!

---

## 🐍 Step 2: Install Python Dependencies

### 2.1 Install required packages
```bash
pip install -r requirements.txt
```

Or manually:
```bash
pip install opencv-python requests numpy
```

---

## 📹 Step 3: Stream Video

### 3.1 Basic usage - Display live stream
```bash
python fetch_video.py 192.168.x.x
```
Replace `192.168.x.x` with the IP address from Step 1.5

### 3.2 Save video to file
```bash
python fetch_video.py 192.168.x.x --output video.mp4
```

### 3.3 Capture specific number of frames
```bash
python fetch_video.py 192.168.x.x --frames 300
```

### 3.4 Test connection only
```bash
python fetch_video.py 192.168.x.x --test
```

### 3.5 Stream without display (background capture)
```bash
python fetch_video.py 192.168.x.x --output video.mp4 --no-display
```

---

## 🎮 Controls

When streaming with display:
- **Press 'q'** to quit and stop streaming
- **Ctrl+C** to force stop

---

## 🔍 Troubleshooting

### ❌ "Connection refused" error
- Check ESP32-CAM is powered on
- Verify IP address is correct
- Ensure both devices are on same WiFi network
- Check firewall settings

### ❌ "No module named 'cv2'"
```bash
pip install opencv-python
```

### ❌ "No module named 'requests'"
```bash
pip install requests
```

### ❌ ESP32 won't connect to WiFi
- Verify SSID and password in `main/wifi_config.h`
- Check WiFi signal strength
- Ensure 2.4GHz band is available (5GHz not supported)
- Check ESP-IDF logs for errors

### ❌ No video stream
- Verify camera is initialized (check ESP logs)
- Try accessing `http://192.168.x.x` in browser first
- Check if `/stream` endpoint is working

---

## 📊 Python Script Features

### Connection Testing
```bash
python fetch_video.py 192.168.x.x --test
```
Tests if ESP32-CAM is reachable before streaming

### Live Display
- Real-time video feed in OpenCV window
- Frame counter and timestamp overlay
- FPS calculation
- Graceful shutdown with 'q' key

### Video Recording
- Saves MJPEG stream to MP4 file
- Preserves frame rate and resolution
- Includes metadata overlay

### Performance Monitoring
- Displays FPS every 30 frames
- Shows total frames captured
- Calculates average FPS

---

## 🌐 Web Interface

You can also view the stream in a web browser:
```
http://192.168.x.x
```

Or directly access the stream endpoint:
```
http://192.168.x.x/stream
```

---

## 📝 WiFi Configuration

To change WiFi credentials, edit `main/wifi_config.h`:

```c
#define WIFI_SSID     "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
```

Then rebuild and flash.

---

## 🚀 Quick Start Commands

```bash
# 1. Build and flash
idf.py build
idf.py -p COM3 flash monitor

# 2. Note the IP address from logs

# 3. In another terminal, stream video
python fetch_video.py 192.168.x.x

# 4. Save to file
python fetch_video.py 192.168.x.x --output drone_video.mp4
```

---

## 📚 File Structure

```
ESP32-CAM-DroneAI/
├── main/
│   ├── main.c                 # Main application
│   ├── wifi_station.c/h       # WiFi connection (NEW)
│   ├── wifi_config.h          # WiFi credentials
│   ├── web_server.c/h         # HTTP server
│   ├── camera.h               # Camera interface
│   └── simple_camera.c        # Camera implementation
├── fetch_video.py             # Python video fetcher (NEW)
├── build_and_flash.bat        # Build script
└── requirements.txt           # Python dependencies
```

---

## 💡 Tips

1. **Find ESP32-CAM IP**: Check your router's connected devices list
2. **Static IP**: Consider setting a static IP on ESP32 for consistency
3. **Performance**: Adjust camera resolution in `simple_camera.c` for better FPS
4. **Network**: Ensure 2.4GHz WiFi band is enabled (5GHz not supported by ESP32)
5. **Power**: Use a stable 5V power supply for best results

---

## 🐛 Debug Logging

To see detailed logs during flashing:
```bash
idf.py -p COM3 flash monitor -v
```

To save logs to file:
```bash
idf.py -p COM3 monitor > esp32_logs.txt
```

---

## ✅ Success Indicators

- ✅ ESP32 boots and connects to WiFi
- ✅ IP address displayed in logs
- ✅ Python script can connect and test connection
- ✅ Video frames are received and displayed
- ✅ FPS counter shows > 0

---

**Happy streaming! 🚁📹**
