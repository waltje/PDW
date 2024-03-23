# CMakeLists.txt
#
# Build script for building with cmake (http://www.cmake.org)

set( PDW_INCLUDES
  ${PDW_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/ostype.h
  ${CMAKE_CURRENT_SOURCE_DIR}/binary.h
  ${CMAKE_CURRENT_SOURCE_DIR}/debug.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ipcnfg.h
  ${CMAKE_CURRENT_SOURCE_DIR}/messagequeue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/rs232.h
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.h
  
  PARENT_SCOPE
)

set( PDW_SRCS
  ${PDW_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/debug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ostype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/binary.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ipcnfg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/messagequeue.c
  ${CMAKE_CURRENT_SOURCE_DIR}/rs232.c
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
  
  PARENT_SCOPE
)

