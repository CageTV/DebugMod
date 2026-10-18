# DebugMenu.dll on alandtse's CommonLibSSE-NG (Skyrim 1.6.1170 and 1.7.x, SE and AE); included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there).
# The repository's own CMakeLists.txt is the older build (CharmedBaryon's CommonLibSSE-NG through $ENV{CommonLibSSEPath_NG}) for Skyrim VR and 1.6.1130 and older.

# Version.h comes from cmake/Version.h.in, the same way the repository's own build makes it
file(READ "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" _dm_cmakelists)
if(NOT _dm_cmakelists MATCHES "set\\(VERSION ([0-9]+)\\.([0-9]+)\\.([0-9]+)\\)")
    message(FATAL_ERROR "DebugMod ng.cmake: no set(VERSION x.y.z) in CMakeLists.txt")
endif()
set(DM_MAJOR ${CMAKE_MATCH_1})
set(DM_MINOR ${CMAKE_MATCH_2})
set(DM_PATCH ${CMAKE_MATCH_3})
set(_dm_ver "${DM_MAJOR}.${DM_MINOR}.${DM_PATCH}")
set(_dm_saved_project_name "${PROJECT_NAME}")
set(PROJECT_NAME "DebugMenu")
set(PROJECT_VERSION_MAJOR ${DM_MAJOR})
set(PROJECT_VERSION_MINOR ${DM_MINOR})
set(PROJECT_VERSION_PATCH ${DM_PATCH})
set(_dm_gen "${CMAKE_BINARY_DIR}/debugmenu-gen/include")
configure_file("${CMAKE_CURRENT_LIST_DIR}/cmake/Version.h.in" "${_dm_gen}/Version.h" @ONLY)
set(PROJECT_NAME "${_dm_saved_project_name}")

# sources: cmake/sourcelist.cmake lists them relative to the repository root
set(sources "")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/sourcelist.cmake")
# the upstream author keeps an unreleased PrismaUI interface out of git (src/Interface_prismaUI is in .gitignore) but still lists it, so skip files a clean checkout lacks
set(_dm_present "")
foreach(_dm_src IN LISTS sources)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_dm_src}")
        list(APPEND _dm_present "${_dm_src}")
    else()
        message(STATUS "DebugMenu ng: skipping missing source ${_dm_src}")
    endif()
endforeach()
set(sources ${_dm_present})

find_package(polyhook_2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(DETOURS_INCLUDE_DIRS "detours/detours.h")
find_library(DETOURS_LIBRARY detours REQUIRED)

ng_plugin(TARGET DebugMenu NAME DebugMenu VERSION ${_dm_ver} AUTHOR "Spodermon" ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES ${sources}
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/src" "${_dm_gen}" "${DETOURS_INCLUDE_DIRS}"
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h"
    DEFINES DEBUGMENU_NG
    LIBS PolyHook_2::PolyHook_2 glm::glm nlohmann_json::nlohmann_json "${DETOURS_LIBRARY}")
