cmake_minimum_required(VERSION 3.20)
project(TinySQLDb VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# warnings para detectar malas practicas
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# dependencias externas
include(FetchContent)

# nlohmann/json - header-only, se descarga al hacer cmake ..
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/include
)
FetchContent_MakeAvailable(nlohmann_json)

# directorios de includes compartidos
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

# STORED DATA MANAGER
# systemcatalog.cpp se agrega en fase 2
add_library(storeddatamanager
    storeddatamanager/StoredDataManager.cpp
    storeddatamanager/SystemCatalog.cpp
)
target_include_directories(storeddatamanager PUBLIC include)

# QUERY PROCESSOR
add_library(queryprocessor
    queryprocessor/QueryProcessor.cpp
)
target_include_directories(queryprocessor PUBLIC include)
target_link_libraries(queryprocessor storeddatamanager)

# WEB API
add_executable(webapi
    webapi/main.cpp
)
target_include_directories(webapi PUBLIC include)
target_link_libraries(webapi queryprocessor storeddatamanager)