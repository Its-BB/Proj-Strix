# Reproducibility guide

This document describes how to reproduce the quantitative results referenced in the PROJECT STRIX paper using this repository.

## 1. Environment

```bash
cd droneai
python -m venv .venv
# activate venv, then:
pip install -r requirements.txt
```

Tested on **Python 3.10+** with CPU inference (AMD Ryzen class workstation). GPU is optional for training only.

## 2. Artifacts not in Git

| Item | Location | How to obtain |
| --- | --- | --- |
| Fine-tuned weights | `droneai/weapon_detection_custom.pt` | Train locally (see [TRAINING.md](TRAINING.md)) or use your own YOLOv8n fine-tune |
| Guns/knives dataset | `STRIX_DATASET_ROOT` | Download a YOLO-format guns/knives dataset (e.g. Kaggle) with `train/`, `valid/`, `test/` splits |
| Result JSON/CSV | `droneai/experiments/results/` | Produced by scripts below |

## 3. Dataset path

The evaluation scripts search for a layout like:

```text
guns-knives-yolo/
  train/images/   train/labels/
  valid/images/   valid/labels/
  test/images/    test/labels/
```

Set the environment variable to the inner folder that contains `data.yaml` or the split directories:

```powershell
$env:STRIX_DATASET_ROOT="C:\data\guns-knives-yolo\guns-knives-yolo"
```

```bash
export STRIX_DATASET_ROOT=/data/guns-knives-yolo/guns-knives-yolo
```

## 4. Script order

Run from `droneai/`:

| Step | Command | Output |
| --- | --- | --- |
| Full ablation + latency + YOLO val | `python experiments/run_full_labs.py --max-images 0 --latency-repeats 3` | `experiments/results/full_labs_summary.json` |
| Robustness + bootstrap CIs | `python experiments/run_robustness_and_stats.py --bootstrap 400` | `experiments/results/robustness_summary.json` |
| Offline E2E timing | `python experiments/run_final_push.py --e2e-frames 40 --bootstrap 0` | `experiments/results/final_push_summary.json` |
| Quick smoke test | `python experiments/run_evaluation.py` | subset metrics |

Figures are written under `experiments/results/figures/` (local only).

## 5. Configuration knobs

- **Input size:** QVGA (320×240) for latency/ablation in the reference runs
- **IoU threshold:** 0.5 for pipeline precision/recall matching
- **Latency:** mean of `--latency-repeats` (default 3) per ablation configuration
- **E2E surrogate:** MJPEG encode/decode round-trip without physical ESP32 (`run_final_push.py`)

## 6. Firmware integration (qualitative)

1. Flash ESP32-CAM stream firmware (`ESP-Drone/core/ESP32-CAM-DroneAI`).
2. Point `droneai/config.yaml` stream URL to `http://<esp32-ip>/stream`.
3. Flash follower firmware per `ESP-Drone/core/Firmware/esp-drone/QUICKSTART.md`.

No automated CI covers hardware-in-the-loop tests.

## 7. Regenerate architecture diagram

```bash
python droneai/experiments/generate_isaia_arch_figure.py
```

Updates `docs/architecture.png` and local `figures/isaia_arch.png` (figures/ is not versioned).
