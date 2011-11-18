cmake_minimum_required(VERSION 2.8)

project(CryptoFinal C)

set(BANKING_VERSION_MAJOR 0)
set(BANKING_VERSION_MINOR 1)

configure_file(
  "${PROJECT_SOURCE_DIR}/banking_constants.h.in"
  "${PROJECT_BINARY_DIR}/banking_constants.h"
)
include_directories("${PROJECT_BINARY_DIR}")

option(BUILD_ATM "Produce binaries for the ATM" ON)
option(BUILD_BANK "Produce binaries for the bank" ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)
option(BUILD_TESTING "Produce testing harnesses" OFF)

if(BUILD_ATM OR BUILD_BANK)
  find_library(READLINE_LIBRARY_PATH readline)
  if(READLINE_LIBRARY_PATH)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${READLINE_LIBRARY_PATH})
  else(READLINE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libreadline")
  endif(READLINE_LIBRARY_PATH)
endif(BUILD_ATM OR BUILD_BANK)

if(BUILD_ATM)
  add_executable(atm atm.c)
  target_link_libraries(atm ${EXTRA_LIBS})
endif(BUILD_ATM)

if(BUILD_BANK)
  find_library(SQLITE_LIBRARY_PATH sqlite)
  if(SQLITE_LIBRARY_PATH)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${SQLITE_LIBRARY_PATH})
  else(SQLITE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libsqlite")
  endif(SQLITE_LIBRARY_PATH)
  add_executable(bank bank.c)
  target_link_libraries(bank ${EXTRA_LIBS})
endif(BUILD_BANK)

if(BUILD_PROXY)
  find_library(SSL_LIBRARY_PATH ssl)
  if(SSL_LIBRARY_PATH)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${SSL_LIBRARY_PATH})
  else(SSL_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libssl")
  endif(SSL_LIBRARY_PATH)
  add_executable(proxy proxy.c)
  target_link_libraries(proxy ${EXTRA_LIBS})
endif(BUILD_PROXY)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
install(
  TARGETS atm bank proxy
  RUNTIME DESTINATION "${EXECUTABLE_OUTPUT_PATH}"
)

if(BUILD_TESTING)
  include(CTest)
  set(CTEST_PROJECT_NAME ${CMAKE_PROJECT_NAME})
endif(BUILD_TESTING)
