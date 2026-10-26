# Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders
  ${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/gamePCH.h)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(game
  ${PRIVATE_SOURCES}
)

# ---------------------------------------------------------------------------
# Eluna 7.3.5
# CollectSourceFiles 只收集 .cpp，Lua 5.1 的 C 源码需要单独编译。
# 不能直接加进 game：game 启用了 C++ PCH，C 源码会触发 STL1003。
# ---------------------------------------------------------------------------
file(GLOB ELUNA_LUA_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src/*.c")

list(REMOVE_ITEM ELUNA_LUA_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src/lua.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src/luac.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src/print.c")

add_library(eluna_lua STATIC
  ${ELUNA_LUA_SOURCES})

set_target_properties(eluna_lua PROPERTIES
  LINKER_LANGUAGE C)

target_include_directories(eluna_lua PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna"
  "${CMAKE_CURRENT_SOURCE_DIR}/Eluna/lua_src")

target_compile_definitions(eluna_lua PRIVATE
  LUA_COMPAT_MODULE)

target_compile_definitions(game PRIVATE
  ELUNA_TRINITY
  ELUNA_EXPANSION=6
  LUA_COMPAT_MODULE)

target_link_libraries(game
  PUBLIC
    eluna_lua)

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(game
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(game
  PRIVATE
    trinity-core-interface
    efsw
  PUBLIC
    shared
    Detour)

set_target_properties(game
    PROPERTIES
      DEFINE_SYMBOL TRINITY_API_EXPORT_GAME
      FOLDER "server")

if( BUILD_SHARED_LIBS )
  if( UNIX )
    install(TARGETS game
      LIBRARY
        DESTINATION lib)
  elseif( WIN32 )
    install(TARGETS game
      RUNTIME
        DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endif()

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(game ${PRIVATE_PCH_HEADER})
endif ()
