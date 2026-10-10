#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "shortcut_entry.h"

class ShortcutManager : public QObject {
    Q_OBJECT

public:
    explicit ShortcutManager(QObject *parent = nullptr);

    // Reload all shortcuts from DBus, configs, and system definitions
    void reload();

    // Get all loaded shortcuts
    const QList<ShortcutEntry> &allShortcuts() const { return m_allShortcuts; }

    // Filter shortcuts by search query
    QList<ShortcutEntry> filter(const QString &query) const;

    // Get total count
    int totalCount() const { return m_allShortcuts.size(); }

    // Helper to find icon for a component
    static QString resolveIconForComponent(const QString &componentUnique, const QString &componentFriendly);

private:
    void loadFromDBus();
    void loadFromConfig();
    void loadFromDesktopFiles();
    void addStandardShortcuts();
    void buildSearchIndex(ShortcutEntry &entry);

    QList<ShortcutEntry> m_allShortcuts;
    QMap<QString, int> m_entryIndexMap; // key: "componentUnique/actionUnique" -> index in m_allShortcuts
};

#endif // SHORTCUT_MANAGER_H
