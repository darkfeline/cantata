/*
 * Cantata
 *
 * Copyright (c) 2011 Craig Drummond <craig.p.drummond@gmail.com>
 *
 */
/*
 * Copyright (c) 2008 Sander Knopper (sander AT knopper DOT tk) and
 *                    Roeland Douma (roeland AT rullzer DOT com)
 *
 * This file is part of QtMPC.
 *
 * QtMPC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * QtMPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QtMPC.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_LIBRARY_ITEM_ROOT_H
#define MUSIC_LIBRARY_ITEM_ROOT_H

#include <QList>
#include <QVariant>
#include <QMap>
#include "musiclibraryitem.h"

class Song;
class MusicLibraryItemArtist;

class MusicLibraryItemRoot : public MusicLibraryItem
{
public:
    MusicLibraryItemRoot(const QString &data=QString());
    ~MusicLibraryItemRoot();

    MusicLibraryItemArtist * artist(const Song &s);
    MusicLibraryItem * child(int row) const;
    int childCount() const;

private:
    QMap<QString, int> m_indexes;
    QList<MusicLibraryItemArtist *> m_childItems;

    friend class MusicLibraryItemArtist;
};

#endif
