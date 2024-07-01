cmake_minimum_required(VERSION 3.10)

project(sqlsplitter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add compile options
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -Wno-unknown-pragmas -Wno-import -O3)
endif()

# Include directories
include_directories(./src)

# Add executable
add_executable(sqlsplitter ./src/main.cpp)

# Custom clean target
add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake
)

# Generate a custom cmake_clean.cmake file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake_clean.cmake
"
file(REMOVE_RECURSE
  \${CMAKE_BINARY_DIR}/CMakeFiles
  \${CMAKE_BINARY_DIR}/CMakeCache.txt
  \${CMAKE_BINARY_DIR}/cmake_install.cmake
  \${CMAKE_BINARY_DIR}/Makefile
  \${CMAKE_BINARY_DIR}/sqlsplitter
)
")
