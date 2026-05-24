# Locates the SDL2_mixer library.
#
# This module defines
# SDL2_MIXER_LIBRARY, the name of the library to link against
# SDL2_MIXER_INCLUDE_DIR, where to find SDL_mixer.h
# SDL2_MIXER_FOUND, if false, do not try to link to SDL2_mixer
#
# $SDL2MIXERDIR is an environment variable that points at an SDL2_mixer
# install prefix. $SDL2DIR is also checked, since SDL2_mixer is usually
# installed alongside SDL2.
#
# This mirrors the bundled FindSDL2.cmake so that, if SDL2 is found, the
# matching SDL2_mixer in the same prefix is found the same way.

SET(SDL2_MIXER_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/homebrew # Homebrew on Apple Silicon
	/opt/csw # Blastwave
	/opt
	${SDL2_MIXER_PATH}
)

FIND_PATH(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h
	HINTS
	$ENV{SDL2MIXERDIR}
	$ENV{SDL2DIR}
	PATH_SUFFIXES include/SDL2 include
	PATHS ${SDL2_MIXER_SEARCH_PATHS}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(PATH_SUFFIXES lib64 lib/x64 lib)
else()
	set(PATH_SUFFIXES lib/x86 lib)
endif()

FIND_LIBRARY(SDL2_MIXER_LIBRARY
	NAMES SDL2_mixer
	HINTS
	$ENV{SDL2MIXERDIR}
	$ENV{SDL2DIR}
	PATH_SUFFIXES ${PATH_SUFFIXES}
	PATHS ${SDL2_MIXER_SEARCH_PATHS}
)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer
	REQUIRED_VARS SDL2_MIXER_LIBRARY SDL2_MIXER_INCLUDE_DIR
)
