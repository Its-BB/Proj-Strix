# ESP32 Leader Drone Broadcaster

This is the firmware for the leader ESP32 drone that broadcasts its position to follower drones using ESP-NOW protocol.

## Features

- **ESP-NOW Broadcasting**: Sends position data to all follower drones on the same WiFi channel
- **10Hz Update Rate**: Position updates sent every 100ms
- **Signal Strength Tracking**: Includes RSSI (Received Signal Strength Indicator) in broadcasts
- **Multi-Follower Support**: Can broadcast to multiple follower drones simultaneously
- **Low Latency**: ESP-NOW provides low-latency communication suitable for drone swarms

## Hardware Requirements

- ESP32 or ESP32-S2/S3 microcontroller
- Connected to the main drone flight controller (via UART/SPI/I2C)
- WiFi antenna for ESP-NOW communication

## Building and Flashing

### Prerequisites

- ESP-IDF v4.4 or later installed
- ESP32 board connected to your computer

### Build Steps

1. Navigate to this directory:
```bash
cd leader_esp32_code
```

2. Set the target chip (if not already set):
```bash
idf.py set-target esp32
```

3. Build the project:
```bash
idf.py build
```

4. Flash to the ESP32:
```bash
idf.py -p /dev/ttyUSB0 flash
```

Replace `/dev/ttyUSB0` with your serial port (e.g., `COM3` on Windows).

5. Monitor output:
```bash
idf.py -p /dev/ttyUSB0 monitor
```

## Data Format

The leader broadcasts the following data structure every 100ms:

```c
typedef struct {
    float x;                    // X position (meters)
    float y;                    // Y position (meters)
    float z;                    // Z position (meters, altitude)
    float vx;                   // X velocity (m/s)
    float vy;                   // Y velocity (m/s)
    float vz;                   // Z velocity (m/s)
    int8_t rssi;                // Signal strength (dBm)
    uint32_t timestamp;         // Timestamp (milliseconds)
    uint8_t leader_id;          // Leader drone ID
} leader_data_t;
```

## Integration with Flight Controller

To integrate this with your main drone flight controller:

1. Connect the ESP32 to your flight controller via UART/SPI/I2C
2. Implement a serial/SPI/I2C interface to read the broadcast data
3. Pass the received data to the follower drone's flight controller using the `leaderSignalDetectorReceiveData()` function

## Configuration

Key parameters that can be modified:

- `BROADCAST_INTERVAL_MS`: Interval between broadcasts (default: 100ms for 10Hz)
- `broadcast_mac`: MAC address of broadcast (default: FF:FF:FF:FF:FF:FF for all devices)
- `leader_id`: Unique identifier for this leader drone (default: 1)

## Troubleshooting

### No broadcasts being sent
- Check WiFi initialization in logs
- Verify ESP-NOW is properly initialized
- Check that broadcast peer is added successfully

### Poor signal strength
- Ensure antenna is properly connected
- Check WiFi channel configuration
- Reduce distance between drones
- Increase transmit power (already set to maximum in code)

### Followers not receiving data
- Verify follower drones are on the same WiFi channel
- Check that follower firmware includes signal detection module
- Verify MAC address filtering on followers (should accept broadcast MAC)

## Future Enhancements

- Add GPS/GNSS integration for absolute positioning
- Implement position feedback from followers
- Add swarm coordination logic
- Support for multiple leaders
- Encrypted communication for security
- Adaptive broadcast rate based on signal quality

## License

This code is part of the ESP-Drone project and follows the same license terms.
