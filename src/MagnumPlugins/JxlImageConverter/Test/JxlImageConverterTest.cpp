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
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Path.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DebugTools/CompareImage.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "configure.h"

namespace Magnum { namespace Trade { namespace Test { namespace {

using namespace Containers::Literals;

struct JxlImageConverterTest: TestSuite::Tester {
    explicit JxlImageConverterTest();

    void wrongFormat();
    void roundTrip();
    void roundTripPng();
    void roundTripLarge();
    void option();
    void optionOutOfRange();
    void unsupportedMetadata();

    PluginManager::Manager<AbstractImageConverter> _converterManager{"nonexistent"};
    PluginManager::Manager<AbstractImporter> _importerManager{"nonexistent"};
};

const struct {
    const char* name;
    PixelFormat format;
    Containers::StringView signature;
} RoundTripData[]{
    {"R8", PixelFormat::R8Unorm, "\xff\x0a"_s},
    {"RG8", PixelFormat::RG8Unorm, "\xff\x0a"_s},
    {"RGB8", PixelFormat::RGB8Unorm, "\xff\x0a"_s},
    {"RGBA8", PixelFormat::RGBA8Unorm, "\xff\x0a"_s},
    {"R16", PixelFormat::R16Unorm, "\x00\x00\x00\x0cJXL \x0d\x0a\x87\x0a"_s},
    {"RG16", PixelFormat::RG16Unorm, "\x00\x00\x00\x0cJXL \x0d\x0a\x87\x0a"_s},
    {"RGB16", PixelFormat::RGB16Unorm, "\x00\x00\x00\x0cJXL \x0d\x0a\x87\x0a"_s},
    {"RGBA16", PixelFormat::RGBA16Unorm, "\x00\x00\x00\x0cJXL \x0d\x0a\x87\x0a"_s},
};

const struct {
    const char* filename;
    PixelFormat format;
} RoundTripPngData[]{
    {"gray.png", PixelFormat::R8Unorm},
    {"gray16.png", PixelFormat::R16Unorm},
    {"ga.png", PixelFormat::RG8Unorm},
    {"rgb.png", PixelFormat::RGB8Unorm},
    {"rgb16.png", PixelFormat::RGB16Unorm},
    {"rgb-palette.png", PixelFormat::RGB8Unorm},
    {"rgba.png", PixelFormat::RGBA8Unorm},
};

const struct {
    const char* name;
    bool tiled;
    Int effort;
} RoundTripLargeData[]{
    {"pseudo-random, effort 1", false, 1},
    {"pseudo-random, effort 3", false, 3},
    {"four colors in 8x8 tiles, effort 3", true, 3},
};

const struct {
    const char* name;
    const char* key;
    Int value;
    Vector2i size;
} OptionData[]{
    {"effort 1", "effort", 1, {67, 65}},
    {"effort 10", "effort", 10, {16, 16}},
    {"decodingSpeed 4", "decodingSpeed", 4, {67, 65}},
    {"threads 4", "threads", 4, {67, 65}},
};

const struct {
    const char* key;
    Int value;
} OptionOutOfRangeData[]{
    {"effort", 0},
    {"effort", 11},
    {"decodingSpeed", -1},
    {"decodingSpeed", 5},
};

const struct {
    const char* name;
    ImageConverterFlags flags;
    bool quiet;
} UnsupportedMetadataData[]{
    {"1D array", {}, false},
    {"1D array, quiet", ImageConverterFlag::Quiet, true},
};

void fillPseudoRandom(Containers::ArrayView<char> data) {
    UnsignedInt state = 0x2545f491u;
    for(char& c: data) {
        state = state*1664525u + 1013904223u;
        c = char(state >> 24);
    }
}

JxlImageConverterTest::JxlImageConverterTest() {
    addTests({&JxlImageConverterTest::wrongFormat});

    addInstancedTests({&JxlImageConverterTest::roundTrip},
        Containers::arraySize(RoundTripData));

    addInstancedTests({&JxlImageConverterTest::roundTripPng},
        Containers::arraySize(RoundTripPngData));

    addInstancedTests({&JxlImageConverterTest::roundTripLarge},
        Containers::arraySize(RoundTripLargeData));

    addInstancedTests({&JxlImageConverterTest::option},
        Containers::arraySize(OptionData));

    addInstancedTests({&JxlImageConverterTest::optionOutOfRange},
        Containers::arraySize(OptionOutOfRangeData));

    addInstancedTests({&JxlImageConverterTest::unsupportedMetadata},
        Containers::arraySize(UnsupportedMetadataData));

    #ifdef JXLIMAGECONVERTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_converterManager.load(JXLIMAGECONVERTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
    #ifdef JXLIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_importerManager.load(JXLIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
    #ifdef PNGIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_importerManager.load(PNGIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
    #ifdef STBIMAGEIMPORTER_PLUGIN_FILENAME
    CORRADE_INTERNAL_ASSERT_OUTPUT(_importerManager.load(STBIMAGEIMPORTER_PLUGIN_FILENAME) & PluginManager::LoadState::Loaded);
    #endif
}

void JxlImageConverterTest::wrongFormat() {
    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");

    const char data[8]{};
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertToData(ImageView2D{PixelFormat::RG32F, {1, 1}, data}));
    CORRADE_COMPARE(out, "Trade::JxlImageConverter::convertToData(): unsupported format PixelFormat::RG32F\n");
}

void JxlImageConverterTest::roundTrip() {
    auto&& data = RoundTripData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    CORRADE_COMPARE(converter->extension(), "jxl");
    CORRADE_COMPARE(converter->mimeType(), "image/jxl");
    converter->configuration().setValue("effort", 3);

    Containers::Array<char> pixels{NoInit, 3*4*pixelFormatSize(data.format)};
    fillPseudoRandom(pixels);
    const ImageView2D image{PixelStorage{}.setRowLength(4).setSkip({1, 1, 0}), data.format, {3, 2}, pixels};

    Containers::Optional<Containers::Array<char>> out = converter->convertToData(image);
    CORRADE_VERIFY(out);
    CORRADE_VERIFY(Containers::StringView{*out}.hasPrefix(data.signature));

    if(_importerManager.loadState("JxlImporter") == PluginManager::LoadState::NotFound)
        CORRADE_SKIP("JxlImporter plugin not found, cannot test");

    Containers::Pointer<AbstractImporter> importer = _importerManager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openData(*out));
    Containers::Optional<ImageData2D> imported = importer->image2D(0);
    CORRADE_VERIFY(imported);
    CORRADE_COMPARE(imported->format(), data.format);
    CORRADE_COMPARE_WITH(*imported, image, DebugTools::CompareImage{});
}

void JxlImageConverterTest::roundTripPng() {
    auto&& data = RoundTripPngData[testCaseInstanceId()];
    setTestCaseDescription(data.filename);

    if(_importerManager.loadState("PngImporter") == PluginManager::LoadState::NotFound)
        CORRADE_SKIP("PngImporter plugin not found, cannot test");
    if(_importerManager.loadState("JxlImporter") == PluginManager::LoadState::NotFound)
        CORRADE_SKIP("JxlImporter plugin not found, cannot test");

    Containers::Pointer<AbstractImporter> pngImporter = _importerManager.instantiate("PngImporter");
    CORRADE_VERIFY(pngImporter->openFile(Utility::Path::join(PNGIMPORTER_TEST_DIR, data.filename)));
    Containers::Optional<ImageData2D> original = pngImporter->image2D(0);
    CORRADE_VERIFY(original);
    CORRADE_COMPARE(original->format(), data.format);

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    converter->configuration().setValue("effort", 3);
    Containers::Optional<Containers::Array<char>> out = converter->convertToData(*original);
    CORRADE_VERIFY(out);

    Containers::Pointer<AbstractImporter> importer = _importerManager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openData(*out));
    Containers::Optional<ImageData2D> imported = importer->image2D(0);
    CORRADE_VERIFY(imported);
    CORRADE_COMPARE(imported->format(), data.format);
    CORRADE_COMPARE_WITH(*imported, *original, DebugTools::CompareImage{});
}

void JxlImageConverterTest::roundTripLarge() {
    auto&& data = RoundTripLargeData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(_importerManager.loadState("JxlImporter") == PluginManager::LoadState::NotFound)
        CORRADE_SKIP("JxlImporter plugin not found, cannot test");

    const Vector2i size{259, 261};
    Containers::Array<char> pixels{NoInit, std::size_t(size.product())*4};
    fillPseudoRandom(pixels);
    if(data.tiled) {
        const char colors[4][4]{{'\x00', '\x00', '\x00', '\xff'}, {'\xff', '\x80', '\x00', '\xff'},
                                {'\x20', '\x40', '\xff', '\x80'}, {'\xff', '\xff', '\xff', '\x00'}};
        char tileRandomBytes[8*8];
        fillPseudoRandom(tileRandomBytes);
        bool colorUsed[4]{};
        for(Int y = 0; y != size.y(); ++y) {
            for(Int x = 0; x != size.x(); ++x) {
                const UnsignedByte color = UnsignedByte(tileRandomBytes[y%8*8 + x%8]) % 4;
                colorUsed[color] = true;
                for(Int i = 0; i != 4; ++i)
                    pixels[(y*size.x() + x)*4 + i] = colors[color][i];
            }
        }
        CORRADE_VERIFY(colorUsed[0] && colorUsed[1] && colorUsed[2] && colorUsed[3]);
    }
    const ImageView2D image{PixelStorage{}.setAlignment(1), PixelFormat::RGBA8Unorm, size, pixels};

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    converter->configuration().setValue("effort", data.effort);
    Containers::Optional<Containers::Array<char>> out = converter->convertToData(image);
    CORRADE_VERIFY(out);

    Containers::Pointer<AbstractImporter> importer = _importerManager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openData(*out));
    Containers::Optional<ImageData2D> imported = importer->image2D(0);
    CORRADE_VERIFY(imported);
    CORRADE_COMPARE_WITH(*imported, image, DebugTools::CompareImage{});
}

