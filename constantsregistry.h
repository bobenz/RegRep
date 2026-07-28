#pragma once
#include <QString>
#include <QMap>
#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QDebug>
#include <QQmlPropertyMap>

#include "regrep_global.h"

struct ConstantEntryNoRegTag {};

// ── Composite key ─────────────────────────────────────────────────────────────
//
// Entries are keyed by (type, code, source) so that different kinds of
// constants (errors, config values, limits, ...) and different subsystems can
// all reuse the same numeric code without collision — e.g. error code -100 on
// CDM vs config code -100 on APP vs error code -100 on PTR.

struct ConstantKey
{
    QString type;
    int     code   { 0 };
    QString source;

    bool operator<(const ConstantKey &o) const
    {
        if (type != o.type) return type < o.type;
        if (code != o.code) return code < o.code;
        return source < o.source;
    }
    bool operator==(const ConstantKey &o) const
    {
        return type == o.type && code == o.code && source == o.source;
    }
};

// ── ConstantEntry ────────────────────────────────────────────────────────────
//
// A generic named constant: { type: "error", name, source, code, description,
// data }. "type" categorizes what kind of constant this is ("error" is just
// the default/most common one); "data" is an arbitrary free-form payload
// (numbers, strings, or a JS object/array passed in from QML).

class REGREP_EXPORT ConstantEntry
{
    Q_GADGET
    Q_PROPERTY(QString  type        READ type)
    Q_PROPERTY(QString  source      READ source)
    Q_PROPERTY(int      code        READ code)
    Q_PROPERTY(QString  description READ description)
    Q_PROPERTY(QVariant data        READ data)
    Q_PROPERTY(bool     valid       READ isValid)
    Q_PROPERTY(QString  text        READ toString)

public:
    ConstantEntry() = default;

    // Self-registering — for static instances in constant-definition headers.
    ConstantEntry(const QString &name,
                  const QString &source,
                  int            code,
                  const QString &description,
                  const QString &type = QStringLiteral("error"),
                  const QVariant &data = QVariant());

    // Non-registering — used by ConstantRegistry::create() / declare() internally.
    ConstantEntry(const QString &name,
                  const QString &source,
                  int            code,
                  const QString &description,
                  const QString &type,
                  const QVariant &data,
                  ConstantEntryNoRegTag);

    // Unambiguous lookup constructor.
    // Finds the entry with exactly this (type, code, source) triple.
    inline ConstantEntry(const QString &type, int code, const QString &source);

    // Legacy lookup constructor — type defaults to "error", so pre-existing
    // ErrorEntry(code, source)-style call sites keep working unchanged.
    // Searches across types when more than one type shares (code, source);
    // logs a qWarning on ambiguity, same rule as ConstantRegistry::lookup(code, source).
    inline ConstantEntry(int code, const QString &source);

    // Very-legacy lookup constructor — searches across all types and sources.
    // Succeeds silently when exactly one entry carries this code; logs a
    // qWarning when multiple (type, source) pairs share the code.
    inline explicit ConstantEntry(int code);

    QString  name()        const { return m_name; }
    QString  type()        const { return m_type; }
    QString  source()      const { return m_source; }
    int      code()        const { return m_code; }
    QString  description() const { return m_description; }
    QVariant data()        const { return m_data; }
    bool     isValid()     const { return !m_name.isEmpty(); }

    Q_INVOKABLE QString toString() const
    {
        if (!isValid()) return QStringLiteral("<unknown %1>").arg(m_type.isEmpty() ? QStringLiteral("constant") : m_type);
        return QString("[%1/%2] (%3) %4").arg(m_type, m_source).arg(m_code).arg(m_description);
    }

private:
    QString  m_name;
    QString  m_type;
    QString  m_source;
    int      m_code        { 0 };
    QString  m_description;
    QVariant m_data;
};

Q_DECLARE_METATYPE(ConstantEntry)

inline bool operator==(const ConstantEntry &lhs, const ConstantEntry &rhs)
{
    return lhs.type() == rhs.type() && lhs.code() == rhs.code() && lhs.source() == rhs.source();
}
inline bool operator!=(const ConstantEntry &lhs, const ConstantEntry &rhs)
{
    return !(lhs == rhs);
}

// ── ConstantRegistry ──────────────────────────────────────────────────────────

class REGREP_EXPORT ConstantRegistry : public QObject
{
    Q_OBJECT

public:
    static ConstantRegistry &instance()
    {
        static ConstantRegistry s_instance;
        return s_instance;
    }

    // Returns the QQmlPropertyMap to expose as e.g. the "Constants" context property.
    QQmlPropertyMap *map() { return &m_map; }

