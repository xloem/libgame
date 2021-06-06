cmake_minimum_required (VERSION 3.10)
project (libgame)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-psabi")
add_subdirectory (dependencies)


include_directories (include)

include_directories (PRIVATE ${SIASKYNETPP_INCLUDE_DIRS} PRIVATE ${JSON_INCLUDE_DIRS})
find_package (Threads)
find_package (OpenSSL REQUIRED)
#link_libraries (bitcoin-explorer)
#link_libraries (bitcoin-system bitcoin-protocol bitcoin-client bitcoin-network ${CMAKE_THREAD_LIBS_INIT} ${SIASKYNETPP_LIBRARIES} OpenSSL::Crypto)
link_libraries (${CMAKE_THREAD_LIBS_INIT} ${SIASKYNETPP_LIBRARIES} OpenSSL::Crypto)

#add_executable (game source/game.cpp source/storage.cpp source/storage_digests_openssl.cpp source/storage_siaskynet)
#target_compile_options(game PRIVATE -ggdb -Werror -Wall -Wextra -Wno-error=ignored-qualifiers)



add_executable (old-stream-up source/stream-up.cpp)


add_executable (old-stream-down source/stream-down.cpp)

add_library (tape-libgame SHARED source/ltfs/libtape-libgame.cpp)
install (TARGETS tape-libgame LIBRARY DESTINATION lib/ltfs)

add_executable (fuse-libgame source/fuse/fuse-libgame.cpp)
target_link_libraries (fuse-libgame fuse++)
