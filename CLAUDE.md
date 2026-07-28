# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

RegRep is a Qt/QML plugin providing two independent, reusable building blocks that were extracted and
generalized out of `QmlConcerto` (a sibling project at `../QmlConcerto`):

1. **A generic named-constants registry** (`ConstantEntry` / `ConstantRegistry`) — was QmlConcerto's
   error-only `ErrorEntry`/`ErrorRegistry`. Generalized with a `type` field (defaults to `"error"`) and a
   free-form `data` payload, so the same registry can hold errors, config values, limits, etc. without
   collisions, keyed by `(type, code, source)`.
2. **A filtered report bus** (`Report`, `Reporter`, `ReportRouter`, `ReportsReceiver`) — `Report` was
   QmlConcerto's `Phrase::Report` struct. `ReportRouter` is a hidden C++ singleton that fans reports out to
   QML `ReportsReceiver` elements filtered by regex on `sourceFilter`/`categoryFilter`/`messageFilter`.
   `Reporter` is the instance-based emitter (`info()`/`warning()`/`error()`/`debug()`/`critical()`) meant to
   be owned as a member by classes that need to report, instead of baking reporting into a base class.

QmlConcerto itself has **not** been changed to consume this — it still has its own local
`errorsregistry.h`/`Report` struct. RegRep is a standalone, from-scratch copy with no build dependency on
QmlConcerto in either direction.

## Dual consumption model

Every type here is usable two ways, mirroring QmlConcerto's own pattern:

- **As a loaded QML plugin** — `RegRepPlugin` (`RegRepPlugin.h/.cpp`) implements `QQmlExtensionPlugin`.
  `import RegRep 1.0` in QML after the built plugin + `qmldir` are deployed together.
- **As source-included C++** — `include(regrep.pri)` in a consumer's `.pro`, then construct
  `RegRepRegistration(engine)` (from `regrep_registration.h`) once after creating the `QQmlEngine`. This is
  what `tests/main.cpp` does, and is the simpler path for anything that isn't itself a QML plugin.

