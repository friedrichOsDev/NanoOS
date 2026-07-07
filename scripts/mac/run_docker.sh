SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" &>/dev/null && pwd)
docker run -it -v "$SCRIPT_DIR/../..:/workspace" nano-cross make clean iso