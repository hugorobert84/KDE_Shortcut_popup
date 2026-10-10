#ifndef SHORTCUT_ENTRY_H
#define SHORTCUT_ENTRY_H

#include <QString>
#include <QStringList>

struct ShortcutEntry {
    QString componentUnique;   // e.g. "kwin", "org.kde.konsole.desktop"
    QString componentFriendly; // e.g. "KWin", "Konsole"
    QString actionUnique;      // e.g. "Overview", "NewTab"
    QString actionFriendly;    // e.g. "Toggle Overview", "Open New Tab"
    QString shortcut;          // Primary shortcut e.g. "Meta+W"
    QString secondaryShortcut; // Alternate shortcut e.g. "Alt+Tab"
    QString defaultShortcut;   // Default shortcut
    bool isCustom = false;     // True if modified by user from default
    QString iconName;          // Icon name e.g. "utilities-terminal", "kwin"
    QString searchNormalized;  // Pre-computed lowercase search index
    QStringList indexedKeys;   // Keys with aliases e.g. ["meta", "super", "win", "w"]
    QStringList indexedWords;  // Words in action & component e.g. ["kwin", "toggle", "overview"]

    // Helper to get formatted list of keys for badge display
    QStringList primaryKeys() const {
        if (shortcut.isEmpty() || shortcut.compare("none", Qt::CaseInsensitive) == 0) {
            return QStringList();
        }
        return shortcut.split('+', Qt::SkipEmptyParts);
    }

    QStringList secondaryKeys() const {
        if (secondaryShortcut.isEmpty() || secondaryShortcut.compare("none", Qt::CaseInsensitive) == 0) {
            return QStringList();
        }
        return secondaryShortcut.split('+', Qt::SkipEmptyParts);
    }
};

#endif // SHORTCUT_ENTRY_H
