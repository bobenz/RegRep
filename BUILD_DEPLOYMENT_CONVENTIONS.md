# Qt/qmake plugin build & deployment conventions

Generalized from the RegRep/QmlConcerto deployment consolidation. Intended to be copied
into any qmake-based Qt plugin project (native `QQmlExtensionPlugin` DLL, `qmldir`-driven)
and adapted by replacing the placeholders below. Nothing here is RegRep/QmlConcerto-specific
except the worked example.

Placeholders used throughout:

- `<ROOT_ENV_VAR>` — the environment variable naming the deploy root (RegRep/QmlConcerto use `CNGO_DIR`).
- `<DEFAULT_ROOT>` — its default value if unset (they use `C:/CnGO`).
- `<ModuleName>` — the QML module/plugin name (e.g. `RegRep`, `Concerto`).
- `<Target>` — the qmake `TARGET` (the DLL base name, e.g. `RegRep`, `QmlConcerto`).

## 1. One deploy root, env-var-defined; Debug is a sibling root, not a subdir

Don't hardcode a deploy path in each `.pro`. Define it once via an environment variable with
a sane default, and give Debug its own **complete, independent** tree rather than nesting it
(`<root>/lib/debug`) — nesting makes "point PATH/import-path at the deploy root" ambiguous
between configs. A `d`-suffixed sibling root keeps Release and Debug trees structurally
identical and mutually exclusive:

```
<DEFAULT_ROOT>          # Release
<DEFAULT_ROOT>d         # Debug
```

## 2. Tree layout: one shared `lib/`, one directory per module

```
<ROOT>/
  lib/
    <Target>.dll         # every plugin's DLL, and nothing else, lands here
    <Target>.lib          # ...and its import lib, for C++ linking consumers
  <ModuleName>/
    qmldir                # "plugin <Target> ../lib" — the only registration mechanism
    *.qml                  # only if qmldir maps QML-file-defined types (see §3)
```

`lib/` is the single source of truth for both runtime (`.dll`) and link-time (`.lib`)
consumption — anything that needs to run against or link against the plugin points at
`lib/`, never at a project's own local build output.

Each module directory holds **only** `qmldir` (+ any `.qml` files it maps) — never a copy
of the DLL. `qmldir`'s `plugin <Target> ../lib` line is a relative path resolved from the
module directory's own location, which is what makes the "one `lib/`, many module dirs"
layout work: every module directory sits one level under `<ROOT>`, so `../lib` always
resolves back to the shared directory regardless of which module you loaded first.

## 3. `qmldir` is the *only* type-registration mechanism for the loaded-plugin path

If a module maps QML-file-defined types (composite types written in `.qml`, not C++), put
them in `qmldir` as file-mapping lines:

```
module <ModuleName>
plugin <Target> ../lib

SomeType   1.0 SomeType.qml
```

Do **not** also register those same types in C++ via
`qmlRegisterType(QUrl("qrc:/..."), ...)` in the plugin's `registerTypes()` — that's a
different, incompatible resolution path (qrc-embedded) and having both is redundant at
best, silently-wrong at worst if the two diverge. The C++ URL-registration form is only
needed for a **separate consumption model** — see §7.

Because `qmldir`'s file-mapping lines resolve relative to its own on-disk directory, the
actual `.qml` source files must be deployed alongside `qmldir` in the module directory, not
just referenced from the project's source tree.

## 4. `deploy.pri` — small, deliberately duplicated per project

```qmake
# deploy.pri
<ROOT_ENV_VAR> = $$(<ROOT_ENV_VAR>)
isEmpty(<ROOT_ENV_VAR>): <ROOT_ENV_VAR> = <DEFAULT_ROOT>

win32:CONFIG(debug, debug|release): DEPLOY_ROOT = $${<ROOT_ENV_VAR>}d
else:                                DEPLOY_ROOT = $$<ROOT_ENV_VAR>
DEPLOY_LIB_DIR = $$DEPLOY_ROOT/lib
```

