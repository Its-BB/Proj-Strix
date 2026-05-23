#!/usr/bin/env python3
"""Generate architecture figure for IEEE ISAIA submission."""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch

ROOT = Path(__file__).resolve().parents[2]
OUT_FIGURES = ROOT / "figures" / "isaia_arch.png"
OUT_DOCS = ROOT / "docs" / "architecture.png"

GAP = 0.7
BOX_H = 1.2
Y_TOP = 2.1
FONT_SIZE = 7

# face, edge, text
STYLES = {
    "edge": ("#ecfdf5", "#047857", "#064e3b"),
    "compute": ("#eff6ff", "#1d4ed8", "#1e3a8a"),
    "output": ("#fffbeb", "#b45309", "#78350f"),
    "control": ("#f5f3ff", "#6d28d9", "#4c1d95"),
}


def box(ax, x, y, w, h, text, style_key: str):
    face, edge, text_color = STYLES[style_key]
    ax.add_patch(
        FancyBboxPatch(
            (x, y),
            w,
            h,
            boxstyle="round,pad=0.05,rounding_size=0.12",
            linewidth=1.4,
            edgecolor=edge,
            facecolor=face,
            zorder=2,
        )
    )
    ax.text(
        x + w / 2,
        y + h / 2,
        text,
        ha="center",
        va="center",
        fontsize=FONT_SIZE,
        color=text_color,
        weight="bold",
        linespacing=1.15,
        zorder=3,
    )
    return x, y, w, h


def h_arrow(ax, x1, x2, y, color="#475569"):
    ax.add_patch(
        FancyArrowPatch(
            (x1 + 0.15, y),
            (x2 - 0.15, y),
            arrowstyle="-|>",
            mutation_scale=13,
            linewidth=1.5,
            color=color,
            shrinkA=0,
            shrinkB=0,
            zorder=1,
        )
    )


def branch_arrow(ax, start, end, color="#6d28d9"):
    ax.add_patch(
        FancyArrowPatch(
            start,
            end,
            arrowstyle="-|>",
            mutation_scale=12,
            linewidth=1.3,
            color=color,
            linestyle="--",
            connectionstyle="arc3,rad=0.12",
            shrinkA=6,
            shrinkB=6,
            zorder=1,
        )
    )


def main() -> int:
    pipeline = [
        ("ESP32-CAM\nQVGA MJPEG", 1.95, "edge"),
        ("Stream\ningest", 1.75, "compute"),
        ("YOLOv8\nbranches", 2.05, "compute"),
        ("Alerts +\nlogging", 1.75, "output"),
    ]

    fig, ax = plt.subplots(figsize=(10.8, 2.75))
    ax.set_axis_off()
    fig.patch.set_facecolor("white")

    placed = []
    x = 0.3
    for label, w, style in pipeline:
        placed.append(box(ax, x, Y_TOP, w, BOX_H, label, style))
        x += w + GAP

    cy = Y_TOP + BOX_H / 2
    for i in range(len(placed) - 1):
        x1, _, w1, _ = placed[i]
        x2, _, _, _ = placed[i + 1]
        h_arrow(ax, x1 + w1, x2, cy)

    x3, y3, w3, h3 = placed[2]
    fx = x3 + (w3 - 2.0) / 2
    fy = 0.3
    fw, fh = 2.0, 1.0
    box(ax, fx, fy, fw, fh, "ESP-NOW\nfollower FSM", "control")
    branch_arrow(
        ax,
        (x3 + w3 / 2, y3),
        (fx + fw / 2, fy + fh),
    )

    ax.set_xlim(0, x + 0.2)
    ax.set_ylim(0, Y_TOP + BOX_H + 0.35)

    for out in (OUT_FIGURES, OUT_DOCS):
        out.parent.mkdir(parents=True, exist_ok=True)
        fig.savefig(out, dpi=300, bbox_inches="tight", pad_inches=0.1, facecolor="white")
    fig.savefig(OUT_FIGURES.with_suffix(".pdf"), bbox_inches="tight", pad_inches=0.1, facecolor="white")
    plt.close(fig)
    print(f"Wrote {OUT_DOCS}")
    print(f"Wrote {OUT_FIGURES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
