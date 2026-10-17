cmake_minimum_required(VERSION 3.20)
project(MuNaveiaGameServer CXX C)

# Build alternativo ao GameServer.sln. No Windows o .sln continua valido.
# Uso (Linux):  cmake -S . -B build -DGAMESERVER_TYPE=0 && cmake --build build -j

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Mesmas definicoes das configuracoes VoxMu (0) e VoxMu_CS (1) do .sln
set(GAMESERVER_TYPE 0 CACHE STRING "0 = VoxMu, 1 = VoxMu_CS (Castle Siege)")
set(GAMESERVER_UPDATE 505 CACHE STRING "Versao do protocolo")
set(GAMESERVER_LANGUAGE 1 CACHE STRING "Idioma")
option(MUNAVEIA_SYNTAX_ONLY "Apenas checar sintaxe (nao gera binario)" OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/GameServer)
file(GLOB GS_SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

if(NOT WIN32)
  # Arquivos que dependem da interface grafica / dump do Windows e ainda
  # nao tem equivalente portavel (ver README-CROSSPLATFORM.md).
  list(FILTER GS_SOURCES EXCLUDE REGEX ".*/(GameServer|ServerDisplayer|MiniDump|Tradebkp)\\.cpp$")
endif()

add_library(gameserver_obj OBJECT ${GS_SOURCES})
target_include_directories(gameserver_obj PUBLIC
  ${SRC_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/include/luajit)
target_compile_definitions(gameserver_obj PUBLIC
  CLIENT_VERSION=1
  GAMESERVER_TYPE=${GAMESERVER_TYPE}
  GAMESERVER_UPDATE=${GAMESERVER_UPDATE}
  GAMESERVER_LANGUAGE=${GAMESERVER_LANGUAGE})

if(WIN32)
  target_compile_definitions(gameserver_obj PUBLIC WIN32 _WINDOWS)
else()
  target_compile_options(gameserver_obj PRIVATE -fpermissive -w -include ${SRC_DIR}/PlatformCompat.h)
endif()

if(NOT MUNAVEIA_SYNTAX_ONLY)
  add_executable(GameServer $<TARGET_OBJECTS:gameserver_obj>)
  if(WIN32)
    target_link_libraries(GameServer PRIVATE ws2_32 winmm)
  else()
    find_package(Threads REQUIRED)
    target_link_libraries(GameServer PRIVATE Threads::Threads)
  endif()
endif()
