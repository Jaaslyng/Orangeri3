#ifndef DATABASE_H
#define DATABASE_H


#include </home/jesper/mariadb-connector-c-3.2.3/include/mysql.h>
#include <string>
#include <spdlog/spdlog.h>

// -----------------------------
//  Type helper (PLACERES HER)
// -----------------------------
template<typename T>
struct is_supported_type : std::false_type {};

template<> struct is_supported_type<int> : std::true_type {};
template<> struct is_supported_type<double> : std::true_type {};
template<> struct is_supported_type<std::string> : std::true_type {};


// -----------------------------
//  Database class
// -----------------------------

class Database
{
public:
    // Constructor and Destructor
    Database();
    ~Database();

    // Public Member Functions
    bool IniSqlConnection(std::string& sqlError);
    void CloseSqlConnection();
    void finish_with_error(MYSQL* conn);


    template<typename T>
    std::string toSqlValue(const T& value)
    {
        if constexpr (std::is_same_v<T, std::string>)
        {
            return "'" + value + "'";
        }
        else
        {
            return std::to_string(value);
        }
    }


    template<typename T>

    bool Update_CurrentValue(const std::string& parameter,
                             const T& value,
                             std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_CurrentValue only supports int, float, double, and std::string");

        std::string sqlValue = toSqlValue(value);
        std::string sqlParam = "'" + parameter + "'";
        spdlog::debug("sqlParam: {} sqlValue: {}", sqlParam, sqlValue);
        std::string query =
          //  "INSERT INTO CurrentValues (Parameter, Value) VALUES (" + sqlParam + ", " + sqlValue + ") "
            "INSERT INTO CurrentValues (Parameter, Value) VALUES (" + sqlParam + "," + sqlValue + ") "
            "ON DUPLICATE KEY UPDATE Value = VALUES(Value)";

        spdlog::debug("Executing query 1: {}", query);
        spdlog::debug("Executing query 2: {}", query.c_str());
        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL Error Executing query: {}", query);
            spdlog::error("SQL Error: {}", sqlError);
            return false;
        }

        return true;
    }


    template<typename T>
    bool Update_HistoryValue(const std::string& parameter,
                             const T& value,
                             std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_HistoryValue only supports int, float, double, and std::string");

        std::string sqlValue = toSqlValue(value);
        std::string sqlParam = "'" + parameter + "'";
        std::string sqlTime  = "'" + getCurrentTime() + "'";

        std::string query =
            "INSERT INTO HistoryValues (Time, Parameter, Value) "
            "VALUES (" + sqlTime + ", " + sqlParam + ", " + sqlValue + ");";

        spdlog::debug("Executing query: {}", query);

        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL Error Executing query: {}", query);
            spdlog::error("SQL Error: {}", sqlError);
            return false;
        }

        return true;
    }

 template<typename T>
 bool AddEventRecord(const std::string& type,
                     const std::string& description,
                     const T& value,
                      std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_Event only supports int, float, double, and std::string");

        std::string sqlValue = toSqlValue(value);
        std::string sqlType = "'" + type + "'";
        std::string sqlDesc = "'" + description + "'";
        std::string sqlTime  = "'" + getCurrentTime() + "'";

        std::string query =
            "INSERT INTO Events (Date, Time, Type, Description, Value) "
            "VALUES (" + sqlTime + ", " + sqlTime + ", " + sqlType + ", " + sqlDesc + ", " +sqlValue + ");";

        spdlog::debug("Executing query: {}", query);

        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL Error Executing query: {}", query);
            spdlog::error("SQL Error: {}", sqlError);
            return false;
        }

        return true;
    }



    template<typename T>
    bool Get_Setpoint(const std::string& parameter,
                      T& outValue,
                      const T& defaultValue,
                      std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Get_Setpoint only supports int, double, and std::string");

        bool success = true;
        outValue = defaultValue;

        std::string query =
            "SELECT SetpointValue FROM Setpoints WHERE Parameter = '" + parameter + "'";

        spdlog::debug("Executing query: {}", query);

        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL query error: {}", sqlError);
            return false;
        }

        MYSQL_RES* res = mysql_store_result(conn);
        if (!res)
        {
            sqlError = mysql_error(conn);
            spdlog::error("mysql_store_result error: {}", sqlError);
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);

        if (!row)
        {
            sqlError = "Parameter not found, using default value";
            spdlog::warn("Parameter '{}' not found, using default", parameter);
            mysql_free_result(res);
            return true;
        }

        if (!row[0])
        {
            sqlError = "NULL value for parameter: " + parameter;
            spdlog::warn("Parameter '{}' has NULL value, using default", parameter);
            mysql_free_result(res);
            return true;
        }

        try
        {
            if constexpr (std::is_same_v<T, int>)
            {
                outValue = std::stoi(row[0]);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                outValue = std::stod(row[0]);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                outValue = row[0];
            }
        }
        catch (...)
        {
            sqlError = "Invalid value format for parameter: " + parameter;
            spdlog::error("Invalid value for '{}': {}", parameter, row[0]);
            success = false;
        }

        mysql_free_result(res);
        return success;
    }

    template<typename T>
    bool Update_Setpoint(const std::string& parameter,
                         const T& value,
                         std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_Setpoint only supports int, double, float, and std::string");

        std::string sqlValue = toSqlValue(value);
        std::string sqlParam = "'" + parameter + "'";

        std::string query =
            "INSERT INTO Setpoints (Parameter, SetpointValue) VALUES (" + sqlParam + ", " + sqlValue + ") "
            "ON DUPLICATE KEY UPDATE SetpointValue = VALUES(SetpointValue);";

        spdlog::debug("Executing query: {}", query);

        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL Error Executing query: {}", query);
            spdlog::error("SQL Error: {}", sqlError);
            return false;
        }

        return true;
    }
