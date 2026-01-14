cmake_minimum_required(VERSION 3.18)
project(crashpad_plus LANGUAGES CXX)

add_library(crashpad_plus OBJECT
    crashpad_plus.h
	sentry/sentry_event_utils.h
)

if (WIN32)
    target_sources(crashpad_plus PRIVATE
        crashpad_plus_win.cc
		sentry/sentry_event_utils_win.cc
    )
endif()

if(APPLE AND NOT IOS)
    target_sources(crashpad_plus PRIVATE
        crashpad_plus_mac.cc
		sentry/sentry_event_utils_mac.cc
    )
endif()

target_compile_definitions(crashpad_plus PRIVATE
    MPACK_MALLOC=malloc
    MPACK_FREE=free
)

if(WIN32)
    target_compile_definitions(crashpad_plus PRIVATE NOMINMAX)
endif()


target_include_directories(crashpad_plus PUBLIC
	${CMAKE_SOURCE_DIR}/external/crashpad
	${CMAKE_SOURCE_DIR}/external/crashpad/third_party/mini_chromium/mini_chromium
	${CMAKE_SOURCE_DIR}/vendor
)

if(WIN32)
    target_include_directories(crashpad_plus PRIVATE
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/non_mac
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/win
    )
else()
    target_include_directories(crashpad_plus PRIVATE
        ${CMAKE_SOURCE_DIR}/external/crashpad/compat/mac
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/non_win
    )
endif()

set_target_properties(crashpad_plus PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