Both paths register the same things: `ReportsReceiver` as a creatable QML type, plus `qRegisterMetaType` for
`ConstantEntry`/`Report` (needed so they can travel through signals/properties as QVariant) and two context
properties: `ConstantRegistry` (call `.declare()`/`.lookup()`/etc.) and `Constants` (the `QQmlPropertyMap` for
dot-notation access, e.g. `Constants.shutter_stuck.description`). `Report` is **not** registered as an
addressable QML type (no `qmlRegisterUncreatableType`) — nothing needs to name it as a type, only read its
fields off a `ReportsReceiver.onReportReceived` argument, and `qmlRegisterUncreatableType` requires an
uppercase-starting name or the plugin fails to load at runtime ("type names must begin with an uppercase
letter") even though value-type-style lowercase names only get a *warning* under direct/source-included
registration — the two code paths enforce this differently, so don't rely on the warning-only behavior.

`Reporter` and `ReportRouter` have **no QML API** by design — `Reporter` is meant to be embedded as a plain
C++ member (e.g. a future `Phrase m_reporter{"Sequence/Withdrawal"}`), and `ReportRouter` is an internal
singleton `ReportsReceiver` talks to. If you need to drive reports from QML (e.g. in tests), you must add a
`Q_INVOKABLE` wrapper — see `TestReporter` in `tests/main.cpp` for the pattern.

## Build

Two Qt kits are relevant to this codebase (see `regrep.pri`'s `QT += quick qml`, which is Qt5/Qt6-portable):

- Qt 6.10.3 / MSVC2022 x64 — installed at `D:\Qt\6.10.3\msvc2022_64`, verified working.
- Qt 5.15.2 / MSVC2019 32-bit — the kit QmlConcerto itself builds with. Not installed on this machine; build
  via Qt Creator with that kit selected if you need to verify it.

Build the plugin from a Developer Command Prompt (or via `vcvars64.bat` first):

```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd D:\Projects\bobenz\RegRep
qmake RegRep.pro
nmake
```

Output: `lib\release\RegRep.dll` (or `lib\debug\...` for a debug build), with `qmldir` copied alongside it
so `import RegRep 1.0` resolves.

## Tests

`tests/RegRepTests.pro` is a Qt Quick Test (`qmltestcase`) app that source-includes `../regrep.pri` directly
(not the built plugin), the same way `QmlConcertoTests` includes `../QmlConcerto/concerto.pri`. Test `.qml`
files live in `tests/tst/`.

```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd D:\Projects\bobenz\RegRep\tests
qmake RegRepTests.pro
nmake
```

Run (needs the Qt `bin` dir on `PATH` for the Qt DLLs, and `-platform offscreen` if there's no display):

```bash
set PATH=D:\Qt\6.10.3\msvc2022_64\bin;%PATH%
release\RegRepTests.exe -platform offscreen
```

The full suite runs in well under a second, so there's little need to filter it down. Passing a bare
test-function name as a positional arg (the normal QTestLib way to select one test) does **not** work
cleanly here — `quick_test_main_with_setup` runs each `tst/*.qml` file as its own `QTest::qExec()` pass with
the same argv, so a function name that only exists in one file makes the pass over the other file fail
before either produces output. To isolate one `.qml` file's tests, temporarily move the other file(s) out of
`tests/tst/` (or point `SRCDIR "/tst"` at a scratch dir containing only the file you want), rebuild is not
required — `nmake` only needs to run once; the test binary reads `.qml` files at runtime.

Test coverage:
- `tst_ConstantsRegistry.qml` — `declare()`/`create()`, `type`/`data` round-tripping, the `(type, code,
  source)` key disambiguating what used to collide, all `lookup`/`describe`/`contains` overloads including
  the legacy ambiguity-warning ones (asserted via `ignoreWarning(/is ambiguous/)`), and the `Constants`
  property map.
- `tst_ReportsReceiver.qml` — end-to-end `Reporter` → `ReportRouter` → `ReportsReceiver` delivery, each
  filter kind (`sourceFilter`/`categoryFilter`/`messageFilter`) matching and not-matching, `Report` field
  integrity (including `data` payloads and a `ConstantEntry` carried as `data`), and that a destroyed
  receiver unregisters cleanly (no stale delivery, no crash).

Since `Reporter` has no QML surface, tests drive it through a test-only `TestReporter` QML singleton
registered in `tests/main.cpp` (`emitReport(source, category, message, data)` / `emitError(source,
constantEntry)`) — this class is not part of the library and shouldn't be treated as public API.

## Conventions to follow when extending this

- Keep new registry-style state inside `ConstantRegistry` by giving it a new `type` string rather than
  inventing a parallel registry class — that's the whole point of the `(type, code, source)` generalization.
- Anything meant to be embedded as a member (like `Reporter`) should stay a plain, cheap-to-copy C++ class
  with no QObject/QML surface; only give it a QML API if there's an actual QML use case, not preemptively.
- Don't register a Q_GADGET value type as an addressable QML type (`qmlRegisterUncreatableType` or similar)
  unless something genuinely needs to name it as a type — `qRegisterMetaType` alone is enough for it to flow
  through signals/properties as QVariant and have its fields read from QML. If you do need one,
  `qmlRegisterUncreatableType` requires an uppercase-starting name — a lowercase one only warns under
  direct/source-included registration but hard-fails ("type names must begin with an uppercase letter") when
  loaded as a compiled plugin, so don't rely on the warning-only behavior to conclude lowercase is safe.
- When adding a new source file, it needs an entry in **both** `regrep.pri`'s `HEADERS`/`SOURCES` and, if it
  has QML-visible behavior, wiring in both `RegRepPlugin.cpp` and `regrep_registration.h` — the two
  consumption paths are kept in sync manually, there's no single source of truth for registration.
