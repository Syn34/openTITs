cmake_minimum_required(VERSION 3.20)
project(openTITs LANGUAGES CXX)

# The decompiled functions in src/ are linked into the game in place of their
# recompiled versions. Generate the game code with
#   opentit-recomp recompile <EBOOT.elf> -o <gen> --decomp src --symbols symbols.txt
# and configure with -DOPENTIT_GENERATED_DIR=<gen> to build the game.
set(OPENTIT_DECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" CACHE PATH "Decompiled DECOMP() sources")

# Tools and the game land in the build folder itself: build/tools/opentit-recomp, build/game/opentit.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

enable_testing()
add_subdirectory(engine)
