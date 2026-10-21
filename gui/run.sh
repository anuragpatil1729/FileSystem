#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"

# Ensure build directory exists
if [ ! -f "$DIR/build/vfs_gui" ]; then
    echo "Building VFS Qt GUI..."
    cmake -S "$DIR" -B "$DIR/build"
    cmake --build "$DIR/build"
fi

echo "Launching VFS Studio GUI..."
cd "$ROOT_DIR"
exec "$DIR/build/vfs_gui" "$@"
