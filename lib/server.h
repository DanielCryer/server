#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QMutex>
#include "httplib.h"

class QHostAddress;


class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    void setHostAddress(const QString &addr);
    void setPort(const uint &port);

public slots:
    void start();
    void stop();
    void close();

signals:
    void closed();

private:
    bool initDB();

    httplib::Server _serv;
    std::thread _listenThread;
    QHostAddress* _addr;
    uint _port;
    QMutex reqMut;
};

#endif // SERVER_H
