cmake_minimum_required(VERSION 2.8)

set(CTEST_PROJECT_NAME CryptoFinal)
project(${CTEST_PROJECT_NAME} C)

set(PROJECT_SETTINGS_HEADER "commands.h")
configure_file(
  "${PROJECT_SOURCE_DIR}/${PROJECT_SETTINGS_HEADER}.in"
  "${PROJECT_SOURCE_DIR}/${PROJECT_SETTINGS_HEADER}"
)
include_directories("${PROJECT_SOURCE_DIR}")

option(BUILD_ATM "Produce binaries for the ATM" ON)
option(BUILD_BANK "Produce binaries for the bank" ON)
option(BUILD_PROXY "Produce binaries for the proxy" ON)
option(BUILD_TESTING "Produce test harness" OFF)

if(BUILD_ATM OR BUILD_BANK)
  find_library(READLINE_LIBRARY_PATH readline)
  if(READLINE_LIBRARY_PATH)
    set(EXTRA_LIBS ${EXTRA_LIBS} READLINE_LIBRARY_PATH)
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
    set(EXTRA_LIBS ${EXTRA_LIBS} SQLITE_LIBRARY_PATH)
  else(SQLITE_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libsqlite")
  endif(SQLITE_LIBRARY_PATH)
  add_executable(bank bank.c)
  target_link_libraries(bank ${EXTRA_LIBS})
endif(BUILD_BANK)

if(BUILD_PROXY)
  find_library(SSL_LIBRARY_PATH ssl)
  if(SSL_LIBRARY_PATH)
    set(EXTRA_LIBS ${EXTRA_LIBS} SSL_LIBRARY_PATH)
  else(SSL_LIBRARY_PATH)
    message(FATAL_ERROR "Cannot find libssl")
  endif(SSL_LIBRARY_PATH)
  add_executable(proxy proxy.c)
  target_link_libraries(proxy ${EXTRA_LIBS})
endif(BUILD_PROXY)

install(TARGETS atm bank proxy DESTINATION bin)

if(BUILD_TESTING)
  include(CTest)
endif(BUILD_TESTING)
