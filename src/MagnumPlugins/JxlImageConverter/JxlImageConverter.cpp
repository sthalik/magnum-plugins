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

#include "JxlImageConverter.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Functions.h>

#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner_cxx.h>
#include <jxl/version.h>

namespace Magnum { namespace Trade {

using namespace Containers::Literals;

JxlImageConverter::JxlImageConverter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin): AbstractImageConverter{manager, plugin} {}

ImageConverterFeatures JxlImageConverter::doFeatures() const { return ImageConverterFeature::Convert2DToData; }

Containers::String JxlImageConverter::doExtension() const { return "jxl"_s; }

Containers::String JxlImageConverter::doMimeType() const { return "image/jxl"_s; }

namespace {

const char* errorString(const JxlEncoderError error) {
    switch(error) {
        case JXL_ENC_ERR_OK: return "no error";
        case JXL_ENC_ERR_GENERIC: return "generic error";
        case JXL_ENC_ERR_OOM: return "out of memory";
        case JXL_ENC_ERR_JBRD: return "JPEG reconstruction data error";
        case JXL_ENC_ERR_BAD_INPUT: return "bad input";
        case JXL_ENC_ERR_NOT_SUPPORTED: return "not supported";
        case JXL_ENC_ERR_API_USAGE: return "API usage error";
    }
    return "unknown error";
}

}

Containers::Optional<Containers::Array<char>> JxlImageConverter::doConvertToData(const ImageView2D& image) {
    if((image.flags() & ImageFlag2D::Array) && !(flags() & ImageConverterFlag::Quiet))
        Warning{} << "Trade::JxlImageConverter::convertToData(): 1D array images are unrepresentable in JPEG XL, saving as a regular 2D image";

    JxlDataType dataType;
    UnsignedInt bitsPerSample;
    switch(image.format()) {
        case PixelFormat::R8Unorm:
        case PixelFormat::RG8Unorm:
        case PixelFormat::RGB8Unorm:
        case PixelFormat::RGBA8Unorm:
            dataType = JXL_TYPE_UINT8;
            bitsPerSample = 8;
            break;
        case PixelFormat::R16Unorm:
        case PixelFormat::RG16Unorm:
        case PixelFormat::RGB16Unorm:
        case PixelFormat::RGBA16Unorm:
            dataType = JXL_TYPE_UINT16;
            bitsPerSample = 16;
            break;
        default:
            Error{} << "Trade::JxlImageConverter::convertToData(): unsupported format" << image.format();
            return {};
    }

    const UnsignedInt channelCount = pixelFormatChannelCount(image.format());
    const bool hasAlpha = channelCount == 2 || channelCount == 4;
    const Int effort = configuration().value<Int>("effort");
    const Int decodingSpeed = configuration().value<Int>("decodingSpeed");
    const Int threads = configuration().value<Int>("threads");

    const JxlEncoderPtr encoder = JxlEncoderMake(nullptr);
    if(!encoder) {
        Error{} << "Trade::JxlImageConverter::convertToData(): can't create an encoder";
        return {};
    }

    JxlThreadParallelRunnerPtr runner;
    if(threads > 1) {
        runner = JxlThreadParallelRunnerMake(nullptr, std::size_t(threads));
        if(!runner || JxlEncoderSetParallelRunner(encoder.get(), JxlThreadParallelRunner, runner.get()) != JXL_ENC_SUCCESS) {
            Error{} << "Trade::JxlImageConverter::convertToData(): can't start" << threads << "threads";
            return {};
        }
    }

    JxlEncoderAllowExpertOptions(encoder.get());

    JxlBasicInfo info;
    JxlEncoderInitBasicInfo(&info);
    info.xsize = UnsignedInt(image.size().x());
    info.ysize = UnsignedInt(image.size().y());
    info.bits_per_sample = bitsPerSample;
    info.exponent_bits_per_sample = 0;
    info.num_color_channels = channelCount >= 3 ? 3 : 1;
    info.num_extra_channels = hasAlpha ? 1 : 0;
    info.alpha_bits = hasAlpha ? bitsPerSample : 0;
    info.alpha_exponent_bits = 0;
    info.alpha_premultiplied = JXL_FALSE;
    info.orientation = JXL_ORIENT_IDENTITY;
    info.have_animation = JXL_FALSE;
    info.uses_original_profile = JXL_TRUE;
    if(JxlEncoderSetBasicInfo(encoder.get(), &info) != JXL_ENC_SUCCESS) {
        Error{} << "Trade::JxlImageConverter::convertToData(): can't set basic info for a" << image.size() << "image:" << errorString(JxlEncoderGetError(encoder.get()));
        return {};
    }

    JxlColorEncoding colorEncoding;
    JxlColorEncodingSetToSRGB(&colorEncoding, info.num_color_channels == 1);
    if(JxlEncoderSetColorEncoding(encoder.get(), &colorEncoding) != JXL_ENC_SUCCESS) {
        Error{} << "Trade::JxlImageConverter::convertToData(): can't set color encoding:" << errorString(JxlEncoderGetError(encoder.get()));
        return {};
    }

    JxlEncoderFrameSettings* const frameSettings = JxlEncoderFrameSettingsCreate(encoder.get(), nullptr);
    if(!frameSettings) {
        Error{} << "Trade::JxlImageConverter::convertToData(): can't create frame settings";
        return {};
    }

    const JxlBitDepth bitDepth{JXL_BIT_DEPTH_FROM_CODESTREAM, 0, 0};
    if(JxlEncoderSetFrameLossless(frameSettings, JXL_TRUE) != JXL_ENC_SUCCESS ||
       JxlEncoderSetFrameDistance(frameSettings, 0.0f) != JXL_ENC_SUCCESS ||
       JxlEncoderSetFrameBitDepth(frameSettings, &bitDepth) != JXL_ENC_SUCCESS ||
       JxlEncoderFrameSettingsSetFloatOption(frameSettings, JXL_ENC_FRAME_SETTING_PHOTON_NOISE, 0.0f) != JXL_ENC_SUCCESS) {
        Error{} << "Trade::JxlImageConverter::convertToData(): can't set lossless frame settings:" << errorString(JxlEncoderGetError(encoder.get()));
        return {};
    }

    const struct {
        const char* name;
        JxlEncoderFrameSettingId id;
        Int value;
    } frameOptions[]{
        {"effort", JXL_ENC_FRAME_SETTING_EFFORT, effort},
        {"decodingSpeed", JXL_ENC_FRAME_SETTING_DECODING_SPEED, decodingSpeed},
        {"modular", JXL_ENC_FRAME_SETTING_MODULAR, 1},
        {"color transform", JXL_ENC_FRAME_SETTING_COLOR_TRANSFORM, 1},
        {"resampling", JXL_ENC_FRAME_SETTING_RESAMPLING, 1},
        {"extra channel resampling", JXL_ENC_FRAME_SETTING_EXTRA_CHANNEL_RESAMPLING, 1},
        {"already downsampled", JXL_ENC_FRAME_SETTING_ALREADY_DOWNSAMPLED, 0},
        {"noise", JXL_ENC_FRAME_SETTING_NOISE, 0},
        {"dots", JXL_ENC_FRAME_SETTING_DOTS, 0},
        {"EPF", JXL_ENC_FRAME_SETTING_EPF, 0},
        {"gaborish", JXL_ENC_FRAME_SETTING_GABORISH, 0},
        {"keep invisible", JXL_ENC_FRAME_SETTING_KEEP_INVISIBLE, 1},
        {"lossy palette", JXL_ENC_FRAME_SETTING_LOSSY_PALETTE, 0},
        #if JPEGXL_NUMERIC_VERSION >= JPEGXL_COMPUTE_NUMERIC_VERSION(0, 13, 0)
        {"strip alpha", JXL_ENC_FRAME_SETTING_STRIP_ALPHA, 0},
        #endif
    };
    for(const auto& option: frameOptions) {
        if(JxlEncoderFrameSettingsSetOption(frameSettings, option.id, option.value) != JXL_ENC_SUCCESS) {
            Error{} << "Trade::JxlImageConverter::convertToData(): can't set" << option.name << "to" << option.value << Debug::nospace << ":" << errorString(JxlEncoderGetError(encoder.get()));
            return {};
        }
    }

    const std::size_t width = std::size_t(image.size().x());
    const std::size_t height = std::size_t(image.size().y());
    const std::size_t pixelSize = image.pixelSize();
    const std::size_t imageSize = width*height*pixelSize;
    {
        Containers::Array<char> pixels{NoInit, imageSize};
        Utility::copy(image.pixels().flipped<0>(), Containers::StridedArrayView3D<char>{pixels, {height, width, pixelSize}});

        const JxlPixelFormat pixelFormat{channelCount, dataType, JXL_NATIVE_ENDIAN, 0};
        if(JxlEncoderAddImageFrame(frameSettings, &pixelFormat, pixels.data(), pixels.size()) != JXL_ENC_SUCCESS) {
            Error{} << "Trade::JxlImageConverter::convertToData(): can't add the image:" << errorString(JxlEncoderGetError(encoder.get()));
            return {};
        }
    }
    JxlEncoderCloseInput(encoder.get());

    Containers::Array<char> output;
    arrayResize(output, NoInit, Math::max(imageSize/2, std::size_t{4096}));
    std::size_t outputSize = 0;
    for(;;) {
        UnsignedByte* next = reinterpret_cast<UnsignedByte*>(output.data() + outputSize);
        std::size_t available = output.size() - outputSize;
        const JxlEncoderStatus status = JxlEncoderProcessOutput(encoder.get(), &next, &available);
        outputSize = std::size_t(reinterpret_cast<char*>(next) - output.data());
        if(status == JXL_ENC_SUCCESS)
            break;
        else if(status == JXL_ENC_NEED_MORE_OUTPUT)
            arrayResize(output, NoInit, output.size()*2);
        else {
            Error{} << "Trade::JxlImageConverter::convertToData(): encoding failed:" << errorString(JxlEncoderGetError(encoder.get()));
            return {};
        }
    }

    arrayResize(output, outputSize);
    arrayShrink(output);

    return Containers::optional(Utility::move(output));
}

}}

CORRADE_PLUGIN_REGISTER(JxlImageConverter, Magnum::Trade::JxlImageConverter,
    MAGNUM_TRADE_ABSTRACTIMAGECONVERTER_PLUGIN_INTERFACE)
