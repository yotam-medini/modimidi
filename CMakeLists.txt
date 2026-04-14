cmake_minimum_required(VERSION 3.10)
project(modimidi LANGUAGES CXX)

# For LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_CXX_STANDARD 20)
set(GENERATE_VERSION_SCRIPT "${CMAKE_SOURCE_DIR}/tools/genver.py")
set(VERSION_CPP "${CMAKE_SOURCE_DIR}/version.cpp")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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


option(BUILD_CLI "Build modimidi command line" ON)
option(BUILD_GUI "Build the Qt GUI and Android targets" OFF)
# option(ANDROID "Build Android target" OFF)

if(BUILD_CLI OR BUILD_GUI)
    # Find the fluidsynth library (adjust paths as needed)
    find_library(FLUIDSYNTH_LIBRARY
        NAMES fluidsynth PATHS /usr/lib /usr/lib/x86_64-linux-gnu)
    if(NOT FLUIDSYNTH_LIBRARY)
        message(FATAL_ERROR
            "fluidsynth library not found. Check the installation and paths.")
    endif()

    # Find the Boost libraries
    find_package(Boost COMPONENTS program_options REQUIRED)

    include_directories(
        ${Boost_INCLUDE_DIRS}
    )
endif()

# /usr/include/fluidsynth
include_directories(
    /usr/include
)

if(BUILD_CLI)
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
endif()

if(BUILD_GUI OR ANDROID)
    find_package(Qt6
        REQUIRED COMPONENTS
            Core
            Gui
            Widgets
    )
    qt_policy(SET QTP0002 NEW)
    set(CMAKE_AUTOMOC ON)
    set(UI_SOURCE_FILES
        debug.cpp
        gplay.cpp
        midi.cpp
        qutil.cpp
        synthseq.cpp
        ui.cpp
        mainwin.h
    )
endif()

if(BUILD_GUI)
    set(GSOURCE_FILES
        gmain.cpp
        ${UI_SOURCE_FILES}
    )
    add_executable(gmodimidi ${GSOURCE_FILES})
    target_compile_features(gmodimidi PUBLIC cxx_std_20)
    target_link_libraries(gmodimidi
        ${Boost_LIBRARIES}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        ${FLUIDSYNTH_LIBRARY}
    )
endif()

if(ANDROID)
    # add_compile_options(-fexperimental-library)
    set(FLUID_DIR "${CMAKE_SOURCE_DIR}/third_party/fluidsynth-android")

    set(FLUID_DEPS
        "fluidsynth"
        "glib-2.0"
        "pcre"
        "gthread-2.0"
        "sndfile"
        "ogg"
        "vorbisenc"
        "vorbis"
        "FLAC"
        "opus"
        "oboe"
    )

    set(FLUID_EXTRA_LIBS_PATHS "")
    set(FLUID_TARGETS "")

    foreach(LIB ${FLUID_DEPS})
        set(LIB_PATH "${FLUID_DIR}/lib/${ANDROID_ABI}/lib${LIB}.so")
        set(TGT "lib_${LIB}")

        add_library(${TGT} SHARED IMPORTED)
        set_target_properties(${TGT} PROPERTIES
            IMPORTED_LOCATION "${LIB_PATH}"
        )

        if(LIB STREQUAL "fluidsynth")
            set_target_properties(${TGT} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FLUID_DIR}/include"
            )
        endif()

        list(APPEND FLUID_EXTRA_LIBS_PATHS "${LIB_PATH}")
        list(APPEND FLUID_TARGETS ${TGT})
    endforeach()

    set(SF2NAME TimGM6mb.sf2)
    set(SYS_SF2NAME /usr/share/sounds/sf2/${SF2NAME})

    add_custom_command(
        OUTPUT ${SF2NAME}
        DEPENDS ${SYS_SF2NAME}
        COMMAND ${CMAKE_COMMAND} -E copy ${SYS_SF2NAME} ${SF2NAME}
        COMMENT "Copying ${SYS_SF2NAME} to ${SF2NAME}"
    )

    # --- Executable Definition ---
    set(ASOURCE_FILES
        amain.cpp
        ${UI_SOURCE_FILES}
        resources.qrc # CMake runs 'rcc' (Qt Resource Compiler)
        ${SF2NAME}
    )
    qt_add_executable(ModiMidi ${ASOURCE_FILES})

    set_target_properties(ModiMidi PROPERTIES
        QT_ANDROID_PACKAGE_NAME "yotam.modimidi"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        OUTPUT_NAME "ModiMidi_arm64-v8a"
        # This property ensures they are physically inside the APK
        QT_ANDROID_EXTRA_LIBS "${FLUID_EXTRA_LIBS_PATHS}"
    )

    # --- Linking ---
    # Using the list ensures the order: fluidsynth -> glib -> pcre
    target_link_libraries(ModiMidi
        PRIVATE
            ${FLUID_TARGETS}
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
    )
endif()
