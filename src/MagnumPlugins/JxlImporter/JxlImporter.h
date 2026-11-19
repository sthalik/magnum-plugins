#ifndef Magnum_Trade_JxlImporter_h
#define Magnum_Trade_JxlImporter_h

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

class MAGNUM_JXLIMPORTER_EXPORT JxlImporter: public AbstractImporter {
    MAGNUM_JXLIMPORTER_LOCAL ImporterFeatures doFeatures() const override;
    MAGNUM_JXLIMPORTER_LOCAL bool doIsOpened() const override;
    MAGNUM_JXLIMPORTER_LOCAL void doClose() override;
    MAGNUM_JXLIMPORTER_LOCAL void doOpenData(Containers::Array<char>&& data, DataFlags dataFlags) override;
    MAGNUM_JXLIMPORTER_LOCAL UnsignedInt doImage2DCount() const override;
    MAGNUM_JXLIMPORTER_LOCAL Containers::Optional<ImageData2D> doImage2D(UnsignedInt id, UnsignedInt level) override;

    public:
        explicit JxlImporter(PluginManager::AbstractManager& manager, const Containers::StringView& plugin);
        ~JxlImporter() override;
};

}} // namespace Magnum::Trade

#endif
