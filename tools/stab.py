#!/usr/bin/env python3

from pathlib import Path
import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ─────────────────────────────────────────────────────────────────────────────
# Global settings
# ─────────────────────────────────────────────────────────────────────────────

DATA_ROOT = Path("../data/output")
PLOT_ROOT = Path("../data/plots")

plt.rcParams.update(
    {
        "font.family": "serif",
        "font.serif": ["Computer Modern Roman", "DejaVu Serif"],
        "mathtext.fontset": "cm",
        "font.size": 10,
    }
)

PROTO_COLORS = {
    "bgp": "tab:green",
    "rpbob": "tab:blue",
    "beaconing": "tab:orange",
}


# ─────────────────────────────────────────────────────────────────────────────
# Interactive helpers
# ─────────────────────────────────────────────────────────────────────────────

def choose_one(prompt, options, display_fn=str):
    if not options:
        raise RuntimeError(f"No options available for: {prompt}")

    print(f"\n{prompt}")
    for i, opt in enumerate(options, 1):
        print(f"  [{i}] {display_fn(opt)}")

    while True:
        s = input("> ").strip()
        if s.isdigit() and 1 <= int(s) <= len(options):
            return options[int(s) - 1]
        print("Invalid choice.")


def choose_many(prompt, options, display_fn=str):
    if not options:
        raise RuntimeError(f"No options available for: {prompt}")

    print(f"\n{prompt}")
    for i, opt in enumerate(options, 1):
        print(f"  [{i}] {display_fn(opt)}")

    print("Write numbers separated by spaces, or 'all'.")

    while True:
        s = input("> ").strip()

        if s.lower() == "all":
            return options

        try:
            idxs = [int(x) for x in s.split()]
            if all(1 <= i <= len(options) for i in idxs):
                return [options[i - 1] for i in idxs]
        except ValueError:
            pass

        print("Invalid choice.")


def ask_optional_int(prompt):
    s = input(f"\n{prompt}").strip()
    return int(s) if s else None


# ─────────────────────────────────────────────────────────────────────────────
# File discovery
# ─────────────────────────────────────────────────────────────────────────────

def discover_folders():
    return sorted(p.name for p in DATA_ROOT.iterdir() if p.is_dir())


def discover_networks(folder):
    base = DATA_ROOT / folder
    return sorted(p.name for p in base.iterdir() if p.is_dir())


def discover_csvs(folder, network, suffix="_convergence.csv"):
    base = DATA_ROOT / folder / network

    files = []
    for proto_dir in sorted(p for p in base.iterdir() if p.is_dir()):
        proto = proto_dir.name

        for path in sorted(proto_dir.glob(f"*{suffix}")):
            files.append(
                {
                    "folder": folder,
                    "network": network,
                    "proto": proto,
                    "path": path,
                    "name": path.name,
                }
            )

    return files


def network_size(network):
    m = re.search(r"n(\d+)$", network)
    if not m:
        raise ValueError(f"Could not parse network size from: {network}")
    return int(m.group(1))


def parse_tags(filename):
    """
    Parses filenames such as:

        triangle_n16_A_lengths_S_linkFailure_convergence.csv
        triangle_n16_A_lengths_N_adaptative_S_linkFailure_period_1.0_convergence.csv

    into:
        {"A": "lengths", "S": "linkFailure", "N": "adaptative", "period": "1.0"}
    """
    stem = filename.removesuffix("_convergence.csv")
    parts = stem.split("_")

    tags = {}
    keys = {"A", "S", "N", "period", "dprop"}

    i = 0
    while i < len(parts) - 1:
        if parts[i] in keys:
            tags[parts[i]] = parts[i + 1]
            i += 2
        else:
            i += 1

    return tags


