cmake_minimum_required(VERSION 3.10)
project(modimidi LANGUAGES CXX)

# For LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_CXX_STANDARD 20)
set(GENERATE_VERSION_SCRIPT "${CMAKE_SOURCE_DIR}/tools/genver.py")
set(VERSION_CPP "${CMAKE_SOURCE_DIR}/version.cpp")

# Add a custom command to generate VERSION.h
add_custom_command(
    OUTPUT ${VERSION_CPP}
    COMMAND ${GENERATE_VERSION_SCRIPT} README.md version.cpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${GENERATE_VERSION_SCRIPT} README.md ${CMAKE_LISTS_FILE}
    COMMENT "Generating version.cpp"
)

# Add a custom target for the generated file
add_custom_target(generate_version ALL
  DEPENDS ${VERSION_CPP} version.h tools/genver.py CMakeLists.txt)

# Find the Boost libraries
find_package(Boost COMPONENTS program_options REQUIRED)

# Find the fluidsynth library (adjust paths as needed)
find_library(FLUIDSYNTH_LIBRARY NAMES fluidsynth PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
if(NOT FLUIDSYNTH_LIBRARY)
    message(FATAL_ERROR "Could not find the fluidsynth library. Please check the installation and paths.")
endif()

# Set the include directories
# /usr/include/fluidsynth
include_directories(
    ${Boost_INCLUDE_DIRS}
    /usr/include
)

# Set the source files
set(SOURCE_FILES
    main.cpp
    dump.cpp
    midi.cpp
    options.cpp
    play.cpp
    rawterm.cpp
    synthseq.cpp
    util.cpp
    version.cpp
)

# Add the executable target
add_executable(modimidi ${SOURCE_FILES})

# C++20 or higher
target_compile_features(modimidi PUBLIC cxx_std_20)

# Make the executable depend on the generated header
add_dependencies(modimidi generate_version)

# Link with the required libraries
target_link_libraries(modimidi
    ${FLUIDSYNTH_LIBRARY}
    ${Boost_LIBRARIES}
)

# modidump2ly
add_executable(modidump2ly
    tools/modidump2ly.cpp
)

# C++20 or higher
target_compile_features(modidump2ly PUBLIC cxx_std_20)


target_link_libraries(modidump2ly
    ${Boost_LIBRARIES}
)
