#include <QtQuickTest/quicktest.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMetaEnum>
#include <cstring>
#include <vector>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

#include "regrep_registration.h"
#include "reporter.h"

// Find the Qt qml/ import directory by asking Windows where QtXCore*.dll was
// loaded from. QLibraryInfo's path lookup can be wrong when no qt.conf sits
// next to the test executable, so we bypass it entirely. Tries Qt6 then Qt5
// module names so the same test binary works under either kit.
static QString qtQmlImportPath()
{
#ifdef Q_OS_WIN
    static const wchar_t *candidates[] = {
        L"Qt6Cored.dll", L"Qt6Core.dll", L"Qt5Cored.dll", L"Qt5Core.dll"
    };
    for (const wchar_t *name : candidates) {
        HMODULE hMod = GetModuleHandleW(name);
        if (!hMod) continue;
        wchar_t buf[32768];
        DWORD n = GetModuleFileNameW(hMod, buf, 32768);
        if (n) {
            // QtXCore(d).dll lives in {QtPrefix}/bin/, QML imports live in {QtPrefix}/qml/
            QString binDir = QFileInfo(QString::fromWCharArray(buf, n)).absoluteDir().absolutePath();
            return QDir::cleanPath(binDir + "/../qml");
        }
    }
#endif
    // Fallback for non-Windows or if GetModuleHandle fails
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
#else
    return QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath);
#endif
}

// Test-only helper: gives QML test cases a way to drive Reporter (a plain
// C++ class with no QML surface by design) so ReportsReceiver filtering can
// be exercised end-to-end from tst_ReportsReceiver.qml.
class TestReporter : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE void emitReport(const QString &source, const QString &category,
                                 const QString &message, const QVariant &data = QVariant())
    {
        Reporter r(source);
        QMetaEnum me = QMetaEnum::fromType<Report::Category>();
        bool ok = false;
        int value = me.keyToValue(category.toLatin1().constData(), &ok);
        r.log(ok ? static_cast<Report::Category>(value) : Report::Info, message, data);
    }

    Q_INVOKABLE void emitError(const QString &source, const ConstantEntry &entry)
    {
        Reporter(source).error(entry);
    }
};

class TestSetup : public QObject
{
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->addImportPath(qtQmlImportPath());
        new RegRepRegistration(engine);
        engine->rootContext()->setContextProperty("TestReporter", new TestReporter(engine));
    }
};

// Qt 5.15 bug (QTBUG-84640): Qt Creator injects -qmljsdebugger=… which starts
// the QML debug-server thread at an unsafe moment → assert in qqmldebugserver.
// Strip the argument before the test runner sees it.
int main(int argc, char **argv)
{
    std::vector<char *> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        if (std::strncmp(argv[i], "-qmljsdebugger", 14) != 0)
            args.push_back(argv[i]);
    }
    int filteredArgc = static_cast<int>(args.size());

    TestSetup setup;
    return quick_test_main_with_setup(filteredArgc, args.data(),
                                      "regreptests", SRCDIR "/tst", &setup);
}

#include "main.moc"
