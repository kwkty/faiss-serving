include(ExternalProject)
include(FetchContent)

find_package(OpenMP REQUIRED)

FetchContent_Declare(
        gflags
        GIT_REPOSITORY https://github.com/gflags/gflags
        GIT_TAG v2.2.2
)

FetchContent_MakeAvailable(gflags)

# BLAS (Faiss dependency)

# Currently, it supports MKL only.
# NOTE: Faiss uses this environment variable internally to detect MKL path.
if (NOT DEFINED ENV{MKLROOT})
    message(FATAL_ERROR "env MKLROOT must be set")
endif ()


# HACK: Unable to find BLAS(MKL) even though env MKLROOT is set on macOS.
# Plus, on Linux, FindBLAS with options BLA_STATIC=ON and BLA_VENDOR=Intel10_64lp returns the dynamic library, not the static one.
# Maybe I made some mistakes, but use a workaround for now.

if (APPLE)
    set(_MKL_LIBRARY_DIR $ENV{MKLROOT}/lib)
    set(_MKL_INCLUDE_DIR $ENV{MKLROOT}/include)
    set(_MKL_LIBRARIES
            ${_MKL_LIBRARY_DIR}/libmkl_intel_lp64.a
            ${_MKL_LIBRARY_DIR}/libmkl_core.a
            ${_MKL_LIBRARY_DIR}/libmkl_intel_thread.a)
else ()
    set(_MKL_LIBRARY_DIR $ENV{MKLROOT}/lib/intel64)
    set(_MKL_INCLUDE_DIR $ENV{MKLROOT}/include)
    set(_MKL_LIBRARIES
            "-Wl,--start-group"
            ${_MKL_LIBRARY_DIR}/libmkl_intel_lp64.a
            ${_MKL_LIBRARY_DIR}/libmkl_core.a
            ${_MKL_LIBRARY_DIR}/libmkl_gnu_thread.a
            "-Wl,--end-group")
endif ()

# Faiss

set(_FAISS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/faiss/include)
set(_FAISS_LIBRARY ${CMAKE_CURRENT_BINARY_DIR}/faiss/lib/libfaiss.a)

# Use ExternalProject_Add instead of FetchContent_Declare to pass CMAKE_ARGS
ExternalProject_Add(
        faiss
        PREFIX faiss
        GIT_REPOSITORY https://github.com/facebookresearch/faiss
        GIT_TAG b4b576b1394e82a9ae7d36d85d3d0770884bb32c
        CMAKE_ARGS -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
)

# Add faiss_serving_lib for testing

add_library(faiss_serving_lib STATIC index_service.cc)

add_dependencies(faiss_serving_lib faiss)
target_include_directories(faiss_serving_lib PUBLIC ${_FAISS_INCLUDE_DIR} ${_MKL_INCLUDE_DIR})
target_link_libraries(faiss_serving_lib PUBLIC faiss_serving_protos OpenMP::OpenMP_CXX ${_FAISS_LIBRARY} ${_MKL_LIBRARIES})

# Add main executable

add_executable(faiss_serving main.cc)

target_link_libraries(faiss_serving PRIVATE gflags faiss_serving_lib)
target_compile_definitions(faiss_serving PRIVATE FAISS_SERVING_VERSION="${FAISS_SERVING_VERSION}")
