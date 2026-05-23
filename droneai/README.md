# droneai — near-edge AI runtime

Python pipeline for MJPEG ingest, multi-branch YOLOv8 weapon detection, cooldown-gated alerts, and optional Ollama scene summaries.

**Parent repo:** [PROJECT STRIX](../README.md) · **Eval:** [experiments/README.md](experiments/README.md) · **Training:** [weapon_training/](weapon_training/)

## Architecture (software)

```text
fetch_video / stream URL  →  weapon_detector (branches)  →  alert_system
                         ↘  scene_analyzer / local_llm_analyzer (optional)
```

See the full system diagram: [`../docs/architecture.png`](../docs/architecture.png).

## Entry points

| File | Purpose |
| --- | --- |
| `app.py` | Primary runtime |
| `detection_system.py` | Modular runtime variant |
| `config.yaml` | Stream URL, thresholds, feature flags |

## Core modules

| Module | Role |
| --- | --- |
| `weapon_detector.py` | Multi-strategy detection and fusion |
| `alert_system.py` | Threat scoring and alert output |
| `local_llm_analyzer.py` | Optional Ollama summaries |
| `scene_analyzer.py` | Scene-level helpers |
| `fetch_video.py` | Stream acquisition |

## Setup

```bash
cd droneai
python -m venv .venv
# activate, then:
pip install -r requirements.txt
```

1. Copy **`weapon_detection_custom.pt`** into this directory (not versioned).
2. Edit **`config.yaml`** — set `stream_url` to your ESP32-CAM `http://<ip>/stream` or a local test source.
3. Run: `python app.py`

## Training

```bash
cd weapon_training
python train_weapon_model.py
```

- Dataset layout and split stats: [`weapon_training/dataset_summary.md`](weapon_training/dataset_summary.md)
- Full guide: [`../docs/TRAINING.md`](../docs/TRAINING.md)

## Evaluation (paper benchmarks)

```powershell
$env:STRIX_DATASET_ROOT="C:\path\to\guns-knives-yolo\guns-knives-yolo"
python experiments/run_full_labs.py --max-images 0 --latency-repeats 3
```

See [`experiments/README.md`](experiments/README.md) and [`../docs/REPRODUCIBILITY.md`](../docs/REPRODUCIBILITY.md).

## Local files (gitignored)

| Path | Reason |
| --- | --- |
| `weapon_detection_custom.pt` | Model weights |
| `weapon_training/dataset/` | Raw images |
| `runs/`, `experiments/results/` | Run outputs |
| `detections/` | Runtime captures |
