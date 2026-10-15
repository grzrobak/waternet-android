cmake_minimum_required(VERSION 3.16)
project(waternet CXX C)

set(CMAKE_CXX_STANDARD 11)
set(GAME_SOURCES src/waternet.cpp src/port.cpp)
set(GAME_FLAGS -DGP2X -Wno-write-strings -Wno-writable-strings -Wno-unknown-warning-option
               -Wno-format-security -Wno-unused-result)

if(ANDROID OR WATERNET_IN_TREE)
    # Built by Gradle: android/app/jni/CMakeLists.txt adds the SDL libraries
    # first, then this directory. Produces libmain.so for SDLActivity.
    add_library(main SHARED ${GAME_SOURCES})
    target_compile_options(main PRIVATE ${GAME_FLAGS})
    target_link_libraries(main PRIVATE SDL2::SDL2 SDL2_image SDL2_mixer SDL2_ttf $<$<BOOL:${ANDROID}>:log>)
else()
    # Desktop build, handy for testing: cmake -B build && cmake --build build
    find_package(SDL2 REQUIRED CONFIG)
    find_package(SDL2_image REQUIRED CONFIG)
    find_package(SDL2_mixer REQUIRED CONFIG)
    find_package(SDL2_ttf REQUIRED CONFIG)
    add_executable(waternet ${GAME_SOURCES})
    target_compile_options(waternet PRIVATE ${GAME_FLAGS})
    target_link_libraries(waternet PRIVATE SDL2::SDL2main SDL2::SDL2 SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer SDL2_ttf::SDL2_ttf)
    # the game data is copied next to the executable
    add_custom_command(TARGET waternet POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:waternet>/data)
endif()
