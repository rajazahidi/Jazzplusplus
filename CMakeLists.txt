cmake_minimum_required(VERSION 3.16)
project(jazzplusplus VERSION 6.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations)
elseif(MSVC)
    add_compile_options(/W3 /wd4996)
endif()

# -----------------------------------------------------------------------------
# Unit Tests
# -----------------------------------------------------------------------------
enable_testing()
add_executable(jazz_tests
    tests/test_main.cpp
    src/ChordScaleData.cpp
    src/AudioEffects.cpp
    src/SoundGenerator.cpp
    src/MidiEffects.cpp
)
target_compile_definitions(jazz_tests PRIVATE JAZZ_UNIT_TEST)
target_include_directories(jazz_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_test(NAME JazzCoreUnitTests COMMAND jazz_tests)
set_tests_properties(JazzCoreUnitTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# JUCE 8 Digital Audio Workstation (jazz)
# -----------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    add_subdirectory(libs/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_gui_app(jazz
    PRODUCT_NAME "Jazz++"
    VERSION 6.0.0
)

target_sources(jazz PRIVATE
    src/juce/JazzJuceApp.cpp
    src/juce/MainComponent.cpp
    src/juce/MainComponent.h
    src/juce/JazzLookAndFeel.h
    src/juce/AudioMidiEngine.cpp
    src/juce/AudioMidiEngine.h
    src/juce/ClapPluginFormat.cpp
    src/juce/ClapPluginFormat.h
    src/juce/PluginHostComponent.cpp
    src/juce/PluginHostComponent.h
    src/juce/ArrangerComponent.cpp
    src/juce/ArrangerComponent.h
    src/juce/PianoRollComponent.cpp
    src/juce/PianoRollComponent.h
    src/juce/DrumMachineComponent.cpp
    src/juce/DrumMachineComponent.h
    src/juce/FretboardComponent.cpp
    src/juce/FretboardComponent.h
    src/juce/AudioEffectsRackComponent.cpp
    src/juce/AudioEffectsRackComponent.h
    src/AudioEffects.cpp
    src/SoundGenerator.cpp
    src/MidiEffects.cpp
    src/ChordScaleData.cpp
)

target_compile_definitions(jazz PRIVATE
    JAZZ_UNIT_TEST=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
)

target_include_directories(jazz PRIVATE
    src
    src/juce
    libs/clap/include
)

target_compile_features(jazz PRIVATE cxx_std_20)

target_link_libraries(jazz PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Convenience symlink for jazz-juce backwards compatibility and build/jazz
if(UNIX AND NOT APPLE)
    add_custom_command(TARGET jazz POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE:jazz>"
            "${CMAKE_BINARY_DIR}/jazz"
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE:jazz>"
            "${CMAKE_BINARY_DIR}/jazz-juce"
        COMMENT "Creating convenience symlinks build/jazz and build/jazz-juce"
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS jazz
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION Applications
)
install(PROGRAMS scripts/jazz-launcher.sh RENAME jazz-launcher DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES jazz.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/jazz.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES resources/icons/jazz.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps)

foreach(size 16 32 48 64 128 256)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/jazz-${size}.png")
        install(FILES resources/icons/jazz-${size}.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
                RENAME jazz.png)
    endif()
endforeach()

install(DIRECTORY conf/ DESTINATION ${CMAKE_INSTALL_DATADIR}/jazz/conf)
install(DIRECTORY song/ DESTINATION ${CMAKE_INSTALL_DATADIR}/jazz/song)
install(DIRECTORY src/HelpFiles/ DESTINATION ${CMAKE_INSTALL_DATADIR}/jazz/HelpFiles)
