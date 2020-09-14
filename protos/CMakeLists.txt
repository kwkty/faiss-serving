include(FetchContent)

FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG v1.32.0
)

FetchContent_MakeAvailable(grpc)

# proto
# REF: https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt

set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
set(_PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

get_filename_component(_FAISS_INDEX_PROTO "index.proto" ABSOLUTE)
get_filename_component(_FAISS_INDEX_PROTO_PATH "${_FAISS_INDEX_PROTO}" PATH)

set(_FAISS_INDEX_PROTO_SRCS "${_PROTO_OUT_DIR}/index.pb.cc")
set(_FAISS_INDEX_PROTO_HEADERS "${_PROTO_OUT_DIR}/index.pb.h")
set(_FAISS_GRPC_SRCS "${_PROTO_OUT_DIR}/index.grpc.pb.cc")
set(_FAISS_GRPC_HEADERS "${_PROTO_OUT_DIR}/index.grpc.pb.h")

add_custom_command(
        OUTPUT "${_FAISS_INDEX_PROTO_SRCS}" "${_FAISS_INDEX_PROTO_HEADERS}" "${_FAISS_GRPC_SRCS}" "${_FAISS_GRPC_HEADERS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${_PROTO_OUT_DIR}"
        --cpp_out "${_PROTO_OUT_DIR}"
        -I "${_FAISS_INDEX_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${_FAISS_INDEX_PROTO}"
        DEPENDS "${_FAISS_INDEX_PROTO}"
)

add_library(faiss_serving_protos ${_FAISS_INDEX_PROTO_SRCS} ${_FAISS_GRPC_SRCS})

target_include_directories(faiss_serving_protos PUBLIC ${_PROTO_OUT_DIR})
target_link_libraries(faiss_serving_protos PUBLIC gflags grpc++_reflection)
