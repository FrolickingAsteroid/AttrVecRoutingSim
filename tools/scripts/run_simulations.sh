#!/usr/bin/env bash
#
# Launch Simulator runs for the clique, triangle, rocketfuel and scion topologies.
# Run with --help for usage and examples.

set -o errexit -o pipefail -o noclobber -o nounset

if (( BASH_VERSINFO[0] < 4 || (BASH_VERSINFO[0] == 4 && BASH_VERSINFO[1] < 4) )); then
    echo "This script needs bash >= 4.4 (found ${BASH_VERSION})." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

# ════════════════════════════════════════════════════════════════
# Configuration — edit here, the rest of the script adapts
# ════════════════════════════════════════════════════════════════

SIMULATOR="./build/Simulator"

ALL_TOPOS=(clique triangle rocketfuel scion focus)
ALL_PROTOCOLS=(R R-notif G C)          # R = -R, R-notif = -R notif, G = -G, C = -C
DEFAULT_NODE_COUNTS=(4 8 16 32)
DEFAULT_AS=3257
DEFAULT_PERIODS=(1.0)                 # R / R-notif update period (-i)

# Rocketfuel: "file:algebra" pairs per AS (files live in data/datasets/rocketfuel/<AS>/)
declare -A ROCKET_COMBOS=(
    [3257]="DelayLength.tsv:lex-length-hops-suffix DelayHops.tsv:lex-hops-suffix"
    [1755]="DelayHops.tsv:lex-hops-suffix DelayLength.tsv:lex-length-hops-suffix"
    [1221]="DelayLengthWidth.tsv:lex-length-width-suffix DelayLengthWidth.tsv:prod-width-length-hops-suffix"
)

# Rocketfuel: top edges by centrality per AS, as comma-separated "id1 id2" pairs
declare -A ROCKET_TOP_EDGES=(
    [3257]="2 34, 34 45, 8 47, 45 6, 16 47, 8 9, 5 6"
    [1755]="18 45, 24 25, 0 18, 25 50, 24 19, 45 13, 0 24, 50 68, 3 19, 0 38"
)

# Rocketfuel failure modes and the Simulator flags each one adds.
# ("edges" is built per edge as: -F <id1> <id2>)
ROCKET_MODES=(normal all-edges cutset edges)
declare -A ROCKET_MODE_FLAGS=(
    [normal]=""
    [all-edges]="-F"
    [cutset]="--cutset"
)

# SCION: "file:algebra" pairs
SCION_COMBOS=(
    "data/datasets/scion/sciera/DelayLength.tsv:lex-length-hops-suffix"
    "data/datasets/scion/sciera/DelayLength.tsv:lex-length-explicit"
    "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-suffix"
    "data/datasets/scion/sciera/DelayHops.tsv:lex-hops-explicit"
)

# ════════════════════════════════════════════════════════════════
# Helpers
# ════════════════════════════════════════════════════════════════

function die  { echo "$(basename "$0"): $*" >&2; echo "Try --help." >&2; exit 1; }
function warn { printf '\nWarning: %s\n' "$*" >&2; }

function in_list {            # in_list <value> <item>...
    local x="$1" y; shift
    for y in "$@"; do
        if [[ "$x" == "$y" ]]; then return 0; fi
    done
    return 1
}

function is_uint { [[ "$1" =~ ^[0-9]+$ ]]; }
function is_rip  { [[ "$1" == R || "$1" == R-notif ]]; }

function progress_bar {       # progress_bar <done> <total>
    local pct=$(( $1 * 100 / $2 ))
    local filled=$(( pct * 40 / 100 ))
    local bar
    bar="$(printf '%*s' "$filled" '' | tr ' ' '#')$(printf '%*s' $(( 40 - filled )) '' | tr ' ' '-')"
    printf '\rProgress : [%s] %d%%' "$bar" "$pct"
}

# ════════════════════════════════════════════════════════════════
# Arg parsing
# ════════════════════════════════════════════════════════════════

