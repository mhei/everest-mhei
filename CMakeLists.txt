cmake_minimum_required(VERSION 3.14.7)

project(everest-mhei
    VERSION 2024.09.0
    DESCRIPTION "Various EVerest modules"
    LANGUAGES CXX
)

find_package(everest-cmake 0.1
    COMPONENTS bundling
    PATHS ../everest-cmake
)

if(NOT DISABLE_EDM)
    # FIXME (aw): this implicit definition for child projects is hacky
    set(THIRD_PARTY_APP_DST "${CMAKE_INSTALL_LIBEXECDIR}/everest/3rd_party")

    evc_setup_edm()
else()
    find_package(everest-core REQUIRED)
endif()

# search for package PkgConfig
find_package(PkgConfig REQUIRED)

ev_add_project()
