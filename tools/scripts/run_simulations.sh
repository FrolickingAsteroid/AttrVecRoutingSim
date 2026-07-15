#!/usr/bin/env bash

# run_simulations.sh
#
# Runs the Simulator for RPBOB, BGP, and RCN

set -o errexit -o pipefail -o noclobber -o nounset

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

getopt --test > /dev/null && true
if [[ $? -ne 4 ]]; then
    echo 'I’m sorry, `getopt --test` failed in this environment.'
    exit 1
fi

LONGOPTS=unit,equal,uniform,metric:,filetype:,append,regen,runs:
OPTIONS=uerm:f:aN:

PARSED=$(getopt --options="$OPTIONS" --longoptions="$LONGOPTS" --name "$0" -- "$@") || exit 2
eval set -- "$PARSED"

type=""
metric="hops"
filetype="DelayHops"
append=0
regen=0
runs=1

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
        -m|--metric)
            metric="$2"
            shift 2
            ;;
        -f|--filetype)
            filetype="$2"
            shift 2
            ;;
        -a|--append)
            append=1
            shift
            ;;
        --regen)
            regen=1
            shift
            ;;
        -N|--runs)
            runs="$2"
            shift 2
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

if [[ "$filetype" != "DelayHops" && "$filetype" != "DelayLength" ]]; then
    echo "$0: --filetype must be DelayHops or DelayLength (got: $filetype)."
    exit 4
fi

if ! [[ "$runs" =~ ^[0-9]+$ ]] || [[ "$runs" -lt 1 ]]; then
    echo "$0: --runs must be a positive integer (got: $runs)."
    exit 4
fi

# repeated runs with no regen would just re-simulate
# the same fixed topology `runs` times, which is pointless for "unit"/"equal"
# (deterministic datasets). As such, use this with "uniform" with --regen set.
if [[ "$runs" -gt 1 && "$regen" -eq 0 ]]; then
    echo "$0: --runs > 1 without --regen will repeat the same dataset every time. Did you mean to add --regen?" >&2
fi

echo "type: $type, metric: $metric, filetype: $filetype, append: $append, regen: $regen, runs: $runs"

SIMULATOR="./build/Simulator"
NODE_COUNTS=(4 8 16 32 64)

if [[ ! -x "${SIMULATOR}" ]]; then
    echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
    exit 1
fi

extra_args=()
if [[ "$append" -eq 1 ]]; then
    extra_args+=(-A)
fi

for n in "${NODE_COUNTS[@]}"; do
    for ((run = 1; run <= runs; run++)); do
        if [[ "$runs" -gt 1 ]]; then
            echo "=== n=${n} : run ${run}/${runs} ==="
        fi

        if [[ "$regen" -eq 1 ]]; then
            echo "regenerating dataset (./networkdataset.py bclique -n ${n} --${type})"
            cd "${SCRIPT_DIR}"
            ./networkdataset.py bclique -n "${n}" --mode "${type}"
            cd "${REPO_ROOT}"
            echo ""
        fi

        DATASET="data/datasets/clique/bclique_n${n}/${filetype}_${type}.tsv"

        if [[ ! -f "${DATASET}" ]]; then
            echo "Warning: dataset not found, skipping n=${n}: ${DATASET}" >&2
            continue
        fi

        echo "n=${n} : RPBOB"
        "${SIMULATOR}" -f "${DATASET}" -R -m 200 -F 0 "${n}" -i 1.0 -t 0 -a "${metric}" "${extra_args[@]}"

        echo "n=${n} : BGP"
        "${SIMULATOR}" -f "${DATASET}" -G -F 0 "${n}" -t 0 -a "${metric}" "${extra_args[@]}"

        echo "n=${n} : RCN"
        "${SIMULATOR}" -f "${DATASET}" -C -F 0 "${n}" -t 0 -a "${metric}" "${extra_args[@]}"
    done
done

echo "All simulations completed."