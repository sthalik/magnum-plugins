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

set(CMAKE_FOLDER "MagnumPlugins/JxlImageConverter/Test")

find_package(Magnum REQUIRED DebugTools)

if(CORRADE_TARGET_EMSCRIPTEN)
    set(PNGIMPORTER_TEST_DIR ".")
else()
    set(PNGIMPORTER_TEST_DIR ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/)
endif()

if(NOT MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC)
    set(JXLIMAGECONVERTER_PLUGIN_FILENAME $<TARGET_FILE:JxlImageConverter>)
    if(MAGNUM_WITH_JXLIMPORTER)
        set(JXLIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:JxlImporter>)
    endif()
    if(MAGNUM_WITH_PNGIMPORTER)
        set(PNGIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:PngImporter>)
    endif()
    if(MAGNUM_WITH_STBIMAGEIMPORTER)
        set(STBIMAGEIMPORTER_PLUGIN_FILENAME $<TARGET_FILE:StbImageImporter>)
    endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/configure.h
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/configure.h.in)

corrade_add_test(JxlImageConverterTest JxlImageConverterTest.cpp
    LIBRARIES
        Magnum::DebugTools
        Magnum::Trade
    FILES
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/ga.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/gray.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/gray16.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/rgb.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/rgb16.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/rgb-palette.png
        ${PROJECT_SOURCE_DIR}/src/MagnumPlugins/PngImporter/Test/rgba.png)
target_include_directories(JxlImageConverterTest PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
if(MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC)
    target_link_libraries(JxlImageConverterTest PRIVATE JxlImageConverter)
    if(MAGNUM_WITH_JXLIMPORTER)
        target_link_libraries(JxlImageConverterTest PRIVATE JxlImporter)
    endif()
    if(MAGNUM_WITH_PNGIMPORTER)
        target_link_libraries(JxlImageConverterTest PRIVATE PngImporter)
    endif()
    if(MAGNUM_WITH_STBIMAGEIMPORTER)
        target_link_libraries(JxlImageConverterTest PRIVATE StbImageImporter)
    endif()
else()
    add_dependencies(JxlImageConverterTest JxlImageConverter)
    if(MAGNUM_WITH_JXLIMPORTER)
        add_dependencies(JxlImageConverterTest JxlImporter)
    endif()
    if(MAGNUM_WITH_PNGIMPORTER)
        add_dependencies(JxlImageConverterTest PngImporter)
    endif()
    if(MAGNUM_WITH_STBIMAGEIMPORTER)
        add_dependencies(JxlImageConverterTest StbImageImporter)
    endif()
endif()
if(CORRADE_BUILD_STATIC AND NOT MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC)
    set_target_properties(JxlImageConverterTest PROPERTIES ENABLE_EXPORTS ON)
endif()
