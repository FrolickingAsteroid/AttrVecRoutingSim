#!/usr/bin/env bash
#
# run_simulations.sh
#
# Runs the Simulator for RPBOB, BGP, and RCN

set -o errexit -o pipefail -o noclobber -o nounset

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

# ignore errexit with `&& true`
getopt --test > /dev/null && true
if [[ $? -ne 4 ]]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

LONGOPTS=unit,equal,uniform
OPTIONS=uer

PARSED=$(getopt --options="$OPTIONS" --longoptions="$LONGOPTS" --name "$0" -- "$@") || exit 2
eval set -- "$PARSED"

type=""

while true; do
    case "$1" in
        -u|--unit)
            type=unit
            shift
            ;;
        -e|--equal)
            type=equal
            shift
            ;;
        -r|--uniform)
            type=uniform
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Programming error"
            exit 3
            ;;
    esac
done


if [[ -z "$type" ]]; then
    echo "$0: A type of dataset is required (-u/--unit, -e/--equal, -r/--uniform)."
    exit 4
fi

echo "type: $type"

SIMULATOR="./build/Simulator"
NODE_COUNTS=(4 8 32 64)

if [[ ! -x "${SIMULATOR}" ]]; then
    echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
    exit 1
fi

for n in "${NODE_COUNTS[@]}"; do
    DATASET="data/datasets/clique/bclique_n${n}/DelayHops_${type}.tsv"

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
