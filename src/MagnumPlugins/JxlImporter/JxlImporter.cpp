#include "JxlImporter.h"

#include <Corrade/Containers/Optional.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Trade/ImageData.h>

namespace Magnum { namespace Trade {

JxlImporter::JxlImporter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin):
    AbstractImporter{manager, plugin} {}

JxlImporter::~JxlImporter() = default;

ImporterFeatures JxlImporter::doFeatures() const {
    return ImporterFeature::OpenData;
}

bool JxlImporter::doIsOpened() const {
    return false;
}

void JxlImporter::doClose() {}

void JxlImporter::doOpenData(Containers::Array<char>&&, DataFlags) {
    Error{} << "Trade::JxlImporter::openData(): JPEG XL decoding is not implemented";
}

UnsignedInt JxlImporter::doImage2DCount() const {
    return 0;
}

Containers::Optional<ImageData2D> JxlImporter::doImage2D(UnsignedInt, UnsignedInt) {
    return {};
}

}} // namespace Magnum::Trade

CORRADE_PLUGIN_REGISTER(JxlImporter, Magnum::Trade::JxlImporter,
    MAGNUM_TRADE_ABSTRACTIMPORTER_PLUGIN_INTERFACE)
