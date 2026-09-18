cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME game2048gui)
project(${SOLUTION_NAME})

if(WIN32)
    enable_language(RC)
endif()
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
if (WIN32)
    set(HOME_ROOT $ENV{USERPROFILE})
    string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
    set(HOME_ROOT $ENV{HOME})
endif()
# natID 4.x moved the framework from $HOME/Work to $HOME/natID.SDK
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)

include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

include(game2048gui.cmake)
