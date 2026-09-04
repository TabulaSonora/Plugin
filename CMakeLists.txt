cmake_minimum_required(VERSION 3.24)

project(TSPlug
    VERSION 0.1.0
    DESCRIPTION "Tabula Sonora as a plugin: the Roland Sound Canvas VA voice, NativeTS engine"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TSPLUG_COPY_AFTER_BUILD "Install the plugins to the user's plug-in folders after building" ${PROJECT_IS_TOP_LEVEL})
option(TSPLUG_BUILD_CLAP "Build the CLAP through clap-juce-extensions" ON)
option(TSPLUG_BUILD_LV2 "Build the LV2" ON)
option(TSPLUG_BUILD_TESTS "Build the engine-glue test" ${PROJECT_IS_TOP_LEVEL})
option(TSPLUG_FETCH_NLOHMANN_JSON "Fetch nlohmann_json for the engine instead of find_package" ON)
option(TSPLUG_FAST_DEBUG "Compile the engine at -O2 in Debug builds" ON)

# The engine first: it is where the numeric-semantics flags come from, and nothing here is worth
# configuring without it.
include(cmake/TSPlugNativeTS.cmake)

# JUCE, from the submodule or a sibling checkout, same rule as the engine.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    set(_tsplug_default_juce "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
else()
    set(_tsplug_default_juce "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
endif()
set(TSPLUG_JUCE_DIR "${_tsplug_default_juce}" CACHE PATH "Path to a JUCE source tree")
if(NOT EXISTS "${TSPLUG_JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "No JUCE source tree at '${TSPLUG_JUCE_DIR}'.\n"
        "Run `git submodule update --init --recursive`, or pass -DTSPLUG_JUCE_DIR=/path/to/JUCE.")
endif()
# Out of tree, so the binary directory has to be named.
add_subdirectory("${TSPLUG_JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------------------------
# The plugin.
#
# Identity is permanent once a session has been saved with it. `LSCo` is LoSnoCo's manufacturer
# code, shared with the Apple player's AUv3; `Tsva` is this plugin's own code, deliberately not the
# AUv3's `tbsn`, so the two coexist in a host. JUCE would otherwise draw a random code on every
# configure.
# ---------------------------------------------------------------------------------------------
set(_tsplug_formats Standalone VST3 AU)
if(TSPLUG_BUILD_LV2)
    list(APPEND _tsplug_formats LV2)
endif()

juce_add_plugin(TSPlug
    VERSION "${PROJECT_VERSION}"
    COMPANY_NAME "LoSnoCo"
    COMPANY_WEBSITE "https://losno.co"
    COMPANY_COPYRIGHT "Copyright (c) 2026 Kevin López Brante"
    BUNDLE_ID "co.losno.TabulaSonoraPlugin"
    PLUGIN_MANUFACTURER_CODE LSCo
    PLUGIN_CODE Tsva
    PRODUCT_NAME "Tabula Sonora"
    PLUGIN_NAME "Tabula Sonora"
    DESCRIPTION "Roland Sound Canvas VA voice, NativeTS engine"
    FORMATS ${_tsplug_formats}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth
    LV2URI "https://losno.co/plugins/tabula-sonora"
    LV2_PLUGIN_CLASS InstrumentPlugin
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
    COPY_PLUGIN_AFTER_BUILD ${TSPLUG_COPY_AFTER_BUILD})

juce_add_binary_data(TSPlugData
    NAMESPACE TSPlugData
    HEADER_NAME TSPlugData.h
    SOURCES Resources/fonts/InterVariableLoSnoCo.ttf)

target_sources(TSPlug PRIVATE
    Source/engine/Session.cpp
    Source/engine/Instrument.cpp
    Source/plugin/Parameters.cpp
    Source/plugin/RomLocator.cpp
    Source/plugin/RomLoader.cpp
    Source/plugin/PluginProcessor.cpp)

target_include_directories(TSPlug PRIVATE Source)

target_compile_definitions(TSPlug PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    TSPLUG_NATIVETS_COMMIT="${TSPLUG_NATIVETS_COMMIT}")

target_link_libraries(TSPlug
    PRIVATE
        ts::tabulasonora
        TSPlugData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# CLAP is not a JUCE format; the wrapper adds a `TSPlug_CLAP` target over the shared code.
if(TSPLUG_BUILD_CLAP)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/clap-juce-extensions/CMakeLists.txt")
        message(FATAL_ERROR
            "clap-juce-extensions is missing. Run `git submodule update --init --recursive`, "
            "or configure with -DTSPLUG_BUILD_CLAP=OFF.")
    endif()
    add_subdirectory(external/clap-juce-extensions EXCLUDE_FROM_ALL)
    clap_juce_extensions_plugin(TARGET TSPlug
        CLAP_ID "co.losno.tabula-sonora"
        CLAP_FEATURES instrument synthesizer)
endif()

include(cmake/TSPlugBundle.cmake)
foreach(_wrapper TSPlug_Standalone TSPlug_VST3 TSPlug_AU TSPlug_LV2 TSPlug_CLAP)
    tsplug_add_legal_resources(${_wrapper})
endforeach()

if(TSPLUG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
