#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1
# insert your custom targets and additional config variables here

# libbaptismdata is required
pkg_search_module(LIBBAPTISMDATA REQUIRED libbaptismdata)

set(MODULE_DESCRIPTION "KVS interface to device's baptism data")

# make CMake project variables usable in source code
configure_file(configuration.h.in configuration.h @ONLY)

target_compile_features(${MODULE_NAME} PRIVATE cxx_std_17)

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${LIBBAPTISMDATA_INCLUDE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${LIBBAPTISMDATA_LIBRARIES}
)
# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

target_sources(${MODULE_NAME}
    PRIVATE
        "kvs/kvsImpl.cpp"
)
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
# insert other things like install cmds etc here
# create a VERSION file
configure_file(VERSION.in VERSION @ONLY)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/VERSION"
    DESTINATION
        "${EVEREST_MODULE_INSTALL_PREFIX}/${MODULE_NAME}"
)
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
