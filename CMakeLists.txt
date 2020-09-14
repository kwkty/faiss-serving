# Faiss v1.6.4 uses CMake v3.17
cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

# Define this variable because src/main.cc uses it too
set(FAISS_SERVING_VERSION 0.1.1)

project(
        FaissServing
        VERSION ${FAISS_SERVING_VERSION}
        DESCRIPTION "A gRPC server for Faiss"
        LANGUAGES CXX
)

# Set options

option(FAISS_SERVING_ENABLE_GPU "Build faiss with gpu (cuda) support" OFF)
option(FAISS_SERVING_TEST "Build test" OFF)

# Check the environment

if (NOT UNIX)
    # In the first place, Faiss doesn't support windows
    message(FATAL_ERROR "Faiss Serving supports Linux or macOS only")
endif ()

# Set variables

set(CMAKE_CXX_STANDARD 17)

if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif ()

# Add subdirectories

add_subdirectory(protos)
add_subdirectory(src)

if (FAISS_SERVING_TEST)
    enable_testing()

    add_subdirectory(test)
endif ()
