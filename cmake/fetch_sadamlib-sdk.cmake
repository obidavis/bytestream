include(FetchContent)
FetchContent_Declare(sadamlib-sdk
    GIT_REPOSITORY https://github.com/SiskaAdam/sadamlib-sdk.git
    GIT_TAG        master)
FetchContent_MakeAvailable(sadamlib-sdk)
add_library(sadamlib-sdk INTERFACE)
target_include_directories(sadamlib-sdk INTERFACE ${sadamlib-sdk_SOURCE_DIR}/src)