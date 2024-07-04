set(C74_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/externals)
include(FetchContent)
FetchContent_Declare(
        max-sdk
        GIT_REPOSITORY https://github.com/Cycling74/max-sdk-base.git
        GIT_TAG        main
)

FetchContent_MakeAvailable(max-sdk)
set(MAX_SDK_BASE ${max-sdk_SOURCE_DIR})
