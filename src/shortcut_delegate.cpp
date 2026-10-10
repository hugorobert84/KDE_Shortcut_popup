#include "shortcut_delegate.h"
#include <QIcon>
#include <QApplication>

Q_DECLARE_METATYPE(ShortcutEntry)

ShortcutDelegate::ShortcutDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize ShortcutDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(400, 52);
}

void ShortcutDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    ShortcutEntry entry = index.data(Qt::UserRole).value<ShortcutEntry>();
    QRect rect = option.rect;

    const bool isSelected = (option.state & QStyle::State_Selected);
    const bool isHovered = (option.state & QStyle::State_MouseOver);

    // Draw row background
    if (isSelected) {
        QColor selColor = option.palette.highlight().color();
        selColor.setAlpha(60);
        painter->setBrush(selColor);
        painter->setPen(QPen(option.palette.highlight().color(), 1.0));
        QRect r = rect.adjusted(4, 2, -4, -2);
        painter->drawRoundedRect(r, 8, 8);
    } else if (isHovered) {
        QColor hoverColor = option.palette.highlight().color();
        hoverColor.setAlpha(25);
        painter->setBrush(hoverColor);
        painter->setPen(Qt::NoPen);
        QRect r = rect.adjusted(4, 2, -4, -2);
        painter->drawRoundedRect(r, 8, 8);
    }

    // 1. Draw Icon
    QIcon icon = QIcon::fromTheme(entry.iconName, QIcon::fromTheme("preferences-desktop-keyboard-shortcuts"));
    int iconSize = 24;
    int iconX = rect.left() + 14;
    int iconY = rect.top() + (rect.height() - iconSize) / 2;
    icon.paint(painter, iconX, iconY, iconSize, iconSize);

    // 2. Measure Key Badges (Right side)
    QStringList primaryKeys = entry.primaryKeys();
    QStringList secondaryKeys = entry.secondaryKeys();

    QFont badgeFont = painter->font();
    badgeFont.setPointSize(badgeFont.pointSize() > 0 ? qMax(badgeFont.pointSize() - 1, 9) : 9);
    badgeFont.setBold(true);
    QFontMetrics fmBadge(badgeFont);

    QFont plusFont = painter->font();
    plusFont.setPointSize(qMax(plusFont.pointSize() - 2, 8));
    QFontMetrics fmPlus(plusFont);

    int badgeHeight = 24;
    int badgeY = rect.top() + (rect.height() - badgeHeight) / 2;
    int rightEdge = rect.right() - 14;

    // Helper lambda to calculate width of key sequence
    auto calcSequenceWidth = [&](const QStringList &keys) -> int {
        if (keys.isEmpty()) return 0;
        int w = 0;
        for (int i = 0; i < keys.size(); ++i) {
            int keyW = fmBadge.horizontalAdvance(keys[i]) + 16;
            w += keyW;
            if (i < keys.size() - 1) {
                w += fmPlus.horizontalAdvance("+") + 6;
            }
        }
        return w;
    };

    int primaryWidth = calcSequenceWidth(primaryKeys);
    int secondaryWidth = calcSequenceWidth(secondaryKeys);

    int totalKeysWidth = primaryWidth;
    if (secondaryWidth > 0) {
        totalKeysWidth += 24 + secondaryWidth; // "or" separator + secondary keys
    }

    // Custom badge width if any
    int customTagWidth = 0;
    if (entry.isCustom) {
        customTagWidth = 60; // "[Custom]" pill
    }

    int keysStartX = rightEdge - totalKeysWidth;
    int curX = keysStartX;

    // Draw primary key sequence
    painter->setFont(badgeFont);
    for (int i = 0; i < primaryKeys.size(); ++i) {
        const QString &k = primaryKeys[i];
        int keyW = fmBadge.horizontalAdvance(k) + 16;

        QRect badgeRect(curX, badgeY, keyW, badgeHeight);
        QColor badgeBg = isSelected ? option.palette.highlight().color() : option.palette.button().color();
        if (isSelected) {
            badgeBg.setAlpha(120);
        } else {
            badgeBg = badgeBg.lighter(108);
        }

        painter->setBrush(badgeBg);
        QColor badgeBorder = option.palette.mid().color();
        painter->setPen(QPen(badgeBorder, 1.0));
        painter->drawRoundedRect(badgeRect, 5, 5);

        painter->setPen(option.palette.buttonText().color());
        painter->drawText(badgeRect, Qt::AlignCenter, k);

        curX += keyW;

        if (i < primaryKeys.size() - 1) {
            QRect plusRect(curX, badgeY, fmPlus.horizontalAdvance("+") + 6, badgeHeight);
            painter->setFont(plusFont);
            QColor plusColor = option.palette.placeholderText().color();
            painter->setPen(plusColor);
            painter->drawText(plusRect, Qt::AlignCenter, "+");
            painter->setFont(badgeFont);
            curX += plusRect.width();
        }
    }

    // Draw secondary keys if any
    if (secondaryWidth > 0) {
        QRect orRect(curX, badgeY, 24, badgeHeight);
        painter->setFont(plusFont);
        painter->setPen(option.palette.placeholderText().color());
        painter->drawText(orRect, Qt::AlignCenter, "or");
        curX += 24;

        painter->setFont(badgeFont);
        for (int i = 0; i < secondaryKeys.size(); ++i) {
            const QString &k = secondaryKeys[i];
            int keyW = fmBadge.horizontalAdvance(k) + 14;

            QRect badgeRect(curX, badgeY, keyW, badgeHeight);
            QColor badgeBg = option.palette.button().color();
            badgeBg.setAlpha(180);
            painter->setBrush(badgeBg);
            painter->setPen(QPen(option.palette.mid().color(), 1.0));
            painter->drawRoundedRect(badgeRect, 5, 5);

            painter->setPen(option.palette.buttonText().color());
            painter->drawText(badgeRect, Qt::AlignCenter, k);

            curX += keyW;
            if (i < secondaryKeys.size() - 1) {
                QRect plusRect(curX, badgeY, fmPlus.horizontalAdvance("+") + 6, badgeHeight);
                painter->setFont(plusFont);
                painter->setPen(option.palette.placeholderText().color());
                painter->drawText(plusRect, Qt::AlignCenter, "+");
                painter->setFont(badgeFont);
                curX += plusRect.width();
            }
        }
    }

    // Draw [Custom] tag if modified
    if (entry.isCustom) {
        int tagX = keysStartX - customTagWidth - 8;
        QRect tagRect(tagX, badgeY + 2, customTagWidth, badgeHeight - 4);
        QColor tagBg = option.palette.accent().color();
        if (!tagBg.isValid()) tagBg = QColor(255, 170, 0);
        tagBg.setAlpha(45);
        painter->setBrush(tagBg);
        QColor tagBorder = option.palette.accent().color();
        if (!tagBorder.isValid()) tagBorder = QColor(255, 170, 0);
        tagBorder.setAlpha(150);
        painter->setPen(QPen(tagBorder, 1.0));
        painter->drawRoundedRect(tagRect, 4, 4);

        painter->setFont(plusFont);
        painter->setPen(tagBorder);
        painter->drawText(tagRect, Qt::AlignCenter, "Custom");
    }

    // 3. Draw Text (Action Name and Component Name)
    int textLeft = iconX + iconSize + 12;
    int textRight = (entry.isCustom ? keysStartX - customTagWidth - 16 : keysStartX - 12);
    int textWidth = qMax(textRight - textLeft, 50);

    // Title (Action)
    QFont titleFont = painter->font();
    titleFont.setPointSize(titleFont.pointSize() > 0 ? titleFont.pointSize() + 1 : 11);
    titleFont.setBold(true);
    QFontMetrics fmTitle(titleFont);

    QString actionTitle = fmTitle.elidedText(entry.actionFriendly, Qt::ElideRight, textWidth);
    QRect titleRect(textLeft, rect.top() + 7, textWidth, fmTitle.height());

    painter->setFont(titleFont);
    painter->setPen(option.palette.text().color());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, actionTitle);

    // Subtitle (Component / Category)
    QFont subFont = painter->font();
    subFont.setPointSize(qMax(subFont.pointSize() - 1, 9));
    subFont.setBold(false);
    QFontMetrics fmSub(subFont);

    QString compSub = fmSub.elidedText(entry.componentFriendly, Qt::ElideRight, textWidth);
    QRect subRect(textLeft, rect.top() + 7 + fmTitle.height() + 2, textWidth, fmSub.height());

    painter->setFont(subFont);
    QColor subColor = option.palette.placeholderText().color();
    painter->setPen(subColor);
    painter->drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter, compSub);

    painter->restore();
}
