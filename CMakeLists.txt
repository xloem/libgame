cmake_minimum_required (VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory (dependencies)


include_directories (include)

include_directories (PRIVATE ${SIASKYNETPP_INCLUDE_DIRS} PRIVATE ${JSON_INCLUDE_DIRS})
find_package (Threads)
find_package (OpenSSL REQUIRED)
link_libraries (bitcoin-explorer)
link_libraries (bitcoin-system bitcoin-protocol bitcoin-client ${CMAKE_THREAD_LIBS_INIT} ${SIASKYNETPP_LIBRARIES} OpenSSL::Crypto)
#link_libraries (${CMAKE_THREAD_LIBS_INIT} ${SIASKYNETPP_LIBRARIES} OpenSSL::Crypto)

add_executable (game source/game.cpp source/storage.cpp source/storage_digests_openssl.cpp source/storage_siaskynet)

add_executable (old-stream-up source/stream-up.cpp)

add_executable (old-stream-down source/stream-down.cpp)
