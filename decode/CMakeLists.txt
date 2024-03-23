# CMakeLists.txt
#
# Build script for building with cmake (http://www.cmake.org)

set( PDW_INCLUDES
  ${PDW_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/decode.h
  ${CMAKE_CURRENT_SOURCE_DIR}/acars.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ermes.h
  ${CMAKE_CURRENT_SOURCE_DIR}/flex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/mobitex.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pocsag.h
  
  PARENT_SCOPE
)

set( PDW_SRCS
  ${PDW_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/decode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/acars.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ermes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/flex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mobitex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pocsag.cpp
  
  PARENT_SCOPE
)
