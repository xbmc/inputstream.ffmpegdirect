cmake_minimum_required(VERSION 3.5)
project(gas-preprocessor)

install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/gas-preprocessor.pl DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
