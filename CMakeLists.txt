cmake_minimum_required (VERSION 2.8)
project (process_doppelganging)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

set (srcs
	main.cpp
	file_helper.cpp
	win_api_ext.cpp
	binary_parser.cpp
	proc_context.cpp
)

set (hdrs
	file_helper.h
	win_api_ext.h
	win_structs.h
	win_kernel_defs.h
	binary_parser.h
	proc_context.h
)

add_executable (process_doppelganging ${hdrs} ${srcs})

INSTALL( TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT ${PROJECT_NAME} )
