#pragma once

#include <memory>
#include <odb/mysql/database.hxx>

template <typename T>
double testOrmSelectTime(std::shared_ptr<odb::mysql::database> db, int limit);

void averageSelectAllModels(std::shared_ptr<odb::mysql::database> db, int limit, int repeats);
void benchmarkInsertOrders(std::shared_ptr<odb::mysql::database> db, int repeats);
void benchmarkUpdateOrders(std::shared_ptr<odb::mysql::database> db, int repeats);
void benchmarkDeleteOrders(std::shared_ptr<odb::mysql::database> db, int repeats);
std::shared_ptr<odb::mysql::database> connectDB();
