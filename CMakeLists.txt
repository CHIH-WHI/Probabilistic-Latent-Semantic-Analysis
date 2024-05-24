# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(ProbabilisticLatentSemanticAnalysis)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add subdirectory for IR-General-library
add_subdirectory(IR-General-Library)

# Find all .cpp files in the src directory
file(GLOB SOURCES "src/*.cpp")

# Add executable with all found .cpp files
add_executable(plsa ${SOURCES})

# Link the header-only library
target_link_libraries(plsa IRGeneralLibrary)
