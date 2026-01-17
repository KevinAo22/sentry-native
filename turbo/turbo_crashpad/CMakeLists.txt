cmake_minimum_required(VERSION 3.18)
project(turbo_crashpad LANGUAGES CXX)

add_library(turbo_crashpad OBJECT
    turbo_crashpad.h
	sentry/sentry_event_utils.h
)

if (WIN32)
    target_sources(turbo_crashpad PRIVATE
        turbo_crashpad_win.cc
		sentry/sentry_event_utils_win.cc
    )
endif()

if(APPLE AND NOT IOS)
    target_sources(turbo_crashpad PRIVATE
        turbo_crashpad_mac.cc
		sentry/sentry_event_utils_mac.cc
    )
endif()

target_compile_definitions(turbo_crashpad PRIVATE
    MPACK_MALLOC=malloc
    MPACK_FREE=free
)

if(WIN32)
    target_compile_definitions(turbo_crashpad PRIVATE NOMINMAX)
endif()


target_include_directories(turbo_crashpad PUBLIC
	${CMAKE_SOURCE_DIR}/external/crashpad
	${CMAKE_SOURCE_DIR}/external/crashpad/third_party/mini_chromium/mini_chromium
	${CMAKE_SOURCE_DIR}/vendor
)

if(WIN32)
    target_include_directories(turbo_crashpad PRIVATE
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/non_mac
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/win
    )
else()
    target_include_directories(turbo_crashpad PRIVATE
        ${CMAKE_SOURCE_DIR}/external/crashpad/compat/mac
		${CMAKE_SOURCE_DIR}/external/crashpad/compat/non_win
    )
endif()

set_target_properties(turbo_crashpad PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
