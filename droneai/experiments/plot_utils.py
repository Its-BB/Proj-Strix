"""Shared matplotlib helpers for experiment figures."""

from __future__ import annotations

from typing import List, Sequence, Tuple

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.cm import ScalarMappable
from matplotlib.colors import Normalize
from matplotlib.lines import Line2D


def format_config_label(name: str) -> str:
    """Human-readable short label for ablation config keys."""
    return (
        name.replace("single_pass", "single pass")
        .replace("plus_", "+ ")
        .replace("_", " ")
    )


def plot_pr_ablation(
    ax,
    names: Sequence[str],
    recall: Sequence[float],
    precision: Sequence[float],
    latency_ms: Sequence[float],
    *,
    title: str,
    xlabel: str = "Recall (IoU=0.5, full test split)",
    cmap: str = "viridis",
) -> None:
    """
    Precision--recall scatter with color = latency and legend (no overlapping point labels).
    """
    rec = np.asarray(recall, dtype=float)
    prec = np.asarray(precision, dtype=float)
    lat = np.asarray(latency_ms, dtype=float)
    labels = [format_config_label(n) for n in names]

    norm = Normalize(vmin=float(lat.min()), vmax=float(lat.max()))
    sm = ScalarMappable(cmap=cmap, norm=norm)
    sm.set_array([])

    sizes = 80 + 40 * (lat - lat.min()) / (lat.max() - lat.min() + 1e-6)
    colors = sm.to_rgba(lat)

    ax.scatter(
        rec,
        prec,
        s=sizes,
        c=colors,
        edgecolors="#334155",
        linewidths=0.6,
        zorder=3,
    )

    handles = [
        Line2D(
            [0],
            [0],
            marker="o",
            color="w",
            markerfacecolor=colors[i],
            markeredgecolor="#334155",
            markeredgewidth=0.6,
            markersize=7,
            label=labels[i],
        )
        for i in range(len(names))
    ]
    ax.legend(
        handles=handles,
        loc="upper right",
        fontsize=6.5,
        framealpha=0.95,
        borderpad=0.4,
        labelspacing=0.35,
        handletextpad=0.5,
    )

    cbar = plt.colorbar(sm, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label("Latency (ms)", fontsize=8)

    ax.set_xlabel(xlabel, fontsize=9)
    ax.set_ylabel("Precision", fontsize=9)
    ax.set_title(title, fontsize=10)
    ax.grid(True, alpha=0.3, linestyle="--")
    ax.set_xlim(max(0.0, rec.min() - 0.04), min(1.0, rec.max() + 0.04))
    ax.set_ylim(max(0.0, prec.min() - 0.08), min(1.02, prec.max() + 0.03))
