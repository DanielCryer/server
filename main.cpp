#include <QCoreApplication>
#include "lib/server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Server serv(&a);
    if (argc > 1) {
        serv.setHostAddress(argv[1]);
        serv.setPort(atoi(argv[2]));
    }
    QObject::connect(&serv, &Server::closed, &a, &QCoreApplication::quit);

    serv.start();

    return a.exec();
}
