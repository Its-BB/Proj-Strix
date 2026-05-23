# droneai — AI runtime

Python pipeline for video ingest, multi-branch weapon detection, alerts, and optional local LLM scene analysis.

## Entry points

- `app.py` — primary runtime
- `detection_system.py` — modular runtime variant
- `config.yaml` — runtime configuration

## Core modules

| Module | Role |
| --- | --- |
| `weapon_detector.py` | Multi-strategy detection and fusion |
| `alert_system.py` | Threat scoring and alert output |
| `local_llm_analyzer.py` | Optional Ollama-based summaries |
| `scene_analyzer.py` | Scene-level helpers |
| `fetch_video.py` | Stream acquisition |

## Training

- `weapon_training/train_weapon_model.py`
- `weapon_training/dataset_summary.md` — class and split summary
- Run training locally; metrics are written under `weapon_training/results/` (not versioned)

## Evaluation

See `experiments/README.md` for ablation, latency, and robustness benchmarks.

## Local setup

```bash
pip install -r requirements.txt
```

Copy `weapon_detection_custom.pt` into this folder for detection and eval runs (not stored in the repository).
