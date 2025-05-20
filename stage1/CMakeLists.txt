cmake_minimum_required(VERSION 3.10)
project(DoppelgangTools)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add MinLoader executable
add_executable(MinLoader WIN32 MinLoader.cpp)
# Link libraries
target_link_libraries(MinLoader PRIVATE Shell32.lib Shlwapi.lib)

# Add Obfuscator executable
add_executable(Obfuscator Obfuscator.cpp) 