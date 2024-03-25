# CMakeLists.txt
#
# Base build script for building with cmake (http://www.cmake.org)

cmake_minimum_required( VERSION 2.8 )

project( PDW )

# Project version number
set( PDW_VERSION_MAJOR 3 )
set( PDW_VERSION_MINOR 2 )
set( PDW_VERSION_MICRO 0 )

# Where is OpenSSL?
set( OPENSSL_VERSION 1_1 )
set( OPENSSL_PATH P:/Libraries/OpenSSL )

# Set base variables
set( PDW_GFXS )
set( PDW_INCLUDES )
set( PWD_SRCS )

# Subdirectories to go to
add_subdirectory( GFX )
add_subdirectory( utils )
add_subdirectory( decode )
include_directories( ${OPENSSL_PATH}/include )
link_directories( ${OPENSSL_PATH}/lib/$(ARCH) )

# Microsoft Visual Studio Specific definitions
if( MSVC )
  add_definitions( "-D_CRT_SECURE_NO_WARNINGS" )
endif( MSVC )

# Include Files
set( PDW_INCLUDES
  ${PDW_INCLUDES}
  resource.h
  pdw.h
  language.h
  gfx.h
  menu.h
  misc.h
  printer.h
  sigind.h
  slicer.h
  smtp.h
  smtp_int.h
  sound_in.h
  toolbar.h
)

# Source Files
set( PDW_SRCS
  ${PDW_SRCS}
  PDW.rc
  main.cpp
  initapp.c
  language.c
  gfx.c
  menu.c
  misc.cpp
  printer.c
  sigind.c
  smtp.c
  sound_in.cpp  
  toolbar.c
)

# Create executable
add_executable( PDW WIN32
  ${PDW_INCLUDES}
  ${PDW_SRCS}
)

# Required libraries
target_link_libraries( PDW
  Comctl32.lib
  Winmm.lib
  Ws2_32.lib
  libssl-${OPENSSL_VERSION}.lib
  libcrypto-${OPENSSL_VERSION}.lib 
)
