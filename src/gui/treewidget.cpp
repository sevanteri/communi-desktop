/*
* Copyright (C) 2008-2013 The Communi Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "treewidget.h"
#include "treeitem.h"
#include "treemenu.h"
#include "navigator.h"
#include "itemdelegate.h"
#include <QContextMenuEvent>
#include <IrcConnection>
#include <QHeaderView>
#include <IrcBuffer>
#include <QTimer>

TreeWidget::TreeWidget(QWidget* parent) : QTreeWidget(parent)
{
    setAnimated(true);
    setColumnCount(2);
    setIndentation(0);
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setFrameStyle(QFrame::NoFrame);

    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    header()->setStretchLastSection(false);
    header()->setResizeMode(0, QHeaderView::Stretch);
    header()->setResizeMode(1, QHeaderView::Fixed);
    header()->resizeSection(1, fontMetrics().width("999"));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ItemDelegate* delegate = new ItemDelegate(this);
    delegate->setRootIsDecorated(true);
    setItemDelegate(delegate);

    d.itemResetBlocked = false;
    d.navigator = new Navigator(this);

    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    d.resetShortcut = new QShortcut(this);
    d.resetShortcut->setKey(QKeySequence(tr("Ctrl+R")));
}

QSize TreeWidget::sizeHint() const
{
    return QSize(20 * fontMetrics().width('#'), QTreeWidget::sizeHint().height());
}

QByteArray TreeWidget::saveState() const
{
    QByteArray state;
//    QDataStream out(&state, QIODevice::WriteOnly);

//    QVariantHash hash;
//    for (int i = 0; i < topLevelItemCount(); ++i) {
//        TreeItem* item = static_cast<TreeItem*>(topLevelItem(i));
//        if (item->currentSortOrder() == TreeItem::Manual)
//            item->resetManualSortOrder();
//        hash.insert(item->text(0), item->manualSortOrder());
//    }

//    if (QTreeWidgetItem* item = currentItem()) {
//        hash.insert("_currentText_", currentItem()->text(0));
//        QTreeWidgetItem* parent = item->parent();
//        if (!parent)
//            parent = invisibleRootItem();
//        hash.insert("_currentIndex_", parent->indexOfChild(item));
//        hash.insert("_currentParent_", parent->text(0));
//    }
//    out << hash;
    return state;
}

void TreeWidget::restoreState(const QByteArray& state)
{
//    QVariantHash hash;
//    QDataStream in(state);
//    in >> hash;

//    for (int i = 0; i < topLevelItemCount(); ++i) {
//        TreeItem* item = static_cast<TreeItem*>(topLevelItem(i));
//        QStringList order = hash.value(item->text(0)).toStringList();
//        if (order != item->manualSortOrder()) {
//            item->setManualSortOrder(order);
//            item->sortChildren(0, Qt::AscendingOrder);
//        }
//    }

//    if (!d.currentRestored && hash.contains("_currentText_")) {
//        QList<QTreeWidgetItem*> candidates = findItems(hash.value("_currentText_").toString(), Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
//        foreach (QTreeWidgetItem* candidate, candidates) {
//            QTreeWidgetItem* parent = candidate->parent();
//            if (!parent)
//                parent = invisibleRootItem();
//            if (parent->indexOfChild(candidate) == hash.value("_currentIndex_").toInt()
//                    && parent->text(0) == hash.value("_currentParent_").toString()) {
//                setCurrentItem(candidate);
//                d.currentRestored = true;
//                break;
//            }
//        }
//    }
}

IrcBuffer* TreeWidget::currentBuffer() const
{
    TreeItem* item = static_cast<TreeItem*>(currentItem());
    if (item)
        return item->buffer();
    return 0;
}

TreeItem* TreeWidget::bufferItem(IrcBuffer* buffer) const
{
    return d.bufferItems.value(buffer);
}

QList<IrcConnection*> TreeWidget::connections() const
{
    return d.connections;
}

void TreeWidget::addBuffer(IrcBuffer* buffer)
{
    TreeItem* item = 0;
    if (buffer->isSticky()) {
        item = new TreeItem(buffer, this);
        IrcConnection* connection = buffer->connection();
        d.connectionItems.insert(connection, item);
        d.connections.append(connection);
    } else {
        TreeItem* parent = d.connectionItems.value(buffer->connection());
        item = new TreeItem(buffer, parent);
    }
    connect(d.resetShortcut, SIGNAL(activated()), item, SLOT(reset()));
    d.bufferItems.insert(buffer, item);
}

void TreeWidget::removeBuffer(IrcBuffer* buffer)
{
    if (buffer->isSticky()) {
        IrcConnection* connection = buffer->connection();
        d.connectionItems.remove(connection);
        d.connections.removeOne(connection);
    }
    delete d.bufferItems.take(buffer);
}

void TreeWidget::setCurrentBuffer(IrcBuffer* buffer)
{
    TreeItem* item = d.bufferItems.value(buffer);
    if (item)
        setCurrentItem(item);
}

void TreeWidget::search(const QString& search)
{
    if (!search.isEmpty()) {
        QList<QTreeWidgetItem*> items = findItems(search, Qt::MatchExactly | Qt::MatchWrap | Qt::MatchRecursive);
        if (items.isEmpty())
            items = findItems(search, Qt::MatchContains | Qt::MatchWrap | Qt::MatchRecursive);
        if (!items.isEmpty() && !items.contains(currentItem()))
            setCurrentItem(items.first());
        emit searched(!items.isEmpty());
    }
}

void TreeWidget::searchAgain(const QString& search)
{
    QTreeWidgetItem* item = currentItem();
    if (item && !search.isEmpty()) {
        QTreeWidgetItemIterator it(item, QTreeWidgetItemIterator::Unselected);
        bool wrapped = false;
        while (*it) {
            if ((*it)->text(0).contains(search, Qt::CaseInsensitive)) {
                setCurrentItem(*it);
                return;
            }
            ++it;
            if (!(*it) && !wrapped) {
                it = QTreeWidgetItemIterator(this, QTreeWidgetItemIterator::Unselected);
                wrapped = true;
            }
        }
    }
}

void TreeWidget::blockItemReset()
{
    d.itemResetBlocked = true;
}

void TreeWidget::unblockItemReset()
{
    d.itemResetBlocked = false;
    delayedReset(currentItem());
}

bool TreeWidget::event(QEvent* event)
{
    if (event->type() == QEvent::WindowActivate)
        delayedReset(currentItem());
    return QTreeWidget::event(event);
}

void TreeWidget::contextMenuEvent(QContextMenuEvent* event)
{
    TreeItem* item = static_cast<TreeItem*>(itemAt(event->pos()));
    if (item) {
        TreeMenu menu(this);
        menu.exec(item, event->globalPos());
    }
}

void TreeWidget::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    QTreeWidget::rowsAboutToBeRemoved(parent, start, end);
    TreeItem* item = static_cast<TreeItem*>(itemFromIndex(parent));
    if (item) {
        for (int i = start; i <= end; ++i) {
            TreeItem* child = static_cast<TreeItem*>(item->child(i));
            if (child) {
                item->d.highlightedChildren.remove(child);
                child->reset();
            }
        }
    }
}

void TreeWidget::onItemExpanded(QTreeWidgetItem* item)
{
    static_cast<TreeItem*>(item)->emitDataChanged();
}

void TreeWidget::onItemCollapsed(QTreeWidgetItem* item)
{
    static_cast<TreeItem*>(item)->emitDataChanged();
}

void TreeWidget::onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (!d.itemResetBlocked) {
        if (previous)
            static_cast<TreeItem*>(previous)->reset();
    }
    if (current) {
        emit currentBufferChanged(static_cast<TreeItem*>(current)->buffer());
        delayedReset(current);
    }
}

void TreeWidget::delayedReset(QTreeWidgetItem* item)
{
    if (item)
        QTimer::singleShot(500, static_cast<TreeItem*>(item), SLOT(reset()));
}
