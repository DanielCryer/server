#include "testhandlers.h"
#include "../lib/handlers.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QFile>



TestHandlers::TestHandlers()
{

}

TestHandlers::~TestHandlers()
{

}

void TestHandlers::initTestCase()
{
    QFile::remove("db.sqlite");

    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("db.sqlite");
    if (!db.open()) {
        qDebug() << "Init_error: " << db.lastError().text();
        return;
    }
    QSqlQuery query;

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





    query.prepare("insert into user (email, name, password_hash) "
                  "values (:email, :name, :pass_hash);");
    query.bindValue(":email", "user@email.com");
    query.bindValue(":name", "username");
    query.bindValue(":pass_hash", QCryptographicHash::hash("Password", QCryptographicHash::Sha256));
    query.exec();

    query.exec("insert into wallet (user_id, USD_balance, RUB_balance) "
              "values (1, 20, 700)");

    query.exec("insert into request "
               "(user_id, status, direction, value, current_value, cost) "
               "values (1, 'open', 'sale', 40, 20, 70), (1, 'closed', 'buy', 20, 0, 70);");

    query.exec("insert into deal (buy_request_id, sale_request_id, value, cost) values (2, 1, 20, 70);");

    qDebug() << query.lastError().text();

    db.close();
}

void TestHandlers::cleanupTestCase()
{
    QFile::remove("db.sqlite");
}




void TestHandlers::registrationHandler_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");

    auto data = QJsonObject{{"email", "test@email.com"},
                            {"name", "testname"},
                            {"password", "testPass"}};
    QTest::addRow("validRequest") << QJsonDocument(QJsonObject{{"registration_data", data}}).toJson() << 200 << "Successful";

    data["email"] = "user@email.com";
    QTest::addRow("nonUniqueEmail") << QJsonDocument(QJsonObject{{"registration_data", data}}).toJson() << 200 << "Email already exist";

    data["name"] = "";
    QTest::addRow("emptyName") << QJsonDocument(QJsonObject{{"registration_data", data}}).toJson() << 200 << "Empty data can't be accepted";

    QTest::addRow("emptyBody") << QByteArray{} << 415 << "Wrong request";
}

void TestHandlers::authorizationHandler_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setHeader");

    QTest::addRow("validRequest") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 200 << "Successful" << true;

    QTest::addRow("wrongUserData") << QByteArray{"Basic QWNTd0Zzc0BlbWFpbC5jb206VGVzdF9QYSpzUw=="} << 200 << "Wrong email or password" << true;

    QTest::addRow("nonAuthHeader") << QByteArray{} << 415 << "Wrong request" << false;
}

void TestHandlers::gettingRequestsHandler_data()
{
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setParams");
    QTest::addColumn<int>("resultArrayCount");

    QTest::addRow("validRequest") << 200 << "Successful" << true << 1;

    QTest::addRow("notAllParams") << 415 << "Wrong request" << false << -1;
}

void TestHandlers::gettingUserRequestsHandler_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setParams");
    QTest::addColumn<int>("resultArrayCount");

    QTest::addRow("validRequest") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 200 << "Successful" << true << 1;

    QTest::addRow("wrongUserData") << QByteArray{"Basic QWNTd0Zzc0BlbWFpbC5jb206VGVzdF9QYSpzUw=="} << 401 << "Wrong email or password" << true << -1;

    QTest::addRow("notAllParams") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 415 << "Wrong request" << false << -1;
}

void TestHandlers::gettingUserDealsByRequestHandler_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<int>("requestId");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setParams");
    QTest::addColumn<int>("resultArrayCount");

    QTest::addRow("validRequest") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 1 << 200 << "Successful" << true << 1;

    QTest::addRow("wrongUserData") << QByteArray{"Basic QWNTd0Zzc0BlbWFpbC5jb206VGVzdF9QYSpzUw=="} << 1 << 401 << "Wrong email or password" << true << -1;

    QTest::addRow("invalidRequestId") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 10 << 404 << "Request not found" << true << -1;

    QTest::addRow("notAllParams") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 1 << 415 << "Wrong request" << false << -1;
}

void TestHandlers::creationRequestHandler_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setBody");

    QTest::addRow("validRequest") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 200 << "Successful" << true;

    QTest::addRow("wrongUserData") << QByteArray{"Basic QWNTd0Zzc0BlbWFpbC5jb206VGVzdF9QYSpzUw=="} << 401 << "Wrong email or password" << true;

    QTest::addRow("emptyBody") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 415 << "Wrong request" << false;
}

