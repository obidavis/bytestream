include(${MAX_SDK_BASE}/script/max-pretarget.cmake)

set(FULL_DIR_PATH "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(CURRENT_DIR_NAME "${FULL_DIR_PATH}" NAME)
project(${CURRENT_DIR_NAME})

file(GLOB PROJECT_SRC
        "*.h"
        "*.hpp"
        "*.c"
        "*.cpp"
        ${MAX_SDK_INCLUDES}/common/commonsyms.c
)

add_library(
        ${PROJECT_NAME}
        MODULE
        ${PROJECT_SRC}
)

target_include_directories(
        ${PROJECT_NAME}
        PRIVATE
        "${MAX_SDK_INCLUDES}"
        "${MAX_SDK_MSP_INCLUDES}"
        "${MAX_SDK_JIT_INCLUDES}"
        "${BYTESTREAM_INCLUDES}"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
find_package(Boost REQUIRED COMPONENTS system)
if (Boost_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
endif ()



include(${MAX_SDK_BASE}/script/max-posttarget.cmake)