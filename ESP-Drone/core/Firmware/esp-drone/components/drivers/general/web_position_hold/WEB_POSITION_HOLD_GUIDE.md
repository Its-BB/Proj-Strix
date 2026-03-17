# 🚁 ESP-Drone Web Position Hold System

## Overview

The Web Position Hold System provides a simple web interface to control your ESP-Drone's altitude using the VL53L0X distance sensor. The system automatically maintains a target height and includes safety features like controlled landing and emergency stop.

## Features

✅ **Simple Web Interface** - Access via WiFi hotspot  
✅ **Automatic Position Hold** - Maintains target height automatically  
✅ **Real-time Monitoring** - Live sensor readings and status  
✅ **Safe Landing** - Controlled descent to ground  
✅ **Emergency Stop** - Immediate thrust cutoff  
✅ **PID Control** - Smooth and stable altitude control  

## How It Works

The system reads distance from the VL53L0X sensor (the same sensor used for altitude logging) and uses a PID controller to adjust thrust to maintain your target height. When you set a target height and start position hold:

1. **Sensor Reading**: Continuously reads ground distance from VL53L0X
2. **PID Control**: Calculates required thrust based on height error  
3. **Thrust Control**: Sends commands directly to flight controller
4. **Safety Monitoring**: Checks for dangerous conditions and auto-lands if needed

## Getting Started

### 1. Power On Your Drone
- Power on the ESP-Drone
- Wait for the system to initialize (about 10-15 seconds)
- Look for the message: ✅ **Web Position Hold System Ready!**

### 2. Connect to WiFi
- Connect your phone/computer to the drone's WiFi hotspot
- Network name: `ESP-DRONE_XXXXXX` (XXXXXX = unique MAC address)
- Password: `12345678`

### 3. Open Web Interface
- Open your web browser
- Navigate to: **http://192.168.43.42/**
- The position hold interface should load

### 4. Use the Interface

#### Status Panel
- **Status**: Current system state (Idle, Position Hold, Landing, etc.)
- **Current Height**: Live reading from VL53L0X sensor
- **Target Height**: Your desired altitude
- **Sensor Status**: OK = working, ERROR = sensor issue

#### Controls
- **Target Height**: Set desired altitude (5-200 cm)
- **Start Position Hold**: Begin automatic altitude control
- **Stop**: Stop position hold (immediate)
- **Land Safely**: Controlled descent to ground
- **Emergency Stop**: Immediate thrust cutoff (only when active)

## Usage Instructions

### Basic Position Hold
1. Place drone on ground
2. Set target height (e.g., 20 cm)
3. Click **"Start Position Hold"**
4. Drone will automatically lift and maintain that height
5. Use **"Stop"** to end position hold

### Safe Landing
1. While in position hold mode
2. Click **"Land Safely"**
3. Drone will gradually descend at 0.5 cm/s
4. Automatically stops when below 3 cm height

### Emergency Stop
1. Only visible during active flight
2. Click **"EMERGENCY STOP"**
3. Confirms before executing
4. **IMMEDIATELY cuts all thrust** - use with caution!

## Safety Features

### Automatic Safety Checks
- **Height Limits**: Prevents operation below 5 cm or above 200 cm
- **Sensor Validation**: Stops if sensor readings become invalid
- **Auto-landing**: Lands automatically if too low for too long
- **Emergency Stop**: Always available during active operation

### Safe Operating Guidelines
- **Always supervise the drone** - this is not fully autonomous
- **Keep clear area** around the drone during operation
- **Start with low heights** (10-20 cm) to test
- **Have manual control ready** as backup
- **Land immediately** if anything seems wrong

## Troubleshooting

### "Sensor not ready" error
- Check VL53L0X sensor connections (SDA=GPIO21, SCL=GPIO22)
- Ensure sensor has clear view of ground
- Try restarting the drone

### Position hold doesn't start
- Ensure sensor shows "OK" status  
- Check that current height reading is reasonable
- Make sure target height is between 5-200 cm

### Drift during position hold
- Check for air currents or wind
- Ensure sensor has clear view to ground
- Verify ground surface is not reflective or transparent
- PID gains may need tuning for your specific setup

### Web interface not loading
- Verify WiFi connection to drone hotspot
- Check IP address: http://192.168.43.42/
- Try refreshing the browser
- Restart drone if needed

## Technical Details

### Control Parameters
- **Update Rate**: 50ms (20Hz control loop)
- **PID Gains**: P=800, I=50, D=200 (tunable in code)
- **Base Thrust**: 35000 (adjust for your drone weight)
- **Landing Rate**: 0.5 cm/s descent

### API Endpoints
- `GET /` - Web interface
- `GET /api/status` - Current system status (JSON)
- `POST /api/start` - Start position hold
- `POST /api/stop` - Stop position hold
- `POST /api/land` - Initiate landing
- `POST /api/set_height` - Set target height
- `POST /api/emergency` - Emergency stop

### Sensor Integration
- Uses existing VL53L0X sensor from altitude logger
- Reads distance in centimeters with averaging
- Validates measurements for quality control
- Handles sensor errors gracefully

## Customization

You can modify the system by editing these files:
- `web_position_hold.h` - Configuration constants
- `web_position_hold.c` - Control logic and web interface
- `web_position_hold_api.c` - API handlers and functions

Key parameters you might want to adjust:
- **PID gains** for different drone characteristics
- **Base thrust** for different drone weights
- **Height limits** for your flying area
- **Landing descent rate** for gentler/faster landing

## Notes

- The system uses the same VL53L0X sensor as the altitude logger
- Compatible with existing flight modes (you can switch between them)
- Web interface works on phones, tablets, and computers
- System automatically initializes after drone startup
- Position hold takes priority 3 in the commander system

## Enjoy Flying! 🚁

This system makes altitude control much easier and safer. Start with small heights and get comfortable with the interface before attempting higher flights.

For questions or issues, check the debug console output for detailed system messages.
