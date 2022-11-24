#ifndef TESTHANDLERS_H
#define TESTHANDLERS_H

#include <QtTest>

class TestHandlers : public QObject
{
    Q_OBJECT

public:
    TestHandlers();
    ~TestHandlers();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void registrationHandler_data();
    void authorizationHandler_data();
    void gettingRequestsHandler_data();
    void gettingUserRequestsHandler_data();
    void gettingUserDealsByRequestHandler_data();
    void creationRequestHandler_data();
    void cancelRequestHandler_data();

    void registrationHandler();
    void authorizationHandler();
    void gettingRequestsHandler();
    void gettingUserRequestsHandler();
    void gettingUserDealsByRequestHandler();
    void creationRequestHandler();
    void cancelRequestHandler();
};

#endif // TESTHANDLERS_H
