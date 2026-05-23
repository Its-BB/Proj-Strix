#!/usr/bin/env python3
"""
Full lab suite: full test split, repeated latency, confusion matrices, YOLO val mAP.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import cv2
import numpy as np
import yaml

ROOT = Path(__file__).resolve().parents[1]
EXPERIMENTS = Path(__file__).resolve().parent
sys.path.insert(0, str(EXPERIMENTS))
sys.path.insert(0, str(ROOT))
os.chdir(ROOT)

from run_evaluation import (  # noqa: E402
    ABLATION_CONFIGS,
    benchmark_latency,
    find_dataset_root,
    list_eval_images,
    load_config,
    load_training_val_map50,
    match_detections,
    parse_yolo_labels,
    yolo_label_path,
)

RESULTS_DIR = Path(__file__).resolve().parent / "results"
FIGURES_DIR = RESULTS_DIR / "figures"
PAPER_FIG = Path(__file__).resolve().parents[2] / "figures"

CLASS_NAMES = {0: "knife", 1: "pistol"}


def evaluate_detailed(detector, images: List[Path]) -> dict:
    totals = {"tp": 0, "fp": 0, "fn": 0}
    per_class: Dict[int, Dict[str, int]] = {
        c: {"tp": 0, "fp": 0, "fn": 0} for c in CLASS_NAMES
    }
    for img_path in images:
        frame = cv2.imread(str(img_path))
        if frame is None:
            continue
        h, w = frame.shape[:2]
        gts = parse_yolo_labels(yolo_label_path(img_path), w, h)
        dets = detector.detect_weapons(frame)
        preds = [
            d["bbox"]
            for d in dets
            if d.get("is_weapon") or d.get("weapon_score", 0) > 0.3
        ]
        gt_tuples = [(c, x1, y1, x2, y2) for c, x1, y1, x2, y2 in gts]
        tp, fp, fn = match_detections(preds, gt_tuples)
        totals["tp"] += tp
        totals["fp"] += fp
        totals["fn"] += fn

        for cls_id in CLASS_NAMES:
            cls_gts = [g for g in gt_tuples if g[0] == cls_id]
            if not cls_gts and not preds:
                continue
            tpc, fpc, fnc = match_detections(preds, cls_gts)
            per_class[cls_id]["tp"] += tpc
            per_class[cls_id]["fp"] += fpc
            per_class[cls_id]["fn"] += fnc

    precision = totals["tp"] / (totals["tp"] + totals["fp"] + 1e-6)
    recall = totals["tp"] / (totals["tp"] + totals["fn"] + 1e-6)
    f1 = 2 * precision * recall / (precision + recall + 1e-6)
    class_metrics = {}
    for cls_id, name in CLASS_NAMES.items():
        t = per_class[cls_id]
        p = t["tp"] / (t["tp"] + t["fp"] + 1e-6)
        r = t["tp"] / (t["tp"] + t["fn"] + 1e-6)
        class_metrics[name] = {
            "precision": round(p, 4),
            "recall": round(r, 4),
            "tp": t["tp"],
            "fp": t["fp"],
            "fn": t["fn"],
        }
    return {
        "precision": round(precision, 4),
        "recall": round(recall, 4),
        "f1": round(f1, 4),
        "tp": totals["tp"],
        "fp": totals["fp"],
        "fn": totals["fn"],
        "per_class": class_metrics,
        "images_evaluated": len(images),
    }


def latency_with_stats(detector, frame: np.ndarray, repeats: int = 5) -> dict:
    runs = []
    for _ in range(repeats):
        runs.append(benchmark_latency(detector, frame, warmup=2, iters=15))
    means = [r["latency_mean_ms"] for r in runs]
    return {
        "latency_mean_ms": round(float(np.mean(means)), 1),
        "latency_std_ms": round(float(np.std(means)), 1),
        "latency_median_ms": round(float(np.median(means)), 1),
        "latency_p95_ms": round(float(max(r["latency_p95_ms"] for r in runs)), 1),
        "fps_approx": round(1000.0 / max(float(np.mean(means)), 1e-3), 2),
        "latency_repeats": repeats,
    }


def run_yolo_test_map(weights: Path, dataset_root: Path) -> Optional[dict]:
    yaml_path = RESULTS_DIR / "eval_dataset.yaml"
    cfg = {
        "path": str(dataset_root.resolve()).replace("\\", "/"),
        "train": "train/images",
        "val": "valid/images",
        "test": "test/images",
        "names": {0: "knife", 1: "pistol"},
        "nc": 2,
    }
    with open(yaml_path, "w", encoding="utf-8") as f:
        yaml.dump(cfg, f, default_flow_style=False)
    try:
        from ultralytics import YOLO

        model = YOLO(str(weights))
        metrics = model.val(data=str(yaml_path.resolve()), split="test", verbose=False)
        return {
            "map50": round(float(metrics.box.map50), 4),
            "map50_95": round(float(metrics.box.map), 4),
            "precision": round(float(metrics.box.mp), 4),
            "recall": round(float(metrics.box.mr), 4),
            "split": "test",
        }
    except Exception as exc:
        return {"error": str(exc)}


def plot_confusion(cm: dict, title: str, out: Path) -> None:
    import matplotlib.pyplot as plt

    labels = ["knife", "pistol"]
    data = np.array([[cm.get(l, {}).get("tp", 0) for l in labels]])  # simplified
    fig, axes = plt.subplots(1, 2, figsize=(8, 3))
    for ax, cls in zip(axes, labels):
        t = cm.get(cls, {})
        mat = np.array([[t.get("tp", 0), t.get("fn", 0)], [t.get("fp", 0), 0]])
        im = ax.imshow(mat, cmap="Blues")
        ax.set_title(cls)
        ax.set_xticks([0, 1])
        ax.set_xticklabels(["Pred+", "Missed"])
        ax.set_yticks([0, 1])
        ax.set_yticklabels(["GT+", "FP"])
        for i in range(2):
            for j in range(2):
                ax.text(j, i, int(mat[i, j]), ha="center", va="center")
        fig.colorbar(im, ax=ax, fraction=0.046)
    fig.suptitle(title)
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    plt.close(fig)


def plot_all_figures(ablation_rows: list, yolo_test: Optional[dict]) -> None:
    import matplotlib.pyplot as plt

    FIGURES_DIR.mkdir(parents=True, exist_ok=True)
    PAPER_FIG.mkdir(parents=True, exist_ok=True)

    names = [r["config"] for r in ablation_rows]
    lat = [r["latency_mean_ms"] for r in ablation_rows]
    lat_err = [r.get("latency_std_ms", 0) for r in ablation_rows]
    prec = [r["precision"] for r in ablation_rows]
    rec = [r["recall"] for r in ablation_rows]

    fig, ax = plt.subplots(figsize=(9, 4))
    x = np.arange(len(names))
    ax.bar(x, lat, yerr=lat_err, capsize=3, color="#2563eb")
    ax.set_xticks(x)
    ax.set_xticklabels([n.replace("_", "\n") for n in names], fontsize=8)
    ax.set_ylabel("Mean latency (ms)")
    ax.set_title("Full test-split ablation latency (QVGA, mean of 5 runs)")
    fig.tight_layout()
    for dest in (FIGURES_DIR / "full_ablation_latency.png", PAPER_FIG / "ablation_latency.png"):
        fig.savefig(dest, dpi=150)
    plt.close(fig)

    from plot_utils import plot_pr_ablation

    fig2, ax2 = plt.subplots(figsize=(7.2, 5.2))
    plot_pr_ablation(
        ax2,
        names,
        rec,
        prec,
        lat,
        title="Precision--recall trade-off (385 images)",
        xlabel="Recall (full test, IoU=0.5)",
    )
    fig2.tight_layout()
    for dest in (FIGURES_DIR / "full_ablation_pr_scatter.png", PAPER_FIG / "ablation_pr_scatter.png"):
        fig2.savefig(dest, dpi=150)
    plt.close(fig2)

    if yolo_test and "map50" in yolo_test:
        fig3, ax3 = plt.subplots(figsize=(4, 3))
        ax3.bar(["mAP@0.5", "mAP@0.5:0.95"], [yolo_test["map50"], yolo_test["map50_95"]], color=["#059669", "#10b981"])
        ax3.set_ylim(0, 1)
        ax3.set_title("YOLO val on test split")
        fig3.tight_layout()
        fig3.savefig(FIGURES_DIR / "yolo_test_map.png", dpi=150)
        fig3.savefig(PAPER_FIG / "yolo_test_map.png", dpi=150)
        plt.close(fig3)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-images", type=int, default=0, help="0 = all test images")
    parser.add_argument("--latency-repeats", type=int, default=5)
    parser.add_argument("--skip-yolo-val", action="store_true")
    parser.add_argument("--configs", nargs="*", default=None)
    args = parser.parse_args()

    from weapon_detector import WeaponDetector

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    config = load_config()
    dataset_root = find_dataset_root()
    if not dataset_root:
        print("ERROR: set STRIX_DATASET_ROOT to guns-knives-yolo folder")
        return 1

    max_n = args.max_images if args.max_images > 0 else 10_000
    images = list_eval_images(dataset_root, max_images=max_n)
    qvga = np.random.randint(0, 255, (240, 320, 3), dtype=np.uint8)
    weights = ROOT / "weapon_detection_custom.pt"
    if not weights.exists():
        alt = Path(r"C:\Users\Daiwik\Desktop\Strix\droneai\weapon_detection_custom.pt")
        if alt.exists():
            import shutil

            shutil.copy(alt, weights)

    configs = args.configs or list(ABLATION_CONFIGS.keys())
    rows = []
    print(f"Full lab: {len(images)} images, configs={configs}")

    yolo_test = None
    if not args.skip_yolo_val and weights.exists():
        print("Running Ultralytics test-split val...")
        yolo_test = run_yolo_test_map(weights, dataset_root)
        print(yolo_test)

    for name in configs:
        strategies = ABLATION_CONFIGS[name]
        print(f"=== {name} ===")
        detector = WeaponDetector(config, strategy_overrides=strategies)
        lat = latency_with_stats(detector, qvga, repeats=args.latency_repeats)
        det = evaluate_detailed(detector, images)
        row = {"config": name, **lat, **{k: det[k] for k in ("precision", "recall", "f1", "tp", "fp", "fn", "images_evaluated", "per_class")}}
        rows.append(row)
        if name in ("single_pass", "full_pipeline"):
            plot_confusion(
                det["per_class"],
                f"{name} per-class (test n={len(images)})",
                FIGURES_DIR / f"confusion_{name}.png",
            )
            dest = PAPER_FIG / f"confusion_{name}.png"
            import shutil

            shutil.copy(FIGURES_DIR / f"confusion_{name}.png", dest)

    out = {
        "dataset_root": str(dataset_root),
        "test_image_count": len(images),
        "training_val_map50_epoch10": load_training_val_map50(),
        "yolo_ultralytics_test": yolo_test,
        "latency_repeats": args.latency_repeats,
        "results": rows,
    }
    path = RESULTS_DIR / "full_lab_results.json"
    path.write_text(json.dumps(out, indent=2), encoding="utf-8")

    with open(RESULTS_DIR / "full_lab_results.csv", "w", newline="", encoding="utf-8") as f:
        fields = ["config", "precision", "recall", "f1", "latency_mean_ms", "latency_std_ms", "fps_approx", "images_evaluated"]
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k) for k in fields})

    plot_all_figures(rows, yolo_test)
    print(json.dumps(rows, indent=2))
    print(f"Wrote {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
