#!/usr/bin/env bash

set -o errexit -o pipefail -o noclobber -o nounset

# ────────────────────────────────────────────────────────────────
# Setup
# ────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

SIMULATOR="./build/Simulator"
BCLIQUE_NODE_COUNTS=(4 8 16 32)

function ProgressBar {
    local _progress=$(( ($1 * 100 / $2 * 100) / 100 ))
    local _done=$(( (_progress * 4) / 10 ))
    local _left=$(( 40 - _done ))
    local _fill=$(printf "%${_done}s")
    local _empty=$(printf "%${_left}s")
    printf "\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%%"
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

function usage {
    cat >&2 <<EOF
Usage: $0 -T <unit|equal> [-N runs] [-n "4 8 16"]
       $0 -D rocketfuel

  -D, --dataset          clique|rocketfuel  which dataset to run (default: clique)
  -T, --type            unit|equal   (required for --dataset clique) bclique dataset variant to evaluate
  -N, --runs            N            number of runs per node-count (default: 1)
  -n, --node-counts     "..."        space-separated node counts (default: ${BCLIQUE_NODE_COUNTS[*]})
EOF
    exit 1
}

function parse_args {
    local LONGOPTS=type:,runs:,node-counts:,clean:,dataset:
    local OPTIONS=T:N:n:C:D:
    PARSED=$(getopt --options="$OPTIONS" --longoptions="$LONGOPTS" --name "$0" -- "$@") || usage
    eval set -- "$PARSED"

    type=""
    output=""
    runs=1
    node_counts=("${BCLIQUE_NODE_COUNTS[@]}")
    dataset="clique"

    while true; do
        case "$1" in
            -C|--clean) output="$2"; break ;;
            -D|--dataset) dataset="$2"; shift 2 ;;
            -T|--type) type="$2"; shift 2 ;;
            -N|--runs) runs="$2"; shift 2 ;;
            -n|--node-counts) node_counts=($2); shift 2 ;;
            --) shift; break ;;
            *) echo "Programming error"; exit 3 ;;
        esac
    done
}

function validate_args {
    if [[ "$output" == "clique" || "$output" == "rocketfuel" || "$output" == "triangle" ]]; then
      return 0;
    fi

    if [[ ! -x "${SIMULATOR}" ]]; then
        echo "Error: ${SIMULATOR} not found or not executable (run from repo root, or build first)." >&2
        exit 1
    fi

    if [[ "$dataset" != "clique" && "$dataset" != "rocketfuel" && "$dataset" != "triangle" && "$dataset" != "rocket_centrality" ]]; then
        echo "$0: --dataset must be clique, rocketfuel or triangle (got: $dataset)." >&2
        usage
    fi

    if [ "$dataset" == "clique" ] && [[ "$type" != "unit" && "$type" != "equal" ]]; then
        echo "$0: --type must be unit or equal (got: ${type:-<empty>})." >&2
        usage
    fi


    if ! [[ "$runs" =~ ^[0-9]+$ ]] || [[ "$runs" -lt 1 ]]; then
        echo "$0: --runs must be a positive integer (got: $runs)." >&2
        exit 4
    fi
}

# ────────────────────────────────────────────────────────────────
# Datasets
# ────────────────────────────────────────────────────────────────

function bclique_dataset_path {
    local id="$1"
    echo "data/datasets/clique/bclique_n${id}/DelayHops_${type}.tsv"
}

function triangle_dataset_path {
    local id="$1"
    echo "data/datasets/triangle/triangle_n${id}/triangle_n${id}.tsv"
}

function clean {
  local cdata="$1"
  # shellcheck disable=SC2283
  CLEAN="data/output/${cdata}"

  cd "$CLEAN"
  echo "Cleaning simulation output from ${CLEAN}"
  find . -name "*.csv" -type f -delete
}

# ────────────────────────────────────────────────────────────────
# Simulators
# ────────────────────────────────────────────────────────────────


