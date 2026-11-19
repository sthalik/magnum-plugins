if(TARGET jxl)
    if(NOT TARGET Jxl::Jxl)
        add_library(Jxl::Jxl INTERFACE IMPORTED)
        set_target_properties(Jxl::Jxl PROPERTIES INTERFACE_LINK_LIBRARIES jxl)
    endif()

    set(_JXL_TARGET jxl)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Jxl DEFAULT_MSG _JXL_TARGET)
    return()
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND AND NOT CMAKE_VERSION VERSION_LESS 3.6)
    pkg_check_modules(PC_Jxl QUIET IMPORTED_TARGET libjxl)
endif()

find_library(Jxl_LIBRARY NAMES jxl jxl-static libjxl
    HINTS ${PC_Jxl_LIBRARY_DIRS})
find_path(Jxl_INCLUDE_DIR NAMES jxl/decode.h
    HINTS ${PC_Jxl_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jxl DEFAULT_MSG
    Jxl_LIBRARY
    Jxl_INCLUDE_DIR)

mark_as_advanced(Jxl_LIBRARY Jxl_INCLUDE_DIR)

if(Jxl_FOUND AND NOT TARGET Jxl::Jxl)
    if(TARGET PkgConfig::PC_Jxl)
        add_library(Jxl::Jxl INTERFACE IMPORTED)
        set_target_properties(Jxl::Jxl PROPERTIES
            INTERFACE_LINK_LIBRARIES PkgConfig::PC_Jxl)
    else()
        add_library(Jxl::Jxl UNKNOWN IMPORTED)
        set_target_properties(Jxl::Jxl PROPERTIES
            IMPORTED_LOCATION ${Jxl_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${Jxl_INCLUDE_DIR})
    endif()

    if(Jxl_LIBRARY MATCHES "\\.a$" AND NOT Jxl_LIBRARY MATCHES "\\.dll\\.a$" OR
       Jxl_LIBRARY MATCHES "jxl-static\\.lib$")
        set_target_properties(Jxl::Jxl PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS JXL_STATIC_DEFINE)
    endif()
endif()
