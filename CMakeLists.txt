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

cmake_minimum_required(VERSION 2.8)

project(Plouton C)

set(BANKING_VERSION_MAJOR 0)
set(BANKING_VERSION_MINOR 5)

set(BANKING_VERSION_STRING
    "v${BANKING_VERSION_MAJOR}.${BANKING_VERSION_MINOR}")

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "None, Debug, Release, RelWithDebInfo, or MinSizeRel" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

## Configuration

# Configure the driver script TODO make this universal
set(BANKING_TERMINAL_COMMAND "gnome-terminal"
    CACHE STRING "System wrapper shell")
set(BANKING_PORT_SERVER "49152"
    CACHE STRING "Default port for server")
set(BANKING_PORT_CLIENT "49153"
    CACHE STRING "Default port for client")
set(BANKING_EXECUTABLE_PATH "${PROJECT_BINARY_DIR}/bin"
    CACHE STRING "Location to build banking executables")
mark_as_advanced(
  BANKING_TERMINAL_COMMAND
  BANKING_PORT_SERVER
  BANKING_PORT_CLIENT
  BANKING_EXECUTABLE_PATH
)
configure_file(
  "${PROJECT_SOURCE_DIR}/run_system.sh.in"
  "${PROJECT_BINARY_DIR}/run_system.sh"
)

# Configure the constants (incl. PINs)
string(RANDOM LENGTH 3 ALPHABET "0123456789" BANKING_PIN_SEED)
# Randomize PINs in releases (and make easy for debugging)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(BANKING_PIN_ALICE "A${BANKING_PIN_SEED}")
  set(BANKING_PIN_BOB   "B${BANKING_PIN_SEED}")
  set(BANKING_PIN_EVE   "E${BANKING_PIN_SEED}")
else(CMAKE_BUILD_TYPE STREQUAL Debug)
  math(EXPR BANKING_PIN_SEED_ALICE "${BANKING_PIN_SEED} + 1")
  math(EXPR BANKING_PIN_SEED_BOB   "${BANKING_PIN_SEED} + 2")
  math(EXPR BANKING_PIN_SEED_EVE   "${BANKING_PIN_SEED} + 5")
  string(RANDOM
         LENGTH 16
         RANDOM_SEED ${BANKING_PIN_SEED_ALICE}
         BANKING_PIN_ALICE)
  string(RANDOM
         LENGTH 16
         RANDOM_SEED ${BANKING_PIN_SEED_BOB}
         BANKING_PIN_BOB)
  string(RANDOM
         LENGTH 16
         RANDOM_SEED ${BANKING_PIN_SEED_EVE}
         BANKING_PIN_EVE)
endif(CMAKE_BUILD_TYPE STREQUAL Debug)
# Force a refresh of the cache
set(BANKING_PIN_ALICE "${BANKING_PIN_ALICE}"
    CACHE STRING "Alice's PIN" FORCE)
set(BANKING_PIN_BOB   "${BANKING_PIN_BOB}"
    CACHE STRING   "Bob's PIN" FORCE)
set(BANKING_PIN_EVE   "${BANKING_PIN_EVE}"
    CACHE STRING   "Eve's PIN" FORCE)
mark_as_advanced(
  BANKING_PIN_ALICE
  BANKING_PIN_BOB
  BANKING_PIN_EVE
)
# Some useful default locations
set(BANKING_IP_ADDR "127.0.0.1" CACHE STRING "IP address of the bank")
if(NOT EXISTS "${PROJECT_BINARY_DIR}/db")
  file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/db")
endif(NOT EXISTS "${PROJECT_BINARY_DIR}/db")
set(BANKING_DB_FILE
    "${PROJECT_BINARY_DIR}/db/bank_accounts_${BANKING_PIN_SEED}.sqlite3"
    CACHE STRING "Location of the bank account database" FORCE)
# Configure and include the constants
configure_file(
  "${PROJECT_SOURCE_DIR}/banking_constants.h.in"
  "${PROJECT_BINARY_DIR}/banking_constants.h"
)
include_directories("${PROJECT_BINARY_DIR}")

