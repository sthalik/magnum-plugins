find_package(Magnum REQUIRED Trade)
find_package(Jxl REQUIRED)

if(MAGNUM_BUILD_PLUGINS_STATIC AND NOT DEFINED MAGNUM_JXLIMPORTER_BUILD_STATIC)
    set(MAGNUM_JXLIMPORTER_BUILD_STATIC 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

add_plugin(JxlImporter
    importers
    "${MAGNUM_PLUGINS_IMPORTER_DEBUG_BINARY_INSTALL_DIR};${MAGNUM_PLUGINS_IMPORTER_DEBUG_LIBRARY_INSTALL_DIR}"
    "${MAGNUM_PLUGINS_IMPORTER_RELEASE_BINARY_INSTALL_DIR};${MAGNUM_PLUGINS_IMPORTER_RELEASE_LIBRARY_INSTALL_DIR}"
    JxlImporter.conf
    JxlImporter.cpp
    JxlImporter.h)
if(MAGNUM_JXLIMPORTER_BUILD_STATIC AND MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(JxlImporter PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(JxlImporter PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(JxlImporter PUBLIC Magnum::Trade)
target_link_libraries(JxlImporter PUBLIC Jxl::Jxl)

install(FILES JxlImporter.h ${CMAKE_CURRENT_BINARY_DIR}/configure.h
    DESTINATION ${MAGNUM_PLUGINS_INCLUDE_INSTALL_DIR}/JxlImporter)

if(MAGNUM_JXLIMPORTER_BUILD_STATIC)
    install(FILES importStaticPlugin.cpp DESTINATION ${MAGNUM_PLUGINS_INCLUDE_INSTALL_DIR}/JxlImporter)
    target_sources(JxlImporter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/importStaticPlugin.cpp)
endif()

add_library(MagnumPlugins::JxlImporter ALIAS JxlImporter)