    // ── Registration ─────────────────────────────────────────────────────────

    void registerEntry(const ConstantEntry &entry)
    {
        ConstantKey key{ entry.type(), entry.code(), entry.source() };
        m_byKey.insert(key, entry);
        m_byName.insert(entry.name(), entry);
        m_map.insert(entry.name(), QVariant::fromValue(entry));
    }

    Q_INVOKABLE void create(const QString &name,
                             const QString &source,
                             int            code,
                             const QString &description,
                             const QString &type = QStringLiteral("error"),
                             const QVariant &data = QVariant())
    {
        ConstantEntry e(name, source, code, description, type, data, ConstantEntryNoRegTag{});
        ConstantKey key{ type, code, source };
        m_byKey.insert(key, e);
        m_byName.insert(name, e);
        m_map.insert(name, QVariant::fromValue(e));
    }

    // Accepts a JS object literal from QML:
    // { name: "shutter_stuck", source: "CDM", code: -2001,
    //   description: "Shutter mechanism stuck", type: "error", data: {...} }
    // "type" defaults to "error" and "data" defaults to an empty QVariant when omitted.
    Q_INVOKABLE void declare(const QVariantMap &def)
    {
        const QString name   = def.value("name").toString();
        const QString source = def.value("source").toString();
        const int     code   = def.value("code").toInt();
        const QString desc   = def.value("description").toString();
        const QString type   = def.contains("type") ? def.value("type").toString() : QStringLiteral("error");
        const QVariant data  = def.value("data");

        if (name.isEmpty() || source.isEmpty()) {
            qWarning() << "[ConstantRegistry] declare(): missing name or source in" << def;
            return;
        }

        ConstantEntry e(name, source, code, desc, type, data, ConstantEntryNoRegTag{});
        ConstantKey key{ type, code, source };
        m_byKey.insert(key, e);
        m_byName.insert(name, e);
        m_map.insert(name, QVariant::fromValue(e));
    }

    // ── Lookup ───────────────────────────────────────────────────────────────

    // Preferred: unambiguous lookup by (type, code, source).
    Q_INVOKABLE QVariant lookup(const QString &type, int code, const QString &source) const
    {
        return QVariant::fromValue(m_byKey.value({ type, code, source }, ConstantEntry{}));
    }

    // Legacy: lookup by (code, source), type defaults to "error" search-wise —
    // i.e. it searches across all types sharing this (code, source). Succeeds
    // unambiguously when exactly one type matches (the common case); logs a
    // qWarning when multiple types collide on the same (code, source).
    Q_INVOKABLE QVariant lookup(int code, const QString &source) const
    {
        return QVariant::fromValue(uniqueByCodeSource(code, source));
    }

    // Very legacy: lookup by code only. Succeeds if exactly one entry carries
    // that code across all (type, source) pairs; logs a warning otherwise.
    Q_INVOKABLE QVariant lookup(int code) const
    {
        return QVariant::fromValue(uniqueByCode(code));
    }

    Q_INVOKABLE QVariant lookupByName(const QString &name) const
    {
        return QVariant::fromValue(m_byName.value(name, ConstantEntry{}));
    }

    // Preferred describe / contains overloads.
    Q_INVOKABLE QString describe(const QString &type, int code, const QString &source) const
    {
        return m_byKey.value({ type, code, source }, ConstantEntry{}).toString();
    }

    Q_INVOKABLE QString describe(int code, const QString &source) const
    {
        return uniqueByCodeSource(code, source).toString();
    }

    Q_INVOKABLE QString describe(int code) const
    {
        return uniqueByCode(code).toString();
    }

    Q_INVOKABLE bool contains(const QString &type, int code, const QString &source) const
    {
        return m_byKey.contains({ type, code, source });
    }

    Q_INVOKABLE bool contains(int code, const QString &source) const
    {
        return !matchingByCodeSource(code, source).isEmpty();
    }

    Q_INVOKABLE bool contains(int code) const
    {
        return !matchingByCode(code).isEmpty();
    }

    Q_INVOKABLE bool containsName(const QString &name) const
    {
        return m_byName.contains(name);
    }

private:
    ConstantRegistry(QObject *parent = nullptr) : QObject(parent) {}

    // Collect all entries whose (code, source) matches, ignoring type.
    QList<ConstantEntry> matchingByCodeSource(int code, const QString &source) const
    {
        QList<ConstantEntry> result;
        for (auto it = m_byKey.cbegin(); it != m_byKey.cend(); ++it) {
            if (it.key().code == code && it.key().source == source)
                result.append(it.value());
        }
        return result;
    }