function usage {
    local me topos modes
    me="$(basename "$0")"
    topos="$(IFS='|'; echo "${ALL_TOPOS[*]}")"
    modes="$(IFS='|'; echo "${ROCKET_MODES[*]}")"
    cat >&2 <<EOF
Usage: $me -t <topo> [options]
       $me -C <topo>                 delete the *.csv output of a topology

Topology:
  -t, --topo          ${topos}   (default: clique)

Clique / triangle:
  -T, --type          unit|equal           clique dataset variant (required for clique)
  -n, --node-counts   "4 8 16"             node counts (default: ${DEFAULT_NODE_COUNTS[*]})

Rocketfuel:
  -a, --as            "3257 1755"          AS number(s) (default: ${DEFAULT_AS}; known: ${!ROCKET_COMBOS[*]})
  -m, --mode          ${modes}
                                           failure mode (default: normal)
                        normal    : no failure
                        all-edges : fail every edge in turn, over all destinations (-F)
                        cutset    : cutset failure (${ROCKET_MODE_FLAGS[cutset]})
                        edges     : fail only the top-centrality edges, one at a time (-F id1 id2)
  -e, --edges         "18 45,24 25"        edge list for --mode edges (default: per-AS list)

General:
  -p, --protocols     "${ALL_PROTOCOLS[*]}"      protocols to launch
                                           (default: R for rocketfuel normal/all-edges and scion, else all)
  -P, --periods       "0.5 1 2" | 0.5:0.5:3
                                           update period(s) for R / R-notif (-i), as a list
                                           or a start:step:end range (default: ${DEFAULT_PERIODS[*]});
                                           G and C are not periodic and run once per step
  -N, --runs          N                    runs per step (default: 1)
      --dry-run                            print the Simulator commands instead of running them
  -C, --clean         <topo>               delete data/output/<topo>/**/*.csv
  -h, --help

Examples:
  $me -t clique -T unit -n "4 8" -N 3
  $me -t triangle -n "4 8 16"
  $me -t rocketfuel -a 3257                     # normal run
  $me -t rocketfuel -a 3257 -m all-edges
  $me -t rocketfuel -a 3257 -m cutset
  $me -t rocketfuel -a "3257 1755" -m edges -N 5
  $me -t rocketfuel -a 1755 -m edges -e "18 45,24 25"
  $me -t rocketfuel -a 3257 -m all-edges -p R -P 0.5:0.5:3
  $me -t clique -T unit -P "0.5 1 2"          # R/R-notif per period, G/C once
  $me -t scion --dry-run
EOF
    exit "${1:-1}"
}

function check_getopt {
    local rc=0
    getopt --test > /dev/null || rc=$?
    if (( rc != 4 )); then
        echo 'Enhanced (GNU) getopt is required; `getopt --test` failed.' >&2
        exit 1
    fi
}

function is_num { [[ "$1" =~ ^[0-9]+([.][0-9]+)?$|^[.][0-9]+$ ]]; }

