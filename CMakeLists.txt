cmake_minimum_required (VERSION 3.20)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.1" CACHE STRING "Minimum OS X deployment version")

project(VNote
    VERSION 3.20.1
    DESCRIPTION "A pleasant note-taking platform"
    HOMEPAGE_URL "https://app.vnote.fun"
    LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, defaults to Release")

# Sources are UTF-8 encoded but saved without a BOM, and some contain non-ASCII (Chinese) comments.
# MSVC would otherwise decode them using the system code page (e.g. 936/GBK), which mis-decodes such
# comments and can swallow the following line of code, resulting in bogus syntax/member errors.
if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(QHOTKEY_INSTALL OFF CACHE BOOL "Disable installing QHotKey" FORCE)
add_subdirectory(libs)

add_subdirectory(src)

# TODO: find a better way to organize tests
# add_subdirectory(tests)
