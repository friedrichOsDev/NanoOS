#!/usr/bin/env bash
# Shared Docker/Podman helpers for the host-side NanoOS scripts.

NANOOS_SCRIPTS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NANOOS_REPO_ROOT="$(cd -- "$NANOOS_SCRIPTS_DIR/.." && pwd)"
NANOOS_IMAGE="${NANOOS_IMAGE:-nano-cross}"
NANOOS_CONTAINER_CLI=""

nanoos_select_container_runtime() {
    local requested="${NANOOS_CONTAINER_ENGINE:-auto}"

    case "$requested" in
        auto)
            if command -v docker >/dev/null 2>&1; then
                NANOOS_CONTAINER_CLI="docker"
            elif command -v podman >/dev/null 2>&1; then
                NANOOS_CONTAINER_CLI="podman"
            else
                echo "Error: Neither Docker nor Podman is installed." >&2
                echo "Install one or set NANOOS_CONTAINER_ENGINE=docker|podman." >&2
                return 1
            fi
            ;;
        docker|podman)
            if ! command -v "$requested" >/dev/null 2>&1; then
                echo "Error: Requested container engine '$requested' is not installed." >&2
                return 1
            fi
            NANOOS_CONTAINER_CLI="$requested"
            ;;
        *)
            echo "Error: NANOOS_CONTAINER_ENGINE must be 'auto', 'docker' or 'podman'." >&2
            return 1
            ;;
    esac

    if ! "$NANOOS_CONTAINER_CLI" info >/dev/null 2>&1; then
        echo "Error: $NANOOS_CONTAINER_CLI is installed but its service/machine is unavailable." >&2
        if [[ "$NANOOS_CONTAINER_CLI" == "podman" ]]; then
            echo "On macOS, start the Podman machine with: podman machine start" >&2
        fi
        return 1
    fi

    echo "Using container engine: $NANOOS_CONTAINER_CLI"
}

nanoos_build_image() {
    nanoos_select_container_runtime || return 1

    "$NANOOS_CONTAINER_CLI" build \
        --tag "$NANOOS_IMAGE" \
        --file "$NANOOS_REPO_ROOT/scripts/Dockerfile" \
        "$NANOOS_REPO_ROOT/scripts"
}

nanoos_run_make() {
    nanoos_select_container_runtime || return 1

    if [[ $# -eq 0 ]]; then
        set -- clean iso
    elif [[ "$1" == "make" ]]; then
        shift
    fi

    local -a run_args
    run_args=(run --rm --volume "$NANOOS_REPO_ROOT:/workspace" --workdir /workspace)

    if [[ -t 0 && -t 1 ]]; then
        run_args+=(-it)
    else
        run_args+=(-i)
    fi

    "$NANOOS_CONTAINER_CLI" "${run_args[@]}" "$NANOOS_IMAGE" make "$@"
}