function expand_periods {     # fills periods from "a b c" or "start:step:end"
    local spec="$1" start step end
    if [[ -z "$spec" ]]; then
        periods=("${DEFAULT_PERIODS[@]}")
    elif [[ "$spec" == *:* ]]; then
        IFS=':' read -r start step end <<< "$spec"
        is_num "${start:-x}" && is_num "${step:-x}" && is_num "${end:-x}" \
            || die "--periods range must be start:step:end with positive numbers (got: $spec)"
        awk -v s="$step" 'BEGIN { exit !(s > 0) }' || die "--periods step must be > 0 (got: $step)"
        mapfile -t periods < <(awk -v a="$start" -v s="$step" -v b="$end" \
            'BEGIN { for (i = 0; a + i*s <= b + s*1e-9; i++) printf "%g\n", a + i*s }')
        (( ${#periods[@]} > 0 )) || die "--periods range is empty (got: $spec)"
    else
        read -ra periods <<< "$spec"
    fi
}

function parse_args {
    local longopts=topo:,dataset:,type:,as:,mode:,edges:,protocols:,runs:,node-counts:,clean:,periods:,dry-run,help
    local shortopts=t:D:T:a:m:e:p:N:n:C:P:h
    local parsed
    parsed=$(getopt --options="$shortopts" --longoptions="$longopts" --name "$(basename "$0")" -- "$@") || usage
    eval set -- "$parsed"

    topo="clique"
    variant=""
    ases=("$DEFAULT_AS")
    mode="normal"
    edges_override=""
    protocols=()
    runs=1
    node_counts=("${DEFAULT_NODE_COUNTS[@]}")
    clean_target=""
    dry_run=0
    periods_spec=""

    while true; do
        case "$1" in
            -t|-D|--topo|--dataset) topo="$2";                   shift 2 ;;
            -T|--type)              variant="$2";                shift 2 ;;
            -a|--as)                read -ra ases <<< "$2";      shift 2 ;;
            -m|--mode)              mode="$2";                   shift 2 ;;
            -e|--edges)             edges_override="$2";         shift 2 ;;
            -p|--protocols)         read -ra protocols <<< "$2"; shift 2 ;;
            -N|--runs)              runs="$2";                   shift 2 ;;
            -n|--node-counts)       read -ra node_counts <<< "$2"; shift 2 ;;
            -C|--clean)             clean_target="$2";           shift 2 ;;
            -P|--periods)           periods_spec="$2";           shift 2 ;;
            --dry-run)              dry_run=1;                   shift ;;
            -h|--help)              usage 0 ;;
            --)                     shift; break ;;
            *)                      echo "Programming error" >&2; exit 3 ;;
        esac
    done

    expand_periods "$periods_spec"

    if (( ${#protocols[@]} == 0 )); then
        if [[ "$topo" == scion || ( "$topo" == rocketfuel && ( "$mode" == normal || "$mode" == all-edges ) ) ]]; then
            protocols=(R)
        else
            protocols=("${ALL_PROTOCOLS[@]}")
        fi
    fi
}

function validate_args {
    if [[ -n "$clean_target" ]]; then
        in_list "$clean_target" "${ALL_TOPOS[@]}" || die "--clean must be one of: ${ALL_TOPOS[*]} (got: $clean_target)"
        return 0
    fi

    in_list "$topo" "${ALL_TOPOS[@]}" || die "--topo must be one of: ${ALL_TOPOS[*]} (got: $topo)"

    if (( ! dry_run )) && [[ ! -x "$SIMULATOR" ]]; then
        echo "Error: ${SIMULATOR} not found or not executable (build first)." >&2
        exit 1
    fi

    local per
    for per in "${periods[@]}"; do
        { is_num "$per" && awk -v p="$per" 'BEGIN { exit !(p > 0) }'; } \
            || die "--periods values must be positive numbers (got: $per)"
    done
    if [[ -n "$periods_spec" ]] && ! in_list R "${protocols[@]}" && ! in_list R-notif "${protocols[@]}"; then
        warn "--periods only affects R / R-notif, which are not in --protocols (${protocols[*]})"
    fi

    { is_uint "$runs" && (( runs >= 1 )); } || die "--runs must be a positive integer (got: $runs)"

    local p
    for p in "${protocols[@]}"; do
        in_list "$p" "${ALL_PROTOCOLS[@]}" || die "unknown protocol '$p' (choose from: ${ALL_PROTOCOLS[*]})"
    done

    case "$topo" in
        clique|triangle)
            if [[ "$topo" == clique ]] && ! in_list "$variant" unit equal; then
                die "--type must be unit or equal for clique (got: ${variant:-<empty>})"
            fi
            local n
            for n in "${node_counts[@]}"; do
                is_uint "$n" || die "--node-counts must be integers (got: $n)"
            done
            ;;
        rocketfuel)
            in_list "$mode" "${ROCKET_MODES[@]}" || die "--mode must be one of: ${ROCKET_MODES[*]} (got: $mode)"
            local as
            for as in "${ases[@]}"; do
                [[ -n "${ROCKET_COMBOS[$as]+x}" ]] || die "unknown AS '$as' (configured: ${!ROCKET_COMBOS[*]})"
                if [[ "$mode" == edges && -z "$edges_override" && -z "${ROCKET_TOP_EDGES[$as]+x}" ]]; then
                    die "no top-edge list configured for AS $as; pass one with --edges"
                fi
            done
            ;;
    esac
}

# ════════════════════════════════════════════════════════════════
# Launching
# ════════════════════════════════════════════════════════════════

PIDS=()
period="${DEFAULT_PERIODS[0]}"   # current R period (set per step in main)
period_idx=0

function launch {             # launch <Simulator args...>  (runs in background)
    if (( dry_run )); then
        printf '  %q' "$SIMULATOR" "$@"; echo
        return 0
    fi
    "$SIMULATOR" "$@" > /dev/null &
    PIDS+=($!)
}

