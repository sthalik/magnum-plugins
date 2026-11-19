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

#include "JxlImporter.h"

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/ConfigurationGroup.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Trade/ImageData.h>

#include <jxl/decode_cxx.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner_cxx.h>

namespace Magnum { namespace Trade {

JxlImporter::JxlImporter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin): AbstractImporter{manager, plugin} {}

JxlImporter::~JxlImporter() = default;

ImporterFeatures JxlImporter::doFeatures() const { return ImporterFeature::OpenData; }

bool JxlImporter::doIsOpened() const { return !!_in; }

void JxlImporter::doClose() {
    _in = nullptr;
    _frameDelays = nullptr;
}

void JxlImporter::doOpenData(Containers::Array<char>&& data, const DataFlags dataFlags) {
    if(data.isEmpty()) {
        Error{} << "Trade::JxlImporter::openData(): the file is empty";
        return;
    }

    const JxlDecoderPtr decoder = JxlDecoderMake(nullptr);
    if(!decoder) {
        Error{} << "Trade::JxlImporter::openData(): can't create a decoder";
        return;
    }

    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSubscribeEvents(decoder.get(), JXL_DEC_BASIC_INFO|JXL_DEC_FRAME));
    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSetInput(decoder.get(), reinterpret_cast<const UnsignedByte*>(data.data()), data.size()));
    JxlDecoderCloseInput(decoder.get());

    JxlBasicInfo info{};
    Containers::Array<Int> frameDelays;
    for(;;) {
        const JxlDecoderStatus status = JxlDecoderProcessInput(decoder.get());
        if(status == JXL_DEC_ERROR) {
            Error{} << "Trade::JxlImporter::openData(): decoding failed";
            return;

        } else if(status == JXL_DEC_BASIC_INFO) {
            CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderGetBasicInfo(decoder.get(), &info));
            if(!info.have_animation)
                break;
            arrayReserve(frameDelays, 16);

        } else if(status == JXL_DEC_FRAME) {
            JxlFrameHeader frameHeader;
            CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderGetFrameHeader(decoder.get(), &frameHeader));
            const UnsignedLong delay = (UnsignedLong{frameHeader.duration}*1000*info.animation.tps_denominator + info.animation.tps_numerator/2)/info.animation.tps_numerator;
            arrayAppend(frameDelays, Int(Math::min(delay, UnsignedLong{0x7fffffff})));

        } else if(status == JXL_DEC_SUCCESS) {
            break;

        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
    }

    if(dataFlags & (DataFlag::Owned|DataFlag::ExternallyOwned))
        _in = Utility::move(data);
    else
        _in = Containers::Array<char>{InPlaceInit, data};
    _frameDelays = Utility::move(frameDelays);
}

UnsignedInt JxlImporter::doImage2DCount() const {
    return _frameDelays.isEmpty() ? 1 : UnsignedInt(_frameDelays.size());
}

const void* JxlImporter::doImporterState() const {
    return _frameDelays.data();
}

