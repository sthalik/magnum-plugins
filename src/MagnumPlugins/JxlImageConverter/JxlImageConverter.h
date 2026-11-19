#ifndef Magnum_Trade_JxlImageConverter_h
#define Magnum_Trade_JxlImageConverter_h

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

class MAGNUM_JXLIMAGECONVERTER_EXPORT JxlImageConverter: public AbstractImageConverter {
    MAGNUM_JXLIMAGECONVERTER_LOCAL ImageConverterFeatures doFeatures() const override;
    MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::String doExtension() const override;
    MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::String doMimeType() const override;
    MAGNUM_JXLIMAGECONVERTER_LOCAL Containers::Optional<Containers::Array<char>> doConvertToData(const ImageView2D& image) override;

    public:
        explicit JxlImageConverter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin);
};

}} // namespace Magnum::Trade

#endif
