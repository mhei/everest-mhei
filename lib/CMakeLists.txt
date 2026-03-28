# utility/helper library

add_library(everest-mhei-utils SHARED)
add_library(everest-mhei::utils ALIAS everest-mhei-utils)
ev_register_library_target(everest-mhei-utils)

target_sources(everest-mhei-utils
    PRIVATE
        src/empty.cpp
)

target_include_directories(everest-mhei-utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(everest-mhei-utils
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
)

set(CBUTILS_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")

install(
    DIRECTORY include/
    TYPE INCLUDE
)

install(
    TARGETS everest-mhei-utils
    EXPORT everest-mhei-utilsTargets
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT everest-mhei-utilsTargets
    FILE everest-mhei-utilsTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/everest-mhei-utils
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/everest-mhei-utilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/everest-mhei-utilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/everest-mhei-utils
    PATH_VARS CBUTILS_INCLUDE_INSTALL_DIR
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/everest-mhei-utilsConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/everest-mhei-utils
)
