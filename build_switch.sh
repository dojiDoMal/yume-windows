#!/usr/bin/env bash
# Runs inside the devkitPro MSYS2 bash shell (invoked from build.bat).
#
# Why a separate script: cmd.exe does not treat single quotes as a string
# delimiter, so a bash one-liner with ';' or '&&' passed via `bash -lc '...'`
# gets chopped by cmd's own parser. Keeping the shell logic in a file avoids
# all cross-shell quoting problems.
#
# Inputs (exported by build.bat as environment variables, Windows-style paths):
#   MSYS2_CMAKE_WIN      - path to the MSYS2 cmake.exe (required by devkitPro)
#   HOST_DXC_WIN         - host dxc.exe that generates the .nxs GLSL
#   HOST_SPIRV_CROSS_WIN - host spirv-cross.exe
#   PROJECT_DIR_WIN      - project root
set -euo pipefail

CMAKE="$(cygpath -u "$MSYS2_CMAKE_WIN")"
HOST_DXC_U="$(cygpath -m "$HOST_DXC_WIN")"
HOST_SPIRV_CROSS_U="$(cygpath -m "$HOST_SPIRV_CROSS_WIN")"

cd "$(cygpath -u "$PROJECT_DIR_WIN")"

"$CMAKE" --preset switch \
    -DHOST_DXC="$HOST_DXC_U" \
    -DHOST_SPIRV_CROSS="$HOST_SPIRV_CROSS_U"

"$CMAKE" --build build/switch

# The default build includes the sandbox's <target>_nro target, so the runnable
# homebrew lands next to the sandbox's build artifacts.
echo "Build done. NRO: build/switch/examples/sandbox/sandbox.nro"
