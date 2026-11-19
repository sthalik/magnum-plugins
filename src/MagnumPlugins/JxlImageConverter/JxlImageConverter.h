#ifndef Magnum_Trade_JxlImageConverter_h
#define Magnum_Trade_JxlImageConverter_h
/*
    This file is part of Magnum.

    Copyright © 2026 Stanisław Halik <sthalik@misaki.pl>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

/** @file
 * @brief Class @ref Magnum::Trade::JxlImageConverter
 * @m_since_latest_{plugins}
 */

#include <Magnum/Trade/AbstractImageConverter.h>

#include "MagnumPlugins/JxlImageConverter/configure.h"

#ifndef DOXYGEN_GENERATING_OUTPUT
#ifndef MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC
    #ifdef JxlImageConverter_EXPORTS
        #define MAGNUM_JXLIMAGECONVERTER_EXPORT CORRADE_VISIBILITY_EXPORT
    #else
        #define MAGNUM_JXLIMAGECONVERTER_EXPORT CORRADE_VISIBILITY_IMPORT
    #endif
#else
    #define MAGNUM_JXLIMAGECONVERTER_EXPORT CORRADE_VISIBILITY_STATIC
#endif
#define MAGNUM_JXLIMAGECONVERTER_LOCAL CORRADE_VISIBILITY_LOCAL
#else
#define MAGNUM_JXLIMAGECONVERTER_EXPORT
#define MAGNUM_JXLIMAGECONVERTER_LOCAL
#endif

namespace Magnum { namespace Trade {

/**
@brief JPEG XL image converter plugin
@m_since_latest_{plugins}

Creates lossless [JPEG XL](https://en.wikipedia.org/wiki/JPEG_XL) (`*.jxl`)
images using the [libjxl](https://github.com/libjxl/libjxl) library. You can
use @ref JxlImporter to import images in this format.

@m_class{m-block m-success}

@thirdparty This plugin makes use of the
    [libjxl](https://github.com/libjxl/libjxl) library, released under the
    @m_class{m-label m-success} **BSD 3-clause** license
    ([license text](https://github.com/libjxl/libjxl/blob/main/LICENSE),
    [choosealicense.com](https://choosealicense.com/licenses/bsd-3-clause/)).
    It requires attribution for public use.

@section Trade-JxlImageConverter-usage Usage

@m_class{m-note m-success}

@par
    This class is a plugin that's meant to be dynamically loaded and used
    via the base @ref AbstractImageConverter interface. See its documentation
    for introduction and usage examples.

This plugin depends on the @ref Trade and [libjxl](https://github.com/libjxl/libjxl)
libraries, version 0.8 or newer, and is built if
`MAGNUM_WITH_JXLIMAGECONVERTER` is enabled when building Magnum Plugins. To use
as a dynamic plugin, load @cpp "JxlImageConverter" @ce via
@relativeref{Corrade,PluginManager::Manager}.

Additionally, if you're using Magnum as a CMake subproject, bundle the
[magnum-plugins](https://github.com/mosra/magnum-plugins) and
[libjxl](https://github.com/libjxl/libjxl) repositories and do the following.
If you want to use system-installed libjxl, omit the first part and point
`CMAKE_PREFIX_PATH` to its installation dir if necessary.

@code{.cmake}
# Disable unneeded functionality
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_DOXYGEN OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_MANPAGES OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_BENCHMARK OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JPEGXL_ENABLE_JNI OFF CACHE BOOL "" FORCE)
add_subdirectory(libjxl EXCLUDE_FROM_ALL)

set(MAGNUM_WITH_JXLIMAGECONVERTER ON CACHE BOOL "" FORCE)
add_subdirectory(magnum-plugins EXCLUDE_FROM_ALL)

# So the dynamically loaded plugin gets built implicitly
add_dependencies(your-app MagnumPlugins::JxlImageConverter)
@endcode

To use as a static plugin or as a dependency of another plugin with CMake, put
[FindMagnumPlugins.cmake](https://github.com/mosra/magnum-plugins/blob/master/modules/FindMagnumPlugins.cmake)
into your `modules/` directory, request the `JxlImageConverter` component of
the `MagnumPlugins` package and link to the
`MagnumPlugins::JxlImageConverter` target:

@code{.cmake}
find_package(MagnumPlugins REQUIRED JxlImageConverter)

# ...
target_link_libraries(your-app PRIVATE MagnumPlugins::JxlImageConverter)
@endcode

See @ref building-plugins, @ref cmake-plugins, @ref plugins and
@ref file-formats for more information.

@section Trade-JxlImageConverter-behavior Behavior and limitations

Accepts 2D images in @ref PixelFormat::R8Unorm, @ref PixelFormat::RG8Unorm,
@ref PixelFormat::RGB8Unorm, @ref PixelFormat::RGBA8Unorm,
@ref PixelFormat::R16Unorm, @ref PixelFormat::RG16Unorm,
@ref PixelFormat::RGB16Unorm and @ref PixelFormat::RGBA16Unorm. One- and
two-channel formats are saved as grayscale, two- and four-channel formats with
an alpha channel. The output is always lossless and marked as sRGB.

The JPEG XL file format doesn't have a way to distinguish between 2D and 1D
array images. If an image has @ref ImageFlag2D::Array set, a warning is printed
and the file is saved as a regular 2D image.

The plugin recognizes @ref ImageConverterFlag::Quiet, which will cause all
conversion warnings to be suppressed.

@section Trade-JxlImageConverter-configuration Plugin-specific configuration

It's possible to tune various options through @ref configuration(). See below
for all options and their default values:

@snippet MagnumPlugins/JxlImageConverter/JxlImageConverter.conf configuration_

See @ref plugins-configuration for more information and an example showing how
to edit the configuration values.
*/
class MAGNUM_JXLIMAGECONVERTER_EXPORT JxlImageConverter: public AbstractImageConverter {
    public:
        /** @brief Plugin manager constructor */
        explicit JxlImageConverter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin);

    private:
        MAGNUM_JXLIMAGECONVERTER_LOCAL ImageConverterFeatures doFeatures() const override;
        MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::String doExtension() const override;
        MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::String doMimeType() const override;
        MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::Optional<Containers::Array<char>> doConvertToData(const ImageView2D& image) override;
};

}}

#endif
