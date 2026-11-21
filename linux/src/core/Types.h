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

#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <cstdint>

namespace msip {

enum class CallDirection : int {
    Outgoing = 0,
    Incoming = 1,
    Missed = 2,
    AutoEnded = 3
};

enum class PresenceStatus : int {
    Unknown = 0,
    Offline = 1,
    Online = 2,
    Away = 3,
    Busy = 4,
    OnThePhone = 5,
    Ringing = 6
};

enum class DtmfMethod : int {
    Auto = 0,
    Rfc2833 = 1,
    InBand = 2,
    SipInfo = 3
};

struct Account {
    int id = 0;
    QString label;
    QString server;
    QString proxy;
    QString username;
    QString domain;
    int port = 0;
    QString authID;
    QString password;
    QString displayName;
    QString dialingPrefix;
    QString dialPlan;
    QString hideCID;
    QString voicemailNumber;
    QString srtp;        // "", "optional", "mandatory"
    QString transport;   // "", "udp", "tcp", "tls"
    QString publicAddr;
    int registerRefresh = 300;
    int keepAlive = 15;
    bool publish = false;
    bool ice = false;
    bool allowRewrite = false;
    bool disableSessionTimer = false;

    bool isEmpty() const {
        return server.isEmpty() && domain.isEmpty() && username.isEmpty();
    }

    QString effectiveLabel() const {
        if (!label.isEmpty()) return label;
        if (!displayName.isEmpty()) return displayName;
        if (!username.isEmpty() && !domain.isEmpty()) return username + "@" + domain;
        if (!username.isEmpty()) return username;
        return server;
    }
};

struct CallRecord {
    int id = 0;
    QString callId;
    QString number;
    QString name;
    CallDirection type = CallDirection::Outgoing;
    int64_t time = 0;
    int duration = 0;
    QString info;
};

struct Contact {
    int id = 0;
    QString name;
    QString number;
    QString firstname;
    QString lastname;
    QString phone;
    QString mobile;
    QString email;
    QString address;
    QString city;
    QString state;
    QString zip;
    QString comment;
    bool presence = false;
    bool starred = false;
    QString info;

    // Runtime BLF/Presence state (not persisted)
    PresenceStatus presenceStatus = PresenceStatus::Unknown;
    bool ringing = false;
};

struct Shortcut {
    QString label;
    QString number;
    QString label2;
    QString number2;
    QString type = "call"; // call, message, dtmf, transfer, attendedTransfer, callOrBT, callOrAT, conference, callOrConference
    bool presence = false;

    // Runtime BLF state
    PresenceStatus presenceStatus = PresenceStatus::Unknown;
    bool ringing = false;
};

struct SipMessage {
    int id = 0;
    QString peerUri;
    QString peerName;
    QString body;
    int64_t timestamp = 0;
    bool incoming = false;
    QString status;
};

struct ActiveCallInfo {
    int callId = -1;
    QString sipCallId;
    QString remoteUri;
    QString remoteName;
    QString number;
    int state = 0; // pjsip_inv_state
    QString stateText;
    int duration = 0;
    bool incoming = false;
    bool onHold = false;
    bool muted = false;
    bool recording = false;
    bool inConference = false;
    bool srtpActive = false;
};

struct AudioDeviceInfo {
    int id = -1;
    QString name;
    QString displayName;
    QString driver;
    unsigned inputCount = 0;
    unsigned outputCount = 0;
    bool isAdvanced = false;
};

} // namespace msip

Q_DECLARE_METATYPE(msip::Account)
Q_DECLARE_METATYPE(msip::CallRecord)
Q_DECLARE_METATYPE(msip::Contact)
Q_DECLARE_METATYPE(msip::Shortcut)
Q_DECLARE_METATYPE(msip::SipMessage)
Q_DECLARE_METATYPE(msip::ActiveCallInfo)
Q_DECLARE_METATYPE(msip::PresenceStatus)
