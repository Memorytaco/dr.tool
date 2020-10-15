cmake_minimum_required(VERSION 3.15)

project(Dtool VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

configure_file(dtool.h.in dtool.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} . util)

set(CORE cmd.c elf.c mod.c disas.c)
list(APPEND CORE util/file.c)

add_link_options(-ldl -lcapstone)
add_compile_options(-fPIC -Wall)
add_library(dlib STATIC ${CORE})

# one main interface and one test tool
add_executable(dtool main.c)
add_executable(modrun modrun.c)
target_link_libraries(dtool dlib)
target_link_libraries(modrun dlib)

add_subdirectory(Module)

install(TARGETS dtool modrun DESTINATION bin/)
