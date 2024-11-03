cmake_minimum_required(VERSION 3.10)
project(SQLite3SpatialiteExample)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SQLite3 and SpatiaLite libraries
find_package(SQLite3 REQUIRED)
find_library(SPATIALITE_LIBRARY NAMES spatialite REQUIRED)

# Executable
add_executable(sqlite3_spatialite_app main.cpp)

# Link SQLite3 and SpatiaLite
target_link_libraries(sqlite3_spatialite_app PRIVATE SQLite::SQLite3 ${SPATIALITE_LIBRARY})

# Include directories if needed (e.g., for SQLite3 and SpatiaLite headers)
target_include_directories(sqlite3_spatialite_app PRIVATE ${SQLite3_INCLUDE_DIRS})
