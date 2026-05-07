generate_config_tmux_run_script(CONFIG bringup-evachargese-evse)

# install configs
install(
    DIRECTORY "."
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/everest"
    FILES_MATCHING PATTERN "*.yaml" PATTERN "*.json" PATTERN "run_tmux_helper.sh"
)

# install helper script to start bring up configs in tmux
install(
    FILES "run_tmux_helper.sh"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/everest"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
