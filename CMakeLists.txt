cmake_minimum_required(VERSION 3.16)
project(modimidi LANGUAGES CXX)

# Needed so find_package(Qt6 ...) honors Qt6_ROOT (see CMakeUserPresets.json).
# Implied by cmake_minimum_required(3.16) already, set explicitly so it
# stays correct even if the minimum version above is ever changed.
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

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
    # Prefer pkg-config: fluidsynth ships a .pc file on virtually every
    # distro/package manager, and this correctly picks up both the
    # library and its include path in one shot.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(FLUIDSYNTH QUIET fluidsynth)
    endif()

    if(FLUIDSYNTH_FOUND)
        set(FLUIDSYNTH_LIBRARY ${FLUIDSYNTH_LIBRARIES})
        set(FLUIDSYNTH_INCLUDE_DIR ${FLUIDSYNTH_INCLUDE_DIRS})
    else()
        # Fallback for systems without pkg-config info (e.g. a fluidsynth
        # built/installed by hand). Set -DFLUIDSYNTH_ROOT=/path/to/prefix
        # or export FLUIDSYNTH_ROOT to point at a non-standard install.
        find_path(FLUIDSYNTH_INCLUDE_DIR
            NAMES fluidsynth.h
            HINTS "$ENV{FLUIDSYNTH_ROOT}/include" "${FLUIDSYNTH_ROOT}/include"
            PATHS /usr/include /usr/local/include)
        find_library(FLUIDSYNTH_LIBRARY
            NAMES fluidsynth
            HINTS "$ENV{FLUIDSYNTH_ROOT}/lib" "${FLUIDSYNTH_ROOT}/lib"
            PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib)
    endif()

    if(NOT FLUIDSYNTH_LIBRARY)
        message(FATAL_ERROR
            "fluidsynth not found. Install libfluidsynth-dev (or your "
            "distro's equivalent), or point at a custom install with "
            "-DFLUIDSYNTH_ROOT=/path/to/prefix.")
    endif()

    # Find the Boost libraries
    find_package(Boost COMPONENTS program_options REQUIRED)

    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${FLUIDSYNTH_INCLUDE_DIR}
    )
endif()

