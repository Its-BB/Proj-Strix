#!/usr/bin/env python3
"""Regenerate ablation figures from experiments/results/ablation_results.json."""

from __future__ import annotations

import json
from pathlib import Path

import matplotlib.pyplot as plt

from plot_utils import plot_pr_ablation

ROOT = Path(__file__).resolve().parent
RESULTS = ROOT / "results" / "ablation_results.json"
FIG_DIR = ROOT / "results" / "figures"
PAPER_FIG = Path(__file__).resolve().parents[2] / "figures"
DOCS_DEMO = Path(__file__).resolve().parents[2] / "docs" / "demo"


def main() -> int:
    if not RESULTS.exists():
        print(f"Missing {RESULTS}; run run_full_labs.py first.")
        return 1

    data = json.loads(RESULTS.read_text(encoding="utf-8"))
    rows = data["results"]
    n_img = data.get("test_image_count") or data.get("eval_image_count") or "?"
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    PAPER_FIG.mkdir(parents=True, exist_ok=True)
    DOCS_DEMO.mkdir(parents=True, exist_ok=True)

    labels = [r["config"] for r in rows]
    recall = [r["recall"] for r in rows]
    precision = [r["precision"] for r in rows]
    latency = [r["latency_mean_ms"] for r in rows]

    fig, ax = plt.subplots(figsize=(7.2, 5.2))
    plot_pr_ablation(
        ax,
        labels,
        recall,
        precision,
        latency,
        title="STRIX pipeline ablation (fine-tuned weights)",
        xlabel=f"Recall (IoU=0.5, {n_img}-image test subset)",
    )
    fig.tight_layout()
    for dest in (
        FIG_DIR / "ablation_pr_scatter.png",
        PAPER_FIG / "ablation_pr_scatter.png",
        DOCS_DEMO / "ablation_pr_scatter.png",
    ):
        fig.savefig(dest, dpi=150, bbox_inches="tight")
    plt.close(fig)

    fig2, ax2 = plt.subplots(figsize=(8, 4))
    ax2.bar([l.replace("_", "\n") for l in labels], latency, color="#2563eb")
    ax2.set_ylabel("Mean latency (ms)")
    ax2.set_title("Inference latency vs. enabled branches (QVGA)")
    plt.xticks(rotation=20, ha="right", fontsize=8)
    fig2.tight_layout()
    for dest in (
        FIG_DIR / "ablation_latency.png",
        PAPER_FIG / "ablation_latency.png",
        DOCS_DEMO / "ablation_latency.png",
    ):
        fig2.savefig(dest, dpi=150, bbox_inches="tight")
    plt.close(fig2)

    print(f"Saved figures to {FIG_DIR}, {PAPER_FIG}, and {DOCS_DEMO}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
