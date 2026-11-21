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

#pragma once

#include "Types.h"
#include <QString>
#include <QVector>

namespace msip {

class Settings {
public:
    Settings();

    void initPaths();
    void load();
    void save();

    // Accounts management (1-based IDs matching MicroSIP INI: [Account1], [Account2], ...)
    QVector<Account> loadAllAccounts() const;
    bool loadAccount(int id, Account& out) const;
    int saveAccount(const Account& acc);
    void deleteAccount(int id);
    Account activeAccount() const;

    // Shortcuts management ([Shortcuts] section)
    QVector<Shortcut> loadShortcuts() const;
    void saveShortcuts(const QVector<Shortcut>& list);

    // Paths
    QString configDir;
    QString dataDir;
    QString iniFile;
    QString dbFile;
    QString logFile;

    // General & Account
    int accountId = 0;
    bool enableLocalAccount = false;
    bool singleMode = false;
    bool alwaysOnTop = false;
    bool startMinimized = false;
    bool enableLog = false;
    bool bringToFrontOnIncoming = true;

    // Audio
    QString ringtone;
    QString audioRingDevice;
    QString audioOutputDevice;
    QString audioInputDevice;
    QString audioCodecs = "opus/48000/2 PCMU/8000/1 PCMA/8000/1 G722/16000/1 GSM/8000/1";
    bool vad = false;
    bool ec = true;
    int volumeRing = 100;
    int volumeOutput = 100;
    int volumeInput = 100;

    // Recording
    QString recordingPath;
    QString recordingFormat = "wav";
    bool autoRecording = false;

    // Network / SIP
    bool rport = true;
    int sourcePort = 0;
    int rtpPortMin = 4000;
    int rtpPortMax = 0;
    QString stun;
    bool enableStun = false;
    DtmfMethod dtmfMethod = DtmfMethod::Auto;
    bool localDtmf = true;
    QString userAgent = "MicroSIP/3.23.5-linux";

    // Call Handling: DND / Auto-Answer / Forwarding
    bool dnd = false;
    bool aa = false;
    QString autoAnswer = "button"; // "", "all", "header", "button"
    int autoAnswerDelay = 0;
    QString autoAnswerNumber;

    bool fwd = false;
    QString forwardingNumber;
    int forwardingDelay = 0;

    int autoHangUpTime = 0;
    int maxConcurrentCalls = 8;
    bool callWaiting = true;

    // Feature Codes
    QString featureCodeCP = "**";
    QString featureCodeBT = "##";
    QString featureCodeAT = "*2";
    bool enableFeatureCodeCP = false;
    bool enableFeatureCodeBT = false;
    bool enableFeatureCodeAT = false;

    // Shortcuts / UI
    bool enableShortcuts = true;
    bool shortcutsBottom = false;
    bool darkTheme = true;
    int activeTab = 0;
    QByteArray windowGeometry;
    QString lastCallNumber;
};

} // namespace msip
