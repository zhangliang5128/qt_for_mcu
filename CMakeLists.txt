#! [Vendor CMakeLists]
include(CMakeDependentOption)
string(TOLOWER ${QUL_PLATFORM} board_name)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${board_name})
#! [Vendor CMakeLists]
