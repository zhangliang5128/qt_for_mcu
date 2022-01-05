#! [Example LinkerScriptLoader.cmake]
SET(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/x9-platform.ld)
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${LINKER_SCRIPT_OPTION} ${LINKER_SCRIPT}")
#! [Example LinkerScriptLoader.cmake]
