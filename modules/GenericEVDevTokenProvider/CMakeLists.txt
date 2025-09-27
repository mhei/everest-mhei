#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

# search for libevdev
pkg_search_module(LIBEVDEV REQUIRED libevdev)

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${LIBEVDEV_INCLUDE_DIRS}
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${LIBEVDEV_LIBRARIES}
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
# ev@c55432ab-152c-45a9-9d2e-7281d50c69c3:v1
