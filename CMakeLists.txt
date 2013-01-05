##
# Copyright 2011 by Tor E. Hagemann <hagemt@rpi.edu>
# This file is part of Plouton.
#
# Plouton is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Plouton is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Plouton.  If not, see <http://www.gnu.org/licenses/>.
##

## Plouton project (global constants)

# Plouton requires CMake 2.8.0+ and is pure C
cmake_minimum_required(VERSION 2.8)
project(Plouton C)

# We use a sophisticated version system
set(CMAKE_PROJECT_VERSION_MAJOR 0)
set(CMAKE_PROJECT_VERSION_MINOR 5)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION_TWEAK 0)
# Create a thorough variety
set(CMAKE_PROJECT_VERSION_HIGH
	"${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}")
set(CMAKE_PROJECT_VERSION_LOW
	"${CMAKE_PROJECT_VERSION_PATCH}.${CMAKE_PROJECT_VERSION_TWEAK}")
set(CMAKE_PROJECT_VERSION_SO
	"${CMAKE_PROJECT_VERSION_HIGH}.${CMAKE_PROJECT_VERSION_TWEAK}")
set(CMAKE_PROJECT_VERSION
	"${CMAKE_PROJECT_VERSION_HIGH}.${CMAKE_PROJECT_VERSION_LOW}")
# Allow customization for various builds
set(CMAKE_PROJECT_VERSION_SUFFIX "dev" CACHE STRING "A custom build label")
mark_as_advanced(FORCE CMAKE_PROJECT_VERSION_SUFFIX)
# Overall version string and logging tag
set(CMAKE_PROJECT_VERSION_STRING
	"v${CMAKE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_SUFFIX}")
set(CMAKE_PROJECT_VERSION_TAG
	"${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_VERSION_STRING}")

# Force declaration of a build type
set(CMAKE_BUILD_TYPES "(None|Debug|Release|RelWithDebInfo|MinSizeRel)")
if(NOT CMAKE_BUILD_TYPE MATCHES "${CMAKE_BUILD_TYPES}")
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "${CMAKE_BUILD_TYPES}" FORCE)
endif(NOT CMAKE_BUILD_TYPE MATCHES "${CMAKE_BUILD_TYPES}")

# Force library/executable code into certain locations
set(LIBRARY_OUTPUT_PATH "${CMAKE_BUILD_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BUILD_DIR}/bin")

## Configuration (pre-build triggers)

# Configure the driver script TODO make universal
set(BANKING_PORT_SERVER "49152"
	CACHE STRING "Default port for server")
set(BANKING_PORT_CLIENT "49153"
	CACHE STRING "Default port for client")
mark_as_advanced(FORCE
	BANKING_TERMINAL_COMMAND
	BANKING_PORT_SERVER
	BANKING_PORT_CLIENT
	BANKING_EXECUTABLE_PATH
)
configure_file(
	"${PROJECT_SOURCE_DIR}/run_system.sh.in"
	"${PROJECT_BINARY_DIR}/run_system.sh"
	@ONLY
)

# Configure the constants (incl. PINs)
if(NOT BANKING_PIN_SEED)
	string(RANDOM LENGTH 3 ALPHABET "0123456789" BANKING_PIN_SEED)
endif(NOT BANKING_PIN_SEED)
macro(random_pin NAME)
	math(EXPR SEED "${BANKING_PIN_SEED} ${ARGN}")
	string(RANDOM LENGTH 16 RANDOM_SEED ${SEED} BANKING_PIN_${NAME})
endmacro()
# Randomize PINs in releases (and make easy for debugging)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	set(BANKING_PIN_ALICE "A${BANKING_PIN_SEED}")
	set(BANKING_PIN_BOB   "B${BANKING_PIN_SEED}")
	set(BANKING_PIN_EVE   "E${BANKING_PIN_SEED}")
else(CMAKE_BUILD_TYPE STREQUAL Debug)
	random_pin("ALICE" + 1)
	random_pin("BOB"   + 2)
	random_pin("EVE"   + 5)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)
# Force a refresh of the cache
set(BANKING_PIN_ALICE "${BANKING_PIN_ALICE}"
	CACHE STRING "Alice's PIN (for ${CMAKE_BUILD_TYPE})" FORCE)
set(BANKING_PIN_BOB "${BANKING_PIN_BOB}"
	CACHE STRING "Bob's PIN (for ${CMAKE_BUILD_TYPE})" FORCE)
set(BANKING_PIN_EVE "${BANKING_PIN_EVE}"
	CACHE STRING "Eve's PIN (for ${CMAKE_BUILD_TYPE})" FORCE)
mark_as_advanced(FORCE
	BANKING_PIN_ALICE
	BANKING_PIN_BOB
	BANKING_PIN_EVE
)

# TODO implement IP protocol version designation
#option(USE_IPV6 "Build with IPv6 support (only)" OFF)

# Ensure a default address is configured (and cached)
if(NOT BANKING_ADDR_IPV4)
	set(BANKING_ADDR_IPV4 "127.0.0.1")
endif(NOT BANKING_ADDR_IPV4)
set(BANKING_ADDR_IPV4 "${BANKING_ADDR_IPV4}"
	CACHE STRING "IPv4 address of the bank")

# Configure the database interface
if(NOT BANKING_DB_TYPE)
	set(BANKING_DB_TYPE "sqlite3"
		CACHE STRING "Bank account database format" FORCE)
endif(NOT BANKING_DB_TYPE)
# For sqlite3 TODO implement and test other methods
if("${BANKING_DB_TYPE}" MATCHES "sqlite")
	if(NOT BANKING_DB_FILE OR NOT EXISTS BANKING_DB_FILE)
		if(NOT EXISTS "${PROJECT_BINARY_DIR}/db")
			file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/db")
		endif(NOT EXISTS "${PROJECT_BINARY_DIR}/db")
		set(BANKING_DB_FILE
			"${PROJECT_BINARY_DIR}/db/accounts_${BANKING_PIN_SEED}.${BANKING_DB_TYPE}"
		CACHE STRING "Location of the bank account database" FORCE)
	endif(NOT BANKING_DB_FILE OR NOT EXISTS BANKING_DB_FILE)
endif("${BANKING_DB_TYPE}" MATCHES "sqlite")
# Configure the account table if specified
option(BANKING_DB_INIT "Perform initial population of bank accounts" ON)
if(BANKING_DB_INIT)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D BANKING_DB_INIT")
endif(BANKING_DB_INIT)

# Options for what to build (always libplouton)
option(BUILD_ATM   "Produce binaries for the ATM"   ON)
option(BUILD_BANK  "Produce binaries for the bank"  ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)

## Libraries, Executables, etc. (build essentials)

# TODO: set REQUIRED more intelligently
find_library(PROMPT_LIBRARY_PATH readline REQUIRED)
find_library(CRYPTO_LIBRARY_PATH gcrypt REQUIRED)
find_library(DB_LIBRARY_PATH sqlite3 REQUIRED)
find_library(THREAD_LIBRARY_PATH pthread REQUIRED)

# FIXME: build everything
add_subdirectory(libplouton)
add_subdirectory(utilities)
add_subdirectory(ui)

## Testing, Packaging, etc. (post-build triggers)

# Use CTest
include(CTest)
if(BUILD_TESTING)
	set(CTEST_PROJECT_NAME "${CMAKE_PROJECT_NAME}")
	add_subdirectory(tests)
endif(BUILD_TESTING)

# Use CPack
include(CPack)
