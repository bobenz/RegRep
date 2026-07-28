import QtQuick 2.15
import QtTest 1.2
import RegRep 1.0

// Tests for the Reporter -> ReportRouter -> ReportsReceiver pipeline.
// Reporter itself has no QML surface by design (it's meant to be embedded as
// a C++ member), so reports are driven through the TestReporter helper that
// tests/main.cpp registers only for this test binary.
TestCase {
    name: "ReportsReceiver"

    function initTestCase() {
        ConstantRegistry.declare({ name: "tst_report_err", source: "TST", code: -88010,
                                    description: "Test report error" })
    }

    // ── Subjects: one receiver per filter kind, plus a catch-all ─────────────
    ReportsReceiver { id: bySource;   sourceFilter: "CDM.*" }
    ReportsReceiver { id: byCategory; categoryFilter: "Error" }
    ReportsReceiver { id: byMessage;  messageFilter: "shutter" }
    ReportsReceiver { id: catchAll }

    SignalSpy { id: sourceSpy;   target: bySource;   signalName: "reportReceived" }
    SignalSpy { id: categorySpy; target: byCategory; signalName: "reportReceived" }
    SignalSpy { id: messageSpy;  target: byMessage;  signalName: "reportReceived" }
    SignalSpy { id: allSpy;      target: catchAll;   signalName: "reportReceived" }

    function cleanup() {
        sourceSpy.clear()
        categorySpy.clear()
        messageSpy.clear()
        allSpy.clear()
    }

    // ── sourceFilter matches, others with non-matching filters do not ────────
    function test_01_sourceFilterMatch() {
        TestReporter.emitReport("CDM/Dispense", "Info", "started")
        compare(sourceSpy.count,   1)
        compare(categorySpy.count, 0)
        compare(messageSpy.count,  0)
        compare(allSpy.count,      1)
    }

    // ── sourceFilter does not match an unrelated source ───────────────────────
    function test_02_sourceFilterNoMatch() {
        TestReporter.emitReport("PTR/Print", "Info", "printing")
        compare(sourceSpy.count, 0)
        compare(allSpy.count,    1)
    }

    // ── categoryFilter matches on category name ───────────────────────────────
    function test_03_categoryFilterMatch() {
        TestReporter.emitReport("PTR/Print", "Error", "paper jam")
        compare(categorySpy.count, 1)
        compare(sourceSpy.count,   0)
        compare(allSpy.count,      1)
    }

    // ── messageFilter matches a substring, case-insensitively ────────────────
    function test_04_messageFilterMatch() {
        TestReporter.emitReport("CDM/Shutter", "Warning", "Shutter stuck")
        compare(messageSpy.count, 1)
        compare(sourceSpy.count,  1)   // also matches CDM.*
        compare(allSpy.count,     1)
    }

    // ── Report fields arrive intact on the receiving signal ──────────────────
    function test_05_reportFieldsIntact() {
        TestReporter.emitReport("CDM/Dispense", "Debug", "signal: enter", { count: 3 })
        compare(allSpy.count, 1)
        var report = allSpy.signalArguments[0][0]
        compare(report.source,   "CDM/Dispense")
        compare(report.category, "Debug")
        compare(report.message,  "signal: enter")
        compare(report.data.count, 3)
        verify(report.timestamp !== undefined)
    }

    // ── Reporter.error() carries the ConstantEntry through as report.data ────
    function test_06_errorCarriesConstantEntry() {
        TestReporter.emitError("TST/Widget", Constants.tst_report_err)
        compare(categorySpy.count, 1)
        var report = categorySpy.signalArguments[0][0]
        compare(report.category, "Error")
        compare(report.message,  "Test report error")
        compare(report.data.code,        -88010)
        compare(report.data.description, "Test report error")
    }

    // ── A destroyed receiver unregisters cleanly — no crash, no stale calls ──
    function test_07_unregisterOnDestroy() {
        var temp = Qt.createQmlObject('import RegRep 1.0; ReportsReceiver {}',
                                       catchAll, "dynamicReceiver")
        verify(temp !== null)
        temp.destroy()
        wait(0)   // let the deferred delete run
        // Publishing after destruction must not crash and must not reach `temp`.
        TestReporter.emitReport("ANY", "Info", "post-destroy")
        compare(allSpy.count, 1)
    }
}
