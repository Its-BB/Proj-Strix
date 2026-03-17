# PROJECT STRIX - Reviewer Package

Clean, submission-oriented code package for academic review.

## Quick Navigation

- Start here: `droneai/README.md`
- Embedded side: `ESP-Drone/README.md`
- Paper metrics source: `droneai/weapon_training/results/weapon_detection2/results.csv`

## What This Package Contains

| Folder | Purpose |
| --- | --- |
| `droneai` | Python inference, threat analysis, and alerting pipeline |
| `ESP-Drone` | Embedded camera, follower firmware, and leader signaling code |

## Why It Is Structured This Way

- Reviewer-first layout with minimal noise
- Removed bulky artifacts (logs, caches, raw training dumps, media extras)
- Kept only code and reproducibility artifacts used by the paper

## Fast Technical Map

### `droneai`
- `app.py` / `detection_system.py`: runtime orchestration
- `weapon_detector.py`: multi-strategy detection
- `alert_system.py`: warning and alert flow
- `local_llm_analyzer.py`: optional local LLM scene analysis
- `weapon_training/`: training script + retained metrics/config summary

### `ESP-Drone`
- `core/ESP32-CAM-DroneAI/main/`: MJPEG stream and Wi-Fi logic
- `core/Firmware/esp-drone/.../leader_signal_detector.*`: follower state logic
- `core/leader_esp32/`: leader broadcast logic

## Reproducibility Notes

- Quantitative metrics in the paper are from:
  `droneai/weapon_training/results/weapon_detection2/results.csv`
- Training configuration snapshot:
  `droneai/weapon_training/results/weapon_detection2/args.yaml`
- Raw runtime logs and bulky training assets are intentionally excluded from this cleaned package

## Scope and Safety

This repository is a research prototype for controlled evaluation.
Claims are intentionally conservative and tied to retained artifacts.
