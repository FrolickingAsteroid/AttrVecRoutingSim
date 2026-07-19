#!/usr/bin/env bash
# run_simulations.sh
# Runs the Simulator for RPBOB, BGP, and RCN

set -o errexit -o pipefail -o noclobber -o nounset

function ProgressBar {
    local _progress=$(( ($1 * 100 / $2 * 100) / 100 ))
    local _done=$(( (_progress * 4) / 10 ))
    local _left=$(( 40 - _done ))
    local _fill=$(printf "%${_done}s")
    local _empty=$(printf "%${_left}s")
    printf "\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%%"
}

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
dataset="bclique"
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
if [[ "$runs" -gt 1 && "$regen" -eq 0 ]]; then
    echo "$0: --runs > 1 without --regen will repeat the same dataset every time. Did you mean to add --regen?" >&2
fi

echo "type: $type, metric: $metric, network: $dataset, filetype: $filetype, append: $append, regen: $regen, runs: $runs"

SIMULATOR="./build/Simulator"
NODE_COUNTS=(4 16 32 64 128)
if [[ ! -x "${SIMULATOR}" ]]; then
    echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
    exit 1
fi
extra_args=()
if [[ "$append" -eq 1 ]]; then
    extra_args+=(-A)
fi

total_steps=$(( ${#NODE_COUNTS[@]} * runs * 3 ))
step=0

for n in "${NODE_COUNTS[@]}"; do
    for ((run = 1; run <= runs; run++)); do
        if [[ "$runs" -gt 1 ]]; then
            printf "\n=== n=%s : run %s/%s ===\n" "${n}" "${run}" "${runs}"
        fi
        if [[ "$regen" -eq 1 ]]; then
            printf "\nregenerating dataset (./networkdataset.py bclique -n %s --%s)\n" "${n}" "${type}"
            cd "${SCRIPT_DIR}"
            ./networkdataset.py bclique -n "${n}" --mode "${type}"
            cd "${REPO_ROOT}"
        fi
        DATASET="data/datasets/clique/bclique_n${n}/${filetype}_${type}.tsv"
        if [[ ! -f "${DATASET}" ]]; then
            printf "\nWarning: dataset not found, skipping n=%s: %s\n" "${n}" "${DATASET}" >&2
            step=$(( step + 3 ))
            ProgressBar "$step" "$total_steps"
            continue
        fi

        "${SIMULATOR}" -f "${DATASET}" -R -m 300 -F 0 "${n}" -i 1.0 -t 0 -a "${metric}" "${extra_args[@]}" > /dev/null
        step=$(( step + 1 ))
        ProgressBar "$step" "$total_steps"

        "${SIMULATOR}" -f "${DATASET}" -G -F 0 "${n}" -t 0 -a "${metric}" "${extra_args[@]}" > /dev/null
        step=$(( step + 1 ))
        ProgressBar "$step" "$total_steps"

        "${SIMULATOR}" -f "${DATASET}" -C -F 0 "${n}" -t 0 -a "${metric}" "${extra_args[@]}" > /dev/null
        step=$(( step + 1 ))
        ProgressBar "$step" "$total_steps"
    done
done

echo ""
echo "All simulations completed."