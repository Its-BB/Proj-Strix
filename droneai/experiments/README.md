# Evaluation scripts

Reproducible benchmarks for the STRIX detection pipeline (full test split, latency, robustness, offline E2E).

**Guide:** [`../../docs/REPRODUCIBILITY.md`](../../docs/REPRODUCIBILITY.md)

## Prerequisites

```bash
cd droneai
pip install -r requirements.txt
```

| Requirement | Notes |
| --- | --- |
| `weapon_detection_custom.pt` | In `droneai/` root |
| `STRIX_DATASET_ROOT` | Path to YOLO dataset with `test/images` + `test/labels` |

```powershell
$env:STRIX_DATASET_ROOT="C:\path\to\guns-knives-yolo\guns-knives-yolo"
```

## Scripts

| Script | Purpose |
| --- | --- |
| `run_evaluation.py` | Quick ablation smoke test |
| `run_full_labs.py` | Full test-split ablation, latency, confusion plots, YOLO val |
| `run_robustness_and_stats.py` | Synthetic stress + bootstrap CIs |
| `run_final_push.py` | Offline E2E timing + pretrained vs. fine-tuned baselines |
| `generate_isaia_arch_figure.py` | Regenerate `docs/architecture.png` |

Paper update helpers (`update_paper_*.py`) are local-only and not required for reproduction.

## Recommended order

```bash
python experiments/run_full_labs.py --max-images 0 --latency-repeats 3
python experiments/run_robustness_and_stats.py --bootstrap 400
python experiments/run_final_push.py --e2e-frames 40 --bootstrap 0
```

## Outputs

Written to `experiments/results/` (gitignored):

- `full_labs_summary.json`
- `robustness_summary.json`
- `final_push_summary.json`
- `figures/*.png`

## CLI examples

```bash
python experiments/run_evaluation.py
python experiments/run_full_labs.py --max-images 50          # subset
python experiments/run_full_labs.py --max-images 0           # all 385 test images
python experiments/run_robustness_and_stats.py --bootstrap 400
python experiments/run_final_push.py --e2e-frames 40 --bootstrap 0
```
