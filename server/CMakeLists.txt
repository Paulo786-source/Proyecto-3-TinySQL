cmake_minimum_required(VERSION 3.20)
project(TinySQLDb VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Directorios de includes compartidos
include_directories(${CMAKE_SOURCE_DIR}/include)

# ─── Stored Data Manager ───────────────────────────────────────────────────
add_library(storeddatamanager
    storeddatamanager/StoredDataManager.cpp
)
target_include_directories(storeddatamanager PUBLIC include)

# ─── Query Processor ───────────────────────────────────────────────────────
add_library(queryprocessor
    queryprocessor/QueryProcessor.cpp
)
target_include_directories(queryprocessor PUBLIC include)
target_link_libraries(queryprocessor storeddatamanager)

# ─── Web API ───────────────────────────────────────────────────────────────
add_executable(webapi
    webapi/main.cpp
)
target_include_directories(webapi PUBLIC include)
target_link_libraries(webapi queryprocessor storeddatamanager)