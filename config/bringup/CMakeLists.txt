generate_config_tmux_run_script(CONFIG bringup-evachargese-ev)
generate_config_tmux_run_script(CONFIG bringup-evachargese-evse)
generate_config_tmux_run_script(CONFIG bringup-mcstester)

# install configs
install(
    DIRECTORY "."
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/everest"
    FILES_MATCHING
        PATTERN "*.yaml"
        PATTERN "*.json"
)
