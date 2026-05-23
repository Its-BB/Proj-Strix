# Evaluation scripts

Reproducible benchmarks for the STRIX detection pipeline.

## Prerequisites

```bash
cd droneai
pip install -r requirements.txt
```

- `weapon_detection_custom.pt` in `droneai/`
- Dataset path:

```powershell
$env:STRIX_DATASET_ROOT="C:\path\to\guns-knives-yolo\guns-knives-yolo"
```

## Scripts

| Script | Purpose |
| --- | --- |
| `run_evaluation.py` | Quick ablation subset |
| `run_full_labs.py` | Full test-split ablation, latency, YOLO val |
| `run_final_push.py` | Offline E2E timing and baselines |
| `run_robustness_and_stats.py` | Synthetic stress tests and bootstrap CIs |

## Examples

```bash
python experiments/run_evaluation.py
python experiments/run_full_labs.py --max-images 0 --latency-repeats 3
python experiments/run_robustness_and_stats.py --bootstrap 400
python experiments/run_final_push.py --e2e-frames 40 --bootstrap 0
```

Outputs are written under `experiments/results/` (local only, not versioned).