Copy this file into every project that deploys into the shared tree, rather than reaching
it via a relative `include()` across project boundaries. Each project should be buildable
standalone without knowing where its sibling projects live on disk; a shared deploy
*convention* (same env var, same layout) achieves the consolidation without a shared build
dependency.

## 5. Deploying the plugin's own DLL/LIB: `QMAKE_POST_LINK`, never `COPIES`

```qmake
include(deploy.pri)

QMAKE_POST_LINK = cmd /c \
    "(if not exist $$shell_path($$DEPLOY_LIB_DIR) mkdir $$shell_path($$DEPLOY_LIB_DIR)) \
    && copy /y $$shell_path($$DESTDIR/<Target>.dll) $$shell_path($$DEPLOY_LIB_DIR) \
    && copy /y $$shell_path($$DESTDIR/<Target>.lib) $$shell_path($$DEPLOY_LIB_DIR)\\"

# qmldir (+ any mapped .qml files) IS safe to deploy via COPIES — it's a static
# source file, not this project's own build output.
<modulename>_qmldir_deploy.files = $$PWD/qmldir
<modulename>_qmldir_deploy.path  = $$DEPLOY_ROOT/<ModuleName>
COPIES += <modulename>_qmldir_deploy
```

**Rule: never point qmake's `COPIES` at a target this same `.pro` file just built.**
`COPIES` treats its `.files` as static, pre-existing sources and generates a qmake
dependency rule for them; pointing it at the project's own just-linked DLL creates a
circular dependency and qmake/nmake fails with `cycle in dependency tree for target
'...dll'`. Use `QMAKE_POST_LINK` (a plain post-build shell command) for anything that is
this project's own build output. `COPIES` remains the right tool — and is preferable, since
it's declaratively tracked — for genuinely static files like `qmldir` or `.qml` sources.

## 6. cmd.exe gotcha: `if not exist DIR mkdir DIR && next` silently drops `next`

This bit us in production: once `DIR` exists (i.e. every build after the very first),
`if not exist DIR mkdir DIR && copy ...` **silently skips the copy too, with exit code 0
and no error output.**

Why: cmd.exe parses `if COND CMD1 && CMD2 && CMD3 ...` as `if COND (CMD1 && CMD2 && CMD3
...)` — every `&&`-chained command becomes part of the `if`'s conditional body, not a
sibling of the whole `if` statement. When `COND` is false, *nothing* in that chain runs,
including everything after the `mkdir`.

**Fix: isolate the `if` in parentheses so it's an atomic step, not the start of the chain:**

```
(if not exist DIR mkdir DIR) && copy ...
```

Now the `if` block's own exit status doesn't gate the rest of the chain — `copy` always
runs (subject to the real exit code of whatever ran immediately before it). Apply this
anywhere a qmake `QMAKE_POST_LINK`/`system()` command mixes a conditional `mkdir` with
subsequent `&&`-chained steps. Verify by testing the *second* build, not just the first —
this bug is invisible until the directory already exists.

## 7. Companion `<target>_dll.pri` for C++ consumers linking the pre-built DLL

