# PROJECT STRIX

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Near-edge **systems prototype** for composable aerial sensing: ESP32-CAM MJPEG streaming, multi-branch YOLOv8 inference on a workstation, structured alerts, and ESP-NOW-gated follower firmware. This repository supports reproducibility of a **research systems study** (offline benchmarks + MJPEG timing surrogate)—not a field-deployed product.

> **Scope:** Controlled academic evaluation only. No operational security or autonomous-intervention claims.

## Architecture

![STRIX system architecture](docs/architecture.png)

| Layer | Color in diagram | Component |
| --- | --- | --- |
| Edge | Green | ESP32-CAM QVGA MJPEG `/stream` |
| Compute | Blue | Python ingest + YOLOv8 branches |
| Output | Amber | Alerts + JSON logs |
| Control | Purple (dashed) | ESP-NOW follower FSM |

Regenerate: `python droneai/experiments/generate_isaia_arch_figure.py`

## Installation

```bash
git clone https://github.com/Its-BB/Proj-Strix.git
cd Proj-Strix/droneai
python -m venv .venv
```

**Windows:** `.venv\Scripts\activate`  
**Linux/macOS:** `source .venv/bin/activate`

```bash
pip install -r requirements.txt
```

Copy fine-tuned weights to `droneai/weapon_detection_custom.pt` (see [docs/MODEL.md](docs/MODEL.md)).

## Run inference

```bash
cd droneai
# Edit config.yaml — set stream_url to http://<esp32-ip>/stream
python app.py
```

Full steps: **[docs/INFERENCE.md](docs/INFERENCE.md)**

## Reproduce paper benchmarks

```powershell
$env:STRIX_DATASET_ROOT="C:\path\to\guns-knives-yolo\guns-knives-yolo"
```

```bash
cd droneai
python experiments/run_full_labs.py --max-images 0 --latency-repeats 3
python experiments/run_robustness_and_stats.py --bootstrap 400
python experiments/run_final_push.py --e2e-frames 40 --bootstrap 0
```

| Doc | Contents |
| --- | --- |
| [docs/REPRODUCIBILITY.md](docs/REPRODUCIBILITY.md) | Environment, script order, outputs |
| [docs/MODEL.md](docs/MODEL.md) | Weights, dataset source, metric definitions |
| [docs/TRAINING.md](docs/TRAINING.md) | Training command and splits |
| [droneai/experiments/README.md](droneai/experiments/README.md) | CLI flags |

## Sample outputs

| File | Description |
| --- | --- |
| [docs/demo/detection_example.jpg](docs/demo/detection_example.jpg) | Validation batch predictions |
| [docs/demo/ablation_latency.png](docs/demo/ablation_latency.png) | Latency vs. branches |
| [docs/demo/ablation_pr_scatter.png](docs/demo/ablation_pr_scatter.png) | Precision–recall trade-off |

## Repository layout

| Path | Description |
| --- | --- |
| `droneai/` | Python runtime, training, experiments |
| `ESP-Drone/` | ESP32-CAM + follower / leader firmware |
| `docs/` | Architecture, inference, model, reproducibility |

## Firmware

[ESP-Drone/README.md](ESP-Drone/README.md) · [QUICKSTART](ESP-Drone/core/Firmware/esp-drone/QUICKSTART.md)

## Requirements

- Python 3.10+ — `droneai/requirements.txt`
- ESP-IDF (firmware builds)
- Optional: [Ollama](https://ollama.com/) for local LLM summaries

## Contact

**Daiwikdharsh S** — daiwikshinoy@gmail.com

## License

MIT — [LICENSE](LICENSE). Third-party components retain their upstream licenses.
