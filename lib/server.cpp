#include "server.h"
#include "handlers.h"
#include <QHostAddress>
#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDebug>

Server::Server(QObject *parent) :
    QObject{parent}, _serv{}, _port{7070},
    _addr{new QHostAddress{QHostAddress::LocalHost}}
{
    if (!initDB()){
        close();
        return;
    }

    _serv.Get("/ping", Handlers::pingHandler);
    _serv.Get("/", Handlers::gettingRequestsHandler);
    _serv.Post("/new_user", Handlers::registrationHandler);
    _serv.Get("/user", Handlers::authorizationHandler);
    _serv.Get("/user/requests", Handlers::gettingUserRequestsHandler);
    _serv.Get("/user/requests/info", Handlers::gettingUserDealsByRequestHandler);
    _serv.Post("/user/requests/new_request", Handlers::creationRequestHandler);
    _serv.Post("/user/requests/cansel_request", Handlers::cancelRequestHandler);
}

void Server::setHostAddress(const QString &addr)
{
    _addr->setAddress(addr);
}

void Server::setPort(const uint &port)
{
    _port = port;
}

void Server::start()
{
    _listenThread = std::thread{[&](){_serv.listen(_addr->toString().toStdString(), _port);}};
    qDebug() << QString{"Server started at %1:%2"}.arg(_addr->toString()).arg(_port);
}

void Server::stop()
{
    if (!_listenThread.joinable())
        return;

    _serv.stop();

    _listenThread.join();

    qDebug() << QString{"Server stoped"};
}

void Server::close()
{
    stop();
    delete _addr;

    qDebug() << QString{"Server closed"};

    emit closed();
}

bool Server::initDB()
{
    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("db.sqlite");
    if (!db.open()) {
        qDebug() << "Error: " << db.lastError().text();
        return false;
    }

    if (auto tList = db.tables(); tList.empty()) {
        QSqlQuery query;

        qDebug() << "Creating new database...";

        query.exec("create table user("
                   "user_id     integer unique primary key, "
                   "email       text not null unique, "
                   "name        text not null, "
                   "password_hash text not null);");

        query.exec("create table wallet ("
                   "wallet_id   integer  unique primary key, "
                   "user_id     integer not null, "
                   "USD_balance integer default 0 not null, "
                   "RUB_balance integer default 0 not null);");

        query.exec("CREATE TABLE request ("
                   "request_id	INTEGER NOT NULL UNIQUE,"
                   "status      TEXT NOT NULL CHECK(status in ('open', 'closed', 'canceled')),"
                   "user_id     INTEGER NOT NULL,"
                   "open_date	TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "direction	TEXT NOT NULL CHECK(direction in ('buy', 'sale')),"
                   "cost        INTEGER NOT NULL CHECK(cost >= 0),"
                   "value       INTEGER NOT NULL CHECK(value > 0),"
                   "current_value INTEGER NOT NULL CHECK(value > 0),"
                   "FOREIGN KEY(user_id) REFERENCES user(user_id),"
                   "PRIMARY KEY(request_id) )");

        query.exec("CREATE TABLE deal ("
                   "deal_id	INTEGER NOT NULL UNIQUE,"
                   "buy_request_id	INTEGER NOT NULL,"
                   "sale_request_id	INTEGER NOT NULL,"
                   "date            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "cost            INTEGER NOT NULL CHECK(cost >= 0),"
                   "value           INTEGER NOT NULL CHECK(value > 0),"
                   "FOREIGN KEY(sale_request_id) REFERENCES request(request_id),"
                   "FOREIGN KEY(buy_request_id) REFERENCES request(request_id),"
                   "PRIMARY KEY(deal_id) )");

        db.close();

        qDebug() << "Database created";

    } else if (tList.size() != 4) {
        qDebug() << "Error: Unknown database file. Delete or replace it";
        return false;

    } else {
        QSet<QString> names = {"user", "wallet", "request", "deal"};

        for (const auto &tName : tList)
            if (!names.contains(tName)) {
                qDebug() << "Error: Unknown database file. Delete or replace it";
                return false;
            }
    }


    return true;
}
