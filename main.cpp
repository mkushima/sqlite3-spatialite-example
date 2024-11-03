#include <iostream>
#include <vector>

#include <sqlite3.h>
#include <spatialite.h>

#include <getopt.h>


/**
 * Handle a SQLite error
 *
 * @param db_handle handle to the database connection
 * @param cache a memory pointer returned by spatialite_alloc_connection()
 * @param err_msg error message returned by SQLite
 */
void handle_error(sqlite3 *db_handle, void *cache, char *err_msg)
{
    std::cerr << "Error: " << err_msg << std::endl;
    sqlite3_free(err_msg);
    sqlite3_close(db_handle);
    spatialite_cleanup_ex(cache);
    spatialite_shutdown();
}

/**
 * Checks if the spatial_ref_sys table exists in the given database
 *
 * @param db_handle handle to the database connection
 * @return true if the table exists, false otherwise
 *
 * The spatial_ref_sys table is a special table that stores the spatial
 * reference systems supported by the database. This function checks if
 * the table exists in the given database.
 */
bool spatial_metadata_exists(sqlite3 *db_handle)
{
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(
        db_handle,
        "SELECT name FROM sqlite_master WHERE type='table' AND name='spatial_ref_sys'",
        -1,
        &stmt,
        NULL
    );

    if (result != SQLITE_OK) {
        std::cerr << "Error checking if spatial_ref_sys table exists: " << sqlite3_errmsg(db_handle) << std::endl;
        throw std::runtime_error("Error checking if spatial_ref_sys table exists");
    }

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
};

/**
 * Example 1: Creating a new SpatiaLite database and adding some
 * tourist places in Brazil to it.
 *
 * This example demonstrates how to create a new SpatiaLite database,
 * initialize the Spatialite library, create a table of points, add
 * a geometry column to the table, and add some tourist places in
 * Brazil to the table.
 *
 * @param db_name name of the database file to create
 * @return 0 if successful, 1 otherwise
 */
int run_example_1(std::string db_name)
{
    sqlite3 *db_handle;
    std::string table_name;
    std::string sql_cmd;
    int ret;
    char *err_msg = NULL;
    void *cache;

    // Show SQLite and Spatialite versions
    std::cout << "SQLite version: " << sqlite3_libversion() << std::endl;
    std::cout << "Spatialite version: " << spatialite_version() << std::endl;

    // Open a new database connection
    std::cout << "Opening database: " << db_name << std::endl;
    
    ret = sqlite3_open_v2(
        db_name.c_str(),
        &db_handle,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        NULL
    );

    if (ret != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db_handle) << std::endl;
        sqlite3_close(db_handle);
        return 1;
    }

    // Initialize the SpatiaLite connection
    cache = spatialite_alloc_connection();
    spatialite_init_ex(db_handle, cache, 0);

    // Check if spatial_ref_sys table exists
    if (! spatial_metadata_exists(db_handle)) {
        // Initialize the Spatialite library
        std::cout << "Initializing Spatialite..." << std::endl;

        sql_cmd = "SELECT InitSpatialMetaData(1);";
        ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

        if (ret != SQLITE_OK) {
            std::cerr << "Error initializing Spatialite: " << err_msg << std::endl;
            handle_error(db_handle, cache, err_msg);
            return 1;
        }
    }

    // Creating a table of points
    table_name = "points";
    std::cout << "Creating table: " << table_name << std::endl;
    
    sql_cmd = "CREATE TABLE IF NOT EXISTS " + table_name + " (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL)";
    ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

    if (ret != SQLITE_OK) {
        std::cerr << "Error creating table: " << err_msg << std::endl;
        handle_error(db_handle, cache, err_msg);
        return 1;
    }

    // Add a geometry column to the table
    std::cout << "Adding geometry column to table: " << table_name << std::endl;
    
    // Syntax for adding a geometry column
    // AddGeometryColumn('table_name', 'column_name', 'srid', 'geometry_type', 'dimension')
    // srid: the SRID of the geometry used in the table.
    //      Example: 4326 = WGS84
    // geometry_type: the geometry type of the column.
    //      Example: 'POINT, LINESTRING, POLYGON, MULTIPOINT, MULTILINESTRING, MULTIPOLYGON'
    // dimension: the dimension of the geometry. 
    //      Example: 
    //          'XY' or 2: 2D points
    //          'XYM: 2D points with M values
    //          'XYZ' or 3: 3D points
    //          'XYZM': 3D points with M values
    sql_cmd = "SELECT AddGeometryColumn('" + table_name + "', 'geom', 4326, 'POINT', 'XY')";
    ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

    if (ret != SQLITE_OK) {
        std::cerr << "Error adding geometry column: " << err_msg << std::endl;
        handle_error(db_handle, cache, err_msg);
        return 1;
    }

    // Adding some tourist places in Brazil
    // Note: SQLite engine is a transactional DB.
    // For performance reasons, we'll wrap multiple statements in a single transaction
    // and commit it all at once.
    std::cout << "Adding some tourist places in Brazil..." << std::endl;

    sql_cmd = "BEGIN TRANSACTION;";
    ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

    if (ret != SQLITE_OK)
    {
        std::cerr << "Error starting transaction: " << err_msg << std::endl;
        handle_error(db_handle, cache, err_msg);
        return 1;
    }

    const std::vector<std::pair<std::string, std::string>> places = {
        {"Rio de Janeiro", "POINT(-43.1729 -22.9068)"},
        {"Foz do Iguacu", "POINT(-54.5854 -25.5165)"},
        {"Fernando de Noronha", "POINT(-32.423786 -3.853808)"}
    };

    for (const auto& place : places) {
        std::cout << "Adding " << place.first << ": " << place.second << std::endl;
        sql_cmd = "INSERT INTO " + table_name + " (geom) VALUES (GeomFromText('" + place.second + "', 4326))";
        ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

        if (ret != SQLITE_OK) {
            std::cerr << "Error adding " << place.first << ": " << err_msg << std::endl;
            handle_error(db_handle, cache, err_msg);
            return 1;
        }
    }

    // Commit the transaction
    std::cout << "Committing transaction..." << std::endl;

    sql_cmd = "COMMIT";
    ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

    if (ret != SQLITE_OK) {
        std::cerr << "Error committing transaction: " << err_msg << std::endl;
        handle_error(db_handle, cache, err_msg);
        return 1;
    }

    // Close the database connection
    ret = sqlite3_close(db_handle);

    if (ret != SQLITE_OK) {
        std::cerr << "Error closing database: " << sqlite3_errmsg(db_handle) << std::endl;
    }

    // Shutdown the Spatialite library
    spatialite_cleanup_ex(cache);
    spatialite_shutdown();

    std::cout << "Example 1 Done." << std::endl;
    return 0;
}

