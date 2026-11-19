#ifndef Magnum_Trade_JxlImporter_h
#define Magnum_Trade_JxlImporter_h
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
 * @brief Class @ref Magnum::Trade::JxlImporter
 * @m_since_latest_{plugins}
 */

#include <Corrade/Containers/Array.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "MagnumPlugins/JxlImporter/configure.h"

#ifndef DOXYGEN_GENERATING_OUTPUT
#ifndef MAGNUM_JXLIMPORTER_BUILD_STATIC
    #ifdef JxlImporter_EXPORTS
        #define MAGNUM_JXLIMPORTER_EXPORT CORRADE_VISIBILITY_EXPORT
    #else
        #define MAGNUM_JXLIMPORTER_EXPORT CORRADE_VISIBILITY_IMPORT
    #endif
#else
    #define MAGNUM_JXLIMPORTER_EXPORT CORRADE_VISIBILITY_STATIC
#endif
#define MAGNUM_JXLIMPORTER_LOCAL CORRADE_VISIBILITY_LOCAL
#else
#define MAGNUM_JXLIMPORTER_EXPORT
#define MAGNUM_JXLIMPORTER_LOCAL
#endif

namespace Magnum { namespace Trade {

/**
@brief JPEG XL importer plugin
@m_since_latest_{plugins}

Imports [JPEG XL](https://en.wikipedia.org/wiki/JPEG_XL) (`*.jxl`) images
using the [libjxl](https://github.com/libjxl/libjxl) library. You can use
@ref JxlImageConverter to encode images into this format.

@m_class{m-block m-success}

@thirdparty This plugin makes use of the
    [libjxl](https://github.com/libjxl/libjxl) library, released under the
    @m_class{m-label m-success} **BSD 3-clause** license
    ([license text](https://github.com/libjxl/libjxl/blob/main/LICENSE),
    [choosealicense.com](https://choosealicense.com/licenses/bsd-3-clause/)).
    It requires attribution for public use.

@section Trade-JxlImporter-usage Usage

@m_class{m-note m-success}

@par
    This class is a plugin that's meant to be dynamically loaded and used
    through the base @ref AbstractImporter interface. See its documentation for
    introduction and usage examples.

This plugin depends on the @ref Trade and [libjxl](https://github.com/libjxl/libjxl)
libraries and is built if `MAGNUM_WITH_JXLIMPORTER` is enabled when building
Magnum Plugins. To use as a dynamic plugin, load @cpp "JxlImporter" @ce via
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

set(MAGNUM_WITH_JXLIMPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(magnum-plugins EXCLUDE_FROM_ALL)

# So the dynamically loaded plugin gets built implicitly
add_dependencies(your-app MagnumPlugins::JxlImporter)
@endcode

To use as a static plugin or as a dependency of another plugin with CMake, put
[FindMagnumPlugins.cmake](https://github.com/mosra/magnum-plugins/blob/master/modules/FindMagnumPlugins.cmake)
into your `modules/` directory, request the `JxlImporter` component of the
`MagnumPlugins` package and link to the `MagnumPlugins::JxlImporter` target:

@code{.cmake}
find_package(MagnumPlugins REQUIRED JxlImporter)

# ...
target_link_libraries(your-app PRIVATE MagnumPlugins::JxlImporter)
@endcode

See @ref building-plugins, @ref cmake-plugins, @ref plugins and
@ref file-formats for more information.

@section Trade-JxlImporter-behavior Behavior and limitations

Grayscale, grayscale + alpha, RGB and RGBA images with up to 8 bits per sample
are imported as @ref PixelFormat::R8Unorm, @ref PixelFormat::RG8Unorm,
@ref PixelFormat::RGB8Unorm and @ref PixelFormat::RGBA8Unorm, images with 9 to
16 bits per sample as @ref PixelFormat::R16Unorm, @ref PixelFormat::RG16Unorm,
@ref PixelFormat::RGB16Unorm and @ref PixelFormat::RGBA16Unorm. Samples are
scaled to the full range of the type, so for example a 10-bit value of
@cpp 1023 @ce is imported as @cpp 65535 @ce. If the alpha channel has more bits
per sample than the color channels, the alpha channel decides the type.
Images with more than 16 bits per sample, floating-point images and CMYK images
aren't supported.

Premultiplied alpha is converted to unassociated alpha. The orientation stored
in the file is applied, so for a rotated image @ref ImageData::size() is the
size after the rotation. The image is flipped to have the origin at the bottom
left, as Magnum expects.

Images encoded in the XYB color space, which lossy encoding uses by default,
are converted to sRGB. Other images are imported in the color space they were
encoded in, without applying an embedded ICC profile. When libjxl rounds an
unpremultiplied or color-converted sample to 8 bits, it applies ordered
dithering, so pixels of the same color in the file may differ by one.

@subsection Trade-JxlImporter-behavior-animated Animated images

In case the file is animated, the importer reports the frame count in
@ref image2DCount() and you can then import each frame separately. Every frame
has the size of the whole image. Additionally, for the lack of better APIs at
the moment, frame delays are exposed through @ref importerState() as an array
of @ref Magnum::Int "Int", where each entry is the number of milliseconds to
wait before advancing to the next frame. For a still image,
@ref importerState() is @cpp nullptr @ce.

@section Trade-JxlImporter-configuration Plugin-specific configuration

It's possible to tune various options through @ref configuration(). See below
for all options and their default values:

@snippet MagnumPlugins/JxlImporter/JxlImporter.conf configuration_

See @ref plugins-configuration for more information and an example showing how
to edit the configuration values.
*/
class MAGNUM_JXLIMPORTER_EXPORT JxlImporter: public AbstractImporter {
    public:
        /** @brief Plugin manager constructor */
        explicit JxlImporter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin);

        ~JxlImporter();

    private:
        MAGNUM_JXLIMPORTER_LOCAL ImporterFeatures doFeatures() const override;
        MAGNUM_JXLIMPORTER_LOCAL bool doIsOpened() const override;
        MAGNUM_JXLIMPORTER_LOCAL void doClose() override;
        MAGNUM_JXLIMPORTER_LOCAL void doOpenData(Containers::Array<char>&& data, DataFlags dataFlags) override;

        MAGNUM_JXLIMPORTER_LOCAL UnsignedInt doImage2DCount() const override;
        MAGNUM_JXLIMPORTER_LOCAL Containers::Optional<ImageData2D> doImage2D(UnsignedInt id, UnsignedInt level) override;

        MAGNUM_JXLIMPORTER_LOCAL const void* doImporterState() const override;

        Containers::Array<char> _in;
        Containers::Array<Int> _frameDelays;
};

}}

#endif
