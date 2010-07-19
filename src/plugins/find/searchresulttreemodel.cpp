/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "searchresulttreemodel.h"
#include "searchresulttreeitems.h"
#include "searchresulttreeitemroles.h"

#include <QtGui/QApplication>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtGui/QTextDocument>
#include <QtGui/QTextCursor>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Find;
using namespace Find::Internal;

SearchResultTreeModel::SearchResultTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_currentParent(0)
    , m_showReplaceUI(false)
{
    m_rootItem = new SearchResultTreeItem;
    m_textEditorFont = QFont(QLatin1String("Courier"));
}

SearchResultTreeModel::~SearchResultTreeModel()
{
    delete m_rootItem;
}

void SearchResultTreeModel::setShowReplaceUI(bool show)
{
    m_showReplaceUI = show;
}

void SearchResultTreeModel::setTextEditorFont(const QFont &font)
{
    layoutAboutToBeChanged();
    m_textEditorFont = font;
    layoutChanged();
}

Qt::ItemFlags SearchResultTreeModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(idx);

    if (idx.isValid()) {
        if (const SearchResultTreeItem *item = treeItemAtIndex(idx)) {
            if (item->isLeaf() && item->isUserCheckable()) {
                flags |= Qt::ItemIsUserCheckable;
            }
        }
    }

    return flags;
}

QModelIndex SearchResultTreeModel::index(int row, int column,
                                         const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = treeItemAtIndex(parent);

    const SearchResultTreeItem *childItem = parentItem->childAt(row);
    if (childItem)
        return createIndex(row, column, (void *)childItem);
    else
        return QModelIndex();
}

QModelIndex SearchResultTreeModel::index(SearchResultTreeItem *item) const
{
    return createIndex(item->rowOfItem(), 0, (void *)item);
}

QModelIndex SearchResultTreeModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    const SearchResultTreeItem *childItem = treeItemAtIndex(idx);
    const SearchResultTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->rowOfItem(), 0, (void *)parentItem);
}

int SearchResultTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const SearchResultTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = treeItemAtIndex(parent);

    return parentItem->childrenCount();
}

int SearchResultTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

SearchResultTreeItem *SearchResultTreeModel::treeItemAtIndex(const QModelIndex &idx) const
{
    return static_cast<SearchResultTreeItem*>(idx.internalPointer());
}

QVariant SearchResultTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    QVariant result;

    if (role == Qt::SizeHintRole) {
        // TODO we should not use editor font height if that is not used by any item
        const int appFontHeight = QApplication::fontMetrics().height();
        const int editorFontHeight = QFontMetrics(m_textEditorFont).height();
        result = QSize(0, qMax(appFontHeight, editorFontHeight));
    } else {
        result = data(treeItemAtIndex(idx), role);
    }

    return result;
}

bool SearchResultTreeModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());
        treeItemAtIndex(idx)->setCheckState(checkState);
        return true;
    }
    return QAbstractItemModel::setData(idx, value, role);
}

QVariant SearchResultTreeModel::data(const SearchResultTreeItem *row, int role) const
{
    QVariant result;

    switch (role)
    {
    case Qt::CheckStateRole:
        if (row->isUserCheckable())
            result = row->checkState();
        break;
    case Qt::ToolTipRole:
        result = row->item.text.trimmed();
        break;
    case Qt::FontRole:
        if (row->item.useTextEditorFont)
            result = m_textEditorFont;
        else
            result = QVariant();
        break;
    case ItemDataRoles::ResultLineRole:
    case Qt::DisplayRole:
        result = row->item.text;
        break;
    case ItemDataRoles::ResultItemRole:
        result = qVariantFromValue(row->item);
        break;
    case ItemDataRoles::ResultLineNumberRole:
        result = row->item.lineNumber;
        break;
    case ItemDataRoles::ResultIconRole:
        result = row->item.icon;
        break;
    case ItemDataRoles::SearchTermStartRole:
        result = row->item.textMarkPos;
        break;
    case ItemDataRoles::SearchTermLengthRole:
        result = row->item.textMarkLength;
        break;
// TODO this looks stupid in case of symbol tree, is it necessary?
//    case Qt::BackgroundRole:
//        if (row->parent() && row->parent()->parent())
//            result = QVariant();
//        else
//            result = QApplication::palette().base().color().darker(105);
//        break;
    default:
        result = QVariant();
        break;
    }

    return result;
}