function launch_protocol {
    local proto="$1"; shift
    # G and C have no period: launch them only on the first period of each step
    if ! is_rip "$proto" && (( period_idx > 0 )); then return 0; fi
    case "$proto" in
        R)       launch -R "$@";;
        R-notif) launch -R notif "$@";;
        G)       launch -G "$@" ;;
        C)       launch -C "$@" ;;
    esac
}

function wait_all {
    local pid status=0
    for pid in "${PIDS[@]}"; do
        wait "$pid" || status=1
    done
    PIDS=()
    return "$status"
}

# ════════════════════════════════════════════════════════════════
# Topologies
#   plan_<topo>  fills STEPS (one entry = one parallel batch)
#   step_<topo>  runs one batch; its args are the words of a STEPS entry
# ════════════════════════════════════════════════════════════════

STEPS=()

# ── Clique ──────────────────────────────────────────────────────
function plan_clique {
    local id ds
    for id in "${node_counts[@]}"; do
        ds="data/datasets/clique/bclique_n${id}/DelayHops_${variant}.tsv"
        if [[ -f "$ds" ]]; then STEPS+=("$ds $id"); else warn "dataset not found, skipping n=$id: $ds"; fi
    done
}

function step_clique {
    local ds="$1" id="$2" proto
    for proto in "${protocols[@]}"; do
        if is_rip "$proto"; then
            launch_protocol "$proto" -f "$ds" -A -i "$period" -F 0 "$id" -t 0
        else
            launch_protocol "$proto" -f "$ds" -A -F 0 "$id" -t 0
        fi
    done
    wait_all
}

# ── Focus ──────────────────────────────────────────────────────
function plan_focus {
    local id ds
    for id in "${node_counts[@]}"; do
        ds="data/datasets/focus/focus_n${id}/focus_n${id}.tsv"
        if [[ -f "$ds" ]]; then STEPS+=("$ds $id"); else warn "dataset not found, skipping n=$id: $ds"; fi
    done
}

function step_focus {
    local ds="$1" id="$2" proto
    for proto in "${protocols[@]}"; do
        if is_rip "$proto"; then
            launch_protocol "$proto" -f "$ds" -A -i "$period" -F "$id" "$(( id + 1 ))" -t "$(( id + 1 ))"
        else
            launch_protocol "$proto" -f "$ds" -A -F "$id" "$(( id + 1 ))" -t "$(( id + 1 ))"
        fi
    done
    wait_all
}


# ── Triangle ────────────────────────────────────────────────────
function plan_triangle {
    local id ds
    for id in "${node_counts[@]}"; do
        ds="data/datasets/triangle/triangle_n${id}/triangle_n${id}.tsv"
        if [[ -f "$ds" ]]; then STEPS+=("$ds $id"); else warn "dataset not found, skipping n=$id: $ds"; fi
    done
}

function step_triangle {
    local ds="$1" id="$2" proto
    for proto in "${protocols[@]}"; do
        if is_rip "$proto"; then
            launch_protocol "$proto" -f "$ds" -i "$period" -F "$(( id - 1 ))" "$id" -t "$id" -m 2
        else
            launch_protocol "$proto" -f "$ds" -F "$(( id - 1 ))" "$id" -t "$id"
        fi
    done
    wait_all
}

# ── Rocketfuel ──────────────────────────────────────────────────
function plan_rocketfuel {
    local as combo file pair id1 id2 extra
    local -a pairs
    for as in "${ases[@]}"; do
        for combo in ${ROCKET_COMBOS[$as]}; do
            file="data/datasets/rocketfuel/${as}/${combo%%:*}"
            [[ -f "$file" ]] || warn "dataset not found for AS $as: $file"
        done

        if [[ "$mode" != edges ]]; then
            STEPS+=("$as")
            continue
        fi

        IFS=',' read -ra pairs <<< "${edges_override:-${ROCKET_TOP_EDGES[$as]}}"
        for pair in "${pairs[@]}"; do
            extra=""
            read -r id1 id2 extra <<< "$pair"
            if ! is_uint "${id1:-x}" || ! is_uint "${id2:-x}" || [[ -n "$extra" ]]; then
                die "bad edge '$pair' (expected two node ids, e.g. \"18 45\")"
            fi
            STEPS+=("$as $id1 $id2")
        done
    done
}