def display_name(item):
    tags = parse_tags(item["name"])

    bits = [item["proto"].upper()]

    if "A" in tags:
        bits.append(tags["A"])
    if "S" in tags:
        bits.append(tags["S"])
    if "N" in tags:
        bits.append(f"N={tags['N']}")
    if "period" in tags:
        bits.append(f"period={tags['period']}")
    if "dprop" in tags:
        bits.append(f"dprop={tags['dprop']}")

    return " | ".join(bits)


def display_legend(item):
    tags = parse_tags(item["name"])

    bits = [item["proto"].upper()]

    if "A" in tags:
        if tags["A"] == "lengths":
            bits.append("SP")
        if tags["A"] == "hops":
            bits.append("H")
    if "period" in tags:
        bits.append(r"$\Delta$=" + f"{tags['period']}")
    if "dprop" in tags:
        bits.append(f"dprop={tags['dprop']}")

    return " ".join(bits)


def path_from_template(folder, network, template):
    old_network = template["network"]
    filename = template["name"].replace(old_network, network, 1)

    return DATA_ROOT / folder / network / template["proto"] / filename


# ─────────────────────────────────────────────────────────────────────────────
# Series loading
# ─────────────────────────────────────────────────────────────────────────────

def load_series(files, dest_filter=None):
    series = []

    for item in files:
        df = pd.read_csv(item["path"])

        if dest_filter is not None:
            df = df[df["dest"] == dest_filter]

        tags = parse_tags(item["name"])

        series.append(
            {
                "df": df,
                "label": display_legend(item),
                "proto": item["proto"],
                "algebra": tags.get("A", "unknown"),
                "scene": tags.get("S", "unknown"),
                "path": item["path"],
                "name": item["name"],
                "network": item["network"],
            }
        )

    return series


def make_output_name(network, series, suffix):
    protos = "_".join(dict.fromkeys(item["proto"] for item in series))
    scenes = "_".join(dict.fromkeys(item["scene"] for item in series))
    algebras = "_".join(dict.fromkeys(item["algebra"] for item in series))

    return PLOT_ROOT / f"{network}_{protos}_{scenes}_{algebras}_{suffix}.png"


# ─────────────────────────────────────────────────────────────────────────────
# Plot 1: CCDF stabilisation time
# ─────────────────────────────────────────────────────────────────────────────

def plot_stab_ccdf(series, network, output_file):
    fig, ax = plt.subplots(figsize=(5.5, 3.8))
    max_x = 0.0

    for item in series:
        df = item["df"]
        proto = item["proto"]
        label = item["label"]

        df = df[df["change_count"] != 0]

        if df.empty:
            continue

        x = np.sort(df["stab_time"].to_numpy())
        n = len(x)

        avg_time = np.mean(x)
        ccdf = 1.0 - np.arange(0, n) / n

        x_plot = np.r_[0, x]
        ccdf_plot = np.r_[1.0, ccdf]

        ax.step(
            x_plot,
            ccdf_plot,
            where="post",
            color=PROTO_COLORS.get(proto, None),
            label=f"{label} (avg {avg_time:.2f} ms)",
            linewidth=1.5,
        )

        max_x = max(max_x, float(np.max(x_plot)))

    ax.set_title(f"Stabilisation time per node ({network})", fontsize=10)
    ax.set_xlabel("Time [ms]")
    ax.set_ylabel("CCDF")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)

    ax.set_xlim([-0.01, max_x])
    ax.set_ylim([-0.01, 1.1])

    fig.tight_layout()
    output_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_file, dpi=250, bbox_inches="tight")
    print(f"Saved: {output_file}")


# ─────────────────────────────────────────────────────────────────────────────
# Plot 2: change-count bar plot
# ─────────────────────────────────────────────────────────────────────────────

