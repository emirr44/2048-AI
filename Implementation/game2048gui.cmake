set(GAME2048GUI_NAME game2048gui)

if(WIN32)
    set(GAME2048GUI_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
else()
    set(GAME2048GUI_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
endif()

file(GLOB GAME2048GUI_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/gui/*.cpp)
file(GLOB GAME2048GUI_INCS    ${CMAKE_CURRENT_LIST_DIR}/src/gui/*.h)

set(GAME2048GUI_CORE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/core/game2048.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/heuristics.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/random_ai.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/priority_ai.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/expectimax_ai.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/ntuple_network.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/ntuple_ai.cpp
)

file(GLOB GAME2048GUI_CORE_INCS
    ${CMAKE_CURRENT_LIST_DIR}/src/core/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/*.h
    ${CMAKE_CURRENT_LIST_DIR}/src/utils/*.h
)

# natID 4.x renamed MY_INC to NATID_SDK_INC
file(GLOB GAME2048GUI_INC_TD  ${NATID_SDK_INC}/td/*.h)
file(GLOB GAME2048GUI_INC_GUI ${NATID_SDK_INC}/gui/*.h)

add_executable(${GAME2048GUI_NAME}
    ${GAME2048GUI_INCS}
    ${GAME2048GUI_SOURCES}
    ${GAME2048GUI_CORE_SOURCES}
    ${GAME2048GUI_CORE_INCS}
    ${GAME2048GUI_INC_TD}
    ${GAME2048GUI_INC_GUI}
    ${GAME2048GUI_ICON}
)

source_group("inc"            FILES ${GAME2048GUI_INCS})
source_group("src"            FILES ${GAME2048GUI_SOURCES})
source_group("core\\src"      FILES ${GAME2048GUI_CORE_SOURCES})
source_group("core\\inc"      FILES ${GAME2048GUI_CORE_INCS})
source_group("inc\\td"        FILES ${GAME2048GUI_INC_TD})
source_group("inc\\gui"       FILES ${GAME2048GUI_INC_GUI})

target_include_directories(${GAME2048GUI_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# so the app and the trainer agree on where the learned weights live
target_compile_definitions(${GAME2048GUI_NAME} PRIVATE
    GAME2048_PROJECT_ROOT="${CMAKE_CURRENT_LIST_DIR}")

if(WIN32)
    set_source_files_properties(${GAME2048GUI_ICON} PROPERTIES
        COMPILE_FLAGS "/I\"${CMAKE_CURRENT_LIST_DIR}/res/appIcon\""
    )
endif()

target_link_libraries(${GAME2048GUI_NAME}
    debug ${MU_LIB_DEBUG} debug ${NATGUI_LIB_DEBUG}
    optimized ${MU_LIB_RELEASE} optimized ${NATGUI_LIB_RELEASE}
)

setTargetPropertiesForGUIApp(${GAME2048GUI_NAME} ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/Info.plist)
setAppIcon(${GAME2048GUI_NAME} ${CMAKE_CURRENT_LIST_DIR})
setIDEPropertiesForGUIExecutable(${GAME2048GUI_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${GAME2048GUI_NAME})

set(TRAIN_NAME train_ntuple)

add_executable(${TRAIN_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/train/train_ntuple.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/core/game2048.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/ntuple_network.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/ntuple_ai.cpp
    # Game2048::spawn_evil_tile() evaluates boards, so the rules pull this in
    ${CMAKE_CURRENT_LIST_DIR}/src/ai/heuristics.cpp
)

target_include_directories(${TRAIN_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(${TRAIN_NAME} PRIVATE
    GAME2048_PROJECT_ROOT="${CMAKE_CURRENT_LIST_DIR}")

source_group("src" FILES ${CMAKE_CURRENT_LIST_DIR}/src/train/train_ntuple.cpp)
