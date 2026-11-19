#include "MagnumPlugins/JxlImporter/configure.h"

#ifdef MAGNUM_JXLIMPORTER_BUILD_STATIC
#include <Corrade/PluginManager/AbstractManager.h>
#include <Corrade/Utility/Macros.h>

static int magnumJxlImporterStaticImporter() {
    CORRADE_PLUGIN_IMPORT(JxlImporter)
    return 1;
} CORRADE_AUTOMATIC_INITIALIZER(magnumJxlImporterStaticImporter)
#endif
