#ifndef POPUP_WINDOW_H
#define POPUP_WINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QListView>
#include <QLabel>
#include <QStandardItemModel>
#include <QTimer>
#include "shortcut_manager.h"
#include "shortcut_delegate.h"

class PopupWindow : public QWidget {
    Q_OBJECT

public:
    explicit PopupWindow(const QString &initialQuery = QString(), QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onSearchTextChanged(const QString &text);
    void onItemActivated(const QModelIndex &index);

private:
    void setupUI();
    void updateModel(const QList<ShortcutEntry> &entries);
    void centerOnScreen();
    void showToast(const QString &message);

    ShortcutManager m_manager;
    QLineEdit *m_searchEdit = nullptr;
    QLabel *m_countLabel = nullptr;
    QListView *m_listView = nullptr;
    QStandardItemModel *m_model = nullptr;
    ShortcutDelegate *m_delegate = nullptr;
    QLabel *m_footerHint = nullptr;

    // Toast overlay
    QLabel *m_toastLabel = nullptr;
    QTimer *m_toastTimer = nullptr;
};

#endif // POPUP_WINDOW_H
