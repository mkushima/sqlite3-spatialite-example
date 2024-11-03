# SQLite3 Spatialite Example

A C++ project using SQLite3 and SpatiaLite to work with spatial data.

## Overview

This project demonstrates how to use SQLite3 and SpatiaLite together in a C++ application to manage spatial data. SQLite3 is a lightweight database engine, and SpatiaLite is a spatial extension that enables SQLite to handle spatial data.

## Prerequisites

To build this project, ensure the following libraries are installed:

- SQLite3: 

        sudo apt install sqlite3 libsqlite3-dev

- SpatiaLite:

        sudo apt install libspatialite-dev

## Usage

The program accepts command-line arguments to specify the example to run and the database file to use.

### Command-Line Options

- `-h`, `--help`: Show usage information.
- `-i`, `--example-id <id>`: Select which example to run. Options:
  - `1` for Example 1: Creates a new SpatiaLite database with tourist places in Brazil.
  - `2` for Example 2: Imports a shapefile and performs spatial queries.
- `-n`, `--db-name <name>`: Specify the database file name. If omitted, an in-memory database is used.

### Examples

Run Example 1 using an in-memory database:

```bash
./sqlite3_spatialite_app --example-id 1
```

Run Example 2 using a specified database file:

```bash
./sqlite3_spatialite_app --example-id 2 --db-name my_spatial_db.db
```

## Example Type

### Example 1: Creating a Spatial Database
- Connects to SQLite and initializes SpatiaLite.
- Creates a table to store tourist locations in Brazil.
- Adds geometry points for specific locations.

### Example 2: Importing Shapefile and Querying
- Imports a shapefile into the database (e.g., Brazilian states).
- Executes spatial queries to find states that correspond to specific geographic points.
