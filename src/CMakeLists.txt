add_library(core
    net/socket.cpp
    
    protocol/message.cpp
)

# Server executable source dir, only for POSIX systems with epoll
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(core net/server.cpp)
else()
    message(STATUS "Server disabled: requires Linux (epoll)")
endif()

target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)