Separate from the plugin's own `.pro`, offer a small `.pri` that other projects `include()`
to link against the **already-built** DLL (not source-include the plugin's sources):

```qmake
# <target>_dll.pri
QT      += quick qml
CONFIG  += c++17
INCLUDEPATH += $$PWD
DEFINES     += "<TARGET>_HOME=\\\"$$PWD\\\"" <TARGET>_DLL

include($$PWD/deploy.pri)

LIBS += -L$$DEPLOY_LIB_DIR -l<Target>
<TARGET>_DLL_SRC = $$DEPLOY_LIB_DIR/<Target>.dll
win32:CONFIG(debug, debug|release): <TARGET>_DLL_DEST = $$OUT_PWD/debug
else:                                <TARGET>_DLL_DEST = $$OUT_PWD/release
!isEmpty(DESTDIR): <TARGET>_DLL_DEST = $$DESTDIR

# Copy the DLL next to whatever links against it — Windows searches the
# executable's own directory first, so this is needed even with lib/ on PATH.
<target>_dll_copy.files = $$<TARGET>_DLL_SRC
<target>_dll_copy.path  = $$<TARGET>_DLL_DEST
COPIES += <target>_dll_copy
```

This links against `$$DEPLOY_LIB_DIR`, i.e. the deploy tree — **not** the plugin project's
own source-relative build output — so any downstream consumer only ever needs to know the
deploy root, never the plugin's source location. The `.lib` deployed in §5 is what makes
this resolvable.

If a plugin's own DLL links against *another* plugin's DLL (e.g. B depends on A), B's `.pro`
should both `include()` A's `<target>_dll.pri` (to link) **and** its own `QMAKE_POST_LINK`
should defensively re-copy A's already-deployed `.dll`/`.lib` into the shared `lib/` (cheap,
and protects against B being rebuilt without A having been rebuilt first).

## 8. Windows won't auto-search a loaded plugin's own directory for its dependencies

If plugin B's DLL depends on plugin A's DLL, and both sit in the same shared `lib/`, that's
*not* automatically enough for `import B 1.0` to resolve at runtime. Windows' standard
`LoadLibrary` search order does not include the *loading* DLL's own directory when
resolving *that DLL's* dependencies — only the main EXE's directory, `PATH`, and system
directories are searched by default. Consuming applications must add the shared `lib/`
directory to `PATH` (or the deploy root generally, since Qt's own import-path resolution
is separate from Windows' DLL search order).

## 9. Verification discipline

- **A `.pro`/deploy change that "looks right" isn't verified until it's rebuilt with the
  artifact actually forced to relink.** `nmake` won't re-run `QMAKE_POST_LINK` if it thinks
  the target is already up to date relative to its object files — a changed
  `QMAKE_POST_LINK` string alone doesn't trigger a relink. Delete the built `.dll`/`.lib`
  (not the whole build tree) to force one cheaply.
- **Test the *second* build, not just the first**, for anything involving `if not exist DIR
  ...` — see §6. A first-build-only test can pass while silently broken on every build after.
- **Don't rely on `qml.exe` for a real end-to-end plugin-load test** — its built-in
  `default.qml` configuration wraps content in assumptions (visual root object, specific
  windowing setup) that can fail with an unhelpful "Did not load any objects, exiting."
  even when the actual `import X 1.0` resolution is fine. A tiny throwaway host
  (`QGuiApplication` + `QQmlApplicationEngine`, `engine.addImportPath(...)`,
  `engine.load(...)`, then check `rootObjects()` is non-empty **and** listen to the
  `warnings()` signal) is far more reliable and gives real diagnostics.
- **Validate the test harness itself with a negative control** before trusting a "passing"
  result — e.g. point it at a directory that deliberately lacks the module, and confirm it
  reports failure clearly. A harness that reports success no matter what proves nothing.
- **Give any throwaway diagnostic host app a console subsystem**
  (`CONFIG += console` / `CONFIG -= app_bundle`) — a default GUI-subsystem Qt app has no
  attached console, so `qDebug()`/`printf`/`console.log` output vanishes silently even
  though the process runs and exits correctly. Exit code alone is trustworthy; captured
  stdout/stderr is not, until this is set.
- **`.bat` files must be CRLF, not LF.** A plain UTF-8 file write typically produces
  LF-only line endings, which corrupts cmd.exe's batch parser — especially around comment
  lines and non-ASCII characters — with confusing, seemingly-unrelated "not recognized as an
  internal or external command" errors. Convert to CRLF (e.g. `sed -i 's/$/\r/' file.bat`)
  and re-test after writing or editing any `.bat` file.
