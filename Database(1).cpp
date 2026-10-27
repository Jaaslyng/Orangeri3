#include "Database.h"
#include <ctime>
#include <iostream>
#include <spdlog/spdlog.h>

Database::Database()
    : conn(nullptr),
      server("localhost"), user("jesper"),
      password("Gartnerkrogen69!"), database("Orangeri") {}

Database::~Database()
{
    CloseSqlConnection();
}

// -----------------------------
//  Forbindelse
// -----------------------------

bool Database::TestConnection()
{
    if (conn == nullptr)
    {
        std::string err;
        return IniSqlConnection(err);
    }

    if (mysql_ping(conn) != 0)
    {
        spdlog::warn("Lost MySQL connection, reconnecting");
        mysql_close(conn);

        std::string err;
        if (!IniSqlConnection(err))
        {
            spdlog::error("Reconnect failed: {}", err);
            return false;
        }
    }
    return true;
}

bool Database::IniSqlConnection(std::string& sqlError)
{
    conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        sqlError = "mysql_init() failed";
        return false;
    }

    unsigned int timeout = 5;
    if (mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) != 0 ||
        mysql_options(conn, MYSQL_OPT_READ_TIMEOUT,    &timeout) != 0 ||
        mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT,   &timeout) != 0)
    {
        sqlError = mysql_error(conn);
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    if (mysql_real_connect(conn, server, user, password,
                           database, 0, nullptr, 0) == nullptr)
    {
        sqlError = mysql_error(conn);
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    spdlog::info("Database connection established");
    return true;
}

void Database::CloseSqlConnection()
{
    if (conn != nullptr)
    {
        mysql_close(conn);
        conn = nullptr;
    }
}

void Database::finish_with_error(MYSQL* conn)
{
    if (conn)
    {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        spdlog::error("SQL Error: {}", mysql_error(conn));
        mysql_close(conn);
    }
}

// -----------------------------
//  Query med retry ved tabt forbindelse
// -----------------------------

bool Database::ExecuteQuery(const std::string& query, std::string& sqlError)
{
    for (int attempt = 1; attempt <= 2; ++attempt)
    {
        if (!TestConnection())
        {
            sqlError = "No connection";
            return false;
        }

        if (mysql_query(conn, query.c_str()) == 0)
            return true;

        const unsigned int err = mysql_errno(conn);
        if (err == CR_SERVER_GONE || err == CR_SERVER_LOST)
        {
            spdlog::warn("Connection lost ({}), attempt {}", err, attempt);
            mysql_close(conn);
            conn = nullptr;
            continue;
        }

        sqlError = mysql_error(conn);
        spdlog::error("SQL error executing query: {}", query);
        spdlog::error("SQL error: {}", sqlError);
        return false;
    }

    sqlError = "Retry failed (connection lost)";
    spdlog::error("SQL error executing query: {}", query);
    return false;
}

bool Database::ExecuteSelect(const std::string& query,
                             MYSQL_RES*& outRes,
                             std::string& sqlError)
{
    outRes = nullptr;

    for (int attempt = 1; attempt <= 2; ++attempt)
    {
        if (!TestConnection())
        {
            sqlError = "No connection";
            return false;
        }

        if (mysql_query(conn, query.c_str()) == 0)
        {
            outRes = mysql_store_result(conn);
            if (outRes != nullptr)
                return true;

            sqlError = mysql_error(conn);
            spdlog::error("mysql_store_result error: {}", sqlError);
            return false;
        }

        const unsigned int err = mysql_errno(conn);
        if (err == CR_SERVER_GONE || err == CR_SERVER_LOST)
        {
            spdlog::warn("Connection lost ({}), attempt {}", err, attempt);
            mysql_close(conn);
            conn = nullptr;
            continue;
        }

        sqlError = mysql_error(conn);
        spdlog::error("SQL query error: {}", sqlError);
        return false;
    }

    sqlError = "Retry failed (connection lost)";
    return false;
}

// -----------------------------
//  Hjælpere
// -----------------------------

std::string Database::getCurrentTime()
{
    time_t now = time(nullptr);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

std::string Database::sqlString(const std::string& value)
{
    // mysql_real_escape_string kræver et gyldigt connection handle
    // (den afhænger af connection charset). Sørg for at have ét.
    if (conn == nullptr)
    {
        std::string err;
        if (!IniSqlConnection(err))
            return "''";   // kan ikke escape uden forbindelse
    }

    // 2*N + 1 er den anbefalede bufferstørrelse ifølge dokumentationen
    std::string escaped;
    escaped.resize(value.size() * 2 + 1);

    const unsigned long len =
        mysql_real_escape_string(conn, escaped.data(), value.c_str(), value.size());

    escaped.resize(len);
    return "'" + escaped + "'";
}

// -----------------------------
//  Setpoints (skal lagres som tal)
// -----------------------------

void Database::Update_Setpoints_double(const std::string& parameter, double value)
{
    std::string sqlError;
    if (!Update_Setpoint(parameter, value, sqlError))
        spdlog::error("Update_Setpoints_double('{}') failed: {}", parameter, sqlError);
}

void Database::Update_Setpoints_int(const std::string& parameter, int value)
{
    std::string sqlError;
    if (!Update_Setpoint(parameter, value, sqlError))
        spdlog::error("Update_Setpoints_int('{}') failed: {}", parameter, sqlError);
}