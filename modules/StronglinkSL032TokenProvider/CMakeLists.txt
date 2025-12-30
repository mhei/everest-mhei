#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1
# insert your custom targets and additional config variables here
set(MODULE_DESCRIPTION "Token provider using Stronglink SL032 RFID readers")

# make CMake project variables usable in source code
configure_file(configuration.h.in configuration.h @ONLY)

# create a VERSION file
configure_file(VERSION.in VERSION @ONLY)

find_package(PalSigslot REQUIRED)

# we need gensio
pkg_search_module(LIBGENSIO REQUIRED libgensiocpp)
pkg_search_module(LIBGENSIOOSH REQUIRED libgensiooshcpp)

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${LIBGENSIO_INCLUDE_DIRS}
        ${LIBGENSIOOSH_INCLUDE_DIRS}
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        fmt::fmt
        Pal::Sigslot
        ${LIBGENSIO_LIBRARIES}
        ${LIBGENSIOOSH_LIBRARIES}
)

target_sources(${MODULE_NAME}
    PRIVATE
        "StronglinkSL032.cpp"
        "StronglinkSL032Event.cpp"
)

# helpers are not properly exported yet
target_compile_definitions(${MODULE_NAME}
    PRIVATE
        DISABLE_EDM=$<BOOL:${DISABLE_EDM}>
)

if(NOT DISABLE_EDM)
target_link_libraries(${MODULE_NAME}
    PRIVATE
        everest::helpers
)
endif()
# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

target_sources(${MODULE_NAME}
    PRIVATE
        "main/auth_token_providerImpl.cpp"
)

# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
# insert other things like install cmds etc here
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/VERSION"
    DESTINATION
        "${EVEREST_MODULE_INSTALL_PREFIX}/${MODULE_NAME}"
)
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
