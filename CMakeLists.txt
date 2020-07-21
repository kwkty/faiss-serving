# As of 2020/06/27, v3.13.4 is the latest version of cmake installed via apt on debian:10
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(faiss_serving)

if (NOT UNIX)
    # In the first place, faiss doesn't support windows
    message(FATAL_ERROR "Your OS is not supported. Use Linux or OS X.")
endif ()

set(CMAKE_CXX_STANDARD 17)

option(BUILD_TEST "Build tests" ON)
option(BUILD_WITH_GPU "Build faiss with gpu (cuda) support" OFF)

set(MKLROOT "/opt/intel/mkl" CACHE PATH "Root path to the MKL")

include(ExternalProject)
include(FetchContent)

if (APPLE)
    # Suppress a warning (no dynamic libraries).
    set(CMAKE_MACOSX_RPATH OFF)
endif ()

# gflags

FetchContent_Declare(
        gflags
        GIT_REPOSITORY https://github.com/gflags/gflags
        GIT_TAG v2.2.2
)

# FetchContent_MakeAvailable is not available on v3.13 yet, so use the old way.

FetchContent_GetProperties(gflags)

if (NOT gflags_POPULATED)
    FetchContent_Populate(gflags)
    add_subdirectory(${gflags_SOURCE_DIR} ${gflags_BINARY_DIR})
endif ()

# gRPC

FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG v1.26.0
)

# FetchContent_MakeAvailable is unavailable on v3.13, so use an old way.
FetchContent_GetProperties(grpc)

if (NOT grpc_POPULATED)
    FetchContent_Populate(grpc)
    add_subdirectory(${grpc_SOURCE_DIR} ${grpc_BINARY_DIR})
endif ()

# proto
# REF: https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP_UNSECURE grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

get_filename_component(_FAISS_INDEX_PROTO "protos/index.proto" ABSOLUTE)
get_filename_component(_FAISS_INDEX_PROTO_PATH "${_FAISS_INDEX_PROTO}" PATH)

