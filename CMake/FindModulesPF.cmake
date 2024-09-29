# FindModulesPF.cmake

FUNCTION(search_with_pkg MODULE_NAME)
    FIND_PACKAGE(PkgConfig QUIET)
    IF(NOT PkgConfig_FOUND)
        MESSAGE(WARNING "Pkg was not found in the system here.")
        SET(FOUND_PKG OFF PARENT_SCOPE)
        RETURN()
    ENDIF()

    pkg_search_module(MODULE ${MODULE_NAME})
    IF(MODULE_FOUND)
        MESSAGE(STATUS "${MODULE_NAME} Found in the system.")
        SET(FOUND_PKG ON PARENT_SCOPE)
    ELSE()
        MESSAGE(STATUS "${MODULE_NAME} Was not found in the system.")
        SET(FOUND_PKG OFF PARENT_SCOPE)
    ENDIF()
ENDFUNCTION()