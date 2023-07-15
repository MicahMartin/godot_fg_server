# SPDX-License-Identifier: Unlicense

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

target_sources( ${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "src"
)
