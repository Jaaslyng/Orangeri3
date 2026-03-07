#include "Database.h"
#include <iostream>
#include <ctime>
#include <spdlog/spdlog.h>

/*
int mysql_query(MYSQL * mysql,
                const char * query);

Returns zero on success, non zero on failure
*/


// Constructor
Database::Database()
    : conn(nullptr), res(nullptr), row(nullptr),
      server("localhost"), user("jesper"),
      password("Gartnerkrogen69!"), database("Orangeri") {}

// Destructor
Database::~Database()
{
    CloseSqlConnection();
}


std::string Database::getCurrentTime()
{
    time_t now = time(nullptr);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

// Initialize SQL Connection
bool Database::IniSqlConnection(std::string& sqlError)
{
    conn = mysql_init(nullptr);

    if (conn == nullptr)
    {
        sqlError = "mysql_init() failed";
        return false;
    }

    if (mysql_real_connect(conn, server, user, password, database, 0, nullptr, 0) == nullptr)
    {
        sqlError = mysql_error(conn);
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    return true;
}

// Close SQL Connection
void Database::CloseSqlConnection()
{
    if (res != nullptr)
    {
        mysql_free_result(res);
        res = nullptr;
    }

    if (conn != nullptr)
    {
        mysql_close(conn);
        conn = nullptr;
    }
}

// Handle Errors
void Database::finish_with_error(MYSQL* conn)
{
    if (conn)
    {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        spdlog::error(std::string("SQL Error: ") + mysql_error(conn));
        mysql_close(conn);
    }
    //exit(EXIT_FAILURE);
}

std::string addQuotesIfNeeded(const std::string& value)
{
    if (value.find('"') == std::string::npos)
    {
        return '"' + value + '"';
    }
    return value;
}


