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
#include "Settings.h"
#include "Database.h"

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QStringList>
#include <QMutex>
#include <pjsua-lib/pjsua.h>

namespace msip {

class SipEngine : public QObject {
    Q_OBJECT
public:
    explicit SipEngine(Settings* settings, Database* db, QObject* parent = nullptr);
    ~SipEngine() override;

    bool initialize();
    void shutdown();
    bool restart();
    bool isRunning() const { return m_running; }

    // Account registration
    bool registerActiveAccount();
    void unregisterAccount();
    bool isRegistered() const { return m_registered; }
    QString registrationStatusText() const { return m_regStatusText; }

    // Audio devices & codecs
    QVector<AudioDeviceInfo> audioDevices() const;
    void applyAudioDevices();
    void applyAudioCodecs();
    QStringList availableAudioCodecs() const;
    void setInputVolume(int percent);
    void setOutputVolume(int percent);
    void setGlobalMuteInput(bool muted);
    void setGlobalMuteOutput(bool muted);
    bool isGlobalMuteInput() const { return m_globalMuteInput; }
    bool isGlobalMuteOutput() const { return m_globalMuteOutput; }

    // Call control
    int makeCall(const QString& number);
    int callPickup(const QString& number);
    bool answerCall(int callId);
    bool answerFirstIncomingCall();
    void hangupCall(int callId);
    void rejectCall(int callId, int statusCode = 486);
    void hangupAll();
    bool toggleHold(int callId);
    bool toggleMute(int callId);
    bool sendDtmf(int callId, const QString& digits);
    void playLocalDtmfDigit(char digit);
    bool blindTransfer(int callId, const QString& destNumber);
    bool attendedTransfer(int callId, int destCallId);
    bool forwardCall(int callId, const QString& destNumber);
    void joinConference();

    // Call recording
    bool startRecording(int callId);
    void stopRecording(int callId);
    bool isRecording(int callId) const;

    // SIP Messaging (SIMPLE IM)
    bool sendMessage(const QString& number, const QString& text);

    // Presence / BLF
    void refreshSubscriptions(const QVector<Contact>& contacts, const QVector<Shortcut>& shortcuts);
    void clearSubscriptions();

    // Active calls
    QVector<ActiveCallInfo> activeCalls() const;
    int primaryCallId() const;

    // URI helpers
    QString formatSipUri(const QString& input) const;
    static QString extractNumberFromUri(const QString& uri);
    static QString extractDisplayNameFromUri(const QString& uri);
    static bool isIncomingRinging(const ActiveCallInfo& call);
    static QString localizedCallState(int state, bool incoming);

    // Ringtone
    void startRingtone();
    void stopRingtone();

signals:
    void registrationStateChanged(bool registered, int code, const QString& reason);
    void incomingCall(const msip::ActiveCallInfo& call);
    void callStateChanged(const msip::ActiveCallInfo& call);
    void callEnded(int callId, const msip::CallRecord& record);
    void activeCallsChanged();
    void recordingStateChanged(int callId, bool recording, const QString& filePath);
    void messageReceived(const msip::SipMessage& msg);
    void messageStatusChanged(const QString& peerUri, int statusCode, const QString& reason);
    void buddyStateChanged(const QString& numberOrUri, msip::PresenceStatus status, bool ringing, const QString& statusText);
    void audioLevelsUpdated(float txLevel, float rxLevel);
    void logMessage(const QString& text);

private:
    struct CallRuntime {
        ActiveCallInfo info;
        pjsua_recorder_id recorderId = PJSUA_INVALID_ID;
        QString recordingFile;
        int64_t startEpoch = 0;
        int64_t connectEpoch = 0;
        bool answered = false;
        int dbRowId = -1;
    };

    // PJSUA static callbacks
    static void onRegState2(pjsua_acc_id acc_id, pjsua_reg_info* info);
    static void onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data* rdata);
    static void onCallState(pjsua_call_id call_id, pjsip_event* e);
    static void onCallMediaState(pjsua_call_id call_id);
    static void onPager2(pjsua_call_id call_id, const pj_str_t* from, const pj_str_t* to,
                         const pj_str_t* contact, const pj_str_t* mime_type,
                         const pj_str_t* body, pjsip_rx_data* rdata, pjsua_acc_id acc_id);
    static void onPagerStatus2(pjsua_call_id call_id, const pj_str_t* to, const pj_str_t* body,
                               void* user_data, pjsip_status_code status, const pj_str_t* reason,
                               pjsip_tx_data* tdata, pjsip_rx_data* rdata, pjsua_acc_id acc_id);
    static void onBuddyState(pjsua_buddy_id buddy_id);

    // GUI-thread handlers
    void handleRegState(bool registered, int code, const QString& reason);
    void handleIncomingCall(int callId, const QString& remoteUri, const QString& sipCallId);
    void handleCallState(int callId, int state, const QString& stateText, const QString& remoteUri, const QString& sipCallId, int lastStatus, const QString& lastStatusText);
    void handleCallMediaState(int callId);
    void handlePager(const QString& fromUri, const QString& body);
    void handleBuddyState(const QString& uri, PresenceStatus status, bool ringing, const QString& statusText);

    void pollTimers();
    void subscribeUri(const QString& number);
    void stopRingtoneIfNoRinging();

    static SipEngine* s_instance;

    Settings* m_settings = nullptr;
    Database* m_db = nullptr;

    bool m_running = false;
    bool m_registered = false;
    QString m_regStatusText = "Offline";

    pjsua_acc_id m_accId = PJSUA_INVALID_ID;
    pjsua_transport_id m_udpTransport = PJSUA_INVALID_ID;
    pjsua_transport_id m_tcpTransport = PJSUA_INVALID_ID;
    pjsua_transport_id m_tlsTransport = PJSUA_INVALID_ID;

    pjsua_player_id m_ringPlayerId = PJSUA_INVALID_ID;
    pj_pool_t* m_ringPool = nullptr;
    pjmedia_port* m_ringTonePort = nullptr;
    pjsua_conf_port_id m_ringToneSlot = PJSUA_INVALID_ID;

    pj_pool_t* m_dtmfPool = nullptr;
    pjmedia_port* m_dtmfPort = nullptr;
    pjsua_conf_port_id m_dtmfSlot = PJSUA_INVALID_ID;

    bool m_globalMuteInput = false;
    bool m_globalMuteOutput = false;
    bool m_lastLevelsActive = false;

    QHash<int, CallRuntime> m_calls;
    QHash<QString, pjsua_buddy_id> m_buddies;
    QTimer* m_pollTimer = nullptr;
    QTimer* m_reconnectTimer = nullptr;
};

} // namespace msip