/**
 * Example 2: Importing shapefile and performing spatial query
 * @param db_name Path to the SQLite database file
 * @return 0 on success, 1 on failure
 *
 * This example shows how to import a shapefile into a Spatialite database and
 * perform a spatial query to find the corresponding state name for given points.
 */
int run_example_2(std::string db_name)
{
    sqlite3 *db_handle;
    std::string table_name = "location";
    std::string sql_cmd;
    std::string shp_file_path = "../shp/BR_UF_2022";
    int ret;
    char *err_msg = NULL;
    void *cache;

    // Setting environment variable to allow calling ImportSHP function
    // Otherwise, it will throw an error
    setenv("SPATIALITE_SECURITY", "relaxed", 1);

    // Open a new database connection
    std::cout << "Opening database: " << db_name << std::endl;
    
    ret = sqlite3_open_v2(
        db_name.c_str(),
        &db_handle,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        NULL
    );

    if (ret != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db_handle) << std::endl;
        sqlite3_close(db_handle);
        return 1;
    }

    // Initialize the SpatiaLite connection
    cache = spatialite_alloc_connection();
    spatialite_init_ex(db_handle, cache, 0);

    // Check if spatial_ref_sys table exists
    if (! spatial_metadata_exists(db_handle)) {
        // Initialize the Spatialite library
        std::cout << "Initializing Spatialite..." << std::endl;

        sql_cmd = "SELECT InitSpatialMetaData(1);";
        ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

        if (ret != SQLITE_OK) {
            std::cerr << "Error initializing Spatialite: " << err_msg << std::endl;
            handle_error(db_handle, cache, err_msg);
            return 1;
        }
    }

    // Importing the shapefile
    std::cout << "Importing shapefile: " << shp_file_path << std::endl;

    sql_cmd = "SELECT ImportSHP('" + shp_file_path + "', '" + table_name + "', 'UTF-8')";
    std::cout << sql_cmd << std::endl;
    ret = sqlite3_exec(db_handle, sql_cmd.c_str(), NULL, NULL, &err_msg);

    if (ret != SQLITE_OK) {
        std::cerr << "Error importing shapefile: " << err_msg << std::endl;
        handle_error(db_handle, cache, err_msg);
        return 1;
    }

    // Checking what are the correspoding State names for the following points
    std::cout << "Checking what are the correspoding State names for the following points:" << std::endl;

    const std::vector<std::pair<std::string, std::string>> places = {
        {"Rio de Janeiro", "POINT(-43.1729 -22.9068)"},
        {"Foz do Iguacu", "POINT(-54.5854 -25.5165)"},
        {"Fernando de Noronha", "POINT(-32.423786 -3.853808)"},
        {"Null Island", "POINT(0 0)"},
        {"New York", "POINT(-74.0060 40.7128)"},
    };

    for (const auto& place : places) {
        sql_cmd = "SELECT NM_UF FROM " + table_name + " WHERE ST_Within(GeomFromText('" + place.second + "', 4326), geometry) =1";
        
        sqlite3_stmt *stmt;
        ret = sqlite3_prepare_v2(db_handle, sql_cmd.c_str(), -1, &stmt, NULL);
        
        if (ret != SQLITE_OK)
        {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(db_handle) << std::endl;
            sqlite3_close(db_handle);
            spatialite_cleanup_ex(cache);
            spatialite_shutdown();
            return 1;
        }

        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW) {
            std::cout << place.first << " ---> " << sqlite3_column_text(stmt, 0) << std::endl;
        } else {
            std::cout << place.first << " ---> " << "Not found" << std::endl;
        }

        sqlite3_finalize(stmt);
    }

    // Close the database connection
    ret = sqlite3_close(db_handle);

    if (ret != SQLITE_OK) {
        std::cerr << "Error closing database: " << sqlite3_errmsg(db_handle) << std::endl;
    }

    // Shutdown the Spatialite library
    spatialite_cleanup_ex(cache);
    spatialite_shutdown();

    std::cout << "Example 2 Done." << std::endl;
    return 0;
}

