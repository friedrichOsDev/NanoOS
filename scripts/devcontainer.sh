#!/usr/bin/env bash
#
# Select the matching NanoOS devcontainer configuration and launch VSCodium.
# Dev Containers OSS currently reads the canonical .devcontainer/devcontainer.json,
# so this script materializes that file from the selected runtime template.
#
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
DEVCONTAINER_DIR="$REPO_ROOT/.devcontainer"

# shellcheck source=container_runtime.sh
source "$SCRIPT_DIR/container_runtime.sh"

if ! command -v codium >/dev/null 2>&1; then
    echo "Error: 'codium' was not found in PATH." >&2
    exit 1
fi

nanoos_select_container_runtime

case "$NANOOS_CONTAINER_CLI" in
    docker)
        TEMPLATE="$DEVCONTAINER_DIR/devcontainer.docker.json"
        # Do not let a stale Podman endpoint redirect Docker commands.
        unset DOCKER_HOST
        ;;
    podman)
        TEMPLATE="$DEVCONTAINER_DIR/devcontainer.podman.json"

        if ! command -v docker >/dev/null 2>&1; then
            echo "Error: Dev Containers OSS invokes the Docker CLI." >&2
            echo "Install the Docker-compatible Podman CLI (for example, podman-docker)." >&2
            exit 1
        fi

        if ! command -v systemctl >/dev/null 2>&1; then
            echo "Error: Cannot activate the rootless Podman API socket without systemctl." >&2
            exit 1
        fi

        export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
        export DOCKER_HOST="unix://$XDG_RUNTIME_DIR/podman/podman.sock"

        systemctl --user start podman.socket

        if [[ ! -S "$XDG_RUNTIME_DIR/podman/podman.sock" ]]; then
            echo "Error: Podman API socket was not created at:" >&2
            echo "  $XDG_RUNTIME_DIR/podman/podman.sock" >&2
            exit 1
        fi

        if ! docker info >/dev/null 2>&1; then
            echo "Error: The Docker-compatible CLI cannot reach the Podman API socket." >&2
            echo "Check that podman-docker (or a Docker CLI using DOCKER_HOST) is installed." >&2
            exit 1
        fi
        ;;
    *)
        echo "Error: Unsupported container runtime '$NANOOS_CONTAINER_CLI'." >&2
        exit 1
        ;;
esac

if [[ ! -f "$TEMPLATE" ]]; then
    echo "Error: Devcontainer template not found: $TEMPLATE" >&2
    exit 1
fi

# The OSS extension uses the standard config filename. Generate it atomically
# from the runtime-specific template so the editor always sees the correct config.
TEMP_CONFIG="$DEVCONTAINER_DIR/devcontainer.json.tmp"
cp -- "$TEMPLATE" "$TEMP_CONFIG"
mv -- "$TEMP_CONFIG" "$DEVCONTAINER_DIR/devcontainer.json"

echo "Selected $NANOOS_CONTAINER_CLI configuration."
echo "Starting NanoOS in VSCodium..."

# Ensure the shared build image exists before the extension tries to create the
# development container from it.
nanoos_ensure_image

# VSCodium may print Chromium option warnings when desktop launch flags are
# injected through environment variables. Those warnings are unrelated to
# NanoOS or the container runtime; launch the normal CLI without forwarding
# unrelated desktop flags from this wrapper.
exec codium --new-window "$REPO_ROOT" "$@"
