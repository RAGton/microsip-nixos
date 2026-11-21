/*
 * MicroSIP - Native Linux / NixOS Port (Qt 6 + PJSIP)
 * Based on MicroSIP (https://www.microsip.org)
 * Copyright (C) 2011-2026 MicroSIP (https://www.microsip.org)
 * Native Linux/NixOS Qt6 Port Copyright (C) 2026 Gabriel Aguiar Rocha (RAGton) <aguiarrocha37@outlook.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace msip {

Settings::Settings() {
    initPaths();
}

void Settings::initPaths() {
    configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.config/microsip";
    }
    dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.local/share/microsip";
    }

    QDir().mkpath(configDir);
    QDir().mkpath(dataDir);

    iniFile = configDir + "/microsip.ini";
    dbFile = dataDir + "/microsip.db";
    logFile = dataDir + "/microsip.log";

    if (recordingPath.isEmpty()) {
        recordingPath = dataDir + "/recordings";
    }
    QDir().mkpath(recordingPath);
}

void Settings::load() {
    QSettings ini(iniFile, QSettings::IniFormat);

    ini.beginGroup("Settings");
    accountId = ini.value("accountId", 0).toInt();
    enableLocalAccount = ini.value("enableLocalAccount", false).toBool();
    singleMode = ini.value("singleMode", false).toBool();
    alwaysOnTop = ini.value("alwaysOnTop", false).toBool();
    startMinimized = ini.value("minimized", false).toBool();
    enableLog = ini.value("enableLog", false).toBool();
    bringToFrontOnIncoming = ini.value("bringToFrontOnIncoming", true).toBool();

    ringtone = ini.value("ringtone", "").toString();
    audioRingDevice = ini.value("audioRingDevice", "").toString();
    audioOutputDevice = ini.value("audioOutputDevice", "").toString();
    audioInputDevice = ini.value("audioInputDevice", "").toString();
    audioCodecs = ini.value("audioCodecs", audioCodecs).toString();
    vad = ini.value("VAD", false).toBool();
    ec = ini.value("EC", true).toBool();
    volumeRing = ini.value("volumeRing", 100).toInt();
    volumeOutput = ini.value("volumeOutput", 100).toInt();
    volumeInput = ini.value("volumeInput", 100).toInt();

    QString recPath = ini.value("recordingPath", "").toString();
    if (!recPath.isEmpty()) {
        recordingPath = recPath;
        QDir().mkpath(recordingPath);
    }
    recordingFormat = ini.value("recordingFormat", "wav").toString();
    autoRecording = ini.value("autoRecording", false).toBool();

    rport = ini.value("rport", true).toBool();
    sourcePort = ini.value("sourcePort", 0).toInt();
    rtpPortMin = ini.value("rtpPortMin", 4000).toInt();
    rtpPortMax = ini.value("rtpPortMax", 0).toInt();
    stun = ini.value("STUN", "").toString();
    enableStun = ini.value("enableSTUN", false).toBool();
    dtmfMethod = static_cast<DtmfMethod>(ini.value("DTMFMethod", 0).toInt());
    localDtmf = ini.value("localDTMF", true).toBool();
    userAgent = ini.value("userAgent", "MicroSIP/3.23.5-linux").toString();

    dnd = ini.value("DND", false).toBool();
    aa = ini.value("AA", false).toBool();
    autoAnswer = ini.value("autoAnswer", "button").toString();
    autoAnswerDelay = ini.value("autoAnswerDelay", 0).toInt();
    autoAnswerNumber = ini.value("autoAnswerNumber", "").toString();

    fwd = ini.value("FWD", false).toBool();
    forwardingNumber = ini.value("forwardingNumber", "").toString();
    forwardingDelay = ini.value("forwardingDelay", 0).toInt();

    autoHangUpTime = ini.value("autoHangUpTime", 0).toInt();
    maxConcurrentCalls = ini.value("maxConcurrentCalls", 8).toInt();
    callWaiting = ini.value("callWaiting", true).toBool();

    featureCodeCP = ini.value("featureCodeCP", "**").toString();
    featureCodeBT = ini.value("featureCodeBT", "##").toString();
    featureCodeAT = ini.value("featureCodeAT", "*2").toString();
    enableFeatureCodeCP = ini.value("enableFeatureCodeCP", false).toBool();
    enableFeatureCodeBT = ini.value("enableFeatureCodeBT", false).toBool();
    enableFeatureCodeAT = ini.value("enableFeatureCodeAT", false).toBool();

    enableShortcuts = ini.value("enableShortcuts", true).toBool();
    shortcutsBottom = ini.value("shortcutsBottom", false).toBool();
    darkTheme = ini.value("darkTheme", true).toBool();
    activeTab = ini.value("activeTab", 0).toInt();
    windowGeometry = QByteArray::fromBase64(ini.value("windowGeometry").toByteArray());
    lastCallNumber = ini.value("lastCallNumber", "").toString();
    ini.endGroup();
}

void Settings::save() {
    QSettings ini(iniFile, QSettings::IniFormat);

    ini.beginGroup("Global");
    ini.setValue("version", "3.23.5-linux");
    ini.endGroup();

    ini.beginGroup("Settings");
    ini.setValue("accountId", accountId);
    ini.setValue("enableLocalAccount", enableLocalAccount);
    ini.setValue("singleMode", singleMode);
    ini.setValue("alwaysOnTop", alwaysOnTop);
    ini.setValue("minimized", startMinimized);
    ini.setValue("enableLog", enableLog);
    ini.setValue("bringToFrontOnIncoming", bringToFrontOnIncoming);

    ini.setValue("ringtone", ringtone);
    ini.setValue("audioRingDevice", audioRingDevice);
    ini.setValue("audioOutputDevice", audioOutputDevice);
    ini.setValue("audioInputDevice", audioInputDevice);
    ini.setValue("audioCodecs", audioCodecs);
    ini.setValue("VAD", vad);
    ini.setValue("EC", ec);
    ini.setValue("volumeRing", volumeRing);
    ini.setValue("volumeOutput", volumeOutput);
    ini.setValue("volumeInput", volumeInput);

    ini.setValue("recordingPath", recordingPath);
    ini.setValue("recordingFormat", recordingFormat);
    ini.setValue("autoRecording", autoRecording);

    ini.setValue("rport", rport);
    ini.setValue("sourcePort", sourcePort);
    ini.setValue("rtpPortMin", rtpPortMin);
    ini.setValue("rtpPortMax", rtpPortMax);
    ini.setValue("STUN", stun);
    ini.setValue("enableSTUN", enableStun);
    ini.setValue("DTMFMethod", static_cast<int>(dtmfMethod));
    ini.setValue("localDTMF", localDtmf);
    ini.setValue("userAgent", userAgent);

    ini.setValue("DND", dnd);
    ini.setValue("AA", aa);
    ini.setValue("autoAnswer", autoAnswer);
    ini.setValue("autoAnswerDelay", autoAnswerDelay);
    ini.setValue("autoAnswerNumber", autoAnswerNumber);

    ini.setValue("FWD", fwd);
    ini.setValue("forwardingNumber", forwardingNumber);
    ini.setValue("forwardingDelay", forwardingDelay);

    ini.setValue("autoHangUpTime", autoHangUpTime);
    ini.setValue("maxConcurrentCalls", maxConcurrentCalls);
    ini.setValue("callWaiting", callWaiting);

    ini.setValue("featureCodeCP", featureCodeCP);
    ini.setValue("featureCodeBT", featureCodeBT);
    ini.setValue("featureCodeAT", featureCodeAT);
    ini.setValue("enableFeatureCodeCP", enableFeatureCodeCP);
    ini.setValue("enableFeatureCodeBT", enableFeatureCodeBT);
    ini.setValue("enableFeatureCodeAT", enableFeatureCodeAT);

    ini.setValue("enableShortcuts", enableShortcuts);
    ini.setValue("shortcutsBottom", shortcutsBottom);
    ini.setValue("darkTheme", darkTheme);
    ini.setValue("activeTab", activeTab);
    if (!windowGeometry.isEmpty()) {
        ini.setValue("windowGeometry", windowGeometry.toBase64());
    }
    ini.setValue("lastCallNumber", lastCallNumber);
    ini.endGroup();

    ini.sync();
    // Protect SIP credentials in microsip.ini (owner read/write only)
    QFile::setPermissions(iniFile, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

bool Settings::loadAccount(int id, Account& out) const {
    if (id <= 0) return false;
    QSettings ini(iniFile, QSettings::IniFormat);
    QString group = QString("Account%1").arg(id);
    if (!ini.childGroups().contains(group)) {
        return false;
    }

    ini.beginGroup(group);
    out = Account{};
    out.id = id;
    out.label = ini.value("label").toString();
    out.server = ini.value("server").toString();
    out.proxy = ini.value("proxy").toString();
    out.username = ini.value("username").toString();
    out.domain = ini.value("domain").toString();
    out.port = ini.value("port", 0).toInt();
    out.authID = ini.value("authID").toString();
    out.password = ini.value("password").toString();
    out.displayName = ini.value("displayName").toString();
    out.dialingPrefix = ini.value("dialingPrefix").toString();
    out.dialPlan = ini.value("dialPlan").toString();
    out.hideCID = ini.value("hideCID").toString();
    out.voicemailNumber = ini.value("voicemailNumber").toString();
    out.srtp = ini.value("SRTP").toString();
    out.transport = ini.value("transport").toString();
    out.publicAddr = ini.value("publicAddr").toString();
    out.registerRefresh = ini.value("registerRefresh", 300).toInt();
    out.keepAlive = ini.value("keepAlive", 15).toInt();
    out.publish = ini.value("publish", false).toBool();
    out.ice = ini.value("ICE", false).toBool();
    out.allowRewrite = ini.value("allowRewrite", false).toBool();
    out.disableSessionTimer = ini.value("disableSessionTimer", false).toBool();
    ini.endGroup();

    return !out.isEmpty();
}

QVector<Account> Settings::loadAllAccounts() const {
    QVector<Account> accounts;
    QSettings ini(iniFile, QSettings::IniFormat);
    for (const QString& group : ini.childGroups()) {
        if (group.startsWith("Account")) {
            bool ok = false;
            int id = group.mid(7).toInt(&ok);
            if (ok && id > 0) {
                Account acc;
                if (loadAccount(id, acc)) {
                    accounts.push_back(acc);
                }
            }
        }
    }
    std::sort(accounts.begin(), accounts.end(), [](const Account& a, const Account& b) {
        return a.id < b.id;
    });
    return accounts;
}

int Settings::saveAccount(const Account& acc) {
    QSettings ini(iniFile, QSettings::IniFormat);
    int id = acc.id;
    if (id <= 0) {
        id = 1;
        while (ini.childGroups().contains(QString("Account%1").arg(id))) {
            ++id;
        }
    }

    QString group = QString("Account%1").arg(id);
    ini.beginGroup(group);
    ini.setValue("label", acc.label);
    ini.setValue("server", acc.server);
    ini.setValue("proxy", acc.proxy);
    ini.setValue("username", acc.username);
    ini.setValue("domain", acc.domain);
    ini.setValue("port", acc.port);
    ini.setValue("authID", acc.authID);
    ini.setValue("password", acc.password);
    ini.setValue("displayName", acc.displayName);
    ini.setValue("dialingPrefix", acc.dialingPrefix);
    ini.setValue("dialPlan", acc.dialPlan);
    ini.setValue("hideCID", acc.hideCID);
    ini.setValue("voicemailNumber", acc.voicemailNumber);
    ini.setValue("SRTP", acc.srtp);
    ini.setValue("transport", acc.transport);
    ini.setValue("publicAddr", acc.publicAddr);
    ini.setValue("registerRefresh", acc.registerRefresh);
    ini.setValue("keepAlive", acc.keepAlive);
    ini.setValue("publish", acc.publish);
    ini.setValue("ICE", acc.ice);
    ini.setValue("allowRewrite", acc.allowRewrite);
    ini.setValue("disableSessionTimer", acc.disableSessionTimer);
    ini.endGroup();
    ini.sync();
    QFile::setPermissions(iniFile, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return id;
}

void Settings::deleteAccount(int id) {
    if (id <= 0) return;
    QSettings ini(iniFile, QSettings::IniFormat);
    ini.remove(QString("Account%1").arg(id));
    ini.sync();
    if (accountId == id) {
        auto remaining = loadAllAccounts();
        accountId = remaining.isEmpty() ? 0 : remaining.first().id;
        save();
    }
}

Account Settings::activeAccount() const {
    Account acc;
    if (accountId > 0 && loadAccount(accountId, acc)) {
        return acc;
    }
    return Account{};
}

QVector<Shortcut> Settings::loadShortcuts() const {
    QVector<Shortcut> result;
    QSettings ini(iniFile, QSettings::IniFormat);
    ini.beginGroup("Shortcuts");
    int count = ini.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        QString prefix = QString("s%1_").arg(i);
        Shortcut sc;
        sc.label = ini.value(prefix + "label").toString();
        sc.number = ini.value(prefix + "number").toString();
        sc.label2 = ini.value(prefix + "label2").toString();
        sc.number2 = ini.value(prefix + "number2").toString();
        sc.type = ini.value(prefix + "type", "call").toString();
        sc.presence = ini.value(prefix + "presence", false).toBool();
        if (!sc.label.isEmpty() || !sc.number.isEmpty()) {
            result.push_back(sc);
        }
    }
    ini.endGroup();
    return result;
}

void Settings::saveShortcuts(const QVector<Shortcut>& list) {
    QSettings ini(iniFile, QSettings::IniFormat);
    ini.remove("Shortcuts");
    ini.beginGroup("Shortcuts");
    ini.setValue("count", list.size());
    for (int i = 0; i < list.size(); ++i) {
        QString prefix = QString("s%1_").arg(i);
        ini.setValue(prefix + "label", list[i].label);
        ini.setValue(prefix + "number", list[i].number);
        ini.setValue(prefix + "label2", list[i].label2);
        ini.setValue(prefix + "number2", list[i].number2);
        ini.setValue(prefix + "type", list[i].type);
        ini.setValue(prefix + "presence", list[i].presence);
    }
    ini.endGroup();
    ini.sync();
}

} // namespace msip
