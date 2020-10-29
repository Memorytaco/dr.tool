cmake_minimum_required(VERSION 3.15)

project(Dtool VERSION 0.1.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

configure_file(dtool.h.in dtool.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} . util)

set(CORE cmd.c elf.c pkg.c disas.c vara.c)
list(APPEND CORE util/file.c)

add_link_options(-ldl -lcapstone)
add_compile_options(-fPIC -Wall)
add_library(dlib STATIC ${CORE})

# one main interface and one test tool
add_executable(dtool main.c)
add_executable(pkg pkginfo.c)
target_link_libraries(dtool dlib)
target_link_libraries(pkg dlib)

add_subdirectory(Package)

install(TARGETS dtool pkg DESTINATION bin/)