def plot_change_count(series, network, output_file):
    fig, ax = plt.subplots(figsize=(5.5, 3.8))

    tick_positions = np.array([], dtype=int)
    n_series = max(1, len(series))

    for i, item in enumerate(series):
        df = item["df"]
        proto = item["proto"]
        label = item["label"]

        counts = df[df["change_count"] > 0]["change_count"]

        if counts.empty:
            continue

        values, freqs = np.unique(counts, return_counts=True)
        freqs = freqs / freqs.sum()

        avg_change = np.average(values, weights=freqs)
        tick_positions = np.union1d(tick_positions, values)

        width = 0.8 / n_series
        offset = (i - (n_series - 1) / 2) * width

        ax.bar(
            values + offset,
            freqs,
            color=PROTO_COLORS.get(proto, None),
            width=width,
            label=f"{label} (avg {avg_change:.2f})",
        )

    ax.set_title(f"Attribute changes per node after failure ({network})", fontsize=10)
    ax.set_xlabel("Number of attribute changes")
    ax.set_ylabel("Frequency per node")
    ax.grid(True, axis="y", alpha=0.3)

    if len(tick_positions) > 0:
        ax.set_xticks(tick_positions)
        ax.set_xticklabels(tick_positions)
        ax.set_xlim([-0.01, max(tick_positions) + 1])

    ax.set_ylim([-0.01, 1.00])
    ax.legend(fontsize=8)

    fig.tight_layout()
    output_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_file, dpi=250, bbox_inches="tight")
    print(f"Saved: {output_file}")


# ─────────────────────────────────────────────────────────────────────────────
# Plot 3: stabilisation time vs number of nodes, log scale
# ─────────────────────────────────────────────────────────────────────────────

from matplotlib.patches import Patch
import numpy as np
import pandas as pd


def plot_max_stab_log(folder, networks, template_files, dest_filter, output_file):
    fig, ax_box = plt.subplots(figsize=(5.5, 3.8))

    all_boxes = []

    max_stab = 0
    for template in template_files:
        proto = template["proto"]
        color = PROTO_COLORS.get(proto, None)
        label = display_legend(template)

        for network in networks:
            path = path_from_template(folder, network, template)

            if not path.exists():
                print(f"[warn] missing: {path}")
                continue

            df = pd.read_csv(path)

            if dest_filter is not None:
                df = df[df["dest"] == dest_filter]

            df = df[df["change_count"] != 0]

            if df.empty:
                continue

            n_nodes = network_size(network)
            stab = df["stab_time"].to_numpy()

            if df["stab_time"].max() > max_stab:
                max_stab = df["stab_time"].max()

            all_boxes.append({
                "n_nodes": n_nodes,
                "stab": stab,
                "proto": proto,
                "color": color,
                "label": label,
            })

    if not all_boxes:
        print("No data to plot.")
        return

    run_labels = list(dict.fromkeys(b["label"] for b in all_boxes))
    n_runs = max(1, len(run_labels))

    box_data = []
    box_positions = []
    box_widths = []
    box_colors = []

    # For legend averages
    values_by_label = {label: [] for label in run_labels}
    color_by_label = {}

    for b in all_boxes:
        run_idx = run_labels.index(b["label"])
        n_nodes = b["n_nodes"]

        # Multiplicative offset works better with log x-axis.
        offset_factor = 1.0 + (run_idx - (n_runs - 1) / 2) * 0.12

        box_data.append(b["stab"])
        box_positions.append(n_nodes * offset_factor)
        box_widths.append(0.08 * n_nodes)
        box_colors.append(b["color"])

        values_by_label[b["label"]].extend(b["stab"])
        color_by_label[b["label"]] = b["color"]

    bp = ax_box.boxplot(
        box_data,
        positions=box_positions,
        widths=box_widths,
        patch_artist=True,
        showfliers=False,
        manage_ticks=False,
        showmeans=True
    )

    for patch, color in zip(bp["boxes"], box_colors):
        patch.set_alpha(0.35)
        if color is not None:
            patch.set_facecolor(color)

    for whisker in bp["whiskers"]:
        whisker.set_linewidth(0.9)

    for cap in bp["caps"]:
        cap.set_linewidth(0.9)

    for median in bp["medians"]:
        median.set_linewidth(1.3)
        median.set_color("black")

    handles = []
    for label in run_labels:
        avg_time = np.mean(values_by_label[label])
        color = color_by_label[label]

        handles.append(
            Patch(
                facecolor=color,
                edgecolor="black",
                label=f"{label} (avg {avg_time:.2f} ms)",
            )
        )

    ax_box.legend(handles=handles, fontsize=8)

    ax_box.set_xscale("log", base=2)
    ax_box.set_yscale("log", base=2)

    ticks = sorted(network_size(n) for n in networks)
    ax_box.set_xticks(ticks)
    ax_box.set_xticklabels([str(t) for t in ticks])

    ax_box.set_xlabel("Number of nodes")
    ax_box.set_ylabel("Stabilization time [ms]")
    ax_box.grid(True, which="both", alpha=0.3)

    fig.tight_layout()
    output_file.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output_file, dpi=250, bbox_inches="tight")
    print(f"Saved: {output_file}")


