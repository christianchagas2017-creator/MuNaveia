#include "stdafx.h"
#include "MySQLManager.h"
#include "ServerInfo.h"
#include "Util.h"

static CMySQL* instance = nullptr;

CMySQL::CMySQL(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
    connectionInfo.Host = host;
    connectionInfo.User = user;
    connectionInfo.Pass = password;
    connectionInfo.Database = database;
    /*
    driver = get_driver_instance();
    connection = driver->connect(connectionInfo.Host, connectionInfo.User, connectionInfo.Pass);
    connection->setSchema(connectionInfo.Database);
    stopThreads = false;
    std::thread asyncThread(&CMySQL::executeQueries, this);
    asyncThread.detach(); // Detach thread to run asynchronously
    */
}

CMySQL::~CMySQL() {
    stopThreads = true;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queueCondition.notify_one(); // Notify thread to exit
    }
    delete connection;
}

void CMySQL::addQuery(const std::string& query, int resultType) {
    /*
    std::lock_guard<std::mutex> lock(queueMutex);
    QUERY_DATA info = { query, resultType};
    queryQueue.push(info);
    queueCondition.notify_one(); // Notify thread to process query
    */
}

void CMySQL::executeQueries() {
    while (!stopThreads) {
        QUERY_DATA info;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            // Wait until there's a query in the queue or stopThreads is true
            queueCondition.wait(lock, [this] { return !queryQueue.empty() || stopThreads; });
            if (stopThreads) return; // Exit thread if stopThreads is true
            info = queryQueue.front();
            queryQueue.pop();
        }
        switch (info.resultType) {
            case 1:
                executeQueryBool(info.query);
                break;
            default:
                executeQuery(info.query); // Execute query
                break;
        }

    }
}

std::unique_ptr<sql::ResultSet> CMySQL::executeQuery(const std::string& query) {
    try {
        connection->close();
        connection = driver->connect(connectionInfo.Host, connectionInfo.User, connectionInfo.Pass);
        connection->setSchema(connectionInfo.Database);
    }
    catch (const std::exception& e) {
        LogAdd(LOG_RED, "Connection failed: %s", e.what());
        return false;
    }

    sql::Statement* stmt = connection->createStatement();
    sql::ResultSet* result = stmt->executeQuery(query);
    if (stmt) delete stmt;
    return std::unique_ptr<sql::ResultSet>(result);
}

bool CMySQL::executeQueryBool(const std::string& query) {

    try {
        connection->close();
        connection = driver->connect(connectionInfo.Host, connectionInfo.User, connectionInfo.Pass);
        connection->setSchema(connectionInfo.Database);
    }
    catch (const std::exception& e) {
        LogAdd(LOG_RED, "Connection failed: %s", e.what());
        return false;
    }
  
    sql::Statement* stmt = connection->createStatement();

    try {
        return stmt->execute(query);
    }
    catch (std::exception e) {
        LogAdd(LOG_RED, const_cast<char*>(e.what()));
    }

    return true;
}

void CMySQL::close() {
    connection->close();
}

CMySQL& CMySQL::getInstance() {
    if (instance == nullptr) {
        // You should handle the initialization parameters here, maybe loading them from a configuration file
        instance = new CMySQL(gServerInfo.m_MySQLServer, gServerInfo.m_MySQLUser, gServerInfo.m_MySQLPass, gServerInfo.m_MySQLDatabase);
    }
    return *instance;
}