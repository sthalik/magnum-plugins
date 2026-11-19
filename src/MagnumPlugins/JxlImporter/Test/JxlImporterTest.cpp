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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "configure.h"

namespace Magnum { namespace Trade { namespace Test { namespace {

struct JxlImporterTest: TestSuite::Tester {
    explicit JxlImporterTest();

    void empty();
    void invalid();
    void truncated();
    void floatingPoint();
    void cmyk();
    void importFile();
    void animated();
    void xybIccProfile();
    void threads();

    PluginManager::Manager<AbstractImporter> _manager{"nonexistent"};
};

const UnsignedByte GrayPixels[]{
    0xff, 0x88, 0x00,
    0x88, 0x00, 0xff
};
const UnsignedShort Gray16Pixels[]{
    1000, 2000,
    3000, 4000,
    5000, 6000
};
const UnsignedByte GrayAlphaPixels[]{
    0x66, 0x99, 0xcc, 0x00, 0x99, 0x66,
    0x00, 0x33, 0x33, 0xff, 0xff, 0xcc
};
const UnsignedShort GrayAlpha16Pixels[]{
    0x0000, 0x1234, 0x8080, 0xabcd, 0xffff, 0xffff
};
const UnsignedByte RgbPixels[]{
    0xca, 0xfe, 0x77, 0xde, 0xad, 0xb5, 0xca, 0xfe, 0x77,
    0xde, 0xad, 0xb5, 0xca, 0xfe, 0x77, 0xde, 0xad, 0xb5
};
const UnsignedShort Rgb10Pixels[]{
    6406, 32031, 65535, 0, 16400, 49199
};
const UnsignedShort Rgb16Pixels[]{
    1000, 2000, 3000, 2000, 3000, 4000,
    3000, 4000, 5000, 4000, 5000, 6000,
    5000, 6000, 7000, 6000, 7000, 8000
};
const UnsignedByte RotatedPixels[]{
    60, 61, 62, 30, 31, 32,
    50, 51, 52, 20, 21, 22,
    40, 41, 42, 10, 11, 12
};
const UnsignedByte RgbaPixels[]{
    0x66, 0x33, 0xff, 0x99, 0xcc, 0x33, 0xff, 0x00, 0x99, 0x33, 0xff, 0x66,
    0x00, 0xcc, 0xff, 0x33, 0x33, 0x66, 0x99, 0xff, 0xff, 0x00, 0x33, 0xcc
};
const UnsignedByte UnpremultipliedRgbaPixels[]{
    50, 100, 200, 51, 50, 100, 200, 51
};
const UnsignedByte AnimatedPixels[][6]{
    {10, 20, 30, 10, 20, 30},
    {200, 100, 50, 200, 100, 50}
};

const struct {
    const char* filename;
    PixelFormat format;
    Vector2i size;
    Containers::ArrayView<const void> pixels;
} ImportFileData[]{
    {"gray.jxl", PixelFormat::R8Unorm, {3, 2}, GrayPixels},
    {"gray16.jxl", PixelFormat::R16Unorm, {2, 3}, Gray16Pixels},
    {"ga.jxl", PixelFormat::RG8Unorm, {3, 2}, GrayAlphaPixels},
    {"ga-alpha16.jxl", PixelFormat::RG16Unorm, {3, 1}, GrayAlpha16Pixels},
    {"rgb.jxl", PixelFormat::RGB8Unorm, {3, 2}, RgbPixels},
    {"rgb10.jxl", PixelFormat::RGB16Unorm, {2, 1}, Rgb10Pixels},
    {"rgb16.jxl", PixelFormat::RGB16Unorm, {2, 3}, Rgb16Pixels},
    {"rotated.jxl", PixelFormat::RGB8Unorm, {2, 3}, RotatedPixels},
    {"rgba.jxl", PixelFormat::RGBA8Unorm, {3, 2}, RgbaPixels},
    {"premultiplied.jxl", PixelFormat::RGBA8Unorm, {2, 1}, UnpremultipliedRgbaPixels},
};

JxlImporterTest::JxlImporterTest() {
    addTests({&JxlImporterTest::empty,
              &JxlImporterTest::invalid,
              &JxlImporterTest::truncated,
              &JxlImporterTest::floatingPoint,
              &JxlImporterTest::cmyk});

    addInstancedTests({&JxlImporterTest::importFile},
        Containers::arraySize(ImportFileData));

    addTests({&JxlImporterTest::animated,
              &JxlImporterTest::xybIccProfile,
              &JxlImporterTest::threads});

    #ifdef JXLIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_manager.load(JXLIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
}

void JxlImporterTest::empty() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");

