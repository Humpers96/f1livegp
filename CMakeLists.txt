cmake_minimum_required (VERSION 3.22)

project(f1livegp
  LANGUAGES CXX
  VERSION 0.1.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL REQUIRED)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
  GIT_TAG        v6.0.2
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)

# --- Fetch nlohmann/json ------------------------------------------------------
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)
# ------------------------------------------------------------------------------

add_executable(f1livegp src/main.cpp)
target_include_directories(f1livegp PRIVATE src)

target_link_libraries(f1livegp
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
  PRIVATE CURL::libcurl
  nlohmann_json::nlohmann_json
)