function step_rocketfuel {    # step_rocketfuel <AS> [id1 id2]
    local as="$1" combo file algebra proto
    local -a fail r_extra=()
    if [[ "$mode" == edges ]]; then
        fail=(-F "$2" "$3")
        r_extra=(-A)
    else
        read -ra fail <<< "${ROCKET_MODE_FLAGS[$mode]}"
    fi

    for combo in ${ROCKET_COMBOS[$as]}; do
        file="data/datasets/rocketfuel/${as}/${combo%%:*}"
        algebra="${combo##*:}"
        for proto in "${protocols[@]}"; do
            if is_rip "$proto"; then
                launch_protocol "$proto" -f "$file" "${r_extra[@]}" -a "$algebra" -i "$period" "${fail[@]}"
            else
                launch_protocol "$proto" -f "$file" -A -a "$algebra" "${fail[@]}"
            fi
        done
    done
    wait_all
}

# ── SCION ───────────────────────────────────────────────────────
function plan_scion {
    local combo
    for combo in "${SCION_COMBOS[@]}"; do
        [[ -f "${combo%%:*}" ]] || warn "dataset not found: ${combo%%:*}"
    done
    STEPS+=("sciera")
}

function step_scion {
    local combo file algebra proto
    for combo in "${SCION_COMBOS[@]}"; do
        file="${combo%%:*}"
        algebra="${combo##*:}"
        for proto in "${protocols[@]}"; do
            if is_rip "$proto"; then
                launch_protocol "$proto" -f "$file" -a "$algebra" -i "$period" -m 0 -A
            else
                launch_protocol "$proto" -f "$file" -A -a "$algebra" -m 0
            fi
        done
    done
    wait_all
}

# ════════════════════════════════════════════════════════════════
# Clean
# ════════════════════════════════════════════════════════════════

function clean_output {
    local dir="data/output/$1"
    [[ -d "$dir" ]] || die "output directory not found: $dir"
    echo "Cleaning simulation output from ${dir}"
    if (( dry_run )); then
        find "$dir" -name '*.csv' -type f -print
    else
        find "$dir" -name '*.csv' -type f -delete
    fi
}

# ════════════════════════════════════════════════════════════════
# Main
# ════════════════════════════════════════════════════════════════

function describe_run {
    case "$topo" in
        clique)     echo "clique ($variant): node-counts ${node_counts[*]}" ;;
        triangle)   echo "triangle: node-counts ${node_counts[*]}" ;;
        focus)   echo "focus: node-counts ${node_counts[*]}" ;;
        rocketfuel) echo "rocketfuel: AS ${ases[*]}, mode $mode" ;;
        scion)      echo "scion" ;;
    esac
    echo "  protocols ${protocols[*]}, periods ${periods[*]}, runs ${runs}, ${#STEPS[@]} step(s)"
}

function main {
    check_getopt
    parse_args "$@"
    validate_args

    if [[ -n "$clean_target" ]]; then
        clean_output "$clean_target"
        exit 0
    fi

    "plan_${topo}"
    (( ${#STEPS[@]} > 0 )) || die "nothing to run for $topo"
    describe_run

    local total=$(( ${#STEPS[@]} * ${#periods[@]} * runs ))
    local done_steps=0 failures=0 entry run
    local -a args

    for entry in "${STEPS[@]}"; do
        read -ra args <<< "$entry"
        for (( run = 1; run <= runs; run++ )); do
            for (( period_idx = 0; period_idx < ${#periods[@]}; period_idx++ )); do
                period="${periods[period_idx]}"
                if (( dry_run )); then echo "# step: ${entry} (period ${period}, run ${run}/${runs})"; fi
                if ! "step_${topo}" "${args[@]}"; then
                    failures=$(( failures + 1 ))
                    warn "step failed: ${entry} (period ${period}, run ${run})"
                fi
                done_steps=$(( done_steps + 1 ))
                if (( ! dry_run )); then progress_bar "$done_steps" "$total"; fi
            done
        done
    done

    echo ""
    if (( failures > 0 )); then
        echo "Completed with ${failures} failed step(s)." >&2
        exit 1
    fi
    echo "All simulations completed."
}

main "$@"