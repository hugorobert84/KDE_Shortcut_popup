#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include "popup_window.h"

int main(int argc, char *argv[])
{
    // Enable High DPI scaling
    QApplication app(argc, argv);
    app.setApplicationName("KDE Shortcut Popup");
    app.setApplicationDisplayName("KDE Shortcut Popup");
    app.setOrganizationDomain("kde.org");

    QStringList args = app.arguments();
    if (args.contains("--help") || args.contains("-h")) {
        QTextStream out(stdout);
        out << "KDE Shortcut Popup - Minimalist shortcut reference for KDE Plasma\n\n";
        out << "Usage:\n";
        out << "  kde-shortcut-popup [options]\n\n";
        out << "Options:\n";
        out << "  -h, --help     Show this help message\n";
        out << "  -l, --list     Print all discovered shortcuts to standard output\n";
        out << "  -t, --toggle   Toggle popup (if already open, closes it; otherwise opens)\n";
        return 0;
    }

    if (args.contains("--list") || args.contains("-l")) {
        ShortcutManager mgr;
        QTextStream out(stdout);
        out << "Discovered " << mgr.totalCount() << " shortcuts across all KDE components:\n\n";
        for (const auto &entry : mgr.allShortcuts()) {
            out << QString("[%1] %2 => %3%4\n")
                   .arg(entry.componentFriendly, -24)
                   .arg(entry.actionFriendly, -36)
                   .arg(entry.shortcut)
                   .arg(entry.isCustom ? " (Custom)" : "");
        }
        return 0;
    }

    int searchIdx = args.indexOf("--search");
    if (searchIdx == -1) searchIdx = args.indexOf("-s");
    if (searchIdx != -1 && searchIdx + 1 < args.size()) {
        QString query = args.at(searchIdx + 1);
        ShortcutManager mgr;
        auto results = mgr.filter(query);
        QTextStream out(stdout);
        out << "Search results for '" << query << "' (" << results.size() << " matches):\n\n";
        for (const auto &entry : results) {
            out << QString("[%1] %2 => %3%4\n")
                   .arg(entry.componentFriendly, -24)
                   .arg(entry.actionFriendly, -36)
                   .arg(entry.shortcut)
                   .arg(entry.isCustom ? " (Custom)" : "");
        }
        return 0;
    }

    QString serverName = QString("kde-shortcut-popup-%1").arg(qgetenv("USER"));

    // Check if another instance is already running
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(200)) {
        // Already running, send toggle message and exit
        socket.write("toggle\n");
        socket.waitForBytesWritten(200);
        return 0;
    }

    // Otherwise, start local server for future toggle commands
    QLocalServer::removeServer(serverName);
    QLocalServer server;
    if (!server.listen(serverName)) {
        // If listen fails, still proceed
    }

    QString initialQuery;
    int qIdx = args.indexOf("--query");
    if (qIdx == -1) qIdx = args.indexOf("-q");
    if (qIdx != -1 && qIdx + 1 < args.size()) {
        initialQuery = args.at(qIdx + 1);
    }

    PopupWindow window(initialQuery);

    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(200);
            QByteArray data = client->readAll();
            if (data.contains("toggle") || data.contains("close")) {
                window.close();
                app.quit();
            }
        }
    });

    window.show();
    window.raise();
    window.activateWindow();

    return app.exec();
}
