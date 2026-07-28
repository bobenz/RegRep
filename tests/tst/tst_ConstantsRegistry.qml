import QtQuick 2.15
import QtTest 1.2
import RegRep 1.0

// Tests for ConstantEntry / ConstantRegistry and the Constants context
// property map. Uses codes in the -88xxx range to avoid collisions with any
// production constants registered elsewhere.
TestCase {
    name: "ConstantsRegistry"

    // ── Register test constants once before all tests run ────────────────────
    function initTestCase() {
        ConstantRegistry.declare({ name: "tst_alpha", source: "TST", code: -88001,
                                    description: "Alpha test error" })
        ConstantRegistry.declare({ name: "tst_beta",  source: "TST", code: -88002,
                                    description: "Beta test error" })
        ConstantRegistry.declare({ name: "tst_gamma", source: "OTH", code: -88003,
                                    description: "Gamma test error" })

        // A non-error constant sharing a code+source with an error constant —
        // "type" is what keeps these apart.
        ConstantRegistry.declare({ name: "tst_alpha_cfg", source: "TST", code: -88001,
                                    description: "Alpha as a config value", type: "config",
                                    data: { unit: "count", max: 5 } })
    }

    // ── type defaults to "error" when omitted ─────────────────────────────────
    function test_01_defaultType() {
        compare(Constants.tst_alpha.type, "error")
    }

    // ── declared entry is accessible via Constants property map ──────────────
    function test_02_constantsSymbolicAccess() {
        var e = Constants.tst_alpha
        verify(e !== undefined && e !== null)
        compare(e.code,        -88001)
        compare(e.source,      "TST")
        compare(e.description, "Alpha test error")
        verify(e.valid)
    }

    // ── ConstantEntry.text format includes type and source ───────────────────
    function test_03_textFormat() {
        var txt = Constants.tst_alpha.text
        verify(txt.indexOf("error") >= 0)
        verify(txt.indexOf("TST")   >= 0)
        verify(txt.indexOf("-88001") >= 0)
    }

    // ── explicit type + free-form data round-trips through declare() ─────────
    function test_04_typeAndDataRoundTrip() {
        var e = Constants.tst_alpha_cfg
        compare(e.type,        "config")
        compare(e.code,        -88001)
        compare(e.source,      "TST")
        compare(e.data.unit,   "count")
        compare(e.data.max,    5)
    }

    // ── same (code, source) but different type does not collide ──────────────
    function test_05_typeDisambiguatesSameCodeAndSource() {
        var err = ConstantRegistry.lookup("error",  -88001, "TST")
        var cfg = ConstantRegistry.lookup("config", -88001, "TST")
        verify(err.valid)
        verify(cfg.valid)
        compare(err.type, "error")
        compare(cfg.type, "config")
        verify(err.description !== cfg.description)
    }

    // ── legacy lookup(code, source) still works when the pair is unambiguous ─
    function test_06_legacyLookupByCodeAndSourceUnambiguous() {
        var e = ConstantRegistry.lookup(-88002, "TST")
        compare(e.code,   -88002)
        compare(e.source, "TST")
        compare(e.type,   "error")
    }

    // ── legacy lookup(code, source) warns and returns a match when ambiguous ─
    function test_07_legacyLookupByCodeAndSourceAmbiguous() {
        ignoreWarning(/is ambiguous/)
        var e = ConstantRegistry.lookup(-88001, "TST")   // matches both error and config
        verify(e.valid)
    }

    // ── legacy lookup(code) searches across type and source ──────────────────
    function test_08_legacyLookupByCodeOnly() {
        ignoreWarning(/is ambiguous/)
        var e = ConstantRegistry.lookup(-88001)   // matches TST/error and TST/config
        verify(e.valid)
    }

    // ── lookupByName(name) ─────────────────────────────────────────────────────
    function test_09_lookupByName() {
        var e = ConstantRegistry.lookupByName("tst_beta")
        compare(e.code,        -88002)
        compare(e.description, "Beta test error")
    }

    // ── contains() overloads ──────────────────────────────────────────────────
    function test_10_contains() {
        verify(ConstantRegistry.contains("error", -88001, "TST"))
        verify(ConstantRegistry.contains(-88001, "TST"))
        verify(ConstantRegistry.contains(-88001))
        verify(!ConstantRegistry.contains(-99999))
    }

    // ── containsName(name) ─────────────────────────────────────────────────────
    function test_11_containsByName() {
        verify(ConstantRegistry.containsName("tst_alpha"))
        verify(!ConstantRegistry.containsName("no_such_constant"))
    }

    // ── describe() returns a non-empty, informative string ───────────────────
    function test_12_describe() {
        var desc = ConstantRegistry.describe("error", -88001, "TST")
        verify(desc.length > 0)
        verify(desc.indexOf("Alpha") >= 0)
    }

    // ── ConstantEntry.valid is false for an unregistered lookup ──────────────
    function test_13_unknownEntryNotValid() {
        var e = ConstantRegistry.lookup("error", -77777, "NOPE")
        verify(!e.valid)
    }

    // ── entries from a different source stay distinct ─────────────────────────
    function test_14_differentSource() {
        var e = Constants.tst_gamma
        compare(e.code,   -88003)
        compare(e.source, "OTH")
    }

    // ── built-in sentinel entries are pre-registered ──────────────────────────
    function test_15_sentinelsPreregistered() {
        verify(ConstantRegistry.containsName("no_error"))
        verify(ConstantRegistry.containsName("unknown_error"))
        compare(Constants.no_error.code, 0)
    }
}
