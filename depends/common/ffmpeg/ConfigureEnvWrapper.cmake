#
# This wrapper script allows the setting of ENV variables when called
# from externalProject_add. Can be used for cases where ENV variables
# like PKG_CONFIG_PATH need to be set.
#
# Here is an example of the calling code:
#
#
# set(CONFIGURE_COMMAND_BODY ${CMAKE_SOURCE_DIR}/configure
# 								--prefix=${CMAKE_INSTALL_PREFIX})
# string(REPLACE ";" "@@" ENCODED_CONFIGURE_COMMAND_BODY "${CONFIGURE_COMMAND_BODY}")
#
# include(ExternalProject)
# externalproject_add(ffmpeg
#                     SOURCE_DIR ${CMAKE_SOURCE_DIR}
#                     CONFIGURE_COMMAND ${CMAKE_COMMAND}
#                       -DWrapperBinaryDir:PATH=<BINARY_DIR>
#                       -DWrapperSourceDir:PATH=<SOURCE_DIR>
#                       -DWrapperInstallDir:PATH=${CMAKE_INSTALL_PREFIX}
#                       -DWrapperConfigureCommand:STRING=${ENCODED_CONFIGURE_COMMAND_BODY} -P
#                       <SOURCE_DIR>/ConfigureEnvWrapper.cmake)
#

set(ENV{PATH} "${WrapperInstallDir}/bin:$ENV{PATH}")
string(REPLACE "@@" ";" WrapperConfigureCommand "${WrapperConfigureCommand}" )

set(ENV{PKG_CONFIG_PATH} ${WrapperInstallDir}/lib/pkgconfig)
set(ENV{LD_LIBRARY_PATH} ${WrapperInstallDir}/lib:$ENV{LD_LIBRARY_PATH})
set(ENV{LDFLAGS} "-L${WrapperInstallDir}/lib")
set(ENV{CFLAGS} "-I${WrapperInstallDir}/include")
set(ENV{CPPFLAGS} "-I${WrapperInstallDir}/include")
set(ENV{PATH} "${WrapperInstallDir}/bin:$ENV{PATH}")

message("
ConfigureEnvWrapper::WrapperBinaryDir ** ${WrapperBinaryDir} **
ConfigureEnvWrapper::WrapperSourceDir ** ${WrapperSourceDir} **
ConfigureEnvWrapper::WrapperInstallDir ** ${WrapperInstallDir} **
ConfigureEnvWrapper::WrapperConfigureCommand ** ${WrapperConfigureCommand} **
ConfigureEnvWrapper::PATH ** $ENV{PATH} **
")

execute_process(COMMAND ${WrapperConfigureCommand}
	    RESULT_VARIABLE status_code
#	    OUTPUT_VARIABLE log
)
execute_process(COMMAND "echo $PKG_CONFIG_PATH")
if(NOT status_code EQUAL 0)
	message(FATAL_ERROR "configure error in line: ${WrapperConfigureCommand}
		    status_code: ${status_code}")
endif()
