# Model and dataset

## Detector

| Item | Value |
| --- | --- |
| Backbone | YOLOv8n (Ultralytics) |
| Classes | `knife`, `pistol` |
| Training | `droneai/weapon_training/train_weapon_model.py` |
| Deployed weights | `droneai/weapon_detection_custom.pt` (local only) |
| Reference run | 10 epochs, imgsz 640, batch 8, CPU |

## Dataset (external)

The repository does **not** ship raw images or labels.

| Split | Images (reference run) |
| --- | --- |
| Train | 4409 |
| Validation | 1043 |
| Test | 385 |

Obtain a compatible **guns/knives YOLO** dataset (e.g. Kaggle) with standard folder layout:

```text
guns-knives-yolo/
  train/images/   train/labels/
  valid/images/   valid/labels/
  test/images/    test/labels/
```

Set:

```powershell
$env:STRIX_DATASET_ROOT="C:\path\to\guns-knives-yolo\guns-knives-yolo"
```

## Metrics in the paper

- **Ultralytics test mAP** — `model.val()` on the test split (fine-tuned checkpoint)
- **Pipeline P/R** — box IoU 0.5 matching in `experiments/run_full_labs.py` (composed runtime)
- **COCO-pretrained YOLOv8n on test split** — reported only as an out-of-domain sanity check (mAP@0.50 $\approx$ 0.066), not a tuned baseline

See [TRAINING.md](TRAINING.md) for training commands.
