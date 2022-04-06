cmake_minimum_required(VERSION 3.17)
project(swamp_typeinfo_example C)

set(CMAKE_C_STANDARD 11)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else()
    set(isDebug FALSE)
endif()

set(deps ../../deps/)

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
    "${deps}piot/*/src/lib/*.c"
    "${deps}swamp/*/src/lib/*.c"
)

add_executable(swamp_typeinfo_example
    ${deps_src}
    main.c
)

if (isDebug)
    message("Debug build detected")
    target_compile_definitions(swamp_typeinfo_example PUBLIC CONFIGURATION_DEBUG=1)
endif()

target_compile_options(swamp_typeinfo_example PRIVATE -Wall -Wextra -Wshadow -Weffc++ -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter)

# target_include_directories(swamp_typeinfo_example PRIVATE ${deps}clog/src/include)
# target_include_directories(swamp_typeinfo_example PRIVATE ${deps}tiny-libc/src/include)
# target_include_directories(swamp_typeinfo_example PRIVATE ${deps}flood-c/src/include)

target_link_libraries(swamp_typeinfo_example swamp_typeinfo_serialize m)
