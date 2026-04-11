# Add the target executable for the sodium-client
add_executable(sodium-server server-main.cpp)

# Link against the core libraries
target_link_libraries(sodium-server PRIVATE core)

# Includes the directories from the include folder
# technically don't need for now since target_link_libraries takes care of that
# target_include_directories(sodium-client PUBLIC ${PROJECT_SOURCE_DIR}/include)