QVariant SearchResultTreeModel::headerData(int section, Qt::Orientation orientation,
                                           int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

/**
 * Makes sure that the nodes for a specific path exist and sets
 * m_currentParent to the last final
 */
QSet<SearchResultTreeItem *> SearchResultTreeModel::addPath(const QStringList &path)
{
    QSet<SearchResultTreeItem *> pathNodes;
    SearchResultTreeItem *currentItem = m_rootItem;
    QModelIndex currentItemIndex = QModelIndex();
    SearchResultTreeItem *partItem = 0;
    QStringList currentPath;
    foreach (const QString &part, path) {
        const int insertionIndex = currentItem->insertionIndex(part, &partItem);
        if (!partItem) {
            SearchResultItem item;
            item.path = currentPath;
            item.text = part;
            partItem = new SearchResultTreeItem(item, currentItem);
            if (m_showReplaceUI) {
                partItem->setIsUserCheckable(true);
                partItem->setCheckState(Qt::Checked);
            }
            partItem->setGenerated(true);
            beginInsertRows(currentItemIndex, insertionIndex, insertionIndex);
            currentItem->insertChild(insertionIndex, partItem);
            endInsertRows();
        }
        pathNodes << partItem;
        currentItemIndex = index(insertionIndex, 0, currentItemIndex);
        currentItem = partItem;
        currentPath << part;
    }

    m_currentParent = currentItem;
    m_currentPath = currentPath;
    m_currentIndex = currentItemIndex;
    return pathNodes;
}

void SearchResultTreeModel::addResultsToCurrentParent(const QList<SearchResultItem> &items, SearchResultWindow::AddMode mode)
{
    if (!m_currentParent)
        return;

    if (mode == SearchResultWindow::AddOrdered) {
        // this is the mode for e.g. text search
        beginInsertRows(m_currentIndex, m_currentParent->childrenCount(), m_currentParent->childrenCount() + items.count());
        foreach (const SearchResultItem &item, items) {
            m_currentParent->appendChild(item);
        }
        endInsertRows();
    } else if (mode == SearchResultWindow::AddSorted) {
        foreach (const SearchResultItem &item, items) {
            SearchResultTreeItem *existingItem;
            const int insertionIndex = m_currentParent->insertionIndex(item, &existingItem);
            if (existingItem) {
                existingItem->setGenerated(false);
                existingItem->item = item;
                QModelIndex itemIndex = m_currentIndex.child(insertionIndex, 0);
                dataChanged(itemIndex, itemIndex);
            } else {
                beginInsertRows(m_currentIndex, insertionIndex, insertionIndex);
                m_currentParent->insertChild(insertionIndex, item);
                endInsertRows();
            }
        }
    }
    dataChanged(m_currentIndex, m_currentIndex); // Make sure that the number after the file name gets updated
}

static bool lessThanByPath(const SearchResultItem &a, const SearchResultItem &b)
{
    if (a.path.size() < b.path.size())
        return true;
    if (a.path.size() > b.path.size())
        return false;
    for (int i = 0; i < a.path.size(); ++i) {
        if (a.path.at(i) < b.path.at(i))
            return true;
        if (a.path.at(i) > b.path.at(i))
            return false;
    }
    return false;
}

/**
 * Adds the search result to the list of results, creating nodes for the path when
 * necessary.
 */
QList<QModelIndex> SearchResultTreeModel::addResults(const QList<SearchResultItem> &items, SearchResultWindow::AddMode mode)
{
    QSet<SearchResultTreeItem *> pathNodes;
    QList<SearchResultItem> sortedItems = items;
    qStableSort(sortedItems.begin(), sortedItems.end(), lessThanByPath);
    QList<SearchResultItem> itemSet;
    foreach (const SearchResultItem &item, sortedItems) {
        if (!m_currentParent || (m_currentPath != item.path)) {
            // first add all the items from before
            if (!itemSet.isEmpty()) {
                addResultsToCurrentParent(itemSet, mode);
                itemSet.clear();
            }
            // switch parent
            pathNodes += addPath(item.path);
        }
        itemSet << item;
    }
    if (!itemSet.isEmpty()) {
        addResultsToCurrentParent(itemSet, mode);
        itemSet.clear();
    }
    QList<QModelIndex> pathIndices;
    foreach (SearchResultTreeItem *item, pathNodes)
        pathIndices << index(item);
    return pathIndices;
}

void SearchResultTreeModel::clear()
{
    m_currentParent = NULL;
    m_rootItem->clearChildren();
    reset();
}

QModelIndex SearchResultTreeModel::nextIndex(const QModelIndex &idx) const
{
    // pathological
    if (!idx.isValid())
        return index(0, 0);

    if (rowCount(idx) > 0) {
        // node with children
        return idx.child(0, 0);
    }
    // leaf node
    QModelIndex nextIndex;
    QModelIndex current = idx;
    while (!nextIndex.isValid()) {
        int row = current.row();
        current = current.parent();
        if (row + 1 < rowCount(current)) {
            // Same parent has another child
            nextIndex = index(row + 1, 0, current);
        } else {
            // go up one parent
            if (!current.isValid()) {
                nextIndex = index(0, 0);
            }
        }
    }
    return nextIndex;
}

QModelIndex SearchResultTreeModel::next(const QModelIndex &idx, bool includeGenerated) const
{
    QModelIndex value = idx;
    do {
        value = nextIndex(value);
    } while (value != idx && !includeGenerated && treeItemAtIndex(value)->isGenerated());
    return value;
}

QModelIndex SearchResultTreeModel::prevIndex(const QModelIndex &idx) const
{
    QModelIndex current = idx;
    bool checkForChildren = true;
    if (current.isValid()) {
        int row = current.row();
        if (row > 0) {
            current = index(row - 1, 0, current.parent());
        } else {
            current = current.parent();
            checkForChildren = !current.isValid();
        }
    }
    if (checkForChildren) {
        // traverse down the hierarchy
        while (int rc = rowCount(current)) {
            current = index(rc - 1, 0, current);
        }
    }
    return current;
}

QModelIndex SearchResultTreeModel::prev(const QModelIndex &idx, bool includeGenerated) const
{
    QModelIndex value = idx;
    do {
        value = prevIndex(value);
    } while (value != idx && !includeGenerated && treeItemAtIndex(value)->isGenerated());
    return value;
}

QModelIndex SearchResultTreeModel::find(const QRegExp &expr, const QModelIndex &index, QTextDocument::FindFlags flags)
{
    QModelIndex resultIndex;
    QModelIndex currentIndex = index;
    bool backward = (flags & QTextDocument::FindBackward);

    do {
        if (backward)
            currentIndex = prev(currentIndex, true);
        else
            currentIndex = next(currentIndex, true);
        if (currentIndex.isValid()) {
            const QString &text = data(currentIndex, ItemDataRoles::ResultLineRole).toString();
            if (expr.indexIn(text) != -1)
                resultIndex = currentIndex;
        }
    } while (!resultIndex.isValid() && currentIndex.isValid() && currentIndex != index);
    return resultIndex;
}

QModelIndex SearchResultTreeModel::find(const QString &term, const QModelIndex &index, QTextDocument::FindFlags flags)
{
    QModelIndex resultIndex;
    QModelIndex currentIndex = index;
    bool backward = (flags & QTextDocument::FindBackward);
    flags = (flags & (~QTextDocument::FindBackward)); // backward is handled by us ourselves

    do {
        if (backward)
            currentIndex = prev(currentIndex, true);
        else
            currentIndex = next(currentIndex, true);
        if (currentIndex.isValid()) {
            const QString &text = data(currentIndex, ItemDataRoles::ResultLineRole).toString();
            QTextDocument doc(text);
            if (!doc.find(term, 0, flags).isNull())
                resultIndex = currentIndex;
        }
    } while (!resultIndex.isValid() && currentIndex.isValid() && currentIndex != index);
    return resultIndex;
}
