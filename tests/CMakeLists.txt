include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

set(WEBVIEW_TEST_SOURCES
    PresetAttachmentTests.cpp
)

add_executable(imagiro_webview_tests ${WEBVIEW_TEST_SOURCES})

target_link_libraries(imagiro_webview_tests PRIVATE
    imagiro_webview
    imagiro_processor
    imagiro_util
    juce::juce_audio_utils
    juce::juce_events
    Catch2::Catch2WithMain
)

target_include_directories(imagiro_webview_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Inherit compile definitions from parent project
if(TARGET ${ProjectName})
    target_compile_definitions(imagiro_webview_tests PRIVATE
        $<TARGET_PROPERTY:${ProjectName},COMPILE_DEFINITIONS>
    )
endif()

catch_discover_tests(imagiro_webview_tests)
