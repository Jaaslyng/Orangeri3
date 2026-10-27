#ifndef DATABASE_H
#define DATABASE_H

#include <mysql.h>
#include <string>
#include <type_traits>

// MariaDB/MySQL fejkoder for tabt forbindelse
#define CR_SERVER_GONE 2006
#define CR_SERVER_LOST 2013

// Kun int, double og std::string er understøttet i DB-metoderne
template<typename T>
struct is_supported_type : std::false_type {};

template<> struct is_supported_type<int>          : std::true_type {};
template<> struct is_supported_type<double>       : std::true_type {};
template<> struct is_supported_type<std::string> : std::true_type {};

class Database
{
public:
    Database();
    ~Database();

    // Forbindelse
    bool IniSqlConnection(std::string& sqlError);
    void CloseSqlConnection();
    bool TestConnection();
    void finish_with_error(MYSQL* conn);

    // Skriv/læs (ikke-skabelon)
    void Update_Setpoints_double(const std::string& parameter, double value);
    void Update_Setpoints_int(const std::string& parameter, int value);

    bool ExecuteQuery(const std::string& query, std::string& sqlError);

    // Caller får ejerskab af *outRes og skal kalde mysql_free_result()
    bool ExecuteSelect(const std::string& query,
                       MYSQL_RES*& outRes,
                       std::string& sqlError);

    // ---- Skabelonmetoder: SKAL ligge i headeren ----

    template<typename T>
    bool Update_CurrentValue(const std::string& parameter,
                              const T& value,
                              std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_CurrentValue only supports int, double and std::string");

        const std::string query =
            "INSERT INTO CurrentValues (Parameter, Value) "
            "VALUES (" + sqlString(parameter) + ", " + toSqlValue(value) + ") "
            "ON DUPLICATE KEY UPDATE Value = VALUES(Value)";

        return ExecuteQuery(query, sqlError);
    }

    template<typename T>
    bool Update_HistoryValue(const std::string& parameter,
                             const T& value,
                             std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_HistoryValue only supports int, double and std::string");

        const std::string query =
            "INSERT INTO HistoryValues (Time, Parameter, Value) "
            "VALUES ('" + getCurrentTime() + "', " + sqlString(parameter) + ", " +
            toSqlValue(value) + ")";

        return ExecuteQuery(query, sqlError);
    }

    template<typename T>
    bool AddEventRecord(const std::string& type,
                        const std::string& description,
                        const T& value,
                        std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "AddEventRecord only supports int, double and std::string");

        const std::string time = getCurrentTime();
        const std::string query =
            "INSERT INTO Events (Date, Time, Type, Description, Value) "
            "VALUES ('" + time + "', '" + time + "', " +
            sqlString(type) + ", " + sqlString(description) + ", " +
            toSqlValue(value) + ")";

        return ExecuteQuery(query, sqlError);
    }

    template<typename T>
    bool Update_Setpoint(const std::string& parameter,
                         const T& value,
                         std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Update_Setpoint only supports int, double and std::string");

        const std::string query =
            "INSERT INTO Setpoints (Parameter, SetpointValue) "
            "VALUES (" + sqlString(parameter) + ", " + toSqlValue(value) + ") "
            "ON DUPLICATE KEY UPDATE SetpointValue = VALUES(SetpointValue)";

        return ExecuteQuery(query, sqlError);
    }

    template<typename T>
    bool Get_Setpoint(const std::string& parameter,
                      T& outValue,
                      const T& defaultValue,
                      std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Get_Setpoint only supports int, double and std::string");

        outValue = defaultValue;

        const std::string query =
            "SELECT SetpointValue FROM Setpoints WHERE Parameter = " + sqlString(parameter);

        MYSQL_RES* res = nullptr;
        if (!ExecuteSelect(query, res, sqlError))
            return false;

        MYSQL_ROW row = mysql_fetch_row(res);

        if (!row)
        {
            sqlError = "Parameter not found, using default value";
            mysql_free_result(res);
            return true;
        }

        if (!row[0])
        {
            sqlError = "NULL value for parameter: " + parameter;
            mysql_free_result(res);
            return true;
        }

        bool success = true;
        try
        {
            if constexpr (std::is_same_v<T, int>)
                outValue = std::stoi(row[0]);
            else if constexpr (std::is_same_v<T, double>)
                outValue = std::stod(row[0]);
            else if constexpr (std::is_same_v<T, std::string>)
                outValue = row[0];
        }
        catch (const std::exception&)
        {
            sqlError = "Invalid value format for parameter: " + parameter;
            success = false;
        }

        mysql_free_result(res);
        return success;
    }

    template<typename T>
    bool Get_CurrentValue(const std::string& parameter,
                          T& outValue,
                          std::string& updateTime,
                          std::string& sqlError)
    {
        static_assert(is_supported_type<T>::value,
                      "Get_CurrentValue only supports int, double and std::string");

        outValue = T{};
        updateTime.clear();

        const std::string query =
            "SELECT Value, UpdateTime FROM CurrentValues WHERE Parameter = " + sqlString(parameter);

        MYSQL_RES* res = nullptr;
        if (!ExecuteSelect(query, res, sqlError))
            return false;

        MYSQL_ROW row = mysql_fetch_row(res);

        if (!row)
        {
            sqlError = "No record found for parameter: " + parameter;
            mysql_free_result(res);
            return false;
        }

        bool success = true;

        if (row[0])
        {
            try
            {
                if constexpr (std::is_same_v<T, int>)
                    outValue = std::stoi(row[0]);
                else if constexpr (std::is_same_v<T, double>)
                    outValue = std::stod(row[0]);
                else if constexpr (std::is_same_v<T, std::string>)
                    outValue = row[0];
            }
            catch (const std::exception&)
            {
                sqlError = "Invalid value format for parameter: " + parameter;
                success = false;
            }
        }
        else
        {
            sqlError = "NULL value for parameter: " + parameter;
            success = false;
        }

        if (row[1])
            updateTime = row[1];
        else
        {
            sqlError = "NULL UpdateTime for parameter: " + parameter;
            success = false;
        }

        mysql_free_result(res);
        return success;
    }

private:
    MYSQL* conn;

    const char* server;
    const char* user;
    const char* password;
    const char* database;

    std::string getCurrentTime();

    // Escaper ' \ og kontroltegn. Returnerer værdien MED omklamrende '.
    // Kalder IniSqlConnection hvis der ikke er en aktiv forbindelse,
    // da mysql_real_escape_string kræver et connection handle (charset).
    std::string sqlString(const std::string& value);

    template<typename T>
    std::string toSqlValue(const T& value)
    {
        if constexpr (std::is_same_v<T, std::string>)
            return sqlString(value);          // escaped med quotes
        else
            return std::to_string(value);      // tal er sikre i sig selv
    }
};

#endif // DATABASE_H