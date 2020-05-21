cmake_minimum_required (VERSION 3.10)

add_subdirectory (dependencies)

add_executable (game source/game.c)
target_link_libraries (game bitcoin-explorer siaskynetpp)
