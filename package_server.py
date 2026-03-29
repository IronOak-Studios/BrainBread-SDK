#!/usr/bin/env python3
"""
Package BrainBread server distributions for Linux and Windows.

Creates two server-only archives from the built DLLs and mod data:
  - dist/brainbread-v{version}-linuxserver.tar.gz
  - dist/brainbread-v{version}-win32server.zip

Usage:
  python3 package_server.py -v 1.2 --maps-dir /path/to/maps

The version argument is validated against:
  - brainbread/liblist.gam   (must match exactly)
  - bb/dlls/game.cpp          (CLI version must be prefixed by the cvar version)

Built binaries and debug symbols are read from game/brainbread/dlls/
(the canonical post-build output directory):
  - Linux:   bb.so (stripped) + bb.so.dbg (DWARF debug symbols)
  - Windows: bb.dll (release)  + bb.pdb   (MSVC debug symbols)

Map BSP files are not stored in the brainbread/ repository and must be
supplied via the --maps-dir argument.  All .bsp files found in that
directory are copied into brainbread/maps/ in the archives.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

# ---------------------------------------------------------------------------
# Path constants (relative to this script's location inside bb/)
# ---------------------------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)  # workspace root (parent of bb/)

BRAINBREAD_DIR = os.path.join(REPO_ROOT, "brainbread")
LIBLIST_GAM = os.path.join(BRAINBREAD_DIR, "liblist.gam")
GAME_CPP = os.path.join(SCRIPT_DIR, "dlls", "game.cpp")

BUILD_DLLS = os.path.join(REPO_ROOT, "game", "brainbread", "dlls")
LINUX_SO = os.path.join(BUILD_DLLS, "bb.so")
WIN_DLL = os.path.join(BUILD_DLLS, "bb.dll")
WIN_PDB = os.path.join(BUILD_DLLS, "bb.pdb")

DIST_DIR = os.path.join(REPO_ROOT, "dist")

# ---------------------------------------------------------------------------
# Exclusion rules -- client-only content stripped from server packages
# ---------------------------------------------------------------------------

# Directories to skip entirely during the os.walk of brainbread/
EXCLUDE_DIRS = {
    ".git",
    # Client DLLs
    "cl_dlls",
    # Existing dlls/ in the repo (replaced by the freshly built binary)
    "dlls",
    # Client-only: skybox TGAs, keyboard bindings, VGUI model images
    "gfx",
    # sprites/ is included -- the engine loads .spr files server-side
    # (e.g. Mod_NumForName for wallsmokepuff.spr, beam sprites, etc.)
    # Client-only: VGUI menus, loading screens, localization strings
    "resource",
    # Client-only: menu music / audio
    "media",
    # Client-only: HTML documentation
    "manual",
    # Client-only: HUD help popup configs
    "help",
    # Client-only: particle system visual configs
    "partsys",
}

# Individual files to skip (exact basename match)
EXCLUDE_FILES = {
    ".gitignore",
    # Windows icon / URL shortcut -- not needed on server
    "BrainBread.ico",
    "BrainBread.url",
    # Hammer map editor definition -- authoring tool only
    "halflife-bb.fgd",
    # Client-only key alias config
    "pe.cfg",
    # Client-only visual fader definitions
    "pe_faders.cfg",
    # Listen server config -- dedicated servers use server.cfg
    "listenserver.cfg",
    # Client-only spectator menu definitions
    "spectatormenu.txt",
    "spectcammenu.txt",
}

# File extensions to skip
EXCLUDE_EXTENSIONS = {".bak"}


# ---------------------------------------------------------------------------
# Version helpers
# ---------------------------------------------------------------------------

def parse_liblist_version(path: str) -> str:
    """Extract the version string from a liblist.gam file."""
    with open(path, "r") as f:
        for line in f:
            m = re.match(r'^version\s+"([^"]+)"', line)
            if m:
                return m.group(1)
    raise RuntimeError(f"Could not find 'version' field in {path}")


def parse_gamecpp_version(path: str) -> str:
    """Extract the version from the peversion/bbversion cvar in game.cpp.

    Looks for a line like:
        cvar_t  peversion = {"bbversion", "BrainBread v1.2", ...};
    Returns the numeric portion after 'v' (e.g. "1.2").
    """
    with open(path, "r") as f:
        for line in f:
            m = re.search(
                r'peversion\s*=\s*\{[^}]*"BrainBread\s+v([0-9]+(?:\.[0-9]+)*)"',
                line,
            )
            if m:
                return m.group(1)
    raise RuntimeError(
        f"Could not find peversion/bbversion cvar in {path}"
    )


def validate_version(cli_version: str) -> None:
    """Validate the CLI version against liblist.gam and game.cpp."""
    errors = []

    # --- liblist.gam: must match exactly ---
    try:
        ll_version = parse_liblist_version(LIBLIST_GAM)
        if ll_version != cli_version:
            errors.append(
                f"liblist.gam version is \"{ll_version}\" but you specified "
                f"\"{cli_version}\".  Update {LIBLIST_GAM} or use the correct "
                f"version argument."
            )
    except RuntimeError as exc:
        errors.append(str(exc))

    # --- game.cpp: CLI version must start with the cvar version ---
    try:
        cpp_version = parse_gamecpp_version(GAME_CPP)
        if not cli_version.startswith(cpp_version):
            errors.append(
                f"game.cpp bbversion cvar is \"v{cpp_version}\" but you "
                f"specified \"{cli_version}\" (expected it to start with "
                f"\"{cpp_version}\").  Update the cvar in {GAME_CPP} or use "
                f"a matching version argument."
            )
    except RuntimeError as exc:
        errors.append(str(exc))

    if errors:
        print("Version validation failed:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        sys.exit(1)


# ---------------------------------------------------------------------------
# liblist.gam fixer
# ---------------------------------------------------------------------------

def fix_liblist(src: str, dst: str) -> None:
    """Copy liblist.gam, fixing gamedll_linux to point to dlls/bb.so."""
    with open(src, "r") as f:
        content = f.read()

    content = re.sub(
        r'^(gamedll_linux\s+").*(")',
        r"\1dlls/bb.so\2",
        content,
        flags=re.MULTILINE,
    )

    with open(dst, "w") as f:
        f.write(content)


# ---------------------------------------------------------------------------
# Staging
# ---------------------------------------------------------------------------

def stage_mod_data(staging_dir: str) -> str:
    """Copy server-relevant mod data to staging_dir/brainbread/.

    Excludes client-only directories, files, and extensions defined in
    the EXCLUDE_* constants above.

    Returns the path to the staged brainbread/ directory.
    """
    dst = os.path.join(staging_dir, "brainbread")

    for dirpath, dirnames, filenames in os.walk(BRAINBREAD_DIR):
        # Prune excluded directories (modifies dirnames in-place)
        dirnames[:] = [
            d for d in dirnames if d not in EXCLUDE_DIRS
        ]

        rel = os.path.relpath(dirpath, BRAINBREAD_DIR)
        dst_dir = os.path.join(dst, rel)
        os.makedirs(dst_dir, exist_ok=True)

        for fname in filenames:
            if fname in EXCLUDE_FILES:
                continue
            _, ext = os.path.splitext(fname)
            if ext in EXCLUDE_EXTENSIONS:
                continue
            src_file = os.path.join(dirpath, fname)
            dst_file = os.path.join(dst_dir, fname)
            shutil.copy2(src_file, dst_file)

    return dst


def stage_liblist(staged_bb: str) -> None:
    """Replace the staged liblist.gam with the fixed version."""
    fix_liblist(LIBLIST_GAM, os.path.join(staged_bb, "liblist.gam"))


def stage_dll(staged_bb: str, dll_src: str, dll_name: str) -> None:
    """Copy a DLL/PDB into the staged brainbread/dlls/ directory."""
    dlls_dir = os.path.join(staged_bb, "dlls")
    os.makedirs(dlls_dir, exist_ok=True)
    shutil.copy2(dll_src, os.path.join(dlls_dir, dll_name))


def stage_linux_dlls(staged_bb: str) -> None:
    """Copy bb.so into staged dlls/, strip it, and create a proper .dbg file.

    The build system's gendbg.sh doesn't actually strip the binary, so we
    do it here:
      1. Copy bb.so to staging
      2. Extract debug symbols into bb.so.dbg  (objcopy --only-keep-debug)
      3. Strip the staged bb.so                (strip --strip-debug)
      4. Link the two                          (objcopy --add-gnu-debuglink)
    """
    dlls_dir = os.path.join(staged_bb, "dlls")
    os.makedirs(dlls_dir, exist_ok=True)

    dst_so = os.path.join(dlls_dir, "bb.so")
    dst_dbg = os.path.join(dlls_dir, "bb.so.dbg")

    shutil.copy2(LINUX_SO, dst_so)

    # Remove any existing debuglink section (the build system may have added
    # one already) before we strip and re-link.
    subprocess.run(
        ["objcopy", "--remove-section=.gnu_debuglink", dst_so],
        check=True,
    )
    subprocess.run(
        ["objcopy", "--only-keep-debug", dst_so, dst_dbg],
        check=True,
    )
    subprocess.run(
        ["strip", "--strip-debug", dst_so],
        check=True,
    )
    subprocess.run(
        ["objcopy", "--add-gnu-debuglink=" + dst_dbg, dst_so],
        check=True,
    )


def stage_maps(staged_bb: str, maps_dir: str) -> int:
    """Copy .bsp files from maps_dir into the staged brainbread/maps/.

    Returns the number of .bsp files copied.
    """
    dst_maps = os.path.join(staged_bb, "maps")
    os.makedirs(dst_maps, exist_ok=True)

    bsp_files = sorted(glob.glob(os.path.join(maps_dir, "*.bsp")))
    for bsp in bsp_files:
        shutil.copy2(bsp, os.path.join(dst_maps, os.path.basename(bsp)))

    return len(bsp_files)


# ---------------------------------------------------------------------------
# Archive creation
# ---------------------------------------------------------------------------

def create_tar_gz(staging_dir: str, output_path: str) -> None:
    """Create a .tar.gz archive from the staging directory."""
    with tarfile.open(output_path, "w:gz") as tar:
        tar.add(
            os.path.join(staging_dir, "brainbread"),
            arcname="brainbread",
        )


def create_zip(staging_dir: str, output_path: str) -> None:
    """Create a .zip archive from the staging directory."""
    bb_root = os.path.join(staging_dir, "brainbread")
    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for dirpath, dirnames, filenames in os.walk(bb_root):
            for fname in filenames:
                full = os.path.join(dirpath, fname)
                arcname = os.path.join(
                    "brainbread",
                    os.path.relpath(full, bb_root),
                )
                zf.write(full, arcname)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def check_prerequisites(maps_dir: str | None) -> None:
    """Check that required source files and directories exist."""
    missing = []
    for path, desc in [
        (BRAINBREAD_DIR, "Mod data directory (brainbread/)"),
        (LIBLIST_GAM, "brainbread/liblist.gam"),
        (GAME_CPP, "bb/dlls/game.cpp"),
        (LINUX_SO, "Linux server DLL (bb.so)"),
        (WIN_DLL, "Windows server DLL (bb.dll)"),
        (WIN_PDB, "Windows debug symbols (bb.pdb)"),
    ]:
        if not os.path.exists(path):
            missing.append(f"  {desc}\n    expected at: {path}")

    if maps_dir is not None:
        if not os.path.isdir(maps_dir):
            missing.append(
                f"  Maps directory\n    expected at: {maps_dir}"
            )
        else:
            bsp_count = len(glob.glob(os.path.join(maps_dir, "*.bsp")))
            if bsp_count == 0:
                missing.append(
                    f"  No .bsp files found in maps directory: {maps_dir}"
                )

    if missing:
        print("Missing required files:", file=sys.stderr)
        print("\n".join(missing), file=sys.stderr)
        sys.exit(1)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Package BrainBread server distributions.",
    )
    parser.add_argument(
        "-v", "--version",
        required=True,
        help="Release version (e.g. 1.2). Must match liblist.gam and game.cpp.",
    )
    parser.add_argument(
        "-m", "--maps-dir",
        default=None,
        help=(
            "Directory containing .bsp map files to include. "
            "If omitted, packages are built without maps."
        ),
    )
    args = parser.parse_args()
    version = args.version
    maps_dir = os.path.abspath(args.maps_dir) if args.maps_dir else None

    # 1. Prerequisite checks
    check_prerequisites(maps_dir)

    # 2. Version validation
    validate_version(version)

    print(f"Packaging BrainBread v{version} server distributions...")

    if maps_dir is None:
        print("  WARNING: No --maps-dir specified; packages will not "
              "contain .bsp map files.", file=sys.stderr)

    # 3. Create dist/ output directory
    os.makedirs(DIST_DIR, exist_ok=True)

    linux_archive = os.path.join(
        DIST_DIR, f"brainbread-v{version}-linuxserver.tar.gz"
    )
    win32_archive = os.path.join(
        DIST_DIR, f"brainbread-v{version}-win32server.zip"
    )

    # 4. Build archives using a temporary staging directory
    with tempfile.TemporaryDirectory(prefix="bb_pkg_") as tmpdir:
        # --- Linux server package ---
        print("Building Linux server package...")
        linux_stage = os.path.join(tmpdir, "linux")
        os.makedirs(linux_stage)
        staged_bb = stage_mod_data(linux_stage)
        stage_liblist(staged_bb)
        stage_linux_dlls(staged_bb)
        if maps_dir:
            n = stage_maps(staged_bb, maps_dir)
            print(f"  {n} map(s) included")
        create_tar_gz(linux_stage, linux_archive)
        print(f"  -> {linux_archive}")

        # --- Windows server package ---
        print("Building Windows server package...")
        win_stage = os.path.join(tmpdir, "win32")
        os.makedirs(win_stage)
        staged_bb = stage_mod_data(win_stage)
        stage_liblist(staged_bb)
        stage_dll(staged_bb, WIN_DLL, "bb.dll")
        stage_dll(staged_bb, WIN_PDB, "bb.pdb")
        if maps_dir:
            n = stage_maps(staged_bb, maps_dir)
            print(f"  {n} map(s) included")
        create_zip(win_stage, win32_archive)
        print(f"  -> {win32_archive}")

    print("Done.")


if __name__ == "__main__":
    main()
