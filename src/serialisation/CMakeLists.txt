add_library(serialisation
        concepts.hpp
        storage.hpp
        type_info.cpp
        type_info.hpp
)

target_include_directories(serialisation PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(serialisation PRIVATE
        ${MAX_SDK_BASE}/c74support)

target_link_libraries(serialisation
        PUBLIC
    magic_enum
        maxutils
)

message(STATUS "Serialisation library added")