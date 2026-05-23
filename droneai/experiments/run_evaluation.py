#!/usr/bin/env python3
"""
Run ablation + latency benchmarks for PROJECT STRIX.
Outputs: experiments/results/ablation_results.csv, latency_results.csv, figures/
"""

from __future__ import annotations

import csv
import json
import os
import sys
import time
from pathlib import Path
from typing import List, Optional

import cv2
import numpy as np
import yaml

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
os.chdir(ROOT)

RESULTS_DIR = Path(__file__).resolve().parent / "results"
FIGURES_DIR = RESULTS_DIR / "figures"

ABLATION_CONFIGS = {
    "single_pass": {
        "single_pass_only": True,
        "multi_angle": False,
        "region_focus": False,
        "multi_scale": False,
        "shape": False,
        "gun_pattern": False,
        "context": False,
        "edge_enhanced": False,
    },
    "plus_multi_angle": {
        "single_pass_only": False,
        "multi_angle": True,
        "region_focus": False,
        "multi_scale": False,
        "shape": False,
        "gun_pattern": False,
        "context": False,
        "edge_enhanced": False,
    },
    "plus_region_focus": {
        "single_pass_only": False,
        "multi_angle": True,
        "region_focus": True,
        "multi_scale": False,
        "shape": False,
        "gun_pattern": False,
        "context": False,
        "edge_enhanced": False,
    },
    "plus_multi_scale": {
        "single_pass_only": False,
        "multi_angle": True,
        "region_focus": True,
        "multi_scale": True,
        "shape": False,
        "gun_pattern": False,
        "context": False,
        "edge_enhanced": False,
    },
    "plus_edge_enhanced": {
        "single_pass_only": False,
        "multi_angle": True,
        "region_focus": True,
        "multi_scale": True,
        "shape": False,
        "gun_pattern": False,
        "context": False,
        "edge_enhanced": True,
    },
    "full_pipeline": {
        "single_pass_only": False,
        "multi_angle": True,
        "region_focus": True,
        "multi_scale": True,
        "shape": True,
        "gun_pattern": True,
        "context": True,
        "edge_enhanced": True,
    },
}


def load_config() -> dict:
    with open(ROOT / "config.yaml", encoding="utf-8") as f:
        return yaml.safe_load(f)


def find_dataset_root() -> Optional[Path]:
    candidates = [
        ROOT / "weapon_training" / "dataset" / "guns-knives-yolo" / "guns-knives-yolo",
        Path(os.environ.get("STRIX_DATASET_ROOT", "")),
    ]
    for path in candidates:
        if path and path.exists() and (path / "test" / "images").exists():
            return path
    return None


def list_eval_images(dataset_root: Path, max_images: int = 80) -> List[Path]:
    test_dir = dataset_root / "test" / "images"
    images = sorted(test_dir.glob("*.jpg")) + sorted(test_dir.glob("*.png"))
    if not images:
        val_dir = dataset_root / "valid" / "images"
        images = sorted(val_dir.glob("*.jpg")) + sorted(val_dir.glob("*.png"))
    if max_images <= 0 or max_images >= len(images):
        return images
    return images[:max_images]


def yolo_label_path(image_path: Path) -> Path:
    parts = list(image_path.parts)
    if "images" in parts:
        idx = parts.index("images")
        parts[idx] = "labels"
        return Path(*parts).with_suffix(".txt")
    return image_path.with_suffix(".txt")


def parse_yolo_labels(label_path: Path, w: int, h: int) -> list[tuple]:
    if not label_path.exists():
        return []
    boxes = []
    for line in label_path.read_text(encoding="utf-8").strip().splitlines():
        if not line.strip():
            continue
        cls, xc, yc, bw, bh = map(float, line.split())
        x1 = int((xc - bw / 2) * w)
        y1 = int((yc - bh / 2) * h)
        x2 = int((xc + bw / 2) * w)
        y2 = int((yc + bh / 2) * h)
        boxes.append((int(cls), x1, y1, x2, y2))
    return boxes


def iou(a, b) -> float:
    ax1, ay1, ax2, ay2 = a
    bx1, by1, bx2, by2 = b
    ix1, iy1 = max(ax1, bx1), max(ay1, by1)
    ix2, iy2 = min(ax2, bx2), min(ay2, by2)
    if ix2 <= ix1 or iy2 <= iy1:
        return 0.0
    inter = (ix2 - ix1) * (iy2 - iy1)
    area_a = (ax2 - ax1) * (ay2 - ay1)
    area_b = (bx2 - bx1) * (by2 - by1)
    return inter / float(area_a + area_b - inter + 1e-6)


def match_detections(preds, gts, iou_thresh: float = 0.5) -> tuple[int, int, int]:
    tp = fp = 0
    matched = set()
    for pbox in preds:
        best_iou = 0.0
        best_j = -1
        for j, gbox in enumerate(gts):
            if j in matched:
                continue
            score = iou(pbox, gbox[1:])
            if score > best_iou:
                best_iou = score
                best_j = j
        if best_iou >= iou_thresh and best_j >= 0:
            tp += 1
            matched.add(best_j)
        else:
            fp += 1
    fn = len(gts) - len(matched)
    return tp, fp, fn


