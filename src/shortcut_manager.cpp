#include "shortcut_manager.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QKeySequence>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <algorithm>

struct RawShortcutInfo {
    QString uniqueName;
    QString friendlyName;
    QString componentUnique;
    QString componentFriendly;
    QString contextUnique;
    QString contextFriendly;
    QList<int> keys;
    QList<int> defaultKeys;
};

QDBusArgument &operator>>(const QDBusArgument &argument, RawShortcutInfo &info) {
    argument.beginStructure();
    argument >> info.uniqueName;
    argument >> info.friendlyName;
    argument >> info.componentUnique;
    argument >> info.componentFriendly;
    argument >> info.contextUnique;
    argument >> info.contextFriendly;
    
    argument.beginArray();
    info.keys.clear();
    while (!argument.atEnd()) {
        int k;
        argument >> k;
        info.keys.append(k);
    }
    argument.endArray();

    argument.beginArray();
    info.defaultKeys.clear();
    while (!argument.atEnd()) {
        int k;
        argument >> k;
        info.defaultKeys.append(k);
    }
    argument.endArray();

    argument.endStructure();
    return const_cast<QDBusArgument&>(argument);
}

ShortcutManager::ShortcutManager(QObject *parent)
    : QObject(parent)
{
    reload();
}

void ShortcutManager::reload()
{
    m_allShortcuts.clear();
    m_entryIndexMap.clear();

    loadFromDBus();
    loadFromConfig();
    loadFromDesktopFiles();
    addStandardShortcuts();

    // Sort entries alphabetically by component, then by action
    std::sort(m_allShortcuts.begin(), m_allShortcuts.end(), [](const ShortcutEntry &a, const ShortcutEntry &b) {
        if (a.componentFriendly != b.componentFriendly) {
            return a.componentFriendly.localeAwareCompare(b.componentFriendly) < 0;
        }
        return a.actionFriendly.localeAwareCompare(b.actionFriendly) < 0;
    });

    // Rebuild index map after sorting
    m_entryIndexMap.clear();
    for (int i = 0; i < m_allShortcuts.size(); ++i) {
        const auto &e = m_allShortcuts.at(i);
        QString key = e.componentUnique + "/" + e.actionUnique;
        m_entryIndexMap[key] = i;
    }
}

void ShortcutManager::loadFromDBus()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel",
        "/kglobalaccel",
        "org.kde.KGlobalAccel",
        "allComponents"
    );

    QDBusReply<QList<QDBusObjectPath>> reply = bus.call(call);
    if (!reply.isValid()) {
        return;
    }

    for (const auto &path : reply.value()) {
        QDBusMessage compCall = QDBusMessage::createMethodCall(
            "org.kde.kglobalaccel",
            path.path(),
            "org.kde.kglobalaccel.Component",
            "allShortcutInfos"
        );

        QDBusMessage compReply = bus.call(compCall);
        if (compReply.type() != QDBusMessage::ReplyMessage || compReply.arguments().isEmpty()) {
            continue;
        }

        const QDBusArgument arg = compReply.arguments().at(0).value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            RawShortcutInfo raw;
            arg >> raw;

            // Extract active keys
            QStringList activeKeyStrs;
            for (int k : raw.keys) {
                if (k != 0) {
                    QString str = QKeySequence(k).toString(QKeySequence::NativeText);
                    if (!str.isEmpty() && !activeKeyStrs.contains(str)) {
                        activeKeyStrs.append(str);
                    }
                }
            }

            QStringList defaultKeyStrs;
            for (int k : raw.defaultKeys) {
                if (k != 0) {
                    QString str = QKeySequence(k).toString(QKeySequence::NativeText);
                    if (!str.isEmpty() && !defaultKeyStrs.contains(str)) {
                        defaultKeyStrs.append(str);
                    }
                }
            }

            // Only display shortcuts that have an assigned shortcut (either active or default)
            if (activeKeyStrs.isEmpty() && defaultKeyStrs.isEmpty()) {
                continue;
            }

            ShortcutEntry entry;
            entry.componentUnique = raw.componentUnique;
            entry.componentFriendly = !raw.componentFriendly.trimmed().isEmpty() 
                                      ? raw.componentFriendly 
                                      : raw.componentUnique;
            entry.actionUnique = raw.uniqueName;

            QString actName = !raw.friendlyName.trimmed().isEmpty() 
                              ? raw.friendlyName 
                              : raw.uniqueName;
            if (actName.startsWith('_')) {
                if (actName == "_launch") {
                    actName = "Launch " + entry.componentFriendly;
                } else {
                    actName.remove(0, 1);
                }
            }
            entry.actionFriendly = actName;

            if (!activeKeyStrs.isEmpty()) {
                entry.shortcut = activeKeyStrs.at(0);
                if (activeKeyStrs.size() > 1) {
                    entry.secondaryShortcut = activeKeyStrs.at(1);
                }
            } else if (!defaultKeyStrs.isEmpty()) {
                entry.shortcut = defaultKeyStrs.at(0);
                if (defaultKeyStrs.size() > 1) {
                    entry.secondaryShortcut = defaultKeyStrs.at(1);
                }
            }

            if (!defaultKeyStrs.isEmpty()) {
                entry.defaultShortcut = defaultKeyStrs.join(", ");
            }

            // Check if user changed it from default
            entry.isCustom = (raw.keys != raw.defaultKeys) && !defaultKeyStrs.isEmpty() && !activeKeyStrs.isEmpty();

            entry.iconName = resolveIconForComponent(entry.componentUnique, entry.componentFriendly);
            buildSearchIndex(entry);

            QString key = entry.componentUnique + "/" + entry.actionUnique;
            m_entryIndexMap[key] = m_allShortcuts.size();
            m_allShortcuts.append(entry);
        }
        arg.endArray();
    }
}

