# droneai - AI Runtime Stack

Python-side pipeline for video ingestion, detection, contextual analysis, and alert generation.

## Start Here

- `app.py` (primary runtime entry)
- `detection_system.py` (modular runtime variant)
- `config.yaml` (runtime configuration)

## Module Guide

| File | Role |
| --- | --- |
| `weapon_detector.py` | Multi-strategy weapon/object detection logic |
| `alert_system.py` | Threat scoring, warning generation, alert formatting |
| `local_llm_analyzer.py` | Optional local LLM scene interpretation via Ollama |
| `scene_analyzer.py` | Scene-level helper analysis |
| `fetch_video.py` | Stream acquisition utility |

## Training and Metrics (Retained)

- `weapon_training/train_weapon_model.py` - training script
- `weapon_training/results/weapon_detection2/results.csv` - epoch-level metrics used in paper tables
- `weapon_training/results/weapon_detection2/args.yaml` - saved run configuration
- `weapon_training/dataset_summary.md` - class and split summary for cleaned package

## Package Policy

- Bulky artifacts (weights, caches, raw dataset dumps, runtime temp outputs) are excluded
- This folder represents a reproducible research prototype snapshot, not a production service
