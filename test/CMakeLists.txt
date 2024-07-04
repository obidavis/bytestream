Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0 # or a later release
)
FetchContent_MakeAvailable(Catch2)
target_compile_definitions(Catch2 PUBLIC CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS)

file(GLOB TEST_SOURCES "*.cpp")
foreach(testsourcefile ${TEST_SOURCES})
    message(add test ${testsourcefile}w)
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    target_include_directories(${testname} PRIVATE ${BYTESTREAM_INCLUDES})
    target_link_libraries(${testname} PRIVATE Catch2::Catch2WithMain)
    target_include_directories(${testname} PRIVATE ${MAX_SDK_BASE}/c74support/max-includes)
endforeach(testsourcefile)

find_package(Boost REQUIRED COMPONENTS system)
if (Boost_FOUND)
    target_include_directories(test_serial PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(test_serial PRIVATE ${Boost_LIBRARIES})
endif ()
target_compile_definitions(test_serial PRIVATE -D_LIBCPP_DISABLE_AVAILABILITY)