//Generisk funktion til læsning af sætpunkter

    template<typename T>
    bool Get_CurrentValue(const std::string& parameter,
                          T& outValue,
                          std::string& updateTime,
                          std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Get_CurrentValue only supports int, double, and std::string");

        bool success = true;

        outValue = T{};
        updateTime.clear();

        std::string query =
            "SELECT Value, UpdateTime FROM CurrentValues WHERE Parameter = '" + parameter + "'";

        spdlog::debug("Executing query: {}", query);

        if (mysql_query(conn, query.c_str()))
        {
            sqlError = mysql_error(conn);
            spdlog::error("SQL query error: {}", sqlError);
            return false;
        }

        MYSQL_RES* res = mysql_store_result(conn);
        if (!res)
        {
            sqlError = mysql_error(conn);
            spdlog::error("mysql_store_result error: {}", sqlError);
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);

        if (!row)
        {
            sqlError = "No record found for parameter: " + parameter;
            spdlog::warn("No record found for '{}'", parameter);
            mysql_free_result(res);
            return false;
        }

        // -----------------------------
        // Value column
        // -----------------------------
        if (row[0])
        {
            try
            {
                if constexpr (std::is_same_v<T, int>)
                {
                    outValue = std::stoi(row[0]);
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    outValue = std::stod(row[0]);
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    outValue = row[0];
                }
            }
            catch (...)
            {
                sqlError = "Invalid value format for parameter: " + parameter;
                spdlog::error("Invalid value for '{}': {}", parameter, row[0]);
                success = false;
            }
        }
        else
        {
            sqlError = "NULL value for parameter: " + parameter;
            spdlog::warn("Parameter '{}' has NULL value", parameter);
            success = false;
        }

        // -----------------------------
        // UpdateTime column
        // -----------------------------
        if (row[1])
        {
            updateTime = row[1];
        }
        else
        {
            sqlError = "NULL UpdateTime for parameter: " + parameter;
            spdlog::warn("Parameter '{}' has NULL UpdateTime", parameter);
            success = false;
        }

        mysql_free_result(res);
        return success;
    }






    /*    template<typename T>
        bool Get_CurrentValue(const std::string& parameter,
                              T& outValue,
                              std::string& updateTime,
                              std::string& sqlError);

    */
    /*bool Get_CurrentValue_string(const std::string& parameter, std::string& value, std::string& updateTime, std::string& sqlError);
    bool Get_CurrentValue_double(const std::string& parameter, double& value, std::string& updateTime, std::string& sqlError);
    bool Get_CurrentValue_int(const std::string& parameter,
                              int& value,
                              std::string& updateTime,
                              std::string& sqlError);
    void Update_CurrentValues_string(const std::string& parameter, std::string value);
    void Update_CurrentValues_float(const std::string& parameter, float myvalue);
    void Update_HistoryValues_float(const std::string& parameter, float value);


    */
    void Update_Setpoints_double(const std::string& parameter, double value);
    void Update_Setpoints_int(const std::string& parameter, int value);

    /* bool Get_Setpoint_double(const std::string& parameter, double& mySetpoint, double defaultValue, std::string& sqlError);
     bool Get_Setpoint_int(const std::string& parameter, int& mySetpoint, int defaultValue, std::string& sqlError);
     bool Get_Setpoint_string(const std::string& parameter,
                                    std::string& mySetpoint,
                                    const std::string& defaultValue,
                                    std::string& sqlError);*/


private:
    // Member Variables
    MYSQL* conn;
    MYSQL_RES* res;
    MYSQL_ROW row;

    const char* server;
    const char* user;
    const char* password;
    const char* database;
    std::string getCurrentTime();

};

#endif // DATABASE_H

