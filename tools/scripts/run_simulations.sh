#!/usr/bin/env bash
#
# run_simulations.sh
#
# Runs the Simulator for RPBOB, BGP, and RCN

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

SIMULATOR="./build/Simulator"
NODE_COUNTS=(4 8 16 32 64)

if [[ ! -x "${SIMULATOR}" ]]; then
    echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
    exit 1
fi

for n in "${NODE_COUNTS[@]}"; do
    DATASET="data/datasets/clique/bclique_n${n}/DelayHops_unit.tsv"

    if [[ ! -f "${DATASET}" ]]; then
        echo "Warning: dataset not found, skipping n=${n}: ${DATASET}" >&2
        continue
    fi

    echo "n=${n} : RPBOB"
    "${SIMULATOR}" -f "${DATASET}" -R -m 200 -F 0 ${n} -i 1.0 -t 0 -a hops

    echo "n=${n} : BGP"
    "${SIMULATOR}" -f "${DATASET}" -G -F 0 ${n} -t 0 -a hops

    echo "n=${n} : RCN"
    "${SIMULATOR}" -f "${DATASET}" -C -F 0 ${n} -t 0 -a hops
done

echo "All simulations completed."
