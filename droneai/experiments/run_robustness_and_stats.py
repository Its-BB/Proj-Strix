#!/usr/bin/env python3
"""Robustness stress tests + fast full-test-set bootstrap (no ESP32 required)."""

from __future__ import annotations

import json
import os
import random
import sys
import time
from pathlib import Path
from typing import Callable, Dict, List, Tuple

import cv2
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
os.chdir(ROOT)

from run_evaluation import (  # noqa: E402
    ABLATION_CONFIGS,
    find_dataset_root,
    list_eval_images,
    load_config,
    match_detections,
    parse_yolo_labels,
    yolo_label_path,
)

RESULTS_DIR = Path(__file__).resolve().parent / "results"
FIGURES_DIR = Path(__file__).resolve().parents[2] / "figures"


def apply_transform(frame: np.ndarray, name: str, rng: random.Random) -> np.ndarray:
    out = frame.copy()
    if name == "clean":
        return out
    if name == "low_light":
        return np.clip(out.astype(np.float32) * 0.45, 0, 255).astype(np.uint8)
    if name == "blur":
        return cv2.GaussianBlur(out, (7, 7), 1.8)
    if name == "jpeg_artifacts":
        ok, buf = cv2.imencode(".jpg", out, [int(cv2.IMWRITE_JPEG_QUALITY), 35])
        if not ok:
            return out
        dec = cv2.imdecode(buf, cv2.IMREAD_COLOR)
        return dec if dec is not None else out
    if name == "occlusion":
        h, w = out.shape[:2]
        bw = max(8, int(w * 0.22))
        bh = max(8, int(h * 0.22))
        x1 = rng.randint(0, max(1, w - bw))
        y1 = rng.randint(0, max(1, h - bh))
        cv2.rectangle(out, (x1, y1), (x1 + bw, y1 + bh), (0, 0, 0), -1)
        return out
    raise ValueError(name)


def eval_condition(
    detector,
    images: List[Path],
    condition: str,
    seed: int = 42,
) -> dict:
    rng = random.Random(seed)
    tp = fp = fn = 0
    t0 = time.perf_counter()
    for img_path in images:
        frame = cv2.imread(str(img_path))
        if frame is None:
            continue
        frame = apply_transform(frame, condition, rng)
        h, w = frame.shape[:2]
        gts = [
            (c, x1, y1, x2, y2)
            for c, x1, y1, x2, y2 in parse_yolo_labels(yolo_label_path(img_path), w, h)
        ]
        preds = [
            d["bbox"]
            for d in detector.detect_weapons(frame)
            if d.get("is_weapon") or d.get("weapon_score", 0) > 0.3
        ]
        t, f, nmiss = match_detections(preds, gts)
        tp += t
        fp += f
        fn += nmiss
    elapsed = time.perf_counter() - t0
    precision = tp / (tp + fp + 1e-6)
    recall = tp / (tp + fn + 1e-6)
    f1 = 2 * precision * recall / (precision + recall + 1e-6)
    return {
        "condition": condition,
        "precision": round(precision, 4),
        "recall": round(recall, 4),
        "f1": round(f1, 4),
        "images": len(images),
        "elapsed_s": round(elapsed, 1),
    }


def precompute_outcomes(detector, images: List[Path]) -> List[Tuple[int, int, int]]:
    outcomes: List[Tuple[int, int, int]] = []
    for img_path in images:
        frame = cv2.imread(str(img_path))
        if frame is None:
            outcomes.append((0, 0, 0))
            continue
        h, w = frame.shape[:2]
        gts = [
            (c, x1, y1, x2, y2)
            for c, x1, y1, x2, y2 in parse_yolo_labels(yolo_label_path(img_path), w, h)
        ]
        preds = [
            d["bbox"]
            for d in detector.detect_weapons(frame)
            if d.get("is_weapon") or d.get("weapon_score", 0) > 0.3
        ]
        outcomes.append(match_detections(preds, gts))
    return outcomes


