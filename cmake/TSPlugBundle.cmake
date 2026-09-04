# ---------------------------------------------------------------------------------------------
# LICENSE and NOTICE.md in every deliverable.
#
# juce_add_plugin builds one wrapper target per format, and resources attached to the shared-code
# static library do not reach them, so this is applied per wrapper. Apple bundles carry the files in
# Contents/Resources; anything that is a bare binary or a folder (the LV2, and every format on other
# platforms) gets them copied beside it after the build.
# ---------------------------------------------------------------------------------------------
function(tsplug_add_legal_resources target)
    if(NOT TARGET ${target})
        return()
    endif()

    set(_files "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/NOTICE.md")

    get_target_property(_is_bundle ${target} BUNDLE)
    get_target_property(_is_app ${target} MACOSX_BUNDLE)
    if(APPLE AND (_is_bundle OR _is_app))
        target_sources(${target} PRIVATE ${_files})
        set_source_files_properties(${_files} PROPERTIES
            HEADER_FILE_ONLY TRUE
            MACOSX_PACKAGE_LOCATION Resources)
    else()
        # Before the link rather than after it: JUCE copies the finished folder to the user's
        # plug-in directory in its own POST_BUILD step, which was registered first and so runs
        # first, and a file placed afterwards never reaches the copy.
        add_custom_command(TARGET ${target} PRE_LINK
            COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_files} "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endif()
endfunction()
