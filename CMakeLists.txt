cmake_minimum_required(VERSION 3.24)
project(tower_defense_mini LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.0
  GIT_SHALLOW ON
)

FetchContent_Declare(
  EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.13.2
  GIT_SHALLOW ON
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW ON
)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SFML EnTT nlohmann_json)

# GameState/WaveDirector live under src/game/ but, like everything in src/core/,
# depend only on entt/nlohmann-json, not SFML -- included here (rather than only in
# the tower_defense_mini executable below) so tests/ can drive WaveDirector directly
# instead of just its pure-math helpers.
add_library(td_mini_core
  src/core/Grid.cpp
  src/core/FlowField.cpp
  src/core/Config.cpp
  src/core/Movement.cpp
  src/core/WaveEscalation.cpp
  src/core/TowerUpgradeMath.cpp
  src/game/GameState.cpp
  src/game/WaveDirector.cpp
)
target_include_directories(td_mini_core PUBLIC src)
target_link_libraries(td_mini_core PUBLIC EnTT::EnTT nlohmann_json::nlohmann_json)
target_compile_definitions(td_mini_core PUBLIC
  TD_MINI_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

add_executable(tower_defense_mini
  src/main.cpp
  src/game/PlacementController.cpp
  src/game/TowerUpgrades.cpp
  src/game/CombatSystem.cpp
  src/game/MovementSystem.cpp
  src/game/RenderSystem.cpp
  src/game/Game.cpp
)
target_link_libraries(tower_defense_mini PRIVATE td_mini_core SFML::Graphics SFML::Window SFML::System)
target_compile_definitions(tower_defense_mini PRIVATE
  TD_MINI_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

enable_testing()
add_subdirectory(tests)
