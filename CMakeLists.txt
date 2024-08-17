cmake_minimum_required(VERSION 3.27)

project(bytestream)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architecture" FORCE)
#set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15e" CACHE STRING "macOS deployment target" FORCE)

include(cmake/fetch_max_sdk_base.cmake)
include(cmake/fetch_maxutils.cmake)
include(cmake/fetch_zpp_bits.cmake)
include(cmake/fetch_sadamlib-sdk.cmake)
add_subdirectory(src/serialisation)
set(BYTESTREAM_LIBS maxutils zpp_bits sadamlib-sdk)

set(BYTESTREAM_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MAX_SDK_BASE}/c74support
)


file(GLOB PROJECTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*)
foreach(PROJECT_DIR ${PROJECTS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_DIR})
        if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_DIR}/CMakeLists.txt)
            continue()
        endif()
        get_filename_component(SUB_PROJECT_NAME ${PROJECT_DIR} NAME)
        if (NOT SUB_PROJECT_NAME MATCHES "bs\.+")
            continue()
        endif()
        message(STATUS "Adding project: ${SUB_PROJECT_NAME}")
        add_subdirectory(${PROJECT_DIR})
        target_link_libraries(${SUB_PROJECT_NAME} PRIVATE ${BYTESTREAM_LIBS})
        target_include_directories(${SUB_PROJECT_NAME} PRIVATE ${BYTESTREAM_INCLUDES})
        target_compile_definitions(${SUB_PROJECT_NAME} PRIVATE -D_LIBCPP_DISABLE_AVAILABILITY)
    endif()
endforeach()

#option(BUILD_TESTS "Build tests" ON)
#if (BUILD_TESTS)
#    enable_testing()
#    add_subdirectory(test)
#endif()