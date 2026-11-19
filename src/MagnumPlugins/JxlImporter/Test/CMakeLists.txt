#
#   This file is part of Magnum.
#
#   Copyright © 2026 Stanisław Halik <sthalik@misaki.pl>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

set(CMAKE_FOLDER "MagnumPlugins/JxlImporter/Test")

find_package(Magnum REQUIRED DebugTools)

if(CORRADE_TARGET_EMSCRIPTEN)
    set(JXLIMPORTER_TEST_DIR ".")
else()
    set(JXLIMPORTER_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(NOT MAGNUM_JXLIMPORTER_BUILD_STATIC)
    set(JXLIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:JxlImporter>)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/configure.h
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)

corrade_add_test(JxlImporterTest JxlImporterTest.cpp
    LIBRARIES
        Magnum::DebugTools
        Magnum::Trade
    FILES
        animated.jxl
        cmyk.jxl
        float.jxl
        ga.jxl
        ga-alpha16.jxl
        gray.jxl
        gray16.jxl
        premultiplied.jxl
        rgb.jxl
        rgb10.jxl
        rgb16.jxl
        rgba.jxl
        rotated.jxl
        xyb-icc.jxl)
target_include_directories(JxlImporterTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
if(MAGNUM_JXLIMPORTER_BUILD_STATIC)
    target_link_libraries(JxlImporterTest PRIVATE JxlImporter)
else()
    add_dependencies(JxlImporterTest JxlImporter)
endif()
if(CORRADE_BUILD_STATIC AND NOT MAGNUM_JXLIMPORTER_BUILD_STATIC)
    set_target_properties(JxlImporterTest PROPERTIES ENABLE_EXPORTS ON)
endif()
