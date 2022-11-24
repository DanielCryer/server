#ifndef DBSINGLETON_H
#define DBSINGLETON_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMutex>

class DBSingleton : public QObject
{
    Q_OBJECT
public:
    explicit DBSingleton(QObject *parent = nullptr);

    QSqlQuery getQuery();
    void lock();
    void unlock();

private:
    QSqlDatabase _db;
    QMutex _mut;
};

#endif // DBSINGLETON_H
