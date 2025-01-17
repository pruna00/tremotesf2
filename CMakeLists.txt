# SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
#
# SPDX-License-Identifier: CC0-1.0

if (WIN32)
    cmake_minimum_required(VERSION 3.21)
else()
    cmake_minimum_required(VERSION 3.16)
endif()
cmake_policy(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION}..3.26)

project(tremotesf VERSION 2.4.0.1 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TREMOTESF_QT6 OFF CACHE BOOL "Build with Qt 6" FORCE)
include(src/libtremotesf/cmake/CommonOptions.cmake)

find_package(Qt${TREMOTESF_QT_VERSION_MAJOR} ${TREMOTESF_MINIMUM_QT_VERSION} REQUIRED COMPONENTS Core)

set(QRC_FILES "")
add_subdirectory("data")
add_subdirectory("translations")
add_subdirectory("src")
if (WIN32)
    add_subdirectory("windows-packaging")
endif()
