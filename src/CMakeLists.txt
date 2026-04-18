set(lib_src_files net/socket.cpp protocol/message.cpp)

# Only compile the server library file if on Linux
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND lib_src_files net/server.cpp)
endif()



add_library(core STATIC
	${lib_src_files}
)

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)
