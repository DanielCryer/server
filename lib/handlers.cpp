#include "handlers.h"
#include "dbsingleton.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QList>

Q_GLOBAL_STATIC(DBSingleton, GLOBAL_DATABASE)

QByteArray getHash(const QString &password);
QJsonObject getUserData(const QString &email,
                        const QString &password);
QJsonObject getUserData(QString header_value);
bool isCorrectPostRequest(const httplib::Request &req,
                          const QList<QString> &checkingObjects = {},
                          const QList<QString> &checkingHeaders = {});
bool isCorrectRequest(const httplib::Request &req,
                         const QList<QString> &checkingParams = {},
                         const QList<QString> &checkingHeaders = {});
void makeDeals(const QJsonObject &req);


void Handlers::registrationHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();

    if (isCorrectPostRequest(req, {"registration_data"})) {
        auto userData = QJsonDocument::fromJson(QByteArray::fromStdString(req.body))
                .object()["registration_data"].toObject();

        auto email = userData["email"].toString();
        auto name = userData["name"].toString();
        auto pass = userData["password"].toString();

        if (email.isEmpty() || name.isEmpty() || pass.isEmpty()) {
            res.status = 200;
            resContent["message"] = "Empty data can't be accepted";
            res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
            return;
        }

        GLOBAL_DATABASE->lock();
        auto q = GLOBAL_DATABASE->getQuery();
        q.prepare("insert into user "
                  "(email, name, password_hash) "
                  "values (:email, :name, :pass_hash);");
        q.bindValue(":email", email);
        q.bindValue(":name", name);
        q.bindValue(":pass_hash", getHash(pass));

        if (q.exec()) {
            q.prepare("insert into wallet (user_id) "
                      "values ((select user_id from user where email = :email))");
            q.bindValue(":email", email);
            q.exec();

            GLOBAL_DATABASE->unlock();

            res.status = 200;
            resContent["message"] = "Successful";
            resContent.insert("user", getUserData(email, pass));
        } else {
            GLOBAL_DATABASE->unlock();

            res.status = 200;
            resContent["message"] = q.lastError().text().contains("UNIQUE constraint failed") ?
                        "Email already exist" : q.lastError().text();
        }
    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::authorizationHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();

    if (isCorrectRequest(req, {}, {"Authorization"})) {
        auto userData = getUserData(QString::fromStdString(req.get_header_value("Authorization")));

        if (!userData.isEmpty()) {
            res.status = 200;
            resContent["message"] = "Successful";
            resContent.insert("user", userData);
        } else {
            res.status = 200;
            resContent["message"] = "Wrong email or password";
        }

    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::gettingRequestsHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();

    if (isCorrectRequest(req, {"order", "direction", "reverse_sort", "limit_start", "limit"})) {
        auto order1 = req.get_param_value("order") == "date" ? "open_date" : "cost";
        auto order2 = req.get_param_value("order") == "date" ? "cost" : "open_date";

        GLOBAL_DATABASE->lock();
        auto q = GLOBAL_DATABASE->getQuery();
        auto str = QString("select * from request where direction = :direction and status = 'open' "
                           "order by %1 %3, %2 desc limit :limit_start, :limit;"
                           ).arg(order1, order2, req.get_param_value("reverse_sort") == "true" ? "asc" : "desc");
        q.prepare(str);
        q.bindValue(":direction", QString::fromStdString(req.get_param_value("direction")));
        q.bindValue(":limit_start", stoi(req.get_param_value("limit_start")));
        q.bindValue(":limit", stoi(req.get_param_value("limit")));

        if (q.exec()) {
            auto requests = QJsonArray();
            auto r = QJsonObject();

            while (q.next()) {
                r["request_id"] = q.value("request_id").toJsonValue();
                r["status"] = q.value("status").toJsonValue();
                r["open_date"] = q.value("open_date").toJsonValue();
                r["user_id"] = q.value("user_id").toJsonValue();
                r["current_value"] = q.value("current_value").toJsonValue();
                r["value"] = q.value("value").toJsonValue();
                r["direction"] = q.value("direction").toJsonValue();
                r["cost"] = q.value("cost").toJsonValue();

                requests.append(r);
            }

            resContent.insert("requests", requests);
            res.status = 200;
            resContent["message"] = "Successful";

        } else {
            res.status = 415;
            resContent["message"] = q.lastError().text();
        }
        GLOBAL_DATABASE->unlock();
    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::gettingUserRequestsHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();
    if (isCorrectRequest(req, {"order", "direction", "reverse_sort", "limit_start", "limit"}, {"Authorization"})) {
        auto userData = getUserData(QString::fromStdString(req.get_header_value("Authorization")));

        if (!userData.isEmpty()) {
            auto order1 = req.get_param_value("order") == "date" ? "open_date" : "cost";
            auto order2 = req.get_param_value("order") == "date" ? "cost" : "open_date";

            GLOBAL_DATABASE->lock();
            auto q = GLOBAL_DATABASE->getQuery();
            q.prepare(QString("select * from request where direction = :direction and user_id = :user_id "
                      "order by %1 %3, %2 desc limit :limit_start, :limit;"
                              ).arg(order1, order2, req.get_param_value("reverse_sort") == "true" ? "asc" : "desc"));
            q.bindValue(":direction", QString::fromStdString(req.get_param_value("direction")));
            q.bindValue(":user_id", userData.value("user_id").toInt());
            q.bindValue(":limit_start", stoi(req.get_param_value("limit_start")));
            q.bindValue(":limit", stoi(req.get_param_value("limit")));

            if (q.exec()) {
                auto requests = QJsonArray();
                auto r = QJsonObject();

                while (q.next()) {
                    r["request_id"] = q.value("request_id").toJsonValue();
                    r["status"] = q.value("status").toJsonValue();
                    r["open_date"] = q.value("open_date").toJsonValue();
                    r["user_id"] = q.value("user_id").toJsonValue();
                    r["current_value"] = q.value("current_value").toJsonValue();
                    r["value"] = q.value("value").toJsonValue();
                    r["direction"] = q.value("direction").toJsonValue();
                    r["cost"] = q.value("cost").toJsonValue();

                    requests.append(r);
                }

                resContent.insert("requests", requests);
                res.status = 200;
                resContent["message"] = "Successful";

            } else {
                res.status = 415;
                resContent["message"] = q.lastError().text();
            }
            GLOBAL_DATABASE->unlock();

        } else {
            res.status = 401;
            resContent["message"] = "Wrong email or password";
        }

    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::gettingUserDealsByRequestHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();
    if (isCorrectRequest(req, {"request_id"}, {"Authorization"})) {
        auto userData = getUserData(QString::fromStdString(req.get_header_value("Authorization")));
        auto request_id = stoi(req.get_param_value("request_id"));

        if (!userData.isEmpty()) {
            GLOBAL_DATABASE->lock();
            auto q = GLOBAL_DATABASE->getQuery();

            q.prepare("select * from request where request_id = :request_id and user_id = :user_id;");
            q.bindValue(":request_id", request_id);
            q.bindValue(":user_id", userData.value("user_id").toInt());

            if (q.exec() && q.first()) {
                resContent.insert("request", QJsonObject({{"request_id", q.value("request_id").toJsonValue()},
                                                          {"user_id", q.value("user_id").toJsonValue()},
                                                          {"status", q.value("status").toJsonValue()},
                                                          {"direction", q.value("direction").toJsonValue()},
                                                          {"value", q.value("value").toJsonValue()},
                                                          {"current_value", q.value("current_value").toJsonValue()},
                                                          {"cost", q.value("cost").toJsonValue()},
                                                          {"open_date", q.value("open_date").toJsonValue()}}));
                auto direction = q.value("direction").toString();

                q.prepare(QString("select deal.deal_id as id, deal.value as value, deal.cost as cost, "
                                  "deal.date as date, user.name as name "
                                  "from deal join request on deal.%1_request_id = request.request_id "
                                  "join user on request.user_id = user.user_id "
                                  "where deal.%2_request_id = :request_id order by date desc;")
                          .arg(direction == "buy" ? "sale" : "buy", direction));
                q.bindValue(":request_id", request_id);

                if (q.exec()) {
                    auto deals = QJsonArray();
                    auto d = QJsonObject();

                    while (q.next()) {
                        d["deal_id"] = q.value("id").toJsonValue();
                        d["value"] = q.value("value").toJsonValue();
                        d["cost"] = q.value("cost").toJsonValue();
                        d["date"] = q.value("date").toJsonValue();
                        d["name"] = q.value("name").toJsonValue();

                        deals.append(d);
                    }

                    resContent.insert("deals", deals);
                    res.status = 200;
                    resContent["message"] = "Successful";

                } else {
                    res.status = 415;
                    resContent["message"] = q.lastError().text();
                }

            } else {
                res.status = 404;
                resContent["message"] = "Request not found";
            }
            GLOBAL_DATABASE->unlock();

        } else {
            res.status = 401;
            resContent["message"] = "Wrong email or password";
        }

    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::creationRequestHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();

    if (isCorrectPostRequest(req, {"request"}, {"Authorization"})) {
        auto requestData = QJsonDocument::fromJson(QByteArray::fromStdString(req.body))
                .object()["request"].toObject();
        auto userData = getUserData(QString::fromStdString(req.get_header_value("Authorization")));

        if (!userData.isEmpty()) {
            GLOBAL_DATABASE->lock();

            auto q = GLOBAL_DATABASE->getQuery();

            q.prepare("insert into request "
                      "(user_id, status, direction, value, current_value, cost) "
                      "values (:user_id, :status, :direction, :value, :current_value, :cost);");
            q.bindValue(":user_id", userData["user_id"].toInt());
            q.bindValue(":status", "open");
            q.bindValue(":direction", requestData["direction"].toString());
            q.bindValue(":value", requestData["value"].toInt());
            q.bindValue(":current_value", requestData["value"].toInt());
            q.bindValue(":cost", requestData["cost"].toInt());

            if (q.exec()) {
                q.exec("select request_id, user_id, status, direction, value, cost, open_date "
                       "from request where request_id = last_insert_rowid();");
                q.first();

                auto reqRes = QJsonObject({{"request_id", q.value("request_id").toJsonValue()},
                                           {"user_id", q.value("user_id").toJsonValue()},
                                           {"status", q.value("status").toJsonValue()},
                                           {"direction", q.value("direction").toJsonValue()},
                                           {"value", q.value("value").toJsonValue()},
                                           {"cost", q.value("cost").toJsonValue()},
                                           {"open_date", q.value("open_date").toJsonValue()}});

                res.status = 200;
                resContent["message"] = "Successful";

                GLOBAL_DATABASE->unlock();

                makeDeals(reqRes);
            } else {
                GLOBAL_DATABASE->unlock();

                res.status = 415;
                resContent["message"] = q.lastError().text();
            }
        } else {
            res.status = 401;
            resContent["message"] = "Wrong email or password";
        }

    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::cancelRequestHandler(const httplib::Request &req, httplib::Response &res)
{
    auto resContent = QJsonObject();

    if (isCorrectPostRequest(req, {"request_id"}, {"Authorization"})) {
        auto request_id = QJsonDocument::fromJson(QByteArray::fromStdString(req.body))
                .object()["request_id"].toInt();
        auto userData = getUserData(QString::fromStdString(req.get_header_value("Authorization")));

        if (!userData.isEmpty()) {
            GLOBAL_DATABASE->lock();
            auto q = GLOBAL_DATABASE->getQuery();

            q.prepare("select status from request where request_id = :request_id and user_id = :user_id;");
            q.bindValue(":request_id", request_id);
            q.bindValue(":user_id", userData["user_id"].toInt());

            if (q.exec() && q.first()) {

                if (q.value("status").toString() == "open") {
                    q.prepare("update request set status = 'canceled' where request_id = :request_id;");
                    q.bindValue(":request_id", request_id);
                    q.exec();

                    res.status = 200;
                    resContent["message"] = "Successful";
                } else {
                    res.status = 200;
                    resContent["message"] = "Request already " + q.value("status").toString();
                }

            } else {
                res.status = 404;
                resContent["message"] = "Request not found";
            }
            GLOBAL_DATABASE->unlock();

        } else {
            res.status = 401;
            resContent["message"] = "Wrong email or password";
        }

    } else {
        res.status = 415;
        resContent["message"] = "Wrong request";
    }

    res.set_content(QJsonDocument(resContent).toJson().toStdString(), "application/json");
}



void Handlers::pingHandler(const httplib::Request &req, httplib::Response &res)
{

}



QByteArray getHash(const QString &password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
}

QJsonObject getUserData(const QString &email, const QString &password)
{
    auto data = QJsonObject();

    GLOBAL_DATABASE->lock();
    auto q = GLOBAL_DATABASE->getQuery();

    q.prepare("select user.user_id, email, password_hash, name, USD_balance, RUB_balance "
              "from user join wallet on user.user_id = wallet.user_id "
              "where email = :email");
    q.bindValue(":email", email);

    if (q.exec() && q.next() && (q.value("password_hash") == getHash(password))) {
        data.insert("user_id", q.value("user.user_id").toJsonValue());
        data.insert("email", q.value("email").toJsonValue());
        data.insert("name", q.value("name").toJsonValue());
        data.insert("USD_balance", q.value("USD_balance").toJsonValue());
        data.insert("RUB_balance", q.value("RUB_balance").toJsonValue());
    }
    GLOBAL_DATABASE->unlock();

    return data;
}

QJsonObject getUserData(QString header_value)
{
    auto base64Data = header_value.split(QRegExp("\\s")).last().toUtf8();
    auto data = QString::fromUtf8(QByteArray::fromBase64(base64Data)).split(QRegExp(":"));
    return getUserData(data.first(), data.last());
}

bool isCorrectPostRequest(const httplib::Request &req,
                          const QList<QString> &checkedObjects,
                          const QList<QString> &checkedHeaders)
{
    if (!isCorrectRequest(req, {}, checkedHeaders))
        return false;

    if (req.get_header_value("Content-type") != "application/json")
        return false;

    auto content = QJsonDocument::fromJson(QByteArray::fromStdString(req.body)).object();
    if (content.isEmpty())
        return false;

    for (const auto &name : checkedObjects) {
        if (!content.contains(name))
            return false;
    }

    return true;
}


bool isCorrectRequest(const httplib::Request &req,
                      const QList<QString> &checkedParams,
                      const QList<QString> &checkedHeaders)
{
    for (const auto &name : checkedHeaders) {
        if (!req.has_header(name.toStdString()))
            return false;
    }

    for (const auto &name : checkedParams) {
        if (!req.has_param(name.toStdString()))
            return false;
    }

    return true;
}

void makeDeals(const QJsonObject &req)
{
    GLOBAL_DATABASE->lock();

    auto currVal{req["value"].toInt()}, dealVal{0}, dealCost{0}, qVal{0};
    auto query{GLOBAL_DATABASE->getQuery()}, q{GLOBAL_DATABASE->getQuery()};
    auto isBuy{req["direction"].toString() == "buy"};

    if (isBuy) {
        query.prepare("select request_id, user_id, current_value, cost from request "
                  "where status = 'open' and  direction = 'sale' and cost <= :cost "
                  "order by cost asc, open_date asc;");
    } else {
        query.prepare("select request_id, user_id, current_value, cost from request "
                  "where status = 'open' and  direction = 'buy' and cost >= :cost "
                  "order by cost asc, open_date asc;");
    }

    query.bindValue(":cost", req["cost"].toInt());
    query.exec();

    while (query.next() && currVal > 0) {
        qVal = query.value("current_value").toInt();
        dealVal = qMin(currVal, qVal);
        dealCost = query.value("cost").toInt();

        q.prepare("insert into deal (buy_request_id, sale_request_id, value, cost) "
                  "values (:buy_id, :sale_id, :value, :cost);");
        q.bindValue(":sale_id", isBuy ? query.value("request_id").toInt() : req["request_id"].toInt());
        q.bindValue(":buy_id", isBuy ? req["request_id"].toInt() : query.value("request_id").toInt());
        q.bindValue(":value", dealVal);
        q.bindValue(":cost", dealCost);
        q.exec();

        q.prepare("update wallet set USD_balance = USD_balance - :usd_val, "
                  "RUB_balance = RUB_balance + :rub_val "
                  "where user_id = :user_id;");
        q.bindValue(":usd_val", dealVal);
        q.bindValue(":rub_val", dealVal * dealCost);
        q.bindValue(":user_id", isBuy ? query.value("user_id").toInt() : req["user_id"].toInt());
        q.exec();

        q.prepare("update wallet set USD_balance = USD_balance + :usd_val, "
                  "RUB_balance = RUB_balance - :rub_val "
                  "where user_id = :user_id;");
        q.bindValue(":usd_val", dealVal);
        q.bindValue(":rub_val", dealVal * dealCost);
        q.bindValue(":user_id", !isBuy ? query.value("user_id").toInt() : req["user_id"].toInt());
        q.exec();

        qVal -= dealVal;
        currVal -= dealVal;

        q.prepare("update request set status = :status, current_value = :curr_val "
                  "where request_id = :id;");
        q.bindValue(":status", qVal ? "open" : "closed");
        q.bindValue(":curr_val", qVal);
        q.bindValue(":id", query.value("request_id").toInt());
        q.exec();

        q.prepare("update request set status = :status, current_value = :curr_val "
                  "where request_id = :id;");
        q.bindValue(":status", currVal ? "open" : "closed");
        q.bindValue(":curr_val", currVal);
        q.bindValue(":id", req["request_id"].toInt());
        q.exec();
    }

    GLOBAL_DATABASE->unlock();
}
