find_package(Magnum REQUIRED Trade)
find_package(Jxl REQUIRED)

if(MAGNUM_BUILD_PLUGINS_STATIC AND NOT DEFINED MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC)
    set(MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

add_plugin(JxlImageConverter
    imageconverters
    "${MAGNUM_PLUGINS_IMAGECONVERTER_DEBUG_BINARY_INSTALL_DIR};${MAGNUM_PLUGINS_IMAGECONVERTER_DEBUG_LIBRARY_INSTALL_DIR}"
    "${MAGNUM_PLUGINS_IMAGECONVERTER_RELEASE_BINARY_INSTALL_DIR};${MAGNUM_PLUGINS_IMAGECONVERTER_RELEASE_LIBRARY_INSTALL_DIR}"
    JxlImageConverter.conf
    JxlImageConverter.cpp
    JxlImageConverter.h)
if(MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC AND MAGNUM_BUILD_STATIC_PIC)
    set_target_properties(JxlImageConverter PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(JxlImageConverter PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(JxlImageConverter PUBLIC Magnum::Trade)
target_link_libraries(JxlImageConverter PUBLIC Jxl::Jxl)

install(FILES JxlImageConverter.h ${CMAKE_CURRENT_BINARY_DIR}/configure.h
    DESTINATION ${MAGNUM_PLUGINS_INCLUDE_INSTALL_DIR}/JxlImageConverter)

if(MAGNUM_JXLIMAGECONVERTER_BUILD_STATIC)
    install(FILES importStaticPlugin.cpp DESTINATION ${MAGNUM_PLUGINS_INCLUDE_INSTALL_DIR}/JxlImageConverter)
    target_sources(JxlImageConverter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/importStaticPlugin.cpp)
endif()

add_library(MagnumPlugins::JxlImageConverter ALIAS JxlImageConverter)
