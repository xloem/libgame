# populate
find_package(Git QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# siaskynetpp
add_subdirectory(siaskynetpp)

# libbitcoin
find_package(Boost 1.62 REQUIRED
	COMPONENTS chrono date_time filesystem iostreams locale log_setup program_options regex system thread unit_test_framework)
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

set (LIBBITCOIN_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin CACHE INTERNAL "")
set (LIBBITCOIN_INCLUDE_DIR ${LIBBITCOIN_PREFIX}/include)
set (LIBBITCOIN_LIBRARY_DIR ${LIBBITCOIN_PREFIX}/lib)
set (LIBBITCOIN_LIBRARIES bitcoin-system CACHE INTERNAL "")
include(ExternalProject)
ExternalProject_Add(
	build-libbitcoin-system
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/. build-libbitcoin-system/. &&
		cd build-libbitcoin-system &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_library(libbitcoin-system STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-system build-libbitcoin-system)
#pkg_check_modules(ZEROMQ REQUIRED libzmq>=4.3.2)
ExternalProject_Add(
	build-libzmq
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/. build-libzmq/. &&
		cd build-libzmq &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_library(libzmq STATIC IMPORTED GLOBAL)
add_dependencies(libzmq build-libzmq)
ExternalProject_Add(
	build-libbitcoin-protocol
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/. build-libbitcoin-protocol/. &&
		cd build-libbitcoin-protocol &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-protocol/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_dependencies(build-libbitcoin-protocol build-libbitcoin-system)
add_dependencies(build-libbitcoin-protocol build-libzmq)
add_library(libbitcoin-protocol STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-protocol build-libbitcoin-protocol)
ExternalProject_Add(
	build-libbitcoin-network
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/. build-libbitcoin-network/. &&
		cd build-libbitcoin-network &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-network/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_dependencies(build-libbitcoin-network build-libbitcoin-system)
add_library(libbitcoin-network STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-network build-libbitcoin-network)
ExternalProject_Add(
	build-libbitcoin-client
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/. build-libbitcoin-client/. &&
		cd build-libbitcoin-client &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-client/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_dependencies(build-libbitcoin-client build-libbitcoin-protocol)
add_library(libbitcoin-client STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-client build-libbitcoin-client)
ExternalProject_Add(
	build-libbitcoin-explorer
	DOWNLOAD_COMMAND
		rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/. build-libbitcoin-explorer/. &&
		cd build-libbitcoin-explorer &&
		./autogen.sh &&
		./configure --prefix=${LIBBITCOIN_PREFIX} --disable-shared
		"CPPFLAGS=${CMAKE_C_FLAGS} -I${LIBBITCOIN_INCLUDE_DIR}"
		"LDFLAGS=-L${LIBBITCOIN_LIBRARY_DIR}"
		"PKG_CONFIG_PATH=${LIBBITCOIN_PREFIX}/lib/pkgconfig"
	UPDATE_COMMAND rsync -Pva ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-explorer/. .
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE ON
	BUILD_COMMAND make ${EXTERNAL_MAKE_FLAGS} install
	INSTALL_COMMAND ""
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
add_dependencies(build-libbitcoin-explorer build-libbitcoin-client build-libbitcoin-network)
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