# ────────────────────────────────────────────────────────────────
# Rocketfuel
function run_rocket {
    local combos=(
        "data/datasets/scion/sciera/DelayLength.tsv:lex-length-hops-suffix"
        "data/datasets/scion/sciera/DelayLength.tsv:lex-length-explicit"
        "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-suffix"
        "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-explicit"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:lex-length-width-suffix"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:prod-width-length-hops-suffix"
        #"data/datasets/rocketfuel/1755/DelayHops.tsv:lex-hops-suffix"
    )

    local pids=()
    local combo file algebra
    for combo in "${combos[@]}"; do
        file="${combo%%:*}"
        algebra="${combo##*:}"

        "${SIMULATOR}" -f "${file}" -R -a "${algebra}" -i 1.0 -F > /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -R notif -a "${algebra}" -i 1.0 -F > /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -G -A -a "${algebra}" -F > /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -C -A -a "${algebra}" -F > /dev/null & pids+=($!)
    done

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

# ────────────────────────────────────────────────────────────────
# SCION
function run_scion {
    local combos=(
        "data/datasets/scion/sciera/DelayLength.tsv:lex-length-hops-suffix"
        "data/datasets/scion/sciera/DelayLength.tsv:lex-length-explicit"
        "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-suffix"
        "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-explicit"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:lex-length-width-suffix"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:prod-width-length-hops-suffix"
        #"data/datasets/rocketfuel/1755/DelayHops.tsv:lex-hops-suffix"
    )

    local pids=()
    local combo file algebra
    for combo in "${combos[@]}"; do
        file="${combo%%:*}"
        algebra="${combo##*:}"

        "${SIMULATOR}" -f "${file}" -R -a "${algebra}" -i 1.0 -m 0 -A> /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -R notif -a "${algebra}" -i 1.0 -F > /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -G -A -a "${algebra}" -F > /dev/null & pids+=($!)
        #"${SIMULATOR}" -f "${file}" -C -A -a "${algebra}" -F > /dev/null & pids+=($!)
    done

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

function run_rocket_cutset {
    local combos=(
        "data/datasets/rocketfuel/3257/DelayLength.tsv:lex-length-hops-suffix"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:lex-length-width-suffix"
        # "data/datasets/rocketfuel/1221/DelayLengthWidth.tsv:prod-width-length-hops-suffix"
        #"data/datasets/rocketfuel/1755/DelayHops.tsv:lex-hops-suffix"
    )

    local pids=()
    local combo file algebra
    for combo in "${combos[@]}"; do
        file="${combo%%:*}"
        algebra="${combo##*:}"

        "${SIMULATOR}" -f "${file}" -R -a "${algebra}" -i 1.0 --cutset > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -R notif -a "${algebra}" -i 1.0 --cutset > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -G -A -a "${algebra}" --cutset > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -C -A -a "${algebra}" --cutset > /dev/null & pids+=($!)
    done

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

function run_clique {
    local ds="$1"
    local id="$2"

    local pids=()
    "${SIMULATOR}" -f "${ds}" -A -R -i 1.0 -F 0 "${id}" -t 0 > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -R notif -i 1.0 -F 0 "${id}" -t 0 > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -G -F 0 "${id}" -t 0 > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -C -F 0 "${id}" -t 0 > /dev/null & pids+=($!)

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

function run_triangle {
    local ds="$1"
    local id="$2"

    local pids=()
    "${SIMULATOR}" -f "${ds}" -A -R -i 1.0 -F "$((id - 1))" "${id}" -t "${id}" -m 2 > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -R notif -i 1.0 -F "$((id - 1))" "${id}" -t "${id}" -m 2 > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -G -F "$((id - 1))" "${id}" -t "${id}" > /dev/null & pids+=($!)
    "${SIMULATOR}" -f "${ds}" -A -C -F "$((id - 1))" "${id}" -t "${id}" > /dev/null & pids+=($!)

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

# Top-10 edges by centrality, as (id1 id2) pairs
TOP_EDGES=(
    "2 34"
    "34 45"
    "8 47"
    "45 6"
    "16 47"
    "8 9"
    "5 6"
)

TOP_EDGES_1755=(
    "18 45"
    "24 25"
    "0 18"
    "25 50"
    "24 19"
    "45 13"
    "0 24"
    "50 68"
    "3 19"
    "0 38"
)

function run_rocket_edge {
    local id1="$1"
    local id2="$2"

    local combos=(
        "data/datasets/rocketfuel/3257/DelayLength.tsv:lex-length-hops-suffix"
        #"data/datasets/rocketfuel/1221/DelayHops.tsv:lex-hops-suffix"
    )

    local pids=()
    local combo file algebra
    for combo in "${combos[@]}"; do
        file="${combo%%:*}"
        algebra="${combo##*:}"

        "${SIMULATOR}" -f "${file}" -A -R -a "${algebra}" -i 1.0 -F "${id1}" "${id2}" > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -A -R notif -a "${algebra}" -i 1.0 -F "${id1}" "${id2}" > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -G -A -a "${algebra}" -F "${id1}" "${id2}" > /dev/null & pids+=($!)
        "${SIMULATOR}" -f "${file}" -C -A -a "${algebra}" -F "${id1}" "${id2}" > /dev/null & pids+=($!)
    done

    local pid status=0
    for pid in "${pids[@]}"; do
        if ! wait "${pid}"; then
            status=1
        fi
    done
    return "${status}"
}

function run_rocket_edges {
    local failures=0
    local edge="$1"
    local id1 id2

    read -r id1 id2 <<< "${edge}"
    if ! run_rocket_edge "${id1}" "${id2}"; then
        failures=$(( failures + 1 ))
        echo "  -> failed" >&2
    fi


    return $(( failures > 0 ? 1 : 0 ))
}


# ────────────────────────────────────────────────────────────────
# Main
# ────────────────────────────────────────────────────────────────

function main {
    check_getopt
    parse_args "$@"
    validate_args

    local failures=0
    local step=0

    ## clean output folders
    if [ "$output" != "" ]; then
      clean "$output"
      exit 0;
    fi

    ## Run Rocketfuel Topologies
    if [ "$dataset" == "rocketfuel" ]; then
      local total_steps=$((runs))
      for ((run = 1; run <= runs; run++)); do
        if ! run_scion; then
            echo "Completed with failed job(s)." >&2
            exit 1
        fi
        step=$(( step + 1 ))
        ProgressBar "$step" "$total_steps"
      done

      printf "\nAll simulations completed."
      exit 0
    fi

    ## Run Rocketfuel Topologies failing edges with large edge centrality
    if [ "$dataset" == "rocket_centrality" ]; then
      local total_steps=$(( ${#TOP_EDGES_1755[@]} * runs ))
      for edge in "${TOP_EDGES_1755[@]}"; do
        for ((run = 1; run <= runs; run++)); do
          if ! run_rocket_edges "${edge}"; then
              echo "Completed with failed job(s)." >&2
              exit 1
          fi

          step=$(( step + 1 ))
          ProgressBar "$step" "$total_steps"
        done
      done

      printf "\nAll simulations completed."
      exit 0
    fi

    ## Run Clique/Triangle Topologies
    local dataset_path_fn run_fn
    case "$dataset" in
        clique)   dataset_path_fn=bclique_dataset_path;  run_fn=run_clique ;;
        triangle) dataset_path_fn=triangle_dataset_path; run_fn=run_triangle ;;
    esac

    local total_steps=$(( ${#node_counts[@]} * runs ))
    echo "${dataset}: runs ${runs}, node-counts ${node_counts[*]}"

    for id in "${node_counts[@]}"; do
        local ds_path
        ds_path=$("${dataset_path_fn}" "${id}")

        if [[ ! -f "${ds_path}" ]]; then
            printf "\nWarning: dataset not found, skipping n=%s: %s\n" "${id}" "${ds_path}" >&2
            step=$(( step + runs ))
            ProgressBar "$step" "$total_steps" "${id}" "${runs}"
            continue
        fi

        for ((run = 1; run <= runs; run++)); do
            if ! "${run_fn}" "${ds_path}" "${id}"; then
                failures=$(( failures + 1 ))
            fi
            step=$(( step + 1 ))
            ProgressBar "$step" "$total_steps" "${id}" "${run}"
        done
    done

    echo ""
    if [[ "${failures}" -gt 0 ]]; then
        echo "Completed with ${failures} failed job(s)." >&2
        exit 1
    fi
    echo "All simulations completed."
}
main "$@"
