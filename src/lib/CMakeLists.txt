cmake_minimum_required(VERSION 3.17)
project(swamp_typeinfo_serialize C)

set(CMAKE_C_STANDARD 11)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else()
    set(isDebug FALSE)
endif()



set(deps ../../deps/)


file(GLOB_RECURSE lib_src FOLLOW_SYMLINKS
        "*.c"
        )



add_library(swamp_typeinfo_serialize
    ${lib_src}
)

if (isDebug)
    message("Debug build detected")
    target_compile_definitions(swamp_typeinfo_serialize PUBLIC CONFIGURATION_DEBUG=1)
endif()


target_compile_options(swamp_typeinfo_serialize PRIVATE -Wall -Wextra -Wshadow -Weffc++ -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter)


target_include_directories(swamp_typeinfo_serialize PUBLIC ${deps}swamp/runtime-c/src/include)
target_include_directories(swamp_typeinfo_serialize PUBLIC ${deps}swamp/typeinfo-c/src/include)
target_include_directories(swamp_typeinfo_serialize PUBLIC ${deps}piot/tiny-libc/src/include)
target_include_directories(swamp_typeinfo_serialize PUBLIC ${deps}piot/flood-c/src/include)
target_include_directories(swamp_typeinfo_serialize PUBLIC ${deps}piot/clog/src/include)
target_include_directories(swamp_typeinfo_serialize PUBLIC ../include)
