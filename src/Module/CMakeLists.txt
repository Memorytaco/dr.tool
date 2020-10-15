set(SUBMODS modelf.c)

add_link_options(-ldl)
add_compile_options(-fPIC -fPIE -Wall)

foreach(ITEM ${SUBMODS})
  string(REGEX REPLACE "^mod(.*)\.c$" \\1 MODNAME ${ITEM})
  message(STATUS "ADD MODULE ${MODNAME} with ${ITEM}")
  add_library(${MODNAME} SHARED ${ITEM})
  target_link_libraries(${MODNAME} dlib)
  set_target_properties(${MODNAME} PROPERTIES PREFIX "mod")
  install(TARGETS ${MODNAME} DESTINATION .mod)
endforeach()

