/*
 * Cantata
 *
 * Copyright (c) 2011-2012 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "musiclibraryitemroot.h"
#include "musiclibraryitemartist.h"
#include "musiclibraryitemalbum.h"
#include "musiclibraryitemsong.h"
#include "song.h"
#include "covers.h"
#include "config.h"
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QApplication>
#include <QtGui/QFontMetrics>

static MusicLibraryItemAlbum::CoverSize coverSize=MusicLibraryItemAlbum::CoverNone;
static QPixmap *theDefaultIcon=0;
static bool useDate=false;
static QSize iconItemSize;

static inline int adjust(int v, int step)
{
    return (((int)(v/step))*step)+(v%step ? step : 0);
}

static int autoListSize=32;
static int autoIconSize=128;

static int setSize(int size, bool iconMode)
{
    for (int i=MusicLibraryItemAlbum::CoverSmall; i<=MusicLibraryItemAlbum::CoverAuto; ++i) {
        int icnSize=MusicLibraryItemAlbum::iconSize((MusicLibraryItemAlbum::CoverSize)i, iconMode);
        if (size<=icnSize) {
            size=icnSize;
            break;
        }
    }

    if (autoIconSize>MusicLibraryItemAlbum::iconSize(MusicLibraryItemAlbum::CoverExtraLarge, iconMode)) {
        size=adjust(size, 4);
    }
    return size;
}

void MusicLibraryItemAlbum::setup()
{
    int height=QApplication::fontMetrics().height();

    autoListSize=setSize(height*2, false);
    autoIconSize=setSize(height*6, true);
}

int MusicLibraryItemAlbum::iconSize(MusicLibraryItemAlbum::CoverSize sz, bool iconMode)
{
    switch (sz) {
    default:
    case CoverNone:       return 0;
    case CoverSmall:      return iconMode ? 76  : 22;
    case CoverMedium:     return iconMode ? 100 : 32;
    case CoverLarge:      return iconMode ? 128 : 48;
    case CoverExtraLarge: return iconMode ? 160 : 64;
    case CoverAuto:       return iconMode ? autoIconSize : autoListSize;
    }
}

int MusicLibraryItemAlbum::iconSize(bool iconMode)
{
    return MusicLibraryItemAlbum::iconSize(coverSize, iconMode);
}

void MusicLibraryItemAlbum::setItemSize(const QSize &sz)
{
    iconItemSize=sz;
}

QSize MusicLibraryItemAlbum::itemSize()
{
    return iconItemSize;
}

MusicLibraryItemAlbum::CoverSize MusicLibraryItemAlbum::currentCoverSize()
{
    return coverSize;
}

void MusicLibraryItemAlbum::setCoverSize(MusicLibraryItemAlbum::CoverSize size)
{
    if (size!=coverSize) {
        if (theDefaultIcon) {
            delete theDefaultIcon;
            theDefaultIcon=0;
        }
        MusicLibraryItemArtist::clearDefaultCover();
        coverSize=size;
    }
}

void MusicLibraryItemAlbum::setShowDate(bool sd)
{
    useDate=sd;
}

bool MusicLibraryItemAlbum::showDate()
{
    return useDate;
}

MusicLibraryItemAlbum::MusicLibraryItemAlbum(const QString &data, quint32 year, MusicLibraryItemContainer *parent)
    : MusicLibraryItemContainer(data, parent)
    , m_year(year)
    , m_coverIsDefault(false)
    , m_cover(0)
    , m_type(Song::Standard)
{
}

MusicLibraryItemAlbum::~MusicLibraryItemAlbum()
{
    delete m_cover;
}

bool MusicLibraryItemAlbum::setCover(const QImage &img) const
{
    if (m_coverIsDefault && !img.isNull()) {
        int size=iconSize(!iconItemSize.isNull());
        m_cover = new QPixmap(QPixmap::fromImage(img).scaled(QSize(size, size), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_coverIsDefault=false;
        return true;
    }

    return false;
}

const QPixmap & MusicLibraryItemAlbum::cover()
{
    if (m_coverIsDefault && theDefaultIcon) {
        return *theDefaultIcon;
    }

    if (!m_cover) {
        int iSize=iconSize(!iconItemSize.isNull());

        if (!theDefaultIcon) {
            int cSize=iSize;
            if (0==cSize) {
                cSize=22;
            }
            theDefaultIcon = new QPixmap(QIcon::fromTheme(DEFAULT_ALBUM_ICON).pixmap(cSize, cSize)
                                        .scaled(QSize(cSize, cSize), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        m_coverIsDefault = true;
        if (Song::SingleTracks!=m_type && parentItem() && iSize && childCount()) {
            MusicLibraryItemSong *firstSong=static_cast<MusicLibraryItemSong*>(childItem(0));
            Song song;
            if (Song::MultipleArtists==m_type) { // Then Cantata has placed this album under 'Various Artists' but the actual album as a different AlbumArtist tag
                song.artist=firstSong->song().albumArtist();
            } else {
                song.albumartist=parentItem()->data();
            }
            song.album=m_itemData;
            song.year=m_year;
            song.file=firstSong->file();
            Covers::self()->requestCover(song, true);
        }
        return *theDefaultIcon;
    }

    return *m_cover;
}

QStringList MusicLibraryItemAlbum::sortedTracks() const
{
    QMap<int, QString> tracks;
    quint32 trackWithoutNumberIndex=0xFFFF; // *Very* unlikely to have tracks numbered greater than 65535!!!

    for (int i = 0; i < childCount(); i++) {
        MusicLibraryItemSong *trackItem = static_cast<MusicLibraryItemSong*>(childItem(i));
        tracks.insert(0==trackItem->track() || trackItem->track()>0xFFFF ? trackWithoutNumberIndex++ : trackItem->track(), trackItem->file());
    }

    return tracks.values();
}

quint32 MusicLibraryItemAlbum::totalTime()
{
    if (0==m_totalTime) {
        foreach (MusicLibraryItem *i, m_childItems) {
            m_totalTime+=static_cast<MusicLibraryItemSong *>(i)->time();
        }
    }
    return m_totalTime;
}

void MusicLibraryItemAlbum::addTracks(MusicLibraryItemAlbum *other)
{
    m_type=Song::SingleTracks;
    foreach (MusicLibraryItem *track, other->m_childItems) {
        static_cast<MusicLibraryItemSong*>(track)->song().type=Song::SingleTracks;
        m_singleTrackFiles.insert(static_cast<MusicLibraryItemSong*>(track)->song().file);
        track->setParent(this);
    }
}

void MusicLibraryItemAlbum::setIsSingleTracks()
{
    foreach (MusicLibraryItem *track, m_childItems) {
        static_cast<MusicLibraryItemSong*>(track)->song().type=Song::SingleTracks;
        m_singleTrackFiles.insert(static_cast<MusicLibraryItemSong*>(track)->song().file);
    }
    m_type=Song::SingleTracks;
}

void MusicLibraryItemAlbum::setIsMultipleArtists()
{
    foreach (MusicLibraryItem *track, m_childItems) {
        static_cast<MusicLibraryItemSong*>(track)->song().type=Song::MultipleArtists;
    }
    m_type=Song::MultipleArtists;
}

void MusicLibraryItemAlbum::append(MusicLibraryItem *i)
{
    MusicLibraryItemContainer::append(i);
    if (Song::SingleTracks==m_type) {
        static_cast<MusicLibraryItemSong*>(i)->song().type=Song::SingleTracks;
        m_singleTrackFiles.insert(static_cast<MusicLibraryItemSong*>(i)->song().file);
    } else if (Song::MultipleArtists==m_type) {
        static_cast<MusicLibraryItemSong*>(i)->song().type=Song::MultipleArtists;
    }
    m_totalTime=0;
}

void MusicLibraryItemAlbum::remove(int row)
{
    delete m_childItems.takeAt(row);
    m_totalTime=0;
}

bool MusicLibraryItemAlbum::detectIfIsMultipleArtists()
{
    if (m_childItems.count()<2) {
        return false;
    }

    if (Song::Standard==m_type) {
        QString a;
        foreach (MusicLibraryItem *track, m_childItems) {
            if (a.isEmpty()) {
                a=static_cast<MusicLibraryItemSong*>(track)->song().artist;
            } else if (static_cast<MusicLibraryItemSong*>(track)->song().artist!=a) {
                m_type=Song::MultipleArtists;
                break;
            }
        }
    }
    return Song::MultipleArtists==m_type;
}

const MusicLibraryItemSong * MusicLibraryItemAlbum::getCueFile() const
{
    if (Song::Standard==m_type && 2==m_childItems.count()) {
        const MusicLibraryItemSong *a=static_cast<const MusicLibraryItemSong *>(m_childItems.at(0));
        const MusicLibraryItemSong *b=static_cast<const MusicLibraryItemSong *>(m_childItems.at(1));

        if ( ( (Song::Playlist==a->song().type && Song::Playlist!=b->song().type) ||
               (Song::Playlist!=a->song().type && Song::Playlist==b->song().type) ) &&
             ( (Song::Playlist==a->song().type && a->song().file.endsWith(".cue", Qt::CaseInsensitive)) ||
               (Song::Playlist==b->song().type && b->song().file.endsWith(".cue", Qt::CaseInsensitive)) ) ) {
            return Song::Playlist==a->song().type ? a : b;
        }
    }

    return 0;
}