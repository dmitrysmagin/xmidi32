CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(xmi_to_midi)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

SET(xmi_sources
	file.cpp
	main.cpp
	xmi.cpp
)

SET(xmi_headers
	file.h
	xmi.h
)

ADD_EXECUTABLE(xmi_to_midi ${xmi_sources} ${xmi_headers})
INSTALL(TARGETS xmi_to_midi RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

