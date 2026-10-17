#ifndef CMYSQL_H
#define CMYSQL_H

#include "mysql_connection.h"
#include "mysql_driver.h"
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

struct QUERY_DATA {
    std::string query;
    int resultType;
};

struct CONNECTION_INFO {
    std::string Host;
    std::string User;
    std::string Pass;
    std::string Database;
};

class CMySQL {
protected:
    sql::Driver* driver;
    sql::Connection* connection;
    std::queue<QUERY_DATA> queryQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    bool stopThreads;

public:
    CMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database);
    virtual ~CMySQL();
    virtual void addQuery(const std::string& query, int resultType = 0);
    virtual std::unique_ptr<sql::ResultSet> executeQuery(const std::string& query);
    virtual bool executeQueryBool(const std::string& query);

    static CMySQL& getInstance();

protected:
    virtual void executeQueries();
    virtual void close();
    CONNECTION_INFO connectionInfo;
};

#endif // CMYSQL_H