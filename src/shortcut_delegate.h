#ifndef SHORTCUT_DELEGATE_H
#define SHORTCUT_DELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include "shortcut_entry.h"

class ShortcutDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ShortcutDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void drawKeyBadge(QPainter *painter, const QString &keyText, int x, int y, int height, bool isSelected) const;
};

#endif // SHORTCUT_DELEGATE_H
