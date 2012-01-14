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
set(BANKING_VERSION_MINOR 4)

set(BANKING_VERSION_STRING "v${BANKING_VERSION_MAJOR}.${BANKING_VERSION_MINOR}")

# Configuration

set(BANKING_TERMINAL_COMMAND "gnome-terminal" CACHE STRING "System wrapper shell")
set(BANKING_SERVER_PORT "49152" CACHE STRING "Default port for server")
set(BANKING_CLIENT_PORT "49153" CACHE STRING "Default port for client")
set(BANKING_EXECUTABLE_PATH "${PROJECT_BINARY_DIR}/bin" CACHE STRING "Location for built binaries")
mark_as_advanced(
  BANKING_TERMINAL_COMMAND
  BANKING_SERVER_PORT
  BANKING_CLIENT_PORT
  BANKING_EXECUTABLE_PATH
)
configure_file(
  "${PROJECT_SOURCE_DIR}/run_system.sh.in"
  "${PROJECT_BINARY_DIR}/run_system.sh"
)

configure_file(
  "${PROJECT_SOURCE_DIR}/banking_constants.h.in"
  "${PROJECT_BINARY_DIR}/banking_constants.h"
)
include_directories("${PROJECT_BINARY_DIR}")

# Options

option(BUILD_ATM "Produce binaries for the ATM" ON)
option(BUILD_BANK "Produce binaries for the bank" ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)

# Libraries

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

# Executables

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

# Testing

include(CTest)
if(BUILD_TESTING)
  set(CTEST_PROJECT_NAME ${CMAKE_PROJECT_NAME})
endif(BUILD_TESTING)

# Packaging

include(CPack)
