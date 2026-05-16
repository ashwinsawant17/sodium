set(lib_src_files net/socket.cpp protocol/message.cpp)


add_library(core STATIC
	${lib_src_files}
)

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)
