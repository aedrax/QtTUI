#include <QtTest/QtTest>

class TestPlaceholder : public QObject {
    Q_OBJECT
private slots:
    void sanityCheck()
    {
        QVERIFY(true);
    }

    void qttuiHeaderIncludable()
    {
        // Verifies the public header path works
        #include <QtTUI/QtTUIGlobal.h>
        QVERIFY(true);
    }
};

QTEST_MAIN(TestPlaceholder)
#include "test_placeholder.moc"
