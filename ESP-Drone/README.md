# ESP-Drone — embedded stack

Firmware for **ESP32-CAM MJPEG streaming**, **ESP-NOW leader/follower signaling**, and safety-gated follower flight states. Pairs with the [`droneai`](../droneai/) Python runtime.

**System diagram:** [`../docs/architecture.png`](../docs/architecture.png)

## Start here

| Path | Role |
| --- | --- |
| [`core/ESP32-CAM-DroneAI/main/web_server.c`](core/ESP32-CAM-DroneAI/main/web_server.c) | HTTP MJPEG `/stream` for `droneai` |
| [`core/Firmware/esp-drone/components/core/crazyflie/modules/src/leader_signal_detector.c`](core/Firmware/esp-drone/components/core/crazyflie/modules/src/leader_signal_detector.c) | Follower FSM (IDLE → DETECTED → FOLLOWING → LOST → LANDING) |
| [`core/Firmware/esp-drone/QUICKSTART.md`](core/Firmware/esp-drone/QUICKSTART.md) | Build, flash, and bench steps |
| [`core/leader_esp32/leader_esp32_code/main.c`](core/leader_esp32/leader_esp32_code/main.c) | Leader ESP-NOW broadcaster |

## Folder layout

| Path | Purpose |
| --- | --- |
| `core/ESP32-CAM-DroneAI` | Camera, Wi-Fi, web streaming |
| `core/Firmware/esp-drone` | Follower flight stack + signal detector |
| `core/leader_esp32` | Leader transmitter firmware |

## Build (summary)

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).

**Follower:**

```bash
cd core/Firmware/esp-drone
idf.py build
idf.py -p PORT flash monitor
```

**Leader broadcaster:**

```bash
cd core/leader_esp32/leader_esp32_code
idf.py set-target esp32
idf.py build
idf.py -p PORT flash
```

See **QUICKSTART.md** for wiring, arming, and expected serial logs.

## Interface with droneai

1. Flash camera firmware and note the ESP32 IP address.
2. Verify stream in a browser: `http://<ip>/stream`
3. Set the same URL in `droneai/config.yaml`.
4. Weapon detections can drive follower states via ESP-NOW (see leader signal detector).

## Scope

Research-prototype firmware for controlled academic evaluation — not certified for autonomous operation in public spaces.
