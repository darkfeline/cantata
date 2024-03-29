/*
 * Cantata
 *
 * Copyright (c) 2011-2014 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "serversettings.h"
#include "settings.h"
#include "support/localize.h"
#include "support/inputdialog.h"
#include "support/messagebox.h"
#include "widgets/icons.h"
#include "models/musiclibrarymodel.h"
#include "mpd-interface/mpduser.h"
#include "support/utils.h"
#include <QDir>
#include <QComboBox>
#include <QPushButton>
#include <QValidator>
#include <QStyle>
#if QT_VERSION > 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

#define REMOVE(w) \
    w->setVisible(false); \
    w->deleteLater(); \
    w=0;

class CoverNameValidator : public QValidator
{
    public:

    CoverNameValidator(QObject *parent) : QValidator(parent) { }

    State validate(QString &input, int &) const
    {
        for (int i=0; i<input.length(); ++i) {
            if (!input[i].isLetterOrNumber() && '%'!=input[i] && ' '!=input[i] && '-'!=input[i]) {
                return Invalid;
            }
        }

        return Acceptable;
    }
};

class CollectionNameValidator : public QValidator
{
    public:

    CollectionNameValidator(QObject *parent) : QValidator(parent) { }

    State validate(QString &input, int &) const
    {
        return input.startsWith(MPDUser::constName) ? Invalid : Acceptable;
    }
};

ServerSettings::ServerSettings(QWidget *p)
    : QWidget(p)
    , haveBasicCollection(false)
    , prevIndex(0)
{
    setupUi(this);
    #if defined ENABLE_DEVICES_SUPPORT
    musicFolderNoteLabel->appendText(i18n("<i> This folder will also be used to locate music files "
                                          "for transferring to (and from) devices.</i>"));
    #endif
    connect(combo, SIGNAL(activated(int)), SLOT(showDetails(int)));
    connect(removeButton, SIGNAL(clicked(bool)), SLOT(remove()));
    connect(addButton, SIGNAL(clicked(bool)), SLOT(add()));
    connect(name, SIGNAL(textChanged(QString)), SLOT(nameChanged()));
    connect(basicDir, SIGNAL(textChanged(QString)), SLOT(basicDirChanged()));
    connect(topLevel, SIGNAL(textChanged(QString)), SLOT(topLevelChanged()));
    addButton->setIcon(Icon("list-add"));
    removeButton->setIcon(Icon("list-remove"));
    addButton->setAutoRaise(true);
    removeButton->setAutoRaise(true);

    #if defined Q_OS_WIN
    hostLabel->setText(i18n("Host:"));
    socketNoteLabel->setVisible(false);
    #endif
    basicCoverName->setToolTip(coverName->toolTip());
    basicCoverNameLabel->setToolTip(coverName->toolTip());
    coverNameLabel->setToolTip(coverName->toolTip());
    coverName->setValidator(new CoverNameValidator(this));
    basicCoverName->setValidator(new CoverNameValidator(this));
    name->setValidator(new CollectionNameValidator(this));
    #ifndef ENABLE_HTTP_STREAM_PLAYBACK
    REMOVE(streamUrlLabel)
    REMOVE(streamUrl)
    REMOVE(streamUrlNoteLabel)
    #endif

    #ifdef Q_OS_MAC
    expandingSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    #endif
}

void ServerSettings::load()
{
    QList<MPDConnectionDetails> all=Settings::self()->allConnections();
    QString currentCon=Settings::self()->currentConnection();

    qSort(all);
    combo->clear();
    int idx=0;
    haveBasicCollection=false;
    foreach (MPDConnectionDetails d, all) {
        combo->addItem(d.getName(), d.name);
        if (d.name==currentCon) {
            prevIndex=idx;
        }
        idx++;
        if (d.name==MPDUser::constName) {
            d.dir=MPDUser::self()->details().dir;
            haveBasicCollection=true;
            prevBasic=d;
        }
        DeviceOptions opts;
        opts.load(MPDConnectionDetails::configGroupName(d.name), true);
        collections.append(Collection(d, opts));
    }
    combo->setCurrentIndex(prevIndex);
    setDetails(collections.at(prevIndex).details);
    removeButton->setEnabled(combo->count()>1);
}

void ServerSettings::save()
{
    if (combo->count()<1) {
        return;
    }

    Collection current=collections.at(combo->currentIndex());
    current.details=getDetails();
    collections.replace(combo->currentIndex(), current);

    QList<MPDConnectionDetails> existingInConfig=Settings::self()->allConnections();
    QList<Collection> toAdd;

    foreach (const Collection &c, collections) {
        bool found=false;
        for (int i=0; i<existingInConfig.count(); ++i) {
            MPDConnectionDetails e=existingInConfig.at(i);
            if (e.name==c.details.name) {
                existingInConfig.removeAt(i);
                found=true;
                if (c.details.hostname!=e.hostname || c.details.port!=e.port || c.details.password!=e.password ||
                    c.details.dir!=e.dir || c.details.coverName!=e.coverName || c.details.topLevel!=e.topLevel
                    #ifdef ENABLE_HTTP_STREAM_PLAYBACK
                    || c.details.streamUrl!=e.streamUrl
                    #endif
                    ) {
                    toAdd.append(c);
                }
            }
        }
        if (!found) {
            toAdd.append(c);
        }
        if (c.details.name==MPDUser::constName) {
            prevBasic=c;
        }
    }

    foreach (const MPDConnectionDetails &c, existingInConfig) {
        Settings::self()->removeConnectionDetails(c.name);
    }

    foreach (const Collection &c, toAdd) {
        Settings::self()->saveConnectionDetails(c.details);
        c.namingOpts.save(MPDConnectionDetails::configGroupName(c.details.name), true);
    }

    if (!haveBasicCollection && MPDUser::self()->isSupported()) {
        MPDUser::self()->cleanup();
    }

    if (current.details.name==MPDUser::constName) {
        MPDUser::self()->setDetails(current.details);
    }
    Settings::self()->saveCurrentConnection(current.details.name);
    MusicLibraryModel::cleanCache();
}

void ServerSettings::cancel()
{
    // If we are canceling any changes, then we need to restore user settings...
    if (prevBasic.details.name==MPDUser::constName) {
        MPDUser::self()->setDetails(prevBasic.details);
    }
}

void ServerSettings::showDetails(int index)
{
    if (-1!=prevIndex && index!=prevIndex) {
        MPDConnectionDetails details=getDetails();
        if (details.name.isEmpty()) {
            details.name=generateName(prevIndex);
        }
        collections.replace(prevIndex, details);
        if (details.name!=MPDUser::constName) {
            combo->setItemText(prevIndex, details.name);
        }
    }
    setDetails(collections.at(index).details);
    prevIndex=index;
}

void ServerSettings::add()
{
    bool addStandard=true;

    if (!haveBasicCollection && MPDUser::self()->isSupported()) {
        static const QChar constBullet(0x2022);

        switch (MessageBox::questionYesNoCancel(this,
                                   i18n("Which type of collection do you wish to connect to?")+QLatin1String("\n\n")+
                                   constBullet+QLatin1Char(' ')+i18n("Standard - music collection may be shared, is on another machine, or is already setup")+QLatin1Char('\n')+
                                   constBullet+QLatin1Char(' ')+i18n("Basic - music collection is not shared with others, and Cantata will configure and control the MPD instance"),
                                   i18n("Add Collection"), GuiItem(i18n("Standard")), GuiItem(i18n("Basic")))) {
        case MessageBox::Yes: addStandard=true; break;
        case MessageBox::No: addStandard=false; break;
        default: return;
        }
    }

    MPDConnectionDetails details;
    if (addStandard) {
        details.name=generateName();
        details.port=6600;
        details.hostname=QLatin1String("localhost");
        details.dir=QLatin1String("/var/lib/mpd/music/");
        combo->addItem(details.name);
    } else {
        details=MPDUser::self()->details(true);
        #if QT_VERSION > 0x050000
        QString dir=QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        #else
        QString dir=QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
        #endif
        if (dir.isEmpty()) {
            QString dir=QDir::homePath()+"/Music";
            dir=dir.replace("//", "/");
        }
        dir=Utils::fixPath(dir);
        basicDir->setText(dir);
        MPDUser::self()->setMusicFolder(dir);
        combo->addItem(MPDUser::translatedName());
    }
    removeButton->setEnabled(combo->count()>1);
    collections.append(Collection(details));
    combo->setCurrentIndex(combo->count()-1);
    prevIndex=combo->currentIndex();
    setDetails(details);
}

void ServerSettings::remove()
{
    int index=combo->currentIndex();
    QString cName=1==stackedWidget->currentIndex() ? MPDUser::translatedName() : name->text();
    if (combo->count()>1 && MessageBox::Yes==MessageBox::questionYesNo(this, i18n("Delete '%1'?", cName),
                                                                       i18n("Delete"), StdGuiItem::del(), StdGuiItem::cancel())) {
        bool isLast=index==(combo->count()-1);
        combo->removeItem(index);
        combo->setCurrentIndex(isLast ? index-1 : index);
        prevIndex=combo->currentIndex();
        collections.removeAt(index);
        if (1==stackedWidget->currentIndex()) {
            haveBasicCollection=false;
        }
        setDetails(collections.at(combo->currentIndex()).details);
    }
    removeButton->setEnabled(combo->count()>1);
}

void ServerSettings::nameChanged()
{
    combo->setItemText(combo->currentIndex(), name->text().trimmed());
}

void ServerSettings::basicDirChanged()
{
    if (!prevBasic.details.dir.isEmpty()) {
        QString d=Utils::fixPath(basicDir->text().trimmed());
        basicMusicFolderNoteLabel->setOn(d.isEmpty() || d!=prevBasic.details.dir);
    }
}

void ServerSettings::topLevelChanged()
{
    topLevelNoteLabel->setOn(0==stackedWidget->currentIndex() && !topLevel->text().trimmed().isEmpty());
}

QString ServerSettings::generateName(int ignore) const
{
    QString n;
    QSet<QString> collectionNames;
    for (int i=0; i<collections.size(); ++i) {
        if (i!=ignore) {
            collectionNames.insert(collections.at(i).details.name);
        }
    }

    for (int i=1; i<512; ++i) {
        n=i18n("New Collection %1", i);
        if (!collectionNames.contains(n)) {
            break;
        }
    }

    return n;
}

void ServerSettings::setDetails(const MPDConnectionDetails &details)
{
    if (details.name==MPDUser::constName) {
        basicDir->setText(Utils::convertPathForDisplay(details.dir));
        basicCoverName->setText(details.coverName);
        stackedWidget->setCurrentIndex(1);
    } else {
        name->setText(details.name.isEmpty() ? i18n("Default") : details.name);
        host->setText(details.hostname);
        port->setValue(details.port);
        password->setText(details.password);
        dir->setText(Utils::convertPathForDisplay(details.dir));
        coverName->setText(details.coverName);
        #ifdef ENABLE_HTTP_STREAM_PLAYBACK
        streamUrl->setText(details.streamUrl);
        #endif
        topLevel->setText(details.topLevel);
        stackedWidget->setCurrentIndex(0);
    }
}

MPDConnectionDetails ServerSettings::getDetails() const
{
    MPDConnectionDetails details;
    if (0==stackedWidget->currentIndex()) {
        details.name=name->text().trimmed();
        if (details.name==MPDUser::constName) {
            details.name=QString();
        }
        details.hostname=host->text().trimmed();
        details.port=port->value();
        details.password=password->text();
        details.dir=Utils::convertPathFromDisplay(dir->text());
        details.coverName=coverName->text().trimmed();
        #ifdef ENABLE_HTTP_STREAM_PLAYBACK
        details.streamUrl=streamUrl->text().trimmed();
        #endif
        details.topLevel=topLevel->text().trimmed();
    } else {
        details=MPDUser::self()->details(true);
        details.dir=Utils::convertPathFromDisplay(basicDir->text());
        details.coverName=basicCoverName->text().trimmed();
        MPDUser::self()->setMusicFolder(details.dir);
    }
    details.setDirReadable();
    return details;
}
