# populate
find_package(Git QUIET)
#execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
#                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# siaskynetpp
add_subdirectory(siaskynetpp)

# at the moment, nlohmann::json is inside siaskynetpp already
set (JSON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/siaskynetpp/dependencies/json/include PARENT_SCOPE)

# libbitcoin
find_program(LIBTOOLIZE libtoolize)
if(NOT LIBTOOLIZE)
	message(FATAL_ERROR "libtool not found")
endif()
find_program(PKGCONFIG pkg-config)
if(NOT PKGCONFIG)
	message(FATAL_ERROR "pkg-config not found")
endif()
#find_package(Boost 1.62 REQUIRED
#	COMPONENTS chrono date_time filesystem iostreams locale log log_setup program_options regex system thread unit_test_framework)

set (EXTERNAL_MAKE_FLAGS "-j2" CACHE STRING "flags passed to make for external projects")

#set (LIBBITCOIN_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/prefix CACHE INTERNAL "")
#set (LIBBITCOIN_EPREFIX ${CMAKE_CURRENT_SOURCE_DIR}/prefix/${CMAKE_SYSTEM} CACHE INTERNAL "")
#set (LIBBITCOIN_INCLUDE_DIR ${LIBBITCOIN_PREFIX}/include)
#set (LIBBITCOIN_LIBRARY_DIR ${LIBBITCOIN_EPREFIX}/lib)

#set (LIBBITCOIN_LIBRARIES bitcoin-system CACHE INTERNAL "")

#include(ExternalProject)
#ExternalProject_Add(libsecp256k1
#	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libsecp256k1
#	DOWNLOAD_COMMAND cd <SOURCE_DIR> && ./autogen.sh
#	INSTALL_DIR ${LIBBITCOIN_PREFIX}
#	CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --exec-prefix=${LIBBITCOIN_EPREFIX}
#		--enable-module-recovery
#		--disable-openssl-tests # only the tests use openssl
#	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS}
#)
#
#include(ExternalProject)
#ExternalProject_Add(libicu
#	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/icu/icu4c/source
#	INSTALL_DIR ${LIBBITCOIN_PREFIX}
#	CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --exec-prefix=${LIBBITCOIN_EPREFIX} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
#	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS}
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/src
#	DEPENDS libsecp256k1
#	DEPENDS libicu
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-system.so.0.0.0
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-system/Makefile ]; then
#		${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/configure 
#			--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#			--without-examples --without-tests
#			--with-icu # needed for key encryption, so add guards if removed
#			"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#			"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#			"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install-exec install-docDATA install-pkgconfigDATA
#)
#add_custom_target(libbitcoin-system DEPENDS ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-system.so.0.0.0)
#add_library(bitcoin-system SHARED IMPORTED GLOBAL)
#add_dependencies(bitcoin-system libbitcoin-system)
#set_target_properties(bitcoin-system PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-system.so.0.0.0
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#	INTERFACE_COMPILE_OPTIONS -DWITH_ICU
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libzmq)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/src
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libzmq
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libzmq
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libzmq/Makefile ]; then 
#		${CMAKE_CURRENT_SOURCE_DIR}/libzmq/configure 
#		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install
#)
#add_custom_target(libzmq DEPENDS ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a)
#add_library(zmq SHARED IMPORTED GLOBAL)
#add_dependencies(zmq libzmq)
#set_target_properties(zmq PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libzmq.a
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/src
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.so.0.0.0
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-protocol/Makefile ]; then 
#		${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/configure 
#		--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#		--without-examples --without-tests
#		"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#		"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install-exec install-docDATA install-pkgconfigDATA
#)
#add_custom_target(libbitcoin-protocol DEPENDS libzmq libbitcoin-system ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.so.0.0.0)
#add_library(bitcoin-protocol SHARED IMPORTED GLOBAL)
#add_dependencies(bitcoin-protocol libbitcoin-protocol)
#set_target_properties(bitcoin-protocol PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-protocol.so.0.0.0
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/src
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.so.0.0.0
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network/Makefile ]; then 
#		${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/configure 
#			--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#			--without-examples --without-tests
#			"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#			"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#			"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install-exec install-docDATA install-pkgconfigDATA
#)
#add_custom_target(libbitcoin-network DEPENDS libbitcoin-system ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.so.0.0.0)
#add_library(bitcoin-network SHARED IMPORTED GLOBAL)
#add_dependencies(bitcoin-network libbitcoin-network)
#set_target_properties(bitcoin-network PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-network.so.0.0.0
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/src
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.so.0.0.0
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-client/Makefile ]; then 
#		${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/configure
#			--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#			--without-examples --without-tests
#			"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#			"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#			"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install-exec install-docDATA install-pkgconfigDATA
#)
#add_custom_target(libbitcoin-client DEPENDS libbitcoin-protocol ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.so.0.0.0)
#add_library(bitcoin-client SHARED IMPORTED GLOBAL)
#add_dependencies(bitcoin-client libbitcoin-client)
#set_target_properties(bitcoin-client PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-client.so.0.0.0
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#)
#
#file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-explorer)
#add_custom_command(
#	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/src
#	OUTPUT ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-explorer.so.0.0.0
#	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-explorer
#	COMMAND if ! [ -e ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/configure ]; then
#		autoreconf -i ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer
#	$<SEMICOLON> fi
#	COMMAND if ! [ -e ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-explorer/Makefile ]; then 
#		${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/configure 
#			--prefix=${LIBBITCOIN_PREFIX} --exec-prefix=${LIBBITCOIN_EPREFIX} #--disable-shared
#			--without-examples --without-tests
#			"CPPFLAGS=${CMAKE_C_FLAGS} -DBOOST_ALL_DYN_LINK=1 -I${LIBBITCOIN_INCLUDE_DIR}"
#			"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
#			"PKG_CONFIG_PATH=${LIBBITCOIN_EPREFIX}/lib/pkgconfig"
#	$<SEMICOLON> fi
#	COMMAND make ${EXTERNAL_MAKE_FLAGS}
#	COMMAND make install-exec install-docDATA install-pkgconfigDATA
#)
#add_custom_target(libbitcoin-explorer DEPENDS libbitcoin-client libbitcoin-network ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-explorer.so.0.0.0)
#add_library(bitcoin-explorer SHARED IMPORTED GLOBAL)
#add_dependencies(bitcoin-explorer libbitcoin-explorer)
#set_target_properties(bitcoin-explorer PROPERTIES
#	IMPORTED_LOCATION ${LIBBITCOIN_LIBRARY_DIR}/libbitcoin-explorer.so.0.0.0
#	INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_INCLUDE_DIR}
#)
