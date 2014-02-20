/*
 * Cantata
 *
 * Copyright (c) 2011 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef STREAMSMODEL_H
#define STREAMSMODEL_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtCore/QHash>

class QTimer;

class StreamsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct Item
    {
        Item(const QString &n) : name(n) { }
        virtual bool isCategory() = 0;
        virtual ~Item() { }
        QString name;
    };

    struct CategoryItem;
    struct StreamItem : public Item
    {
        StreamItem(const QString &n, const QUrl &u, CategoryItem *p=0) : Item(n), url(u), parent(p) { }
        bool isCategory() { return false; }
        QUrl url;
        CategoryItem *parent;
    };

    struct CategoryItem : public Item
    {
        CategoryItem(const QString &n) : Item(n) { }
        virtual ~CategoryItem() { clearStreams(); }
        bool isCategory() { return true; }
        void clearStreams();
        QHash<QString, StreamItem *> itemMap;
        QList<StreamItem *> streams;
    };

    StreamsModel();
    ~StreamsModel();
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex&) const { return 1; }
    bool hasChildren(const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QVariant data(const QModelIndex &, int) const;
    void reload();
    void save(bool force=false);
    bool save(const QString &filename);
    bool import(const QString &filename) { return load(filename, false); }
    bool add(const QString &cat, const QString &name, const QString &url);
    void editCategory(const QModelIndex &index, const QString &name);
    void editStream(const QModelIndex &index, const QString &oldCat, const QString &newCat, const QString &name, const QString &url);
    void remove(const QModelIndex &index);
    QString name(const QString &cat, const QString &url) { return name(getCategory(cat), url); }
    bool entryExists(const QString &cat, const QString &name, const QUrl &url=QUrl()) { return entryExists(getCategory(cat), name, url); }
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData * mimeData(const QModelIndexList &indexes) const;
    void mark(const QList<int> &rows, bool f);

private:
    void clearCategories();
    bool load(const QString &filename, bool isInternal);
    CategoryItem * getCategory(const QString &name, bool create=false, bool signal=false);
    QString name(CategoryItem *cat, const QString &url);
    bool entryExists(CategoryItem *cat, const QString &name, const QUrl &url=QUrl());

private Q_SLOTS:
    void persist();

private:
    QList<CategoryItem *> items;
    bool modified;
    QTimer *timer;
};

#endif