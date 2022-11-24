#include <QtTest>
#include "testhandlers.h"


int main(int argc, char *argv[])
{
    QTest::qExec(new TestHandlers, argc, argv);
    return 0;
}
