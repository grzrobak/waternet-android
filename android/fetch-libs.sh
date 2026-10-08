#!/usr/bin/env bash
# Downloads the SDL2 libraries (pinned versions) into android/app/jni/.
# Needs git. Run once before building:  bash android/fetch-libs.sh
set -euo pipefail
cd "$(dirname "$0")/app/jni"

fetch() { # dir repo tag
    if [ -d "$1/.git" ] || [ -f "$1/CMakeLists.txt" ]; then echo "$1 already present"; return; fi
    git -c advice.detachedHead=false clone --depth 1 --branch "$3" "https://github.com/libsdl-org/$2.git" "$1"
}
fetch SDL       SDL       release-2.32.10
fetch SDL_image SDL_image release-2.8.12
fetch SDL_mixer SDL_mixer release-2.8.2
fetch SDL_ttf   SDL_ttf   release-2.24.0
# SDL_ttf bundles FreeType as a git submodule
if [ ! -f SDL_ttf/external/freetype/CMakeLists.txt ]; then
    git -C SDL_ttf submodule update --init --depth 1 external/freetype
fi
echo "Libraries ready."
