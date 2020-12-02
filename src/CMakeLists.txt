cmake_minimum_required(VERSION 3.15)

project(Dtool VERSION 0.2.0 LANGUAGES C)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)

configure_file(dtool.h.in dtool.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR} . util)

set(CORE command.c elf.c pkg.c disas.c vara.c log.c)
list(APPEND CORE util/file.c)

add_link_options(-ldl -lcapstone)

if(${Debug})
add_compile_options(-fPIC -Wall -Wextra -g)
else()
add_compile_options(-fPIC -Wall -Wextra)
endif()

add_library(dlib STATIC ${CORE})

# one main interface and one test tool
add_executable(dtool main.c)
add_executable(pkg pkginfo.c)
target_link_libraries(dtool dlib)
target_link_libraries(pkg dlib)

add_subdirectory(Package)

install(TARGETS dtool pkg DESTINATION bin/)
