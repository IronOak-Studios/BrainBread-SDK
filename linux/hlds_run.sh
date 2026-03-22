#!/bin/sh
#
# BrainBread dedicated server launcher for Linux.
#
# Wraps Valve's hlds_linux binary with the stat_fix.so preload shim so
# the 32-bit GoldSrc engine can stat files on filesystems that use
# 64-bit inodes (overlayfs, Docker, various container runtimes).
#
# Usage:
#   ./hlds_run.sh -game brainbread +map bb_chp1_heavensgate +maxplayers 8
#
# The script expects to live next to (or be copied into) a standard HLDS
# installation that already contains hlds_linux, engine_i486.so, etc.
# stat_fix.so must be present in the same directory; build it with:
#   make stat_fix   (from the linux/ directory of the BrainBread SDK)
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Locate the HLDS root — default to the directory this script is in.
HLDS_DIR="${HLDS_DIR:-$SCRIPT_DIR}"

cd "$HLDS_DIR"

# Ensure required files exist.
if [ ! -x "$HLDS_DIR/hlds_linux" ]; then
    echo "Error: hlds_linux not found in $HLDS_DIR" >&2
    echo "Set HLDS_DIR or copy this script into your HLDS installation." >&2
    exit 1
fi

STAT_FIX="$HLDS_DIR/stat_fix.so"
if [ ! -f "$STAT_FIX" ]; then
    # Also check next to the script itself (build output location).
    STAT_FIX="$SCRIPT_DIR/stat_fix.so"
fi

export LD_LIBRARY_PATH="$HLDS_DIR:${LD_LIBRARY_PATH:-}"

# Detect the 32-bit dynamic linker.
LD32=""
for candidate in /lib32/ld-linux.so.2 /lib/ld-linux.so.2 /lib/i386-linux-gnu/ld-linux.so.2; do
    if [ -x "$candidate" ]; then
        LD32="$candidate"
        break
    fi
done

if [ -f "$STAT_FIX" ] && [ -n "$LD32" ]; then
    # Use the 32-bit linker to preload the shim — this avoids the
    # "wrong ELF class" error from the 64-bit ld.so on mixed systems.
    exec "$LD32" --preload "$STAT_FIX" "$HLDS_DIR/hlds_linux" "$@"
else
    # No shim available or no 32-bit linker found; run directly.
    # This works fine on filesystems with 32-bit inodes.
    exec "$HLDS_DIR/hlds_linux" "$@"
fi
