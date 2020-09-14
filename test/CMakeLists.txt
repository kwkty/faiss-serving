include(FetchContent)

FetchContent_Declare(
        gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)

FetchContent_MakeAvailable(gtest)

add_executable(test_faiss_serving test_index_service.cc)

target_link_libraries(test_faiss_serving PRIVATE gmock gtest gtest_main faiss_serving_lib)
