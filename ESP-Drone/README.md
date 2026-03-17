# ESP-Drone - Embedded Control Stack

Clean embedded-side package aligned with the `droneai` runtime interface.

## Start Here

- `core/ESP32-CAM-DroneAI/main/web_server.c` - MJPEG `/stream` endpoint consumed by `droneai`
- `core/Firmware/esp-drone/components/core/crazyflie/modules/src/leader_signal_detector.c` - follower state logic
- `core/Firmware/esp-drone/QUICKSTART.md` - firmware setup notes
- `core/leader_esp32/leader_esp32_code/main.c` - leader signal broadcast flow

## Folder Layout

| Path | Purpose |
| --- | --- |
| `core/ESP32-CAM-DroneAI` | Camera capture, Wi-Fi, web streaming components |
| `core/Firmware/esp-drone` | Main flight/follower firmware stack |
| `core/leader_esp32` | Leader transmitter side firmware |

## Cleanup Notes

- Removed large images, PDFs, GIFs, and demo/presentation extras
- Removed build artifacts and nested repository metadata
- Kept only source and setup artifacts required for technical review

## Scope

Research-prototype firmware package intended for controlled academic evaluation.
