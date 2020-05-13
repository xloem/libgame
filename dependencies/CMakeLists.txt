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

set (LIBBITCOIN_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libbitcoin CACHE INTERNAL "")
set (LIBBITCOIN_LIBRARIES bitcoin-system CACHE INTERNAL "")
include(ExternalProject)
ExternalProject_Add(
	build-libbitcoin-system
	GIT_REPOSITORY ${CMAKE_CURRENT_SOURCE_DIR}/libbitcoin-system
	BUILD_IN_SOURCE ON
	CONFIGURE_COMMAND git config user.name cmake && git config user.email cmake@localhost && ./autogen.sh && ./configure --prefix=${LIBBITCOIN_PREFIX}
	BUILD_COMMAND make install
	INSTALL_COMMAND
	INSTALL_DIR ${LIBBITCOIN_PREFIX}
)
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
add_library(libbitcoin-system STATIC IMPORTED GLOBAL)
add_dependencies(libbitcoin-system build-libbitcoin-system)
##add_dependencies(bitcoin-system build-libbitcoin-system)
#set_target_properties(libbitcoin-system PROPERTIES IMPORTED_LOCATION test-libbitcoin-system INTERFACE_INCLUDE_DIRECTORIES ${LIBBITCOIN_PREFIX}/include)
