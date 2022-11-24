#include "dbsingleton.h"
#include <QDebug>
#include <QSqlError>

QSqlQuery DBSingleton::getQuery()
{
    return QSqlQuery(_db);
}

void DBSingleton::lock()
{
    _mut.lock();
}

void DBSingleton::unlock()
{
    _mut.unlock();
}

DBSingleton::DBSingleton(QObject *parent)
    : QObject{parent}
{
    _db = QSqlDatabase::addDatabase("QSQLITE", "server_connection");
    _db.setDatabaseName("db.sqlite");
    _db.open();
}