# ─────────────────────────────────────────────────────────────────────────────
# Prompt modes
# ─────────────────────────────────────────────────────────────────────────────

def run_ccdf_prompt():
    folder = choose_one("Choose folder:", discover_folders())
    network = choose_one("Choose network:", discover_networks(folder))

    files = discover_csvs(folder, network)

    selected_files = choose_many(
        "Choose convergence files for CCDF:",
        files,
        display_fn=display_name,
    )

    dest_filter = ask_optional_int("Destination filter? Press Enter for all: ")

    series = load_series(selected_files, dest_filter)

    if not series:
        print("No data to plot.")
        return

    output_file = make_output_name(network, series, "stab_ccdf")
    plot_stab_ccdf(series, network, output_file)


def run_change_count_prompt():
    folder = choose_one("Choose folder:", discover_folders())
    network = choose_one("Choose network:", discover_networks(folder))

    files = discover_csvs(folder, network)

    selected_files = choose_many(
        "Choose convergence files for change-count plot:",
        files,
        display_fn=display_name,
    )

    dest_filter = ask_optional_int("Destination filter? Press Enter for all: ")

    series = load_series(selected_files, dest_filter)

    if not series:
        print("No data to plot.")
        return

    output_file = make_output_name(network, series, "change_count")
    plot_change_count(series, network, output_file)


def run_max_stab_log_prompt():
    folder = choose_one("Choose folder:", discover_folders())
    networks = choose_many("Choose networks:", discover_networks(folder))

    template_network = networks[0]
    files = discover_csvs(folder, template_network)

    selected_files = choose_many(
        f"Choose run patterns from {template_network}:",
        files,
        display_fn=display_name,
    )

    dest_filter = ask_optional_int("Destination filter? Press Enter for all: ")

    proto_tag = "_".join(dict.fromkeys(f["proto"] for f in selected_files))
    output_file = PLOT_ROOT / f"{folder}_{proto_tag}_max_stab_log.png"

    plot_max_stab_log(
        folder=folder,
        networks=networks,
        template_files=selected_files,
        dest_filter=dest_filter,
        output_file=output_file,
    )


# ─────────────────────────────────────────────────────────────────────────────
# Main
# ─────────────────────────────────────────────────────────────────────────────

def main():
    plot_type = choose_one(
        "Choose plot type:",
        [
            "ccdf stabilization time",
            "attribute change count",
            "stabilization time log plot",
        ],
    )

    if plot_type == "ccdf stabilization time":
        run_ccdf_prompt()

    elif plot_type == "attribute change count":
        run_change_count_prompt()

    elif plot_type == "stabilization time log plot":
        run_max_stab_log_prompt()

    else:
        raise RuntimeError(f"Unknown plot type: {plot_type}")


if __name__ == "__main__":
    main()
