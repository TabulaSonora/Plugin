# ---------------------------------------------------------------------------------------------
# The engine.
#
# NativeTS is consumed as a source tree and built into the plugin, position-independent, which is
# what a bundle needs. The submodule first, then a sibling ../NativeTS checkout, so both a fresh
# recursive clone and a working tree that already has the engine next to it configure without an
# argument. LinuxTSPlayer does the same and this follows it.
# ---------------------------------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/external/NativeTS/CMakeLists.txt")
    set(_tsplug_default_nativets "${CMAKE_SOURCE_DIR}/external/NativeTS")
else()
    set(_tsplug_default_nativets "${CMAKE_SOURCE_DIR}/../NativeTS")
endif()

set(TSPLUG_NATIVETS_DIR "${_tsplug_default_nativets}" CACHE PATH "Path to a NativeTS source tree")

if(NOT EXISTS "${TSPLUG_NATIVETS_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "No NativeTS source tree at '${TSPLUG_NATIVETS_DIR}'.\n"
        "Run `git submodule update --init --recursive`, or pass -DTSPLUG_NATIVETS_DIR=/path/to/NativeTS.")
endif()

# NOTICE.md travels with every binary because the engine carries Roland-derived effect coefficients
# compiled in, and the engine's own notice is the authoritative text. A copy that drifts from the
# submodule is worse than no copy, so the two are compared here and a mismatch stops the configure.
file(SHA256 "${CMAKE_SOURCE_DIR}/NOTICE.md" _tsplug_notice_ours)
file(SHA256 "${TSPLUG_NATIVETS_DIR}/NOTICE.md" _tsplug_notice_engine)
if(NOT _tsplug_notice_ours STREQUAL _tsplug_notice_engine)
    message(FATAL_ERROR
        "NOTICE.md differs from ${TSPLUG_NATIVETS_DIR}/NOTICE.md. "
        "Copy the engine's file over ours verbatim; it is the text that must ship.")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/NOTICE.md" "${TSPLUG_NATIVETS_DIR}/NOTICE.md")

# nlohmann_json is the engine's one build dependency (src/CMakeLists.txt does a
# find_package(nlohmann_json CONFIG REQUIRED)). It is header-only and does not reach the plugin's
# interface, so pulling it in here costs a download and nothing else -- which beats standing up a
# package manager for a single header. OVERRIDE_FIND_PACKAGE makes the engine's find_package land
# on this copy. Turn the option off to use a vcpkg or system package instead.
if(TSPLUG_FETCH_NLOHMANN_JSON)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
        OVERRIDE_FIND_PACKAGE)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# The library and nothing else: no Catch2, no CLI11, no install rules leaking into ours.
set(TS_BUILD_TESTS  OFF CACHE BOOL "" FORCE)
set(TS_BUILD_CLI    OFF CACHE BOOL "" FORCE)
set(TS_BUILD_PLAYER OFF CACHE BOOL "" FORCE)
set(TS_BUILD_TUI    OFF CACHE BOOL "" FORCE)
set(TS_BUILD_WEB    OFF CACHE BOOL "" FORCE)
set(TS_BUILD_DOCS   OFF CACHE BOOL "" FORCE)
set(TS_INSTALL      OFF CACHE BOOL "" FORCE)

# The engine links FLAC, Vorbis, Ogg and zlib whenever it can find them, for its SoundFont exporter
# and the XMF reader. A plugin never calls the exporter, and a bundle that dlopens /opt/homebrew/lib
# does not load on any other machine. Both finds are optional upstream, so disabling them is clean;
# the variables are plain (not cache) and are unset again straight after, so JUCE's own pkg-config
# lookups on Linux are untouched.
set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
add_subdirectory("${TSPLUG_NATIVETS_DIR}" "${CMAKE_BINARY_DIR}/nativets" EXCLUDE_FROM_ALL)
unset(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig)
unset(CMAKE_DISABLE_FIND_PACKAGE_ZLIB)

# Never run the engine unoptimised: at -O0 it renders at roughly realtime on one core, and a
# Debug plugin that glitches under a chord is not debuggable. The plugin's own code stays at -O0.
if(TSPLUG_FAST_DEBUG AND NOT MSVC)
    target_compile_options(tabulasonora PRIVATE $<$<CONFIG:Debug>:-O2>)
endif()

# Which engine this binary carries. The voice moves underneath the plugin between two builds of
# the same TSPlug version, so the commit is the first thing worth knowing about a report that a
# sound is wrong. Read from the tree actually compiled, and re-read whenever the submodule moves.
set(TSPLUG_NATIVETS_COMMIT "")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${TSPLUG_NATIVETS_DIR}" rev-parse HEAD
        OUTPUT_VARIABLE _tsplug_nativets_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _tsplug_rev_parse)
    if(_tsplug_rev_parse EQUAL 0)
        set(TSPLUG_NATIVETS_COMMIT "${_tsplug_nativets_commit}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${TSPLUG_NATIVETS_DIR}" rev-parse --absolute-git-dir
            OUTPUT_VARIABLE _tsplug_nativets_git_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(EXISTS "${_tsplug_nativets_git_dir}/HEAD")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${_tsplug_nativets_git_dir}/HEAD")
            file(READ "${_tsplug_nativets_git_dir}/HEAD" _tsplug_head)
            string(STRIP "${_tsplug_head}" _tsplug_head)
            if(_tsplug_head MATCHES "^ref: (.+)$" AND EXISTS "${_tsplug_nativets_git_dir}/${CMAKE_MATCH_1}")
                set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                    "${_tsplug_nativets_git_dir}/${CMAKE_MATCH_1}")
            endif()
        endif()
    endif()
endif()
if(TSPLUG_NATIVETS_COMMIT)
    message(STATUS "NativeTS at ${TSPLUG_NATIVETS_COMMIT}")
else()
    message(STATUS "NativeTS commit unknown")
endif()
