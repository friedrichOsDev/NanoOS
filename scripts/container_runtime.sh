#!/usr/bin/env bash
# Shared Docker/Podman helpers for the host-side NanoOS scripts.

NANOOS_SCRIPTS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NANOOS_REPO_ROOT="$(cd -- "$NANOOS_SCRIPTS_DIR/.." && pwd)"
NANOOS_IMAGE="${NANOOS_IMAGE:-nano-cross}"
NANOOS_CONTAINER_CLI=""

nanoos_runtime_is_ready() {
    local cli="$1"

    if ! command -v "$cli" >/dev/null 2>&1; then
        return 1
    fi

    if "$cli" info >/dev/null 2>&1; then
        return 0
    fi

    echo "Warning: $cli is installed, but its service/machine is unavailable." >&2
    if [[ "$cli" == "podman" ]]; then
        echo "On macOS, start the Podman machine with: podman machine start" >&2
    fi
    return 1
}

nanoos_select_container_runtime() {
    local requested="${NANOOS_CONTAINER_ENGINE:-auto}"
    local candidate

    case "$requested" in
        auto)
            # Prefer Docker when it is actually usable, not merely installed.
            for candidate in docker podman; do
                if nanoos_runtime_is_ready "$candidate"; then
                    NANOOS_CONTAINER_CLI="$candidate"
                    echo "Using container engine: $NANOOS_CONTAINER_CLI"
                    return 0
                fi
            done

            echo "Error: No usable Docker or Podman runtime was found." >&2
            echo "Start the Docker daemon or Podman machine, or set NANOOS_CONTAINER_ENGINE=docker|podman." >&2
            return 1
            ;;
        docker|podman)
            if ! command -v "$requested" >/dev/null 2>&1; then
                echo "Error: Requested container engine '$requested' is not installed." >&2
                return 1
            fi
            if ! nanoos_runtime_is_ready "$requested"; then
                echo "Error: Requested container engine '$requested' is not available." >&2
                return 1
            fi
            NANOOS_CONTAINER_CLI="$requested"
            echo "Using container engine: $NANOOS_CONTAINER_CLI"
            ;;
        *)
            echo "Error: NANOOS_CONTAINER_ENGINE must be 'auto', 'docker' or 'podman'." >&2
            return 1
            ;;
    esac
}

nanoos_build_image() {
    if [[ -z "$NANOOS_CONTAINER_CLI" ]]; then
        nanoos_select_container_runtime || return 1
    fi

    "$NANOOS_CONTAINER_CLI" build \
        --tag "$NANOOS_IMAGE" \
        --file "$NANOOS_REPO_ROOT/scripts/Dockerfile" \
        "$NANOOS_REPO_ROOT/scripts"
}

nanoos_ensure_image() {
    if [[ -z "$NANOOS_CONTAINER_CLI" ]]; then
        nanoos_select_container_runtime || return 1
    fi

    if ! "$NANOOS_CONTAINER_CLI" image inspect "$NANOOS_IMAGE" >/dev/null 2>&1; then
        echo "Container image '$NANOOS_IMAGE' is missing; building it now."
        nanoos_build_image || return 1
    fi
}

nanoos_run_make() {
    nanoos_select_container_runtime || return 1
    nanoos_ensure_image || return 1

    # Preserve the Makefile's default target (currently 'iso'). Do not force
    # a clean build on every invocation; callers can explicitly request clean.
    if [[ "${1:-}" == "make" ]]; then
        shift
    fi

    local -a run_args
    run_args=(run --rm --volume "$NANOOS_REPO_ROOT:/workspace" --workdir /workspace)

    if [[ "$NANOOS_CONTAINER_CLI" == "podman" ]]; then
        # The image's vscode account is UID/GID 1000. Map the invoking host
        # user to that account so files created in the bind mount stay owned
        # by the host user in rootless Podman.
        run_args+=(--userns=keep-id:uid=1000,gid=1000)
    else
        # Docker has no portable equivalent of Podman's keep-id user namespace.
        # Run with the host numeric IDs to avoid root-owned build artifacts.
        run_args+=(--user "$(id -u):$(id -g)")
    fi

    if [[ -t 0 && -t 1 ]]; then
        run_args+=(-it)
    elif [[ -t 0 ]]; then
        run_args+=(-i)
    fi

    "$NANOOS_CONTAINER_CLI" "${run_args[@]}" "$NANOOS_IMAGE" make "$@"
}