Containers::Optional<ImageData2D> JxlImporter::doImage2D(const UnsignedInt id, UnsignedInt) {
    static_assert(JXL_DEC_SUCCESS == 0,
        "cannot use JXL_DEC_SUCCESS in shorthand branches");

    const JxlDecoderPtr decoder = JxlDecoderMake(nullptr);
    if(!decoder) {
        Error{} << "Trade::JxlImporter::image2D(): can't create a decoder";
        return {};
    }

    JxlThreadParallelRunnerPtr runner;
    const Int threads = configuration().value<Int>("threads");
    if(threads > 1) {
        runner = JxlThreadParallelRunnerMake(nullptr, std::size_t(threads));
        if(!runner || JxlDecoderSetParallelRunner(decoder.get(), JxlThreadParallelRunner, runner.get())) {
            Error{} << "Trade::JxlImporter::image2D(): can't start" << threads << "threads";
            return {};
        }
    }

    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSubscribeEvents(decoder.get(), JXL_DEC_BASIC_INFO|JXL_DEC_COLOR_ENCODING|JXL_DEC_FULL_IMAGE));
    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSetKeepOrientation(decoder.get(), JXL_FALSE));
    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSetUnpremultiplyAlpha(decoder.get(), JXL_TRUE));
    CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSetInput(decoder.get(), reinterpret_cast<const UnsignedByte*>(_in.data()), _in.size()));
    JxlDecoderCloseInput(decoder.get());
    JxlDecoderSkipFrames(decoder.get(), id);

    JxlBasicInfo info{};
    JxlPixelFormat format{};
    PixelFormat pixelFormat{};
    Containers::Array<char> out;
    for(;;) {
        const JxlDecoderStatus status = JxlDecoderProcessInput(decoder.get());
        if(status == JXL_DEC_ERROR) {
            Error{} << "Trade::JxlImporter::image2D(): decoding failed";
            return {};

        } else if(status == JXL_DEC_BASIC_INFO) {
            CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderGetBasicInfo(decoder.get(), &info));

            if(info.exponent_bits_per_sample || info.alpha_exponent_bits) {
                Error{} << "Trade::JxlImporter::image2D(): floating-point images are not supported";
                return {};
            }

            for(UnsignedInt i = 0; i != info.num_extra_channels; ++i) {
                JxlExtraChannelInfo extraChannelInfo;
                CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderGetExtraChannelInfo(decoder.get(), i, &extraChannelInfo));
                if(extraChannelInfo.type == JXL_CHANNEL_BLACK) {
                    Error{} << "Trade::JxlImporter::image2D(): CMYK images are not supported";
                    return {};
                }
            }

            const UnsignedInt bitsPerSample = Math::max(info.bits_per_sample, info.alpha_bits);
            PixelFormat channelFormat;
            JxlDataType dataType;
            if(bitsPerSample <= 8) {
                channelFormat = PixelFormat::R8Unorm;
                dataType = JXL_TYPE_UINT8;
            } else if(bitsPerSample <= 16) {
                channelFormat = PixelFormat::R16Unorm;
                dataType = JXL_TYPE_UINT16;
            } else {
                Error{} << "Trade::JxlImporter::image2D(): unsupported bit depth" << bitsPerSample;
                return {};
            }

            const UnsignedInt channelCount = info.num_color_channels + (info.alpha_bits ? 1 : 0);
            pixelFormat = Magnum::pixelFormat(channelFormat, channelCount, false);
            format = JxlPixelFormat{channelCount, dataType, JXL_NATIVE_ENDIAN, 0};

        } else if(status == JXL_DEC_COLOR_ENCODING) {
            /* Without a CMS, libjxl converts only XYB images. Others keep their
               color space, as libjxl provides a CMS only since 0.9. */
            if(!info.uses_original_profile) {
                JxlColorEncoding srgb;
                JxlColorEncodingSetToSRGB(&srgb, info.num_color_channels == 1);
                if(JxlDecoderSetPreferredColorProfile(decoder.get(), &srgb)) {
                    Error{} << "Trade::JxlImporter::image2D(): can't convert the image to sRGB";
                    return {};
                }
            }

        } else if(status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            std::size_t size;
            CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderImageOutBufferSize(decoder.get(), &format, &size));
            CORRADE_INTERNAL_ASSERT(size == std::size_t{info.xsize}*info.ysize*pixelFormatSize(pixelFormat));
            out = Containers::Array<char>{NoInit, size};
            CORRADE_INTERNAL_ASSERT_OUTPUT(!JxlDecoderSetImageOutBuffer(decoder.get(), &format, out.data(), out.size()));

        } else if(status == JXL_DEC_FULL_IMAGE) {
            break;

        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
    }

    const std::size_t rowSize = std::size_t{info.xsize}*pixelFormatSize(pixelFormat);
    Utility::flipInPlace<0>(Containers::StridedArrayView2D<char>{out, {info.ysize, rowSize}});

    PixelStorage storage;
    if(rowSize % 4 != 0)
        storage.setAlignment(1);
    return ImageData2D{storage, pixelFormat, {Int(info.xsize), Int(info.ysize)}, Utility::move(out)};
}

}}

CORRADE_PLUGIN_REGISTER(JxlImporter, Magnum::Trade::JxlImporter,
    MAGNUM_TRADE_ABSTRACTIMPORTER_PLUGIN_INTERFACE)