void ShortcutManager::loadFromConfig()
{
    QString configPath = QDir::homePath() + "/.config/kglobalshortcutsrc";
    if (!QFileInfo::exists(configPath)) {
        return;
    }

    QSettings settings(configPath, QSettings::IniFormat);
    const QStringList groups = settings.childGroups();

    for (const QString &group : groups) {
        settings.beginGroup(group);
        QString friendlyComp = settings.value("_k_friendly_name", group).toString();
        const QStringList keys = settings.childKeys();

        for (const QString &k : keys) {
            if (k.startsWith('_')) continue;

            QString val = settings.value(k).toString();
            // Format: current,default,description
            QStringList parts = val.split(',');
            QString cur = parts.value(0).trimmed();
            QString def = parts.value(1).trimmed();
            QString desc = parts.value(2).trimmed();

            if (cur == "none" && def == "none") {
                continue;
            }

            QString primaryShort = (cur != "none" && !cur.isEmpty()) ? cur : def;
            if (primaryShort == "none" || primaryShort.isEmpty()) {
                continue;
            }

            // Split tab-separated multi-shortcuts
            QStringList shortList = primaryShort.split(QRegularExpression("[\\t]"), Qt::SkipEmptyParts);
            QString sc1 = shortList.value(0).trimmed();
            QString sc2 = shortList.size() > 1 ? shortList.value(1).trimmed() : QString();

            QString actDesc = !desc.isEmpty() ? desc : k;

            QString mapKey = group + "/" + k;
            if (m_entryIndexMap.contains(mapKey)) {
                // Already in list from DBus, keep DBus entry as primary
                continue;
            }

            ShortcutEntry entry;
            entry.componentUnique = group;
            entry.componentFriendly = friendlyComp;
            entry.actionUnique = k;
            entry.actionFriendly = actDesc;
            entry.shortcut = sc1;
            entry.secondaryShortcut = sc2;
            entry.defaultShortcut = def;
            entry.isCustom = (cur != def && cur != "none" && def != "none");
            entry.iconName = resolveIconForComponent(group, friendlyComp);
            buildSearchIndex(entry);

            m_entryIndexMap[mapKey] = m_allShortcuts.size();
            m_allShortcuts.append(entry);
        }
        settings.endGroup();
    }
}

void ShortcutManager::loadFromDesktopFiles()
{
    QStringList appDirs = {
        "/usr/share/applications",
        QDir::homePath() + "/.local/share/applications"
    };

    for (const QString &dirPath : appDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        const auto entries = dir.entryInfoList({"*.desktop"}, QDir::Files);
        for (const auto &fileInfo : entries) {
            QSettings desktopFile(fileInfo.absoluteFilePath(), QSettings::IniFormat);
            desktopFile.beginGroup("Desktop Entry");
            
            QString shortcuts = desktopFile.value("X-KDE-Shortcuts").toString();
            if (shortcuts.isEmpty()) {
                desktopFile.endGroup();
                continue;
            }

            QString name = desktopFile.value("Name").toString();
            QString comment = desktopFile.value("Comment").toString();
            QString icon = desktopFile.value("Icon").toString();
            desktopFile.endGroup();

            QString appId = fileInfo.baseName();
            QString mapKey = appId + "/_launch";
            if (m_entryIndexMap.contains(mapKey) || m_entryIndexMap.contains("services][" + fileInfo.fileName() + "/_launch")) {
                continue;
            }

            QStringList scList = shortcuts.split(',', Qt::SkipEmptyParts);
            if (scList.isEmpty()) continue;

            ShortcutEntry entry;
            entry.componentUnique = appId;
            entry.componentFriendly = !name.isEmpty() ? name : appId;
            entry.actionUnique = "_launch";
            entry.actionFriendly = !comment.isEmpty() ? comment : ("Launch " + entry.componentFriendly);
            entry.shortcut = scList.at(0).trimmed();
            if (scList.size() > 1) {
                entry.secondaryShortcut = scList.at(1).trimmed();
            }
            entry.defaultShortcut = entry.shortcut;
            entry.isCustom = false;
            entry.iconName = !icon.isEmpty() ? icon : resolveIconForComponent(appId, entry.componentFriendly);
            buildSearchIndex(entry);

            m_entryIndexMap[mapKey] = m_allShortcuts.size();
            m_allShortcuts.append(entry);
        }
    }
}