    Containers::String out;
    Error redirectError{&out};
    char a{};
    CORRADE_VERIFY(!importer->openData({&a, 0}));
    CORRADE_COMPARE(out, "Trade::JxlImporter::openData(): the file is empty\n");
}

void JxlImporterTest::invalid() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!importer->openData("this is not a JPEG XL file"));
    CORRADE_COMPARE(out, "Trade::JxlImporter::openData(): decoding failed\n");
}

void JxlImporterTest::truncated() {
    Containers::Optional<Containers::Array<char>> data = Utility::Path::read(Utility::Path::join(JXLIMPORTER_TEST_DIR, "rgb.jxl"));
    CORRADE_VERIFY(data);

    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openData(data->prefix(data->size()/2)));

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!importer->image2D(0));
    CORRADE_COMPARE(out, "Trade::JxlImporter::image2D(): decoding failed\n");
}

void JxlImporterTest::floatingPoint() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, "float.jxl")));

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!importer->image2D(0));
    CORRADE_COMPARE(out, "Trade::JxlImporter::image2D(): floating-point images are not supported\n");
}

void JxlImporterTest::cmyk() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, "cmyk.jxl")));

    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!importer->image2D(0));
    CORRADE_COMPARE(out, "Trade::JxlImporter::image2D(): CMYK images are not supported\n");
}

void JxlImporterTest::importFile() {
    auto&& data = ImportFileData[testCaseInstanceId()];
    setTestCaseDescription(data.filename);

    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, data.filename)));
    CORRADE_COMPARE(importer->image2DCount(), 1);
    CORRADE_VERIFY(!importer->importerState());

    Containers::Optional<ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->flags(), ImageFlags2D{});
    CORRADE_COMPARE(image->format(), data.format);
    CORRADE_COMPARE(image->size(), data.size);
    CORRADE_COMPARE_WITH(*image,
        (ImageView2D{PixelStorage{}.setAlignment(1), data.format, data.size, data.pixels}),
        DebugTools::CompareImage{});
}

void JxlImporterTest::animated() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, "animated.jxl")));
    CORRADE_COMPARE(importer->image2DCount(), 2);

    CORRADE_VERIFY(importer->importerState());
    CORRADE_COMPARE_AS(
        Containers::arrayView(reinterpret_cast<const Int*>(importer->importerState()), importer->image2DCount()),
        Containers::arrayView<Int>({100, 100}),
        TestSuite::Compare::Container);

    for(UnsignedInt i = 0; i != importer->image2DCount(); ++i) {
        CORRADE_ITERATION(i);
        Containers::Optional<ImageData2D> image = importer->image2D(i);
        CORRADE_VERIFY(image);
        CORRADE_COMPARE_WITH(*image,
            (ImageView2D{PixelStorage{}.setAlignment(1), PixelFormat::RGB8Unorm, {2, 1}, AnimatedPixels[i]}),
            DebugTools::CompareImage{});
    }
}

void JxlImporterTest::xybIccProfile() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, "xyb-icc.jxl")));

    Containers::Optional<ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE(image->format(), PixelFormat::RGB8Unorm);
    Color3ub expected[64];
    for(Color3ub& i: expected)
        i = Color3ub{128};
    CORRADE_COMPARE_WITH(*image,
        (ImageView2D{PixelFormat::RGB8Unorm, {8, 8}, expected}),
        (DebugTools::CompareImage{3.0f, 2.0f}));
}

void JxlImporterTest::threads() {
    Containers::Pointer<AbstractImporter> importer = _manager.instantiate("JxlImporter");
    importer->configuration().setValue("threads", 4);
    CORRADE_VERIFY(importer->openFile(Utility::Path::join(JXLIMPORTER_TEST_DIR, "rgba.jxl")));

    Containers::Optional<ImageData2D> image = importer->image2D(0);
    CORRADE_VERIFY(image);
    CORRADE_COMPARE_WITH(*image,
        (ImageView2D{PixelFormat::RGBA8Unorm, {3, 2}, RgbaPixels}),
        DebugTools::CompareImage{});
}

}}}}

CORRADE_TEST_MAIN(Magnum::Trade::Test::JxlImporterTest)
