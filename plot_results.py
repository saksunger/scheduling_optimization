"""
plot_results.py — Generate report-ready plots from the benchmark CSVs.

Reads the CSV files produced by performance_test.exe (moved into ./results) and
writes PNG figures into ./results/plots.

Figures produced:
  1. convergence_harmony.png        - HS best-fitness vs iteration (4 sets, min-max band)
  2. convergence_ant_colony.png     - ACO best-fitness vs iteration (4 sets, min-max band)
  3. convergence_best_hs_vs_aco.png - head-to-head of each algorithm's Set 4 (60 iters)
  4. fitness_comparison.png         - avg fitness per set, HS vs ACO (best-worst error bars)
  5. accuracy_comparison.png        - relative accuracy vs best-found solution (%), per set
  6. time_comparison.png            - avg computation time per set, HS vs ACO (log scale)
  7. service_metrics.png            - per-UE-type latency & energy for the Set 4 configs
  8. fitness_distribution.png       - spread of final fitness across the 10 runs (box plots)

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
HS_COLOR = "#4C72B0"
ACO_COLOR = "#DD8452"
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


def read_iteration_matrix(path):
    """Return (iterations, matrix) where matrix is [n_iter x n_retest].

    FLT_MAX sentinels and blanks become NaN so they are skipped when plotting.
    """
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


def iteration_files(prefix):
    """Sorted list of *_fitness_iterations_paramset_N.csv for a given prefix."""
    files = RESULTS.glob(f"{prefix}_fitness_iterations_paramset_*.csv")
    return sorted(files, key=lambda p: int(p.stem.split("_")[-1]))


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
# Load aggregated data
# --------------------------------------------------------------------------- #
HS_AGG = read_dicts(RESULTS / "harmony_aggregated_results.csv")
ACO_AGG = read_dicts(RESULTS / "ant_colony_aggregated_results.csv")
N_SETS = len(HS_AGG)
SET_LABELS = [f"Set {i + 1}" for i in range(N_SETS)]

# Best (lowest-cost) solution found anywhere — used as the accuracy reference.
BEST_KNOWN = min(
    min(d["BestFitness"] for d in HS_AGG),
    min(d["BestFitness"] for d in ACO_AGG),
)


def conv_label(prefix, i):
    if prefix == "harmony":
        d = HS_AGG[i]
        lab = (f"Set {i + 1}  (it={int(d['Iterations'])}, HMS={int(d['HMS'])}, "
               f"HMCR={d['HMCR']:g}, PAR={d['PAR']:g}")
        lab += f", LSR={d['LSR']:g})" if d.get("LSR", 0) else ")"
        return lab
    d = ACO_AGG[i]
    return (f"Set {i + 1}  (it={int(d['Iterations'])}, ants={int(d['NumAnts'])}, "
            f"rho={d['Rho']:g}, Q={int(d['Q'])})")


def fitness_errors(rows):
    """Asymmetric error bars: [avg-best, worst-avg]."""
    lo = [d["AvgFitness"] - d["BestFitness"] for d in rows]
    hi = [d["WorstFitness"] - d["AvgFitness"] for d in rows]
    return np.array([lo, hi])


def time_errors(rows):
    lo = [d["AvgTime(ms)"] - d["BestTime(ms)"] for d in rows]
    hi = [d["WorstTime(ms)"] - d["AvgTime(ms)"] for d in rows]
    return np.array([lo, hi])


# --------------------------------------------------------------------------- #
# 1-2. Convergence per algorithm
# --------------------------------------------------------------------------- #
def plot_convergence(prefix, title, fname):
    fig, ax = plt.subplots(figsize=(9, 5.5))
    cmap = plt.cm.tab10
    for i, fp in enumerate(iteration_files(prefix)):
        x, M = read_iteration_matrix(fp)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            mean = np.nanmean(M, axis=1)
            lo = np.nanmin(M, axis=1)
            hi = np.nanmax(M, axis=1)
        m = ~np.isnan(mean)
        ax.plot(x[m], mean[m], color=cmap(i), lw=2, label=conv_label(prefix, i))
        ax.fill_between(x[m], lo[m], hi[m], color=cmap(i), alpha=0.12)
    ax.set_xlabel("Iteration")
    ax.set_ylabel("Fitness (total cost — lower is better)")
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8, loc="upper right")
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 3. Head-to-head convergence (Set 4 of each algorithm)
# --------------------------------------------------------------------------- #
def plot_convergence_head_to_head(fname):
    idx = N_SETS - 1  # Set 4
    fig, ax = plt.subplots(figsize=(9, 5.5))
    for prefix, color, name in [("harmony", HS_COLOR, "Harmony Search (Set 4, LSR=0.3)"),
                                ("ant_colony", ACO_COLOR, "Ant Colony (Set 4)")]:
        fp = iteration_files(prefix)[idx]
        x, M = read_iteration_matrix(fp)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            mean = np.nanmean(M, axis=1)
            lo = np.nanmin(M, axis=1)
            hi = np.nanmax(M, axis=1)
        m = ~np.isnan(mean)
        ax.plot(x[m], mean[m], color=color, lw=2.2, label=name)
        ax.fill_between(x[m], lo[m], hi[m], color=color, alpha=0.12)
    ax.set_xlabel("Iteration")
    ax.set_ylabel("Fitness (total cost — lower is better)")
    ax.set_title("Convergence — best configuration of each algorithm (60 iterations)")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=9)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 4. Fitness comparison
# --------------------------------------------------------------------------- #
def plot_fitness_comparison(fname):
    x = np.arange(N_SETS)
    w = 0.38
    hs = [d["AvgFitness"] for d in HS_AGG]
    aco = [d["AvgFitness"] for d in ACO_AGG]
    fig, ax = plt.subplots(figsize=(9, 5.5))
    b1 = ax.bar(x - w / 2, hs, w, yerr=fitness_errors(HS_AGG), capsize=4,
                color=HS_COLOR, label="Harmony Search")
    b2 = ax.bar(x + w / 2, aco, w, yerr=fitness_errors(ACO_AGG), capsize=4,
                color=ACO_COLOR, label="Ant Colony")
    annotate_bars(ax, b1)
    annotate_bars(ax, b2)
    ymin = min(d["BestFitness"] for d in HS_AGG + ACO_AGG)
    ymax = max(d["WorstFitness"] for d in HS_AGG + ACO_AGG)
    ax.set_ylim(ymin * 0.99, ymax * 1.01)  # zoom to make small differences visible
    ax.set_xticks(x)
    ax.set_xticklabels(SET_LABELS)
    ax.set_ylabel("Fitness (total cost — lower is better)")
    ax.set_title("Average fitness by parameter set\n(error bars span best–worst of 10 runs; y-axis zoomed)")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.3)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 5. Accuracy comparison (relative to best solution found)
# --------------------------------------------------------------------------- #
def plot_accuracy_comparison(fname):
    x = np.arange(N_SETS)
    w = 0.38
    hs = [BEST_KNOWN / d["AvgFitness"] * 100 for d in HS_AGG]
    aco = [BEST_KNOWN / d["AvgFitness"] * 100 for d in ACO_AGG]
    fig, ax = plt.subplots(figsize=(9, 5.5))
    b1 = ax.bar(x - w / 2, hs, w, color=HS_COLOR, label="Harmony Search")
    b2 = ax.bar(x + w / 2, aco, w, color=ACO_COLOR, label="Ant Colony")
    annotate_bars(ax, b1, fmt="{:.2f}")
    annotate_bars(ax, b2, fmt="{:.2f}")
    lo = min(hs + aco)
    ax.set_ylim(lo - 2, 100.6)
    ax.axhline(100, color="gray", ls="--", lw=1)
    ax.set_xticks(x)
    ax.set_xticklabels(SET_LABELS)
    ax.set_ylabel("Relative accuracy (%)  —  higher is better")
    ax.set_title(f"Solution accuracy relative to the best solution found "
                 f"(= {BEST_KNOWN:.0f})\naccuracy = best_found / avg_fitness x 100")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.3)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 6. Computation time comparison
# --------------------------------------------------------------------------- #
def plot_time_comparison(fname):
    x = np.arange(N_SETS)
    w = 0.38
    hs = [d["AvgTime(ms)"] for d in HS_AGG]
    aco = [d["AvgTime(ms)"] for d in ACO_AGG]
    fig, ax = plt.subplots(figsize=(9, 5.5))
    b1 = ax.bar(x - w / 2, hs, w, yerr=time_errors(HS_AGG), capsize=4,
                color=HS_COLOR, label="Harmony Search")
    b2 = ax.bar(x + w / 2, aco, w, yerr=time_errors(ACO_AGG), capsize=4,
                color=ACO_COLOR, label="Ant Colony")
    annotate_bars(ax, b1, fmt="{:.0f}")
    annotate_bars(ax, b2, fmt="{:.0f}")
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(SET_LABELS)
    ax.set_ylabel("Avg computation time (ms, log scale)")
    ax.set_title("Average computation time by parameter set\n"
                 "(HS Set 4 includes the Local Search operator)")
    ax.legend()
    ax.grid(True, axis="y", which="both", alpha=0.3)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 7. Service-specific metrics (Set 4 configs)
# --------------------------------------------------------------------------- #
def plot_service_metrics(fname):
    idx = N_SETS - 1
    hs, aco = HS_AGG[idx], ACO_AGG[idx]
    lat_hs = [hs[f"AvgLatency{t}"] for t in UE_COLS]
    lat_aco = [aco[f"AvgLatency{t}"] for t in UE_COLS]
    en_hs = [hs[f"AvgEnergy{t}"] for t in UE_COLS]
    en_aco = [aco[f"AvgEnergy{t}"] for t in UE_COLS]

    x = np.arange(len(UE_COLS))
    w = 0.38
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    b1 = ax1.bar(x - w / 2, lat_hs, w, color=HS_COLOR, label="Harmony Search")
    b2 = ax1.bar(x + w / 2, lat_aco, w, color=ACO_COLOR, label="Ant Colony")
    annotate_bars(ax1, b1, fmt="{:.1f}")
    annotate_bars(ax1, b2, fmt="{:.1f}")
    ax1.set_yscale("log")
    ax1.set_xticks(x); ax1.set_xticklabels(UE_LABELS)
    ax1.set_ylabel("Avg latency (time slots, log scale)")
    ax1.set_title("Average latency per UE type")
    ax1.legend(); ax1.grid(True, axis="y", which="both", alpha=0.3)

    b3 = ax2.bar(x - w / 2, en_hs, w, color=HS_COLOR, label="Harmony Search")
    b4 = ax2.bar(x + w / 2, en_aco, w, color=ACO_COLOR, label="Ant Colony")
    annotate_bars(ax2, b3, fmt="{:.1f}")
    annotate_bars(ax2, b4, fmt="{:.1f}")
    ax2.set_xticks(x); ax2.set_xticklabels(UE_LABELS)
    ax2.set_ylabel("Avg energy")
    ax2.set_title("Average energy per UE type")
    ax2.legend(); ax2.grid(True, axis="y", alpha=0.3)

    fig.suptitle("Service-specific metrics — Set 4 configurations "
                 "(Harmony Search with Local Search vs Ant Colony)", fontsize=12)
    save(fig, fname)


# --------------------------------------------------------------------------- #
# 8. Final-fitness distribution across the 10 runs
# --------------------------------------------------------------------------- #
def final_values(prefix, i):
    _, M = read_iteration_matrix(iteration_files(prefix)[i])
    last = M[-1]
    return last[~np.isnan(last)]


def plot_fitness_distribution(fname):
    fig, axes = plt.subplots(1, 2, figsize=(12, 5), sharey=True)
    for ax, prefix, color, name in [(axes[0], "harmony", HS_COLOR, "Harmony Search"),
                                    (axes[1], "ant_colony", ACO_COLOR, "Ant Colony")]:
        data = [final_values(prefix, i) for i in range(N_SETS)]
        bp = ax.boxplot(data, tick_labels=SET_LABELS, patch_artist=True,
                        medianprops=dict(color="black"))
        for box in bp["boxes"]:
            box.set(facecolor=color, alpha=0.6)
        ax.set_title(name)
        ax.grid(True, axis="y", alpha=0.3)
    axes[0].set_ylabel("Final fitness across 10 runs (lower is better)")
    fig.suptitle("Distribution of final fitness over 10 independent runs", fontsize=12)
    save(fig, fname)


# --------------------------------------------------------------------------- #
def main():
    print(f"Reading CSVs from : {RESULTS}")
    print(f"Writing plots to  : {PLOTS}")
    print(f"Parameter sets    : {N_SETS}   |   best solution found: {BEST_KNOWN:.1f}\n")

    plot_convergence("harmony", "Harmony Search — convergence (mean of 10 runs, band = min–max)",
                     "convergence_harmony.png")
    plot_convergence("ant_colony", "Ant Colony — convergence (mean of 10 runs, band = min–max)",
                     "convergence_ant_colony.png")
    plot_convergence_head_to_head("convergence_best_hs_vs_aco.png")
    plot_fitness_comparison("fitness_comparison.png")
    plot_accuracy_comparison("accuracy_comparison.png")
    plot_time_comparison("time_comparison.png")
    plot_service_metrics("service_metrics.png")
    plot_fitness_distribution("fitness_distribution.png")

    print("\nDone.")


if __name__ == "__main__":
    main()