/**
 * Prints the usage message for the application.
 *
 * The usage message is written to standard output and includes the
 * command-line options available for the application.
 */
void show_usage()
{
    std::cout << "Usage: sqlite3_spatialite_app [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -i, --example-id <id>   ID of the example to run" << std::endl;
    std::cout << "  -n, --db-name <name>    Name of the database file (if not provided, in-memory)" << std::endl;
}

/**
 * The main entry point for the application.
 *
 * This function parses command-line arguments to determine the database
 * configuration and which example to run. It supports in-memory database
 * operations and allows users to choose between running example 1 or
 * example 2.
 *
 * Command-line options:
 *  -h, --help              Show the usage message and exit.
 *  -i, --example-id <id>   ID of the example to run (1 or 2).
 *  -n, --db-name <name>    Name of the database file to use. If not
 *                          provided, an in-memory database will be used.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Returns 0 if the chosen example runs successfully, or 1 if there
 *         is an error or if an unknown example ID is specified.
 */
int main (int argc, char *argv[])
{
    bool in_memory_db = true;
    std::string db_name;
    uint8_t example_id = 0;

    auto parse_args = [&]() {
        std::cout << "parsing arguments..." << std::endl;
        int option_index = 0;
        int c;

        const option long_options[] =
        {
            {"help", no_argument, nullptr, 'h'},
            {"example-id", required_argument, nullptr, 'i'},
            {"db-name", required_argument, nullptr, 'n'},

            {nullptr, 0, nullptr, 0}
        };

        while ((c = getopt_long(argc, argv, "i:n:h", long_options, &option_index)) != -1)
        {
            switch (c) {
                case 'i':
                    example_id = atoi(optarg);
                    break;
                case 'n':
                    db_name = optarg;
                    in_memory_db = false;
                    break;
                case 'h':
                case '?':
                    show_usage();
                    std::exit(0);
                default:
                    break;
            }
        }

        if (optind < argc) {
            std::cerr << "Unexpected argument: " << argv[optind] << std::endl;
            std::exit(1);
        }
    };

    parse_args();

    if (in_memory_db) {
        std::cout << "Using in-memory database" << std::endl;
        db_name = ":memory:";
    }
    
    switch (example_id) {
        case 1:
            std::cout << "Running example 1..." << std::endl;
            return run_example_1(db_name);
        case 2:
            std::cout << "Running example 2..." << std::endl;
            return run_example_2(db_name);
        default:
            std::cerr << "Unknown example ID: " << example_id << std::endl;
            return 1;
    }
}
