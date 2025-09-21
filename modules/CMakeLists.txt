add_compile_options(-Wall -Wextra -Werror=switch)

ev_add_cpp_module(BaptismData)
ev_add_cpp_module(GenericEVDevTokenProvider)
ev_add_cpp_module(SimpleEVDevTxStopBtn)

if (BUILD_TEST_MODULES)
    ev_add_cpp_module(BaptismDataTest)
    ev_add_cpp_module(SimpleSwitch3ph1phAPI)
endif()
