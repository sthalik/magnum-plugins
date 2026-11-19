#include "MagnumPlugins/JxlImageConverter/configure.h"

#ifdef MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC
#include <Corrade/PluginManager/AbstractManager.h>
#include <Corrade/Utility/Macros.h>

static int magnumJxlImageConverterStaticImporter() {
    CORRADE_PLUGIN_IMPORT(JxlImageConverter)
    return 1;
} CORRADE_AUTOMATIC_INITIALIZER(magnumJxlImageConverterStaticImporter)
#endif