set(_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/protos")

set(_FAISS_INDEX_PROTO_SRCS "${_PROTO_OUT_DIR}/index.pb.cc")
set(_FAISS_INDEX_PROTO_HEADERS "${_PROTO_OUT_DIR}/index.pb.h")
set(_FAISS_GRPC_SRCS "${_PROTO_OUT_DIR}/index.grpc.pb.cc")
set(_FAISS_GRPC_HEADERS "${_PROTO_OUT_DIR}/index.grpc.pb.h")

file(MAKE_DIRECTORY ${_PROTO_OUT_DIR})

add_custom_command(
        OUTPUT "${_FAISS_INDEX_PROTO_SRCS}" "${_FAISS_INDEX_PROTO_HEADERS}" "${_FAISS_GRPC_SRCS}" "${_FAISS_GRPC_HEADERS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${_PROTO_OUT_DIR}"
        --cpp_out "${_PROTO_OUT_DIR}"
        -I "${_FAISS_INDEX_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${_FAISS_INDEX_PROTO}"
        DEPENDS "${_FAISS_INDEX_PROTO}")

# MKL (faiss dependency)

if (APPLE)
    set(_MKL_LIBRARY_DIR ${MKLROOT}/lib)
    set(_MKL_INCLUDE_DIR ${MKLROOT}/include)
    set(_MKL_LIBRARIES
            ${_MKL_LIBRARY_DIR}/libmkl_intel_lp64.a
            ${_MKL_LIBRARY_DIR}/libmkl_core.a
            ${_MKL_LIBRARY_DIR}/libmkl_intel_thread.a)
else ()
    set(_MKL_LIBRARY_DIR ${MKLROOT}/lib/intel64)
    set(_MKL_INCLUDE_DIR ${MKLROOT}/include)

    # On Linux environment, libraries must be wrapped with --start-group and --end-group
    # https://software.intel.com/en-us/mkl-linux-developer-guide-linking-with-intel-mkl-cluster-software
    set(_MKL_LIBRARIES
            "-Wl,--start-group"
            ${_MKL_LIBRARY_DIR}/libmkl_intel_lp64.a
            ${_MKL_LIBRARY_DIR}/libmkl_core.a
            ${_MKL_LIBRARY_DIR}/libmkl_gnu_thread.a
            "-Wl,--end-group")
endif ()

# OpenMP (faiss dependency)

if (APPLE)
    # HACK: find_package(OpenMP) doesn't work well on my OS X environment due to the same problem below
    #
    # https://gitlab.kitware.com/cmake/cmake/issues/18520
    #
    # In addition, find_library(lib NAMES libomp.a) returns libomp.dylib path instead of the static file...
    # So define a variable directly, check the existence and add this variable to the target links.
    set(_OMP_LIBRARY /usr/local/lib/libomp.a)

    if (NOT EXISTS ${_OMP_LIBRARY})
        message(FATAL_ERROR "/usr/local/lib/libomp.a not found. Please install it via 'brew install libomp'.")
    endif ()
else ()
    find_package(OpenMP REQUIRED)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

# faiss

set(_FAISS_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/faiss/src/faiss)
set(_FAISS_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/faiss/lib)
set(_FAISS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/faiss/include)
set(_FAISS_LIBRARY ${_FAISS_LIBRARY_DIR}/libfaiss.a)

message(${_FAISS_LIBRARY_DIR})

if (${BUILD_WITH_GPU})
    set(_FAISS_CONFIGURE_OPTIONS --libdir=${_FAISS_LIBRARY_DIR} --includedir=${_FAISS_INCLUDE_DIR})
else ()
    set(_FAISS_CONFIGURE_OPTIONS --without-cuda --libdir=${_FAISS_LIBRARY_DIR} --includedir=${_FAISS_INCLUDE_DIR})
endif ()

if (BUILD_TEST AND (EXISTS ${_FAISS_LIBRARY}))
    # Use build cache
    ExternalProject_Add(
            faiss
            PREFIX faiss
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
    )
else ()
    ExternalProject_Add(
            faiss
            PREFIX faiss
            GIT_REPOSITORY https://github.com/facebookresearch/faiss
            GIT_TAG v1.6.3
            CONFIGURE_COMMAND cd ${_FAISS_SOURCE_DIR} && LDFLAGS=-L${_MKL_LIBRARY_DIR} ./configure ${_FAISS_CONFIGURE_OPTIONS}
            BUILD_COMMAND cd ${_FAISS_SOURCE_DIR} && LD_LIBRARY_PATH=${_MKL_LIBRARY_DIR} make
            INSTALL_COMMAND cd ${_FAISS_SOURCE_DIR} && make install
    )
endif ()

# Main Configuration

if (APPLE)
    # TODO
else ()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -m64 -Wall -g -O3  -msse4 -mpopcnt -fopenmp -Wno-sign-compare")
endif ()

add_definitions(-DFINTEGER=int)

add_executable(faiss_serving src/main.cc src/index_service.cc ${_FAISS_INDEX_PROTO_SRCS} ${_FAISS_GRPC_SRCS})
add_dependencies(faiss_serving faiss)

target_include_directories(faiss_serving PRIVATE ${_PROTO_OUT_DIR} ${_FAISS_INCLUDE_DIR} ${_MKL_INCLUDE_DIR})
target_link_libraries(faiss_serving gflags grpc++ ${_OMP_LIBRARY} ${_FAISS_LIBRARY} ${_MKL_LIBRARIES})

# Test

if (BUILD_TEST)
    FetchContent_Declare(
            gtest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.10.0
    )

    # FetchContent_MakeAvailable is unavailable on v3.13, so use an old way.
    FetchContent_GetProperties(gtest)

    if (NOT gtest_POPULATED)
        FetchContent_Populate(gtest)
        add_subdirectory(${gtest_SOURCE_DIR} ${gtest_BINARY_DIR})
    endif ()

    add_executable(tests_run test/test_index_service.cc src/index_service.cc ${_FAISS_INDEX_PROTO_SRCS} ${_FAISS_GRPC_SRCS})

    add_dependencies(tests_run faiss)

    target_include_directories(tests_run PRIVATE ${_PROTO_OUT_DIR} ${_FAISS_INCLUDE_DIR} ${_MKL_INCLUDE_DIR})
    target_link_libraries(tests_run gmock gtest gtest_main grpc++ ${_OMP_LIBRARY} ${_FAISS_LIBRARY} ${_MKL_LIBRARIES})
endif ()
