cmake_minimum_required(VERSION 3.15)

project(openssl VERSION 3.5.8 LANGUAGES C)

find_package(StrawberryPerl REQUIRED)

include(CheckSymbolExists)
check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
check_symbol_exists(_ARM64_ "Windows.h" _ARM64_)

if(_X86_)
	message(STATUS "Win32")
	set(OPENSSL_PLATFORM VC-WIN32)
elseif(_AMD64_)
	message(STATUS "x64")
	set(OPENSSL_PLATFORM VC-WIN64A)
elseif(_ARM64_)
	message(STATUS "arm64")
	set(OPENSSL_PLATFORM VC-WIN64-ARM)
else()
	message(FATAL_ERROR "Unsupported target architecture")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	set(OPENSSL_PLATFORM "${OPENSSL_PLATFORM}-UWP")
endif()

set(configure_options no-shared no-asm no-apps no-capieng no-tests no-ui-console no-winstore zlib --prefix=${CMAKE_INSTALL_PREFIX} --with-zlib-include=${OUTPUT_DIR}/include)

set(ENV{PATH} "${STRAWBERRYPERL_PATH};$ENV{PATH}")
execute_process(COMMAND perl Configure ${OPENSSL_PLATFORM} ${configure_options}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND_ECHO STDOUT)

add_custom_target(openssl ALL COMMAND nmake build_generated libcrypto.lib libssl.lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(DIRECTORY include/openssl DESTINATION include/ FILES_MATCHING PATTERN "*.h")
install(FILES libcrypto.lib libssl.lib DESTINATION lib/)
