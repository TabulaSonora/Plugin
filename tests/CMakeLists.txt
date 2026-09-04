# Links the engine and Source/engine with no JUCE at all. That is the point: it proves the CMake
# glue (submodule or sibling, nlohmann_json, the forced options) and that the engine layer has
# stayed JUCE-free, before a single plugin format is involved.
add_executable(tsplug_engine_glue
    engine_glue_test.cpp
    "${CMAKE_SOURCE_DIR}/Source/engine/Session.cpp"
    "${CMAKE_SOURCE_DIR}/Source/engine/Instrument.cpp")
target_include_directories(tsplug_engine_glue PRIVATE "${CMAKE_SOURCE_DIR}/Source")
target_link_libraries(tsplug_engine_glue PRIVATE ts::tabulasonora)
target_compile_features(tsplug_engine_glue PRIVATE cxx_std_20)

add_test(NAME engine_glue COMMAND tsplug_engine_glue)