void TestHandlers::cancelRequestHandler_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<int>("requestId");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("resultMessage");
    QTest::addColumn<bool>("setBody");

    QTest::addRow("validRequest") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 1 << 200 << "Successful" << true;

    QTest::addRow("wrongUserData") << QByteArray{"Basic QWNTd0Zzc0BlbWFpbC5jb206VGVzdF9QYSpzUw=="} << 1 << 401 << "Wrong email or password" << true;

    QTest::addRow("invalidRequestId") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 10 << 404 << "Request not found" << true;

    QTest::addRow("alreadyClosedRequestId") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 2 << 200 << "Request already closed" << true;

    QTest::addRow("emptyBody") << QByteArray{"Basic dXNlckBlbWFpbC5jb206UGFzc3dvcmQ="} << 1 << 415 << "Wrong request" << false;
}






void TestHandlers::registrationHandler()
{
    QFETCH(QByteArray, body);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);

    auto res = httplib::Response{};
    auto req = httplib::Request{};
    req.set_header("Content-type", "application/json");
    req.body = body.toStdString();

    Handlers::registrationHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
}

void TestHandlers::authorizationHandler()
{
    QFETCH(QByteArray, header);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setHeader);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    if (setHeader)
        req.set_header("Authorization", header.toStdString());

    Handlers::authorizationHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
}

void TestHandlers::gettingRequestsHandler()
{
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setParams);
    QFETCH(int, resultArrayCount);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    req.params.insert({{"order", "date"}, {"direction", "sale"}});

    if (setParams)
        req.params.insert({{"reverse_sort", "true"}, {"limit_start", "0"}, {"limit", "20"}});

    Handlers::gettingRequestsHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
    if (setParams)
        QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["requests"].toArray().count(), resultArrayCount);
}

void TestHandlers::gettingUserRequestsHandler()
{
    QFETCH(QByteArray, header);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setParams);
    QFETCH(int, resultArrayCount);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    req.set_header("Authorization", header.toStdString());
    req.params.insert({{"order", "date"}, {"direction", "sale"}});

    if (setParams)
        req.params.insert({{"reverse_sort", "true"}, {"limit_start", "0"}, {"limit", "20"}});

    Handlers::gettingUserRequestsHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
    if (resultMessage == "Successful")
        QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["requests"].toArray().count(), resultArrayCount);
}

void TestHandlers::gettingUserDealsByRequestHandler()
{
    QFETCH(QByteArray, header);
    QFETCH(int, requestId);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setParams);
    QFETCH(int, resultArrayCount);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    req.set_header("Authorization", header.toStdString());

    if (setParams)
        req.params.insert({{"request_id", std::to_string(requestId)}});

    Handlers::gettingUserDealsByRequestHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
    if (resultMessage == "Successful")
        QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["deals"].toArray().count(), resultArrayCount);
}

void TestHandlers::creationRequestHandler()
{
    QFETCH(QByteArray, header);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setBody);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    req.set_header("Content-type", "application/json");
    req.set_header("Authorization", header.toStdString());

    if (setBody) {
        auto data = QJsonObject{{"direction", "buy"},
                                {"value", 10},
                                {"cost", 70}};
        req.body = QJsonDocument(QJsonObject{{"request", data}}).toJson().toStdString();
    }

    Handlers::creationRequestHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);

    if (res.status == 200) {
        req.params.insert({{"request_id", "1"}});

        Handlers::gettingUserDealsByRequestHandler(req, res);

        QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["deals"].toArray().count(), 2);
    }
}

void TestHandlers::cancelRequestHandler()
{
    QFETCH(QByteArray, header);
    QFETCH(int, requestId);
    QFETCH(int, statusCode);
    QFETCH(QString, resultMessage);
    QFETCH(bool, setBody);

    auto res = httplib::Response{};
    auto req = httplib::Request{};

    req.set_header("Content-type", "application/json");
    req.set_header("Authorization", header.toStdString());

    if (setBody) {
        req.body = QJsonDocument(QJsonObject{{"request_id", requestId}}).toJson().toStdString();
    }

    Handlers::cancelRequestHandler(req, res);

    QCOMPARE(res.status, statusCode);
    QCOMPARE(QJsonDocument::fromJson(res.body.data()).object()["message"].toString(), resultMessage);
}