def bootstrap_from_outcomes(
    outcomes: List[Tuple[int, int, int]],
    n_boot: int = 400,
    seed: int = 42,
) -> dict:
    rng = random.Random(seed)
    n = len(outcomes)
    if n == 0:
        return {"error": "no outcomes"}
    recalls, precisions, f1s = [], [], []
    for _ in range(n_boot):
        sample = [outcomes[rng.randint(0, n - 1)] for _ in range(n)]
        tp = sum(s[0] for s in sample)
        fp = sum(s[1] for s in sample)
        fn = sum(s[2] for s in sample)
        p = tp / (tp + fp + 1e-6)
        r = tp / (tp + fn + 1e-6)
        precisions.append(p)
        recalls.append(r)
        f1s.append(2 * p * r / (p + r + 1e-6))
    prec = np.array(precisions)
    rec = np.array(recalls)
    f1 = np.array(f1s)
    return {
        "test_images": n,
        "bootstrap_n": n_boot,
        "precision_mean": round(float(prec.mean()), 4),
        "precision_ci95": [round(float(np.percentile(prec, 2.5)), 4), round(float(np.percentile(prec, 97.5)), 4)],
        "recall_mean": round(float(rec.mean()), 4),
        "recall_ci95": [round(float(np.percentile(rec, 2.5)), 4), round(float(np.percentile(rec, 97.5)), 4)],
        "f1_mean": round(float(f1.mean()), 4),
        "f1_ci95": [round(float(np.percentile(f1, 2.5)), 4), round(float(np.percentile(f1, 97.5)), 4)],
    }


def plot_robustness(rows: List[dict]) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        return
    labels = [r["condition"] for r in rows]
    recall = [r["recall"] for r in rows]
    fig, ax = plt.subplots(figsize=(7, 4))
    colors = ["#16a34a" if c == "clean" else "#dc2626" for c in labels]
    ax.bar(labels, recall, color=colors)
    ax.set_ylabel("Recall (IoU=0.5)")
    ax.set_title("Single-pass recall under synthetic stress (test split)")
    ax.set_ylim(0, 1.0)
    plt.xticks(rotation=20, ha="right")
    fig.tight_layout()
    for d in (RESULTS_DIR / "figures", FIGURES_DIR):
        d.mkdir(parents=True, exist_ok=True)
        fig.savefig(d / "robustness_recall.png", dpi=150)
    plt.close(fig)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--max-images", type=int, default=0, help="0 = full test split")
    parser.add_argument("--bootstrap", type=int, default=400)
    parser.add_argument("--conditions", nargs="*", default=["clean", "low_light", "blur", "jpeg_artifacts", "occlusion"])
    args = parser.parse_args()

    os.environ["YOLO_VERBOSE"] = "False"
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    dataset_root = find_dataset_root()
    if not dataset_root:
        print("Set STRIX_DATASET_ROOT to guns-knives-yolo folder")
        return 1

    images = list_eval_images(dataset_root, max_images=args.max_images)
    config = load_config()
    weights = ROOT / "weapon_detection_custom.pt"
    if not weights.exists():
        alt = Path(r"C:\Users\Daiwik\Desktop\Strix\droneai\weapon_detection_custom.pt")
        if alt.exists():
            import shutil

            shutil.copy(alt, weights)
    if not weights.exists():
        print("Missing weapon_detection_custom.pt")
        return 1

    from weapon_detector import WeaponDetector

    detector = WeaponDetector(config, strategy_overrides=ABLATION_CONFIGS["single_pass"])

    print(f"Robustness ({len(images)} images)...")
    robustness = []
    for cond in args.conditions:
        print(f"  {cond}...")
        robustness.append(eval_condition(detector, images, cond))

    print("Precomputing per-image outcomes for bootstrap...")
    outcomes = precompute_outcomes(detector, images)
    print(f"Bootstrap n={args.bootstrap} on full test set...")
    bootstrap = bootstrap_from_outcomes(outcomes, n_boot=args.bootstrap)

    clean = next(r for r in robustness if r["condition"] == "clean")
    report = {
        "dataset_root": str(dataset_root),
        "test_images": len(images),
        "robustness_single_pass": robustness,
        "recall_drop_vs_clean": {
            r["condition"]: round(clean["recall"] - r["recall"], 4)
            for r in robustness
            if r["condition"] != "clean"
        },
        "bootstrap_single_pass_full_test": bootstrap,
        "literature_note": (
            "External papers use different datasets/splits; numbers are contextual only."
        ),
    }

    out = RESULTS_DIR / "robustness_and_stats.json"
    out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    plot_robustness(robustness)
    print(json.dumps(report, indent=2))
    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
