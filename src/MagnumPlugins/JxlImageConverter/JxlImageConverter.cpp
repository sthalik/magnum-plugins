#include "JxlImageConverter.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/ImageView.h>

namespace Magnum { namespace Trade {

using namespace Containers::Literals;

JxlImageConverter::JxlImageConverter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin):
    AbstractImageConverter{manager, plugin} {}

ImageConverterFeatures JxlImageConverter::doFeatures() const {
    return ImageConverterFeature::Convert2DToData;
}

Containers::String JxlImageConverter::doExtension() const {
    return "jxl"_s;
}

Containers::String JxlImageConverter::doMimeType() const {
    return "image/jxl"_s;
}

Containers::Optional<Containers::Array<char>> JxlImageConverter::doConvertToData(const ImageView2D&) {
    Error{} << "Trade::JxlImageConverter::convertToData(): JPEG XL encoding is not implemented";
    return {};
}

}} // namespace Magnum::Trade

CORRADE_PLUGIN_REGISTER(JxlImageConverter, Magnum::Trade::JxlImageConverter,
    MAGNUM_TRADE_ABSTRACTIMAGECONVERTER_PLUGIN_INTERFACE)
