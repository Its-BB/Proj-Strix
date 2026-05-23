# Dataset and training

## Dataset

PROJECT STRIX uses a **two-class** weapon detection dataset in YOLO format:

| Class ID | Name |
| --- | --- |
| 0 | `knife` |
| 1 | `pistol` |

**Reference split sizes** (from the training run documented in the paper):

| Split | Images |
| --- | --- |
| Train | 4409 |
| Validation | 1043 |
| Test | 385 |

Raw images and label files are **not** redistributed in this repository. Obtain a compatible public dataset (e.g. guns/knives YOLO on Kaggle) and place it under `droneai/weapon_training/dataset/` or set paths in your `data.yaml`.

See also: [`droneai/weapon_training/dataset_summary.md`](../droneai/weapon_training/dataset_summary.md).

## Training script

```bash
cd droneai/weapon_training
pip install -r ../requirements.txt
python train_weapon_model.py
```

Default configuration in the reference run:

| Parameter | Value |
| --- | --- |
| Model | YOLOv8n |
| Epochs | 10 |
| Image size | 640 |
| Batch size | 8 |
| Device | CPU |

Metrics and checkpoints are written to `weapon_training/results/` (gitignored).

## Using trained weights

Copy the best checkpoint to the runtime root:

```text
droneai/weapon_detection_custom.pt
```

Both `app.py` and `experiments/*.py` expect this filename unless overridden in `config.yaml`.

## Evaluation vs. training metrics

- **Ultralytics val/test mAP** — integrated metric from `model.val()` / test split
- **Pipeline ablation P/R** — box-level IoU 0.5 matching in `experiments/run_full_labs.py` (reflects composed runtime, not identical to mAP)

Report both when comparing to the paper tables.

## Notes

- Single seed / 10-epoch run in the reference paper — repeat training for rigorous comparison
- Optional shape/gun-pattern heuristics in `weapon_detector.py` are inactive when using the fine-tuned custom checkpoint
