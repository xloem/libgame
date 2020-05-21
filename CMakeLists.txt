cmake_minimum_required (VERSION 3.10)

add_subdirectory (dependencies)

find_package(OpenSSL REQUIRED)

include_directories (include)

add_executable (game source/game.cpp source/storage.cpp source/storage_digests_openssl.cpp)
target_link_libraries (game bitcoin-explorer siaskynetpp OpenSSL::Crypto)
