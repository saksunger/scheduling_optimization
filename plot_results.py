"""
plot_results.py — Generate report-ready plots from the benchmark CSVs.

Reads the CSV files produced by performance_test (in ./results) and writes PNG
figures into ./results/plots.

Experiment protocol behind the data: 4 fixed problem instances, 3 methods
(Harmony Search, HS + Local Search, Ant Colony Optimization), 8 parameter
configurations per method, every run terminated at exactly 2000 fitness
function evaluations (FFE), 10 independent runs per configuration.

Figures produced:
  1. convergence_harmony.png        - HS vs HS+LS best-fitness vs FFE, per instance
  2. convergence_ant_colony.png     - ACO best-fitness vs FFE, 8 configs, per instance
  3. convergence_best_hs_vs_aco.png - best config of each method, per instance
  4. fitness_comparison.png         - avg fitness of all configs x 3 methods, per instance
  5. accuracy_comparison.png        - relative accuracy vs best-known, per instance
  6. time_comparison.png            - avg computation time at equal FFE budget
  7. service_metrics.png            - per-UE-type latency & energy (best configs)
  8. fitness_distribution.png       - final fitness across 10 runs (box plots)

Usage:  python plot_results.py [results_dir]
"""

import csv
import sys
import warnings
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # headless: write files, never open a window
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parent
RESULTS = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else ROOT / "results"
PLOTS = RESULTS / "plots"
PLOTS.mkdir(parents=True, exist_ok=True)

INF_SENTINEL = 1e30          # values >= this are the FLT_MAX "infeasible" marker
N_INSTANCES = 4
N_CONFIGS = 8
BUDGET = 2000

METHODS = ["harmony", "harmony_ls", "ant_colony"]
METHOD_LABELS = {"harmony": "HS", "harmony_ls": "HS+LS", "ant_colony": "ACO"}
METHOD_COLORS = {"harmony": "#4C72B0", "harmony_ls": "#55A868", "ant_colony": "#DD8452"}

INSTANCE_LABELS = [
    "Instance 1 (baseline 100/250/250)",
    "Instance 2 (URLLC-heavy 200/200/200)",
    "Instance 3 (mMTC-heavy 60/180/360)",
    "Instance 4 (eMBB-heavy 80/350/170)",
]
INSTANCE_SHORT = ["Inst. 1", "Inst. 2", "Inst. 3", "Inst. 4"]

UE_COLS = ["URLLC", "EMBB", "MMTC"]    # exact column-name suffixes used in the CSVs
UE_LABELS = ["URLLC", "eMBB", "mMTC"]  # nicer labels for display


# --------------------------------------------------------------------------- #
# CSV helpers
# --------------------------------------------------------------------------- #
def read_dicts(path):
    """Read a CSV with a header row into a list of dicts (numbers as float)."""
    rows = []
    with open(path, newline="") as f:
        for d in csv.DictReader(f):
            row = {}
            for k, v in d.items():
                if k is None:
                    continue
                v = (v or "").strip()
                try:
                    row[k] = float(v)
                except ValueError:
                    row[k] = v
            rows.append(row)
    return rows


def read_ffe_matrix(method, inst, config):
    """Return (evaluations, matrix) where matrix is [n_rows x n_retest].

    FLT_MAX sentinels and blanks become NaN so they are skipped when plotting.
    """
    path = RESULTS / f"{method}_ffe_instance{inst}_config{config}.csv"
    xs, data = [], []
    with open(path, newline="") as f:
        reader = csv.reader(f)
        next(reader)  # header
        for row in reader:
            if not row or row[0].strip() == "":
                continue
            xs.append(float(row[0]))
            vals = []
            for cell in row[1:]:
                cell = cell.strip()
                if cell == "":
                    vals.append(np.nan)
                else:
                    val = float(cell)
                    vals.append(np.nan if val >= INF_SENTINEL else val)
            data.append(vals)
    return np.array(xs), np.array(data, dtype=float)


