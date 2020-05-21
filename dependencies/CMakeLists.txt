# populate
find_package(Git QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# siaskynetpp
add_subdirectory(siaskynetpp)

# libbitcoin
find_package(Boost 1.62 REQUIRED
	COMPONENTS chrono date_time filesystem iostreams locale log log_setup program_options regex system thread unit_test_framework)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECP256K1 REQUIRED libsecp256k1)
find_program(LIBTOOLIZE libtoolize)
if(NOT LIBTOOLIZE)
	message(FATAL_ERROR "libtool not found")
endif()
find_program(PKGCONFIG pkg-config)
if(NOT PKGCONFIG)
	message(FATAL_ERROR "pkg-config not found")
endif()

set (EXTERNAL_MAKE_FLAGS "-j2" CACHE STRING "flags passed to make for external projects")

set (LIBBITCOIN_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/prefix CACHE INTERNAL "")
set (LIBBITCOIN_EPREFIX ${CMAKE_CURRENT_SOURCE_DIR}/prefix/${CMAKE_SYSTEM} CACHE INTERNAL "")
set (LIBBITCOIN_INCLUDE_DIR ${LIBBITCOIN_PREFIX}/include)
set (LIBBITCOIN_LIBRARY_DIR ${LIBBITCOIN_EPREFIX}/lib)

set (LIBBITCOIN_LIBRARIES bitcoin-system CACHE INTERNAL "")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system/Makefile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system
	COMMAND autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/configure 
		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
)
add_custom_command(
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system/Makefile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/src
	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin.a
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system
	COMMAND make ${EXTERNAL_MAKE_FLAGS}
	COMMAND make install-exec install-docDATA install-pkgconfigDATA
)
add_custom_target(libbitcoin-system DEPENDS ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin.a)
set_target_properties(libbitcoin-system PROPERTIES
	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin.a
	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
	INTERFACE_LINK_DIRECTORIES ${LIBBITCOIN_LIBRARY_DIR}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libzmq)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libzmq/Makefile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libzmq
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/libzmq && ./autogen.sh
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/configure 
		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
)
add_custom_command(
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libzmq/Makefile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/src
	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libzmq
	COMMAND make ${EXTERNAL_MAKE_FLAGS}
	COMMAND make install-exec install-docDATA install-pkgconfigDATA
)
add_custom_target(libzmq DEPENDS ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a)
set_target_properties(libzmq PROPERTIES
	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a
	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
	INTERFACE_LINK_DIRECTORIES ${LIBBITCOIN_LIBRARY_DIR}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol/Makefile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol
	COMMAND autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/configure 
		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
)
add_custom_command(
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol/Makefile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/src
	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.a
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol
	COMMAND make ${EXTERNAL_MAKE_FLAGS}
	COMMAND make check
	COMMAND make install-exec install-docDATA install-pkgconfigDATA
)
add_custom_target(libbitcoin-protocol DEPENDS libzmq libbitcoin-system ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.a)
set_target_properties(libbitcoin-protocol PROPERTIES
	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.a
	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
	INTERFACE_LINK_DIRECTORIES ${LIBBITCOIN_LIBRARY_DIR}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network/Makefile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network
	COMMAND autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/configure 
		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
)
add_custom_command(
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network/Makefile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/src
	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.a
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network
	COMMAND make ${EXTERNAL_MAKE_FLAGS}
	COMMAND make install-exec install-docDATA install-pkgconfigDATA
)
add_custom_target(libbitcoin-network DEPENDS libbitcoin-system ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.a)
set_target_properties(libbitcoin-network PROPERTIES
	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.a
	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
	INTERFACE_LINK_DIRECTORIES ${LIBBITCOIN_LIBRARY_DIR}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client)
add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client/Makefile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client
	COMMAND autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/configure 
		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
)
add_custom_command(
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client/Makefile
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/src
	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.a
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client
	COMMAND make ${EXTERNAL_MAKE_FLAGS}
	COMMAND make install-exec install-docDATA install-pkgconfigDATA
)
add_custom_target(libbitcoin-client DEPENDS libbitcoin-protocol ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.a)
set_target_properties(libbitcoin-client PROPERTIES
	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.a
	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
	INTERFACE_LINK_DIRECTORIES ${LIBBITCOIN_LIBRARY_DIR}
)

include(ExternalProject)
ExternalProject_Add(
	build-libbitcoin-explorer
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/. build-libbitcoin-explorer/. &&
		cd build-libbitcoin-explorer &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} check
	INSTALL_COMMAND make install
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_dependencies(build-libbitcoin-explorer libbitcoin-client libbitcoin-network)
add_library(libbitcoin-explorer STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-explorer build-libbitcoin-explorer)
#file(MAKE_DIRECTORY ${LIBBITCOIN_INCLUDE_DIR})
#add_custom_command(
#	OUTPUT test-libbitcoin-system
#	COMMAND ./autogen.sh
#	COMMAND ./configure --prefix=${LIBBITCOIN_PREFIX}
#	COMMAND make
#	COMMAND make install
#	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system
#)
#add_custom_target(build-libbitcoin-system DEPENDS test-libbitcoin-system)
##add_custom_target(build-libbitcoin-system DEPENDS test-libbitcoin-system)
##add_dependencies(bitcoin-system build-libbitcoin-system)
#set_target_properties(libbitcoin-system PROPERTIES IMPORTED_LOCATION test-libbitcoin-system INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_PREFIX}/include)
