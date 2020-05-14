cmake_minimum_required (VERSION 3.10)

add_subdirectory (dependencies)

add_executable (atest atest.c)
target_link_libraries (atest libbitcoin-client)
