#!/usr/bin/env python3
"""Generate architecture figure for IEEE ISAIA submission."""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

ROOT = Path(__file__).resolve().parents[2]
OUT_FIGURES = ROOT / "figures" / "isaia_arch.png"
OUT_DOCS = ROOT / "docs" / "architecture.png"

# Layout constants (data coordinates)
GAP = 0.65
BOX_H = 1.25
Y_TOP = 2.15
FONT_SIZE = 7.5


def box(ax, x, y, w, h, text):
    ax.add_patch(
        FancyBboxPatch(
            (x, y),
            w,
            h,
            boxstyle="round,pad=0.04,rounding_size=0.1",
            linewidth=1.2,
            edgecolor="#1e40af",
            facecolor="#eff6ff",
            clip_on=False,
        )
    )
    ax.text(
        x + w / 2,
        y + h / 2,
        text,
        ha="center",
        va="center",
        fontsize=FONT_SIZE,
        weight="bold",
        linespacing=1.2,
        clip_on=True,
        zorder=3,
    )
    return x, y, w, h


def h_arrow(ax, x1, x2, y, pad=0.12):
    ax.add_patch(
        FancyArrowPatch(
            (x1 + pad, y),
            (x2 - pad, y),
            arrowstyle="-|>",
            mutation_scale=11,
            linewidth=1.2,
            color="#334155",
            shrinkA=0,
            shrinkB=0,
        )
    )


def main() -> int:
    pipeline = [
        ("ESP32-CAM\nQVGA MJPEG", 2.05),
        ("Decode +\nstream ingest", 2.05),
        ("Multi-branch\nYOLOv8", 2.25),
        ("Alert +\nlogging", 1.85),
    ]

    fig, ax = plt.subplots(figsize=(10.5, 2.85))
    ax.set_axis_off()

    placed = []
    x = 0.25
    for label, w in pipeline:
        bx = box(ax, x, Y_TOP, w, BOX_H, label)
        placed.append(bx)
        x += w + GAP

    cy = Y_TOP + BOX_H / 2
    for i in range(len(placed) - 1):
        x1, _, w1, _ = placed[i]
        x2, _, _, _ = placed[i + 1]
        h_arrow(ax, x1 + w1, x2, cy)

    # Follower branch below YOLO (index 2)
    x3, _, w3, _ = placed[2]
    fx = x3 + (w3 - 2.1) / 2
    fy = 0.35
    fw, fh = 2.1, 1.05
    box(ax, fx, fy, fw, fh, "ESP-NOW\nfollower FSM")
    ax.add_patch(
        FancyArrowPatch(
            (x3 + w3 / 2, Y_TOP),
            (fx + fw / 2, fy + fh),
            arrowstyle="-|>",
            mutation_scale=11,
            linewidth=1.2,
            color="#334155",
            connectionstyle="arc3,rad=0.0",
            shrinkA=4,
            shrinkB=4,
        )
    )

    margin = 0.35
    ax.set_xlim(-margin, x + margin)
    ax.set_ylim(0, Y_TOP + BOX_H + 0.45)

    for out in (OUT_FIGURES, OUT_DOCS):
        out.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out, dpi=300, bbox_inches="tight", pad_inches=0.08, facecolor="white")
    pdf = OUT_FIGURES.with_suffix(".pdf")
    fig.savefig(pdf, bbox_inches="tight", pad_inches=0.08, facecolor="white")
    plt.close(fig)
    print(f"Wrote {OUT_DOCS}")
    print(f"Wrote {OUT_FIGURES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
