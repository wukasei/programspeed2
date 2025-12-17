#pragma once
#include <string>
#include <vector>
#include <memory>
#include <jdbc/mysql_connection.h>

std::unique_ptr<sql::Connection> connectToDatabase();
void testRawSelect(const std::string& table, int limit, sql::Connection* con);
void averageSelectAllTables(int limit, int repeats, sql::Connection* con, const std::vector<std::string>& tables);

double testRawJoin(const std::string& type, int limit, sql::Connection* con);
void averageJoinTime(const std::string& type, int limit, int repeats, sql::Connection* con);

std::vector<int> testRawInsertTransaction(const std::string& table, int count, sql::Connection* con);
double testRawUpdateTransaction(const std::string& table, int id, sql::Connection* con);
double testRawDeleteTransaction(const std::string& table, int id, sql::Connection* con);
void testInsertUpdateDelete(const std::string& table, int count, sql::Connection* con);
