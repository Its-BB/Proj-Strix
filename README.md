# PROJECT STRIX

Low-cost edge-AI prototype for aerial safety monitoring: ESP32-CAM streaming, near-edge Python inference, and ESP-Drone follower firmware.

## Repository layout

| Folder | Purpose |
| --- | --- |
| `droneai/` | Python runtime: detection, alerts, optional local LLM, training and evaluation scripts |
| `ESP-Drone/` | Embedded stack: ESP32-CAM MJPEG stream, follower FSM, leader ESP-NOW |

## Quick start

1. Read `droneai/README.md` for the inference pipeline and dependencies.
2. Read `ESP-Drone/README.md` for firmware paths and build notes.
3. Place trained weights locally (`weapon_detection_custom.pt` in `droneai/`) and set `STRIX_DATASET_ROOT` for evaluation scripts (see `droneai/experiments/README.md`).

## Requirements

- Python 3.10+ and packages in `droneai/requirements.txt`
- ESP-IDF toolchain for firmware builds (ESP-Drone side)

## License and scope

Research prototype for controlled evaluation. Not intended as a production deployment system.
