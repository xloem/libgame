cmake_minimum_required (VERSION 3.10)

add_subdirectory (dependencies)

find_package(OpenSSL REQUIRED)

include_directories (include)

include_directories (PRIVATE ${SIASKYNETPP_INCLUDE_DIRS} PRIVATE ${JSON_INCLUDE_DIRS})
link_libraries (bitcoin-explorer ${SIASKYNETPP_LIBRARIES} OpenSSL::Crypto)

add_executable (game source/game.cpp source/storage.cpp source/storage_digests_openssl.cpp source/storage_siaskynet)

add_executable (old-stream-up source/stream-up.cpp)

add_executable (old-stream-down source/stream-down.cpp)
