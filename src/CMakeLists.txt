add_library(core
    net/socket.cpp

)

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)