void ShortcutManager::addStandardShortcuts()
{
    struct StdItem {
        const char *action;
        const char *shortcut;
        const char *category;
    };

    static const StdItem stdItems[] = {
        {"Copy", "Ctrl+C", "Standard Clipboard"},
        {"Cut", "Ctrl+X", "Standard Clipboard"},
        {"Paste", "Ctrl+V", "Standard Clipboard"},
        {"Select All", "Ctrl+A", "Standard Editing"},
        {"Undo", "Ctrl+Z", "Standard Editing"},
        {"Redo", "Ctrl+Shift+Z", "Standard Editing"},
        {"Find / Search", "Ctrl+F", "Standard Navigation"},
        {"Find Next", "F3", "Standard Navigation"},
        {"Find Previous", "Shift+F3", "Standard Navigation"},
        {"Replace", "Ctrl+R", "Standard Editing"},
        {"New Tab / Document", "Ctrl+N", "Standard Document"},
        {"Open Document / File", "Ctrl+O", "Standard Document"},
        {"Save", "Ctrl+S", "Standard Document"},
        {"Save As", "Ctrl+Shift+S", "Standard Document"},
        {"Print", "Ctrl+P", "Standard Document"},
        {"Close Tab / Window", "Ctrl+W", "Standard Window"},
        {"Quit Application", "Ctrl+Q", "Standard Application"},
        {"Fullscreen Toggle", "F11", "Standard Window"},
        {"Help / Documentation", "F1", "Standard System"},
        {"Preferences / Settings", "Ctrl+Shift+,", "Standard Application"},
        {"Zoom In", "Ctrl++", "Standard View"},
        {"Zoom Out", "Ctrl+-", "Standard View"},
        {"Reset Zoom", "Ctrl+0", "Standard View"},
    };

    for (const auto &item : stdItems) {
        ShortcutEntry entry;
        entry.componentUnique = "standard_shortcuts";
        entry.componentFriendly = item.category;
        entry.actionUnique = item.action;
        entry.actionFriendly = item.action;
        entry.shortcut = item.shortcut;
        entry.defaultShortcut = item.shortcut;
        entry.isCustom = false;
        entry.iconName = "input-keyboard";
        buildSearchIndex(entry);

        m_allShortcuts.append(entry);
    }
}

