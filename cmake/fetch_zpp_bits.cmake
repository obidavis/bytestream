include(FetchContent)
FetchContent_Declare(
        zpp_bits
        GIT_REPOSITORY https://github.com/eyalz800/zpp_bits.git
        GIT_TAG        v4.4.20
)
FetchContent_MakeAvailable(zpp_bits)
add_library(zpp_bits INTERFACE)
target_include_directories(zpp_bits INTERFACE ${zpp_bits_SOURCE_DIR})
message(STATUS "zpp_bits_SOURCE_DIR: ${zpp_bits_SOURCE_DIR}")