void JxlImageConverterTest::option() {
    auto&& data = OptionData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    if(_importerManager.loadState("JxlImporter") == PluginManager::LoadState::NotFound)
        CORRADE_SKIP("JxlImporter plugin not found, cannot test");

    Containers::Array<char> pixels{NoInit, std::size_t(data.size.product())*8};
    fillPseudoRandom(pixels);
    const ImageView2D image{PixelFormat::RGBA16Unorm, data.size, pixels};

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    converter->configuration().setValue("effort", 3);
    converter->configuration().setValue(data.key, data.value);
    Containers::Optional<Containers::Array<char>> out = converter->convertToData(image);
    CORRADE_VERIFY(out);

    Containers::Pointer<AbstractImporter> importer = _importerManager.instantiate("JxlImporter");
    CORRADE_VERIFY(importer->openData(*out));
    Containers::Optional<ImageData2D> imported = importer->image2D(0);
    CORRADE_VERIFY(imported);
    CORRADE_COMPARE_WITH(*imported, image, DebugTools::CompareImage{});
}

void JxlImageConverterTest::optionOutOfRange() {
    auto&& data = OptionOutOfRangeData[testCaseInstanceId()];
    setTestCaseDescription(Utility::format("{} {}", data.key, data.value));

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    converter->configuration().setValue(data.key, data.value);

    const char pixels[4]{};
    Containers::String out;
    Error redirectError{&out};
    CORRADE_VERIFY(!converter->convertToData(ImageView2D{PixelFormat::RGBA8Unorm, {1, 1}, pixels}));
    CORRADE_COMPARE(out, Utility::format("Trade::JxlImageConverter::convertToData(): can't set {} to {}: not supported\n", data.key, data.value));
}

void JxlImageConverterTest::unsupportedMetadata() {
    auto&& data = UnsupportedMetadataData[testCaseInstanceId()];
    setTestCaseDescription(data.name);

    Containers::Pointer<AbstractImageConverter> converter = _converterManager.instantiate("JxlImageConverter");
    converter->addFlags(data.flags);
    converter->configuration().setValue("effort", 3);

    const char pixels[4]{};
    Containers::String out;
    Warning redirectWarning{&out};
    CORRADE_VERIFY(converter->convertToData(ImageView2D{PixelFormat::RGBA8Unorm, {1, 1}, pixels, ImageFlag2D::Array}));
    if(data.quiet)
        CORRADE_COMPARE(out, "");
    else
        CORRADE_COMPARE(out, "Trade::JxlImageConverter::convertToData(): 1D array images are unrepresentable in JPEG XL, saving as a regular 2D image\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Trade::Test::JxlImageConverterTest)
