cmake_minimum_required(VERSION 2.8)

project(CryptoFinal C)

set(BANKING_VERSION_MAJOR 0)
set(BANKING_VERSION_MINOR 1)

# Configuration

configure_file(
  "${PROJECT_SOURCE_DIR}/banking_constants.h.in"
  "${PROJECT_BINARY_DIR}/banking_constants.h"
)
include_directories("${PROJECT_BINARY_DIR}")

# Options

option(BUILD_ATM "Produce binaries for the ATM" ON)
option(BUILD_BANK "Produce binaries for the bank" ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)
option(BUILD_TESTING "Produce testing harnesses" OFF)

# Libraries

if(BUILD_ATM OR BUILD_BANK)
  # Add readline
  find_library(READLINE_LIBRARY_PATH readline)
  if(READLINE_LIBRARY_PATH)
    set(CURRENT_LIBRARIES ${CURRENT_LIBRARIES} ${READLINE_LIBRARY_PATH})
  else(READLINE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libreadline")
  endif(READLINE_LIBRARY_PATH)
endif(BUILD_ATM OR BUILD_BANK)

if(BUILD_BANK)
  # Add sqlite
  find_library(SQLITE_LIBRARY_PATH sqlite)
  if(SQLITE_LIBRARY_PATH)
    set(CURRENT_LIBRARIES ${CURRENT_LIBRARIES} ${SQLITE_LIBRARY_PATH})
  else(SQLITE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libsqlite")
  endif(SQLITE_LIBRARY_PATH)
endif(BUILD_BANK)

if(BUILD_PROXY)
  # Add openssl
  find_library(SSL_LIBRARY_PATH ssl)
  if(SSL_LIBRARY_PATH)
    set(CURRENT_LIBRARIES ${CURRENT_LIBRARIES} ${SSL_LIBRARY_PATH})
  else(SSL_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libssl")
  endif(SSL_LIBRARY_PATH)
endif(BUILD_PROXY)

# Executables

if(BUILD_ATM)
  add_executable(atm atm.c)
  set(CURRENT_EXECUTABLES atm ${CURRENT_EXECUTABLES})
  target_link_libraries(atm ${CURRENT_LIBRARIES})
endif(BUILD_ATM)

if(BUILD_BANK)
  add_executable(bank bank.c)
  set(CURRENT_EXECUTABLES bank ${CURRENT_EXECUTABLES})
  target_link_libraries(bank ${CURRENT_LIBRARIES})
endif(BUILD_BANK)

if(BUILD_PROXY)
  add_executable(proxy proxy.c)
  set(CURRENT_EXECUTABLES proxy ${CURRENT_EXECUTABLES})
  target_link_libraries(proxy ${CURRENT_LIBRARIES})
endif(BUILD_PROXY)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
install(
  TARGETS ${CURRENT_EXECUTABLES}
  RUNTIME DESTINATION "${EXECUTABLE_OUTPUT_PATH}"
)

# Testing

if(BUILD_TESTING)
  include(CTest)
  set(CTEST_PROJECT_NAME ${CMAKE_PROJECT_NAME})
endif(BUILD_TESTING)