    // Collect all entries whose numeric code matches, ignoring type and source.
    QList<ConstantEntry> matchingByCode(int code) const
    {
        QList<ConstantEntry> result;
        for (auto it = m_byKey.cbegin(); it != m_byKey.cend(); ++it) {
            if (it.key().code == code)
                result.append(it.value());
        }
        return result;
    }

    ConstantEntry uniqueByCodeSource(int code, const QString &source) const
    {
        const QList<ConstantEntry> matches = matchingByCodeSource(code, source);
        if (matches.isEmpty())
            return ConstantEntry{};
        if (matches.size() > 1) {
            QStringList types;
            for (const ConstantEntry &e : matches)
                types << e.type();
            qWarning() << "[ConstantRegistry] lookup(" << code << "," << source << ") is ambiguous — "
                                                                                     "multiple types registered this (code, source):" << types
                       << "— use lookup(type, code, source) for an unambiguous result.";
        }
        return matches.first();
    }

    // Returns the unique entry for a bare code, or warns + returns invalid.
    ConstantEntry uniqueByCode(int code) const
    {
        const QList<ConstantEntry> matches = matchingByCode(code);
        if (matches.isEmpty()) {
            return ConstantEntry{};
        }
        if (matches.size() > 1) {
            QStringList sources;
            for (const ConstantEntry &e : matches)
                sources << QString("%1/%2").arg(e.type(), e.source());
            qWarning() << "[ConstantRegistry] lookup(" << code << ") is ambiguous — "
                                                                   "multiple type/source pairs registered this code:" << sources
                       << "— use lookup(type, code, source) for an unambiguous result.";
        }
        return matches.first();
    }

    QMap<ConstantKey, ConstantEntry> m_byKey;   // primary store: (type, code, source) → entry
    QMap<QString,     ConstantEntry> m_byName;  // secondary: name → entry
    QQmlPropertyMap                  m_map;     // QML "Constants.*" property map
};

// ── Inline constructors ───────────────────────────────────────────────────────

inline ConstantEntry::ConstantEntry(const QString &name,
                                     const QString &source,
                                     int            code,
                                     const QString &description,
                                     const QString &type,
                                     const QVariant &data)
    : m_name(name), m_type(type), m_source(source), m_code(code), m_description(description), m_data(data)
{
    ConstantRegistry::instance().registerEntry(*this);
}

inline ConstantEntry::ConstantEntry(const QString &name,
                                     const QString &source,
                                     int            code,
                                     const QString &description,
                                     const QString &type,
                                     const QVariant &data,
                                     ConstantEntryNoRegTag)
    : m_name(name), m_type(type), m_source(source), m_code(code), m_description(description), m_data(data)
{
    // intentionally does NOT call registerEntry
}

inline ConstantEntry::ConstantEntry(const QString &type, int code, const QString &source)
{
    ConstantRegistry &reg = ConstantRegistry::instance();
    if (reg.contains(type, code, source)) {
        *this = reg.lookup(type, code, source).value<ConstantEntry>();
    } else {
        qWarning() << "[ConstantEntry] No entry found for type" << type << "code" << code
                   << "source" << source << "— leaving invalid.";
        m_type   = type;
        m_source = source;
        m_code   = code;
        // m_name stays empty → isValid() == false, toString() → "<unknown ...>"
    }
}

inline ConstantEntry::ConstantEntry(int code, const QString &source)
{
    // Same ambiguity-warning logic as ConstantRegistry::lookup(code, source):
    // succeeds silently for a unique type match, warns when multiple types
    // share (code, source), leaves isValid()==false when nothing is registered.
    ConstantRegistry &reg = ConstantRegistry::instance();
    QVariant v = reg.lookup(code, source);
    if (v.isValid())
        *this = v.value<ConstantEntry>();
    else {
        m_source = source;
        m_code   = code;
    }
}

inline ConstantEntry::ConstantEntry(int code)
{
    // Same ambiguity-warning logic as ConstantRegistry::lookup(int):
    // succeeds silently for a unique match, warns when multiple type/source
    // pairs share the code, leaves isValid()==false when nothing is registered.
    ConstantRegistry &reg = ConstantRegistry::instance();
    QVariant v = reg.lookup(code);
    if (v.isValid())
        *this = v.value<ConstantEntry>();
    // else: default-constructed → isValid() == false
}

// ── Built-in sentinel entries ─────────────────────────────────────────────────

inline ConstantEntry NoError     ("no_error",      "APP", 0,        "No error");
inline ConstantEntry UnknownError("unknown_error", "SYS", -1000000, "Unknown error code");