void ShortcutManager::buildSearchIndex(ShortcutEntry &entry)
{
    entry.indexedKeys.clear();
    entry.indexedWords.clear();

    // 1. Index keys
    auto addKey = [&](const QString &k) {
        QString keyNorm = k.trimmed().toLower();
        if (keyNorm.isEmpty()) return;
        if (!entry.indexedKeys.contains(keyNorm)) {
            entry.indexedKeys.append(keyNorm);
        }
        if (keyNorm == "meta") {
            if (!entry.indexedKeys.contains("super")) entry.indexedKeys.append("super");
            if (!entry.indexedKeys.contains("win")) entry.indexedKeys.append("win");
            if (!entry.indexedKeys.contains("cmd")) entry.indexedKeys.append("cmd");
        } else if (keyNorm == "ctrl" || keyNorm == "control") {
            if (!entry.indexedKeys.contains("ctrl")) entry.indexedKeys.append("ctrl");
            if (!entry.indexedKeys.contains("control")) entry.indexedKeys.append("control");
        } else if (keyNorm == "alt" || keyNorm == "opt" || keyNorm == "option") {
            if (!entry.indexedKeys.contains("alt")) entry.indexedKeys.append("alt");
            if (!entry.indexedKeys.contains("option")) entry.indexedKeys.append("option");
        } else if (keyNorm == "return" || keyNorm == "enter") {
            if (!entry.indexedKeys.contains("return")) entry.indexedKeys.append("return");
            if (!entry.indexedKeys.contains("enter")) entry.indexedKeys.append("enter");
        } else if (keyNorm == "print" || keyNorm == "prtscn" || keyNorm == "prtscr") {
            if (!entry.indexedKeys.contains("print")) entry.indexedKeys.append("print");
            if (!entry.indexedKeys.contains("prtscn")) entry.indexedKeys.append("prtscn");
        }
    };

    for (const QString &k : entry.primaryKeys()) {
        addKey(k);
    }
    for (const QString &k : entry.secondaryKeys()) {
        addKey(k);
    }

    // 2. Index words from component and action
    auto addWordsFromText = [&](const QString &text) {
        QStringList words = text.toLower().split(QRegularExpression("[^a-zA-Z0-9]+"), Qt::SkipEmptyParts);
        for (const QString &w : words) {
            if (!entry.indexedWords.contains(w)) {
                entry.indexedWords.append(w);
            }
        }
    };

    addWordsFromText(entry.componentFriendly);
    addWordsFromText(entry.componentUnique);
    addWordsFromText(entry.actionFriendly);
    addWordsFromText(entry.actionUnique);

    // Contextual aliases
    QString comp = entry.componentFriendly.toLower();
    QString act = entry.actionFriendly.toLower();

    if (comp.contains("spectacle") || act.contains("capture") || entry.indexedKeys.contains("print")) {
        addWordsFromText("screenshot capture screen recording snip snippet");
    }
    if (comp.contains("konsole") || comp.contains("terminal")) {
        addWordsFromText("terminal bash shell console cmd cli");
    }
    if (comp.contains("dolphin") || act.contains("file")) {
        addWordsFromText("file manager files folder explorer directory");
    }
    if (comp.contains("system settings")) {
        addWordsFromText("preferences config configuration options control panel");
    }
    if (act.contains("overview")) {
        addWordsFromText("expose mission control windows workspaces");
    }
    if (comp.contains("volume") || comp.contains("kmix") || act.contains("volume")) {
        addWordsFromText("sound audio speaker mic microphone louder quieter");
    }
    if (act.contains("tile")) {
        addWordsFromText("tiling snap split grid");
    }
}

QList<ShortcutEntry> ShortcutManager::filter(const QString &query) const
{
    QString q = query.trimmed().toLower();
    if (q.isEmpty()) {
        return m_allShortcuts;
    }

    // Replace '+' and '-' with space for combination typing
    q.replace('+', ' ');
    q.replace('-', ' ');

    QStringList tokens = q.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return m_allShortcuts;
    }

    QList<ShortcutEntry> matches;
    matches.reserve(m_allShortcuts.size());

    for (const auto &entry : m_allShortcuts) {
        bool allMatch = true;

        for (const QString &tok : tokens) {
            bool tokMatch = false;

            // 1. Check if token matches any indexed key
            if (entry.indexedKeys.contains(tok)) {
                tokMatch = true;
            }
            // 2. If token is more than 1 char, also search words in action/component
            else if (tok.length() > 1) {
                for (const QString &w : entry.indexedWords) {
                    if (w.contains(tok)) {
                        tokMatch = true;
                        break;
                    }
                }
            }

            if (!tokMatch) {
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            matches.append(entry);
        }
    }

    return matches;
}

QString ShortcutManager::resolveIconForComponent(const QString &componentUnique, const QString &componentFriendly)
{
    QString comp = componentUnique.toLower();
    QString friendly = componentFriendly.toLower();

    if (comp.contains("konsole") || friendly.contains("konsole") || comp.contains("terminal")) return "utilities-terminal";
    if (comp.contains("dolphin") || friendly.contains("dolphin") || comp.contains("file")) return "system-file-manager";
    if (comp.contains("spectacle") || friendly.contains("spectacle") || comp.contains("screenshot")) return "spectacle";
    if (comp.contains("kwin")) return "preferences-system-windows";
    if (comp.contains("kmix") || comp.contains("volume") || comp.contains("audio")) return "audio-volume-high";
    if (comp.contains("media")) return "media-playback-start";
    if (comp.contains("power") || comp.contains("powerdevil")) return "preferences-system-power-management";
    if (comp.contains("ksmserver") || comp.contains("session")) return "system-log-out";
    if (comp.contains("plasmashell")) return "kde";
    if (comp.contains("krunner")) return "krunner";
    if (comp.contains("kcalc") || comp.contains("calc")) return "accessories-calculator";
    if (comp.contains("systemmonitor") || comp.contains("system-monitor")) return "utilities-system-monitor";
    if (comp.contains("settings")) return "preferences-system";
    if (comp.contains("touchpad")) return "input-touchpad";
    if (comp.contains("screen") || comp.contains("display")) return "video-display";
    if (comp.contains("emoji")) return "accessories-character-map";
    if (comp.contains("standard")) return "input-keyboard";

    return "preferences-desktop-keyboard-shortcuts";
}
