# utility/helper library

find_package(PalSigslot REQUIRED)

# we need gensio
pkg_search_module(LIBGENSIO REQUIRED libgensiocpp)
pkg_search_module(LIBGENSIOOSH REQUIRED libgensiooshcpp)

add_library(everest-mhei-utils SHARED)
add_library(everest-mhei::utils ALIAS everest-mhei-utils)
ev_register_library_target(everest-mhei-utils)

get_target_property(GENERATED_INCLUDE_DIR generate_cpp_files EVEREST_GENERATED_INCLUDE_DIR)

add_dependencies(everest-mhei-utils generate_cpp_files)

target_sources(everest-mhei-utils
    PRIVATE
        src/KL02.cpp
        src/KL02USSProtocol.cpp
        src/USSFrame.cpp
)

target_include_directories(everest-mhei-utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(everest-mhei-utils
    PUBLIC
        Pal::Sigslot
        ${LIBGENSIO_LIBRARIES}
        ${LIBGENSIOOSH_LIBRARIES}
    PRIVATE
        everest::log
        fmt::fmt
)

set_target_properties(everest-mhei-utils
    PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
)

set(MHEIUTILS_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")

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
    PATH_VARS MHEIUTILS_INCLUDE_INSTALL_DIR
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/everest-mhei-utilsConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/everest-mhei-utils
)