if(BUILD_CLI)
# Set the source files
set(SOURCE_FILES
    main.cpp
    dump.cpp
    dyntiming.cpp
    midi.cpp
    options.cpp
    play.cpp
    player.cpp
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

# ============================================================================
# CMake function to set icon.png dependency for about.cpp and ui.cpp
# This ensures they are rebuilt when icon.png changes
# ============================================================================
function(SetIconDependencies ICON_PNG_PATH)
    set_source_files_properties(
        about.cpp
        ui.cpp
        PROPERTIES
        OBJECT_DEPENDS "${ICON_PNG_PATH}"
    )
endfunction()

if(BUILD_GUI OR ANDROID)
    # find_package(Qt6 ...) honors Qt6_ROOT natively (CMP0074), so we only
    # need to seed a default here. Override per-machine with
    # -DQt6_ROOT=/path/to/Qt/6.x.y/gcc_64, or set it in a
    # CMakeUserPresets.json (see README) instead of editing this file.
    if(BUILD_GUI AND NOT ANDROID
       AND NOT DEFINED Qt6_ROOT AND NOT DEFINED ENV{Qt6_ROOT})
        if(EXISTS "$ENV{HOME}/Qt/6.8.0/gcc_64")
            set(Qt6_ROOT "$ENV{HOME}/Qt/6.8.0/gcc_64" CACHE PATH "Qt6 install root")
        endif()
    endif()

    find_package(Qt6
        REQUIRED COMPONENTS
            Core
            Gui
            Widgets
    )
    # qt_policy(SET QTP0002 NEW) # commented out for local test build
    set(CMAKE_AUTOMOC ON)
    set(UI_SOURCE_FILES
        about.cpp
        buttonedit.cpp
        debug.cpp
        dyntiming.cpp
        gplay.cpp
        inlinepopup.cpp
        mainwin.cpp
        midi.cpp
        mixer.cpp
        player.cpp
        qutil.cpp
        rangeslider.cpp
        rawterm.cpp # how to avoid ?
        speedtune.cpp
        sqrbutton.cpp
        synthseq.cpp
        touchcombobox.cpp
        version.cpp
        about.h
        debug.h
        ui.cpp
        util.cpp
        buttonedit.h
        dyntiming.h
        inlinepopup.h
        mainwin.h
        mixer.h
        rangeslider.h
        speedtune.h
        sqrbutton.h
        touchcombobox.h
        version.h
    )

    # Both gmodimidi and Android (ModiMidi) need the same icon.png.
    set(ICON_PNG "${CMAKE_SOURCE_DIR}/lily.d/icon.png")
    set(ANDROID_ICON_DIR
        "${CMAKE_SOURCE_DIR}/android/res/mipmap-xxxhdpi")
    set(ANDROID_ICON_PNG "${ANDROID_ICON_DIR}/icon.png")

    add_custom_command(
        OUTPUT ${ICON_PNG}
        COMMAND make -C "${CMAKE_SOURCE_DIR}/lily.d" icon.png
        DEPENDS
            "${CMAKE_SOURCE_DIR}/lily.d/icon.ly"
            "${CMAKE_SOURCE_DIR}/lily.d/Makefile"
        COMMENT "lily.d: generating icon.png (lilypond + imagemagick)"
    )

    add_custom_command(
        OUTPUT ${ANDROID_ICON_PNG}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_ICON_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${ICON_PNG} ${ANDROID_ICON_PNG}
        DEPENDS ${ICON_PNG}
        COMMENT "Exporting icon.png to android/res/mipmap-xxxhdpi/"
    )

    add_custom_target(app_icon ALL
        DEPENDS ${ICON_PNG} ${ANDROID_ICON_PNG}
    )
endif()

if(BUILD_GUI)
    # Call the icon dependency function before adding the target
    SetIconDependencies("${ICON_PNG}")

    set(GSOURCE_FILES
        gmain.cpp
        ${UI_SOURCE_FILES}
    )
    add_executable(gmodimidi ${GSOURCE_FILES})
    target_compile_features(gmodimidi PUBLIC cxx_std_20)

    # MOC/UIC/RCC scoped to gmodimidi only — not applied to CLI targets
    set_target_properties(gmodimidi PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )

    # Embed icon.png into the binary as ":/icon.png", so the desktop
    set_source_files_properties("${ICON_PNG}" PROPERTIES
        QT_RESOURCE_ALIAS "icon.png"
    )
    qt_add_resources(gmodimidi "app_icon_qrc"
        PREFIX "/"
        FILES "${ICON_PNG}"
    )
    add_dependencies(gmodimidi app_icon)

    target_link_libraries(gmodimidi
        ${Boost_LIBRARIES}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        ${FLUIDSYNTH_LIBRARY}
    )
endif()

if(ANDROID)
    # Call the icon dependency function before adding the target
    SetIconDependencies("${ICON_PNG}")

    # add_compile_options(-fexperimental-library)

    # Prebuilt fluidsynth-for-Android bundle (libs + headers). Defaults to
    # the in-tree copy but is a normal CACHE PATH, so anyone vendoring it
    # elsewhere (or building it themselves) can point at it without
    # touching this file:
    #   cmake --preset ... -DFLUID_DIR=/path/to/fluidsynth-android
    set(FLUID_DIR
        "${CMAKE_SOURCE_DIR}/third_party/fluidsynth-android"
        CACHE PATH "Prebuilt Android fluidsynth bundle (libs + headers)")

    if(NOT EXISTS "${FLUID_DIR}")
        message(FATAL_ERROR
            "FLUID_DIR (${FLUID_DIR}) does not exist. Fetch/build the "
            "Android fluidsynth bundle, or set -DFLUID_DIR to its location.")
    endif()

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
        set(TGT "lib_${LIB}")

        # find_library applies the platform's lib prefix/suffix itself
        # (libfoo.so under the Android toolchain), so this both avoids
        # assuming the exact filename and fails with a clear, per-library
        # message instead of a linker error much later.
        find_library("${TGT}_PATH"
            NAMES ${LIB}
            PATHS "${FLUID_DIR}/lib/${ANDROID_ABI}"
            NO_DEFAULT_PATH
            # The Android NDK toolchain sets CMAKE_FIND_ROOT_PATH_MODE_LIBRARY
            # to ONLY, which restricts find_library() to the NDK sysroot and
            # silently ignores PATHS outside it. FLUID_DIR lives outside the
            # sysroot, so this must be explicitly opted back in here.
            NO_CMAKE_FIND_ROOT_PATH)

        if(NOT ${TGT}_PATH)
            message(FATAL_ERROR
                "Android dependency '${LIB}' not found under "
                "${FLUID_DIR}/lib/${ANDROID_ABI}. Check FLUID_DIR / "
                "ANDROID_ABI, or that the bundle was built for this ABI.")
        endif()

        add_library(${TGT} SHARED IMPORTED)
        set_target_properties(${TGT} PROPERTIES
            IMPORTED_LOCATION "${${TGT}_PATH}"
        )

        if(LIB STREQUAL "fluidsynth")
            set_target_properties(${TGT} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FLUID_DIR}/include"
            )
        endif()

        list(APPEND FLUID_EXTRA_LIBS_PATHS "${${TGT}_PATH}")
        list(APPEND FLUID_TARGETS ${TGT})
    endforeach()

    set(SF2NAME TimGM6mb.sf2)
    set(SYS_SF2NAME /usr/share/sounds/sf2/${SF2NAME})
    set(GEN_SF2PATH "${CMAKE_CURRENT_BINARY_DIR}/${SF2NAME}")

    add_custom_command(
        OUTPUT ${SF2NAME}
        DEPENDS ${SYS_SF2NAME}
        COMMAND ${CMAKE_COMMAND} -E copy ${SYS_SF2NAME} ${GEN_SF2PATH}
        COMMENT "Copying ${SYS_SF2NAME} to ${GEN_SF2PATH}"
    )

    # --- Executable Definition ---
    set(ASOURCE_FILES
        amain.cpp
        ${UI_SOURCE_FILES}
        ${SF2NAME}
    )
    qt_add_executable(ModiMidi ${ASOURCE_FILES})

    # Packaging (androiddeployqt, run by `ninja apk`) picks up files
    # under android/res as-is, so the exported icon.png must exist
    # before that runs.
    add_dependencies(ModiMidi app_icon)

    # In-app About-page icon, embedded as ":/icon.png" — same as gmodimidi.
    # (Distinct from the Android launcher icon under android/res, which
    # app_icon/ANDROID_ICON_PNG above handles separately.)
    set_source_files_properties("${ICON_PNG}" PROPERTIES
        QT_RESOURCE_ALIAS "icon.png"
    )
    qt_add_resources(ModiMidi "app_icon_qrc"
        PREFIX "/"
        FILES "${ICON_PNG}"
    )

    set_source_files_properties("${GEN_SF2PATH}" PROPERTIES 
        QT_RESOURCE_ALIAS "${SF2NAME}"
    )

    qt_add_resources(ModiMidi "sf2_data"
        PREFIX "/"
        FILES ${GEN_SF2PATH}
    )

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
