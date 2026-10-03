SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" &>/dev/null && pwd)
docker buildx build -t nano-cross "$SCRIPT_DIR/.."
