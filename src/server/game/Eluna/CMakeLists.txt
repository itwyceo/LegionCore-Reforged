# ---------------------------------------------------------------------------
# Eluna 7.3.5（LegionCore-Reforged）
#
# 独立构建用；如果直接放入核心 src/server/game/Eluna，请按
# ../ELUNA_INTEGRATION.md 的说明改核心 CMake，而不是使用本文件。
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.18)
project(eluna735 CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ELUNA_CPP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

file(GLOB ELUNA_LUA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lua_src/*.c")

list(REMOVE_ITEM ELUNA_LUA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lua_src/lua.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua_src/luac.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua_src/print.c")

add_library(eluna735 STATIC
    ${ELUNA_CPP_SOURCES}
    ${ELUNA_LUA_SOURCES}
)

target_include_directories(eluna735 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${ELUNA_CORE_INCLUDE_DIRS}
)

target_compile_definitions(eluna735 PUBLIC
    ELUNA_TRINITY
    ELUNA_EXPANSION=6
    LUA_COMPAT_MODULE
)

target_link_libraries(eluna735 PUBLIC
    ${ELUNA_CORE_LIBS}
    ${ELUNA_BOOST_LIBS}
)
