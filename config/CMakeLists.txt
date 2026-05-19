generate_config_run_script(CONFIG hil-dc-d20)
generate_config_run_script(CONFIG hil-mcs-ev)

add_subdirectory(bringup)

# install configs
install(
    DIRECTORY "."
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/everest"
    FILES_MATCHING PATTERN "*.yaml"
)
