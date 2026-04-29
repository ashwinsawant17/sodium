
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package(Curses REQUIRED)
endif()


# Add the target executable for the sodium-client
add_executable(sodium-client client.cpp client-main.cpp)

# Link against the core libraries
target_link_libraries(sodium-client PRIVATE core)

# Link against necessary external libraries
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(sodium-client PRIVATE ws2_32)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_include_directories(sodium-client PRIVATE ${CURSES_INCLUDE_DIRS})
	target_link_libraries(sodium-client PRIVATE ${CURSES_LIBRARIES})
endif()



# Includes the directories from the include folder
# technically don't need for now since target_link_libraries takes care of that
# target_include_directories(sodium-client PUBLIC ${PROJECT_SOURCE_DIR}/include)
