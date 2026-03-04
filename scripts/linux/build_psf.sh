#!/bin/bash
SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" &>/dev/null && pwd)
DEST_DIR="${1:-$SCRIPT_DIR/../../assets}"
TEMP_DIR="/tmp/terminus_build"
SOURCE_URL="https://deac-riga.dl.sourceforge.net/project/terminus-font/terminus-font-4.49/terminus-font-4.49.1.tar.gz"
DEST_DIR=$(realpath -m "$DEST_DIR")

mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR" || exit

curl -sL "$SOURCE_URL" | tar -xz --strip-components=1 || { echo "Download failed"; exit 1; }
./configure >/dev/null 2>&1
make psf
mkdir -p "$DEST_DIR"
[[ -f ter-v16n.psf.gz ]] && zcat ter-v16n.psf.gz > "$DEST_DIR/font.psf"
[[ -f ter-v16n.psf ]] && cp ter-v16n.psf "$DEST_DIR/font.psf"
cd /
rm -rf "$TEMP_DIR"