def benchmark_latency(detector, frame: np.ndarray, warmup: int = 3, iters: int = 20) -> dict:
    for _ in range(warmup):
        detector.detect_weapons(frame)
    times = []
    for _ in range(iters):
        t0 = time.perf_counter()
        detector.detect_weapons(frame)
        times.append((time.perf_counter() - t0) * 1000.0)
    times.sort()
    return {
        "latency_mean_ms": round(float(np.mean(times)), 1),
        "latency_median_ms": round(float(np.median(times)), 1),
        "latency_p95_ms": round(float(np.percentile(times, 95)), 1),
        "fps_approx": round(1000.0 / max(float(np.mean(times)), 1e-3), 2),
    }


def evaluate_detection_metrics(detector, images: list[Path]) -> dict:
    tp = fp = fn = 0
    for img_path in images:
        frame = cv2.imread(str(img_path))
        if frame is None:
            continue
        h, w = frame.shape[:2]
        labels = parse_yolo_labels(yolo_label_path(img_path), w, h)
        gts = [(cls, x1, y1, x2, y2) for cls, x1, y1, x2, y2 in labels]
        dets = detector.detect_weapons(frame)
        preds = [d["bbox"] for d in dets if d.get("is_weapon") or d.get("weapon_score", 0) > 0.3]
        t, f, n = match_detections(preds, gts)
        tp += t
        fp += f
        fn += n
    precision = tp / (tp + fp + 1e-6)
    recall = tp / (tp + fn + 1e-6)
    f1 = 2 * precision * recall / (precision + recall + 1e-6)
    return {
        "precision": round(precision, 4),
        "recall": round(recall, 4),
        "f1": round(f1, 4),
        "images_evaluated": len(images),
    }


def load_training_val_map50() -> Optional[float]:
    csv_path = ROOT / "weapon_training" / "results" / "weapon_detection2" / "results.csv"
    if not csv_path.exists():
        return None
    with open(csv_path, newline="", encoding="utf-8") as f:
        rows = [{k.strip(): v.strip() for k, v in r.items()} for r in csv.DictReader(f)]
    if not rows:
        return None
    last = rows[-1]
    for key in ("metrics/mAP50(B)", "metrics/mAP50 (B)"):
        if key in last and last[key]:
            return round(float(last[key]), 4)
    return None


def save_figures(rows: list[dict]) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        return
    FIGURES_DIR.mkdir(parents=True, exist_ok=True)
    names = [r["config"] for r in rows]
    latency = [r["latency_mean_ms"] for r in rows]
    plt.figure(figsize=(8, 4))
    plt.bar(names, latency, color="#2563eb")
    plt.ylabel("Mean latency (ms)")
    plt.title("STRIX ablation: inference latency (QVGA)")
    plt.xticks(rotation=25, ha="right")
    plt.tight_layout()
    plt.savefig(FIGURES_DIR / "ablation_latency.png", dpi=150)
    plt.close()

    if any(r.get("recall") is not None for r in rows):
        recall = [r.get("recall") or 0 for r in rows]
        plt.figure(figsize=(8, 4))
        plt.bar(names, recall, color="#059669")
        plt.ylabel("Recall (IoU=0.5, test subset)")
        plt.title("STRIX ablation: detection recall")
        plt.xticks(rotation=25, ha="right")
        plt.tight_layout()
        plt.savefig(FIGURES_DIR / "ablation_recall.png", dpi=150)
        plt.close()


def main() -> int:
    from weapon_detector import WeaponDetector

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    config = load_config()
    dataset_root = find_dataset_root()
    eval_images = list_eval_images(dataset_root) if dataset_root else []

    qvga = np.random.randint(0, 255, (240, 320, 3), dtype=np.uint8)
    training_map50 = load_training_val_map50()
    rows = []

    print(f"Dataset: {dataset_root or 'not found (latency-only + training val mAP for single-pass)'}")
    print(f"Eval images: {len(eval_images)}")

    for name, strategies in ABLATION_CONFIGS.items():
        print(f"Running {name}...")
        detector = WeaponDetector(config, strategy_overrides=strategies)
        lat = benchmark_latency(detector, qvga)
        row = {"config": name, **lat}
        if eval_images:
            metrics = evaluate_detection_metrics(detector, eval_images)
            row.update(metrics)
        elif name == "single_pass" and training_map50 is not None:
            row["val_map50_training_run"] = training_map50
            row["note"] = "mAP@0.50 from YOLOv8n fine-tune val (epoch 10, results.csv)"
        rows.append(row)

    csv_path = RESULTS_DIR / "ablation_results.csv"
    fieldnames = sorted({k for r in rows for k in r.keys()})
    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    with open(RESULTS_DIR / "ablation_results.json", "w", encoding="utf-8") as f:
        json.dump(
            {
                "dataset_root": str(dataset_root) if dataset_root else None,
                "eval_image_count": len(eval_images),
                "training_val_map50_epoch10": training_map50,
                "results": rows,
            },
            f,
            indent=2,
        )

    save_figures(rows)
    print(json.dumps(rows, indent=2))
    print(f"Wrote {csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