def save(fig, name):
    out = PLOTS / name
    fig.savefig(out, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  saved {out.relative_to(ROOT)}")


def annotate_bars(ax, bars, fmt="{:.0f}", dy=3):
    for b in bars:
        h = b.get_height()
        ax.annotate(fmt.format(h), (b.get_x() + b.get_width() / 2, h),
                    textcoords="offset points", xytext=(0, dy),
                    ha="center", va="bottom", fontsize=7)


# --------------------------------------------------------------------------- #
# Load aggregated data: AGG[method][instance] = list of 8 config rows
# --------------------------------------------------------------------------- #
AGG = {}
for method in METHODS:
    AGG[method] = {}
    for inst in range(1, N_INSTANCES + 1):
        AGG[method][inst] = read_dicts(
            RESULTS / f"{method}_aggregated_instance{inst}.csv")


def best_config(method, inst):
    """Index (0-based) of the configuration with the lowest average fitness."""
    rows = AGG[method][inst]
    return min(range(len(rows)), key=lambda i: rows[i]["AvgFitness"])


def best_known(inst):
    """Best (lowest-cost) solution found by any method on this instance."""
    return min(min(d["BestFitness"] for d in AGG[m][inst]) for m in METHODS)


def hs_config_label(row):
    lab = (f"C{int(row['Config'])} (HMS={int(row['HMS'])}, "
           f"HMCR={row['HMCR']:g}, PAR={row['PAR']:g}")
    lab += f", LSR={row['LSR']:g})" if row.get("LSR", 0) else ")"
    return lab


def aco_config_label(row):
    return (f"C{int(row['Config'])} (ants={int(row['NumAnts'])}, "
            f"a={row['PheroCoeff']:g}, b={row['HeurCoeff']:g}, "
            f"rho={row['Rho']:g})")


def mean_band(M):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", RuntimeWarning)
        return np.nanmean(M, axis=1), np.nanmin(M, axis=1), np.nanmax(M, axis=1)


# --------------------------------------------------------------------------- #
# 1. HS vs HS+LS convergence per instance (Local Search effect)
# --------------------------------------------------------------------------- #
def plot_convergence_hs_vs_ls(fname):
    fig, axes = plt.subplots(2, 2, figsize=(11, 7.5), sharex=True)
    for inst, ax in zip(range(1, N_INSTANCES + 1), axes.flat):
        for method in ["harmony", "harmony_ls"]:
            c = best_config(method, inst)
            row = AGG[method][inst][c]
            x, M = read_ffe_matrix(method, inst, c + 1)
            mean, lo, hi = mean_band(M)
            m = ~np.isnan(mean)
            label = f"{METHOD_LABELS[method]} best: {hs_config_label(row)}"
            ax.plot(x[m], mean[m], color=METHOD_COLORS[method], lw=2, label=label)
            ax.fill_between(x[m], lo[m], hi[m], color=METHOD_COLORS[method], alpha=0.15)
        ax.set_title(INSTANCE_LABELS[inst - 1], fontsize=10)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=7, loc="upper right")
    for ax in axes[1]:
        ax.set_xlabel("Fitness evaluations (FFE)")
    for ax in axes[:, 0]:
        ax.set_ylabel("Fitness (lower is better)")
    fig.suptitle("Harmony Search with vs without Local Search — convergence under "
                 f"an equal budget of {BUDGET} FFE\n(mean of 10 runs, band = min–max, "
                 "best configuration of each variant)", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 2. ACO convergence per instance, all 8 configs
# --------------------------------------------------------------------------- #
def plot_convergence_aco(fname):
    cmap = plt.cm.tab10
    fig, axes = plt.subplots(2, 2, figsize=(11, 7.5), sharex=True)
    for inst, ax in zip(range(1, N_INSTANCES + 1), axes.flat):
        for c in range(N_CONFIGS):
            x, M = read_ffe_matrix("ant_colony", inst, c + 1)
            mean, _, _ = mean_band(M)
            m = ~np.isnan(mean)
            ax.plot(x[m], mean[m], color=cmap(c), lw=1.6,
                    label=aco_config_label(AGG["ant_colony"][inst][c]))
        ax.set_title(INSTANCE_LABELS[inst - 1], fontsize=10)
        ax.grid(True, alpha=0.3)
        if inst == 1:
            ax.legend(fontsize=6, loc="upper right")
    for ax in axes[1]:
        ax.set_xlabel("Fitness evaluations (FFE)")
    for ax in axes[:, 0]:
        ax.set_ylabel("Fitness (lower is better)")
    fig.suptitle("Ant Colony Optimization — convergence of all 8 configurations under "
                 f"{BUDGET} FFE\n(mean of 10 runs; legend shown for Instance 1)", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 3. Head-to-head: best config of each method, per instance
# --------------------------------------------------------------------------- #
def plot_convergence_head_to_head(fname):
    fig, axes = plt.subplots(2, 2, figsize=(11, 7.5), sharex=True)
    for inst, ax in zip(range(1, N_INSTANCES + 1), axes.flat):
        for method in METHODS:
            c = best_config(method, inst)
            x, M = read_ffe_matrix(method, inst, c + 1)
            mean, lo, hi = mean_band(M)
            m = ~np.isnan(mean)
            ax.plot(x[m], mean[m], color=METHOD_COLORS[method], lw=2.2,
                    label=f"{METHOD_LABELS[method]} (best config C{c + 1})")
            ax.fill_between(x[m], lo[m], hi[m], color=METHOD_COLORS[method], alpha=0.12)
        ax.set_title(INSTANCE_LABELS[inst - 1], fontsize=10)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8, loc="upper right")
    for ax in axes[1]:
        ax.set_xlabel("Fitness evaluations (FFE)")
    for ax in axes[:, 0]:
        ax.set_ylabel("Fitness (lower is better)")
    fig.suptitle("HS vs HS+LS vs ACO — convergence under an equal budget of "
                 f"{BUDGET} FFE\n(mean of 10 runs, band = min–max, best configuration "
                 "of each method)", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 4. Fitness comparison: all configs x 3 methods, per instance
# --------------------------------------------------------------------------- #
def fitness_errors(rows):
    """Asymmetric error bars: [avg-best, worst-avg]."""
    lo = [d["AvgFitness"] - d["BestFitness"] for d in rows]
    hi = [d["WorstFitness"] - d["AvgFitness"] for d in rows]
    return np.array([lo, hi])


def plot_fitness_comparison(fname):
    x = np.arange(N_CONFIGS)
    w = 0.26
    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    for inst, ax in zip(range(1, N_INSTANCES + 1), axes.flat):
        ymin, ymax = np.inf, -np.inf
        for k, method in enumerate(METHODS):
            rows = AGG[method][inst]
            avg = [d["AvgFitness"] for d in rows]
            ax.bar(x + (k - 1) * w, avg, w, yerr=fitness_errors(rows), capsize=2,
                   color=METHOD_COLORS[method], label=METHOD_LABELS[method],
                   error_kw=dict(lw=0.8))
            ymin = min(ymin, min(d["BestFitness"] for d in rows))
            ymax = max(ymax, max(d["WorstFitness"] for d in rows))
        ax.set_ylim(ymin * 0.995, ymax * 1.005)  # zoom to make differences visible
        ax.set_xticks(x)
        ax.set_xticklabels([f"C{i + 1}" for i in range(N_CONFIGS)], fontsize=8)
        ax.set_title(INSTANCE_LABELS[inst - 1], fontsize=10)
        ax.grid(True, axis="y", alpha=0.3)
        if inst == 1:
            ax.legend(fontsize=8)
    for ax in axes[:, 0]:
        ax.set_ylabel("Avg fitness (lower is better)")
    fig.suptitle("Average fitness of every configuration under an equal budget of "
                 f"{BUDGET} FFE\n(error bars span best–worst of 10 runs; y-axis zoomed)",
                 fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 5. Accuracy comparison (relative to per-instance best-known solution)
# --------------------------------------------------------------------------- #
def plot_accuracy_comparison(fname):
    x = np.arange(N_INSTANCES)
    w = 0.26
    fig, ax = plt.subplots(figsize=(9, 5.5))
    lo_val = 100.0
    for k, method in enumerate(METHODS):
        vals = []
        for inst in range(1, N_INSTANCES + 1):
            c = best_config(method, inst)
            vals.append(best_known(inst) / AGG[method][inst][c]["AvgFitness"] * 100)
        b = ax.bar(x + (k - 1) * w, vals, w, color=METHOD_COLORS[method],
                   label=METHOD_LABELS[method])
        annotate_bars(ax, b, fmt="{:.2f}")
        lo_val = min(lo_val, min(vals))
    ax.set_ylim(lo_val - 2, 100.8)
    ax.axhline(100, color="gray", ls="--", lw=1)
    ax.set_xticks(x)
    ax.set_xticklabels(INSTANCE_SHORT)
    ax.set_ylabel("Relative accuracy (%)  —  higher is better")
    ax.set_title("Solution accuracy of the best configuration of each method\n"
                 "accuracy = best_known(instance) / avg_fitness x 100")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.3)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 6. Computation time at equal FFE budget
# --------------------------------------------------------------------------- #
def plot_time_comparison(fname):
    x = np.arange(N_INSTANCES)
    w = 0.26
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for k, method in enumerate(METHODS):
        vals = []
        for inst in range(1, N_INSTANCES + 1):
            rows = AGG[method][inst]
            vals.append(np.mean([d["AvgTime(ms)"] for d in rows]))
        b = ax.bar(x + (k - 1) * w, vals, w, color=METHOD_COLORS[method],
                   label=METHOD_LABELS[method])
        annotate_bars(ax, b, fmt="{:.0f}")
    ax.set_xticks(x)
    ax.set_xticklabels(INSTANCE_SHORT)
    ax.set_ylabel("Avg computation time per run (ms)")
    ax.set_title(f"Average computation time per run at an equal budget of {BUDGET} FFE\n"
                 "(mean over the 8 configurations and 10 runs)")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.3)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 7. Service-specific metrics (best config of each method, instance 1)
# --------------------------------------------------------------------------- #
def plot_service_metrics(fname):
    inst = 1
    x = np.arange(len(UE_COLS))
    w = 0.26
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    for k, method in enumerate(METHODS):
        row = AGG[method][inst][best_config(method, inst)]
        lat = [row[f"AvgLatency{t}"] for t in UE_COLS]
        en = [row[f"AvgEnergy{t}"] for t in UE_COLS]

        b1 = ax1.bar(x + (k - 1) * w, lat, w, color=METHOD_COLORS[method],
                     label=METHOD_LABELS[method])
        annotate_bars(ax1, b1, fmt="{:.1f}")
        b2 = ax2.bar(x + (k - 1) * w, en, w, color=METHOD_COLORS[method],
                     label=METHOD_LABELS[method])
        annotate_bars(ax2, b2, fmt="{:.1f}")

    ax1.set_yscale("log")
    ax1.set_xticks(x); ax1.set_xticklabels(UE_LABELS)
    ax1.set_ylabel("Avg latency (time slots, log scale)")
    ax1.set_title("Average latency per UE type")
    ax1.legend(); ax1.grid(True, axis="y", which="both", alpha=0.3)

    ax2.set_xticks(x); ax2.set_xticklabels(UE_LABELS)
    ax2.set_ylabel("Avg energy")
    ax2.set_title("Average energy per UE type")
    ax2.legend(); ax2.grid(True, axis="y", alpha=0.3)

    fig.suptitle("Service-specific metrics — best configuration of each method "
                 "(Instance 1)", fontsize=12)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 8. Final-fitness distribution across the 10 runs
# --------------------------------------------------------------------------- #
def final_values(method, inst, config):
    _, M = read_ffe_matrix(method, inst, config)
    last = M[-1]
    return last[~np.isnan(last)]


def plot_fitness_distribution(fname):
    fig, axes = plt.subplots(2, 2, figsize=(11, 7.5))
    for inst, ax in zip(range(1, N_INSTANCES + 1), axes.flat):
        data, labels, colors = [], [], []
        for method in METHODS:
            c = best_config(method, inst)
            data.append(final_values(method, inst, c + 1))
            labels.append(f"{METHOD_LABELS[method]}\n(C{c + 1})")
            colors.append(METHOD_COLORS[method])
        bp = ax.boxplot(data, tick_labels=labels, patch_artist=True,
                        medianprops=dict(color="black"))
        for box, color in zip(bp["boxes"], colors):
            box.set(facecolor=color, alpha=0.6)
        ax.set_title(INSTANCE_LABELS[inst - 1], fontsize=10)
        ax.grid(True, axis="y", alpha=0.3)
    for ax in axes[:, 0]:
        ax.set_ylabel("Final fitness (lower is better)")
    fig.suptitle("Distribution of final fitness over 10 independent runs at "
                 f"{BUDGET} FFE\n(best configuration of each method)", fontsize=12)
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    save(fig, fname)


# --------------------------------------------------------------------------- #
def main():
    print(f"Reading CSVs from : {RESULTS}")
    print(f"Writing plots to  : {PLOTS}")
    for inst in range(1, N_INSTANCES + 1):
        print(f"  Instance {inst}: best known = {best_known(inst):.1f}")

    plot_convergence_hs_vs_ls("convergence_harmony.png")
    plot_convergence_aco("convergence_ant_colony.png")
    plot_convergence_head_to_head("convergence_best_hs_vs_aco.png")
    plot_fitness_comparison("fitness_comparison.png")
    plot_accuracy_comparison("accuracy_comparison.png")
    plot_time_comparison("time_comparison.png")
    plot_service_metrics("service_metrics.png")
    plot_fitness_distribution("fitness_distribution.png")

    print("\nDone.")


if __name__ == "__main__":
    main()
