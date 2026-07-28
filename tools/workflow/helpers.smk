SIMULATOR = config["simulator"]
PROTOCOLS = config["protocols"]

RUNS = config["runs"]
METRIC = config["metric"]
PERIOD = config["period"]
KINDS = ["traffic", "convergence", "mslog"]


# ────────────────────────────────────────────────────────────────
# Dataset path helpers
# ────────────────────────────────────────────────────────────────

def rocketfuel_dataset_path(as_num, filetype):
    return f"data/datasets/rocketfuel/{as_num}/{filetype}.tsv"


def output_basename(proto, filetype, scenario="normal"):
    p = PROTOCOLS[proto]
    parts = [filetype, "A", METRIC]
    if p["period"]:
        parts += ["N", "adaptive"]
    parts += ["S", scenario]
    if p["notif"]:
        parts.append("notif")
    if p["has_period"]:
        parts += ["period", str(PERIOD)]
    return "_".join(parts)


def output_files(out_dir, proto, filetype, scenario="normal"):
    base = output_basename(proto, filetype, scenario)
    return [f"{out_dir}/{base}_{kind}.csv" for kind in KINDS]
