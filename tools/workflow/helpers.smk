SIMULATOR = config["simulator"]
PROTOCOLS = config["protocols"]

RUNS = config["runs"]
METRIC = config["metric"]  # "hops" or "lengths" — used in output filenames
PERIOD = float(config["period"])
KINDS = ["traffic", "convergence", "msglog"]

# The simulator's -a flag only accepts "hops" or "length" (singular), while
# output filenames always spell it "hops"/"lengths" (AlgebraName in
# src/core/argparsing.hpp). Translate so config.yaml can keep using the
# filename spelling.
METRIC_CLI = "length" if METRIC == "lengths" else METRIC


# ────────────────────────────────────────────────────────────────
# Dataset path helpers
# ────────────────────────────────────────────────────────────────

def rocketfuel_dataset_path(as_num, filetype):
    return f"data/datasets/rocketfuel/{as_num}/{filetype}.tsv"


# ────────────────────────────────────────────────────────────────
# Simulator CLI / output-path helpers
#
# These mirror RPBOBParams/BGPParams (src/core/argparsing.hpp) and
# makeOutputPath (src/utils/stats/csv-output.hpp) so the filenames we
# predict here match what the simulator actually writes on disk.
# ────────────────────────────────────────────────────────────────

def sim_flags(proto):
    return " ".join(PROTOCOLS[proto]["flags"])


def output_basename(proto, filetype, scenario="normal"):
    p = PROTOCOLS[proto]
    parts = [filetype, f"A_{METRIC}"]
    if p["period"]:
        parts.append("N_adaptative")
    parts.append(f"S_{scenario}")
    if p["notif"]:
        parts.append("notif")
    if p["period"]:
        parts.append(f"period_{PERIOD:.1f}")
    return "_".join(parts)


def output_dir(proto, as_num):
    return f"data/output/rocketfuel/{as_num}/{PROTOCOLS[proto]['output_proto']}"


def output_file(proto, as_num, filetype, kind, scenario="normal"):
    base = output_basename(proto, filetype, scenario)
    return f"{output_dir(proto, as_num)}/{base}_{kind}.csv"


def output_files(proto, as_num, filetype, scenario="normal"):
    return [output_file(proto, as_num, filetype, kind, scenario) for kind in KINDS]
