#!/usr/bin/env bash
# run_simulations.sh
# Runs the Simulator for RPBOB, BGP, and RCN across bclique or rocketfuel datasets.

set -o errexit -o pipefail -o noclobber -o nounset

# ────────────────────────────────────────────────────────────────
# Setup
# ────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

SIMULATOR="./build/Simulator"
BCLIQUE_NODE_COUNTS=(4 8 16 32 64)
ROCKETFUEL_AS_LIST=(1221 1755)

function ProgressBar {
    local _progress=$(( ($1 * 100 / $2 * 100) / 100 ))
    local _done=$(( (_progress * 4) / 10 ))
    local _left=$(( 40 - _done ))
    local _fill=$(printf "%${_done}s")
    local _empty=$(printf "%${_left}s")
    printf "\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%% net ${3} : run ${4}/${5}"
}

# ────────────────────────────────────────────────────────────────
# Arg parsing
# ────────────────────────────────────────────────────────────────

function check_getopt {
    getopt --test > /dev/null && true
    if [[ $? -ne 4 ]]; then
        echo 'I’m sorry, `getopt --test` failed in this environment.'
        exit 1
    fi
}

function parse_args {
    local LONGOPTS=unit,equal,uniform,metric:,filetype:,append,regen,runs:,dataset:,as:
    local OPTIONS=uerm:f:aN:d:
    PARSED=$(getopt --options="$OPTIONS" --longoptions="$LONGOPTS" --name "$0" -- "$@") || exit 2
    eval set -- "$PARSED"

    type=""
    metric="hops"
    filetype="DelayHops"
    dataset="bclique"
    append=0
    regen=0
    runs=1
    as_list=""

    while true; do
        case "$1" in
            -u|--unit) type=unit; shift ;;
            -e|--equal) type=equal; shift ;;
            -r|--uniform) type=uniform; shift ;;
            -m|--metric) metric="$2"; shift 2 ;;
            -f|--filetype) filetype="$2"; shift 2 ;;
            -a|--append) append=1; shift ;;
            --regen) regen=1; shift ;;
            -N|--runs) runs="$2"; shift 2 ;;
            -d|--dataset) dataset="$2"; shift 2 ;;
            --as) as_list="$2"; shift 2 ;;
            --) shift; break ;;
            *) echo "Programming error"; exit 3 ;;
        esac
    done
}

function validate_args {
    if [[ "$dataset" != "bclique" && "$dataset" != "rocketfuel" ]]; then
        echo "$0: --dataset must be bclique or rocketfuel (got: $dataset)." >&2
        exit 4
    fi

    if [[ "$dataset" == "bclique" && -z "$type" ]]; then
        echo "$0: bclique requires a type (-u/--unit, -e/--equal, -r/--uniform)." >&2
        exit 4
    fi

    if [[ "$filetype" != "DelayHops" && "$filetype" != "DelayLength" ]]; then
        echo "$0: --filetype must be DelayHops or DelayLength (got: $filetype)." >&2
        exit 4
    fi

    if ! [[ "$runs" =~ ^[0-9]+$ ]] || [[ "$runs" -lt 1 ]]; then
        echo "$0: --runs must be a positive integer (got: $runs)." >&2
        exit 4
    fi

    if [[ "$dataset" == "bclique" && "$runs" -gt 1 && "$regen" -eq 0 ]]; then
        echo "$0: --runs > 1 without --regen will repeat the same dataset every time. Did you mean to add --regen?" >&2
    fi

    if [[ ! -x "${SIMULATOR}" ]]; then
        echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
        exit 1
    fi
}

# ────────────────────────────────────────────────────────────────
# Dataset resolution (family-specific)
# ────────────────────────────────────────────────────────────────

function bclique_ids {
    echo "${BCLIQUE_NODE_COUNTS[@]}"
}

function rocketfuel_ids {
    if [[ -n "$as_list" ]]; then
        echo "$as_list"
    else
        echo "${ROCKETFUEL_AS_LIST[@]}"
    fi
}

function rocketfuel_regen {
    local as="$1"
    cd "${SCRIPT_DIR}"
    ./networkdataset.py -a "${as}" build > /dev/null
    cd "${REPO_ROOT}"
}

function bclique_regen {
    local id="$1"
    cd "${SCRIPT_DIR}"
    ./networkdataset.py bclique -n "${id}" --mode "${type}" > /dev/null
    cd "${REPO_ROOT}"
}

function bclique_dataset_path {
    local id="$1"
    echo "data/datasets/clique/bclique_n${id}/${filetype}_${type}.tsv"
}

function rocketfuel_dataset_path {
    local id="$1"
    echo "data/datasets/rocketfuel/${id}/${filetype}.tsv"
}

# ────────────────────────────────────────────────────────────────
# Simulator
# ────────────────────────────────────────────────────────────────

# <dataset_path> <link_failure_target_or_empty>
function run {
    local ds="$1"
    local fail_target="${2:-}"
    local fail_args=()
    if [[ -n "$fail_target" ]]; then
        fail_args=(-F "${fail_target}")
    fi

    "${SIMULATOR}" -f "${ds}" -R "${fail_args[@]}" -i 1.0 -m 0 -a "${metric}" "${extra_args[@]}" > /dev/null
    #"${SIMULATOR}" -f "${ds}" -R notif "${fail_args[@]}" -i 1.0 -a "${metric}" "${extra_args[@]}" > /dev/null
    "${SIMULATOR}" -f "${ds}" -G "${fail_args[@]}"  -a "${metric}" "${extra_args[@]}" > /dev/null
    #"${SIMULATOR}" -f "${ds}" -C "${fail_args[@]}" -a "${metric}" "${extra_args[@]}" > /dev/null
}

# ────────────────────────────────────────────────────────────────
# Main driver
# ────────────────────────────────────────────────────────────────

function main {
    check_getopt
    parse_args "$@"
    validate_args

    extra_args=()
    if [[ "$append" -eq 1 ]]; then
        extra_args+=(-A)
    fi

    local ids
    ids=($(${dataset}_ids))

    local total_steps=$(( ${#ids[@]} * runs * 4 ))
    local step=0

    echo "dataset: $dataset, type: ${type:-n/a}, metric: $metric, filetype: $filetype, append: $append, regen: $regen, runs: $runs"

    for id in "${ids[@]}"; do
        for ((run = 1; run <= runs; run++)); do
            if [[ "$dataset" == "bclique" && "$regen" -eq 1 ]]; then
                bclique_regen "${id}"
            fi

#            if [[ "$dataset" == "rocketfuel" && "$regen" -eq 1 ]]; then
#                rocketfuel_regen "${id}"
#            fi

            local ds_path
            ds_path=$(${dataset}_dataset_path "${id}")

            if [[ ! -f "${ds_path}" ]]; then
                printf "\nWarning: dataset not found, skipping %s: %s\n" "${id}" "${ds_path}" >&2
                step=$(( step + 4 ))
                ProgressBar "$step" "$total_steps" "${id}" "${run}" "${runs}"
                continue
            fi

            local fail_target=""
            if [[ "$dataset" == "bclique" ]]; then
                fail_target="0 ${id}"
            fi

            run "${ds_path}" "${fail_target}"
            step=$(( step + 4 ))
            ProgressBar "$step" "$total_steps" "${id}" "${run}" "${runs}"
        done
    done

    echo ""
    echo "All simulations completed."
}

main "$@"