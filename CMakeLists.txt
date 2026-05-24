cmake_minimum_required(VERSION 3.12...4.2)
project(EnTT_Pacman)

# Glob every source and header under src/. CONFIGURE_DEPENDS makes the build
# system re-run the glob on each build, so adding or removing a file is picked
# up without manually re-running cmake.
file(GLOB_RECURSE pacman_sources CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

add_executable(pacman ${pacman_sources})

if(CMAKE_TOOLCHAIN_FILE)
    message("Using SDL2 toolchain file")
    message(${CMAKE_TOOLCHAIN_FILE})
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)
    target_link_libraries(pacman
        PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_mixer::SDL2_mixer
    )
else()
    message("Finding SDL2")
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
    find_package(SDL2 REQUIRED)
    find_package(SDL2_mixer REQUIRED)

    target_include_directories(pacman
        PRIVATE
        "${SDL2_INCLUDE_DIR}"
        "${SDL2_MIXER_INCLUDE_DIR}"
    )
    target_link_libraries(pacman
        PRIVATE
        "${SDL2_LIBRARY}"
        "${SDL2_MIXER_LIBRARY}"
    )
endif()

target_include_directories(pacman
    PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party"
    "${CMAKE_SOURCE_DIR}/src"
)

target_compile_features(pacman
    PRIVATE
    cxx_std_17
)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    target_compile_options(pacman
        PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

# The game loads its audio at runtime, relative to the executable. Copy the
# audio folder next to the built executable so SDL_GetBasePath() finds it.
add_custom_command(TARGET pacman POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/audio"
        "$<TARGET_FILE_DIR:pacman>/audio"
)
