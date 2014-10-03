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

#include "httpserversettings.h"
#include "gui/settings.h"
#include "support/localize.h"
#include "httpserver.h"
#include <QComboBox>
#include <QNetworkInterface>

static int isIfaceType(const QNetworkInterface &iface, const QString &prefix)
{
    return iface.name().length()>prefix.length() && iface.name().startsWith(prefix) && iface.name()[prefix.length()].isDigit();
}

static QString details(const QNetworkInterface &iface)
{
    QString d=iface.humanReadableName();
    if (!iface.addressEntries().isEmpty()) {
        d+=QLatin1String(" - ")+iface.addressEntries().first().ip().toString();
    }
    return d;
}

static QString displayName(const QNetworkInterface &iface)
{
    if (iface.name()=="lo") {
        return i18n("Local loopback (%1)", details(iface));
    }
    if (isIfaceType(iface, "eth")) {
        return i18n("Wired (%1)", details(iface));
    }
    if (isIfaceType(iface, "wlan")) {
        return i18n("Wireless (%1)", details(iface));
    }
    return details(iface);
}

static void initInterfaces(QComboBox *combo)
{
    combo->addItem(i18n("First active interface"), QString());
    QList<QNetworkInterface> ifaces=QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &iface, ifaces) {
        if (iface.flags()&QNetworkInterface::IsUp && !isIfaceType(iface, "vboxnet") && !isIfaceType(iface, "vmnet")) {
            combo->addItem(displayName(iface), iface.name());
        }
    }
}

HttpServerSettings::HttpServerSettings(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    initInterfaces(httpInterface);
}

void HttpServerSettings::load()
{
    QString iface=Settings::self()->httpInterface();
    for (int i=0; i<httpInterface->count(); ++i) {
        if (httpInterface->itemData(i)==iface) {
            httpInterface->setCurrentIndex(i);
            break;
        }
    }
}

bool HttpServerSettings::haveMultipleInterfaces() const
{
    return httpInterface->count()>3;
}

void HttpServerSettings::save()
{
    Settings::self()->saveHttpInterface(httpInterface->itemData(httpInterface->currentIndex()).toString());
    HttpServer::self()->readConfig();
}