set(SUBMODS pkgelf.c)

add_link_options(-ldl)
add_compile_options(-fPIC -fPIE -Wall)

foreach(ITEM ${SUBMODS})
  string(REGEX REPLACE "^pkg(.*)\.c$" \\1 PKGNAME ${ITEM})
  message(STATUS "ADD PACKAGE ${PKGNAME} with ${ITEM}")
  add_library(${PKGNAME} SHARED ${ITEM})
  target_link_libraries(${PKGNAME} dlib)
  set_target_properties(${PKGNAME} PROPERTIES SUFFIX ".pkg" PREFIX "")
  install(TARGETS ${PKGNAME} DESTINATION packages)
endforeach()

