#include "popup_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QClipboard>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QScrollBar>
#include <QGraphicsDropShadowEffect>
#include <QWindow>
#include <LayerShellQt/Window>

PopupWindow::PopupWindow(const QString &initialQuery, QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    centerOnScreen();

    // Populate with all shortcuts
    if (!initialQuery.isEmpty()) {
        m_searchEdit->setText(initialQuery);
        updateModel(m_manager.filter(initialQuery));
    } else {
        updateModel(m_manager.allShortcuts());
    }
}

void PopupWindow::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(700, 540);

    winId();
    if (windowHandle()) {
        auto lsWin = LayerShellQt::Window::get(windowHandle());
        if (lsWin) {
            lsWin->setLayer(LayerShellQt::Window::LayerOverlay);
            lsWin->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
            lsWin->setAnchors(LayerShellQt::Window::AnchorNone);
            lsWin->setWantsToBeOnActiveScreen(true);
            lsWin->setCloseOnDismissed(true);
            lsWin->setScope("kde-shortcut-popup");
        }
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    // Search Bar Area
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search shortcuts, apps, or keys (e.g. 'kwin', 'ctrl alt', 'meta w')...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(44);
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid palette(mid);"
        "  border-radius: 8px;"
        "  padding: 0px 14px;"
        "  font-size: 14px;"
        "  background: palette(base);"
        "  color: palette(text);"
        "}"
        "QLineEdit:focus {"
        "  border: 1.5px solid palette(highlight);"
        "}"
    );
    m_searchEdit->installEventFilter(this);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PopupWindow::onSearchTextChanged);

    m_countLabel = new QLabel(this);
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_countLabel->setStyleSheet("color: palette(placeholderText); font-size: 12px; padding-right: 6px;");

    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(m_countLabel, 0);
    mainLayout->addLayout(searchLayout);

    // Shortcuts List
    m_listView = new QListView(this);
    m_model = new QStandardItemModel(this);
    m_delegate = new ShortcutDelegate(this);

    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setStyleSheet(
        "QListView {"
        "  background: transparent;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 6px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: palette(mid);"
        "  border-radius: 3px;"
        "  min-height: 24px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: palette(highlight);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );

    connect(m_listView, &QListView::clicked, this, &PopupWindow::onItemActivated);
    mainLayout->addWidget(m_listView, 1);

    // Footer Hint
    m_footerHint = new QLabel("Esc Close  •  ↑↓ Navigate  •  Enter / Click Copy  •  Global Shortcut to Toggle", this);
    m_footerHint->setAlignment(Qt::AlignCenter);
    m_footerHint->setStyleSheet("color: palette(placeholderText); font-size: 11px; padding-top: 4px;");
    mainLayout->addWidget(m_footerHint, 0);

    // Toast Notification Label (overlay)
    m_toastLabel = new QLabel(this);
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setStyleSheet(
        "QLabel {"
        "  background-color: rgba(30, 30, 30, 230);"
        "  color: #ffffff;"
        "  border: 1px solid rgba(255, 255, 255, 0.2);"
        "  border-radius: 18px;"
        "  padding: 8px 18px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
    );
    m_toastLabel->hide();

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toastLabel, &QLabel::hide);

    // Auto-focus search field
    m_searchEdit->setFocus();
}

void PopupWindow::centerOnScreen()
{
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen) {
        QRect sg = screen->geometry();
        int x = sg.x() + (sg.width() - width()) / 2;
        int y = sg.y() + (sg.height() - height()) * 0.42; // slightly elevated
        move(x, y);
    }
}

void PopupWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect rect = this->rect();

    // Fill background with card styling
    QPainterPath path;
    path.addRoundedRect(rect.adjusted(1, 1, -1, -1), 14, 14);

    QColor bg = palette().window().color();
    bg.setAlpha(245); // subtle acrylic translucency
    painter.fillPath(path, bg);

    // Border
    QColor border = palette().mid().color();
    border.setAlpha(160);
    painter.setPen(QPen(border, 1.2));
    painter.drawPath(path);
}

void PopupWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        // Dismiss popup when user clicks outside
        static bool initialFocusSeen = false;
        if (isActiveWindow()) {
            initialFocusSeen = true;
        } else if (initialFocusSeen) {
            close();
        }
    }
    QWidget::changeEvent(event);
}

void PopupWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool PopupWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_searchEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            close();
            return true;
        } else if (ke->key() == Qt::Key_Down) {
            int row = m_listView->currentIndex().row();
            if (row < m_model->rowCount() - 1) {
                QModelIndex next = m_model->index(row + 1, 0);
                m_listView->setCurrentIndex(next);
                m_listView->scrollTo(next);
            }
            return true;
        } else if (ke->key() == Qt::Key_Up) {
            int row = m_listView->currentIndex().row();
            if (row > 0) {
                QModelIndex prev = m_model->index(row - 1, 0);
                m_listView->setCurrentIndex(prev);
                m_listView->scrollTo(prev);
            }
            return true;
        } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            QModelIndex cur = m_listView->currentIndex();
            if (cur.isValid()) {
                onItemActivated(cur);
            }
            return true;
        } else if (ke->key() == Qt::Key_PageDown) {
            int row = qMin(m_listView->currentIndex().row() + 6, m_model->rowCount() - 1);
            if (row >= 0) {
                QModelIndex idx = m_model->index(row, 0);
                m_listView->setCurrentIndex(idx);
                m_listView->scrollTo(idx);
            }
            return true;
        } else if (ke->key() == Qt::Key_PageUp) {
            int row = qMax(m_listView->currentIndex().row() - 6, 0);
            if (row < m_model->rowCount()) {
                QModelIndex idx = m_model->index(row, 0);
                m_listView->setCurrentIndex(idx);
                m_listView->scrollTo(idx);
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PopupWindow::onSearchTextChanged(const QString &text)
{
    QList<ShortcutEntry> matches = m_manager.filter(text);
    updateModel(matches);
}

void PopupWindow::updateModel(const QList<ShortcutEntry> &entries)
{
    m_model->clear();
    for (const auto &entry : entries) {
        QStandardItem *item = new QStandardItem();
        item->setData(QVariant::fromValue(entry), Qt::UserRole);
        m_model->appendRow(item);
    }

    if (m_model->rowCount() > 0) {
        m_listView->setCurrentIndex(m_model->index(0, 0));
    }

    // Update count label
    int total = m_manager.totalCount();
    if (entries.size() == total) {
        m_countLabel->setText(QString("%1 shortcuts").arg(total));
    } else {
        m_countLabel->setText(QString("%1 of %2").arg(entries.size()).arg(total));
    }
}

void PopupWindow::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid()) return;
    ShortcutEntry entry = index.data(Qt::UserRole).value<ShortcutEntry>();

    QString toCopy = !entry.shortcut.isEmpty() ? entry.shortcut : entry.actionFriendly;
    QGuiApplication::clipboard()->setText(toCopy);

    showToast(QString("Copied: %1").arg(toCopy));
}

void PopupWindow::showToast(const QString &message)
{
    m_toastLabel->setText(message);
    m_toastLabel->adjustSize();

    int x = (width() - m_toastLabel->width()) / 2;
    int y = height() - m_toastLabel->height() - 40;
    m_toastLabel->move(x, y);
    m_toastLabel->show();
    m_toastLabel->raise();

    m_toastTimer->start(1600);
}