## Options

option(BUILD_ATM   "Produce binaries for the ATM"   ON)
option(BUILD_BANK  "Produce binaries for the bank"  ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)

option(BANKING_DB_INIT "Perform initial population of bank accounts" ON)
if(BANKING_DB_INIT)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D BANKING_DB_INIT")
endif(BANKING_DB_INIT)

## Libraries

if(BUILD_ATM OR BUILD_BANK)
  find_library(READLINE_LIBRARY_PATH readline)
  find_library(CRYPTO_LIBRARY_PATH gcrypt)
endif(BUILD_ATM OR BUILD_BANK)

if(BUILD_BANK)
  find_library(SQLITE_LIBRARY_PATH sqlite3)
  find_library(THREAD_LIBRARY_PATH pthread)
endif(BUILD_BANK)

if(BUILD_ATM)
  # Add readline
  if(READLINE_LIBRARY_PATH)
    set(ATM_LIBRARIES ${ATM_LIBRARIES} ${READLINE_LIBRARY_PATH})
  else(READLINE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find readline library")
  endif(READLINE_LIBRARY_PATH)
  # Add gcrypt
  if(CRYPTO_LIBRARY_PATH)
    set(ATM_LIBRARIES ${ATM_LIBRARIES} ${CRYPTO_LIBRARY_PATH})
  else(CRYPTO_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find crypto library")
  endif(CRYPTO_LIBRARY_PATH)
endif(BUILD_ATM)

if(BUILD_BANK)
  # Add readline
  if(READLINE_LIBRARY_PATH)
    set(BANK_LIBRARIES ${BANK_LIBRARIES} ${READLINE_LIBRARY_PATH})
  else(READLINE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find readline library")
  endif(READLINE_LIBRARY_PATH)
  # Add gcrypt
  if(CRYPTO_LIBRARY_PATH)
    set(BANK_LIBRARIES ${BANK_LIBRARIES} ${CRYPTO_LIBRARY_PATH})
  else(CRYPTO_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find crypto library")
  endif(CRYPTO_LIBRARY_PATH)
  # Add sqlite3
  if(SQLITE_LIBRARY_PATH)
    set(BANK_LIBRARIES ${BANK_LIBRARIES} ${SQLITE_LIBRARY_PATH})
  else(SQLITE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find sqlite library")
  endif(SQLITE_LIBRARY_PATH)
  # Add pthreads
  if(THREAD_LIBRARY_PATH)
    set(BANK_LIBRARIES ${BANK_LIBRARIES} ${THREAD_LIBRARY_PATH})
  else(THREAD_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find thread library")
  endif(THREAD_LIBRARY_PATH)
endif(BUILD_BANK)

## Executables

if(BUILD_ATM)
  add_executable(atm atm.c)
  set(CURRENT_EXECUTABLES atm ${CURRENT_EXECUTABLES})
  target_link_libraries(atm ${ATM_LIBRARIES})
endif(BUILD_ATM)

if(BUILD_BANK)
  add_executable(bank bank.c)
  set(CURRENT_EXECUTABLES bank ${CURRENT_EXECUTABLES})
  target_link_libraries(bank ${BANK_LIBRARIES})
endif(BUILD_BANK)

if(BUILD_PROXY)
  add_executable(proxy proxy.c)
  set(CURRENT_EXECUTABLES proxy ${CURRENT_EXECUTABLES})
  target_link_libraries(proxy ${PROXY_LIBRARIES})
endif(BUILD_PROXY)

set(EXECUTABLE_OUTPUT_PATH "${BANKING_EXECUTABLE_PATH}")
install(
  TARGETS ${CURRENT_EXECUTABLES}
  RUNTIME DESTINATION "${EXECUTABLE_OUTPUT_PATH}"
)

## Testing, etc.

include(CTest)
if(BUILD_TESTING)
  set(CTEST_PROJECT_NAME ${CMAKE_PROJECT_NAME})
endif(BUILD_TESTING)

include(CPack)
