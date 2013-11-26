/*
 * Cantata
 *
 * Copyright (c) 2011-2013 Craig Drummond <craig.p.drummond@gmail.com>
 *
 */
/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ultimatelyricsprovider.h"
#include "networkaccessmanager.h"
#include "song.h"
#include <QTextCodec>

static QString extract(const QString &source, const QString &begin, const QString &end)
{
    int beginIdx = source.indexOf(begin);
    if (-1==beginIdx) {
        return QString();
    }
    beginIdx += begin.length();

    int endIdx = source.indexOf(end, beginIdx);
    if (-1==endIdx) {
        return QString();
    }

    return source.mid(beginIdx, endIdx - beginIdx - 1);
}

static QString extractXmlTag(const QString &source, const QString &tag)
{
    QRegExp re("<(\\w+).*>"); // ಠ_ಠ
    if (-1==re.indexIn(tag)) {
        return QString();
    }

    return extract(source, tag, "</" + re.cap(1) + ">");
}

static QString exclude(const QString &source, const QString &begin, const QString &end)
{
    int beginIdx = source.indexOf(begin);
    if (-1==beginIdx) {
        return source;
    }

    int endIdx = source.indexOf(end, beginIdx + begin.length());
    if (-1==endIdx) {
        return source;
    }

    return source.left(beginIdx) + source.right(source.length() - endIdx - end.length());
}

static QString excludeXmlTag(const QString &source, const QString &tag)
{
    QRegExp re("<(\\w+).*>"); // ಠ_ಠ
    if (-1==re.indexIn(tag)) {
        return source;
    }

    return exclude(source, tag, "</" + re.cap(1) + ">");
}

static void applyExtractRule(const UltimateLyricsProvider::Rule &rule, QString *content)
{
    foreach (const UltimateLyricsProvider::RuleItem& item, rule) {
        if (item.second.isNull()) {
            *content = extractXmlTag(*content, item.first);
        } else {
            *content = extract(*content, item.first, item.second);
        }
    }
}

static void applyExcludeRule(const UltimateLyricsProvider::Rule &rule, QString *content)
{
    foreach (const UltimateLyricsProvider::RuleItem& item, rule) {
        if (item.second.isNull()) {
            *content = excludeXmlTag(*content, item.first);
        } else {
            *content = exclude(*content, item.first, item.second);
        }
    }
}

static QString noSpace(const QString &text)
{
    QString ret(text);
    ret.remove(' ');
    return ret;
}

static QString firstChar(const QString &text)
{
    return text.isEmpty() ? text : text[0].toLower();
}

static QString titleCase(const QString &text)
{
    if (0==text.length()) {
        return QString();
    }
    if (1==text.length()) {
        return text[0].toUpper();
    }
    return text[0].toUpper() + text.right(text.length() - 1).toLower();
}

UltimateLyricsProvider::UltimateLyricsProvider()
    : enabled(true)
    , relevance(0)
{
}

UltimateLyricsProvider::~UltimateLyricsProvider()
{
    abort();
}

void UltimateLyricsProvider::fetchInfo(int id, const Song &metadata)
{
    #if QT_VERSION < 0x050000
    const QTextCodec *codec = QTextCodec::codecForName(charset.toAscii().constData());
    #else
    const QTextCodec *codec = QTextCodec::codecForName(charset.toLatin1().constData());
    #endif
    if (!codec) {
        emit lyricsReady(id, QString());
        return;
    }

    QString artistFixed=metadata.basicArtist();
    // Fill in fields in the URL
    QString urlText(url);
    doUrlReplace("{artist}",  artistFixed.toLower(),             urlText);
    doUrlReplace("{artist2}", noSpace(artistFixed.toLower()),    urlText);
    doUrlReplace("{album}",   metadata.album.toLower(),          urlText);
    doUrlReplace("{album2}",  noSpace(metadata.album.toLower()), urlText);
    doUrlReplace("{title}",   metadata.title.toLower(),          urlText);
    doUrlReplace("{Artist}",  artistFixed,                       urlText);
    doUrlReplace("{Album}",   metadata.album,                    urlText);
    doUrlReplace("{ARTIST}",  artistFixed.toUpper(),             urlText);
    doUrlReplace("{year}",    QString::number(metadata.year),    urlText);
    doUrlReplace("{Title}",   metadata.title,                    urlText);
    doUrlReplace("{Title2}",  titleCase(metadata.title),         urlText);
    doUrlReplace("{a}",       firstChar(artistFixed),            urlText);
    doUrlReplace("{track}",   QString::number(metadata.track),   urlText);

    // For some reason Qt messes up the ? -> %3F and & -> %26 conversions - by placing 25 after the %
    // So, try and revert this...
    QUrl url(urlText);
    QByteArray data=url.toEncoded();
    data.replace("%253F", "%3F");
    data.replace("%253f", "%3f");
    data.replace("%2526", "%26");

    NetworkJob *reply = NetworkAccessManager::self()->get(QUrl::fromEncoded(data, QUrl::StrictMode));
    requests[reply] = id;
    connect(reply, SIGNAL(finished()), this, SLOT(lyricsFetched()));
}

void UltimateLyricsProvider::abort()
{
    QHash<NetworkJob *, int>::ConstIterator it(requests.constBegin());
    QHash<NetworkJob *, int>::ConstIterator end(requests.constEnd());

    for (; it!=end; ++it) {
        disconnect(it.key(), SIGNAL(finished()), this, SLOT(lyricsFetched()));
        it.key()->deleteLater();
        it.key()->close();
    }
    requests.clear();
}

void UltimateLyricsProvider::lyricsFetched()
{
    NetworkJob *reply = qobject_cast<NetworkJob*>(sender());
    if (!reply) {
        return;
    }

    int id = requests.take(reply);
    reply->deleteLater();

    if (!reply->ok()) {
        //emit Finished(id);
        emit lyricsReady(id, QString());
        return;
    }

    #if QT_VERSION < 0x050000
    const QTextCodec *codec = QTextCodec::codecForName(charset.toAscii().constData());
    #else
    const QTextCodec *codec = QTextCodec::codecForName(charset.toLatin1().constData());
    #endif
    const QString originalContent = codec->toUnicode(reply->readAll());

    // Check for invalid indicators
    foreach (const QString &indicator, invalidIndicators) {
        if (originalContent.contains(indicator)) {
            //emit Finished(id);
            emit lyricsReady(id, QString());
            return;
        }
    }

    QString lyrics;

    // Apply extract rules
    foreach (const Rule& rule, extractRules) {
        QString content = originalContent;
        applyExtractRule(rule, &content);

        if (!content.isEmpty()) {
            lyrics = content;
        }
    }

    // Apply exclude rules
    foreach (const Rule& rule, excludeRules) {
        applyExcludeRule(rule, &lyrics);
    }

    lyrics=lyrics.trimmed();
    emit lyricsReady(id, lyrics);
}

void UltimateLyricsProvider::doUrlReplace(const QString &tag, const QString &value, QString &u) const
{
    if (!u.contains(tag)) {
        return;
    }

    // Apply URL character replacement
    QString valueCopy(value);
    foreach (const UltimateLyricsProvider::UrlFormat& format, urlFormats) {
        QRegExp re("[" + QRegExp::escape(format.first) + "]");
        valueCopy.replace(re, format.second);
    }
    // ? and & should ony be used in URL queries...
    valueCopy.replace(QLatin1Char('&'), QLatin1String("%26"));
    valueCopy.replace(QLatin1Char('?'), QLatin1String("%3f"));
    u.replace(tag, valueCopy, Qt::CaseInsensitive);
}
